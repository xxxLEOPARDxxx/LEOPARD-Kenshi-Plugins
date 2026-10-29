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

struct Character;
struct Building;
struct MedicalSystem;
struct OrdersReceiver;
struct AI;
struct TownBase;
struct Position {};
enum { IN_NOTHING, IN_BED, USE_BED_ORDER=258, USE_BED=259, GET_OUT_OF_BED=260 };
int nativeLessCalls=0,nativeEqualCalls=0;
struct hand {
    int type; unsigned container,containerSerial,index,serial;
    hand() : type(0),container(0),containerSerial(0),index(0),serial(0) {}
    hand(Character* c);
    hand(Building* b);
    bool operator<(const hand& h) const {
        ++nativeLessCalls;return index < h.index; // Deliberately does not promise serial ordering.
    }
    bool operator==(const hand& h) const { ++nativeEqualCalls;return index == h.index; }
    bool operator!=(const hand& h) const { return !(*this == h); }
};
struct Character {
 unsigned serial,containerSerial;int inSomething;bool isCarryingSomething;
 MedicalSystem* medical;OrdersReceiver* orders;AI* ai;hand handle;
 Character(unsigned s=1):serial(s),containerSerial(1),inSomething(IN_NOTHING),isCarryingSomething(false),medical(NULL),orders(NULL),ai(NULL){handle=hand(this);}
 MedicalSystem* getMedical(){return medical;}OrdersReceiver* getOrdersReciever(){return orders;}AI* getAI(){return ai;}
 bool isDead(){return false;}bool isUnconcious(){return false;}bool isDown(){return false;}
 bool isBeingCarried(){return false;}bool isInCombatMode(bool,bool){return false;}
 Position getPosition(){return Position();}TownBase* getCurrentTownLocation(){return NULL;}
};
struct Building {
 unsigned serial,containerSerial;std::string displayName;hand handle;
 Building(unsigned s=1):serial(s),containerSerial(1),displayName("bed"){handle=hand(this);}
 Position getPosition(){return Position();}
};
hand::hand(Character* c):type(1),container(7),containerSerial(c ? c->containerSerial:0),index((unsigned)(uintptr_t)c),serial(c ? c->serial : 0){}
hand::hand(Building* b):type(2),container(7),containerSerial(b ? b->containerSerial:0),index((unsigned)(uintptr_t)b),serial(b ? b->serial : 0){}
template<class T> struct lektor : std::vector<T> {};
lektor<Building*> liveBeds;
lektor<Building*>& Machines(){liveBeds.clear();return liveBeds;}
struct AI {
 Building* candidate;
 void getAllLocalMachines(lektor<Building*>& out,unsigned){if(candidate)out.push_back(candidate);}
};
enum { BF_SKELETON_BED=25 };
struct Player { lektor<Character*> playerCharacters; };
struct GameWorld { Player* player; };
struct ForgottenGUI {};
struct UseableStuff:Building { bool isFreeSlot(const hand&){return true;} };
struct AITaskSytem {};
struct MedicalSystem { bool repaired;MedicalSystem():repaired(false){} };
struct TaskGoal { int task;int key()const{return task;} };
struct OrdersReceiver {
 int goal,count,lastTask;bool cleared,shifted;
 OrdersReceiver():goal(USE_BED_ORDER),count(0),lastTask(0),cleared(false),shifted(false){}
 TaskGoal getCurrentGoal(){TaskGoal g={goal};return g;}
 void addOrder(int task,const hand&,const Position&,bool clear,bool shift){++count;lastTask=goal=task;cleared=clear;shifted=shift;}
};
struct TownBase {std::string displayName;};
struct SaveFileSystem {};

GameWorld* ou=NULL;
bool g_debug=false,g_selfRepair=true,g_lifecycleReady=true,g_sessionActive=false;
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
bool IsRobot(Character*){return true;}
bool HasRoboticParts(MedicalSystem*,bool){return true;}
bool RoboticPartsRepaired(MedicalSystem* med,bool){return med->repaired;}
float WorstWear(MedicalSystem*,bool){return 60;}
void NoteSelfRepair(Character*,const std::string&){}
Building* FindRepairBed(AI* ai,Character*,const ReservedSet*){return ai->candidate;}
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
 MCM_Describe(&api);Check(toggles==4 && (debugFlags & MCM_RESTART)!=0,"debug capture advertises required restart through actual API");
 calls=0;GuiUpdate_hook(&gui);Check(calls==0,"no world heartbeat prevents squad scan despite stale ou");

 SelfRepairState prior={0,0,2,true,&bed,0,0,0};g_selfRepairState[&c]=prior;
 c.serial=2;seenTries=-1;SelfRepairScan();Check(seenTries==0,"same address with new character serial must not inherit attempts");
 g_selfRepairState.clear();g_selfRepairState[&c]=prior;c.containerSerial=2;
 seenTries=-1;SelfRepairScan();Check(seenTries==0,"new character container serial must not inherit attempts");
 AI ai;ai.candidate=&bed;hand oldBed(&bed);bed.serial=2;
 Check(LiveRepairBed(&ai,oldBed)==NULL,"recycled building address cannot validate an old reservation");
 Check(LiveRepairBed(&ai,hand(&bed))==&bed,"current building identity remains eligible");
 oldBed=hand(&bed);++bed.containerSerial;
 Check(LiveRepairBed(&ai,oldBed)==NULL,"new building container serial cannot validate old reservation");

#ifdef HAS_LIFECYCLE
 World_hook(&world,0);calls=0;GuiUpdate_hook(&gui);Check(calls==1,"fresh world heartbeat permits scan");
 OrdersReceiver orders;c.medical=&med;c.orders=&orders;c.ai=&ai;c.inSomething=IN_BED;med.repaired=true;exerciseRepairPolicy=true;
 prior.bed=hand(&bed);g_selfRepairState[&c]=prior;g_reserved[&bed]=&c;g_notedSearch[&c]=&bed;g_lastSnap[&c]=1;
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
 checkInvalidated=true;LoadGame_hook(NULL,"same-save");calls=0;GuiUpdate_hook(&gui);
 Check(calls==0 && g_selfRepairState.empty() && g_reserved.empty(),"same-address load clears state and waits for new world");
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
 Check(nativeLessCalls==0 && nativeEqualCalls==0,"AIFixed identity does not depend on native hand comparison semantics");
 printf("AIFixed lifetime/debug regressions: %s (%d checks, %d failure(s))\n",failures ? "FAILED":"PASSED",checks,failures);
 return failures ? 1:0;
}
