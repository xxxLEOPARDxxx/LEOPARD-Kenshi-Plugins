#include <cstdio>
#include <EngineerPriorityPolicy.h>
using namespace AIFixedEngineerPriority;
static unsigned checks=0,failures=0;
static void Check(bool b,const char* s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
static Key K(unsigned i,unsigned serial=9,unsigned cs=7){Key k={0,3,cs,i,serial};return k;}
static Candidate C(unsigned i,bool productive=true,bool wall=false,bool complete=false){Candidate c={};c.key=K(i);c.productive=productive;c.wall=wall;c.complete=complete;return c;}
int main(){
 Registry r;r.Reset(1);Round q;Key out;
 Check(r.Record(K(100))&&r.Record(K(2)),"new placements accepted in observed order");
 r.Begin(q,1);r.Observe(q,C(2));r.Observe(q,C(100));
 Check(r.Select(q,1,C(2),out)&&Equal(out,K(100)),"earlier placement wins despite reverse discovery and numeric handles");
 r.Begin(q,1);r.Observe(q,C(2));r.Observe(q,C(100));
 Check(!r.Select(q,1,C(8,true,true),out),"native wall selection stays native");
 Check(!r.Select(q,1,C(2,true,false,true),out),"native repair of complete object stays native");
 Check(!r.Select(q,1,C(2,false),out),"unrelated native object stays native");
 r.Begin(q,1);r.Observe(q,C(100));r.Observe(q,C(6));
 Check(!r.Select(q,1,C(2),out),"unknown old blueprint prevents invented chronology");
 r.Begin(q,1);r.Observe(q,C(2));r.Observe(q,C(100,true,true));
 Check(!r.Select(q,1,C(2),out),"wall candidate cannot become productive replacement");
 r.Begin(q,1);r.Observe(q,C(100));Candidate paused=C(2);paused.paused=true;r.Observe(q,paused);
 Check(!r.Select(q,1,paused,out),"paused native target cannot be replaced");
 Candidate dismantled=C(2);dismantled.dismantled=true;
 Check(!r.Select(q,1,dismantled,out),"dismantled native target cannot be replaced");
 r.Begin(q,1);r.Observe(q,C(100));r.Observe(q,C(2));
 Check(!r.Select(q,2,C(2),out),"world generation change rejects selection");
 r.Poison(q);Check(!r.Select(q,1,C(2),out),"nested or invalid scope poison rejects selection");
 r.Begin(q,1);r.Observe(q,C(100));r.Observe(q,C(2));
 Check(r.Record(K(100)),"duplicate placement notification retains its original ordinal");
 Check(r.Select(q,1,C(2),out)&&Equal(out,K(100)),"duplicate notification does not reorder placement");
 r.Forget(K(100));r.Begin(q,1);r.Observe(q,C(100));r.Observe(q,C(2));
 Check(!r.Select(q,1,C(2),out),"forgotten cancelled target becomes unknown");
 Check(r.Record(K(100,10)),"recycled index with new object serial is a separate placement");
 r.Begin(q,1);r.Observe(q,C(2));Candidate recycled=C(100);recycled.key=K(100,10);r.Observe(q,recycled);
 Check(!r.Select(q,1,C(2),out),"later recycled object cannot outrank earlier remaining blueprint");
 r.Begin(q,1);r.Observe(q,recycled);Candidate wrong=recycled;wrong.key.containerSerial++;
 r.Observe(q,wrong);Check(!r.Select(q,1,recycled,out),"container serial participates in full identity");
 r.Reset(2);r.Begin(q,2);r.Observe(q,C(100));r.Observe(q,C(2));
 Check(!r.Select(q,2,C(2),out),"load resets transient ordering instead of fabricating save persistence");
 r.Start(K(100));r.Start(K(100));r.Start(K(2));
 r.Begin(q,2);r.Observe(q,C(2));r.Observe(q,C(6));r.Observe(q,C(100),true);
 Check(r.Select(q,2,C(2),out)&&Equal(out,K(100))&&q.waiting,"working machine with fully booked deliveries holds against another machine");
 Check(!r.Select(q,2,C(8,true,true),out),"waiting machine cannot block wall work");
 r.Begin(q,2);r.Observe(q,C(2));r.Observe(q,C(100),true);r.Observe(q,C(100));
 Check(r.Select(q,2,C(2),out)&&!q.waiting,"a viable delivery outranks waiting for the same machine");
 r.Begin(q,2);r.Observe(q,C(2));
 Check(!r.Select(q,2,C(2),out),"unreachable, unsupplied or finished earlier focus is absent from native selection");
 r.Begin(q,2);r.Observe(q,C(100));r.Observe(q,C(6));
 Check(r.Select(q,2,C(6),out)&&Equal(out,K(100)),"selected existing blueprint holds priority without inventing placement order");
 r.Forget(K(100));Check(!r.Select(q,2,C(6),out),"retired focus cannot suppress new construction");
 r.Reset(3);for(unsigned i=1;i<=Registry::MaxEntries;++i)Check(r.Record(K(i)),"metadata capacity can hold admitted placements");
 Check(!r.Record(K(Registry::MaxEntries+1)),"full metadata refuses another placement without eviction");
 r.Forget(K(20));Check(r.Record(K(Registry::MaxEntries+1)),"deleted metadata slot can be reused");
 r.Begin(q,3);for(unsigned i=0;i<=Registry::MaxObserved;++i)r.Observe(q,C(1));
 Check(!r.Select(q,3,C(2),out),"candidate budget overflow preserves native");
 Key batch[8];unsigned cursor=0;unsigned total=0;for(unsigned pass=0;pass<128;++pass){unsigned n=r.Collect(batch,8,cursor);Check(n<=8,"bounded metadata key snapshot");total+=n;}Check(total>0,"metadata iterator eventually visits registered keys");
 std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
