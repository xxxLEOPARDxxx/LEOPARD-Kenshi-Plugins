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

#include <cstdio>
#include <map>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
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


    std::string IniPath()
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
    Character* PatientOf(Character* medic)
    {
        OrdersReceiver* const orders = medic->getOrdersReciever();
        if (orders == NULL)
            return NULL;

        // Лечение по приказу игрока: пациент прямо в цели.
        const TaskMatch& goal = orders->getCurrentGoal();
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
    };

    // Ключ - указатель, но только для поиска: разыменовываем мы лишь
    // персонажей из живого списка игрока, а записи о тех, кого в этом
    // проходе не лечат, выбрасываются.
    std::map<Character*, PatientState> g_patients;


    void Note(PatientState& state, Character* patient, const std::string& what)
    {
        if (!g_debug || state.logged == what)
            return;

        state.logged = what;
        DebugLog("PatientSelfAid: " + patient->displayName + " - " + what);
    }


    // Персонаж ждёт лекаря: стоит на месте по его просьбе.
    bool IsWaiting(TaskType goal, TaskType action)
    {
        return goal == STAND_STILL || action == STAND_STILL;
    }


    void Handle(Character* patient, Character* medic, PatientState& state)
    {
        const TaskType goal = GoalOf(patient);
        const TaskType action = ActionOf(patient);

        const std::string status =
            "treated by " + medic->displayName +
            " (goal " + TaskText(goal) + ", action " + TaskText(action) + ")";

        if (patient->isDead() || patient->isUnconcious() || patient->isDown())
        {
            Note(state, patient, status + ": down or unconscious, cannot help");
            return;
        }

        // Уже лечит себя, лечит кого-то ещё или занят другим - не мешаем.
        if (IsTreating(goal))
        {
            Note(state, patient, status + ": already treating someone");
            return;
        }

        if (!IsWaiting(goal, action))
        {
            Note(state, patient, status + ": busy with something else");
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
        else if (ai->needsFirstAid_robot(self, where) &&
                 ai->haveFirstAidKit_robot(self, where))
            order = FIRST_AID_ROBOT;

        if (order == NULL_TASK)
        {
            Note(state, patient, status + ": no kit or nothing to treat");
            return;
        }

        const DWORD now = GetTickCount();

        if (state.lastOrder != 0 && now - state.lastOrder < RETRY_MS)
            return;

        if (state.tries >= MAX_TRIES)
        {
            Note(state, patient, status +
                 ": the order does not stick, giving up for this treatment");
            return;
        }

        if (state.tries > 0)
            Note(state, patient, status + ": previous order did not stick");

        OrdersReceiver* const orders = patient->getOrdersReciever();
        if (orders == NULL)
            return;

        // Как клик игрока без Shift: прежние приказы сбрасываются.
        orders->addOrder(order, self, where, true, false);

        state.lastOrder = now;
        ++state.tries;

        Note(state, patient, status + ": ordered " + TaskText(order) +
             " on self, try " + std::string(1, char('0' + state.tries)));
    }


    // Ближайший к пациенту персонаж отряда, который сейчас лечит или
    // чинит, - в пределах NEAR_UNITS. NULL - такого рядом нет.
    Character* NearbyMedic(const lektor<Character*>& squad, Character* patient)
    {
        const Ogre::Vector3 at = patient->getPosition();
        const float limit = NEAR_UNITS * NEAR_UNITS;

        Character* best = NULL;
        float bestDistance = limit;

        for (unsigned int i = 0; i < squad.size(); ++i)
        {
            Character* const other = squad[i];
            if (other == NULL || other == patient)
                continue;
            if (!IsTreating(GoalOf(other)))
                continue;

            const float distance = (other->getPosition() - at).squaredLength();
            if (distance <= bestDistance)
            {
                best = other;
                bestDistance = distance;
            }
        }

        return best;
    }


    // Только для журнала. Персонаж стоит смирно, ему есть что лечить и
    // есть чем, а лекаря мы не нашли - значит, лекарь занят задачей,
    // которую плагин не узнаёт. Пишем цели всех остальных в отряде, чтобы
    // по журналу было видно, какую. Один раз на смену набора целей.
    std::map<Character*, std::string> g_unexplained;

    void NoteUnexplainedWaits(const lektor<Character*>& squad,
                              const std::map<Character*, Character*>& treated)
    {
        std::map<Character*, std::string> seen;

        for (unsigned int i = 0; i < squad.size(); ++i)
        {
            Character* const c = squad[i];
            if (c == NULL || treated.find(c) != treated.end())
                continue;
            if (GoalOf(c) != STAND_STILL)
                continue;
            if (c->isDead() || c->isUnconcious() || c->isDown())
                continue;

            AI* const ai = c->getAI();
            if (ai == NULL)
                continue;

            const hand self = ai->getHandle();
            const Ogre::Vector3 where = c->getPosition();
            const bool needs =
                (ai->needsFirstAid(self, where) && ai->haveFirstAidKit(self, where)) ||
                (ai->needsFirstAid_robot(self, where) &&
                 ai->haveFirstAidKit_robot(self, where));
            if (!needs)
                continue;

            std::string goals;
            for (unsigned int j = 0; j < squad.size(); ++j)
            {
                Character* const other = squad[j];
                if (other == NULL || other == c)
                    continue;
                const TaskType g = GoalOf(other);
                if (g == NULL_TASK || g == IDLE)
                    continue;
                goals += " " + other->displayName + "=" + TaskText(g);
            }

            seen[c] = goals;

            std::map<Character*, std::string>::iterator old = g_unexplained.find(c);
            if (old != g_unexplained.end() && old->second == goals)
                continue;

            DebugLog("PatientSelfAid: " + c->displayName +
                     " stands still with a kit, but no medic was recognised;"
                     " squad goals:" + goals);
        }

        g_unexplained.swap(seen);
    }


    void Scan()
    {
        if (ou == NULL || ou->player == NULL)
            return;

        const lektor<Character*>& squad = ou->player->playerCharacters;

        // Кого лечат в этом проходе и кто. Записи о прочих выбросим.
        std::map<Character*, Character*> treated;

        for (unsigned int i = 0; i < squad.size(); ++i)
        {
            Character* const medic = squad[i];
            if (medic == NULL)
                continue;

            Character* const patient = PatientOf(medic);
            if (patient == NULL || patient == medic ||
                !patient->isPlayerCharacter())
                continue;

            treated[patient] = medic;
        }

        // Второй способ - по соседству. Работа «ремонт роботов» своего
        // текущего пациента не держит ни в цели, ни в выполняемой задаче
        // (по журналу 25.09: у лекарей JOB_REPAIR_ROBOT, а пациента нигде
        // нет), и первый способ её не видит. Зато видно другое: пациент
        // стоит смирно, а рядом с ним работает лекарь из отряда. Кто ещё
        // может велеть стоять смирно - стража при досмотре, - тот не из
        // отряда, и лечащего рядом тогда нет.
        for (unsigned int i = 0; i < squad.size(); ++i)
        {
            Character* const patient = squad[i];
            if (patient == NULL || treated.find(patient) != treated.end())
                continue;
            if (GoalOf(patient) != STAND_STILL)
                continue;

            Character* const medic = NearbyMedic(squad, patient);
            if (medic != NULL)
                treated[patient] = medic;
        }

        std::map<Character*, PatientState> kept;

        for (std::map<Character*, Character*>::iterator it = treated.begin();
             it != treated.end(); ++it)
        {
            std::map<Character*, PatientState>::iterator old =
                g_patients.find(it->first);

            PatientState state;
            if (old != g_patients.end())
                state = old->second;
            else
            {
                state.lastOrder = 0;
                state.tries = 0;
            }

            Handle(it->first, it->second, state);
            kept[it->first] = state;
        }

        g_patients.swap(kept);

        if (g_debug)
            NoteUnexplainedWaits(squad, treated);
    }


    // ---------------------------------------------------------------
    // Кадр интерфейса
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;
    DWORD g_lastScan = 0;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);

        const DWORD now = GetTickCount();
        if (now - g_lastScan < SCAN_MS)
            return;

        g_lastScan = now;
        Scan();
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
    api->toggle(api, "PatientSelfAid", "Enabled", Tr("Enabled"), Tr("A patient being treated treats himself too."), 1, MCM_RESTART);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "PatientSelfAid", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: who is treated, who treats, what the patient is doing and whether the order took. Written only when the state changes."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("PatientSelfAid: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                           GuiUpdate_hook,
                           &g_origGuiUpdate))
    {
        ErrorLog("PatientSelfAid: could not hook the interface update, "
                 "the plugin stays off");
        return;
    }

    DebugLog("PatientSelfAid: installed");
}
