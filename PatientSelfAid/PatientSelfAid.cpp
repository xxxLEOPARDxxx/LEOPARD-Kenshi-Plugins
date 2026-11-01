// PatientSelfAid - пациент лечит себя сам, пока его лечат.
//
// ЧТО НЕ ТАК В ИГРЕ
//
// Когда один персонаж оказывает первую помощь другому, лечащийся просто
// стоит и ждёт: лекарь ставит ему задачу «стой на месте» (STAND_STILL),
// чтобы тот не ушёл из-под рук. Аптечка у пациента при этом может быть
// своя, и лечить себя он вполне способен - время уходит впустую.
//
// ЧТО ДЕЛАЕТ ПЛАГИН
//
// То же, что игрок руками: выделить пациента и указать курсором лекаря на
// него самого. Раз в полсекунды перебираем персонажей игрока; если кто-то
// из них лечит другого, а тот стоит и ждёт, в сознании, нуждается в
// помощи и имеет аптечку (скелет - ремкомплект), пациенту отдаётся приказ
// лечить себя. Приказ - через OrdersReceiver::addOrder, тот же путь, что
// у клика игрока (PlayerInterface::addOrderSelectedCharacters): без Shift,
// прежние приказы сбрасываются.
//
// Лечение складывается: лекарь и сам пациент работают над одним телом
// одновременно, каждый своим навыком.
//
// ЧЕГО НЕ ТРОГАЕТ
//
// Пациента, который сам кого-то лечит, дерётся или занят чем угодно ещё,
// кроме ожидания лекаря. Персонажей не из отряда игрока. Лежащих и без
// сознания - они не могут.
//
// ПОЧЕМУ ХУК НА ForgottenGUI::update
//
// Приказы игрока отдаются из кода интерфейса, и мы отдаём оттуда же. Хук
// AITaskSytem::periodicUpdate, изнутри самого AI, уже занят SquadAutonomy,
// и вешаться вторым на ту же функцию незачем.

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "patient_self_aid"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>
#include <IniDefaults.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Character.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/Enums.h>
#include <kenshi/Tasker.h>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/util/hand.h>
#include <kenshi/gui/ForgottenGUI.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_debug = false;
    volatile LONG g_loadGeneration = 0;
    LONG g_seenGeneration = 0;
    void ResetState();

    // Как часто смотреть на отряд. Чаще незачем: лечение длится секунды.
    const DWORD SCAN_MS = 500;

    // Если приказ не прижился (пациент через это время всё ещё стоит и
    // ждёт), пробуем ещё раз, но не больше MAX_TRIES за одно лечение -
    // иначе, если игра его упорно сбрасывает, будем дёргать бесконечно.
    const DWORD RETRY_MS = 2000;
    const int MAX_TRIES = 3;

    // Лекарь «рядом с пациентом»: 6 метров, в игре 10 единиц на метр (так
    // BetterLooting переводит свой радиус). Лечащий стоит вплотную, а
    // дальше уже не он.
    const float NEAR_UNITS = 60.0f;


    std::string IniFileName()
    {
        char path[MAX_PATH] = {};

        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string file(path, length);

        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string()
                                        : file.substr(0, dot) + ".ini";
    }


    // Путь к ini игрока; при первом обращении - копия или дописка из
    // <Мод>.defaults.ini (shared/IniDefaults.h, 11.10.2026).
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
    }


    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        const char* const section = "PatientSelfAid";

        g_enabled =
            GetPrivateProfileIntA(section, "Enabled", 1, ini.c_str()) != 0;
        g_debug =
            GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
        KLocLog::SetEnabled(g_debug);
        ResetState();
    }


    // ---------------------------------------------------------------
    // Задачи
    // ---------------------------------------------------------------

    // Задачи, которыми один персонаж лечит или чинит другого. У приказа
    // игрока пациент - в subject, у работы медика - в subtarget.
    bool IsTreating(TaskType t)
    {
        return t == FIRST_AID_ORDER || t == FIRST_AID_ROBOT ||
               t == JOB_MEDIC || t == JOB_REPAIR_ROBOT;
    }


    const char* TaskName(TaskType t)
    {
        switch (t)
        {
        case FIRST_AID_ORDER:  return "FIRST_AID_ORDER";
        case FIRST_AID_ROBOT:  return "FIRST_AID_ROBOT";
        case JOB_MEDIC:        return "JOB_MEDIC";
        case JOB_REPAIR_ROBOT: return "JOB_REPAIR_ROBOT";
        case STAND_STILL:      return "STAND_STILL";
        case IDLE:             return "IDLE";
        default:               return NULL;
        }
    }


    std::string TaskText(TaskType t)
    {
        const char* const name = TaskName(t);
        if (name != NULL)
            return name;

        char number[24];
        sprintf_s(number, "task %d", static_cast<int>(t));
        return number;
    }


    // Текущая цель персонажа. NULL_TASK - если спросить не у кого.
    TaskType GoalOf(Character* c)
    {
        OrdersReceiver* const orders = c->getOrdersReciever();
        if (orders == NULL)
            return NULL_TASK;

        return orders->getCurrentGoal().key();
    }


    // Действие, которое персонаж выполняет прямо сейчас.
    TaskType ActionOf(Character* c)
    {
        AI* const ai = c->getAI();
        if (ai == NULL || ai->taskSystemAI == NULL)
            return NULL_TASK;

        Tasker* const task = ai->taskSystemAI->actions.getFirstTask();
        return task != NULL ? task->key() : NULL_TASK;
    }


    // Пациент из задачи, которую лекарь выполняет прямо сейчас.
    //
    // Работа медика (JOB_MEDIC, JOB_REPAIR_ROBOT) перебирает пациентов по
    // одному, и текущий лежит не в копии цели (там subject и subtarget
    // пустые), а в самой задаче: AITaskSytem::currentlySubTasking ->
    // currentSubTarget. Первая версия смотрела только в копию цели и
    // лечение по работе не видела вовсе - Шпильку лечили двое, а в журнал
    // она не попала ни разу; видно было только лечение по приказу.
    //
    // Поле читаем по раскладке из заголовка, поэтому под защитой: не
    // совпала раскладка - этого лекаря пропускаем, игру не роняем. В
    // функции нет объектов с деструкторами - иначе __try в VC10 нельзя.
    Character* SubTaskPatient(AITaskSytem* tasks)
    {
        __try
        {
            Tasker* const task = tasks->currentlySubTasking;
            if (task == NULL || !IsTreating(task->key()))
                return NULL;

            return task->currentSubTarget.getCharacter();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return NULL;
        }
    }


    // Кого персонаж лечит сейчас, если лечит. Смотрим цель, а не действие:
    // пока лекарь идёт к пациенту, действие у него - ходьба, а цель уже
    // лечение.
    Character* PatientOf(Character* medic, const TaskMatch* currentGoal)
    {
        if (currentGoal == NULL)
            return NULL;

        // Лечение по приказу игрока: пациент прямо в цели.
        const TaskMatch& goal = *currentGoal;
        if (IsTreating(goal.key()))
        {
            Character* const target = goal.subtarget.getCharacter();
            if (target != NULL)
                return target;

            Character* const subject = goal.subject.getCharacter();
            if (subject != NULL)
                return subject;
        }

        // Лечение по работе медика: пациент в выполняемой задаче.
        AI* const ai = medic->getAI();
        if (ai == NULL || ai->taskSystemAI == NULL)
            return NULL;

        return SubTaskPatient(ai->taskSystemAI);
    }


    // ---------------------------------------------------------------
    // Пациенты
    // ---------------------------------------------------------------

    struct PatientState
    {
        DWORD lastOrder;     // когда отдали приказ, 0 - ещё не отдавали
        int tries;           // сколько раз отдавали за это лечение
        std::string logged;  // последнее, что писали в журнал
        bool live;

        PatientState() : lastOrder(0), tries(0), live(false) {}
    };

    // Serial-bearing handles prevent a recycled Character address inheriting
    // another patient's retry count. Only this scan's live patients survive.
    std::map<hand, PatientState> g_patients;


    void Note(PatientState& state, Character* patient, Character* medic,
              TaskType goal, TaskType action, const char* reason,
              TaskType order = NULL_TASK)
    {
        if (!g_debug)
            return;
        std::string what = "treated by " + medic->displayName +
            " (goal " + TaskText(goal) + ", action " + TaskText(action) + "): " + reason;
        if (order != NULL_TASK)
            what += TaskText(order) + " on self, try " + std::string(1, char('0' + state.tries));
        if (state.logged == what)
            return;

        state.logged = what;
        DebugLog("PatientSelfAid: " + patient->displayName + " - " + what);
    }


    // Персонаж ждёт лекаря: стоит на месте по его просьбе.
    bool IsWaiting(TaskType goal, TaskType action)
    {
        return goal == STAND_STILL || action == STAND_STILL;
    }


    // needsFirstAid_robot also applies the AI's target-selection rules. A
    // biological patient with replacement limbs can be omitted there even
    // while another character is repairing those limbs. The medical system
    // already scores robot first aid independently of the patient's race.
    // Keep ordinary injury treatment first; wear alone needs a repair bed.
    bool NeedsRobotAid(Character* patient, AI* ai, const hand& self,
                       const Ogre::Vector3& where)
    {
        if (ai->needsFirstAid_robot(self, where))
            return true;
        MedicalSystem* const med = patient->getMedical();
        return med != NULL && med->hasRobotics() &&
               med->scoreFirstAidNeed(true) > 0.0f;
    }


    void Handle(Character* patient, Character* medic, PatientState& state)
    {
        const TaskType goal = GoalOf(patient);
        const TaskType action = ActionOf(patient);

        if (patient->isDead() || patient->isUnconcious() || patient->isDown())
        {
            Note(state, patient, medic, goal, action, "down or unconscious, cannot help");
            return;
        }

        // Уже лечит себя, лечит кого-то ещё или занят другим - не мешаем.
        if (IsTreating(goal))
        {
            Note(state, patient, medic, goal, action, "already treating someone");
            return;
        }

        if (!IsWaiting(goal, action))
        {
            Note(state, patient, medic, goal, action, "busy with something else");
            return;
        }

        // Failed orders are bounded before repeating medical/inventory queries.
        const DWORD now = GetTickCount();
        if (state.lastOrder != 0 && now - state.lastOrder < RETRY_MS)
            return;
        if (state.tries >= MAX_TRIES)
        {
            Note(state, patient, medic, goal, action,
                 "the order does not stick, giving up for this treatment");
            return;
        }

        AI* const ai = patient->getAI();
        if (ai == NULL)
            return;

        const hand self = ai->getHandle();
        const Ogre::Vector3 where = patient->getPosition();

        // Скелет чинится ремкомплектом, живой лечится аптечкой.
        TaskType order = NULL_TASK;
        if (ai->needsFirstAid(self, where) && ai->haveFirstAidKit(self, where))
            order = FIRST_AID_ORDER;
        else if (NeedsRobotAid(patient, ai, self, where) &&
                 ai->haveFirstAidKit_robot(self, where))
            order = FIRST_AID_ROBOT;

        if (order == NULL_TASK)
        {
            Note(state, patient, medic, goal, action, "no kit or nothing to treat");
            return;
        }

        if (state.tries > 0)
            Note(state, patient, medic, goal, action, "previous order did not stick");

        OrdersReceiver* const orders = patient->getOrdersReciever();
        if (orders == NULL)
            return;

        // Как клик игрока без Shift: прежние приказы сбрасываются.
        orders->addOrder(order, self, where, true, false);

        state.lastOrder = now;
        ++state.tries;

        Note(state, patient, medic, goal, action, "ordered ", order);
    }


    struct Observed
    {
        Character* character;
        Character* medic;
        Character* target;
        TaskType goal;
        Ogre::Vector3 position;
    };
    std::vector<Observed> g_squad;
    std::vector<unsigned> g_medics;

    Character* NearbyMedic(const Observed& patient)
    {
        Character* best = NULL;
        float bestDistance = NEAR_UNITS * NEAR_UNITS;
        for (size_t i = 0; i < g_medics.size(); ++i)
        {
            const Observed& other = g_squad[g_medics[i]];
            if (other.character == patient.character) continue;
            const float distance = (other.position - patient.position).squaredLength();
            if (distance <= bestDistance)
            {
                best = other.character;
                bestDistance = distance;
            }
        }
        return best;
    }

    struct WaitState
    {
        std::string goals;
        bool live;
        WaitState() : live(false) {}
    };
    std::map<hand, WaitState> g_unexplained;

    void NoteUnexplainedWaits()
    {
        for (std::map<hand, WaitState>::iterator i = g_unexplained.begin(); i != g_unexplained.end(); ++i)
            i->second.live = false;
        for (size_t i = 0; i < g_squad.size(); ++i)
        {
            const Observed& observed = g_squad[i];
            Character* const c = observed.character;
            if (observed.medic != NULL || observed.goal != STAND_STILL) continue;
            if (c->isDead() || c->isUnconcious() || c->isDown()) continue;
            AI* const ai = c->getAI();
            if (ai == NULL) continue;
            const hand self = ai->getHandle();
            const Ogre::Vector3& where = observed.position;
            const bool needs =
                (ai->needsFirstAid(self, where) && ai->haveFirstAidKit(self, where)) ||
                (NeedsRobotAid(c, ai, self, where) && ai->haveFirstAidKit_robot(self, where));
            if (!needs) continue;

            std::string goals;
            for (size_t j = 0; j < g_squad.size(); ++j)
            {
                const Observed& other = g_squad[j];
                if (other.character == c || other.goal == NULL_TASK || other.goal == IDLE) continue;
                goals += " " + other.character->displayName + "=" + TaskText(other.goal);
            }
            std::pair<std::map<hand, WaitState>::iterator, bool> entry =
                g_unexplained.insert(std::make_pair(self, WaitState()));
            WaitState& state = entry.first->second;
            state.live = true;
            if (!entry.second && state.goals == goals) continue;
            state.goals.swap(goals);
            DebugLog("PatientSelfAid: " + c->displayName +
                     " stands still with a kit, but no medic was recognised;"
                     " squad goals:" + state.goals);
        }
        for (std::map<hand, WaitState>::iterator i = g_unexplained.begin(); i != g_unexplained.end();)
            if (!i->second.live) g_unexplained.erase(i++); else ++i;
    }

    void ResetState()
    {
        g_patients.clear();
        g_unexplained.clear();
        std::vector<Observed>().swap(g_squad);
        std::vector<unsigned>().swap(g_medics);
    }

    void Scan()
    {
        if (!g_enabled) return;
        if (ou == NULL || ou->player == NULL) { ResetState(); return; }
        const lektor<Character*>& squad = ou->player->playerCharacters;
        // Reuse buffers while a roster is stable; release excessive old capacity
        // after large roster shrinkage. Never create a local owning lektor.
        g_squad.clear();
        g_medics.clear();
        if (g_squad.capacity() > 2 * squad.size() + 64)
            std::vector<Observed>().swap(g_squad);
        if (g_medics.capacity() > 2 * squad.size() + 64)
            std::vector<unsigned>().swap(g_medics);
        g_squad.reserve(squad.size());
        g_medics.reserve(squad.size());
        for (unsigned i = 0; i < squad.size(); ++i)
        {
            Character* const c = squad[i];
            if (c == NULL) continue;
            Observed observed;
            observed.character = c;
            observed.medic = NULL;
            OrdersReceiver* const orders = c->getOrdersReciever();
            const TaskMatch* const goal = orders != NULL ? &orders->getCurrentGoal() : NULL;
            observed.goal = goal != NULL ? goal->key() : NULL_TASK;
            observed.target = PatientOf(c, goal);
            if (observed.goal == STAND_STILL || IsTreating(observed.goal))
                observed.position = c->getPosition();
            if (IsTreating(observed.goal))
                g_medics.push_back(static_cast<unsigned>(g_squad.size()));
            g_squad.push_back(observed);
        }

        // Explicit task target has priority, as before. Require membership in
        // the current roster before reading the target or retaining its handle.
        for (size_t i = 0; i < g_squad.size(); ++i)
        {
            Character* const medic = g_squad[i].character;
            Character* const patient = g_squad[i].target;
            if (patient == NULL || patient == medic) continue;
            for (size_t j = 0; j < g_squad.size(); ++j)
                if (g_squad[j].character == patient)
                {
                    g_squad[j].medic = medic;
                    break;
                }
        }
        for (size_t i = 0; i < g_squad.size(); ++i)
        {
            Observed& patient = g_squad[i];
            if (patient.medic == NULL && patient.goal == STAND_STILL)
                patient.medic = NearbyMedic(patient);
        }
        for (std::map<hand, PatientState>::iterator i = g_patients.begin(); i != g_patients.end(); ++i)
            i->second.live = false;
        for (size_t i = 0; i < g_squad.size(); ++i)
        {
            const Observed& patient = g_squad[i];
            if (patient.medic == NULL) continue;
            const hand key(patient.character);
            PatientState& state = g_patients[key];
            state.live = true;
            Handle(patient.character, patient.medic, state);
        }
        for (std::map<hand, PatientState>::iterator i = g_patients.begin(); i != g_patients.end();)
            if (!i->second.live) g_patients.erase(i++); else ++i;
        if (g_debug) NoteUnexplainedWaits();
    }


    // ---------------------------------------------------------------
    // Кадр интерфейса
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;
    DWORD g_lastScan = 0;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);
        if (!g_enabled) return;

        const DWORD now = GetTickCount();
        if (now - g_lastScan < SCAN_MS)
            return;

        const LONG generation = InterlockedCompareExchange(&g_loadGeneration, 0, 0);
        if (generation != g_seenGeneration)
        {
            ResetState();
            g_seenGeneration = generation;
            if (g_debug) DebugLog("PatientSelfAid: save load, treatment state reset");
        }
        g_lastScan = now;
        Scan();
    }

    void (*g_origLoad)(Character*, GameSaveState*) = NULL;
    void Load(Character* character, GameSaveState* state)
    {
        g_origLoad(character, state);
        if (g_enabled && character != NULL && character->isPlayerCharacter())
            InterlockedIncrement(&g_loadGeneration);
    }

    void (*g_origNewGame)(SaveFileSystem*) = NULL;
    void NewGame(SaveFileSystem* self)
    {
        InterlockedIncrement(&g_loadGeneration);
        g_origNewGame(self);
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "PatientSelfAid", "Patient Self Aid", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("A patient being treated keeps treating himself instead of lying idle."));
    api->section(api, Tr("Treatment"));
    api->toggle(api, "PatientSelfAid", "Enabled", Tr("Enabled"), Tr("A patient being treated treats himself too."), 1, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "PatientSelfAid", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: who is treated, who treats, what the patient is doing and whether the order took. Written only when the state changes."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                           GuiUpdate_hook,
                           &g_origGuiUpdate))
    {
        ErrorLog("PatientSelfAid: could not hook the interface update, "
                 "the plugin stays off");
        return;
    }

    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_loadFromSerialise),
                          Load, &g_origLoad) != KenshiLib::SUCCESS)
        ErrorLog("PatientSelfAid: cannot hook character load");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),
                          NewGame, &g_origNewGame) != KenshiLib::SUCCESS)
        ErrorLog("PatientSelfAid: cannot hook new game");
    if (g_debug) DebugLog("PatientSelfAid: installed");
}
