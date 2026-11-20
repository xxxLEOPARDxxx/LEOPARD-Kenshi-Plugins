#include <Windows.h>
#include <cstdio>
#include <EngineerSurplusPolicy.h>
static int fail=0,checks=0;static void Check(bool x,const char*s){++checks;if(!x){++fail;printf("FAIL %s\n",s);}}
static unsigned char* image=(unsigned char*)0x140000000ULL;static ULONG_PTR caller;static unsigned depth=0;static bool ready=true,permit=true,prepared=true,ledger=true,storageValid=true,tryEmpty=true,cargoValid=true;static LONG world=3;static unsigned planningToken=0;static LONG planningEpoch=3;
static int calls=0,deletes=0,retired=0,slot=2;static float first=0,second=1;
static void* From(){return(void*)caller;}
#define _ReturnAddress From
struct DepthScope{DepthScope(){++depth;}~DepthScope(){--depth;}};
struct Identity{int n;Identity(int v=0):n(v){}bool operator==(const Identity&b)const{return n==b.n;}};
struct hand{int n;hand(int v=0):n(v){}struct Building*getBuilding()const;};static Identity Of(const hand&h){return Identity(h.n);}
namespace Ogre{struct Vector3{};}
enum{JOB_BUILDER=125,FILL_MACHINE=93};struct TaskRepertoire{};struct GameData{};struct Inventory{};
struct Tasker{ULONG_PTR vt;int type;hand subject;int priority;float weight;int startTime,endTime;Tasker(int t=125):vt((ULONG_PTR)image+0x16be0f0),type(t),subject(6),priority(3),weight(1),startTime(0),endTime(100){}int key(){return type;}};
struct StorageBuilding;struct UseableStuff{StorageBuilding*getFunctionStuff();};struct Building{UseableStuff*getUseableStuff();};struct StorageBuilding:Building{Ogre::Vector3 getPosition(){return Ogre::Vector3();}};static StorageBuilding store;static UseableStuff use;static GameData data;
Building*hand::getBuilding()const{return&store;}UseableStuff*Building::getUseableStuff(){return&use;}StorageBuilding*UseableStuff::getFunctionStuff(){return&store;}
struct AI{TaskRepertoire*taskRepertoire;};struct Character{AI*ai;AI*getAI(){return ai;}Inventory*getInventory(){return NULL;}};struct Body{Tasker*action;Tasker*getCurrentAction(){return action;}};struct List{Tasker*last;bool isEmpty(){return tryEmpty;}Tasker*getLastTask(){return last;}};struct AITaskSytem{AI*aiImplementation;Character*character;List actionsTryList;Body*body;};
static AITaskSytem* normalRunScope;
static AITaskSytem* planningSystem;
static bool* planningAccepted;
static bool completedProject=true, unfinishedAssignment=false, nativeHandoff=true, cancelOnStart=false;
static int primaryCalls=0, returnCalls=0;
static int NormalSlot(AITaskSytem*,Tasker*){return slot;}
static bool UnfinishedAssignment(AITaskSytem*){return unfinishedAssignment;}
struct Owned{int material;};struct Episode{bool used,committed,transaction,handedOff;unsigned token;int jobSlot;Identity actor,storage;ULONG_PTR fill;Owned credit;Episode():used(true),committed(false),transaction(false),handedOff(false),token(7),jobSlot(2),actor(4),storage(8),fill(0){}};static Episode episodes[1];enum{Capacity=1};struct Guard{};
static LONG Epoch(){return world;}static bool Live(LONG e){return e==world;}static bool Eligible(Character*,LONG){return permit;}static int BuilderSlot(AITaskSytem*,Tasker*t){return t&&t->key()==125?slot:-1;}static bool Prepare(AITaskSytem*,Tasker*,int,Episode&e){e=episodes[0];return prepared;}
static void Retire(const Identity&){++retired;}static void Planning(unsigned,bool){}static void DeleteSeed(Tasker*p){if(p){++deletes;delete p;}}static Inventory*Backpack(Character*){return NULL;}static GameData*DataIn(Inventory*,int){return&data;}static int Total(Character*,GameData*){return 5;}static bool ValidStorage(Character*,StorageBuilding*,GameData*,LONG){return storageValid;}
namespace EngineerMaterialsRuntime{static hand Handle(const Identity&i){return hand(i.n);}static bool CheckOwned(const Owned&,int,int&u){u=2;return ledger;}}
static bool CargoReady(Character*,GameData*,LONG){return cargoValid;}
static bool Returnable(Character*,const Owned&,GameData*,LONG,int&u){u=2;return ledger&&completedProject;}
static Tasker fill(93);static float Native(AITaskSytem*t,Tasker*k,bool,bool){++calls;if(k->key()!=93){if(k->key()!=125)++primaryCalls;return first;}++returnCalls;if(second<=0||!nativeHandoff)return second;episodes[0].fill=(ULONG_PTR)&fill;episodes[0].handedOff=true;*planningAccepted=true;if(cancelOnStart){episodes[0].used=false;++world;}Check(k->subject.n==8,"native seed storage identity");t->body->action=&fill;t->actionsTryList.last=&fill;return second;}static float(*originalGoap)(AITaskSytem*,Tasker*,bool,bool)=Native;
static Tasker*Make(TaskRepertoire*,int type,const hand&h,int p,float w,Ogre::Vector3,int s,int e){Tasker*t=new Tasker(type);t->subject=h;t->priority=p;t->weight=w;t->startTime=s;t->endTime=e;return t;}static Tasker*(*factory)(TaskRepertoire*,int,const hand&,int,float,Ogre::Vector3,int,int)=Make;
// @PRODUCTION@
static void Reset(AITaskSytem&t){permit=prepared=ledger=storageValid=tryEmpty=ready=cargoValid=completedProject=nativeHandoff=true;unfinishedAssignment=cancelOnStart=false;normalRunScope=NULL;calls=deletes=retired=primaryCalls=returnCalls=0;first=0;second=1;depth=0;world=3;slot=2;episodes[0]=Episode();t.actionsTryList.last=NULL;t.body->action=NULL;fill.subject=hand(8);caller=(ULONG_PTR)image+0x50fd28;}
int main(){AI a;Character c;c.ai=&a;Body b;AITaskSytem t;t.character=&c;t.body=&b;t.aiImplementation=&a;a.taskRepertoire=(TaskRepertoire*)1;Tasker job;
Reset(t);Check(GoapHook(&t,&job,true,false)==1&&calls==2,"only failed native builder gets fallback");Check(deletes==1&&episodes[0].fill==(ULONG_PTR)&fill&&!episodes[0].committed&&depth==0,"native seed freed; generated fill awaits elected commit");
Reset(t);first=4;Check(GoapHook(&t,&job,true,false)==4&&calls==1&&deletes==0,"successful builder never replaced");
Reset(t);Check(GoapHook(&t,&job,true,true)==0&&calls==1,"direct player order stays native");
Reset(t);caller=3;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"foreign caller stays native");
Reset(t);prepared=false;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"no owned credit no fallback");
Reset(t);ledger=false;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"changed inventory refuses fallback");
Reset(t);cargoValid=false;Check(GoapHook(&t,&job,true,false)==0&&calls==1&&deletes==0,"changed unsupported cargo cannot create or start fallback native plan");
Reset(t);tryEmpty=false;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"existing trial plan retained");
Reset(t);slot=-1;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"nonpermanent builder native");
Reset(t);depth=1;Check(GoapHook(&t,&job,true,false)==0&&calls==1,"nested goap native once");
Tasker primary(124);
Reset(t);normalRunScope=&t;first=4;
Check(GoapHook(&t,&primary,true,false)==1&&returnCalls==1&&primaryCalls==0,"completed construction unloads before switching to a higher normal permanent job");
Reset(t);normalRunScope=&t;first=4;completedProject=false;
Check(GoapHook(&t,&primary,true,false)==4&&primaryCalls==1&&returnCalls==0,"an unfinished project cannot delay the main job");
Reset(t);normalRunScope=&t;first=4;unfinishedAssignment=true;
Check(GoapHook(&t,&primary,true,false)==4&&primaryCalls==1&&returnCalls==0,"an older completed receipt cannot interrupt a newer unfinished assignment");
Reset(t);first=4;
Check(GoapHook(&t,&primary,true,false)==4&&primaryCalls==1&&returnCalls==0,"normal-permanent election scope is required");
Reset(t);normalRunScope=&t;first=4;second=0;
Check(GoapHook(&t,&primary,true,false)==4&&primaryCalls==1&&returnCalls==1,"failed optional cleanup falls through to the primary job");
Reset(t);normalRunScope=&t;first=4;nativeHandoff=false;
Check(GoapHook(&t,&primary,true,false)==4&&primaryCalls==1&&returnCalls==1,"positive planner result cannot replace real work without a tagged handoff");
Reset(t);normalRunScope=&t;first=4;cancelOnStart=true;
Check(GoapHook(&t,&primary,true,false)==1&&primaryCalls==0&&t.body->action==&fill,
      "cancelled metadata after handoff cannot overwrite a task the engine already owns");
Reset(t);normalRunScope=&t;first=4;
Check(GoapHook(&t,&primary,true,true)==4&&primaryCalls==1&&returnCalls==0,"manual order retains priority after completion");
printf("%d checks, %d failures\n",checks,fail);return fail?1:0;}
