#define BOOST_ALL_NO_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY
#include "../../atlas-terrain/world-native-rebind/NativeSession.h"
#include "../../atlas-terrain/fine-coordinator/FineCoordinator.h"
#include "_generated/actual_budget.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool yes,const char*what){++checks;if(!yes){std::printf("FAIL %s\n",what);throw 1;}}
static FineCoordinator::Residency Resident(unsigned count){FineCoordinator::Residency r;r.viewerGpu=34143518;r.occupied=count;r.fineGpu=FineJob::WindowBytes*count;return r;}
int main(){try{
 const unsigned counts[]={0,6,FineJob::CacheSlots};
 for(unsigned i=0;i<3;++i){FineCoordinator::Residency r=Resident(counts[i]);C(TestStartup(r),"actual town viewer plus retained fine cache starts");r.viewerGpu=FineJob::ViewerAllowance;C(TestStartup(r),"inclusive40MiB viewer starts with retained fine");++r.viewerGpu;C(!TestStartup(r),"viewer allowance plus1 refuses");}
 FineCoordinator::Residency r=Resident(6);r.pendingGpu=FineJob::ViewerAllowance-r.viewerGpu;C(TestStartup(r),"pending bytes attributed to viewer inclusive cap");++r.pendingGpu;C(!TestStartup(r),"pending viewer debt plus1 refuses");
 r=Resident(6);r.retiredGpu=FineJob::ViewerAllowance-r.viewerGpu;C(TestStartup(r),"retired bytes attributed to viewer inclusive cap");++r.retiredGpu;C(!TestStartup(r),"retired viewer debt plus1 refuses");
 r=Resident(FineJob::CacheSlots);r.viewerGpu=FineJob::ViewerAllowance;
 const FineJob::U64 peak=r.viewerGpu+r.fineGpu+WorldCohort::GpuReservation(2);
 C(peak==515058740ULL&&peak<=AtlasView::GpuCap,"actual full256 cache plus40MiB viewer and K2 remain within512MiB");
 r=Resident(1);r.fineGpu=0;C(!TestStartup(r),"occupied slot cannot claim zero bytes");r=Resident(FineJob::CacheSlots+1);C(!TestStartup(r),"physical slot cap enforced");r=Resident(0);r.fineGpu=FineJob::CacheSlots*FineJob::WindowBytes+1;C(!TestStartup(r),"fine bytes physical bound enforced");
 r=Resident(0);r.pendingGpu=~FineJob::U64(0);C(!TestStartup(r),"pending overflow refused");r=Resident(0);r.retiredGpu=~FineJob::U64(0);C(!TestStartup(r),"retired overflow refused");r=Resident(0);r.fineGpu=~FineJob::U64(0);C(!TestStartup(r),"fine overflow refused");
 ActualBudget actual;FineStore::Ledger cpu;cpu.other=7880725;FineJob::Budget budget;
 C(actual.MakeBudget(Resident(FineJob::CacheSlots),cpu,budget),"actual native static reservation and controller/store overhead admit observed7,880,725B external CPU");
 C(budget.producerCpu==ActualNativeBytes()&&budget.otherCpu>=cpu.other,"native reservation and measured external bytes preserved");
 std::printf("actual producerCpu=%I64u otherCpu=%I64u totalCpu=%I64u CPUcap=%I64u K2=%I64u GPUpeak=%I64u\n",budget.producerCpu,budget.otherCpu,budget.producerCpu+budget.otherCpu,AtlasView::CpuCap,WorldCohort::GpuReservation(2),peak);
 C(FineJob::Internal::ProducerFits(budget),"actual MakeBudget full physical cache still permits producer");
 cpu.other=AtlasView::CpuCap;C(!actual.MakeBudget(Resident(0),cpu,budget),"CPU overcommit still refused");
 std::printf("PASS %u actual startup/admission controls\n",checks);return 0;
 }catch(...){return 1;}}
