#include <Windows.h>
#include <cstdio>
#include <EngineerMaterialsPolicy.h>
static unsigned checks=0,failures=0;
static void Check(bool b,const char* s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
typedef AIFixedEngineer::Identity Identity;typedef AIFixedEngineer::Owned Owned;
struct Tasker{};struct hand{Identity id;};static Identity Of(const hand& h){return h.id;}
static SRWLOCK metadata=SRWLOCK_INIT;static long currentEpoch=3;static long Epoch(){return currentEpoch;}enum{Capacity=128};
static unsigned deleting=0;static void* NativeDelete(Tasker* p,unsigned){++deleting;return p;}static void*(*originalFillDelete)(Tasker*,unsigned)=NativeDelete;
// @PRODUCTION@
static Identity Actor(unsigned n){Identity a;a.type=1;a.index=n;a.serial=4;a.container=2;a.containerSerial=9;return a;}
static void Reset(){for(unsigned i=0;i<Capacity;++i)episodes[i]=Episode();currentEpoch=3;deleting=0;}
static void Make(unsigned n,Tasker* p){Episode& e=episodes[n];e.used=e.committed=true;e.generation=3;e.token=n+1;e.actor=Actor(n+7);e.fill=reinterpret_cast<ULONG_PTR>(p);}
int main(){Tasker fill,foreign;Episode out;
 Reset();Make(0,&fill);Check(ReadFill(&fill,out)&&out.token==1,"exact generated task address registered");Check(!ReadFill(&foreign,out),"foreign task not owned");
 Invalidate();Check(ReadFill(&fill,out)&&out.retired,"disable/load keeps deny tombstone");Check(!BeginTransaction(1),"retired task cannot detach");
 FillDeleteHook(&fill,1);Check(deleting==1&&!ReadFill(&fill,out),"native destructor releases tombstone before pointer reuse");
 Reset();Make(0,&fill);Check(BeginTransaction(1),"bounded receipt reserved");Check(!BeginTransaction(1),"duplicate transaction refused");Pending(1,Actor(9),Actor(10));FillDeleteHook(&fill,1);Check(ReadToken(1,out)&&out.pending&&out.transaction,"native task deletion retains in-flight receipt");EndTransaction(1);Check(ReadToken(1,out)&&out.pending&&!out.transaction,"rollback failure retains serial-only receipt");ClearPending(1);EndTransaction(1);Check(!ReadToken(1,out),"completed rollback releases deleted episode");
 Reset();Make(0,&fill);Pending(1,Actor(9),Actor(10));currentEpoch=4;Invalidate();Check(ReadFill(&fill,out)&&out.retired&&!out.pending,"new world never resolves old item receipt");FillDeleteHook(&fill,1);Check(!ReadToken(1,out),"old world metadata retired after native task teardown");
 Reset();Make(0,&fill);BeginTransaction(1);currentEpoch=4;Invalidate();Pending(1,Identity(),Actor(10));EndTransaction(1);Check(ReadFill(&fill,out)&&out.retired&&!out.pending,"post-reset split receipt cannot resurrect old-world pending metadata");
 Reset();for(unsigned i=0;i<Capacity;++i){Make(i,reinterpret_cast<Tasker*>(static_cast<ULONG_PTR>(i+1)));episodes[i].transaction=true;}Check(!BeginTransaction(900)&&!BeginTransaction(1),"saturated/held slots refuse new receipt");
 Reset();Make(0,&fill);hand h;h.id=Actor(7);Cancel(h);Check(ReadFill(&fill,out)&&out.retired,"user cancellation does not remove protective tombstone");
 std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
