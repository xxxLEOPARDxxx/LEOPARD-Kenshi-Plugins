#include <Windows.h>
#include <cstdio>
#include <cmath>
#include <float.h>
#include <GatePolicy.h>
#include <UnstuckPolicy.h>

enum itemType { BUILDING=0,CHARACTER=1,EMPTY_HANDLE=11 };
enum TaskType { NULL_TASK=0,IDLE=1,PATROL_TOWN=156,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE=157,OTHER_TASK=999 };
enum { BF_FLUFF=18,BF_BED=19,MOVE_NORMAL=0,MOVE_DIRECTION=2,IN_NOTHING=0 };
namespace WaterState {enum { THIGH_DEEP_WATER=2 };}
namespace RobotLimbs {enum Limb { LEFT_LEG,RIGHT_LEG };}
enum LimbState { LIMB_OK,LIMB_STUMP,LIMB_CRUSHED };
namespace Ogre {struct Vector3 {float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}};}
struct Character;
Character* actorObject=NULL;
struct hand {
    unsigned type,container,containerSerial,index,serial;
    hand(unsigned i=0,unsigned s=0,itemType t=EMPTY_HANDLE,unsigned c=0,unsigned cs=0)
        :type(t),container(c),containerSerial(cs),index(i),serial(s){}
    Character* getCharacter() const;
};
struct Building {
    hand handle;
    int function;
    bool disabled;
    struct {bool isComplete;} _buildState;
    Building():handle(7,1688598656u,BUILDING,2724,11111),function(BF_FLUFF),disabled(false){_buildState.isComplete=true;}
    int getSpecialFunction(){return function;}bool isDisabled(){return disabled;}
};
struct StandingHandle {Building* object;Building* getBuilding(){return object;}};
struct HVector {struct {float m128_f32[4];} m_quad;};
struct HavokCharacter {
    enum CharacterState {IDLE,GOAL_REACHED,FOLLOWING_PATH,MANUAL};
    enum PathState {NONE,COMPLETE,INCOMPLETE,FAILED,WAITING,UPDATING,BROKEN};
    HVector position,goal;
    CharacterState state;PathState path;
    HavokCharacter():state(IDLE),path(NONE){}
    CharacterState getState(){return state;}PathState getPathState(){return path;}
};
struct CharMovement {
    HavokCharacter* havokCharacter;StandingHandle support;
    bool animationOverride,tracking,indoors;int movementMode;
    CharMovement():havokCharacter(NULL),animationOverride(false),tracking(false),indoors(false),movementMode(MOVE_NORMAL){support.object=NULL;}
    bool isTrackingAnimationMode(){return tracking;}bool isIndoors(){return indoors;}
    StandingHandle isStandingOnSomething(){return support;}
};
struct MedicalSystem {
    bool crippled;LimbState legs[2];MedicalSystem():crippled(false){legs[0]=legs[1]=LIMB_OK;}
    bool isCrippled(){return crippled;}LimbState getLimbState(RobotLimbs::Limb n){return legs[n];}
};
struct TaskData {TaskType type;};
struct TaskMatch {
    TaskData* taskData;hand subject;
    TaskMatch():taskData(NULL){}TaskType key()const{return taskData?taskData->type:NULL_TASK;}
};
struct Character {
    hand handle;CharMovement* movement;Ogre::Vector3 position;TaskData data;TaskMatch currentGoal;
    bool player,dead,down,unconscious,ragdoll,crippled,carried,isCarryingSomething,isUsingTurret,isChained,combat;
    int inSomething;bool inWhat;int water;bool animal;MedicalSystem medical;
    Character():handle(6,3684570368u,CHARACTER,15,98510744),movement(NULL),player(false),dead(false),down(false),unconscious(false),ragdoll(false),crippled(false),carried(false),isCarryingSomething(false),isUsingTurret(false),isChained(false),combat(false),inSomething(IN_NOTHING),inWhat(false),water(0),animal(false),isPhysicalMode(true){}
    bool isPlayerCharacter(){return player;}bool isDead(){return dead;}bool isDown(){return down;}
    bool isUnconcious(){return unconscious;}bool isRagdoll(){return ragdoll;}bool isCrippled(){return crippled;}
    bool isBeingCarried(){return carried;}bool isInCombatMode(bool,bool){return combat;}int getWaterLevel(){return water;}
    bool isAnimal(){return animal;}MedicalSystem* getMedical(){return &medical;}Ogre::Vector3 getPosition(){return position;}
    bool isPhysicalMode;void teleport(const Ogre::Vector3& p);
};
Character* hand::getCharacter()const {
    return actorObject && type==actorObject->handle.type && container==actorObject->handle.container &&
        containerSerial==actorObject->handle.containerSerial && index==actorObject->handle.index && serial==actorObject->handle.serial?actorObject:NULL;
}
struct GameWorld {bool paused;GameWorld():paused(false){}bool isPaused(){return paused;}};

// @STRUCTS@

