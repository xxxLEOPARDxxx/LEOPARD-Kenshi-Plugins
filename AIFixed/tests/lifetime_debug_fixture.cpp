#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <new>
#include <ModConfigMenu.h>
#include <RestPolicy.h>

struct Character;
struct Building;
struct MedicalSystem;
struct OrdersReceiver;
struct AI;
struct TownBase;
struct UseableStuff;
struct Position {
 float x,y,z;Position(float v=0):x(v),y(0),z(0){}
 Position operator-(const Position& p)const{return Position(x-p.x);}
 float squaredLength()const{return x*x;}
};
namespace Ogre{typedef Position Vector3;}
typedef int TaskType;
enum { IN_NOTHING, IN_BED, USE_BED_ORDER=258, USE_BED=259, GET_OUT_OF_BED=260 };
int nativeLessCalls=0,nativeEqualCalls=0;
struct hand {
    int type; unsigned container,containerSerial,index,serial;void* object;
    hand() : type(0),container(0),containerSerial(0),index(0),serial(0),object(NULL) {}
    hand(Character* c);
    hand(Building* b);
    Building* getBuilding()const{return static_cast<Building*>(object);}
    bool operator<(const hand& h) const {
        ++nativeLessCalls;return index < h.index; // Deliberately does not promise serial ordering.
    }
    bool operator==(const hand& h) const { ++nativeEqualCalls;return index == h.index; }
    bool operator!=(const hand& h) const { return !(*this == h); }
};
struct Character {
 unsigned serial,containerSerial;int inSomething;bool isCarryingSomething,robot;
 MedicalSystem* medical;OrdersReceiver* orders;AI* ai;hand handle,inWhat;
 Character(unsigned s=1):serial(s),containerSerial(1),inSomething(IN_NOTHING),isCarryingSomething(false),robot(true),medical(NULL),orders(NULL),ai(NULL){handle=hand(this);}
 MedicalSystem* getMedical(){return medical;}OrdersReceiver* getOrdersReciever(){return orders;}AI* getAI(){return ai;}
 bool isDead(){return false;}bool isUnconcious(){return false;}bool isDown(){return false;}
 bool isBeingCarried(){return false;}bool isInCombatMode(bool,bool){return false;}
 Position getPosition(){return Position();}TownBase* getCurrentTownLocation(){return NULL;}
 void* getFaction(){return NULL;}
};
struct Building {
 unsigned serial,containerSerial;std::string displayName;hand handle;bool valid;
 Building(unsigned s=1):serial(s),containerSerial(1),displayName("bed"),valid(true){handle=hand(this);}
 Position getPosition(){return Position();}
 int getSpecialFunction(){return 25;}UseableStuff* getUseableStuff();
 TownBase* getTown(){return NULL;}void* getFaction(){return NULL;}
};
hand::hand(Character* c):type(1),container(7),containerSerial(c ? c->containerSerial:0),index((unsigned)(uintptr_t)c),serial(c ? c->serial : 0),object(c){}
hand::hand(Building* b):type(2),container(7),containerSerial(b ? b->containerSerial:0),index((unsigned)(uintptr_t)b),serial(b ? b->serial : 0),object(b){}
template<class T> struct lektor : std::vector<T> {};
lektor<Building*> liveBeds;
lektor<Building*>& Machines(){liveBeds.clear();return liveBeds;}
struct AI {
 Building* candidate;
 void getAllLocalMachines(lektor<Building*>& out,unsigned){if(candidate)out.push_back(candidate);}
};
enum { BF_BED=6,BF_SKELETON_BED=25 };
struct Player { lektor<Character*> playerCharacters; };
struct TimeOfDay {double hours;double getTotalHours()const{return hours;}};
struct GameWorld {
 Player* player;bool paused;double simulationSeconds;
 GameWorld():player(NULL),paused(false),simulationSeconds(0){}
 bool isPaused()const{return paused;}
 TimeOfDay getTimeStamp_inGameHours(){TimeOfDay stamp={simulationSeconds/100.0};return stamp;}
 float getLengthOfHourInRealSeconds(){return 100.0f;}
};
struct ForgottenGUI {};
struct UseableStuff:Building { bool isFreeSlot(const hand&){return true;} };
UseableStuff uGlobal;
UseableStuff* Building::getUseableStuff(){return &uGlobal;}
struct AITaskSytem {};
struct MedicalSystem { bool repaired;float hungerSpeed,repairWear;MedicalSystem():repaired(false),hungerSpeed(1),repairWear(60){}float getHungerSpeedModifier(){return hungerSpeed;} };
struct TaskGoal { int task;int key()const{return task;} };
struct Tasker {int task;hand subject;int key(){return task;}};
struct OrdersReceiver {
 int goal,count,lastTask;bool cleared,shifted;
 Tasker first,second;bool queued,present,failAdd;struct Queue{OrdersReceiver* owner;Tasker* getSecondTask(){return owner->queued?&owner->second:NULL;}}orders;
 OrdersReceiver():goal(USE_BED_ORDER),count(0),lastTask(0),cleared(false),shifted(false),queued(false),present(true),failAdd(false){orders.owner=this;first.task=USE_BED_ORDER;}
 TaskGoal getCurrentGoal(){TaskGoal g={goal};return g;}
 Tasker* getFirstOrder(){return present?&first:NULL;}bool hasPlayerOrders(){return present;}
 void addOrder(int task,const hand& target,const Position&,bool clear,bool shift){if(failAdd)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);++count;lastTask=goal=task;first.task=task;first.subject=target;cleared=clear;shifted=shift;}
};
struct TownBase {std::string displayName;};
struct SaveFileSystem {};

