// AIFixed - точечные правки ИИ Kenshi (22.10.2026).
//
// Правила (просьба пользователя): чинить только то место, где ошибка, и
// только в том случае, который чиним; всегда звать оригинал игры; не
// держать своего состояния ИИ - так плагин уживается с другими модами на
// ИИ (SquadAutonomy и подобными), которые висят на тех же функциях.
//
// ---------------------------------------------------------------------
// 1. СКЕЛЕТОВ НЕСУТ НА ОБЫЧНЫЕ КРОВАТИ
//
// Задача ИИ «найти раненого союзника и отнести в кровать»
// (FIND_AND_RESCUE_IF_THERES_BEDS) выбирает цели двумя функциями: раненого -
// AI::findDisabledAlly, кровать - AI::findOwnedFreeBed. Вторая (разбор кода
// игры 22.10.2026) аргумент «для кого» не читает вовсе, а тип мебели в ней
// зашит: BF_BED. У персонажей игрока кандидаты - getAllLocalMachines(список,
// 1 << BF_BED), у NPC - кровати дома. Ремкровать (BF_SKELETON_BED) не
// рассматривается никогда, и скелета кладут на обычную кровать.
//
// Правка: хук на findOwnedFreeBed. Сначала - оригинал. Если кровать ищут
// для скелета (раса robot: несомый персонаж или персонаж из аргумента) и
// игра дала не ремкровать, ищем свободную ремкровать тем же способом, что
// игра ищет кровать для отряда игрока: getAllLocalMachines с битом
// BF_SKELETON_BED, те же отсевы (никто уже не несёт туда, место свободно,
// мебель своей фракции или союзной), своя фракция раньше союзной, потом
// ближняя. Нашли - отдаём её с той же оценкой успеха, что у игры (1.0).
// Не нашли - остаётся выбор игры: обычная кровать лучше, чем земля.
//
// Не трогаем: кровати для живых; поход скелета спать самому (персонаж из
// аргумента - он сам); всё, кроме выбора кровати.
// ---------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "ai_fixed"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>
#include <intrin.h>
#include <IniDefaults.h>

#include <cstdio>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/FactionRelations.h>
#include <kenshi/RaceData.h>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/Blackboard.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Tasker.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

