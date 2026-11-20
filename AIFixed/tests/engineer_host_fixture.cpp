#include <Windows.h>
#include <cstdio>
#include "EngineerMaterialsPolicy.h"
#include "EngineerPriorityPolicy.h"
enum BuildingFunction { BF_ANY,BF_MINE,BF_RESOURCE_STORAGE,BF_RESEARCH,BF_REFINERY,BF_GENERATOR,BF_BED,BF_TRAINING,BF_CAGE,BF_SHOP,BF_CRAFTING,BF_CORPSE_DISPOSAL,BF_TURRET,BF_GENERAL_STORAGE,BF_ITEM_FURNACE,BF_LIGHT,BF_TABLE,BF_CHAIR,BF_FLUFF,BF_SHELL_WITH_INTERIOR,BF_WALL,BF_GATE,BF_DOOR,BF_BATTERY,BF_THRONE,BF_SKELETON_BED,BF_RAIN_COLLECTOR,BF_MINE_NATURAL,BF_STEERING,BF_ENGINE,BF_LIQUID_TANK };
enum itemType { BUILDING=0 };
enum TaskType { NO_TASK,JOB_BUILDER };
enum BuildingClassType { BCTYPE_USABLE,BCTYPE_PRODUCTION,BCTYPE_CRAFTING,BCTYPE_FARM,BCTYPE_RESEARCH,BCTYPE_ITEM_FURNACE };
const int IN_NOTHING=0;
struct Building;
struct hand {
 unsigned type,container,containerSerial,index,serial;
 hand(unsigned i=1,unsigned s=2,itemType t=BUILDING,unsigned c=3,unsigned cs=4):type(t),container(c),containerSerial(cs),index(i),serial(s){}
 Building* getBuilding()const;
};
struct TaskMatch {TaskType type;hand subtarget;TaskMatch():type(NO_TASK){}TaskType key()const{return type;} };
struct OrdersReceiver {bool jobs,orders;TaskMatch goal;OrdersReceiver():jobs(true),orders(false){}bool isJobsEnabled(){return jobs;}bool hasPlayerOrders(){return orders;}const TaskMatch& getCurrentGoal(){return goal;} };
struct Character {
 bool player,dead,down,unconscious,carried,combat,isCarryingSomething,isChained,isUsingTurret,throwPlayer;int inSomething;
 OrdersReceiver receiver;
 Character():player(true),dead(false),down(false),unconscious(false),carried(false),combat(false),isCarryingSomething(false),isChained(false),isUsingTurret(false),throwPlayer(false),inSomething(0){}
 bool isPlayerCharacter(){if(throwPlayer)throw 17;return player;}bool isDead(){return dead;}bool isDown(){return down;}bool isUnconcious(){return unconscious;}bool isBeingCarried(){return carried;}bool isInCombatMode(bool,bool){return combat;}
 OrdersReceiver* getOrdersReciever(){return &receiver;}
};
struct AI {Character* me;};
struct Faction {bool isPlayer;Faction():isPlayer(true){} };
struct LevelEditor {bool levelEditModeOn;LevelEditor():levelEditModeOn(false){} };
struct Player {LevelEditor* levelEditor;Player():levelEditor(0){} };
struct World {Player* player;}world;
World* ou=&world;
struct Building {
 struct ConstructionState {
  struct Materials {unsigned size()const{return 3;} } mats;
  bool isComplete,isPaused,isDismantled,missing;int buildersThisFrame;
  ConstructionState():isComplete(false),isPaused(false),isDismantled(false),missing(true),buildersThisFrame(0){}
  bool needMats()const{return missing;}
 } state;
 hand handle;Faction faction;BuildingFunction function;BuildingClassType classType;bool disabled,wall;
 Building():function(BF_MINE),classType(BCTYPE_USABLE),disabled(false),wall(false){}
 bool isDisabled(){return disabled;}Faction* getFaction(){return &faction;}bool isAWall(){return wall;}
 BuildingFunction getSpecialFunction(){return function;}ConstructionState* getBuildState(){return &state;}
 BuildingClassType getBuildingClass()const{return classType;}
}*resolved=0;
Building* hand::getBuilding()const{return resolved;}
volatile LONG g_engineers=1,g_engineerDebug=0,g_engineerReady=1,g_loading=0,g_loadGeneration=7;
volatile LONG g_engineerGlobalEnabled=1;
bool g_enabled=true,g_lifecycleReady=true,g_newHooksReady=true,activeWorld=true;
bool WorldActive(DWORD){return activeWorld;}
DWORD hostTick=1000;
DWORD HostTick(){return hostTick;}
#define GetTickCount HostTick
namespace PriorityNative {void Maintenance(unsigned){}}
namespace EngineerMaterialsRuntime
{
    AIFixedEngineer::Identity completed;
    unsigned completionGeneration=0;
    bool knownNewCompletion=false;
    void Maintenance(){}
    bool ProjectNeedsCovered(Character*,Building*,unsigned){return false;}
    void CompleteProject(const AIFixedEngineer::Identity& key,unsigned e,bool newCompletion)
    {completed=key;completionGeneration=e;knownNewCompletion=newCompletion;}
}
unsigned pendingMaintenanceCalls=0;bool pendingWorldSafe=false;
namespace EngineerSurplusRuntime {void Maintenance(bool safe){++pendingMaintenanceCalls;pendingWorldSafe=safe;}}
// @PRODUCTION@
int checks=0,failed=0;
void Check(bool value,const char* label){++checks;if(!value){++failed;printf("FAIL %s\n",label);}}
int main(){
 Player player;world.player=&player;unsigned generation=0;Character actor;AI ai={&actor};
 Check(EngineerCurrent(&generation)&&generation==7,"ready callback publishes current stable generation");
 Check(EngineerAllowed(&actor)&&EngineerAllowActor(&ai),"ordinary player worker remains eligible");
 g_loading=1;Check(!EngineerCurrent(&generation)&&!EngineerAllowed(&actor),"load suspension prevents all native callbacks");g_loading=0;
 g_engineerReady=0;Check(!EngineerCurrent(&generation),"partial hook installation remains inactive");g_engineerReady=1;
 g_engineers=0;Check(!EngineerCurrent(&generation),"MCM disabling immediately stops admission");g_engineers=1;
 g_engineerGlobalEnabled=0;Check(!EngineerCurrent(&generation),"master enable is an atomic worker snapshot");g_engineerGlobalEnabled=1;
 activeWorld=false;Check(!EngineerCurrent(&generation),"old heartbeat suspends work without clearing native goals");activeWorld=true;
 g_newHooksReady=false;Check(!EngineerCurrent(&generation),"missing input hooks prevents coordination");g_newHooksReady=true;
 actor.player=false;Check(!EngineerAllowed(&actor),"NPC and mod AI packages excluded");actor.player=true;
 actor.receiver.orders=true;Check(!EngineerAllowed(&actor),"direct orders win");actor.receiver.orders=false;
 actor.receiver.jobs=false;Check(!EngineerAllowed(&actor),"jobs disabled wins");actor.receiver.jobs=true;
 actor.combat=true;Check(!EngineerAllowed(&actor),"combat wins");actor.combat=false;
 actor.dead=true;Check(!EngineerAllowed(&actor),"dead actor cannot reserve or unload");actor.dead=false;
 actor.carried=true;Check(!EngineerAllowed(&actor),"carried actor excluded");actor.carried=false;
 actor.isChained=true;Check(!EngineerAllowed(&actor),"chained actor excluded");actor.isChained=false;
 actor.inSomething=1;Check(!EngineerAllowed(&actor),"bed/use state retained");actor.inSomething=0;
 actor.throwPlayer=true;bool propagated=false;
 try{EngineerAllowActor(&ai);}catch(int code){propagated=code==17;}
 Check(propagated,"foreign native C++ exception is not swallowed by read-only admission wrappers");actor.throwPlayer=false;
 Building b;resolved=&b;AIFixedEngineerPriority::Candidate candidate={};
 Check(EngineerSnapshot(&b,&candidate)&&candidate.productive&&!candidate.wall,"production uses native function category");
 Check(!EngineerAssignedProject(&ai,&candidate.key),"a non-engineering goal never establishes construction affinity");
 actor.receiver.goal.type=JOB_BUILDER;actor.receiver.goal.subtarget=b.handle;
 Check(EngineerAssignedProject(&ai,&candidate.key)&&candidate.key.index==b.handle.index,
       "the current native engineer subtarget identifies the assignment");
 b.state.buildersThisFrame=1;
 Check(EngineerSnapshot(&b,&candidate)&&!candidate.assembling,"active worker on site still leaves room for useful deliveries");
 b.state.missing=false;
 Check(EngineerSnapshot(&b,&candidate)&&candidate.assembling,"only stocked machine with an active builder is marked assembling");
 b.state.buildersThisFrame=0;b.state.missing=true;
 Check(EngineerResolve(candidate.key)==&b,"fresh resolution validates complete serial identity");
 EngineerCompletedProject(candidate.key,generation);
 const AIFixedEngineer::Identity& completed=EngineerMaterialsRuntime::completed;
 Check(completed.type==candidate.key.type&&completed.index==candidate.key.index&&
       completed.serial==candidate.key.serial&&completed.container==candidate.key.container&&
       completed.containerSerial==candidate.key.containerSerial&&
       EngineerMaterialsRuntime::completionGeneration==generation&&EngineerMaterialsRuntime::knownNewCompletion,
       "completion forwards full identity and world generation");
 AIFixedEngineerPriority::Key wrong=candidate.key;++wrong.containerSerial;
 Check(!EngineerResolve(wrong),"reused container cannot resolve old target");
 b.function=BF_WALL;b.wall=true;Check(EngineerSnapshot(&b,&candidate)&&candidate.wall&&!candidate.productive,"walls stay observable but native parallelism retained");
 b.wall=false;b.function=BF_GENERAL_STORAGE;Check(EngineerSnapshot(&b,&candidate)&&!candidate.productive,"ordinary storage does not become a production machine");
 b.function=BF_ANY;b.classType=BCTYPE_FARM;Check(EngineerSnapshot(&b,&candidate)&&candidate.productive,"native farm class covers modded functional categories without name lists");
 b.classType=BCTYPE_PRODUCTION;Check(EngineerSnapshot(&b,&candidate)&&candidate.productive,"modded production subclass is recognized by final native class");
 b.function=BF_CRAFTING;b.state.isPaused=true;Check(EngineerSnapshot(&b,&candidate)&&candidate.paused,"pause copied from shared virtual construction state");
 b.faction.isPlayer=false;Check(!EngineerSnapshot(&b,&candidate),"foreign faction building excluded");b.faction.isPlayer=true;
 LevelEditor editor;player.levelEditor=&editor;editor.levelEditModeOn=true;
 Check(!EngineerSnapshot(&b,&candidate),"editor placements do not acquire production chronology");
 g_engineers=0;g_engineerGlobalEnabled=0;EngineerMaintenance();
 Check(pendingMaintenanceCalls==1&&pendingWorldSafe,"pending inventory rollback remains serviceable after feature and master OFF");
 activeWorld=false;hostTick+=250;EngineerMaintenance();
 Check(pendingMaintenanceCalls==2&&!pendingWorldSafe,"pending recovery receives no native world permission after heartbeat loss");
 g_engineerReady=0;hostTick+=250;EngineerMaintenance();
 Check(pendingMaintenanceCalls==2,"no maintenance for an uninstalled engineer subsystem");
 printf("Engineer host admission: %s (%d checks)\n",failed?"FAIL":"PASS",checks);return failed?1:0;
}
