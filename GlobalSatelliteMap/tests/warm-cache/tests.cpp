#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include "../../GsmFinePolicy.h"
#include "../../atlas-terrain/fine-viewer/FineJob.h"
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
int main(){try{
 C(AtlasView::GpuCap==512ULL*1024*1024&&AtlasView::CpuCap==64ULL*1024*1024,"fixed development GPU512MiB with CPU64 unchanged");
 C(FineJob::MaxVisible==96&&FineJob::Slots==96&&FineJob::CacheSlots==256&&FineStore::Slots==512,"render96 separate from256 GPU and512disk neighborhood");
 FineJob::Demand d={};d.scope.thread=GetCurrentThreadId();d.scope.world=7;d.scope.sourceEpoch=0x100000007ULL;d.scope.job=0x200000007ULL;d.revision=0x300000009ULL;d.view.stamp.source=d.scope.sourceEpoch;d.view.stamp.material=0x400000007ULL;d.view.world.x=d.view.world.z=-14336;d.view.world.w=2048;d.view.world.h=1024;d.view.screenWidth=2048;d.view.screenHeight=1024;
 FineJob::Budget b={};b.producerCpu=FineJob::Internal::ProducerCpuFloor();b.viewerGpu=28ULL*1024*1024;FineJob::Plan first=FineJob::MakePlan(d,b,FineJob::VisibleSet);C(first.state==FineJob::Detail&&first.count<=96,"unchanged bounded visible source set");
 FineWindows::Key held[256];bool valid[256]={};FineWindows::U64 touch[256]={};unsigned n=0;for(;n<first.count;++n){held[n]=first.keys[n];valid[n]=true;touch[n]=100;}
 // Fill a genuinely distinct offscreen neighborhood, not invented occupied bytes.
 for(unsigned z=0;n<256;++z)for(unsigned x=0;x<16&&n<256;++x){FineWindows::Key k={1,x,z,d.view.stamp};held[n]=k;valid[n]=true;touch[n]=n+1;++n;}
 b.occupied=256;b.fineGpu=256*FineJob::WindowBytes;FineJob::Plan warm=FineJob::MakePlan(d,b,FineJob::VisibleSet,held,256);C(warm.state==FineJob::Detail&&warm.spacing==1&&warm.count==first.count,"full physical cache still reuses wanted visible keys without coarsening");C(warm.publicationPeak==b.viewerGpu+b.fineGpu,"warm current view has no invented future-window allocation");C(FineJob::CanRender(warm,d,b),"full256fine neighborhood coexists with actual K2 under512MiB");
 FineJob::Demand wide=d;wide.view.world.w=8448;wide.view.world.h=4608;wide.view.screenWidth=1500;wide.view.screenHeight=900;FineJob::Budget empty={};empty.producerCpu=b.producerCpu;FineJob::Plan bound=FineJob::MakePlan(wide,empty,FineJob::VisibleSet);C(bound.state==FineJob::Detail&&bound.spacing==3&&bound.count<=96,"large physical cache never renders oversized1m visible set");
 int victim=GsmFinePolicy::Lru(held,valid,touch,256,d.view);C(victim>=int(first.count)&&victim<256,"pressure selects offscreen tile before visible tile");touch[victim]=10000;int next=GsmFinePolicy::Lru(held,valid,touch,256,d.view);C(next>=int(first.count)&&next!=victim,"recently used offscreen tile survives older neighbor");for(unsigned i=0;i<256;++i)valid[i]=i<first.count;C(GsmFinePolicy::Lru(held,valid,touch,256,d.view,NULL,0,true)<0,"outside-only pressure never sacrifices visible data");
 FineJob::Demand coarse=d;coarse.view.screenWidth=256;coarse.view.screenHeight=128;FineJob::Plan low=FineJob::MakePlan(coarse,empty,FineJob::VisibleSet);C(low.state==FineJob::Detail&&low.spacing==3,"mixed-spacing fixture has genuine current3m keys");FineWindows::Key mixed[256];for(unsigned i=0;i<low.count;++i)mixed[i]=low.keys[i];for(unsigned i=low.count;i<256;++i){FineWindows::Key k={1,i%16,i/16,d.view.stamp};mixed[i]=k;}C(FineJob::MakePlan(d,b,FineJob::VisibleSet,mixed,256).state==FineJob::Coarse,"full mixed cache triggers pressure instead of permanently downgrading feasible1m to cached3m");
 FineJob::Demand wrong=d;wrong.revision^=1ULL<<32;C(!FineJob::CanRender(warm,wrong,b),"high64bit stale demand refused");FineJob::UploadGate gate={0,true};C(!FineJob::CanUpload(warm,d,first.keys[0],b,gate),"physical257th slot refused");b.occupied=255;b.fineGpu=255*FineJob::WindowBytes;C(FineJob::CanUpload(warm,d,first.keys[0],b,gate),"physical256th slot admitted within aggregate");b.viewerGpu=FineJob::ViewerAllowance+1;C(!FineJob::CanRender(warm,d,b),"layered viewer production limit retained");
 // Actual failing scene: loaded city stages raised the viewer from below32MiB
 // to34,143,518B. Include two pre-producer base windows and maximum cached-town
 // reservation instead of testing only an unrealistically empty viewer.
 FineJob::Budget city={};city.producerCpu=FineJob::Internal::ProducerCpuFloor();
 city.viewerGpu=34143518ULL+2*FineJob::WindowBytes+3*512*1024;
 for(unsigned zoom=6;zoom<=8;zoom+=2){FineJob::Demand high=d;high.view.screenWidth=2160;high.view.screenHeight=1044;
  const double scale=73728.0/(4096*zoom);high.view.world.w=2160*scale;high.view.world.h=1044*scale;
  FineJob::Plan cityPlan=FineJob::MakePlan(high,city,FineJob::VisibleSet);
  C(cityPlan.state==FineJob::Detail&&cityPlan.spacing==3&&cityPlan.count<=96,"actual x6/x8 city layers admit whole-view detail");
  C(FineJob::CanRender(cityPlan,high,city),"layered x6/x8 retains real producer headroom");
 }
 printf("PASS %u warm neighborhood controls\n",checks);return 0;}catch(...){return 1;}}