enum {HistorySize=2048};
History history[HistorySize];
DWORD activeMs=0,lastRockProbeMs=0,lastProbeMs=0;
LONG g_loadGeneration=0,g_loading=0;
bool g_debug=false;
unsigned teleports=0,probes=0;
bool allowLanding=true,reachedDuringProbe=false;
double firstRelocationHours=0,firstRelocationAge=0,currentHours=0;
const AIFixedGate::Point capturedSlope(-.8357168157f,.5356243592f,-.1211773484f);
History& Entry(const Identity& id){
    const unsigned start=id.Hash()&(HistorySize-4);
    for(unsigned i=0;i<4;++i)if(history[start+i].actor==id)return history[start+i];
    return history[start];
}
void Character::teleport(const Ogre::Vector3& p){position=p;++teleports;if(!firstRelocationHours){firstRelocationHours=currentHours;firstRelocationAge=currentHours-Entry(Identity(handle)).watch.since;}}
bool IsWallFollower(Character*,CharMovement*,TaskType){return false;}
AIFixedGate::Point Point(const Ogre::Vector3& v){return AIFixedGate::Point(v.x,v.y,v.z);}
bool MovingDestination(TaskType task){return task==PATROL_TOWN;}
const TaskMatch* CurrentGoal(Character* c){return &c->currentGoal;}
TaskType GoalTask(const TaskMatch* g){return g && g->taskData?g->key():NULL_TASK;}
bool ReadIntent(Character*,CharMovement*,const TaskMatch*,TaskType,Identity&,AIFixedGate::Point&){return false;}

// @CAN_TRAVEL@
// @GUARD_CYCLE@

Building* BlockedSupport(Character*,CharMovement*,TaskType,const Ogre::Vector3&){return NULL;}
bool NativePositionAgrees(HavokCharacter*,const Ogre::Vector3&){return true;}
bool FindLanding(GameWorld*,Character* c,Building*,const Ogre::Vector3& here,unsigned,Ogre::Vector3& landing,unsigned& sites,bool guard){
    ++probes;sites=1;
    if(reachedDuringProbe){c->movement->havokCharacter->state=HavokCharacter::GOAL_REACHED;c->movement->havokCharacter->goal=c->movement->havokCharacter->position;}
    // Native terrain/collision/navmesh/barrier queries remain explicit fakes.
    // Their real guard branch and all geometry limits are unchanged by this fix.
    landing=Ogre::Vector3(here.x-12.0f,here.y-40.0f,here.z-2.0f);
    return guard && allowLanding && AIFixedUnstuck::GuardLeavesSlope(Point(here),Point(landing),capturedSlope);
}
void DebugLog(const char*){}

// @INSPECT@

struct Captured {
    TaskType task;int state,path;float position[3],navPosition[3],navGoal[3];unsigned count;
};
// @CAPTURED@

