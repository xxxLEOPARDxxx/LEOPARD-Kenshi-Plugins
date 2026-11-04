#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include "../../GsmFinePrefetch.h"
#include "../../GsmFinePolicy.h"
static unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
int main(){try{
 C(GsmCacheProfile::Configure(8,8),"maximum requested session budget");
 FineJob::Demand d={};d.scope.thread=GetCurrentThreadId();d.scope.world=7;d.scope.sourceEpoch=17;d.scope.job=19;d.revision=23;
 d.view.stamp.source=17;d.view.stamp.material=29;d.view.screenWidth=1024;d.view.screenHeight=512;d.view.world.x=d.view.world.z=-12000;d.view.world.w=1024;d.view.world.h=512;
 FineJob::Budget b={};b.producerCpu=FineJob::Internal::ProducerCpuFloor();
 FineJob::Plan visible=FineJob::MakePlan(d,b,FineJob::VisibleSet);C(visible.state==FineJob::Detail,"real visible planner");
 FineJob::Plan third=GsmFinePrefetch::Make(visible,d,b,1,0,2);C(third.state==FineJob::Detail,"RAM prewarming extends beyond old two-pair ceiling");
 FineWindows::Key keys[128];unsigned count=0;
 for(unsigned i=0;i<64;++i){FineJob::Plan p=GsmFinePrefetch::Make(visible,d,b,0,0,i);C(p.state==FineJob::Detail&&p.count==2&&p.pairCount==1,"stationary viewport prewarms bounded surrounding pair");
  for(unsigned n=0;n<2;++n){C(!GsmFinePolicy::Intersects(p.keys[n],d.view),"prefetch never delays current visible data");for(unsigned j=0;j<count;++j)C(!FineWindows::Equal(keys[j],p.keys[n]),"surrounding pairs are distinct");keys[count++]=p.keys[n];}}
 C(GsmFinePrefetch::Make(visible,d,b,1,0,64).state!=FineJob::Detail,"prewarm ceiling enforced");
 C(!GsmFinePrefetch::Eligible(599,0,true,false,0,true,visible,d,b,1,0),"settle delay respected");
 C(!GsmFinePrefetch::Eligible(1000,0,false,false,0,true,visible,d,b,1,0),"visible demand strictly precedes warmup");
 C(!GsmFinePrefetch::Eligible(1000,0,true,true,0,true,visible,d,b,1,0),"center stage strictly precedes warmup");
 FineJob::Budget full=b;full.occupied=FineJob::ActiveSlots();full.fineGpu=FineJob::WindowBytes*full.occupied;FineJob::UploadGate readGate={0,true};
 C(FineJob::CanRead(visible,d,visible.keys[0],full,readGate),"full VRAM cache still permits RAM-only reads");
 C(!FineJob::CanUpload(visible,d,visible.keys[0],full,readGate),"RAM read does not authorize another GPU slot");
 readGate.producerReservedGpu=1;C(!FineJob::CanRead(visible,d,visible.keys[0],full,readGate),"RAM reads still wait for source GPU drain");
 readGate.producerReservedGpu=0;readGate.consumersJoined=false;C(!FineJob::CanRead(visible,d,visible.keys[0],full,readGate),"borrowed source consumer must join before RAM read");
 b.viewerGpu=AtlasView::GpuCap;C(GsmFinePrefetch::Make(visible,d,b,1,0,0).state!=FineJob::Detail,"real GPU headroom still required for producer");
 printf("PASS %u expanded RAM-prewarm controls\n",checks);return 0;
}catch(...){return 1;}}
