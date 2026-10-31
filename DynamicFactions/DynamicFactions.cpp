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
// внутри сохранения через SaveFileSystem (writeFile при сохранении,
// readFile при загрузке). Деньги и люди - у игры (они в её сохранении).

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

#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Town.h>
#include <kenshi/SaveManager.h>
#include <kenshi/gui/ForgottenGUI.h>

#include <mygui/MyGUI_InputManager.h>

#include "core/DfBrain.h"
#include "core/DfNames.h"
#include "game/DfHunger.h"
#include "core/DfState.h"
#include "game/DfGame.h"
#include "game/DfHire.h"
#include "game/DfLabour.h"
#include "game/DfMine.h"
#include "game/DfSell.h"
#include "game/DfRecruit.h"
#include "ui/DfWindow.h"
#include "ui/DfWorkInfo.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_debug = false;
    bool g_workEnabled = true;
    int g_mineRadius = 3000;
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
        g_workEnabled = GetPrivateProfileIntA(section, "Work", 1, ini.c_str()) != 0;
        g_mineRadius = GetPrivateProfileIntA(section, "MineRadius", 3000, ini.c_str());
        if (g_mineRadius < 500) g_mineRadius = 500;
        if (g_mineRadius > 5000) g_mineRadius = 5000;
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
        bool treasuryRestoreFailed;
        double zeroSince;                   // с какого часа численность 0 (-1 - не 0)
        DfHireTask hire;                    // поход в бар за рекрутом
        DfLabour labour;                    // работа: жилы, копатели, носильщики
        int lastPrice;                      // самый дешёвый рекрут на последнем ходе (для окна)
        Runtime() : treasuryRestoreFailed(false), zeroSince(-1.0), lastPrice(-1) {}
    };

    std::vector<DfFactionState> g_states;
    std::vector<Runtime> g_runtime;
    bool g_ready = false;                   // мир этой игры поднят
    DWORD g_readyAt = 0;
    DWORD g_lastTick = 0;
    volatile LONG g_saveDepth = 0;
    bool g_restoreTreasury = false;
    enum SessionSource { SESSION_NONE, SESSION_LOAD, SESSION_NEW };
    SessionSource g_completedSfs = SESSION_NONE;

    void UpdateOwnFactions();
    void BeginSession(const char* why);
    void RestoreTreasuries();

    bool WorkInfoManaged(Character* c)
    {
        return g_ready && c!=NULL && DfIsOwnFaction(c->getFaction());
    }


    void SetStates(std::vector<DfFactionState>& states, const char* why)
    {
        // Replacement is metadata only: native objects may already belong to
        // another world. Never cancel orders by resolving a previous handle.
        DfSetOwnFactions(std::vector<Faction*>());
        g_states.swap(states);
        g_runtime.assign(g_states.size(), Runtime());
        UpdateOwnFactions();
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
        if (g_ready) return;
        RestoreTreasuries();
        g_ready = true;
        g_readyAt = GetTickCount();
        UpdateOwnFactions();
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
        DfRules rules = DfPhase2Rules();
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
            UpdateOwnFactions();
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
        UpdateOwnFactions();
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
        // Название - каждую секунду (дёшево, ставится только при расхождении):
        // после загрузки не ждать игрового часа (ревью 23.10).
        DfApplyName(f, s.name);
        if (hour - s.lastTickHour < 1.0)
            return;

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

        const DfRules rules = DfPhase2Rules();
        const std::vector<DfAction> acts = DfDecide(s, snap, rules);
        std::vector<DfAction> done;
        for (size_t i = 0; i < acts.size(); ++i)
        {
            if (acts[i].type == DF_RECRUIT)
            {
                if (DfHireStart(rt.hire, f, hub, acts[i].amount, recruit, snap.recruitPrice))
                {
                    char line[256];
                    sprintf_s(line, sizeof(line), "DynamicFactions: %s sends someone to hire a recruit in the Hub for %d",
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
        std::vector<std::string> work(g_states.size());
        for (size_t i = 0; i < g_states.size() && i < g_runtime.size(); ++i)
            work[i] = DfTransferFaulted() ? Tr("Work paused: a transfer needs reconciliation.") : DfLabourStatus(g_runtime[i].labour);
        DfWindowUpdate(g_states, snaps, hires, work, DfPhase2Rules());
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

    // ---------------------------------------------------------------
    // Работа (этап 2а): раз в 5 с по часам найма (на паузе стоят)
    // ---------------------------------------------------------------

    const unsigned long kWorkEveryMs = 5000;
    const unsigned long kTraderCheckMs = 60000;
    unsigned long g_workAt = 0;
    unsigned long g_traderCheckedAt = 0;
    bool g_traderHasMoney = false;

    void BeginSession(const char* why)
    {
        // Must run BEFORE native teardown/loading. Clearing these containers
        // does not dereference Character/Faction pointers or resolve any hand.
        g_ready = false;
        g_restoreTreasury = false;
        g_completedSfs = SESSION_NONE;
        g_readyAt = g_lastTick = 0;
        DfRecruitResetSession();
        g_runtime.clear();
        g_states.clear();
        DfHireResetClock();
        DfResetTransferFault();
        g_workAt = g_traderCheckedAt = 0;
        g_traderHasMoney = false;
        g_keyWas = false;
        if (g_debug)
            DebugLog(std::string("DynamicFactions: session cleared (") + why + ")");
    }

    bool SafeLabourStep(DfFactionState& s, DfLabour& rt, Faction* f, TownBase* hub, Character* envoy, double hour,
                        unsigned long now, int committed)
    {
        __try
        {
            DfLabourStep(s, rt, f, hub, envoy, g_traderHasMoney, hour, now, g_debug,
                static_cast<float>(g_mineRadius),committed,g_workEnabled);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("DynamicFactions: work step fault - no automatic retry");
            return false;
        }
    }

    void WorkStep(double hour)
    {
        const unsigned long now = DfHireNow();
        if (g_workAt != 0 && now - g_workAt < kWorkEveryMs)
            return;
        g_workAt = now;
        TownBase* const hub = DfFindHub();
        for (size_t i = 0; i < g_states.size(); ++i)
        {
            DfFactionState& s = g_states[i];
            if (!s.founded || s.extinct)
                continue;
            Faction* const f = DfBindFaction(s.recordId);
            if (f == NULL)
                continue;
            Character* const envoy = g_runtime[i].hire.busy() ? g_runtime[i].hire.envoy.getCharacter() : NULL;
            const int committed=g_runtime[i].hire.busy() ? g_runtime[i].hire.price : 0;
            if (!SafeLabourStep(s, g_runtime[i].labour, f, hub, envoy, hour, now, committed))
            {
                static bool told = false;
                if (!told)
                {
                    told = true;
                    ErrorLog("DynamicFactions: a work step failed once - the faction skips it");
                }
            }
        }
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
            sprintf_s(line, sizeof(line), "DynamicFactions: %s hired a recruit in the Hub for %d",
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

    // Наши фракции - для блока перенайма (DfInstallHireBlock): как только мир
    // готов, без ожидания 10 с (ревью этапа 1: после загрузки было окно для
    // перенайма).
    void UpdateOwnFactions()
    {
        std::vector<Faction*> own;
        for (size_t i = 0; i < g_states.size(); ++i)
            if (g_states[i].founded && !g_states[i].extinct)
                if (Faction* const f = DfBindFaction(g_states[i].recordId))
                    own.push_back(f);
        DfSetOwnFactions(own);
    }

    void Tick()
    {
        if (!g_ready || ou == NULL || ou->player == NULL)
            return;
        UpdateOwnFactions();
        if (ou->isPaused() || g_saveDepth != 0) return;
        if (GetTickCount() - g_readyAt < kGraceMs)
            return;
        if (g_states.empty())
        {
            std::vector<DfFactionState> fresh(1, FreshFaction());
            SetStates(fresh, "new faction");
            DebugLog("DynamicFactions: new faction " + Quote(g_states[0].name) + " waits for the Hub");
        }
        if (DfTransferFaulted())
        { for (size_t i=0;i<g_states.size();++i) g_states[i].workFault=true; return; }
        const double hour = DfGameHour();
        if (hour < 0.0)
            return;
        for (size_t i = 0; i < g_states.size(); ++i)
        {
            TickFaction(g_states[i], g_runtime[i], hour);
            StepHire(g_states[i], g_runtime[i], hour);
        }
        WorkStep(hour);
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;
    bool (*g_origSfsSave)(SaveFileSystem*, const std::string&) = NULL;
    void (*g_origSfsLoad)(SaveFileSystem*, const std::string&) = NULL;
    void (*g_origSfsNew)(SaveFileSystem*) = NULL;
    int (*g_origSaveManagerGame)(SaveManager*, const std::string&, const std::string&) = NULL;
    int (*g_origLoadGame)(SaveManager*, const std::string&, const std::string&) = NULL;
    void (*g_origResetGame)(GameWorld*) = NULL;
    void (*g_origInitNewWorld)(GameWorld*, GameDataManager*) = NULL;

    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        if (!g_ready || ou == NULL || ou->player == NULL)
            return;
        DfHireClock(ou->isPaused());
        PollKey();
        DfWindowFrame();
        const DWORD now = GetTickCount();
        if (now - g_lastTick < kTickMs)
            return;
        g_lastTick = now;
        Tick();
        if (g_ready && DfWindowIsOpen())
            UpdateWindow();
    }

    void RestoreTreasuries()
    {
        if (!g_restoreTreasury) return;
        g_restoreTreasury = false; // Consume before writes: no repeated credit within this session.
        for (size_t i=0;i<g_states.size();++i)
        {
            DfFactionState& s=g_states[i];
            if (!s.founded) continue;
            if (s.treasury < 0)
            {
                ErrorLog("DynamicFactions: legacy save has no treasury snapshot; native balance kept");
                continue;
            }
            if (!DfRestoreTreasury(DfBindFaction(s.recordId),s.treasury))
            {
                g_runtime[i].treasuryRestoreFailed=true;
                s.workFault=true;
                DfQuarantineTransfer("DynamicFactions: saved treasury could not be restored - economy stopped");
            }
            else if (g_debug)
            {
                char line[128];
                sprintf_s(line,sizeof(line),"DynamicFactions: treasury restored exactly %d",s.treasury);
                DebugLog(line);
            }
        }
    }

    void WriteState(SaveFileSystem* sfs)
    {
        if (!g_ready)
            return;
        if (DfTransferFaulted())
            for (size_t i=0;i<g_states.size();++i) g_states[i].workFault=true;
        for (size_t i=0;i<g_states.size();++i)
        {
            DfFactionState& s=g_states[i];
            if (!s.founded) continue;
            if (!DfCaptureTreasury(DfBindFaction(s.recordId),g_runtime[i].treasuryRestoreFailed,s.treasury))
            {
                s.workFault=true;
                DfQuarantineTransfer("DynamicFactions: could not capture treasury for saving");
                return;
            }
        }
        const std::string path = sfs->writeFile(kStateFile);
        if (path.empty() || !WriteFile(path, DfSerialize(g_states)))
            ErrorLog("DynamicFactions: could not write the faction state (" + path + ")");
        else if (g_debug)
            DebugLog("DynamicFactions: state written to " + path);
    }

    void ReadState(SaveFileSystem* sfs)
    {
        // Failure to read any part must never preserve another save's state.
        g_states.clear();
        g_runtime.clear();
        DfSetOwnFactions(std::vector<Faction*>());
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
        g_restoreTreasury = true;
        for (size_t i=0;i<g_states.size();++i)
            if (g_states[i].workFault) DfQuarantineTransfer("DynamicFactions: save contains an unresolved transfer - work remains stopped");
        for (size_t i = 0; i < g_states.size(); ++i)
            DebugLog("DynamicFactions: restored " + Quote(g_states[i].name) +
                     (g_states[i].extinct ? " (extinct)" : g_states[i].founded ? "" : " (not founded yet)"));
    }

    int SaveManagerGame_hook(SaveManager* self, const std::string& location, const std::string& name)
    {
        InterlockedIncrement(&g_saveDepth);
        if (DfTransferBusy())
            DfQuarantineTransfer("DynamicFactions: reentrant save during a transfer - manual reconciliation required");
        __try { return g_origSaveManagerGame(self, location, name); }
        __finally { InterlockedDecrement(&g_saveDepth); }
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
        BeginSession("save filesystem load");
        g_origSfsLoad(self, path);
        __try
        {
            ReadState(self);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // No state or native handles from the old game survive a failed read.
            BeginSession("faction state read failed");
            ErrorLog("DynamicFactions: reading the faction state failed - the faction is taken over from the game");
        }
        g_completedSfs = SESSION_LOAD;
        UpdateOwnFactions();
    }

    void SfsNew_hook(SaveFileSystem* self)
    {
        BeginSession("save filesystem new game");
        g_origSfsNew(self);
        std::vector<DfFactionState> none;
        SetStates(none, "new game");
        g_completedSfs = SESSION_NEW;
    }

    // Мир поднят: после загрузки сейва и после создания мира новой игры.
    int LoadGame_hook(SaveManager* self, const std::string& location, const std::string& name)
    {
        BeginSession("load requested");
        const int result = g_origLoadGame(self, location, name);
        // The meaning of the native int is not established. Require both the
        // explicit SFS completion and this known outer completion callback.
        if (g_completedSfs == SESSION_LOAD)
            MarkReady("save filesystem and load completed");
        else
            ErrorLog("DynamicFactions: no completed save filesystem load - waiting for a valid world");
        return result;
    }

    void ResetGame_hook(GameWorld* self)
    {
        BeginSession("world reset");
        if (g_debug)
            DebugLog("DynamicFactions: world reset");
        g_origResetGame(self);
    }

    void InitNewWorld_hook(GameWorld* self, GameDataManager* data)
    {
        g_origInitNewWorld(self, data);
        if (g_completedSfs == SESSION_NEW)
            MarkReady("new save filesystem and world initialised");
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "DynamicFactions", "Dynamic Factions", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("A faction in the Hub mines and sells real ore, buys food, medical supplies and suitable backpacks, and hires recruits from its actual budget. No passive income."));
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
                 Tr("Money of a new faction when it is founded. Further income comes only from real sales. Applies to factions founded after the change."),
                 5000, 0, 100000, 0);
    api->toggle(api, "DynamicFactions", "Work", Tr("Mining and trade"),
                Tr("Mine real resources, carry ore and sell it to town traders. Supply runs continue when work is disabled."), 1, 0);
    api->integer(api, "DynamicFactions", "MineRadius", Tr("Resource search radius"),
                 Tr("Search and scout within this distance from the Hub. Only resources observed by faction members become known."), 3000, 500, 5000, 0);
    api->hotkey(api, "DynamicFactions", "WindowKey", Tr("Faction window key"),
                Tr("Opens and closes the window with the state of the factions. Click and press a key; Backspace - no key."), "CTRL+F10", 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "DynamicFactions", "Debug", Tr("Detailed log"),
                Tr("Details of supply, confirmed purchases, work, saving and loading in RE_Kenshi_log.txt. Errors are always logged."), 0, 0);
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
                                                 SaveManagerGame_hook, &g_origSaveManagerGame) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::saveGame),
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

    if (!DfInstallWorkInfo(WorkInfoManaged))
        ErrorLog("DynamicFactions: could not stabilize the native work-skill information");

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                                                 GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("DynamicFactions: could not hook the interface update - the mod stays off");
        return;
    }

    if(!DfInstallNaturalHunger())
        ErrorLog("DynamicFactions: could not disable NPC hunger replenishment; native hunger remains");
    DebugLog("DynamicFactions: installed");
}