#pragma intrinsic(_ReturnAddress)


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_skeletonBeds = true;
    bool g_debug = false;


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
    // <Мод>.defaults.ini (shared/IniDefaults.h).
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
    }


    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        const char* const section = "AIFixed";

        g_enabled =
            GetPrivateProfileIntA(section, "Enabled", 1, ini.c_str()) != 0;
        g_skeletonBeds =
            GetPrivateProfileIntA(section, "SkeletonRepairBeds", 1, ini.c_str()) != 0;
        g_debug =
            GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
    }


    // ---------------------------------------------------------------
    // 1. Ремкровати для скелетов
    // ---------------------------------------------------------------

    // Список, который заполняет игра. У lektor нет деструктора, а буфер
    // выделяет и наращивает сама игра своим распределителем: освобождать его
    // нам нечем. Поэтому один список на поток на всё время (память не
    // растёт), и свой у каждого потока - на случай, если ИИ думает не только
    // в главном. Нули - то же состояние, что даёт конструктор lektor.
    __declspec(thread) char t_machines[sizeof(lektor<Building*>)];

    lektor<Building*>& Machines()
    {
        lektor<Building*>& list = *reinterpret_cast<lektor<Building*>*>(t_machines);
        list.clear();
        return list;
    }


    bool IsRobot(Character* c)
    {
        RaceData* const race = c->getRace();
        return race != NULL && race->robot;
    }


    // Для кого ищут кровать: кого несут, иначе персонаж из аргумента. Сам
    // ищущий - не пациент: так игра ищет кровать, чтобы лечь самому.
    Character* PatientOf(AI* ai, const hand& in)
    {
        Character* const me = ai->me;
        if (me == NULL)
            return NULL;

        if (me->isCarryingSomething)
        {
            Character* const carried = me->getCarryingObject().getCharacter();
            if (carried != NULL && carried != me)
                return carried;
        }

        Character* const subject = in.getCharacter();
        return subject != me ? subject : NULL;
    }


    // Свободная ремкровать рядом, теми же отсевами, что у игры для кроватей
    // отряда игрока. NULL - такой нет.
    Building* FindRepairBed(AI* ai, Character* me)
    {
        lektor<Building*>& machines = Machines();
        ai->getAllLocalMachines(machines, 1u << BF_SKELETON_BED);
        if (machines.size() == 0)
            return NULL;

        Blackboard* const board = ai->getBlackboard();
        Faction* const myFaction = me->getFaction();
        const Ogre::Vector3 here = me->getPosition();

        Building* best = NULL;
        bool bestOwn = false;
        float bestDistance = 0.0f;

        for (uint32_t i = 0; i < machines.size(); ++i)
        {
            Building* const bed = machines[i];
            if (bed == NULL || bed->getSpecialFunction() != BF_SKELETON_BED)
                continue;

            // Туда уже несут кого-то другого.
            if (board != NULL && board->howManyGuysWorkingOnThisSubTarget(me->handle, bed->handle) > 0)
                continue;

            // Свободное место - как спрашивает игра, от имени несущего.
            if (!static_cast<UseableStuff*>(bed)->isFreeSlot(me->handle))
                continue;

            Faction* const bedFaction = bed->getFaction();
            const bool own = bedFaction == myFaction;
            if (!own && (bedFaction == NULL || bedFaction->relations == NULL ||
                         !bedFaction->relations->isAlly(myFaction)))
                continue;

            const float distance = (bed->getPosition() - here).squaredLength();
            if (best == NULL || (own && !bestOwn) || (own == bestOwn && distance < bestDistance))
            {
                best = bed;
                bestOwn = own;
                bestDistance = distance;
            }
        }

        return best;
    }


    void Note(Character* patient, Building* bed, bool justAsking)
    {
        if (!g_debug)
            return;

        char line[512];
        _snprintf_s(line, _TRUNCATE, "AIFixed: repair bed for %s%s - %s (thread %lu)",
                    patient->displayName.c_str(), justAsking ? " (just asking)" : "",
                    bed != NULL ? bed->displayName.c_str() : "none free, the game's choice stays",
                    GetCurrentThreadId());
        DebugLog(line);
    }


    // Правка поверх результата игры. true - кровать заменена.
    bool UseRepairBed(AI* ai, const hand& in, hand& out, bool justAsking, float score)
    {
        Character* const patient = PatientOf(ai, in);
        if (patient == NULL || !IsRobot(patient))
            return false;

        // Игра уже дала ремкровать (другой мод или в будущем она сама).
        if (score > 0.0f)
        {
            Building* const chosen = out.getBuilding();
            if (chosen != NULL && chosen->getSpecialFunction() == BF_SKELETON_BED)
                return false;
        }

        Building* const bed = FindRepairBed(ai, ai->me);
        Note(patient, bed, justAsking);
        if (bed == NULL)
            return false;

        out = bed;
        return true;
    }


    // Разбор полей игровых объектов - под защитой: не совпала раскладка или
    // объект умер на полпути - остаётся выбор игры. Объектов с деструкторами
    // в этой функции нет (требование __try в VC10).
    bool SafeUseRepairBed(AI* ai, const hand& in, hand& out, bool justAsking, float score)
    {
        __try
        {
            return UseRepairBed(ai, in, out, justAsking, score);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static LONG told = 0;
            if (InterlockedExchange(&told, 1) == 0)
                ErrorLog("AIFixed: repair bed lookup failed once - the game's bed choice is kept");
            return false;
        }
    }


    // ---------------------------------------------------------------
    // Диагностика укладывания скелетов (22.10.2026; только при Debug=1 на
    // старте игры - иначе эти хуки не ставятся вовсе)
    //
    // Первая проверка: скелетов стали носить к ремкроватям, но клали рядом.
    // Действие укладывания (PUT_SOMEONE_IN_BED) и оценка «положить в
    // кровать» тип кровати не смотрят - чтобы не гадать, пишем, кто кого
    // роняет и откуда, кого кладут в кровать, какая задача сорвалась.
    // ---------------------------------------------------------------

    std::string Where(const void* a)
    {
        HMODULE m = NULL;
        char name[MAX_PATH] = "?";
        if (a != NULL &&
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(a), &m) && m != NULL)
        {
            GetModuleFileNameA(m, name, sizeof(name));
            const char* slash = strrchr(name, 0x5C);
            char buf[MAX_PATH + 32];
            _snprintf_s(buf, _TRUNCATE, "%s+0x%llX", slash != NULL ? slash + 1 : name,
                        static_cast<unsigned long long>(static_cast<const char*>(a) - reinterpret_cast<char*>(m)));
            return buf;
        }
        return "?";
    }


    std::string TasksOf(Character* c)
    {
        char buf[96] = "goal ? action ?";
        AI* const ai = c->getAI();
        OrdersReceiver* const orders = c->getOrdersReciever();
        const int goal = orders != NULL ? static_cast<int>(orders->getCurrentGoal().key()) : -1;
        int action = -1;
        if (ai != NULL && ai->taskSystemAI != NULL)
        {
            Tasker* const task = ai->taskSystemAI->actions.getFirstTask();
            if (task != NULL)
                action = static_cast<int>(task->key());
        }
        _snprintf_s(buf, _TRUNCATE, "goal 0x%X action 0x%X", goal, action);
        return buf;
    }


    Character* CarriedRobot(Character* carrier)
    {
        if (carrier == NULL || !carrier->isCarryingSomething)
            return NULL;
        Character* const carried = carrier->getCarryingObject().getCharacter();
        return carried != NULL && IsRobot(carried) ? carried : NULL;
    }


    void (*g_origDrop)(Character*, bool, bool) = NULL;

    void NoteDrop(Character* self, bool ragdoll, bool removeOnly, const void* from)
    {
        Character* const carried = CarriedRobot(self);
        if (carried == NULL)
            return;
        DebugLog("AIFixed: " + self->displayName + " drops skeleton " + carried->displayName +
                 (ragdoll ? " (ragdoll)" : "") + (removeOnly ? " (remove only)" : "") +
                 ", " + TasksOf(self) + ", from " + Where(from));
    }

    void SafeNoteDrop(Character* self, bool ragdoll, bool removeOnly, const void* from)
    {
        __try
        {
            NoteDrop(self, ragdoll, removeOnly, from);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void Drop_hook(Character* self, bool ragdoll, bool removeOnly)
    {
        SafeNoteDrop(self, ragdoll, removeOnly, _ReturnAddress());
        g_origDrop(self, ragdoll, removeOnly);
    }


    void (*g_origSetBedMode)(Character*, bool, UseableStuff*) = NULL;

    void NoteBed(Character* self, bool on, UseableStuff* bed, const void* from)
    {
        if (!IsRobot(self))
            return;
        char line[128];
        _snprintf_s(line, _TRUNCATE, "%s, function %d", on ? "on" : "off",
                    bed != NULL ? static_cast<int>(bed->getSpecialFunction()) : -1);
        DebugLog("AIFixed: bed mode for skeleton " + self->displayName + ": " +
                 (bed != NULL ? bed->displayName : std::string("no bed")) + " " + line +
                 ", from " + Where(from));
    }

    void SafeNoteBed(Character* self, bool on, UseableStuff* bed, const void* from)
    {
        __try
        {
            NoteBed(self, on, bed, from);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void SetBedMode_hook(Character* self, bool on, UseableStuff* bed)
    {
        SafeNoteBed(self, on, bed, _ReturnAddress());
        g_origSetBedMode(self, on, bed);
    }


    void (*g_origTaskImpossible)(AITaskSytem*) = NULL;

    void NoteImpossible(AITaskSytem* tasks, const void* from)
    {
        Character* const me = tasks->me;
        Character* const carried = CarriedRobot(me);
        if (carried == NULL)
            return;
        DebugLog("AIFixed: " + me->displayName + " carrying skeleton " + carried->displayName +
                 ": task impossible, " + TasksOf(me) + ", from " + Where(from));
    }

    void SafeNoteImpossible(AITaskSytem* tasks, const void* from)
    {
        __try
        {
            NoteImpossible(tasks, from);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void TaskImpossible_hook(AITaskSytem* self)
    {
        SafeNoteImpossible(self, _ReturnAddress());
        g_origTaskImpossible(self);
    }


    float (*g_origFindOwnedFreeBed)(AI*, const hand&, hand&, bool) = NULL;

    float FindOwnedFreeBed_hook(AI* self, const hand& in, hand& out, bool justAsking)
    {
        const float score = g_origFindOwnedFreeBed(self, in, out, justAsking);

        if (!g_skeletonBeds || self == NULL)
            return score;

        // Успех у игры - 1.0 (findOwnedFreeBed, ветка выбора кровати).
        return SafeUseRepairBed(self, in, out, justAsking, score) ? 1.0f : score;
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "AIFixed", "AI Fixed", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Small fixes of the game's AI. Each one only touches the case it fixes."));
    api->section(api, Tr("General"));
    api->toggle(api, "AIFixed", "Enabled", Tr("Enabled"), Tr("All fixes of this mod."), 1, MCM_RESTART);
    api->section(api, Tr("Fixes"));
    api->toggle(api, "AIFixed", "SkeletonRepairBeds", Tr("Skeletons to repair beds"),
                Tr("A wounded skeleton is carried to a free skeleton repair bed instead of an ordinary bed. No free repair bed nearby - an ordinary bed, as in the game."), 1, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "AIFixed", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: for whom a repair bed was looked for and which one was chosen."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("AIFixed: disabled in the ini");
        return;
    }

    // Хук стоит всегда, выключатель правки читается на ходу.
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&AI::findOwnedFreeBed),
                           FindOwnedFreeBed_hook,
                           &g_origFindOwnedFreeBed))
    {
        ErrorLog("AIFixed: could not hook the bed search, skeletons keep going to ordinary beds");
    }

    // Диагностика - только при подробном журнале на старте.
    if (g_debug)
    {
        if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::dropCarriedObject),
                                                     Drop_hook, &g_origDrop) ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::setBedMode),
                                                     SetBedMode_hook, &g_origSetBedMode) ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&AITaskSytem::taskImpossible),
                                                     TaskImpossible_hook, &g_origTaskImpossible))
            ErrorLog("AIFixed: some diagnostic hooks failed - the log will be incomplete");
        else
            DebugLog("AIFixed: diagnostics on (drops, bed mode, impossible tasks of skeleton carriers)");
    }

    DebugLog("AIFixed: installed");
}
