// DynamicFactions - фракции, которые живут сами «как игрок» (22.10.2026).
//
// Этап 1: одна фракция людей в Хабе с уникальным названием. Копит деньги,
// нанимает, растёт в технике; все члены убиты - вымерла, её код больше не
// выполняется. Подробно - docs/superpowers/specs/2026-10-22-dynamic-factions-design.md.
//
// Слои: core/ - решения без игры (тестируются отдельно, tests/), game/ -
// адаптер к игре, этот файл - хуки и тик. Объекты игры трогаем только из
// главного потока (ForgottenGUI::update), каждое обращение - под SEH.
//
// Своё состояние (имя, техника, журнал событий) - файл dynamicfactions.txt
// в папке сохранения: пишется после SaveManager::saveGame, читается после
// loadGame. Деньги и люди - у игры (они в её сохранении).

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <IniDefaults.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Faction.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/SaveManager.h>
#include <kenshi/gui/ForgottenGUI.h>

#include "core/DfBrain.h"
#include "core/DfNames.h"
#include "core/DfState.h"
#include "game/DfGame.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_debug = false;

    std::string IniFileName()
    {
        char path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();
        std::string file(path, length);
        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string() : file.substr(0, dot) + ".ini";
    }

    // Путь к ini игрока; при первом обращении - из DynamicFactions.defaults.ini.
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
    }

    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        const char* const section = "DynamicFactions";
        g_enabled = GetPrivateProfileIntA(section, "Enabled", 1, ini.c_str()) != 0;
        g_debug = GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
    }


    // ---------------------------------------------------------------
    // Состояние текущей игры
    // ---------------------------------------------------------------

    const DWORD kTickMs = 1000;             // проверка «прошёл ли час» - раз в секунду
    const DWORD kGraceMs = 10000;           // после загрузки мир дозагружается

    struct Runtime
    {
        double zeroSince;                   // с какого часа численность 0 (-1 - не 0)
        Runtime() : zeroSince(-1.0) {}
    };

    std::vector<DfFactionState> g_states;
    std::vector<Runtime> g_runtime;
    bool g_ready = false;                   // состояние этой игры загружено
    bool g_needFresh = false;               // новая игра - фракцию заводим на первом тике
    DWORD g_readyAt = 0;
    DWORD g_lastTick = 0;

    void Reset(const char* why)
    {
        g_states.clear();
        g_runtime.clear();
        g_ready = false;
        g_needFresh = false;
        if (g_debug)
            DebugLog(std::string("DynamicFactions: state cleared (") + why + ")");
    }

    void MarkReady()
    {
        g_runtime.assign(g_states.size(), Runtime());
        g_ready = true;
        g_readyAt = GetTickCount();
    }

    DfFactionState FreshFaction()
    {
        std::vector<std::string> taken;
        DfGameFactionNames(taken);
        DfFactionState s;
        s.recordId = kDfFactionRecord;
        s.race = "human";
        s.name = DfMakeName(s.race, GetTickCount(), taken);
        return s;
    }

    std::string Quote(const std::string& s)
    {
        return "\"" + s + "\"";
    }

    bool ReadFile(const std::string& path, std::string& out)
    {
        std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
        if (!f)
            return false;
        std::ostringstream buf;
        buf << f.rdbuf();
        out = buf.str();
        return true;
    }

    // Во временный файл, потом подмена: оборванная запись не портит прежний.
    bool WriteFile(const std::string& path, const std::string& text)
    {
        const std::string tmp = path + ".tmp";
        {
            std::ofstream f(tmp.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (!f)
                return false;
            f.write(text.data(), static_cast<std::streamsize>(text.size()));
            if (!f)
                return false;
        }
        return MoveFileExA(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
    }

    void LoadStateFor(const std::string& location, const std::string& name)
    {
        const std::string path = DfStatePath(location, name);
        std::string text;
        std::vector<DfFactionState> loaded;
        if (!path.empty() && ReadFile(path, text))
        {
            if (DfDeserialize(text, loaded) && !loaded.empty())
            {
                g_states.swap(loaded);
                for (size_t i = 0; i < g_states.size(); ++i)
                    DebugLog("DynamicFactions: restored " + Quote(g_states[i].name) +
                             (g_states[i].extinct ? " (extinct)" : g_states[i].founded ? "" : " (not founded yet)"));
                MarkReady();
                return;
            }
            ErrorLog("DynamicFactions: " + path + " is damaged - a new faction is started");
        }
        g_states.assign(1, FreshFaction());
        DebugLog("DynamicFactions: no saved state - new faction " + Quote(g_states[0].name) + " waits for the Hub");
        MarkReady();
    }

    void SaveStateTo(const std::string& location, const std::string& name)
    {
        if (!g_ready)
            return;
        const std::string path = DfStatePath(location, name);
        if (path.empty() || !WriteFile(path, DfSerialize(g_states)))
            ErrorLog("DynamicFactions: could not write " + path);
        else if (g_debug)
            DebugLog("DynamicFactions: state written to " + path);
    }


    // ---------------------------------------------------------------
    // Тик: раз в секунду смотрим, не прошёл ли игровой час
    // ---------------------------------------------------------------

    void Found(DfFactionState& s, Faction* f, double hour)
    {
        TownBase* const hub = DfFindHub();
        if (hub == NULL || f == NULL)
            return;
        const DfRules rules = DfPhase1Rules();
        DfApplyName(f, s.name);
        if (!DfFound(f, hub, rules.startMembers, rules.startMoney))
            return;
        std::vector<DfAction> done(1);
        done[0].type = DF_FOUND;
        done[0].amount = rules.startMoney;
        DfApplyBookkeeping(s, done, hour, rules);
        DfSnapshot snap;
        const int members = DfTakeSnapshot(f, snap) ? snap.members : -1;
        char line[256];
        sprintf_s(line, sizeof(line), "DynamicFactions: founded %s in the Hub, %d members", Quote(s.name).c_str(), members);
        DebugLog(line);
    }

    void TickFaction(DfFactionState& s, Runtime& rt, double hour)
    {
        if (s.extinct)
            return;

        Faction* const f = DfBindFaction(s.recordId);
        if (f == NULL)
            return;

        if (!s.founded)
        {
            Found(s, f, hour);
            return;
        }
        if (hour - s.lastTickHour < 1.0)
            return;

        DfApplyName(f, s.name);
        DfSnapshot snap;
        if (!DfTakeSnapshot(f, snap))
            return;

        // Численность 0 - вымирание, но только если она держится час: сразу
        // после загрузки отряды игры ещё могут не быть подняты.
        if (snap.members <= 0)
        {
            if (rt.zeroSince < 0.0)
                rt.zeroSince = hour;
            if (hour - rt.zeroSince < 1.0)
                return;
        }
        else
            rt.zeroSince = -1.0;

        const DfRules rules = DfPhase1Rules();
        const std::vector<DfAction> acts = DfDecide(s, snap, rules);
        TownBase* const hub = snap.homeLoaded ? DfFindHub() : NULL;
        std::vector<DfAction> done;
        for (size_t i = 0; i < acts.size(); ++i)
        {
            if (DfExecute(f, hub, acts[i], rules))
                done.push_back(acts[i]);
            else if (g_debug)
            {
                char line[128];
                sprintf_s(line, sizeof(line), "DynamicFactions: action %d (%d) not carried out",
                          static_cast<int>(acts[i].type), acts[i].amount);
                DebugLog(line);
            }
        }
        DfApplyBookkeeping(s, done, hour, rules);
        for (size_t i = 0; i < done.size(); ++i)
            if (done[i].type == DF_RECRUIT)
                DebugLog("DynamicFactions: " + Quote(s.name) + " recruited a new member");

        if (s.extinct)
            DebugLog("DynamicFactions: " + Quote(s.name) + " is extinct - all members are dead");
        else if (g_debug)
        {
            char line[256];
            sprintf_s(line, sizeof(line), "DynamicFactions: tick %s hour %.1f members %d money %d tech %d/%d",
                      Quote(s.name).c_str(), hour, snap.members, snap.money, s.techLevel, s.techPoints);
            DebugLog(line);
        }
    }

    void Tick()
    {
        if (!g_ready || ou == NULL || ou->player == NULL)
            return;
        if (GetTickCount() - g_readyAt < kGraceMs)
            return;
        if (g_needFresh)
        {
            g_needFresh = false;
            g_states.assign(1, FreshFaction());
            g_runtime.assign(1, Runtime());
            DebugLog("DynamicFactions: new game - new faction " + Quote(g_states[0].name) + " waits for the Hub");
        }
        const double hour = DfGameHour();
        if (hour < 0.0)
            return;
        for (size_t i = 0; i < g_states.size(); ++i)
            TickFaction(g_states[i], g_runtime[i], hour);
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;
    int (*g_origSaveGame)(SaveManager*, const std::string&, const std::string&) = NULL;
    int (*g_origLoadGame)(SaveManager*, const std::string&, const std::string&) = NULL;
    void (*g_origResetGame)(GameWorld*) = NULL;
    void (*g_origInitNewWorld)(GameWorld*, GameDataManager*) = NULL;

    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        const DWORD now = GetTickCount();
        if (now - g_lastTick < kTickMs)
            return;
        g_lastTick = now;
        Tick();
    }

    int SaveGame_hook(SaveManager* self, const std::string& location, const std::string& name)
    {
        const int result = g_origSaveGame(self, location, name);
        if (g_debug)
        {
            char line[64];
            sprintf_s(line, sizeof(line), " (result %d)", result);
            DebugLog("DynamicFactions: saveGame " + location + " | " + name + line);
        }
        SaveStateTo(location, name);
        return result;
    }

    int LoadGame_hook(SaveManager* self, const std::string& location, const std::string& name)
    {
        Reset("loading a save");
        const int result = g_origLoadGame(self, location, name);
        if (g_debug)
        {
            char line[64];
            sprintf_s(line, sizeof(line), " (result %d)", result);
            DebugLog("DynamicFactions: loadGame " + location + " | " + name + line);
        }
        LoadStateFor(location, name);
        return result;
    }

    void ResetGame_hook(GameWorld* self)
    {
        Reset("world reset");
        g_origResetGame(self);
    }

    // Новая игра. Загрузка сейва, если тоже идёт сюда, после себя всё равно
    // читает файл в LoadGame_hook - он и победит.
    void InitNewWorld_hook(GameWorld* self, GameDataManager* data)
    {
        g_origInitNewWorld(self, data);
        if (g_debug)
            DebugLog("DynamicFactions: new game world initialised");
        g_states.clear();
        g_needFresh = true;
        MarkReady();
    }
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    if (!g_enabled)
    {
        DebugLog("DynamicFactions: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::saveGame),
                                                 SaveGame_hook, &g_origSaveGame) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::loadGame),
                                                 LoadGame_hook, &g_origLoadGame) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::resetGame),
                                                 ResetGame_hook, &g_origResetGame) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::initialiseNewGameWorld),
                                                 InitNewWorld_hook, &g_origInitNewWorld))
    {
        ErrorLog("DynamicFactions: could not hook saving and loading - the mod stays off");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                                                 GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("DynamicFactions: could not hook the interface update - the mod stays off");
        return;
    }

    DebugLog("DynamicFactions: installed");
}
