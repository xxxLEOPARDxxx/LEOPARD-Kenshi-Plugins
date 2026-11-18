#include <cstdio>
#include <EngineerSurplusPolicy.h>
using namespace AIFixedEngineerSurplus;
static unsigned checks=0,failures=0;
static void Check(bool b,const char* s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
struct Mock
{
    typedef int Piece; bool admitted,same,dest,back,detachWorks;int source,stored,pending,attempts,receipts;
    Mock():admitted(true),same(true),dest(true),back(true),detachWorks(true),source(3),stored(0),pending(0),attempts(0),receipts(0){}
    bool CanStart(){return admitted&&pending==0;}
    Piece Detach(){++attempts;if(!detachWorks)return 0;--source;return 1;}
    void Keep(Piece p){pending=p;}
    bool SameEpoch(){return same;}
    bool PutDestination(Piece){if(!dest)return false;++stored;return true;}
    bool PutBack(Piece){if(!back)return false;++source;return true;}
    void Commit(){pending=0;++receipts;}
    void Restored(){pending=0;}
};
int main()
{
    Admission a;a.enabled=a.normalJobCall=a.exactBuilder=a.owned=a.eligible=a.epochStable=true;
    Check(Admit(0,a)==1,"owned builder admitted without changing native rank");
    Check(Admit(8,a)==8,"positive native construction score preserved");
    a.normalJobCall=false;Check(Admit(0,a)==0,"foreign score caller native");a.normalJobCall=true;
    a.owned=false;Check(Admit(-2,a)==-2,"no owned credit native");a.owned=true;
    a.epochStable=false;Check(Admit(0,a)==0,"epoch mismatch native");
    Check(Surplus(5,3)==2&&Surplus(5,8)==0&&Surplus(5,0)==5,"retain remaining project need before returning owned credit");
    Check(Surplus(5,-1)==0&&Surplus(-1,0)==0,"ambiguous need or credit refuses cleanup");
    Mock m;Check(TransferOne(m)==Delivered,"one unit delivered");Check(m.source==2&&m.stored==1&&m.pending==0&&m.receipts==1,"success conservation and receipt");
    m=Mock();m.dest=false;Check(TransferOne(m)==RolledBack,"failed destination rolls back");Check(m.source==3&&m.stored==0&&m.pending==0&&m.receipts==0,"rollback never consumes credit");
    m=Mock();m.dest=false;m.back=false;Check(TransferOne(m)==KeptPending,"failed rollback retains detached unit");Check(m.source+m.stored+m.pending==3,"failed rollback no loss");
    m=Mock();m.same=false;Check(TransferOne(m)==KeptPending,"load invalidation keeps piece without native writes");Check(m.source==2&&m.stored==0&&m.pending==1,"old world piece retained");
    m=Mock();m.admitted=false;Check(TransferOne(m)==NoTransfer&&m.attempts==0,"unowned items never detached");
    m=Mock();m.detachWorks=false;Check(TransferOne(m)==NoTransfer&&m.receipts==0,"failed split no false receipt");
    std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
