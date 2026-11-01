// AIFixed - точечные правки ИИ Kenshi (22.10.2026).
//
// Правила (просьба пользователя): чинить только то место, где ошибка, и
// только в том случае, который чиним; всегда звать оригинал игры.
// Автоматический отдых и проход ворот хранят только ограниченные временные
// записи собственных приказов; постоянные работы игры не меняются.
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
#include <map>
#include <set>
#include <string>
#include <vector>
#include <exception>
#include "RestPolicy.h"
#include "GatePolicy.h"

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/FactionRelations.h>
#include <kenshi/RaceData.h>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/Blackboard.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Tasker.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/Building/DoorStuff.h>
#include <kenshi/Town.h>
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
    bool g_selfRepair = true;
    int g_selfRepairWear = 50;      // износ части, с которого идти чиниться (0-100)
    bool g_autoSleep=false,g_repairHealth=false,g_ownBedsOnly=true;
    int g_sleepBelow=90,g_healAbove=99,g_robotBelow=90,g_bedRadius=100;
    bool g_gatePassage=false,g_gateUnlock=false,g_gateClose=true,g_gateRelock=true;
    int g_gateRadius=30,g_gateCloseDelay=3;
    bool g_restConflict=false,g_gateConflict=false,g_newHooksReady=false,g_gateHooksReady=false;
    CRITICAL_SECTION g_intentLock;
    __declspec(thread) bool t_ownOrder=false;
    bool g_debug = false;
    AIFixedRest::SimulationClock g_restClock,g_gateClock;

    // Native load hooks only publish invalidation; GUI owns repair containers.
    volatile LONG g_loadGeneration = 0;
    volatile LONG g_worldActiveAt = 0, g_worldReadyGeneration = -1;
    volatile LONG g_loading = 0;
    LONG g_seenGeneration = -1;
    bool g_lifecycleReady = false, g_sessionActive = false;


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
        g_selfRepair =
            GetPrivateProfileIntA(section, "SelfRepair", 1, ini.c_str()) != 0;
        g_selfRepairWear =
            GetPrivateProfileIntA(section, "SelfRepairWear", 50, ini.c_str());
        if (g_selfRepairWear < 0)
            g_selfRepairWear = 0;
        if (g_selfRepairWear > 100)
            g_selfRepairWear = 100;
        g_autoSleep=GetPrivateProfileIntA(section,"AutoSleep",0,ini.c_str())!=0;
        g_repairHealth=GetPrivateProfileIntA(section,"RepairHealth",0,ini.c_str())!=0;
        g_ownBedsOnly=GetPrivateProfileIntA(section,"OwnBedsOnly",1,ini.c_str())!=0;
        g_sleepBelow=(std::max)(10,(std::min)(99,(int)GetPrivateProfileIntA(section,"SleepBelowHealth",90,ini.c_str())));
        g_healAbove=(std::max)(g_sleepBelow,(std::min)(100,(int)GetPrivateProfileIntA(section,"WakeAboveHealth",99,ini.c_str())));
        g_robotBelow=(std::max)(10,(std::min)(99,(int)GetPrivateProfileIntA(section,"RepairBelowHealth",90,ini.c_str())));
        g_bedRadius=(std::max)(10,(std::min)(500,(int)GetPrivateProfileIntA(section,"BedSearchRadius",100,ini.c_str())));
        g_gatePassage=GetPrivateProfileIntA(section,"GatePassage",0,ini.c_str())!=0;
        g_gateUnlock=GetPrivateProfileIntA(section,"GateOpenLocked",0,ini.c_str())!=0;
        g_gateClose=GetPrivateProfileIntA(section,"GateCloseAfterPassage",1,ini.c_str())!=0;
        g_gateRelock=GetPrivateProfileIntA(section,"GateRestoreLock",1,ini.c_str())!=0;
        g_gateRadius=(std::max)(5,(std::min)(100,(int)GetPrivateProfileIntA(section,"GateSearchRadius",30,ini.c_str())));
        g_gateCloseDelay=(std::max)(1,(std::min)(30,(int)GetPrivateProfileIntA(section,"GateCloseDelay",3,ini.c_str())));
        g_debug =
            GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
        KLocLog::SetEnabled(g_debug);
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


    // Compare only the SDK's public identity fields, never vptr/padding or the
    // game's undocumented hand ordering. Both object and container reuse matter.
    struct HandIdentityLess
    {
        bool operator()(const hand& a, const hand& b) const
        {
            if (a.type != b.type) return a.type < b.type;
            if (a.container != b.container) return a.container < b.container;
            if (a.containerSerial != b.containerSerial) return a.containerSerial < b.containerSerial;
            if (a.index != b.index) return a.index < b.index;
            return a.serial < b.serial;
        }
    };

    bool SameIdentity(const hand& a, const hand& b)
    {
        const HandIdentityLess less;
        return !less(a, b) && !less(b, a);
    }

    typedef std::set<hand, HandIdentityLess> ReservedSet;

    struct IntentGuard
    {
        IntentGuard(){EnterCriticalSection(&g_intentLock);}~IntentGuard(){LeaveCriticalSection(&g_intentLock);}
    private:IntentGuard(const IntentGuard&);IntentGuard& operator=(const IntentGuard&);
    };
    typedef std::map<hand,unsigned,HandIdentityLess> IntentMap;
    IntentMap g_intents;unsigned g_intentSerial=0;bool g_intentOverflow=false;
    struct MoveIntent{hand actor;Ogre::Vector3 destination,sample;unsigned serial;};
    std::vector<MoveIntent> g_moveIntents;
    unsigned InputSequence(Character* c)
    {
        const hand key(c);IntentGuard guard;
        IntentMap::const_iterator it=g_intents.find(key);
        return g_intentOverflow?~0u:it==g_intents.end()?0:it->second;
    }
    bool InputIdentity(OrdersReceiver* orders,hand* out)
    {
        __try
        {
            if(t_ownOrder||!g_newHooksReady||(!g_selfRepair&&!g_autoSleep&&!g_repairHealth&&!g_gatePassage)||
               !orders||!orders->me||!orders->me->isPlayerCharacter()||
               InterlockedCompareExchange(&g_loading,0,0)!=0)return false;
            *out=orders->me->handle;return true;
        }
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    void RecordInput(const hand& who)
    {
        IntentGuard guard;
        if(g_intents.find(who)==g_intents.end()&&g_intents.size()>=1024)
        {if(!g_intentOverflow){g_intentOverflow=true;ErrorLog("AIFixed: input identity capacity reached; automatic orders suspended");}return;}
        if(++g_intentSerial==0)++g_intentSerial;
        g_intents[who]=g_intentSerial;
    }
    void (*g_origOrdersAdd)(OrdersReceiver*,TaskType,const hand&,const Ogre::Vector3&,bool,bool)=NULL;
    void OrdersAdd_hook(OrdersReceiver* self,TaskType task,const hand& subject,const Ogre::Vector3& position,bool clearOld,bool shift)
    {
        hand who;const bool capture=InputIdentity(self,&who);
        const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
        g_origOrdersAdd(self,task,subject,position,clearOld,shift);
        if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))RecordInput(who);
    }
    void (*g_origOrdersClear)(OrdersReceiver*)=NULL;
    void OrdersClear_hook(OrdersReceiver* self)
    {
        hand who;const bool capture=InputIdentity(self,&who);
        const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
        g_origOrdersClear(self);
        if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))RecordInput(who);
    }
    void OwnRestOrder(OrdersReceiver* orders,TaskType task,const hand& target,const Ogre::Vector3& position)
    {
        const bool old=t_ownOrder;t_ownOrder=true;
        __try{orders->addOrder(task,target,position,true,false);}
        __finally{t_ownOrder=old;}
    }

    #include "GateRuntime.h"

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


    // Скорость починки ремкровати - как её считает medicalUpdate:
    // getProductionMult() * calculateEfficiencyMult() её UseableStuff.
    // calculateEfficiencyMult в заголовке protected - зовём по таблице
    // (vtable +0x368), как зовёт сама игра.
    float EfficiencyOf(UseableStuff* u)
    {
        typedef float (*Fn)(UseableStuff*);
        return (*reinterpret_cast<Fn*>(*reinterpret_cast<char**>(u) + 0x368))(u);
    }

    float RepairRate(Building* bed)
    {
        UseableStuff* const useable = bed->getUseableStuff();
        return useable != NULL ? bed->getProductionMult() * EfficiencyOf(useable) : 0.0f;
    }


    // Свободная ремкровать рядом, теми же отсевами, что у игры для кроватей
    // отряда игрока, и только та, что чинит (есть питание). NULL - такой нет.
    // me - кто ищет: несущий раненого или сам скелет (саморемонт).
    struct RestBedContext
    {
        Character* actor;TownBase* town;Faction* owner;Ogre::Vector3 here;hand inside;int floor;
        explicit RestBedContext(Character* c):actor(c),town(c->getCurrentTownLocation()),
            owner(c->getFaction()),here(c->getPosition()),inside(c->isInsideBuilding),floor(c->floorNum){}
    };
    bool RestBedAllowed(Building* bed,const RestBedContext& context,AIFixedRest::Care care,AIFixedRest::Bed& candidate)
    {
        if(!bed||!context.owner)return false;
        candidate.properFunction=bed->getSpecialFunction()==(care==AIFixedRest::Repair?BF_SKELETON_BED:BF_BED);
        candidate.furniture=bed->isFurniture();
        if(!candidate.properFunction||!candidate.furniture)return false;
        Faction* owner=bed->getFaction();candidate.own=owner==context.owner;
        candidate.allied=!g_ownBedsOnly&&owner&&owner!=context.owner&&owner->relations&&owner->relations->isAlly(context.owner);
        candidate.sameTown=bed->getTown()==context.town;
        const Ogre::Vector3 delta=bed->getPosition()-context.here;candidate.distanceSq=delta.squaredLength();
        candidate.sameFloor=std::fabs(delta.y)<=10.0f;
        if(SameIdentity(context.inside,bed->isInsideBuilding)&&!context.inside.isNull()&&context.floor!=bed->floorNum)candidate.sameFloor=false;
        // Local machines can include all player bases. Reject geography and
        // ownership before native slot/blackboard/power work.
        if(!AIFixedRest::LocationAllowed(candidate,g_ownBedsOnly,(float)g_bedRadius))return false;
        UseableStuff* useable=bed->getUseableStuff();candidate.useable=useable!=NULL;
        if(!useable)return false;
        candidate.free=useable->isFreeSlot(context.actor->handle)&&useable->couldIOperate(context.actor->handle);
        candidate.powered=care!=AIFixedRest::Repair||RepairRate(bed)>0;
        return AIFixedRest::Allowed(candidate,care,g_ownBedsOnly,(float)g_bedRadius);
    }
    Building* FindRestBed(AI* ai,Character* me,const ReservedSet* reserved,AIFixedRest::Care care)
    {
        if(!ai||!me)return NULL;
        const RestBedContext context(me);
        if(!context.owner)return NULL;
        lektor<Building*>& machines=Machines();
        const BuildingFunction function=care==AIFixedRest::Repair?BF_SKELETON_BED:BF_BED;
        ai->getAllLocalMachines(machines,1u<<function);
        if(machines.size()>4096)return NULL;
        Blackboard* const board=ai->getBlackboard();
        Building* best=NULL;bool bestOwn=false;float bestDistance=0;
        for(uint32_t i=0;i<machines.size();++i)
        {
            Building* bed=machines[i];AIFixedRest::Bed candidate;
            if(!RestBedAllowed(bed,context,care,candidate))continue;
            if(reserved&&reserved->find(hand(bed))!=reserved->end())continue;
            if(board&&board->howManyGuysWorkingOnThisSubTarget(me->handle,bed->handle)>0)continue;
            if(!best||(candidate.own&&!bestOwn)||(candidate.own==bestOwn&&candidate.distanceSq<bestDistance))
            {best=bed;bestOwn=candidate.own;bestDistance=candidate.distanceSq;}
        }
        return best;
    }
    Building* FindRepairBed(AI* ai,Character* me,const ReservedSet* reserved)
    {return FindRestBed(ai,me,reserved,AIFixedRest::Repair);}
    Building* SafeFindRestBed(AI* ai,Character* me,const ReservedSet* reserved,AIFixedRest::Care care,bool* failed=NULL)
    {
        __try{return FindRestBed(ai,me,reserved,care);}
        __except(NativeFaultFilter(GetExceptionCode()))
        {if(failed)*failed=true;ErrorLog("AIFixed: invalid bed lookup; injury episode suppressed");return NULL;}
    }


    // Журнал - одна строка на смену: игра переспрашивает кровать каждые
    // ~0.3 с, а лежащих каждые ~0.2 с заново «укладывает» в ту же кровать.
    // Ключи - только для сравнения, не разыменовываются. Только при Debug.
    CRITICAL_SECTION g_noteLock;
    typedef std::map<Character*, const void*> NotedMap;
    NotedMap g_notedSearch;     // какую кровать нашли
    NotedMap g_notedBed;        // в какой кровати лежит
    std::map<Character*, DWORD> g_lastSnap;

    bool Changed(NotedMap& noted, Character* c, const void* what)
    {
        EnterCriticalSection(&g_noteLock);
        NotedMap::iterator it = noted.find(c);
        const bool changed = it == noted.end() || it->second != what;
        noted[c] = what;
        LeaveCriticalSection(&g_noteLock);
        return changed;
    }


    void Note(Character* patient, Building* bed, bool justAsking)
    {
        if (!g_debug || !Changed(g_notedSearch, patient, bed))
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

        Building* const bed = FindRepairBed(ai, ai->me, NULL);
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
    // 1б. Скелет встаёт с ремкровати сразу, не починившись
    //
    // Проверка 22.10.2026: уложенный в ремкровать скелет (и RLBS, и робо-
    // робот) тут же встаёт. Задача «встать, когда вылечился»
    // (AI::scoreGetOutOfBedOnceFullyHealed) прибавляет 1.0 к оценке «встать»,
    // когда MedicalSystem::restedState > 0.9. А restedState игра считает в
    // precalculateFirstAidNeedScore только по живым частям: часть робота
    // (race->robot или протез) пропускается, и у скелета restedState всегда
    // 1.0 - «вылечен» сразу.
    //
    // Правка: кто лежит в ремкровати (скелет или живой с протезами), пока его
    // робот-части не починены полностью (износ снят, прочность до полного
    // максимума - ремкровать чинит и то и другое), прибавку «вылечен» не
    // получает. Первая сборка 22.10 мерила как игра у живых (> 0.9 от
    // максимума за вычетом износа) - вставали недочиненными. Кровать без
    // питания не чинит - тогда решает игра, как раньше.
    // ---------------------------------------------------------------

    // Часть - робот: у скелета все части, у живого - протезы.
    bool IsRoboticPart(const MedicalSystem::HealthPartStatus* p, bool robotRace)
    {
        return robotRace || p->robotLimb != NULL;
    }


    // Починены ли все робот-части: износ снят, прочность - до полного
    // максимума (ремкровать чинит и то и другое - HealthPartStatus::update,
    // ветка robotWear). Потерянные части (isDead) кровать не вернёт - их не
    // ждём. Допуски - полединицы, чтобы не ждать дробей.
    bool RoboticPartsRepaired(MedicalSystem* med, bool robotRace)
    {
        const lektor<MedicalSystem::HealthPartStatus*>& parts = med->anatomy;
        for (uint32_t i = 0; i < parts.size(); ++i)
        {
            const MedicalSystem::HealthPartStatus* const p = parts[i];
            if (p == NULL || !IsRoboticPart(p, robotRace) || p->isDead())
                continue;
            const float full = p->_maxHealth * p->age * p->HPMult;
            if (full <= 0.0f)
                continue;
            if (p->wearDamage > 0.5f || p->flesh - p->fleshStun < full - 0.5f)
                return false;
        }
        return true;
    }


    bool HasRoboticParts(MedicalSystem* med, bool robotRace)
    {
        if (robotRace)
            return true;
        const lektor<MedicalSystem::HealthPartStatus*>& parts = med->anatomy;
        for (uint32_t i = 0; i < parts.size(); ++i)
            if (parts[i] != NULL && parts[i]->robotLimb != NULL)
                return true;
        return false;
    }


    // Порог и прибавка - те же, что в коде игры (scoreGetOutOfBedOnceFullyHealed).
    const float HEALED_ABOVE = 0.9f;
    const float HEALED_BONUS = 1.0f;

    float StayUntilRepaired(AI* ai, float score)
    {
        Character* const me = ai->me;
        if (me == NULL || me->inSomething != IN_BED)
            return score;

        Building* const bed = me->inWhat.getBuilding();
        if (bed == NULL || bed->getSpecialFunction() != BF_SKELETON_BED)
            return score;

        MedicalSystem* const med = ai->medical;
        if (med == NULL || !(med->restedState > HEALED_ABOVE))
            return score;           // прибавки «вылечен» игра не давала

        const bool robotRace = IsRobot(me);
        if (!HasRoboticParts(med, robotRace))
            return score;           // чинить в ремкровати нечего

        // Кровать не чинит (нет питания) - решает игра, иначе лежать вечно.
        if (!(RepairRate(bed) > 0.0f))
            return score;

        if (RoboticPartsRepaired(med, robotRace))
            return score;           // починился - пусть встаёт

        return score - HEALED_BONUS;
    }


    float SafeStayUntilRepaired(AI* ai, float score)
    {
        __try
        {
            return StayUntilRepaired(ai, score);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static LONG told = 0;
            if (InterlockedExchange(&told, 1) == 0)
                ErrorLog("AIFixed: repair bed stay check failed once - the game's choice is kept");
            return score;
        }
    }


    float (*g_origGetOutHealed)(AI*, const hand&, const Ogre::Vector3&) = NULL;

    float GetOutHealed_hook(AI* self, const hand& subject, const Ogre::Vector3& v)
    {
        const float score = g_origGetOutHealed(self, subject, v);
        if (!g_skeletonBeds || self == NULL)
            return score;
        return SafeStayUntilRepaired(self, score);
    }


    // ---------------------------------------------------------------
    // 2. Саморемонт при износе (22.10.2026, просьба игроков)
    //
    // Скелет или живой с протезами, у кого износ какой-нибудь робот-части
    // выше порога (SelfRepairWear, 0-100, по умолчанию 50), сам идёт на
    // ближайшую свободную ремкровать с питанием - тот же приказ «лечь в
    // кровать», что даёт игрок (USE_BED_ORDER), прежние приказы
    // сбрасываются, постоянные работы остаются. Лежит до полной починки
    // (правка 1б), потом наш приказ снимается - и он встаёт к своим делам.
    //
    // Не трогаем: тех, кто в бою, без сознания, лежит, несёт кого-то или
    // кого несут, уже в кровати или тюрьме. Игрок отменил приказ -
    // повторяем не чаще раза в 20 с и не больше 3 раз, пока износ не
    // опустится ниже порога.
    //
    // Раз в 3 с из обновления интерфейса - оттуда игра отдаёт приказы игрока.
    // ---------------------------------------------------------------

    const DWORD SELF_REPAIR_SCAN_MS = 3000;
    const DWORD SELF_REPAIR_RETRY_MS = 20000;
    const int SELF_REPAIR_TRIES = 3;

    struct SelfRepairState
    {
        DWORD ordered;      // когда отдали приказ, 0 - ещё не отдавали
        DWORD lastOrder;    // когда отдали последний (и при смене кровати)
        int tries;          // сколько раз отдавали
        bool sent;          // идёт к кровати или лежит по нашему приказу
        hand bed;           // serial-bearing identity; resolve only via live list
        int noted;          // что последним писали в журнал (повторы не пишем)
        DWORD getUpAt;      // когда велели встать после починки
        int getUpTries;
        int care;
        unsigned inputSerial;
        bool interrupted;
        float progress;
        DWORD progressAt;
        bool failed;
        bool foodPaused;
    };

    enum { NOTED_NONE, NOTED_NO_BED, NOTED_SENT, NOTED_HEADING, NOTED_NOT_GOING };

    // Сколько ждём, пока приказ дойдёт до ИИ и персонаж пойдёт.
    const DWORD SELF_REPAIR_GRACE_MS = 5000;

    // Брони саморемонта: кровать -> кто на неё отправлен и ещё идёт. Без неё
    // за один проход все отправлялись на одну кровать (22.10.2026): пока
    // никто не дошёл, она для всех «свободна».
    typedef std::map<hand, hand, HandIdentityLess> ReservationMap;
    ReservationMap g_reserved;

    ReservedSet ReservedByOthers(Character* c)
    {
        ReservedSet out;
        const hand who(c);
        for (ReservationMap::const_iterator it = g_reserved.begin(); it != g_reserved.end(); ++it)
            if (!SameIdentity(it->second, who))
                out.insert(it->first);
        return out;
    }

    // A recycled address must not inherit another character's attempts/order.
    typedef std::map<hand, SelfRepairState, HandIdentityLess> RepairStateMap;
    RepairStateMap g_selfRepairState;


    float WorstWear(MedicalSystem* med, bool robotRace)
    {
        float worst = 0.0f;
        const lektor<MedicalSystem::HealthPartStatus*>& parts = med->anatomy;
        for (uint32_t i = 0; i < parts.size(); ++i)
        {
            const MedicalSystem::HealthPartStatus* const p = parts[i];
            if (p != NULL && IsRoboticPart(p, robotRace) && !p->isDead() && p->wearDamage > worst)
                worst = p->wearDamage;
        }
        return worst;
    }

    AIFixedRest::Health RestHealth(MedicalSystem* med,bool robotRace)
    {
        AIFixedRest::Health health;
        if(!med||med->anatomy.size()==0||med->anatomy.size()>64){health.valid=false;return health;}
        for(uint32_t i=0;i<med->anatomy.size();++i)
        {
            MedicalSystem::HealthPartStatus* p=med->anatomy[i];if(!p)continue;
            health.Part(IsRoboticPart(p,robotRace),p->isDead(),p->selfHealing,
                p->derivedFleshHealthPercent,p->wearDamage,p->flesh,p->bandaging,p->_maxHealth*p->age*p->HPMult,p->fleshStun);
        }
        return health;
    }
    AIFixedRest::Care RestNeeded(const AIFixedRest::Health& health)
    {return AIFixedRest::Needed(health,g_autoSleep,g_selfRepair,g_repairHealth,g_sleepBelow*.01f,g_robotBelow*.01f,(float)g_selfRepairWear,g_healAbove*.01f);}
    bool OwnRestOrderMatches(Character* c,OrdersReceiver* orders,const SelfRepairState& state)
    {
        if(state.interrupted||InputSequence(c)!=state.inputSerial)return false;
        Tasker* first=orders->getFirstOrder();
        if(!first)return c->inSomething==IN_BED&&SameIdentity(c->inWhat,state.bed)&&!orders->hasPlayerOrders();
        if(orders->orders.getSecondTask()!=NULL)return false;
        if(state.getUpAt!=0&&first->key()==GET_OUT_OF_BED)return SameIdentity(first->subject,c->handle);
        return first->key()==USE_BED_ORDER&&SameIdentity(first->subject,state.bed);
    }


    // Кровать из записи - только если игра её всё ещё видит рядом: по
    // сохранённому указателю не обращаемся, пока не нашли его в живом списке.
    Building* LiveRepairBed(AI* ai, const hand& stored)
    {
        if (SameIdentity(stored, hand()))
            return NULL;
        lektor<Building*>& machines = Machines();
        ai->getAllLocalMachines(machines, (1u << BF_SKELETON_BED)|(1u << BF_BED));
        for (uint32_t i = 0; i < machines.size(); ++i)
            if (machines[i] != NULL && SameIdentity(hand(machines[i]), stored))
                return machines[i];
        return NULL;
    }


    void NoteSelfRepair(Character* c, const std::string& what)
    {
        if (g_debug)
            DebugLog("AIFixed: self-repair " + c->displayName + " - " + what);
    }


    // Один персонаж. false - запись о нём больше не нужна.
    bool SelfRepairOne(Character* c, SelfRepairState& state, DWORD now)
    {
        if(!state.sent&&((!g_selfRepair&&!g_autoSleep&&!g_repairHealth)||g_restConflict))return false;
        MedicalSystem* med=c->getMedical();OrdersReceiver* orders=c->getOrdersReciever();
        if(!med||!orders)return false;
        const bool robotRace=IsRobot(c);
        const AIFixedRest::Health health=RestHealth(med,robotRace);
        if(!health.valid)return false;
        const AIFixedRest::Care needed=RestNeeded(health);
        const AIFixedRest::Care care=state.care?static_cast<AIFixedRest::Care>(state.care):AIFixedRest::Repair;
        if(state.ordered&&InputSequence(c)!=state.inputSerial){state.interrupted=true;state.sent=false;}
        if(state.interrupted)return needed!=AIFixedRest::None;
        const bool owned=state.sent&&OwnRestOrderMatches(c,orders,state);
        if(state.sent&&!owned){state.interrupted=true;state.sent=false;return needed!=AIFixedRest::None;}
        const bool foodPriority=!AIFixedRest::NutritionAllows(robotRace,robotRace?1.0f:med->getHungerSpeedModifier());
        if(c->inSomething==IN_BED)
        {
            if(!owned)return needed!=AIFixedRest::None;
            if(!SameIdentity(c->inWhat,state.bed))return false;
            Building* bed=c->inWhat.getBuilding();
            const bool working=bed&&(care!=AIFixedRest::Repair||RepairRate(bed)>0);
            const bool active=care==AIFixedRest::Sleep?g_autoSleep:(g_selfRepair||g_repairHealth);
            const float progress=care==AIFixedRest::Sleep?health.fleshRatio*100:-health.robotMissing-health.wear;
            // New damage lowers the comparison baseline; subsequent healing is
            // progress even before reaching the patient's earlier health peak.
            if(progress<state.progress-.01f)state.progress=progress;
            if(state.progressAt==0||progress>state.progress+.01f){state.progress=progress;state.progressAt=now;}
            const bool stalled=state.progressAt!=0&&now-state.progressAt>300000;
            const bool complete=AIFixedRest::Complete(health,care,g_healAbove*.01f);
            if(complete||!working||!active||stalled||foodPriority)
            {
                state.foodPaused=foodPriority&&!complete&&working&&active&&!stalled;
                if(!complete)state.failed=!state.foodPaused;
                if(state.getUpTries>=SELF_REPAIR_TRIES)return false;
                if(state.getUpAt&&now-state.getUpAt<SELF_REPAIR_GRACE_MS)return true;
                OwnRestOrder(orders,GET_OUT_OF_BED,c->handle,c->getPosition());
                state.getUpAt=now;++state.getUpTries;
            }
            return true;
        }
        if(state.getUpAt)
        {
            if(state.foodPaused)
            {
                state.sent=false;state.bed=hand();state.getUpAt=0;state.getUpTries=0;
                state.lastOrder=now;state.foodPaused=false;return needed!=AIFixedRest::None;
            }
            if(state.failed){state.interrupted=true;state.sent=false;return needed!=AIFixedRest::None;}
            return false;
        }
        if(foodPriority)
        {
            if(owned){OwnRestOrder(orders,GET_OUT_OF_BED,c->handle,c->getPosition());state.sent=false;state.bed=hand();state.lastOrder=now;}
            return needed!=AIFixedRest::None;
        }
        if(c->inSomething!=IN_NOTHING||c->isDead()||c->isUnconcious()||c->isDown()||
           c->isCarryingSomething||c->isBeingCarried()||c->isInCombatMode(true,true))return state.ordered!=0;
        if(g_restConflict||!g_newHooksReady||InputSequence(c)==~0u)return false;
        if(orders->hasPlayerOrders()&&!owned)return needed!=AIFixedRest::None;
        if(needed==AIFixedRest::None&&!state.sent)return false;
        AI* ai=c->getAI();if(!ai)return false;
        if(state.sent)
        {
            Building* target=LiveRepairBed(ai,state.bed);
            const bool active=care==AIFixedRest::Sleep?g_autoSleep:(g_selfRepair||g_repairHealth);
            const RestBedContext context(c);AIFixedRest::Bed candidate;
            const bool valid=active&&RestBedAllowed(target,context,care,candidate);
            if(valid&&now-state.lastOrder<60000)
            {g_reserved[state.bed]=hand(c);return true;}
            // Retire only our sole exact order. Never clear another command or job.
            OwnRestOrder(orders,GET_OUT_OF_BED,c->handle,c->getPosition());state.sent=false;state.bed=hand();
            state.lastOrder=now;return true;
        }
        if(needed==AIFixedRest::None)return false;
        if(state.tries>=SELF_REPAIR_TRIES||(state.lastOrder&&now-state.lastOrder<SELF_REPAIR_RETRY_MS))return true;
        Building* bed=NULL;bool lookupFailed=false;
        {
            // Catch native lookup faults inside this allocation-owning scope.
            // Destroy the reservation copy before any later native command.
            const ReservedSet reserved=ReservedByOthers(c);
            bed=SafeFindRestBed(ai,c,&reserved,needed,&lookupFailed);
        }
        if(lookupFailed){state.interrupted=true;return true;}
        if(!bed)return true;
        state.inputSerial=InputSequence(c);
        OwnRestOrder(orders,USE_BED_ORDER,bed->handle,bed->getPosition());
        state.bed=hand(bed);state.sent=true;state.care=needed;state.ordered=state.lastOrder=now;++state.tries;
        state.progressAt=now;state.progress=needed==AIFixedRest::Sleep?health.fleshRatio*100:-health.robotMissing-health.wear;
        g_reserved[state.bed]=hand(c);NoteSelfRepair(c,"assigned a nearby eligible bed");return true;
    }


    // Под защитой: не совпала раскладка или персонаж умер на полпути -
    // пропускаем его в этом проходе. Объектов с деструкторами здесь нет.
    bool SelfRepairOneCpp(Character* c,SelfRepairState& state,DWORD now)
    {
        try{return SelfRepairOne(c,state,now);}
        catch(const std::exception&){ErrorLog("AIFixed: rest operation failed; episode retired");return false;}
    }
    bool SafeSelfRepairOne(Character* c, SelfRepairState& state, DWORD now)
    {
        __try
        {
            return SelfRepairOneCpp(c, state, now);
        }
        __except (NativeFaultFilter(GetExceptionCode()))
        {
            static LONG told = 0;
            if (InterlockedExchange(&told, 1) == 0)
                ErrorLog("AIFixed: self-repair check failed once - that character was skipped");
            return false;
        }
    }


    bool ReadRestClock(GameWorld* world,DWORD* tick)
    {
        __try
        {
            const bool paused=world->isPaused();
            const double seconds=world->getTimeStamp_inGameHours().getTotalHours()*world->getLengthOfHourInRealSeconds();
            unsigned value=0;
            if(!g_restClock.Sample(seconds,paused,value))return false;
            *tick=static_cast<DWORD>(value);return true;
        }
        __except(NativeFaultFilter(GetExceptionCode()))
        {
            g_restClock.anchored=false;
            static LONG told=0;
            if(InterlockedExchange(&told,1)==0)ErrorLog("AIFixed: simulation clock unavailable; rest scan skipped");
            return false;
        }
    }
    bool ReadGateClock(GameWorld* world,DWORD* tick)
    {
        __try
        {
            const bool paused=world->isPaused();
            const double seconds=world->getTimeStamp_inGameHours().getTotalHours()*world->getLengthOfHourInRealSeconds();
            unsigned value=0;
            if(!g_gateClock.Sample(seconds,paused,value))return false;
            *tick=static_cast<DWORD>(value);return true;
        }
        __except(NativeFaultFilter(GetExceptionCode()))
        {
            g_gateClock.anchored=false;
            static LONG told=0;
            if(InterlockedExchange(&told,1)==0)ErrorLog("AIFixed: simulation clock unavailable; gate scan skipped");
            return false;
        }
    }
    void SelfRepairScan(DWORD now)
    {
        if (ou == NULL || ou->player == NULL)
            return;
        const bool restConflict=GetModuleHandleA("SleepFix.dll")!=NULL;
        const bool gateConflict=GetModuleHandleA("GateFix.dll")!=NULL;
        if(restConflict&&!g_restConflict)ErrorLog("AIFixed: SleepFix.dll detected; automatic rest suppressed");
        if(gateConflict&&!g_gateConflict)ErrorLog("AIFixed: GateFix.dll detected; gate assistance suppressed");
        g_restConflict=restConflict;g_gateConflict=gateConflict;

        const lektor<Character*>& squad = ou->player->playerCharacters;
        RepairStateMap kept;

        // Брони прошлого прохода: кто ещё идёт - подтвердит свою заново.
        g_reserved.clear();
        for (RepairStateMap::const_iterator it = g_selfRepairState.begin();
             it != g_selfRepairState.end(); ++it)
            if (it->second.sent && !SameIdentity(it->second.bed, hand()))
                g_reserved[it->second.bed] = it->first;

        // Reserve capacity for existing episodes even if the squad is reordered.
        // New actors cannot displace an owned order before its cleanup runs.
        size_t newBudget=256-(std::min)(size_t(256),g_selfRepairState.size());
        for (uint32_t i = 0; i < squad.size(); ++i)
        {
            Character* const c = squad[i];
            if (c == NULL)
                continue;

            SelfRepairState state = { 0, 0, 0, false, hand(), NOTED_NONE, 0, 0 };
            const hand key(c);
            if(kept.find(key)!=kept.end())continue;
            RepairStateMap::const_iterator old = g_selfRepairState.find(key);
            if (old != g_selfRepairState.end())
                state = old->second;
            else if(!newBudget)continue;

            if (SafeSelfRepairOne(c, state, now))
            {
                kept[key] = state;
                if(old==g_selfRepairState.end())--newBudget;
            }
        }

        g_selfRepairState.swap(kept);
        // Remove identities no longer in the player's live list. Keep sequence
        // numbers for current actors even when they have no rest episode.
        IntentGuard guard;
        for(IntentMap::iterator it=g_intents.begin();it!=g_intents.end();)
        {
            bool live=false;
            for(uint32_t i=0;i<squad.size();++i)
                if(squad[i]&&SameIdentity(hand(squad[i]),it->first)){live=true;break;}
            if(!live)g_intents.erase(it++);else ++it;
        }
    }


    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;
    DWORD g_lastSelfRepairScan = 0;

    void InvalidateWorld()
    {
        InterlockedExchange(&g_worldActiveAt, 0);
        InterlockedIncrement(&g_loadGeneration);
    }

    bool WorldActive(DWORD now)
    {
        const DWORD last = static_cast<DWORD>(InterlockedCompareExchange(&g_worldActiveAt, 0, 0));
        return g_lifecycleReady && InterlockedCompareExchange(&g_loading, 0, 0) == 0 &&
               last != 0 && now - last <= 300 &&
               InterlockedCompareExchange(&g_worldReadyGeneration, 0, 0) ==
               InterlockedCompareExchange(&g_loadGeneration, 0, 0);
    }

    void ResetSessionState()
    {
        // GUI-owned metadata only: never dereference a stored game object.
        g_selfRepairState.clear();
        g_reserved.clear();
        g_lastSelfRepairScan = 0;
        g_restClock.Reset();
        g_gateClock.Reset();
        g_gateStates.clear();g_gateCursor=hand();g_lastGateScan=0;
        {IntentGuard guard;g_intents.clear();g_moveIntents.clear();g_gateDoorChanges.clear();g_intentOverflow=false;}
        EnterCriticalSection(&g_noteLock);
        g_notedSearch.clear();
        g_notedBed.clear();
        g_lastSnap.clear();
        LeaveCriticalSection(&g_noteLock);
    }

    bool SelfRepairSessionReady(DWORD now)
    {
        const LONG generation = InterlockedCompareExchange(&g_loadGeneration, 0, 0);
        if (generation != g_seenGeneration)
        {
            ResetSessionState();
            g_seenGeneration = generation;
            g_sessionActive = false;
        }
        if (ou == NULL)
        {
            if (g_sessionActive) ResetSessionState();
            g_sessionActive = false;
            return false;
        }
        // A slow frame or a paused loop is not a world teardown. Preserve our
        // sent-order ownership/retry budget and merely skip access/commands.
        if (!WorldActive(now)) return false;
        // Player fields are read only after a fresh native-world heartbeat.
        if (ou->player == NULL)
        {
            if (g_sessionActive) ResetSessionState();
            g_sessionActive = false;
            return false;
        }
        g_sessionActive = true;
        return true;
    }

    void (*g_origWorld)(GameWorld*, float) = NULL;
    void World_hook(GameWorld* self, float time)
    {
        const LONG generation = InterlockedCompareExchange(&g_loadGeneration, 0, 0);
        g_origWorld(self, time);
        if (self == ou && InterlockedCompareExchange(&g_loading, 0, 0) == 0 &&
            generation == InterlockedCompareExchange(&g_loadGeneration, 0, 0))
        {
            InterlockedExchange(&g_worldReadyGeneration, generation);
            InterlockedExchange(&g_worldActiveAt, static_cast<LONG>(GetTickCount()));
        }
    }

    void (*g_origLoadGame)(SaveFileSystem*, const std::string&) = NULL;
    void LoadGame_hook(SaveFileSystem* self, const std::string& name)
    {
        InterlockedIncrement(&g_loading);
        InvalidateWorld();
        __try { g_origLoadGame(self, name); }
        __finally { InvalidateWorld(); InterlockedDecrement(&g_loading); }
    }

    void (*g_origNewGame)(SaveFileSystem*) = NULL;
    void NewGame_hook(SaveFileSystem* self)
    {
        InterlockedIncrement(&g_loading);
        InvalidateWorld();
        __try { g_origNewGame(self); }
        __finally { InvalidateWorld(); InterlockedDecrement(&g_loading); }
    }

    void (*g_origResetGame)(GameWorld*) = NULL;
    void ResetGame_hook(GameWorld* self)
    {
        InterlockedIncrement(&g_loading);
        InvalidateWorld();
        __try { g_origResetGame(self); }
        __finally { InvalidateWorld(); InterlockedDecrement(&g_loading); }
    }

    void (*g_origClearWorld)(GameWorld*) = NULL;
    void ClearWorld_hook(GameWorld* self)
    {
        InterlockedIncrement(&g_loading);
        InvalidateWorld();
        __try { g_origClearWorld(self); }
        __finally { InvalidateWorld(); InterlockedDecrement(&g_loading); }
    }

    void GateUpdate(DWORD wallNow)
    {
        if(wallNow-g_lastGateScan<250)return;
        g_lastGateScan=wallNow;
        bool pending=false;
        {
            IntentGuard guard;
            if(!g_gatePassage)g_moveIntents.clear();
            pending=!g_moveIntents.empty();
        }
        if(g_gateStates.empty()&&!pending)
        {
            // No native clock polling while there is no gate work. A future
            // episode starts from its first active scanner sample, not idle time.
            g_gateClock.anchored=false;return;
        }
        DWORD simulationNow=0;
        if(ReadGateClock(ou,&simulationNow))GateScan(simulationNow);
    }
    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);

        const DWORD now = GetTickCount();
        if (!SelfRepairSessionReady(now))
            return;
        GateUpdate(now);
        if(!g_selfRepair&&!g_autoSleep&&!g_repairHealth&&!g_gatePassage&&
           g_selfRepairState.empty()&&g_gateStates.empty())return;
        if (now - g_lastSelfRepairScan < SELF_REPAIR_SCAN_MS)
            return;
        g_lastSelfRepairScan = now;
        DWORD simulationNow=0;
        if(ReadRestClock(ou,&simulationNow))SelfRepairScan(simulationNow);
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
        if (g_debug) SafeNoteDrop(self, ragdoll, removeOnly, _ReturnAddress());
        g_origDrop(self, ragdoll, removeOnly);
    }


    void (*g_origSetBedMode)(Character*, bool, UseableStuff*) = NULL;

    void NoteBed(Character* self, bool on, UseableStuff* bed, const void* from)
    {
        if (!IsRobot(self) || !Changed(g_notedBed, self, on ? static_cast<const void*>(bed) : NULL))
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
        if (g_debug) SafeNoteBed(self, on, bed, _ReturnAddress());
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
        if (g_debug) SafeNoteImpossible(self, _ReturnAddress());
        g_origTaskImpossible(self);
    }


    // Снимок починки в ремкровати (22.10.2026, тоже только при Debug=1):
    // робо-робот, уложенный кем-то в ремкровать, лежал на полу и не чинился.
    // MedicalSystem::medicalUpdate чинит в ремкровати, если персонаж «в
    // кровати» (inSomething = IN_BED), inWhat - ремкровать, а множитель
    // починки = getProductionMult() кровати * calculateEfficiencyMult() её
    // UseableStuff. Раз в 5 с на скелета игрока в кровати пишем эти значения.

    void Snap(MedicalSystem* med)
    {
        Character* const c = med->me;
        if (c == NULL || c->inSomething != IN_BED || !c->isPlayerCharacter() ||
            !HasRoboticParts(med, IsRobot(c)))
            return;

        const DWORD now = GetTickCount();
        EnterCriticalSection(&g_noteLock);
        DWORD& last = g_lastSnap[c];
        const bool due = now - last >= 5000;
        if (due)
            last = now;
        LeaveCriticalSection(&g_noteLock);
        if (!due)
            return;

        Building* const bed = c->inWhat.getBuilding();
        UseableStuff* const useable = bed != NULL ? bed->getUseableStuff() : NULL;
        float worstWear = 0.0f;
        for (uint32_t i = 0; i < med->anatomy.size(); ++i)
        {
            const MedicalSystem::HealthPartStatus* const p = med->anatomy[i];
            if (p != NULL && IsRoboticPart(p, IsRobot(c)) && p->wearDamage > worstWear)
                worstWear = p->wearDamage;
        }
        char line[448];
        _snprintf_s(line, _TRUNCATE,
                    "repaired %d, worst wear %.1f, rested %.2f, function %d, production %.3f, efficiency %.3f, power %d, distance %.1f, down %d, unconscious %d",
                    RoboticPartsRepaired(med, IsRobot(c)) ? 1 : 0, worstWear, med->restedState,
                    bed != NULL ? static_cast<int>(bed->getSpecialFunction()) : -1,
                    bed != NULL ? bed->getProductionMult() : -1.0f,
                    useable != NULL ? EfficiencyOf(useable) : -1.0f,
                    useable != NULL ? (useable->hasPower() ? 1 : 0) : -1,
                    bed != NULL ? (bed->getPosition() - c->getPosition()).length() : -1.0f,
                    c->isDown() ? 1 : 0, c->isUnconcious() ? 1 : 0);
        DebugLog("AIFixed: in bed " + c->displayName + ": " +
                 (bed != NULL ? bed->displayName : std::string("?")) + ", " + line);
    }

    void SafeSnap(MedicalSystem* med)
    {
        __try
        {
            Snap(med);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void (*g_origMedical)(MedicalSystem*, float) = NULL;

    void Medical_hook(MedicalSystem* self, float frameTime)
    {
        g_origMedical(self, frameTime);
        if (g_debug) SafeSnap(self);
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
    api->toggle(api, "AIFixed", "SelfRepair", Tr("Self-repair when worn"),
                Tr("A skeleton, or someone with prosthetics, whose part wear goes above the threshold goes to the nearest free powered repair bed by himself, stays until fully repaired and then goes back to work. Not in combat, not while carrying someone."), 1, 0);
    api->integer(api, "AIFixed", "SelfRepairWear", Tr("Wear threshold"),
                 Tr("Wear of any robotic part above which the character goes to repair himself."), 50, 0, 100, 0);
    api->section(api, Tr("Automatic rest"));
    api->toggle(api,"AIFixed","AutoSleep",Tr("Sleep when wounded"),Tr("Bandaged biological wounds trigger a nearby ordinary bed. Healthy workers and manual travel orders are left alone. Off by default."),0,0);
    api->integer(api,"AIFixed","SleepBelowHealth",Tr("Sleep below health (%)"),Tr("Lowest biological part health that starts automatic sleep."),90,10,99,0);
    api->integer(api,"AIFixed","WakeAboveHealth",Tr("Wake above health (%)"),Tr("Biological health required to finish our sleep order. Prosthetic health is handled separately. Cannot be below the sleep threshold."),99,10,100,0);
    api->toggle(api,"AIFixed","RepairHealth",Tr("Repair damaged robotic parts"),Tr("Robotic part health can trigger a powered repair bed independently of wear. Works for skeletons and prosthetics. Off by default."),0,0);
    api->integer(api,"AIFixed","RepairBelowHealth",Tr("Repair below robotic health (%)"),Tr("Lowest robotic part health that starts automatic repair when enabled."),90,10,99,0);
    api->integer(api,"AIFixed","BedSearchRadius",Tr("Bed search radius (m)"),Tr("Maximum distance for automatic rest and replacement rescue beds. Only the same settlement is considered."),100,10,500,0);
    api->toggle(api,"AIFixed","OwnBedsOnly",Tr("Own beds only"),Tr("Use only your faction's beds. If disabled, allied beds may also be considered; enemy and neutral beds are excluded."),1,0);
    api->section(api,Tr("Gate passage"));
    api->toggle(api,"AIFixed","GatePassage",Tr("Open own gates while travelling"),Tr("A stalled explicit movement order can open a closed gate belonging to your faction, pass and resume its destination. No teleporting. Off by default."),0,0);
    api->toggle(api,"AIFixed","GateOpenLocked",Tr("Allow opening locked gates"),Tr("Allow passage to unlock your own gates. Disabled means locked gates remain untouched."),0,0);
    api->toggle(api,"AIFixed","GateCloseAfterPassage",Tr("Close after passage"),Tr("Request normal closing after passage only when the gate area is clear. Manual orders and lock changes cancel our assistance."),1,0);
    api->toggle(api,"AIFixed","GateRestoreLock",Tr("Restore original lock"),Tr("If we unlocked the gate, restore its lock only after it closes normally. Requires closing after passage."),1,0);
    api->integer(api,"AIFixed","GateSearchRadius",Tr("Gate search radius (m)"),Tr("Local search distance around a stalled traveller; gates outside the movement corridor are ignored."),30,5,100,0);
    api->integer(api,"AIFixed","GateCloseDelay",Tr("Gate close delay (s)"),Tr("Minimum delay after crossing before requesting closure. Nearby characters postpone it."),3,1,30,0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "AIFixed", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: for whom a repair bed was looked for and which one was chosen."), 0, MCM_RESTART);
}


__declspec(dllexport) void startPlugin()
{
    InitializeCriticalSection(&g_noteLock);
    InitializeCriticalSection(&g_intentLock);
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("AIFixed: disabled in the ini");
        return;
    }

    const bool worldHook = KenshiLib::SUCCESS ==
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff),
                          World_hook, &g_origWorld);
    const bool loadHook = KenshiLib::SUCCESS ==
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),
                          LoadGame_hook, &g_origLoadGame);
    const bool newHook = KenshiLib::SUCCESS ==
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),
                          NewGame_hook, &g_origNewGame);
    const bool resetHook = KenshiLib::SUCCESS ==
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::resetGame),
                          ResetGame_hook, &g_origResetGame);
    const bool clearHook = KenshiLib::SUCCESS ==
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_clearAndDestroyGameWorldStuff),
                          ClearWorld_hook, &g_origClearWorld);
    g_lifecycleReady = worldHook && loadHook && newHook && resetHook && clearHook;
    if (!g_lifecycleReady)
        ErrorLog("AIFixed: lifecycle hooks incomplete, automatic self-repair disabled");

    g_restConflict=GetModuleHandleA("SleepFix.dll")!=NULL;
    g_gateConflict=GetModuleHandleA("GateFix.dll")!=NULL;
    if(g_restConflict)ErrorLog("AIFixed: SleepFix.dll present; automatic rest suppressed to avoid competing orders");
    if(g_gateConflict)ErrorLog("AIFixed: GateFix.dll present; gate assistance suppressed to avoid competing orders");
    const bool orderHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&OrdersReceiver::addOrder),OrdersAdd_hook,&g_origOrdersAdd);
    const bool clearOrderHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&OrdersReceiver::clearOrders),OrdersClear_hook,&g_origOrdersClear);
    const bool moveHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&Character::_NV_playerMoveOrderDefault),Move_hook,&g_origMove);
    const bool lockHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&DoorStuff::lockDoor),GateLock_hook,&g_origGateLock);
    const bool unlockHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&DoorStuff::unlockDoor),GateUnlock_hook,&g_origGateUnlock);
    const bool lockButtonHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&DoorStuff::lockButton),GateLockButton_hook,&g_origGateLockButton);
    const bool openButtonHook=KenshiLib::SUCCESS==KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&DoorStuff::openButton),GateOpenButton_hook,&g_origGateOpenButton);
    g_newHooksReady=orderHook&&clearOrderHook;
    g_gateHooksReady=moveHook&&lockHook&&unlockHook&&lockButtonHook&&openButtonHook;
    if(!g_newHooksReady)ErrorLog("AIFixed: intent hooks incomplete; automatic rest/gates suspended");
    if(!g_gateHooksReady)ErrorLog("AIFixed: gate intent hooks incomplete; gate assistance suspended");

    // Хук стоит всегда, выключатель правки читается на ходу.
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&AI::findOwnedFreeBed),
                           FindOwnedFreeBed_hook,
                           &g_origFindOwnedFreeBed))
    {
        ErrorLog("AIFixed: could not hook the bed search, skeletons keep going to ordinary beds");
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&AI::_NV_scoreGetOutOfBedOnceFullyHealed),
                           GetOutHealed_hook,
                           &g_origGetOutHealed))
    {
        ErrorLog("AIFixed: could not hook the get-out-of-bed check, skeletons leave repair beds at once");
    }

    // Саморемонт - из обновления интерфейса; выключатель читается на ходу.
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                           GuiUpdate_hook,
                           &g_origGuiUpdate))
    {
        ErrorLog("AIFixed: could not hook the interface update, no self-repair");
    }

    // Диагностика - только при подробном журнале на старте.
    if (g_debug)
    {
        if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::dropCarriedObject),
                                                     Drop_hook, &g_origDrop) ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::setBedMode),
                                                     SetBedMode_hook, &g_origSetBedMode) ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&AITaskSytem::taskImpossible),
                                                     TaskImpossible_hook, &g_origTaskImpossible) ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&MedicalSystem::medicalUpdate),
                                                     Medical_hook, &g_origMedical))
            ErrorLog("AIFixed: some diagnostic hooks failed - the log will be incomplete");
        else
            DebugLog("AIFixed: diagnostics on (drops, bed mode, impossible tasks of skeleton carriers)");
    }

    DebugLog("AIFixed: installed");
}
