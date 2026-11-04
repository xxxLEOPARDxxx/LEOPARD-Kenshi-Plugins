#define BOOST_ALL_NO_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY
#include "../../GsmFinePolicy.h"
#include "../../atlas-terrain/fine-viewer/FineJob.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
static FineJob::Demand View(double zoom){FineJob::Demand d={};d.scope.thread=GetCurrentThreadId();d.scope.world=d.scope.sourceEpoch=d.scope.job=d.revision=1;d.view.stamp.source=1;d.view.stamp.material=2;d.view.screenWidth=2160;d.view.screenHeight=1044;d.view.world.x=-55000;d.view.world.z=0;d.view.world.w=2160.0*73728/(4096*zoom);d.view.world.h=1044.0*73728/(4096*zoom);return d;}
static FineJob::Budget Budget(){FineJob::Budget b={};b.viewerGpu=31522078;b.producerCpu=FineJob::Internal::ProducerCpuFloor();return b;}
int main(int argc,char**argv){try{
 C(GsmCacheProfile::Configure(argc>1?std::atoi(argv[1]):1,32),"independent real GPU profile");
 const double zooms[]={2,2.5,3,4,6,8};const unsigned worst[]={378,242,180,112,60,40};
 for(unsigned i=0;i<6;++i){FineJob::Demand d=View(zooms[i]);FineJob::Plan p=FineJob::MakePlan(d,Budget(),FineJob::VisibleSet);
  C(FineWindows::Internal::Worst(d.view,3)==worst[i],"measured native2160x1044 coverage matches density");
  C(GsmFinePolicy::CanRefine(d.view)&&p.state==FineJob::Detail,"all fixed x2..x8 steps admit whole visible refinement");
  C(p.spacing==3&&p.count<=512,"larger capacity does not silently multiply x8 source work by selecting1m");
  for(unsigned k=0;k<p.count;++k){FineJob::Placement at;C(FineJob::Map(p.keys[k],at)&&at.grid.columns*at.grid.rows<=4,"every actual visible key preserves native lattice mapping");}
  std::printf("zoom=%.2f mpp=%.3f worst3=%u count=%u pairs=%u\n",zooms[i],d.view.world.w/d.view.screenWidth,worst[i],p.count,p.pairCount);
 }
 FineJob::Demand d=View(2);d.view.world.w*=1.000000000001;d.view.world.h*=1.000000000001;
 C(!GsmFinePolicy::CanRefine(d.view)&&FineJob::MakePlan(d,Budget(),FineJob::VisibleSet).state==FineJob::Coarse,"density genuinely above9 still selects ordinary source");
 d=View(8);d.view.screenWidth=d.view.screenHeight=1024;d.view.world.w=d.view.world.h=2048;
 C(FineJob::MakePlan(d,Budget(),FineJob::VisibleSet).spacing==1,"old whole1m81-window case keeps its quality");
 d.view.world.w=d.view.world.h=2304;
 C(FineWindows::Internal::Worst(d.view,1)==100&&FineJob::MakePlan(d,Budget(),FineJob::VisibleSet).spacing==3,"new512 cap does not upgrade old100-window1m refusal");
 d=View(2);d.view.world.w=d.view.world.h=50000;
 C(!GsmFinePolicy::CanRefine(d.view),"oversize viewport remains safely bounded");
 std::printf("PASS %u fixed-step production LOD controls\n",checks);return 0;
 }catch(...){return 1;}}