GameWorld* ou=NULL;
bool g_debug=false,g_selfRepair=true,g_lifecycleReady=true,g_sessionActive=false;
bool g_autoSleep=false,g_repairHealth=false,g_ownBedsOnly=true,g_restConflict=false,g_newHooksReady=true,t_ownOrder=false;
bool g_gateConflict=false,g_gatePassage=false;
AIFixedRest::SimulationClock g_restClock,g_gateClock;
int NativeFaultFilter(unsigned code){return code==0xe06d7363?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;}
void ErrorLog(const char*){}
int g_bedRadius=100,g_healAbove=99;
int g_selfRepairWear=50;
volatile LONG g_loadGeneration=0,g_worldActiveAt=0,g_worldReadyGeneration=-1,g_loading=0;
LONG g_seenGeneration=0;
DWORD g_lastSelfRepairScan=0, tick=10000;
DWORD TestTick(){return tick;}
#define GetTickCount TestTick
CRITICAL_SECTION g_noteLock;
typedef std::map<Character*,const void*> NotedMap;
NotedMap g_notedSearch,g_notedBed;
std::map<Character*,DWORD> g_lastSnap;
int calls=0,notes=0,originals=0,seenTries=-1,failures=0,checks=0;
void Check(bool condition,const char* what){++checks;if(!condition){++failures;printf("FAIL: %s\n",what);}}
void SafeNoteDrop(Character*,bool,bool,const void*){++notes;}
void SafeNoteBed(Character*,bool,UseableStuff*,const void*){++notes;}
void SafeNoteImpossible(AITaskSytem*,const void*){++notes;}
void SafeSnap(MedicalSystem*){++notes;}
void OrigDrop(Character*,bool,bool){++originals;}
void OrigBed(Character*,bool,UseableStuff*){++originals;}
void OrigImpossible(AITaskSytem*){++originals;}
void OrigMedical(MedicalSystem*,float){++originals;}
void OrigGui(ForgottenGUI*){++originals;}
void (*g_origDrop)(Character*,bool,bool)=OrigDrop;
void (*g_origSetBedMode)(Character*,bool,UseableStuff*)=OrigBed;
void (*g_origTaskImpossible)(AITaskSytem*)=OrigImpossible;
void (*g_origMedical)(MedicalSystem*,float)=OrigMedical;
void (*g_origGuiUpdate)(ForgottenGUI*)=OrigGui;
void World_hook(GameWorld*,float);
void InvalidateWorld();
void OrigWorld(GameWorld*,float){++originals;}
bool checkInvalidated=false,throwNative=false;
void CheckNativeLoad(){
    if(checkInvalidated)Check(g_worldActiveAt==0 && g_loading>0,"invalidate before native load/newgame");
    World_hook(ou,0); // A reentrant world callback must not reopen the gate while loading.
    if(throwNative)RaiseException(0xE1234567,0,0,NULL);
}
void OrigLoad(SaveFileSystem*,const std::string&){++originals;CheckNativeLoad();}
void OrigNew(SaveFileSystem*){++originals;CheckNativeLoad();}
void (*g_origWorld)(GameWorld*,float)=OrigWorld;
void (*g_origLoadGame)(SaveFileSystem*,const std::string&)=OrigLoad;
void (*g_origNewGame)(SaveFileSystem*)=OrigNew;
void OrigReset(GameWorld*){++originals;CheckNativeLoad();}
void (*g_origResetGame)(GameWorld*)=OrigReset;
void (*g_origClearWorld)(GameWorld*)=OrigReset;
std::string IniPath(){return "fixture.ini";}
void LoadSettings(){}
const char* Tr(const char* s){return s;}
unsigned debugFlags=0;int toggles=0;
void Begin(MCM_Api*,const char*,const char*,const char*,MCM_ApplyFn){}
void Info(MCM_Api*,const char*){}
void Toggle(MCM_Api*,const char*,const char* key,const char*,const char*,int,unsigned flags){
 ++toggles;if(std::string(key)=="Debug")debugFlags=flags;
}
void Integer(MCM_Api*,const char*,const char*,const char*,const char*,int,int,int,unsigned){}

