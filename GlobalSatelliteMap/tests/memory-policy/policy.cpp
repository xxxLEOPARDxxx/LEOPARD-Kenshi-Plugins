#define BOOST_ALL_NO_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY
#include "../../atlas-terrain/fine-coordinator/FineCoordinator.h"
#include "../../GsmFineGpu.h"
#include "../../GsmFinePolicy.h"
#include "../../atlas-water/AtlasWater.h"
#include "../../atlas-lights/terrain-hdr/AtlasLocalLights.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void C(bool v,const char*s){++checks;if(!v){std::printf("FAIL %s\n",s);throw 1;}}
static void ActualFence(){
 AtlasView::Com<ID3D11Device>d;AtlasView::Com<ID3D11DeviceContext>c;
 C(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p)),"real fence WARP device");
 GsmFineGpu::Retirements*debt=new GsmFineGpu::Retirements;bool all=true;
 for(unsigned i=0;i<GsmCacheProfile::RetirementCount();++i)all=debt->Charge(1)&&all;
 C(all&&!debt->Charge(1)&&debt->Bytes()==GsmCacheProfile::RetirementCount(),"active metadata bound holds scaled retirement records");
 C(SUCCEEDED(debt->Tick(d.p))&&debt->Bytes()!=0,"first submission does not credit unfenced debt");
 DWORD start=GetTickCount();while(debt->Bytes()&&DWORD(GetTickCount()-start)<5000){C(SUCCEEDED(debt->Tick(d.p)),"owned fence poll succeeds");Sleep(1);}
 C(!debt->Bytes()&&debt->Charge(7),"known complete fence frees metadata for reuse");
 while(debt->Bytes()&&DWORD(GetTickCount()-start)<5000){C(SUCCEEDED(debt->Tick(d.p)),"reused metadata fence succeeds");Sleep(1);}
 C(!debt->Bytes(),"second actual fence drains reused slot");delete debt;
}
#ifdef MEMORY_LEGACY
namespace GsmCacheProfile {
inline const unsigned long long&GpuBudgetBytes(){return GpuBytes;}
inline const unsigned long long&RamBudgetBytes(){static const unsigned long long n=0;return n;}
inline bool Configure(unsigned,unsigned){return false;}
inline unsigned FineSlots(){return FineCacheSlots;}
}
#endif
int main(int argc,char**argv){try{
 const unsigned long long GiB=1024ULL*1024*1024;
 C(GsmCacheProfile::GpuBudgetBytes()==GiB&&GsmCacheProfile::RamBudgetBytes()==2*GiB,"default independent VRAM1/RAM2 GiB");
 unsigned gpu=argc>1?std::atoi(argv[1]):1,ram=argc>2?std::atoi(argv[2]):9-gpu;
 C(!GsmCacheProfile::Configure(0,ram)&&!GsmCacheProfile::Configure(9,ram)&&!GsmCacheProfile::Configure(gpu,0)&&!GsmCacheProfile::Configure(gpu,33)&&!GsmCacheProfile::Configure(~0u,ram)&&!GsmCacheProfile::Configure(gpu,~0u),"invalid independent min max overflow rejected before freezing");
 C(GsmCacheProfile::Configure(gpu,ram),"one valid boot snapshot accepted");
 C(GsmCacheProfile::GpuBudgetBytes()==gpu*GiB&&GsmCacheProfile::RamBudgetBytes()==ram*GiB,"RAM and GPU are independent64bit snapshots");
 C(AtlasView::GpuCap==gpu*GiB&&AtlasWater::Cap==gpu*GiB&&AtlasLocalLights::Cap==gpu*GiB&&FineWindows::GpuCap==gpu*GiB,"all GPU aliases observe configured budget, not static initialization value");
 C(AtlasView::CpuCap==64ULL*1024*1024&&SourceReceipts::CpuCap==AtlasView::CpuCap,"producer scratch remains separate64MiB");
 C(GsmCacheProfile::Configure(gpu,ram)&&!GsmCacheProfile::Configure(gpu==8?7:gpu+1,ram)&&!GsmCacheProfile::Configure(gpu,ram==32?31:ram+1),"both session budgets immutable after successful configuration");
 C(FineJob::MaxVisible==512&&FineJob::Slots==512&&GsmCacheProfile::OneMetreVisible==96,"wider visible3m demand keeps original bounded1m threshold");
 const unsigned active=GsmCacheProfile::FineSlots();
 C(active>=96&&active<=6144&&active>256,"real active cache grows beyond old256 slots");
#ifndef MEMORY_LEGACY
 C(FineJob::ActiveSlots()==active&&FineJob::CacheSlots==6144,"maximum physical metadata distinct from active residency");
#endif
 C(gpu!=1||active==665,"oneGiB retains665 fine slots with192MiB reserve");C(gpu!=8||active==6144,"eightGiB supports6144 actual slots");
 FineCoordinator::Residency r;r.viewerGpu=FineJob::ViewerAllowance;r.occupied=active;r.fineGpu=FineJob::WindowBytes*active;
 C(FineCoordinator::StartupFits(r),"full configured fine cache and actual K2 fit total GPU cap");
 C(r.viewerGpu+r.fineGpu+WorldCohort::GpuReservation(2)<=AtlasView::GpuCap,"producer headroom preserved at every configured capacity");
 ++r.occupied;r.fineGpu+=FineJob::WindowBytes;C(!FineCoordinator::StartupFits(r),"active slots plus1 refused even below maximum physical metadata");
 r=FineCoordinator::Residency();r.viewerGpu=FineJob::ViewerAllowance;r.retiredGpu=1;C(!FineCoordinator::StartupFits(r),"unfenced debt never credited");
 std::vector<FineWindows::Key>held(active);for(unsigned i=0;i<active;++i){held[i].spacing=3;held[i].x=i%128;held[i].z=i/128;held[i].stamp.source=1;held[i].stamp.material=2;}
 FineJob::Demand demand={};demand.scope.thread=GetCurrentThreadId();demand.scope.world=1;demand.scope.sourceEpoch=1;demand.scope.job=1;demand.revision=1;demand.view.world.x=demand.view.world.z=-147456;demand.view.world.w=demand.view.world.h=768;demand.view.screenWidth=demand.view.screenHeight=256;demand.view.stamp=held[0].stamp;
 FineJob::Budget b={};b.viewerGpu=FineJob::ViewerAllowance;b.fineGpu=FineJob::WindowBytes*active;b.occupied=active;b.producerCpu=FineJob::Internal::ProducerCpuFloor();
 C(FineJob::MakePlan(demand,b,FineJob::VisibleSet,&held[0],active).state==FineJob::Detail,"complete configured residency validates without quadratic duplicate scan");
 held[active-1]=held[0];C(FineJob::MakePlan(demand,b,FineJob::VisibleSet,&held[0],active).state==FineJob::Invalid,"last resident duplicate still refused");
 C(GsmFineGpu::Fits(AtlasView::GpuCap-137571380ULL,0,137571380ULL,0),"actual K2 exact aggregate cap accepted");
 C(!GsmFineGpu::Fits(AtlasView::GpuCap-137571380ULL,1,137571380ULL,0),"retirement byte above aggregate refuses");
 C(!GsmFineGpu::Fits(0,~FineJob::U64(0),0,0)&&!GsmFineGpu::Fits(0,0,~FineJob::U64(0),0),"64bit overflow cannot bypass cap");
 GsmFineGpu::Retirements*debt=new GsmFineGpu::Retirements;
 C(debt->Charge(AtlasView::GpuCap)&&debt->Bytes()==AtlasView::GpuCap&&!debt->Charge(1),"retirement ledger accounts multiGiB without allocating payload");delete debt;
 ActualFence();
 std::printf("PASS %u memory policy controls GPU=%uGiB RAM=%uGiB active=%u peak=%I64u\n",checks,gpu,ram,active,40ULL*1024*1024+FineJob::WindowBytes*active+WorldCohort::GpuReservation(2));return 0;
 }catch(...){return 1;}}
