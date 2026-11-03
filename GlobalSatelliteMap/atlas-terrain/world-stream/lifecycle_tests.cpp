#include "WorldStream.h"
#include <cstdio>
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Provider {
 HRESULT Capture(const SourceReceipts::WorkerRequest&,SourceReceipts::MainSnapshot&){return E_ACCESSDENIED;}
 HRESULT Refresh(const SourceReceipts::MainSnapshot&,SourceReceipts::MainSnapshot&){return E_ACCESSDENIED;}
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*,unsigned,const SourceReceipts::Scope&){return E_ACCESSDENIED;}
 void EndBank(){}
};
int main(){try{
 using namespace WorldStream;
 Provider p;Controller<Provider>*c=new Controller<Provider>(p);
 Check(c->State()==Idle,"default idle");
 Check(c->Start()==S_OK,"one worker starts without synchronous IO");
 SourceReceipts::Scope scope={GetCurrentThreadId(),1,1,1};WorldBank::Ledger l={};
 for(unsigned n=0;n<10000&&c->State()==Booting;++n){c->Pump(false,NULL,scope,l);Sleep(1);}
 Check(c->State()==Idle,"worker initialized");
 Check(c->CpuReservation()<=SourceReceipts::CpuCap,"all explicit CPU owners fit64MiB");
 Job j={};CapturedSources tokens;
 Check(c->Submit(false,j,tokens,scope)==S_FALSE,"OFF refuses before request access");
 Check(c->Submit(true,j,tokens,scope)==E_INVALIDARG,"malformed request fails closed");
 WorldBank::Ledger counted={};Check(c->CountedLedger(l,counted)&&WorldBank::Fits(counted),"nominal512 reservation fits192MiB");
 l.retired=AtlasView::GpuCap;Check(!c->CountedLedger(l,counted),"external retired resources participate");
 l.retired=0;l.pending=~WorldBank::U64(0);Check(!c->CountedLedger(l,counted),"overflow refuses");
 Check(!c->Result(true,scope,j.frame)&&!c->Plane(0,true,scope,j.frame)&&!c->Plane(3,true,scope,j.frame),"no premature diagnostic publication");
 c->Cancel();for(unsigned n=0;n<10000&&c->State()!=Idle;++n){c->Pump(false,NULL,scope,l);Sleep(1);}
 Check(c->State()==Idle,"idle cancellation drains worker");
 c->Shutdown();Check(c->State()==Stopped,"explicit join shutdown");delete c;
 printf("PASS %u production controller lifecycle checks\n",checks);return 0;
 }catch(...){return 1;}}