// @STATE@
struct IntentGuard{};
typedef std::map<hand,unsigned,HandIdentityLess> IntentMap;
IntentMap g_intents;std::vector<int> g_moveIntents;bool g_intentOverflow=false;
std::map<int,int> g_gateStates,g_gateDoorChanges;hand g_gateCursor;DWORD g_lastGateScan=0;
void GateScan(DWORD){}
unsigned inputSerial=0;
unsigned InputSequence(Character*){return inputSerial;}
bool IsRobot(Character* c){return c->robot;}
bool HasRoboticParts(MedicalSystem*,bool){return true;}
bool RoboticPartsRepaired(MedicalSystem* med,bool){return med->repaired;}
float WorstWear(MedicalSystem* med,bool){return med->repaired?0:60;}
float RepairRate(Building*){return 1;}
AIFixedRest::Health RestHealth(MedicalSystem* med,bool){AIFixedRest::Health h;h.Part(true,false,false,med->repaired?1:.5f,med->repaired?0:med->repairWear,100,0,100);return h;}
AIFixedRest::Care RestNeeded(const AIFixedRest::Health& h){return AIFixedRest::Needed(h,g_autoSleep,g_selfRepair,g_repairHealth,.9f,.9f,50);}
void NoteSelfRepair(Character*,const std::string&){}
Building* FindRepairBed(AI* ai,Character*,const ReservedSet*){return ai->candidate;}
Building* FindRestBed(AI* ai,Character*,const ReservedSet*,AIFixedRest::Care){return ai->candidate;}
Building* SafeFindRestBed(AI* ai,Character*,const ReservedSet*,AIFixedRest::Care,bool* =NULL){return ai->candidate;}
struct RestBedContext{explicit RestBedContext(Character*){}};
bool RestBedAllowed(Building* bed,const RestBedContext&,AIFixedRest::Care,AIFixedRest::Bed&){return bed&&bed->valid;}
bool SelfRepairOne(Character*,SelfRepairState&,DWORD);
bool exerciseRepairPolicy=false;
bool SafeSelfRepairOne(Character* c,SelfRepairState& state,DWORD now){++calls;seenTries=state.tries;return exerciseRepairPolicy ? SelfRepairOne(c,state,now):true;}
// @LIFECYCLE@
#ifndef HAS_LIFECYCLE
void World_hook(GameWorld*,float){}
void InvalidateWorld(){}
#endif
// @PRODUCTION@
#ifdef HAS_LIFECYCLE
bool CatchLoad(const std::string& name){
 bool caught=false;
 __try{LoadGame_hook(NULL,name);}__except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
 return caught;
}
#endif
bool CatchOwn(OrdersReceiver* orders,const hand& target)
{
 __try{OwnRestOrder(orders,GET_OUT_OF_BED,target,Position());}
 __except(EXCEPTION_EXECUTE_HANDLER){return true;}
 return false;
}