int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;printf("FAIL: %s\n",message);}}
void Reset(Character& c,CharMovement& m,HavokCharacter& nav,Building& b){
    for(unsigned i=0;i<HistorySize;++i)history[i]=History();
    c=Character();m=CharMovement();nav=HavokCharacter();b=Building();
    c.movement=&m;c.isPhysicalMode=true;m.havokCharacter=&nav;m.support.object=&b;actorObject=&c;
    activeMs=lastProbeMs=lastRockProbeMs=0;teleports=probes=0;firstRelocationHours=firstRelocationAge=0;allowLanding=true;reachedDuringProbe=false;
}
void Apply(Character& c,HavokCharacter& nav,const Captured& s){
    c.position=Ogre::Vector3(s.position[0],s.position[1],s.position[2]);
    c.data.type=s.task;c.currentGoal.taskData=s.task==NULL_TASK?NULL:&c.data;
    nav.state=static_cast<HavokCharacter::CharacterState>(s.state);nav.path=static_cast<HavokCharacter::PathState>(s.path);
    for(unsigned i=0;i<3;++i){nav.position.m_quad.m128_f32[i]=s.navPosition[i]*.1f;nav.goal.m_quad.m128_f32[i]=s.navGoal[i]*.1f;}
}
const Captured& Expanded(unsigned n){
    n%=316;
    for(unsigned i=0;i<sizeof(captured)/sizeof(captured[0]);++i){if(n<captured[i].count)return captured[i];n-=captured[i].count;}
    return captured[0];
}
void Replay(GameWorld& w,Character& c,HavokCharacter& nav,unsigned offset,bool failuresAllowed,bool mobile,bool testArrival=false){
    const Identity id(c.handle);bool arrived=false;
    // Synthetic game time and order are explicit: 316 recorded histogram
    // observations per cycle, twelve repeated cycles per four game hours.
    // Original capture has no per-sample timestamps/game clock/complete order.
    for(unsigned n=0;n<316*16 && !teleports;++n){
        Captured s=Expanded(n+offset);
        if(!failuresAllowed && s.task==STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE && s.path==HavokCharacter::FAILED)s.path=HavokCharacter::WAITING;
        Apply(c,nav,s);if(mobile)c.position.x+=float(n)*6.0f;
        currentHours=100.0+double(n)/(316*3.0);activeMs=1000+n*25;
        if(testArrival && arrived && currentHours>104.9)break;
        Inspect(&w,id,currentHours,0);
        if(testArrival && !arrived && currentHours>101.0){
            c.position.x+=600.0f;nav.state=HavokCharacter::GOAL_REACHED;nav.goal=nav.position;
            c.data.type=STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE;c.currentGoal.taskData=&c.data;
            Inspect(&w,id,currentHours+.00001,0);
            Check(!Entry(id).watch.active,"physical arrival at a real guard goal retires the old stationary watch");
            Check(teleports==0,"a real reached guard goal is never relocated");arrived=true;
        }
    }
}
int main(){
    GameWorld world;Character actor;CharMovement movement;HavokCharacter nav;Building platform;
    Check(sizeof(captured)/sizeof(captured[0])==9,"all nine original distinct snapshots retained");
    unsigned total=0;for(unsigned i=0;i<9;++i)total+=captured[i].count;Check(total==316,"all 316 original histogram observations represented");
    // Every distinct first-seen state can be the entry point, including reached
    // acknowledgements; real failure still has to begin the episode.
    for(unsigned phase=0,offset=0;phase<9;offset+=captured[phase++].count){
        Reset(actor,movement,nav,platform);Replay(world,actor,nav,offset,true,false);
        Check(teleports==1,"recurring genuine captured failures survive every native idle/reached/reset phase for four hours");
        if(teleports)Check(firstRelocationHours>=104.0 && firstRelocationAge>=4.0,"no captured cycle can shorten the four-hour requirement");
    }
    Reset(actor,movement,nav,platform);Replay(world,actor,nav,0,false,false);Check(teleports==0&&probes==0,"waiting and retained stale failures cannot invent recent failed attempts");
    Reset(actor,movement,nav,platform);Replay(world,actor,nav,0,true,true);Check(teleports==0&&probes==0,"a mobile guard cannot accumulate the stationary window");
    Reset(actor,movement,nav,platform);Replay(world,actor,nav,0,true,false,true);Check(teleports==0,"successful real arrival requires a new full four-hour episode");
    Reset(actor,movement,nav,platform);reachedDuringProbe=true;Replay(world,actor,nav,0,true,false);Check(teleports==0&&probes>0,"a reached goal between probe and final revalidation blocks relocation");
    Reset(actor,movement,nav,platform);allowLanding=false;Replay(world,actor,nav,0,true,false);Check(teleports==0,"a mature watch still cannot bypass physical landing evidence");
    const Identity heldId(actor.handle);
    History& held=Entry(heldId);
    const double since=held.watch.since;
    const unsigned priorProbes=probes;
    allowLanding=true;held.watch.nextProbe=currentHours;
    Apply(actor,nav,captured[7]);Inspect(&world,heldId,currentHours,0);
    Check(teleports==0&&probes==priorProbes&&held.watch.active&&held.watch.since==since,
        "a mature no-goal reached acknowledgement retains history without even probing a landing");
    held.watch.nextProbe=currentHours;
    Apply(actor,nav,captured[8]);Inspect(&world,heldId,currentHours,0);
    Check(teleports==0&&probes==priorProbes&&held.watch.active&&held.watch.since==since,
        "a mature guard goal reset to the feet retains history without probing or relocating");
    activeMs+=1000;Apply(actor,nav,captured[6]);Inspect(&world,heldId,currentHours,0);
    Check(teleports==1&&firstRelocationAge>=4.0,"a subsequent genuine failed active guard request can use the retained mature history");
    for(unsigned kind=0;kind<5;++kind){
        Reset(actor,movement,nav,platform);
        if(kind==0)actor.down=true;if(kind==1)actor.crippled=true;if(kind==2)actor.medical.legs[0]=LIMB_STUMP;if(kind==3)actor.combat=true;if(kind==4)actor.player=true;
        Replay(world,actor,nav,0,true,false);Check(teleports==0&&probes==0,"native medical/combat/player exclusion remains mandatory");
    }
    Reset(actor,movement,nav,platform);
    const Identity id(actor.handle);
    for(unsigned n=0;n<400;++n){
        Apply(actor,nav,captured[8]);currentHours=200+n*.02;activeMs=1000+n*100;Inspect(&world,id,currentHours,0);
    }
    Check(teleports==0&&probes==0&&!Entry(id).watch.active,"a legitimate untracked guard post at its goal never begins recovery");
    Reset(actor,movement,nav,platform);Apply(actor,nav,captured[6]);currentHours=300;activeMs=1000;Inspect(&world,id,currentHours,0);
    actor.data.type=OTHER_TASK;actor.currentGoal.taskData=&actor.data;Inspect(&world,id,currentHours+.001,0);
    Check(!Entry(id).watch.active,"an unrelated real goal still retires the blocked guard episode");
    Reset(actor,movement,nav,platform);Apply(actor,nav,captured[6]);currentHours=400;activeMs=1000;Inspect(&world,id,currentHours,0);
    platform.handle.serial++;currentHours+=.1;activeMs+=1000;Inspect(&world,id,currentHours,0);
    Check(Entry(id).watch.since==currentHours,"a different supporting platform starts its own complete observation window");
    printf("AIFixed extracted guard trace: %s (%d checks; 316 captured observations, synthetic repeated order/time)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
