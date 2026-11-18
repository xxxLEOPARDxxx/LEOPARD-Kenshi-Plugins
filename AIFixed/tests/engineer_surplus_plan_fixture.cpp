#include <Windows.h>
#include <cstdio>
#include <deque>
#include <EngineerMaterialsPolicy.h>
static unsigned checks=0,failures=0;static void Check(bool b,const char*s){++checks;if(!b){++failures;printf("FAIL %s\n",s);}}
typedef AIFixedEngineer::Identity Identity;static Identity Id(unsigned n){Identity i;i.type=1;i.index=n;i.serial=3;return i;}struct hand{Identity id;};static Identity Of(const hand&h){return h.id;}
static unsigned char*image=(unsigned char*)0x140000000ULL;static unsigned planningToken=7;static LONG planningEpoch=3,currentEpoch=3;static bool permit=true,job=true,found=true;static ULONG_PTR owned[6];static ULONG_PTR OwnedFillVtable(){return(ULONG_PTR)&owned[1];}enum{FILL_MACHINE=93,OPERATE_STORAGE=124,Capacity=128};
struct TaskData{};struct Tasker{ULONG_PTR vt;TaskData*taskData;int type;hand subject;Tasker():vt((ULONG_PTR)image+0x16be0f0),taskData((TaskData*)1),type(93){subject.id=Id(8);}int key(){return type;}};
struct AITaskSytem{struct ActionsListMgr{std::deque<Tasker*>list;};ActionsListMgr actionsTryList;void*character;};
struct Episode{bool used,retired,committed;unsigned token;LONG generation;Identity storage;ULONG_PTR fill;Episode():used(true),retired(false),committed(false),token(7),generation(3),storage(Id(8)),fill(0){}};static Episode episodes[Capacity];struct Guard{};
static bool Live(LONG e){return e==currentEpoch;}static bool ReadToken(unsigned,Episode&e){e=episodes[0];return found;}static bool Eligible(void*,LONG){return permit;}static bool JobStillThere(AITaskSytem*,const Episode&){return job;}
static unsigned starts=0;static bool NativeSet(AITaskSytem*,AITaskSytem::ActionsListMgr*from){++starts;for(size_t i=0;i<from->list.size();++i)if(from->list[i]->type==93||from->list[i]->type==124)Check(from->list[i]->vt==OwnedFillVtable(),"owned fill tagged before first native startAction");return true;}static bool(*originalSetTask)(AITaskSytem*,AITaskSytem::ActionsListMgr*)=NativeSet;
// @SET_TASK@
// @BUDGET_TYPES@
// @BUDGET@
static void Reset(AITaskSytem&t){starts=0;planningToken=7;planningEpoch=currentEpoch=3;permit=job=found=true;episodes[0]=Episode();for(unsigned i=1;i<Capacity;++i)episodes[i].used=false;t.actionsTryList.list.clear();}
int main(){AITaskSytem tasks;Tasker fill,second,move;move.type=29;move.vt=(ULONG_PTR)image+0x16b1110;
 Reset(tasks);tasks.actionsTryList.list.push_back(&move);tasks.actionsTryList.list.push_back(&fill);tasks.actionsTryList.list.push_back(&second);Check(SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==1,"valid entire native trial plan admitted");Check(fill.vt==OwnedFillVtable()&&second.vt==OwnedFillVtable(),"all fills tagged even when first action is movement");
 Reset(tasks);fill=Tasker();fill.vt=(ULONG_PTR)image+0x16b9990;tasks.actionsTryList.list.push_back(&fill);Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"wrong native fill class rejected before start");
 Reset(tasks);fill=Tasker();fill.subject.id=Id(99);tasks.actionsTryList.list.push_back(&fill);Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"foreign storage rejected before start");
 Reset(tasks);fill=Tasker();tasks.actionsTryList.list.push_back(&fill);currentEpoch=4;Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"load during planning rejects all actions before start");
 Reset(tasks);fill=Tasker();tasks.actionsTryList.list.push_back(&fill);found=false;Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"missing metadata refuses plan before start");
 Reset(tasks);fill=Tasker();tasks.actionsTryList.list.push_back(&fill);episodes[0].retired=true;Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"input cancellation before handoff refuses plan");
 Reset(tasks);fill=Tasker();fill.vt=123;tasks.actionsTryList.list.push_back(&fill);Check(!SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==0,"mod action class only refused within own cleanup plan");
 Reset(tasks);planningToken=0;tasks.actionsTryList.list.push_back(&move);Check(SetTaskHook(&tasks,&tasks.actionsTryList)&&starts==1,"foreign native plan forwarded once");
 unsigned material=0,storage=0,queries=0;searchGeneration=-1;for(unsigned i=0;i<1000;++i)queries+=SearchTicket(Id(i+1),3,100,material,storage)?1:0;Check(queries==4,"1000 repeated score queries share four native search grants per 250ms");
 for(unsigned i=0;i<1000;++i)Check(!SearchTicket(Id(1),3,100,material,storage),"same paused timestamp cannot query native storage again");
 Check(SearchTicket(Id(5),3,350,material,storage),"quota-denied actor was not stamped into backoff");Check(!SearchTicket(Id(1),3,350,material,storage),"actor retry waits 1500ms despite new global window");
 Check(SearchTicket(Id(1),3,1600,material,storage)&&storage==16&&material==1,"retry rotates storage window and material");
 searchGeneration=-1;Check(SearchTicket(Id(1),8,0xffffff00UL,material,storage),"wrapped clock initial grant");Check(!SearchTicket(Id(1),8,100,material,storage),"clock wrap retains actor backoff");Check(SearchTicket(Id(1),8,1400,material,storage),"clock wrap eventually permits retry");
 printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