int main(){
 InitializeCriticalSection(&g_noteLock);
 Character c;Building bed;ForgottenGUI gui;UseableStuff u;AITaskSytem task;MedicalSystem med;
 Player player;player.playerCharacters.push_back(&c);GameWorld world;world.player=&player;ou=&world;
 Drop_hook(&c,false,false);SetBedMode_hook(&c,true,&u);TaskImpossible_hook(&task);Medical_hook(&med,0);
 Check(notes==0,"debug off skips all four diagnostic bodies");Check(originals==4,"debug off retains all original calls");
 g_debug=true;notes=originals=0;
 Drop_hook(&c,false,false);SetBedMode_hook(&c,true,&u);TaskImpossible_hook(&task);Medical_hook(&med,0);
 Check(notes==4 && originals==4,"debug on preserves diagnostics and originals");
 g_debug=false;notes=0;Medical_hook(&med,0);Check(notes==0,"live debug off stops already-installed medical diagnostics");
 MCM_Api api={0};api.version=6;api.beginMod=Begin;api.info=Info;api.section=Info;api.toggle=Toggle;api.integer=Integer;
 MCM_Describe(&api);Check(toggles==11 && (debugFlags & MCM_RESTART)!=0,"debug capture advertises required restart through actual API");
 calls=0;GuiUpdate_hook(&gui);Check(calls==0,"no world heartbeat prevents squad scan despite stale ou");

 SelfRepairState prior={0,0,2,true,&bed,0,0,0};g_selfRepairState[&c]=prior;
 c.serial=2;seenTries=-1;SelfRepairScan(tick);Check(seenTries==0,"same address with new character serial must not inherit attempts");
 g_selfRepairState.clear();g_selfRepairState[&c]=prior;c.containerSerial=2;
 seenTries=-1;SelfRepairScan(tick);Check(seenTries==0,"new character container serial must not inherit attempts");
 AI ai;ai.candidate=&bed;hand oldBed(&bed);bed.serial=2;
 Check(LiveRepairBed(&ai,oldBed)==NULL,"recycled building address cannot validate an old reservation");
 Check(LiveRepairBed(&ai,hand(&bed))==&bed,"current building identity remains eligible");
 oldBed=hand(&bed);++bed.containerSerial;
 Check(LiveRepairBed(&ai,oldBed)==NULL,"new building container serial cannot validate old reservation");

#ifdef HAS_LIFECYCLE
 World_hook(&world,0);calls=0;GuiUpdate_hook(&gui);Check(calls==1,"fresh world heartbeat permits scan");
 OrdersReceiver orders;c.medical=&med;c.orders=&orders;c.ai=&ai;c.inSomething=IN_BED;med.repaired=true;exerciseRepairPolicy=true;
 prior.bed=hand(&bed);orders.first.subject=prior.bed;c.inWhat=prior.bed;g_selfRepairState[&c]=prior;g_reserved[&bed]=&c;g_notedSearch[&c]=&bed;g_lastSnap[&c]=1;
 tick+=301;calls=0;GuiUpdate_hook(&gui);
 Check(calls==0,"stale heartbeat skips commands and scans");
 Check(!g_selfRepairState.empty() && g_selfRepairState.begin()->second.sent && g_selfRepairState.begin()->second.tries==2 && !g_reserved.empty(),"same-generation slow frame preserves sent ownership attempts and reservations");
 Check(!g_notedSearch.empty() && !g_lastSnap.empty(),"timeout alone does not pretend that a world was destroyed");
 Check(orders.count==0,"stale heartbeat emits no get-up order even when repaired");
 World_hook(&world,0);calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && orders.count==0,"fresh heartbeat after short stall retains normal three-second scan interval");
 tick+=2700;World_hook(&world,0);GuiUpdate_hook(&gui);
 Check(orders.count==1 && orders.lastTask==GET_OUT_OF_BED && orders.cleared && !orders.shifted,"preserved sent ownership reaches actual full-repair get-out order after heartbeat resumes");
 exerciseRepairPolicy=false;c.inSomething=IN_NOTHING;
 World_hook(&world,0);GuiUpdate_hook(&gui);g_selfRepairState[&c]=prior;g_reserved[&bed]=&c;
 unsigned gateTick=0;g_gateClock.Sample(10,false,gateTick);g_gateClock.Sample(11,false,gateTick);g_gateStates[1]=1;g_moveIntents.push_back(1);
 checkInvalidated=true;LoadGame_hook(NULL,"same-save");calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty() && g_reserved.empty(),"same-address load clears state and waits for new world");
 Check(g_gateStates.empty()&&g_moveIntents.empty()&&!g_gateClock.anchored&&g_gateClock.milliseconds==1,"native load lifecycle resets gate clock and pending episodes before next world");
 World_hook(&world,0);GuiUpdate_hook(&gui);g_selfRepairState[&c]=prior;NewGame_hook(NULL);calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty(),"new game clears state and closes readiness");
 World_hook(&world,0);GuiUpdate_hook(&gui);g_selfRepairState[&c]=prior;ResetGame_hook(&world);calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty(),"world reset invalidates before teardown without heartbeat timeout");
 World_hook(&world,0);GuiUpdate_hook(&gui);g_selfRepairState[&c]=prior;ClearWorld_hook(&world);calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty(),"world clear invalidates before destruction without heartbeat timeout");
 checkInvalidated=false;World_hook(&world,0);world.player=NULL;calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty(),"missing player blocks and clears session");
 world.player=&player;g_lifecycleReady=false;World_hook(&world,0);calls=0;GuiUpdate_hook(&gui);
 Check(calls==0,"failed lifecycle hook registration fails closed");g_lifecycleReady=true;
 ou=reinterpret_cast<GameWorld*>(1);g_worldActiveAt=0;calls=0;GuiUpdate_hook(&gui);
 Check(calls==0,"inactive world does not dereference a stale non-null world pointer");ou=&world;
 throwNative=true;bool caught=CatchLoad("fails");
 Check(caught && g_loading==0 && g_worldActiveAt==0,"native load exception restores loading guard and closes gate");
 throwNative=false;
