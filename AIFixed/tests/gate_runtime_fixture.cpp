#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstdio>
#include <cmath>
#include <limits>
#include <GatePolicy.h>
#include <RestPolicy.h>
namespace Ogre {struct Vector3{float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);}Vector3 operator-(const Vector3& b)const{return Vector3(x-b.x,y-b.y,z-b.z);}Vector3 operator*(float k)const{return Vector3(x*k,y*k,z*k);}float squaredLength()const{return x*x+y*y+z*z;}};}
struct Building;struct Character;struct DoorStuff;
struct hand
{
 int type;unsigned container,containerSerial,index,serial;void* object;
 hand():type(0),container(0),containerSerial(0),index(0),serial(0),object(NULL){}
 hand(Building*);hand(Character*);Building* getBuilding()const;
};
struct RootObject{unsigned serial;hand handle;Ogre::Vector3 position;RootObject():serial(1){}virtual ~RootObject(){};};
struct Faction{} mine;
enum {BF_GATE=21,BUILDING=0,CHARACTER=1,MOVE_CUS_ORDERED=29,OPEN_DOOR=72};typedef int TaskType;
struct Building:RootObject
{
 DoorStuff* door;Faction* owner;bool live,gate,disabled;
 Building():door(NULL),owner(&mine),live(true),gate(true),disabled(false){handle=hand(this);}
 int getSpecialFunction(){return gate?BF_GATE:0;}Building* isGate(){return gate?this:NULL;}
 int amInsideTownWalls(){return 0;}
 bool isDisabled(){return disabled;}
 Faction* getFaction(){return owner;}DoorStuff* getDoor(){return door;}
};
struct Tasker{TaskType task;hand subject;Ogre::Vector3 position;int key(){return task;}Ogre::Vector3 getLocation(){return position;}};
struct OrdersReceiver
{
 Character* me;Tasker first,second;bool present,queued,fail;unsigned adds,clears;
 struct Queue{OrdersReceiver* owner;Tasker* getSecondTask(){return owner->queued?&owner->second:NULL;}}orders;
 OrdersReceiver():me(NULL),present(false),queued(false),fail(false),adds(0),clears(0){orders.owner=this;}
 Tasker* getFirstOrder(){return present?&first:NULL;}bool hasPlayerOrders(){return present;}
 void addOrder(TaskType,const hand&,const Ogre::Vector3&,bool,bool);void clearOrders();
};
struct DataPanelLine{};
struct TownBase{Building* gate;Building* getNearestGate(const Ogre::Vector3&){return gate;}};
struct DoorStuff:Building
{
 bool isSetupComplete,broken,locked,wantsToLock,failOpen,failClose;float amount;
 unsigned opens,closes,locks,unlocks,buttons;
 Ogre::Vector3 inside,outside;
 DoorStuff():isSetupComplete(true),broken(false),locked(true),wantsToLock(true),failOpen(false),failClose(false),amount(0),opens(0),closes(0),locks(0),unlocks(0),buttons(0),inside(-2,0,0),outside(2,0,0){door=this;}
 bool isBroken(){return broken;}bool isLocked(){return locked;}float getDoorOpenAmount(){return amount;}
 unsigned short getGateCode(){return 27;}
 const Ogre::Vector3& getDoorPosition(){return position;}const Ogre::Vector3& getDoorPosInside(){return inside;}const Ogre::Vector3& getDoorPosOutside(){return outside;}
 void unlockDoor();void lockDoor();bool closeDoor();
};
struct Character:RootObject
{
 OrdersReceiver orders;bool dead,down,unconscious,player,isPhysicalMode;unsigned doorOrders;Building* lastDest;RootObject* lastSubject;bool lastShift,lastClear;
 TownBase* town;
 Character():dead(false),down(false),unconscious(false),player(true),isPhysicalMode(true),doorOrders(0),lastDest(NULL),lastSubject(NULL),town(NULL){handle=hand(this);orders.me=this;position=Ogre::Vector3(-4,0,0);}
 bool isPlayerCharacter(){return player;}bool isDead(){return dead;}bool isDown(){return down;}bool isUnconcious(){return unconscious;}
 OrdersReceiver* getOrdersReciever(){return &orders;}Ogre::Vector3 getPosition(){return position;}Faction* getFaction(){return &mine;}
 float getRadius(){return 8.0f;}
 TownBase* getCurrentTownLocation(){return town;}
 int amInsideTownWalls(){return position.x<0?27:0;}
 void addOrder(Building*,TaskType,RootObject*,bool,bool,const Ogre::Vector3&);
};
hand::hand(Building* b):type(2),container(1),containerSerial(1),index((unsigned)(uintptr_t)b),serial(b?b->serial:0),object(b){}
hand::hand(Character* c):type(1),container(1),containerSerial(1),index((unsigned)(uintptr_t)c),serial(c?c->serial:0),object(c){}
Building* hand::getBuilding()const{Building* b=static_cast<Building*>(object);return b&&b->live&&b->serial==serial?b:NULL;}
template<class T>struct lektor:std::vector<T>{};
struct Player{lektor<Character*> playerCharacters;Faction* getFaction()const{return &mine;}};
struct TimeOfDay{double hours;double getTotalHours()const{return hours;}};
struct ZoneSpacialGrid
{
 lektor<RootObject*>* buildings;unsigned* queries;
 int getObjects(const Ogre::Vector3& center,float radius,int function,lektor<RootObject*>& out,int)
 {
  ++*queries;
  for(size_t i=0;i<buildings->size();++i)
  {
   Building* building=static_cast<Building*>((*buildings)[i]);
   if(!(static_cast<unsigned>(function)&(1u<<building->getSpecialFunction())))continue;
   const Ogre::Vector3 delta=building->position-center;
   if(delta.x*delta.x+delta.z*delta.z<radius*radius)out.push_back(building);
  }
  return static_cast<int>(out.size());
 }
};
struct ZoneManager{ZoneSpacialGrid buildingGrid;};
struct NavMesh{bool isGenerating(){return false;}int getGateCode(const Ogre::Vector3& p){return p.x<0?27:0;}};
struct GameWorld
{
 Player* player;lektor<RootObject*> buildings;unsigned queries,clockReads;bool paused,clockFault;double simulationSeconds;
 ZoneManager zoneManager;ZoneManager* zoneMgr;NavMesh* navmesh;
 GameWorld():player(NULL),queries(0),clockReads(0),paused(false),clockFault(false),simulationSeconds(0),zoneMgr(&zoneManager),navmesh(NULL)
 {zoneManager.buildingGrid.buildings=&buildings;zoneManager.buildingGrid.queries=&queries;}
 bool isPaused()const{return paused;}
 TimeOfDay getTimeStamp_inGameHours(){++clockReads;if(clockFault)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);TimeOfDay stamp={simulationSeconds/100.0};return stamp;}
 float getLengthOfHourInRealSeconds(){return 100.0f;}
 void getObjectsWithinSphere(lektor<RootObject*>& out,const Ogre::Vector3& center,float radius,int type,int,RootObject*)
 {
  ++queries;
  if(type==BUILDING){for(size_t i=0;i<buildings.size();++i)if(!static_cast<Building*>(buildings[i])->disabled&&(buildings[i]->position-center).squaredLength()<=radius*radius)out.push_back(buildings[i]);}
  else for(size_t i=0;i<player->playerCharacters.size();++i)if((player->playerCharacters[i]->position-center).squaredLength()<=radius*radius)out.push_back(player->playerCharacters[i]);
 }
};
GameWorld* ou=NULL;
CRITICAL_SECTION g_intentLock,g_noteLock;
AIFixedRest::SimulationClock g_gateClock,g_restClock;
DWORD g_lastSelfRepairScan=0;
const DWORD SELF_REPAIR_SCAN_MS=3000;
std::map<int,int> g_selfRepairState,g_reserved,g_notedSearch,g_notedBed,g_lastSnap;
bool t_ownOrder=false,g_newHooksReady=true,g_gateHooksReady=true,g_gatePassage=true,g_gateConflict=false,g_gateUnlock=true,g_gateClose=true,g_gateRelock=true,g_debug=false;
bool g_selfRepair=false,g_autoSleep=false,g_repairHealth=false;
int g_gateRadius=30,g_gateCloseDelay=3;
volatile LONG g_loading=0,g_loadGeneration=0;
DWORD tick=1000;DWORD TestTick(){return tick;}
#define GetTickCount TestTick
unsigned errors=0,moveOriginals=0;
void ErrorLog(const char*){++errors;}void DebugLog(const char*){}
struct ForgottenGUI{} gui;
unsigned guiOriginals=0,restScans=0;
bool worldReady=true;LONG seenGeneration=0;
void OrigGui(ForgottenGUI*){++guiOriginals;}
void (*g_origGuiUpdate)(ForgottenGUI*)=OrigGui;
bool SelfRepairSessionReady(DWORD);
bool ReadRestClock(GameWorld*,DWORD*){return false;}
void SelfRepairScan(DWORD){++restScans;}
// @PRODUCTION@
bool SelfRepairSessionReady(DWORD)
{
 if(seenGeneration!=g_loadGeneration){ResetSessionState();seenGeneration=g_loadGeneration;}
 return worldReady&&ou&&ou->player;
}
void NativeClear(OrdersReceiver* o){++o->clears;o->present=o->queued=false;}
void NativeAdd(OrdersReceiver* o,TaskType t,const hand& subject,const Ogre::Vector3& pos,bool clear,bool)
{
 if(o->fail)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);
 if(clear)o->clearOrders();else if(t==OPEN_DOOR&&o->present){o->second=o->first;o->queued=true;}
 ++o->adds;o->present=true;o->first.task=t;o->first.subject=subject;o->first.position=pos;
}
void OrdersReceiver::addOrder(TaskType t,const hand& h,const Ogre::Vector3& p,bool clear,bool shift){OrdersAdd_hook(this,t,h,p,clear,shift);}
void OrdersReceiver::clearOrders(){OrdersClear_hook(this);}
void Character::addOrder(Building* dest,TaskType t,RootObject* subject,bool shift,bool clear,const Ogre::Vector3& p)
{
 ++doorOrders;lastDest=dest;lastSubject=subject;lastShift=shift;lastClear=clear;
 DoorStuff* d=static_cast<DoorStuff*>(dest);if(d->failOpen)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);
 orders.addOrder(t,subject->handle,p,clear,shift);
}
void NativeLock(DoorStuff* d){++d->locks;d->wantsToLock=true;if(d->amount<.01f)d->locked=true;}
void NativeUnlock(DoorStuff* d){++d->unlocks;d->locked=false;}
void NativeLockButton(DoorStuff* d,DataPanelLine*){++d->buttons;d->wantsToLock=!d->wantsToLock;}
void NativeOpenButton(DoorStuff* d,DataPanelLine*){++d->buttons;d->locked=false;}
void DoorStuff::unlockDoor(){GateUnlock_hook(this);}void DoorStuff::lockDoor(){GateLock_hook(this);}
bool DoorStuff::closeDoor(){++closes;if(failClose)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);return true;}
void NativeMove(Character* c,Building*,RootObject*,const Ogre::Vector3& p){++moveOriginals;c->orders.addOrder(MOVE_CUS_ORDERED,hand(),p,true,false);}
int checks=0,failures=0;
void Check(bool value,const char* what){++checks;if(!value){++failures;printf("FAIL: %s\n",what);}}
void Reset(Character& c,Building& gate,DoorStuff& leaf,GameWorld& world,Player& player)
{
 g_intents.clear();g_moveIntents.clear();g_gateStates.clear();g_gateDoorChanges.clear();g_gateCursor=hand();g_intentOverflow=false;g_intentSerial=0;
 c=Character();c.orders.me=&c;c.orders.orders.owner=&c.orders;gate=Building();gate.handle=hand(&gate);leaf=DoorStuff();leaf.handle=hand(&leaf);leaf.door=&leaf;gate.door=&leaf;
 c.handle=hand(&c);player.playerCharacters.clear();player.playerCharacters.push_back(&c);world.player=&player;world.buildings.clear();world.buildings.push_back(&gate);world.queries=0;ou=&world;
 tick=1000;g_gatePassage=g_gateClose=g_gateRelock=g_gateUnlock=true;g_gateRadius=30;g_gateCloseDelay=3;t_ownOrder=false;
 world.paused=false;world.clockFault=false;world.simulationSeconds=0;world.clockReads=0;world.navmesh=NULL;worldReady=true;seenGeneration=g_loadGeneration;
 g_gateClock.Reset();g_restClock.Reset();g_lastGateScan=g_lastSelfRepairScan=0;
}
void Begin(Character& c){Move_hook(&c,NULL,NULL,Ogre::Vector3(20,0,0));GateScan(tick);tick+=1000;GateScan(tick);}
void Cross(Character& c,DoorStuff& leaf){leaf.amount=1;tick+=250;GateScan(tick);c.position=Ogre::Vector3(120,0,0);tick+=250;GateScan(tick);}
void Pump(GameWorld& world,DWORD wallMilliseconds,double simulationSeconds)
{tick+=wallMilliseconds;world.simulationSeconds+=simulationSeconds;GuiUpdate_hook(&gui);}
void BeginGui(Character& c,GameWorld& world)
{Move_hook(&c,NULL,NULL,Ogre::Vector3(20));GuiUpdate_hook(&gui);Pump(world,1000,1);}
int main()
{
 InitializeCriticalSection(&g_intentLock);InitializeCriticalSection(&g_noteLock);g_origOrdersAdd=NativeAdd;g_origOrdersClear=NativeClear;g_origMove=NativeMove;g_origGateLock=NativeLock;g_origGateUnlock=NativeUnlock;g_origGateLockButton=NativeLockButton;g_origGateOpenButton=NativeOpenButton;
 Character c;Building gate;DoorStuff leaf;GameWorld world;Player player;DataPanelLine button;
 Reset(c,gate,leaf,world,player);Begin(c);
 Check(moveOriginals==1,"move hook calls original exactly once");
 Check(c.doorOrders==1&&c.lastDest==&leaf&&c.lastSubject==&leaf&&!c.lastShift&&!c.lastClear,"native gate order uses actual child and preserves original queue");
 Check(!leaf.locked&&g_gateStates.begin()->second.stage==1,"allowed own locked gate unlocks once");
 unsigned seq=InputSequence(&c);Cross(c,leaf);
 Check(c.orders.first.task==MOVE_CUS_ORDERED&&InputSequence(&c)==seq,"owned resume preserves explicit destination and input sequence");
 tick+=3000;GateScan(tick);
 Check(leaf.closes==1&&!g_gateStates.empty()&&g_gateStates.begin()->second.stage==3,"normal close requested after actor clears crossing");
 tick+=250;GateScan(tick);Check(leaf.locks==0,"lock restoration waits for fully closed gate");
 leaf.amount=0;tick+=250;GateScan(tick);
 Check(leaf.locks==1&&g_gateStates.empty()&&g_gateDoorChanges.empty(),"original lock restored once and transient metadata retired");
 // The native task can disappear between accepting a ground order and the gate scan.
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==1,"missing native move retains a stationary, still-current gate intent");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.clearOrders();GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==0,"manual Stop cancels before gate search even when the move task is gone");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.addOrder(OPEN_DOOR,leaf.handle,Ogre::Vector3(),true,false);GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==0,"replacement order cancels before gate search");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;c.position=Ogre::Vector3(20);GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==0,"arrived character with no native task never starts gate search");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;world.buildings.clear();GateScan(tick);tick+=1000;GateScan(tick);
 world.buildings.push_back(&gate);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==2,"empty task intent expires after a completed gate search without a match");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;GateScan(tick);c.position=Ogre::Vector3(-8);
 tick+=250;GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==0,"unrelated movement after task loss cancels gate intent");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;GateScan(tick);tick+=1000;GateScan(tick);
 c.orders.present=false;leaf.amount=1;unsigned fallbackAdds=c.orders.adds;tick+=250;GateScan(tick);
 Check(c.orders.present&&c.orders.first.task==MOVE_CUS_ORDERED&&c.orders.adds==fallbackAdds+1,"completed open order with an empty queue resumes original movement once");
 c.position=Ogre::Vector3(120);tick+=250;GateScan(tick);tick+=3000;GateScan(tick);
 leaf.amount=0;tick+=250;GateScan(tick);
 Check(leaf.closes==1&&leaf.locks==1&&c.orders.adds==fallbackAdds+1,"fallback passage closes and restores lock without resending the move");
 Reset(c,gate,leaf,world,player);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;GateScan(tick);tick+=1000;GateScan(tick);
 c.orders.clearOrders();leaf.amount=1;fallbackAdds=c.orders.adds;tick+=250;GateScan(tick);
 Check(c.orders.adds==fallbackAdds&&g_gateStates.empty(),"manual Stop during fallback opening prevents resume");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-40);Move_hook(&c,NULL,NULL,Ogre::Vector3(20));
 c.orders.present=false;c.position=Ogre::Vector3(-4);GateScan(tick);tick+=1000;GateScan(tick);
 Check(c.doorOrders==0&&world.queries==0,"displacement before first empty task sample cannot revive old intent");
 Reset(c,gate,leaf,world,player);g_gateUnlock=false;Begin(c);
 Check(c.doorOrders==0&&leaf.locked,"locked gate respected when unlock option disabled");
 Reset(c,gate,leaf,world,player);g_gatePassage=false;Move_hook(&c,NULL,NULL,Ogre::Vector3(20));GateScan(tick);
 Check(g_gateStates.empty()&&g_moveIntents.empty()&&world.queries==0,"disabled module performs no local query");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);c.orders.clearOrders();tick+=4000;GateScan(tick);
 Check(g_gateStates.empty()&&leaf.closes==0&&leaf.locks==0,"manual Stop cancels pending close and relock");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);GateLockButton_hook(&leaf,&button);tick+=4000;GateScan(tick);
 Check(g_gateStates.empty()&&leaf.buttons==1&&leaf.closes==0,"direct lock GUI intent overrides passage episode");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);GateOpenButton_hook(&leaf,&button);tick+=4000;GateScan(tick);
 Check(g_gateStates.empty()&&leaf.buttons==1&&leaf.closes==0,"direct open GUI intent overrides passage episode");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);GateUnlock_hook(&leaf);tick+=4000;GateScan(tick);
 Check(leaf.closes==1,"unchanged internal unlock does not cancel the watch");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);++gate.serial;tick+=4000;GateScan(tick);
 Check(g_gateStates.empty()&&leaf.closes==0,"recycled parent identity never touches old door");
 Reset(c,gate,leaf,world,player);leaf.failOpen=true;Begin(c);
 Check(g_gateStates.empty()&&!t_ownOrder&&errors>0,"native open fault retires state and restores TLS");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);leaf.failClose=true;tick+=4000;GateScan(tick);
 Check(g_gateStates.empty()&&!t_ownOrder,"native close fault retires state and restores TLS");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);Character blocker;blocker.position=Ogre::Vector3();player.playerCharacters.push_back(&blocker);tick+=4000;GateScan(tick);
 Check(leaf.closes==0,"another character in gate corridor prevents closing");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);Character traveller;traveller.position=Ogre::Vector3(-200);
 traveller.orders.present=true;traveller.orders.first.task=MOVE_CUS_ORDERED;
 player.playerCharacters.push_back(&traveller);
 GateState second=g_gateStates.begin()->second;second.actor=hand(&traveller);second.sample=traveller.position;
 second.passedAt=0;second.wasLocked=false;second.serial=InputSequence(&traveller);g_gateStates[second.actor]=second;
 tick+=4000;GateScan(tick);
 Check(leaf.closes==0,"second tracked traveller who has yet to cross keeps gate open outside occupancy radius");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);c.position=Ogre::Vector3(-120);tick+=4000;GateScan(tick);
 Check(leaf.closes==0,"an actor who returns to their original side resets the close timer");
 Reset(c,gate,leaf,world,player);Begin(c);Cross(c,leaf);blocker.position=Ogre::Vector3(0,70,0);
 player.playerCharacters.push_back(&blocker);tick+=4000;GateScan(tick);
 Check(leaf.closes==1,"standing on a different floor does not hold the gate open");
 Reset(c,gate,leaf,world,player);Begin(c);tick+=31000;GateScan(tick);
 Check(g_gateStates.empty()&&c.orders.first.task==MOVE_CUS_ORDERED,"expired owned open action resumes original movement once");
 Check(leaf.locked&&leaf.locks==1,"expired opening restores the lock on an unchanged closed gate");
 Reset(c,gate,leaf,world,player);Begin(c);c.orders.clearOrders();tick+=250;GateScan(tick);
 Check(g_gateStates.empty()&&leaf.locked&&leaf.locks==1,"Stop while approaching restores the unchanged closed gate lock");
 Reset(c,gate,leaf,world,player);Begin(c);GateOpenButton_hook(&leaf,&button);c.orders.clearOrders();tick+=250;GateScan(tick);
 Check(g_gateStates.empty()&&!leaf.locked&&leaf.locks==0,"direct door control wins over cancellation cleanup");
 Reset(c,gate,leaf,world,player);Begin(c);Move_hook(&c,NULL,NULL,Ogre::Vector3(-200));tick+=250;GateScan(tick);
 Check(leaf.locked&&leaf.locks==1,"new movement retires the old gate attempt and restores the lock");
 Reset(c,gate,leaf,world,player);g_gateCloseDelay=30;Begin(c);Cross(c,leaf);tick+=29750;GateScan(tick);
 Check(leaf.closes==0&&!g_gateStates.empty(),"maximum delay preserves the post-crossing watch until all 30 seconds pass");
 tick+=250;GateScan(tick);Check(leaf.closes==1&&!g_gateStates.empty(),"maximum delay closes the gate after 30 seconds");
 tick+=500;leaf.amount=0;GateScan(tick);Check(leaf.locked&&leaf.locks==1,"late close gets a separate time window to restore the lock");
 Reset(c,gate,leaf,world,player);gate.gate=false;Begin(c);
 Check(c.doorOrders==0,"wrong building function cannot become gate target");
 Reset(c,gate,leaf,world,player);gate.disabled=true;Begin(c);
 Check(c.doorOrders==0,"disabled gates remain excluded by the direct building grid query");
 Reset(c,gate,leaf,world,player);Faction other;gate.owner=&other;Begin(c);
 Check(c.doorOrders==0,"other faction gate not opened");
 Reset(c,gate,leaf,world,player);for(unsigned i=0;i<128;++i)world.buildings.push_back(&gate);Begin(c);
 Check(c.doorOrders==0,"crowded returned native query declined after native work");
 Reset(c,gate,leaf,world,player);Building distant;distant.gate=false;distant.position=Ogre::Vector3(200);
 for(unsigned i=0;i<128;++i)world.buildings.push_back(&distant);Begin(c);
 Check(c.lastDest==&leaf&&world.queries==1,"near gate found before querying farther crowded buildings");
 Reset(c,gate,leaf,world,player);NavMesh nav;TownBase town={&gate};world.navmesh=&nav;c.town=&town;
 c.position=Ogre::Vector3(-60,-42,115);Begin(c);
 Check(c.lastDest==&leaf&&world.queries==0,"town gate connecting native regions handles curved or sloped routes without spatial scan");
 Reset(c,gate,leaf,world,player);world.navmesh=&nav;c.position=Ogre::Vector3(-90,-42,115);Begin(c);
 Check(c.lastDest==&leaf&&world.queries==1,"local fallback also recognizes a gate between distinct native regions");
 Reset(c,gate,leaf,world,player);world.navmesh=&nav;c.town=&town;c.position=Ogre::Vector3(60,-42,115);Begin(c);
 Check(c.doorOrders==0,"town gate stays shut for a destination in the same region");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-750);g_gateRadius=50;Begin(c);
 Check(c.doorOrders==0,"50 m search excludes a gate 75 m away");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-750);g_gateRadius=100;
 distant.position=Ogre::Vector3(-200);
 for(unsigned i=0;i<128;++i)world.buildings.push_back(&distant);Begin(c);
 Check(c.lastDest==&leaf&&world.queries<=2,"100 m search finds the same gate among 128 unrelated buildings within budget");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-4990,-400,0);g_gateRadius=500;Begin(c);
 Check(c.lastDest==&leaf,"500 m search finds a distant gate where a sloping route crosses its floor");
 for(unsigned i=0;i<5;++i){c.position.x+=900;tick+=10000;GateScan(tick);}
 Check(c.orders.first.task==OPEN_DOOR&&!g_gateStates.empty(),"progress keeps the far native approach active beyond 30 seconds");
 c.position=Ogre::Vector3(-4);leaf.amount=1;tick+=10000;GateScan(tick);
 Check(c.orders.first.task==MOVE_CUS_ORDERED&&!g_gateStates.empty(),"far gate opening resumes the original order");
 c.position=Ogre::Vector3(120);tick+=250;GateScan(tick);tick+=3000;GateScan(tick);
 leaf.amount=0;tick+=250;GateScan(tick);
 Check(leaf.closes==1&&leaf.locks==1&&g_gateStates.empty(),"far passage keeps the complete close and relock sequence");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-5010);g_gateRadius=500;Begin(c);
 Check(c.doorOrders==0,"500 m search excludes a gate beyond the configured limit");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-4990);g_gateRadius=500;
 Move_hook(&c,NULL,NULL,Ogre::Vector3(20));GateScan(tick);
 for(unsigned i=0;i<4;++i){c.position.x+=1150;tick+=10000;GateScan(tick);}
 tick+=1000;GateScan(tick);
 Check(c.doorOrders==1,"ground order that first stalls after 40 seconds can still find a gate");
 Reset(c,gate,leaf,world,player);c.position=Ogre::Vector3(-4000);g_gateRadius=500;Begin(c);
 for(unsigned i=0;i<30;++i){c.position.x+=10;tick+=10000;GateScan(tick);}
 Check(c.orders.first.task==OPEN_DOOR&&!g_gateStates.empty(),"progress preserves far approach for the full five minutes");
 tick+=250;GateScan(tick);
 Check(c.orders.first.task==MOVE_CUS_ORDERED&&g_gateStates.empty(),"far approach expires even with progress after the five-minute ceiling");
 Reset(c,gate,leaf,world,player);Building sideGate;DoorStuff sideLeaf;sideGate.door=&sideLeaf;
 gate.position=leaf.position=Ogre::Vector3(0,0,15);
 sideGate.position=sideLeaf.position=Ogre::Vector3(0,0,4);
 sideLeaf.inside.z=sideLeaf.outside.z=30;world.buildings.insert(world.buildings.begin(),&sideGate);Begin(c);
 Check(c.lastDest==&leaf,"gate aligned with the movement line wins over closer offset gate");
 Reset(c,gate,leaf,world,player);Begin(c);g_gateQueryBudget=0;
 Check(GateOccupied(&leaf)&&world.queries==1,"exhausted global query budget postpones closure");
 Reset(c,gate,leaf,world,player);std::vector<Character*> crowd;
 for(unsigned n=0;n<129;++n){Character* a=new Character;crowd.push_back(a);player.playerCharacters.push_back(a);Move_hook(a,NULL,NULL,Ogre::Vector3(20));}
 Check(g_moveIntents.size()==128,"pending movement queue has fixed actor capacity");
 GateScan(tick);Check(g_gateStates.size()==128,"active movement metadata has fixed actor capacity");
 tick+=1000;GateScan(tick);Check(world.queries==2,"global gate query budget bounds simultaneous stalled actors");
 tick+=250;GateScan(tick);Check(world.queries==4,"round robin advances to later actor batch");
 for(size_t n=0;n<crowd.size();++n)delete crowd[n];
 Reset(c,gate,leaf,world,player);
 for(unsigned n=1;n<=1024;++n){hand identity(&c);identity.serial=n;RecordInput(identity);}
 hand overflow(&c);overflow.serial=1025;RecordInput(overflow);
 Check(g_intents.size()==1024&&InputSequence(&c)==~0u,"input capacity overflow fails closed without unbounded growth");
 // GUI integration uses the real dispatcher and clock boundary, never a wall
 // timestamp manually passed as if it were active simulation time.
 Reset(c,gate,leaf,world,player);world.paused=true;
 Move_hook(&c,NULL,NULL,Ogre::Vector3(20));GuiUpdate_hook(&gui);Pump(world,3600000,3600);
 Check(c.doorOrders==0&&leaf.unlocks==0&&world.queries==0,"paused pending gate input makes no native door or query changes");
 Check(g_moveIntents.size()==1&&g_gateStates.empty(),"queued movement survives a long pause before episode creation");
 world.paused=false;Pump(world,250,0);
 Check(c.doorOrders==0&&g_gateStates.size()==1,"resume creates a fresh active-time episode without spending paused wall time");
 Pump(world,1000,1);
 Check(c.doorOrders==1&&g_gateStates.begin()->second.stage==1,"pending input opens gate after active quiet interval");
 Reset(c,gate,leaf,world,player);BeginGui(c,world);unsigned beforeAdds=c.orders.adds;
 world.paused=true;leaf.amount=1;Pump(world,3600000,3600);
 Check(g_gateStates.size()==1&&c.orders.first.task==OPEN_DOOR&&c.orders.adds==beforeAdds,"open-door episode neither expires nor resumes movement while paused");
 world.paused=false;Pump(world,250,0);
 Check(g_gateStates.size()==1&&g_gateStates.begin()->second.stage==2&&c.orders.adds==beforeAdds+1,"owned open episode resumes movement once after unpause");
 Reset(c,gate,leaf,world,player);BeginGui(c,world);leaf.amount=1;Pump(world,250,.25);c.position=Ogre::Vector3(120);Pump(world,250,.25);
 world.paused=true;Pump(world,3600000,3600);
 Check(!g_gateStates.empty()&&leaf.closes==0&&leaf.locks==0,"post-crossing watch survives paused time without closing or locking");
 world.paused=false;Pump(world,250,0);
 Check(leaf.closes==0,"close delay budget remains after resume");
 Pump(world,3000,3);Check(leaf.closes==1&&g_gateStates.begin()->second.stage==3,"close request follows actual active-time delay");
 leaf.amount=0;world.paused=true;Pump(world,60000,60);
 Check(leaf.locks==0&&!g_gateStates.empty(),"completed closure cannot trigger automatic relock during pause");
 world.paused=false;Pump(world,250,0);Check(leaf.locks==1&&g_gateStates.empty(),"original lock restored once after unpause");
 Reset(c,gate,leaf,world,player);BeginGui(c,world);leaf.amount=1;beforeAdds=c.orders.adds;unsigned beforeGui=guiOriginals;
 world.clockFault=true;Pump(world,250,.25);
 Check(c.orders.adds==beforeAdds&&!g_gateStates.empty(),"native clock fault suspends gate commands and retains episode");
 Check(guiOriginals==beforeGui+1,"clock fault does not suppress original GUI update");
 world.clockFault=false;Pump(world,250,100);
 Check(c.orders.adds==beforeAdds+1&&g_gateStates.begin()->second.stage==2,"valid clock reanchors and resumes surviving owned order");
 Reset(c,gate,leaf,world,player);world.simulationSeconds=std::numeric_limits<double>::quiet_NaN();
 Move_hook(&c,NULL,NULL,Ogre::Vector3(20));beforeGui=guiOriginals;GuiUpdate_hook(&gui);
 Check(g_moveIntents.size()==1&&g_gateStates.empty()&&world.queries==0&&guiOriginals==beforeGui+1,"invalid numeric clock preserves queued input and original updates");
 world.simulationSeconds=0;Pump(world,250,0);Pump(world,1000,1);
 Check(c.doorOrders==1,"queued input starts from valid active clock after unavailable interval");
 Reset(c,gate,leaf,world,player);BeginGui(c,world);world.paused=true;Pump(world,3600000,0);
 Check(!g_gateStates.empty()&&c.orders.first.task==OPEN_DOOR,"failed opening does not time out during pause");
 world.paused=false;Pump(world,250,0);Pump(world,31000,31);
 Check(g_gateStates.empty()&&c.orders.first.task==MOVE_CUS_ORDERED,"failed gate still retires after bounded active simulation time");
 Reset(c,gate,leaf,world,player);BeginGui(c,world);g_gateClock.Sample(10,false,beforeGui);g_gateClock.Sample(11,false,beforeGui);
 ++g_loadGeneration;worldReady=false;ou=reinterpret_cast<GameWorld*>(1);beforeGui=guiOriginals;Pump(world,250,0);
 Check(g_gateStates.empty()&&g_moveIntents.empty()&&!g_gateClock.anchored&&g_gateClock.milliseconds==1,"world generation clears gate state and active clock before stale-world access");
 Check(guiOriginals==beforeGui+1,"world switch readiness does not freeze original GUI update");
 ou=&world;worldReady=true;world.simulationSeconds=0;Pump(world,250,0);
 Check(leaf.closes==0&&leaf.locks==0,"old gate watch cannot mutate a replacement world");
 Reset(c,gate,leaf,world,player);GuiUpdate_hook(&gui);Pump(world,250,.25);
 Check(world.clockReads==0&&world.queries==0,"idle enabled gate module performs no native clock poll or spatial query");
 printf("AIFixed extracted gate/intent runtime: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
 return failures?1:0;
}
