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
#define KLOC_DOMAIN "dynamic_factions"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>
#include <IniDefaults.h>
#include <HoldKey.h>

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
#include <kenshi/SaveFileSystem.h>
#include <kenshi/SaveManager.h>
#include <kenshi/gui/ForgottenGUI.h>

#include <mygui/MyGUI_InputManager.h>

#include "core/DfBrain.h"
#include "core/DfNames.h"
#include "core/DfState.h"
#include "game/DfGame.h"
#include "game/DfHire.h"
#include "game/DfRecruit.h"
#include "ui/DfWindow.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_debug = false;
    HoldKey::Key g_windowKey;
    float g_startStats = 30.0f;             // навыки основателей - по сложности
    int g_startMoney = 5000;                // деньги фракции при основании

    // Сложность: что в ini и какие навыки у основателей.
    const char* const kDifficultyKeys[] = { "easy", "medium", "hard", "veryhard" };
    const float kDifficultyStats[] = { 10.0f, 30.0f, 50.0f, 70.0f };
    const int kDifficultyCount = 4;

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
        DfHireSetDebug(g_debug);
        g_startMoney = GetPrivateProfileIntA(section, "StartMoney", 5000, ini.c_str());
        if (g_startMoney < 0)
            g_startMoney = 0;
        char key[64] = {};
        GetPrivateProfileStringA(section, "WindowKey", "CTRL+F10", key, sizeof(key), ini.c_str());
        g_windowKey = HoldKey::Parse(key);
        char difficulty[32] = {};
        GetPrivateProfileStringA(section, "Difficulty", "medium", difficulty, sizeof(difficulty), ini.c_str());
        g_startStats = kDifficultyStats[1];
        for (int i = 0; i < kDifficultyCount; ++i)
            if (_stricmp(difficulty, kDifficultyKeys[i]) == 0)
                g_startStats = kDifficultyStats[i];
    }


    // ---------------------------------------------------------------
    // Состояние текущей игры
    //
    // Файл dynamicfactions.txt - внутри сохранения, через SaveFileSystem
    // (рецепт Formations.save, 17.10.2026): saveGame пишет в его временную
    // папку writeFile(имя) ДО оригинала, loadGame читает readFile(имя) после.
    // Писать в папку сейва после SaveManager::saveGame нельзя: сохранение
    // собирается во временной папке и копируется потом (22.10: «could not
    // write»). newGame - новая игра: состояния нет. Пустое состояние на
    // первом тике = новая фракция (или принятие уже живущей - Found).
    // ---------------------------------------------------------------

    const DWORD kTickMs = 1000;             // проверка «прошёл ли час» - раз в секунду
    const DWORD kGraceMs = 10000;           // после загрузки мир дозагружается
    const char* const kStateFile = "dynamicfactions.txt";

    struct Runtime
    {
        double zeroSince;                   // с какого часа численность 0 (-1 - не 0)
        DfHireTask hire;                    // поход в бар за рекрутом
        int lastPrice;                      // самый дешёвый рекрут на последнем ходе (для окна)
        Runtime() : zeroSince(-1.0), lastPrice(-1) {}
    };

    std::vector<DfFactionState> g_states;
    std::vector<Runtime> g_runtime;
    bool g_ready = false;                   // мир этой игры поднят
    DWORD g_readyAt = 0;
    DWORD g_lastTick = 0;

    void SetStates(std::vector<DfFactionState>& states, const char* why)
    {
        for (size_t i = 0; i < g_runtime.size(); ++i)
            DfHireCancel(g_runtime[i].hire);
        DfSetOwnFactions(std::vector<Faction*>());
        g_states.swap(states);
        g_runtime.assign(g_states.size(), Runtime());
        if (g_debug)
        {
            char line[128];
            sprintf_s(line, sizeof(line), "DynamicFactions: %d faction state(s) (%s)",
                      static_cast<int>(g_states.size()), why);
            DebugLog(line);
        }
    }

    void MarkReady(const char* why)
    {
        g_ready = true;
        g_readyAt = GetTickCount();
        if (g_debug)
            DebugLog(std::string("DynamicFactions: world ready (") + why + ")");
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

    bool WriteFile(const std::string& path, const std::string& text)
    {
        std::ofstream f(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!f)
            return false;
        f.write(text.data(), static_cast<std::streamsize>(text.size()));
        return !f.fail();
    }

    // Словарь названий на языке игры: locale/<язык>/LC_MESSAGES/
    // dynamic_factions_names.txt. Нет файла - английские названия.
    const DfNameDict& NameDict()
    {
        static bool loaded = false;
        static DfNameDict dict;
        if (loaded)
            return dict;
        loaded = true;
        KLoc::EnsureLoaded();
        const std::string base = KLoc::FindLocaleBase();
        std::vector<std::string> ids;
        KLoc::AddCandidates(ids, KLoc::LoadedLocaleId());
        for (size_t i = 0; i < ids.size() && !base.empty(); ++i)
        {
            const std::string path = KLoc::Join(KLoc::Join(base, ids[i]), "LC_MESSAGES\\dynamic_factions_names.txt");
            std::string text;
            if (!ReadFile(path, text))
                continue;
            if (DfParseNameDict(text, dict))
                DebugLog("DynamicFactions: faction names from " + path);
            else
                ErrorLog("DynamicFactions: " + path + " has a bad line - English names");
            break;
        }
        return dict;
    }

    DfFactionState FreshFaction()
    {
        std::vector<std::string> taken;
        DfGameFactionNames(taken);
        DfFactionState s;
        s.recordId = kDfFactionRecord;
        s.race = "human";
        s.name = DfMakeNameFrom(NameDict(), s.race, GetTickCount(), taken);
        return s;
    }

    // ---------------------------------------------------------------
    // Тик: раз в секунду смотрим, не прошёл ли игровой час
    // ---------------------------------------------------------------

    // Основание. Если у записи фракции в игре уже есть живые люди (сейв
    // без нашего файла, другой плагин и т.п.) - фракция принимается как
    // есть, без нового отряда и денег: 22.10 после загрузки сейва без файла
    // получился второй отряд.
    void Found(DfFactionState& s, Faction* f, double hour)
    {
        TownBase* const hub = DfFindHub();
        if (hub == NULL || f == NULL)
            return;
        DfRules rules = DfPhase1Rules();
        rules.startMoney = g_startMoney;
        DfSnapshot before;
        if (!DfTakeSnapshot(f, before))
            return;
        std::vector<DfAction> done(1);
        done[0].type = DF_FOUND;
        done[0].amount = rules.startMoney;
        if (before.members > 0)
        {
            DfApplyBookkeeping(s, done, hour, rules);
            DfApplyName(f, s.name);
            char line[256];
            sprintf_s(line, sizeof(line), "DynamicFactions: %s took over the faction already in the game, %d members",
                      Quote(s.name).c_str(), before.members);
            DebugLog(line);
            return;
        }
        DfApplyName(f, s.name);
        if (!DfFound(f, hub, rules.startMembers, rules.startMoney, g_startStats))
            return;
        DfApplyBookkeeping(s, done, hour, rules);
        DfSnapshot snap;
        const int members = DfTakeSnapshot(f, snap) ? snap.members : -1;
        char line[256];
        sprintf_s(line, sizeof(line), "DynamicFactions: founded %s in the Hub, %d members, skills %.0f",
                  Quote(s.name).c_str(), members, g_startStats);
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

        // Кого можно нанять в баре дома и почём - для решения ядра. Идёт
        // поход за рекрутом - нового не начинаем, его цена отложена.
        TownBase* const hub = snap.homeLoaded ? DfFindHub() : NULL;
        Character* recruit = NULL;
        if (rt.hire.busy())
        {
            snap.recruitPrice = -1;
            snap.money -= rt.hire.price;
        }
        else if (hub == NULL || !DfFindBarRecruit(hub, f, recruit, snap.recruitPrice))
            snap.recruitPrice = -1;
        rt.lastPrice = snap.recruitPrice;
        if (g_debug && hub != NULL && !rt.hire.busy())
        {
            const DfRecruitSearchStats& st = DfLastRecruitSearch();
            char line[256];
            sprintf_s(line, sizeof(line), "DynamicFactions: bar recruits - %d templates, %d such people in town, "
                      "%d of them in a bar, %d can be hired; cheapest %d: ",
                      st.templates, st.generic, st.inBar, st.recruitable, snap.recruitPrice);
            DebugLog(line + st.list);
        }

        const DfRules rules = DfPhase1Rules();
        const std::vector<DfAction> acts = DfDecide(s, snap, rules);
        std::vector<DfAction> done;
        for (size_t i = 0; i < acts.size(); ++i)
        {
            if (acts[i].type == DF_RECRUIT)
            {
                if (DfHireStart(rt.hire, f, hub, acts[i].amount))
                {
                    char line[256];
                    sprintf_s(line, sizeof(line), "DynamicFactions: %s sends someone to a bar of the Hub to hire a recruit for %d",
                              Quote(s.name).c_str(), rt.hire.price);
                    DebugLog(line);
                }
                continue;
            }
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
        if (s.extinct)
            DfHireCancel(rt.hire);

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

    // Окно: живые числа каждого состояния (не только на часовом тике).
    void UpdateWindow()
    {
        std::vector<DfSnapshot> snaps(g_states.size());
        for (size_t i = 0; i < g_states.size(); ++i)
        {
            snaps[i].members = -1;
            snaps[i].money = 0;
            snaps[i].recruitPrice = i < g_runtime.size() && !g_runtime[i].hire.busy() ? g_runtime[i].lastPrice : -1;
            Faction* const f = g_states[i].founded ? DfBindFaction(g_states[i].recordId) : NULL;
            if (f != NULL && !DfTakeSnapshot(f, snaps[i]))
                snaps[i].members = -1;
        }
        std::vector<std::string> hires(g_states.size());
        for (size_t i = 0; i < g_states.size() && i < g_runtime.size(); ++i)
            hires[i] = DfHireStatus(g_runtime[i].hire);
        DfWindowUpdate(g_states, snaps, hires, DfPhase1Rules());
    }

    // Хоткей - ровно эта комбинация: CTRL+F10 не срабатывает на CTRL+SHIFT+F10.
    bool Exact(const HoldKey::Key& k)
    {
        if (k.vk == 0 || !HoldKey::Held(k))
            return false;
        if (!k.ctrl && k.vk != VK_CONTROL && HoldKey::Down(VK_CONTROL))
            return false;
        if (!k.shift && k.vk != VK_SHIFT && HoldKey::Down(VK_SHIFT))
            return false;
        if (!k.alt && k.vk != VK_MENU && HoldKey::Down(VK_MENU))
            return false;
        return true;
    }

    bool g_keyWas = false;

    void PollKey()
    {
        // Печатает игрок в поле - клавиша не наша.
        const bool typing = MyGUI::InputManager::getInstance().getKeyFocusWidget() != NULL;
        const bool down = !typing && Exact(g_windowKey);
        if (down && !g_keyWas)
        {
            DfWindowToggle();
            g_lastTick = 0;                 // окно сразу с числами
        }
        g_keyWas = down;
    }

    // Поход за рекрутом - каждую секунду, не только на часовом ходе.
    void StepHire(DfFactionState& s, Runtime& rt, double hour)
    {
        if (!rt.hire.busy())
            return;
        Faction* const f = DfBindFaction(s.recordId);
        const DfHireResult r = f != NULL ? DfHireStep(rt.hire, f, s.name) : DF_HIRE_FAILED;
        if (r == DF_HIRE_RUNNING)
            return;
        char line[256];
        if (r == DF_HIRE_DONE)
        {
            DfAddEvent(s, hour, "recruit");
            sprintf_s(line, sizeof(line), "DynamicFactions: %s hired a recruit in a bar of the Hub for %d",
                      Quote(s.name).c_str(), rt.hire.price);
        }
        else
        {
            DfHireCancel(rt.hire);
            sprintf_s(line, sizeof(line), "DynamicFactions: %s - the hire fell through: %s",
                      Quote(s.name).c_str(), rt.hire.why.c_str());
        }
        DebugLog(line);
    }

    void Tick()
    {
        if (!g_ready || ou == NULL || ou->player == NULL)
            return;
        if (GetTickCount() - g_readyAt < kGraceMs)
            return;
        if (g_states.empty())
        {
            std::vector<DfFactionState> fresh(1, FreshFaction());
            SetStates(fresh, "new faction");
            DebugLog("DynamicFactions: new faction " + Quote(g_states[0].name) + " waits for the Hub");
        }
        const double hour = DfGameHour();
        if (hour < 0.0)
            return;
        // Наши фракции - для блока перенайма (DfInstallHireBlock).
        std::vector<Faction*> own;
        for (size_t i = 0; i < g_states.size(); ++i)
            if (g_states[i].founded && !g_states[i].extinct)
                if (Faction* const f = DfBindFaction(g_states[i].recordId))
                    own.push_back(f);
        DfSetOwnFactions(own);
        for (size_t i = 0; i < g_states.size(); ++i)
        {
            TickFaction(g_states[i], g_runtime[i], hour);
            StepHire(g_states[i], g_runtime[i], hour);
        }
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;
    bool (*g_origSfsSave)(SaveFileSystem*, const std::string&) = NULL;
    void (*g_origSfsLoad)(SaveFileSystem*, const std::string&) = NULL;
    void (*g_origSfsNew)(SaveFileSystem*) = NULL;
    int (*g_origLoadGame)(SaveManager*, const std::string&, const std::string&) = NULL;
    void (*g_origResetGame)(GameWorld*) = NULL;
    void (*g_origInitNewWorld)(GameWorld*, GameDataManager*) = NULL;

    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        if (ou == NULL || ou->player == NULL)
            return;
        PollKey();
        DfWindowFrame();
        const DWORD now = GetTickCount();
        if (now - g_lastTick < kTickMs)
            return;
        g_lastTick = now;
        Tick();
        if (DfWindowIsOpen())
            UpdateWindow();
    }

    void WriteState(SaveFileSystem* sfs)
    {
        if (!g_ready)
            return;
        const std::string path = sfs->writeFile(kStateFile);
        if (path.empty() || !WriteFile(path, DfSerialize(g_states)))
            ErrorLog("DynamicFactions: could not write the faction state (" + path + ")");
        else if (g_debug)
            DebugLog("DynamicFactions: state written to " + path);
    }

    void ReadState(SaveFileSystem* sfs)
    {
        std::vector<DfFactionState> loaded;
        if (!sfs->fileExists(kStateFile))
        {
            SetStates(loaded, "this save has no faction state");
            return;
        }
        const std::string path = sfs->readFile(kStateFile);
        std::string text;
        if (!ReadFile(path, text) || !DfDeserialize(text, loaded))
        {
            ErrorLog("DynamicFactions: " + path + " is damaged - the faction is taken over from the game");
            loaded.clear();
        }
        SetStates(loaded, "loaded");
        for (size_t i = 0; i < g_states.size(); ++i)
            DebugLog("DynamicFactions: restored " + Quote(g_states[i].name) +
                     (g_states[i].extinct ? " (extinct)" : g_states[i].founded ? "" : " (not founded yet)"));
    }

    bool SfsSave_hook(SaveFileSystem* self, const std::string& path)
    {
        __try
        {
            WriteState(self);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return g_origSfsSave(self, path);
    }

    void SfsLoad_hook(SaveFileSystem* self, const std::string& path)
    {
        g_origSfsLoad(self, path);
        __try
        {
            ReadState(self);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("DynamicFactions: reading the faction state failed - the faction is taken over from the game");
        }
    }

    void SfsNew_hook(SaveFileSystem* self)
    {
        g_origSfsNew(self);
        std::vector<DfFactionState> none;
        SetStates(none, "new game");
    }

    // Мир поднят: после загрузки сейва и после создания мира новой игры.
    int LoadGame_hook(SaveManager* self, const std::string& location, const std::string& name)
    {
        g_ready = false;
        const int result = g_origLoadGame(self, location, name);
        MarkReady("save loaded");
        return result;
    }

    void ResetGame_hook(GameWorld* self)
    {
        g_ready = false;
        if (g_debug)
            DebugLog("DynamicFactions: world reset");
        g_origResetGame(self);
    }

    void InitNewWorld_hook(GameWorld* self, GameDataManager* data)
    {
        g_origInitNewWorld(self, data);
        MarkReady("world initialised");
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "DynamicFactions", "Dynamic Factions", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Factions that live on their own: a new faction with its own name appears in the Hub, earns money, recruits and grows in tech. When all its members are dead it is gone."));
    api->section(api, Tr("General"));
    api->toggle(api, "DynamicFactions", "Enabled", Tr("Enabled"), Tr("The whole mod."), 1, MCM_RESTART);
    static const char* labels[kDifficultyCount] = {};
    labels[0] = Tr("Easy - skills 10");
    labels[1] = Tr("Medium - skills 30");
    labels[2] = Tr("Hard - skills 50");
    labels[3] = Tr("Very hard - skills 70");
    api->choice(api, "DynamicFactions", "Difficulty", Tr("Difficulty"),
                Tr("Skills of the founders of a new faction: every skill starts at this level. Applies to factions founded after the change."),
                "medium", kDifficultyKeys, labels, kDifficultyCount, 0);
    api->integer(api, "DynamicFactions", "StartMoney", Tr("Start money"),
                 Tr("Money of a new faction when it is founded. Recruits in the Hub ask 3000-6000; until stage 2 the faction only earns 15 per member an hour. Applies to factions founded after the change."),
                 5000, 0, 100000, 0);
    api->hotkey(api, "DynamicFactions", "WindowKey", Tr("Faction window key"),
                Tr("Opens and closes the window with the state of the factions. Click and press a key; Backspace - no key."), "CTRL+F10", 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "DynamicFactions", "Debug", Tr("Detailed log"),
                Tr("Details in RE_Kenshi_log.txt: every hourly tick of a faction, saving and loading."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    if (!g_enabled)
    {
        DebugLog("DynamicFactions: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::saveGame),
                                                 SfsSave_hook, &g_origSfsSave) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),
                                                 SfsLoad_hook, &g_origSfsLoad) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),
                                                 SfsNew_hook, &g_origSfsNew) ||
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

    if (!DfInstallHireBlock())
        ErrorLog("DynamicFactions: could not hook dialogue conditions - the player can re-hire faction members");

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                                                 GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("DynamicFactions: could not hook the interface update - the mod stays off");
        return;
    }

    DebugLog("DynamicFactions: installed");
}