#endif
 // Exact production ownership/scheduler decisions, independent of heartbeat tests.
 OrdersReceiver ownOrders;c.orders=&ownOrders;c.medical=&med;c.ai=&ai;c.inSomething=IN_BED;
 c.inWhat=hand(&bed);ownOrders.first.subject=c.inWhat;med.repaired=true;
 SelfRepairState owned={tick,tick,1,true,c.inWhat,0,0,0};
 Check(OwnRestOrderMatches(&c,&ownOrders,owned),"sole exact owned bed order matches");
 ownOrders.queued=true;
 Check(!OwnRestOrderMatches(&c,&ownOrders,owned),"queued player command prevents replacing a bed order");
 ownOrders.queued=false;ownOrders.first.subject=hand();
 Check(!OwnRestOrderMatches(&c,&ownOrders,owned),"same task with different target is not ours");
 ownOrders.first.subject=owned.bed;++inputSerial;med.repaired=false;
 Check(SelfRepairOne(&c,owned,tick)&&owned.interrupted&&!owned.sent&&ownOrders.count==0,"manual order cancels our injury episode without replacement");
 ownOrders.present=false;ownOrders.goal=29;c.inSomething=IN_NOTHING;
 Check(SelfRepairOne(&c,owned,tick+30000)&&ownOrders.count==0,"cancelled wounded traveller is not repeatedly sent to bed");
 med.repaired=true;
 Check(!SelfRepairOne(&c,owned,tick+60000),"healed episode retires cancellation metadata");
 SelfRepairState fresh={};med.repaired=false;ownOrders.count=0;ownOrders.goal=0;
 Check(SelfRepairOne(&c,fresh,tick+90000)&&ownOrders.count==1&&fresh.sent,"job worker without manual orders can receive one repair episode");
 ownOrders.present=true;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=fresh.bed;
 fresh.inputSerial=inputSerial;g_selfRepair=false;
 Check(SelfRepairOne(&c,fresh,tick+90001)&&ownOrders.lastTask==GET_OUT_OF_BED&&!fresh.sent,"disabling module retires only our pending order");
 g_selfRepair=true;ownOrders.failAdd=true;t_ownOrder=false;
 Check(CatchOwn(&ownOrders,hand(&bed))&&!t_ownOrder,"native order fault always restores observer TLS");
 t_ownOrder=true;
 Check(CatchOwn(&ownOrders,hand(&bed))&&t_ownOrder,"nested native order fault preserves prior TLS scope");
 t_ownOrder=false;
 ownOrders.failAdd=false;ownOrders.present=true;ownOrders.queued=false;ownOrders.count=0;
 c.inSomething=IN_BED;c.inWhat=hand(&bed);ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState stalled={1,1,1,true,c.inWhat,0,0,0};stalled.inputSerial=inputSerial;stalled.progressAt=1;stalled.progress=100;
 Check(SelfRepairOne(&c,stalled,300002)&&stalled.failed&&ownOrders.lastTask==GET_OUT_OF_BED,"no healing progress yields only our bed after five minutes");
 c.inSomething=IN_NOTHING;
 Check(SelfRepairOne(&c,stalled,300005)&&stalled.interrupted&&!stalled.sent,"failed rest episode cannot immediately reacquire bed");
 c.inSomething=IN_BED;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState overtime={1,1,1,true,c.inWhat,0,0,0};overtime.inputSerial=inputSerial;overtime.progressAt=600000;overtime.progress=-100;
 ownOrders.count=0;
 Check(SelfRepairOne(&c,overtime,600002)&&!overtime.failed&&ownOrders.count==0,"progressing repair does not expire after ten minutes");
 ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState steady={1,1,1,true,c.inWhat,0,0,0};steady.inputSerial=inputSerial;steady.progressAt=1;steady.progress=-60;
 for(unsigned minute=1;minute<=20;++minute){med.repairWear=60.0f-minute;SelfRepairOne(&c,steady,1+minute*60000);}
 Check(ownOrders.count==0&&steady.sent&&!steady.failed,"twenty-minute progressing recovery retains owned bed");med.repairWear=60;
 ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState recovering={1,1,1,true,c.inWhat,0,0,0};recovering.inputSerial=inputSerial;recovering.progressAt=1;recovering.progress=-40;
 med.repairWear=80;SelfRepairOne(&c,recovering,299001);
 med.repairWear=79;SelfRepairOne(&c,recovering,310001);
 Check(ownOrders.count==0&&!recovering.failed,"healing after new damage refreshes watchdog without exceeding the old health peak");med.repairWear=60;
 c.inSomething=IN_NOTHING;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState changed={tick,tick,1,true,c.inWhat,0,0,0};changed.inputSerial=inputSerial;bed.valid=false;
 Check(SelfRepairOne(&c,changed,tick+1)&&!changed.sent&&ownOrders.lastTask==GET_OUT_OF_BED,"pending rest revalidates shared bed eligibility before retaining its order");bed.valid=true;
 c.robot=false;med.hungerSpeed=.75f;ownOrders.present=false;ownOrders.count=0;
 SelfRepairState hungry={};
 Check(SelfRepairOne(&c,hungry,tick+1)&&ownOrders.count==0,"hungry biological patient does not receive a new rest order");
 SelfRepairState feeding={tick,tick,1,true,c.inWhat,0,0,0};feeding.inputSerial=inputSerial;
 c.inSomething=IN_BED;ownOrders.present=true;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 Check(SelfRepairOne(&c,feeding,tick+1)&&feeding.foodPaused&&ownOrders.lastTask==GET_OUT_OF_BED,"owned rest yields to native hunger penalty before starvation");
 c.inSomething=IN_NOTHING;
 Check(SelfRepairOne(&c,feeding,tick+2)&&!feeding.sent&&!feeding.interrupted,"food pause releases only our completed get-up order");
 ownOrders.present=false;ownOrders.count=0;
 Check(SelfRepairOne(&c,feeding,tick+30000)&&ownOrders.count==0,"food pause does not repeat bed commands while hungry");
 med.hungerSpeed=1;ownOrders.goal=0;
 Check(SelfRepairOne(&c,feeding,tick+30001)&&feeding.sent&&ownOrders.lastTask==USE_BED_ORDER,"rest can resume after feeding with unchanged intent and remaining attempt budget");
 c.robot=true;
 // Actual GUI/scan integration must ignore wall time spent paused.
 ou=&world;world.player=&player;player.playerCharacters.clear();player.playerCharacters.push_back(&c);
 g_lifecycleReady=true;g_loading=0;g_seenGeneration=g_loadGeneration;g_selfRepair=true;exerciseRepairPolicy=true;
 c.inSomething=IN_BED;c.inWhat=hand(&bed);c.orders=&ownOrders;c.medical=&med;med.repaired=false;med.hungerSpeed=1;
 ownOrders.count=0;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;ownOrders.present=true;
 SelfRepairState pausedState={1,1,1,true,c.inWhat,0,0,0};pausedState.inputSerial=inputSerial;pausedState.progress=-60;pausedState.progressAt=1;
 g_selfRepairState.clear();g_selfRepairState[hand(&c)]=pausedState;g_lastSelfRepairScan=0;
 world.simulationSeconds=10;world.paused=false;tick=10000;World_hook(&world,0);GuiUpdate_hook(&gui);
 world.paused=true;tick+=3600000;World_hook(&world,0);GuiUpdate_hook(&gui);
 Check(ownOrders.count==0&&g_selfRepairState[hand(&c)].sent,"one-hour paused GUI performs no rest timeout command");
 world.paused=false;tick+=3000;World_hook(&world,0);GuiUpdate_hook(&gui);
 Check(ownOrders.count==0&&g_selfRepairState[hand(&c)].sent,"first frame after long pause retains stalled deadline budget");
 world.simulationSeconds+=301;tick+=3000;World_hook(&world,0);GuiUpdate_hook(&gui);
 Check(ownOrders.count==1&&g_selfRepairState[hand(&c)].failed,"five minutes of actual simulation without recovery still retires stalled rest");
 c.inSomething=IN_NOTHING;ownOrders.count=0;ownOrders.first.task=USE_BED_ORDER;ownOrders.first.subject=c.inWhat;
 SelfRepairState approachTimeout={1,1,1,true,c.inWhat,0,0,0};approachTimeout.inputSerial=inputSerial;
 Check(SelfRepairOne(&c,approachTimeout,60002)&&!approachTimeout.sent&&ownOrders.lastTask==GET_OUT_OF_BED,"failed approach still retires after sixty simulation seconds");
 Check(nativeLessCalls==0 && nativeEqualCalls==0,"AIFixed identity does not depend on native hand comparison semantics");
 printf("AIFixed lifetime/debug regressions: %s (%d checks, %d failure(s))\n",failures ? "FAILED":"PASSED",checks,failures);
 return failures ? 1:0;
}
