#define _CRT_SECURE_NO_WARNINGS
#include "../../fine-viewer/FineJob.h"
#include "../../compact-material/_generated/Shaders.h"
#include "../../staged-material/_generated/StagedShaders.h"
#include <cstdio>
// MODE
static unsigned checks=0;
static void C(bool ok,const char*name){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",name);throw 1;}std::printf("PASS %s\n",name);}
struct Exact {
// EXTRACTED_PAIR_MATCHES
};
// ROUTING_CONTROLS
// QUEUE_CONTROLS
static FineJob::Plan P(){
 FineJob::Plan p;p.state=FineJob::Detail;p.count=4;p.pairCount=2;p.spacing=1;p.demand.revision=1;
 p.demand.scope.thread=GetCurrentThreadId();p.demand.scope.world=0x100000001ULL;p.demand.scope.sourceEpoch=0x200000002ULL;p.demand.scope.job=0x300000003ULL;
 p.demand.view.world.x=-147456;p.demand.view.world.z=-147456;p.demand.view.world.w=1024;p.demand.view.world.h=256;p.demand.view.screenWidth=1024;p.demand.view.screenHeight=256;
 p.demand.view.stamp.source=p.demand.scope.sourceEpoch;p.demand.view.stamp.material=9;
 for(unsigned n=0;n<4;++n){FineWindows::Key k={1,n,0,p.demand.view.stamp};p.keys[n]=k;p.pairs[n/2].key[n%2]=k;p.pairs[n/2].wanted[n%2]=true;p.pairs[n/2].region=p.pairs[n/2].cell=0;}
 return p;
}
int main(){try{
 using namespace FineJob;Plan p=P();C(Internal::Fresh(p,p.demand),"four-key production fixture is fresh");
 Snapshot snapshot={};snapshot.stamp=p.demand.view.stamp;snapshot.material.source.value=p.demand.scope.sourceEpoch;snapshot.material.source.profile=WorldBank::PinnedBaseSnapshot;snapshot.material.generation=9;snapshot.material.region=0;
 Settings settings={16000,40000,0,0};WorldRaster::Shaders shaders={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{StagedMaterial::PartialShader,sizeof(StagedMaterial::PartialShader)},{StagedMaterial::TailShader,sizeof(StagedMaterial::TailShader)}};
 WorldCohort::Job job={};
#ifdef LEGACY
 C(MakeJob(p,0,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job),"existing pair can construct");
#else
 C(CanBatchNext(p,0)&&!CanBatchNext(p,1),"only bounded consecutive same-cell pair batches");
 C(MakeJob(p,0,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,2),"two existing pairs construct one native job");
#endif
 C(job.count==4,"two same-cell pairs share one four-view source job");
#ifndef LEGACY
 C(sizeof(Pair)==80&&sizeof(Plan)==57504,"existing Pair and Plan metadata storage unchanged");
 C(WorldCohort::ValidJob(job),"native four-view source/material contract remains valid");
 Queues(p,job);
 Routes(p,job);
 Plan padded=p;padded.count=3;padded.pairs[1].wanted[1]=false;WorldCohort::Job paddedJob={};C(MakeJob(padded,0,padded.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",paddedJob,2),"three wanted plus final companion forms four native views");Routes(padded,paddedJob);
 padded=p;padded.count=3;padded.keys[1]=p.keys[2];padded.keys[2]=p.keys[3];padded.pairs[0].wanted[1]=false;C(MakeJob(padded,0,padded.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",paddedJob,2),"unwanted second view in first pair remains legal companion");Routes(padded,paddedJob);
 for(unsigned n=0;n<4;++n){Placement place;C(Map(p.pairs[n/2].key[n%2],place)&&job.frame[n].x==place.x&&job.frame[n].z==place.z&&job.frame[n].side==256,"all four outputs keep exact original key geometry");}
 Exact exact;C(exact.PairMatches(p,0,p.demand,job),"coordinator accepts exact four-view snapshot");
 WorldCohort::Job changed=job;changed.frame[2].x+=1;C(!exact.PairMatches(p,0,p.demand,changed),"coordinator checks third frame identity");changed=job;changed.grid[3].x+=64;C(!exact.PairMatches(p,0,p.demand,changed),"coordinator checks fourth grid identity");changed=job;changed.scope.job^=1ULL<<32;C(!exact.PairMatches(p,0,p.demand,changed),"coordinator checks full64-bit scope identity");
 WorldCohort::Job unchanged=job;C(!MakeJob(p,1,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,2)&&!std::memcmp(&job,&unchanged,sizeof(job)),"last-pair span refuses before arrays and preserves output");
 C(!MakeJob(p,0,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,0)&&!MakeJob(p,0,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,3),"zero/unbounded span refused");
 Plan other=p;other.pairs[1].cell=1;C(!CanBatchNext(other,0)&&!MakeJob(other,0,other.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,2),"different native cell cannot share batch");
 other=p;other.pairs[1].key[0]=other.pairs[0].key[0];C(!CanBatchNext(other,0),"duplicate output keys refuse batch");
 other=p;other.count=Slots;other.pairCount=Slots+1;C(!CanBatchNext(other,0)&&!CanBatchNext(p,~0u),"malformed count and unsigned overflow index refuse before arrays");
 other=p;other.count=2;other.pairs[0].wanted[1]=other.pairs[1].wanted[1]=false;other.pairs[1].key[1]=other.pairs[0].key[1];C(!CanBatchNext(other,0),"duplicate unwanted companions cannot share batch");
 other=p;other.pairs[1].key[1].stamp.source^=1ULL<<32;C(!CanBatchNext(other,0),"stale high64-bit companion stamp refuses batch");
 other=p;other.pairs[1].wanted[1]=false;C(!MakeJob(other,0,other.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,2),"forged unwanted membership refuses exact job");
 other=p;other.demand.revision++;C(!MakeJob(p,0,other.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job,2),"changed viewport revision cannot reuse batch");
 C(MakeJob(p,0,p.demand,snapshot,settings,shaders,L"D:/tiny-height.tif",job)&&job.count==2&&exact.PairMatches(p,0,p.demand,job),"default remains original two-view center/prefetch job");
 Budget b={};b.viewerGpu=ViewerAllowance;b.producerCpu=Internal::ProducerCpuFloor();C(CanRender(p,p.demand,b,2)&&CanRender(p,p.demand,b,4)&&!CanRender(p,p.demand,b,3),"actual two/four count participates in render admission");
 // Use exact slot bytes near the real cap:2 fits while4 refuses.
 const U64 available=AtlasView::GpuCap-b.viewerGpu-WorldCohort::GpuReservation(2);b.occupied=unsigned(available/WindowBytes);if(b.occupied>ActiveSlots())b.occupied=ActiveSlots();b.fineGpu=U64(b.occupied)*WindowBytes;
 C(CanRender(p,p.demand,b,2)&&!CanRender(p,p.demand,b,4),"four-view extra reservation cannot borrow two-view headroom");
 UploadGate gate={1,true};C(!CanRead(p,p.demand,p.keys[0],b,gate)&&!CanUpload(p,p.demand,p.keys[0],b,gate),"one byte retained GPU still forbids cache publication");
 std::printf("PASS %u focused four-view controls; Pair %Iu Plan %Iu bytes\n",checks,sizeof(Pair),sizeof(Plan));
#endif
 return 0;
 }catch(...){return 1;}}
