#pragma once
#include "../world-layout/WorldLayout.h"
#include "../world-cohort/WorldCohort.h"
namespace FineJob {
typedef FineWindows::U64 U64;
enum{MaxVisible=GsmCacheProfile::MaxVisible,Slots=MaxVisible,CacheSlots=GsmCacheProfile::FineCacheSlots};
const U64 ViewerAllowance=GsmCacheProfile::ViewerBytes,WindowBytes=256ULL*256*20;
inline unsigned ActiveSlots(){return GsmCacheProfile::FineSlots();}
struct Demand{FineWindows::Request view;SourceReceipts::Scope scope;U64 revision;};
struct Budget{U64 viewerGpu,fineGpu,producerCpu,otherCpu;unsigned occupied;};
struct UploadGate{U64 producerReservedGpu;bool consumersJoined;};
struct Placement{FineWindows::Key key;unsigned region,cell;float x,z;WorldRaster::Grid grid;};
struct Pair{FineWindows::Key key[2];bool wanted[2];unsigned region,cell;};
enum Mode{CenterPair,VisibleSet};
enum State{Invalid,Empty,Coarse,Detail};
struct Plan{Demand demand;FineWindows::Key keys[Slots];Pair pairs[Slots];unsigned count,pairCount,spacing,densitySpacing,worstVisible;U64 producerPeak,publicationPeak,cpuPeak;State state;Plan():count(0),pairCount(0),spacing(9),densitySpacing(9),worstVisible(0),producerPeak(0),publicationPeak(0),cpuPeak(0),state(Invalid){std::memset(&demand,0,sizeof(demand));std::memset(keys,0,sizeof(keys));std::memset(pairs,0,sizeof(pairs));}};
struct Snapshot{FineWindows::Stamp stamp;WorldRaster::Material material;};
struct Settings{float cameraY,farClip,waterHeight,wetness;};
// All values are copied; this policy owns no engine/resource pointers.
inline bool Map(const FineWindows::Key&key,Placement&out){
 WorldLayout::Placement place;if(!WorldLayout::Fine(key,place))return false;
 Placement p={};p.key=key;p.region=place.region;p.cell=place.cell;p.x=float(place.centreX);p.z=float(place.centreZ);
 p.grid.x=place.gridX;p.grid.z=place.gridZ;p.grid.columns=place.columns;p.grid.rows=place.rows;p.grid.sourceEpoch=key.stamp.source;
 if(!WorldRaster::ValidGrid(p.grid)||p.grid.columns*p.grid.rows>4)return false;out=p;return true;
}
namespace Internal{
inline bool DemandValid(const Demand&r){
 const FineWindows::Rect&w=r.view.world;
 return SourceReceipts::ScopeValid(r.scope)&&r.revision&&FineWindows::Same(r.view.stamp,r.view.stamp)&&r.view.stamp.source==r.scope.sourceEpoch&&!r.view.apron&&
 _finite(w.x)&&_finite(w.z)&&_finite(w.w)&&_finite(w.h)&&w.w>0&&w.h>0&&_finite(w.x+w.w)&&_finite(w.z+w.h)&&w.x+w.w>w.x&&w.z+w.h>w.z&&
 r.view.screenWidth&&r.view.screenHeight&&r.view.screenWidth<=FineWindows::MaxScreen&&r.view.screenHeight<=FineWindows::MaxScreen;
}
inline bool SameDemand(const Demand&a,const Demand&b){
 return DemandValid(a)&&DemandValid(b)&&SourceReceipts::SameScope(a.scope,b.scope)&&a.revision==b.revision&&FineWindows::Same(a.view.stamp,b.view.stamp)&&
 a.view.screenWidth==b.view.screenWidth&&a.view.screenHeight==b.view.screenHeight&&a.view.apron==b.view.apron&&a.view.world.x==b.view.world.x&&a.view.world.z==b.view.world.z&&a.view.world.w==b.view.world.w&&a.view.world.h==b.view.world.h;
}
// Necessary lower floor from the frozen producer's actual payloads. It excludes
// template/controller/provider overhead; caller MUST supply the real owning
// NativeSession::CpuReservation(), plus all other live owners in otherCpu.
inline U64 ProducerCpuFloor(){return sizeof(WorldCohort::Work)+WorldHeight::MaxSamples*2+32ULL*1024*1024+512ULL*512*16+WorldHeight::AuthScratch+WorldPixels::Accepted::RequiredBytes(512)-sizeof(WorldPixels::Accepted);}
inline bool BudgetValid(const Budget&b){
 return b.viewerGpu<=ViewerAllowance&&b.occupied<=ActiveSlots()&&b.fineGpu>=U64(b.occupied)*WindowBytes&&b.fineGpu<=U64(ActiveSlots())*WindowBytes&&
 b.producerCpu>=ProducerCpuFloor()&&b.producerCpu<=AtlasView::CpuCap&&b.otherCpu<=AtlasView::CpuCap-b.producerCpu;
}
// Fixed metadata scratch: validate a large retained neighborhood without an
// O(N*N) comparison loop or per-pan heap allocations. Load is at most3/8.
inline bool ResidentsValid(const FineWindows::Key*resident,unsigned count,const FineWindows::Stamp&stamp){
 if(!resident||count>ActiveSlots())return false;
 unsigned short index[16384]={};
 for(unsigned n=0;n<count;++n){const FineWindows::Key&k=resident[n];
  if(!FineWindows::Valid(k)||!FineWindows::Same(k.stamp,stamp))return false;
  unsigned at=(k.x*73856093u^k.z*19349663u^k.spacing*83492791u)&16383u,probes=0;
  while(index[at]){if(FineWindows::Equal(k,resident[index[at]-1]))return false;if(++probes==16384)return false;at=(at+1)&16383u;}
  index[at]=static_cast<unsigned short>(n+1);
 }return true;
}
inline bool ProducerFits(const Budget&b,unsigned views=2){return WorldCohort::CountValid(views)&&BudgetValid(b)&&b.viewerGpu+b.fineGpu<=AtlasView::GpuCap-WorldCohort::GpuReservation(views);}
inline bool Fresh(const Plan&p,const Demand&r){return p.state==Detail&&p.count&&p.count<=Slots&&p.pairCount&&p.pairCount<=p.count&&(p.spacing==1||p.spacing==3)&&SameDemand(p.demand,r);}
inline bool Has(const Plan&p,const FineWindows::Key&k){if(p.count>Slots||!FineWindows::Valid(k)||!FineWindows::Same(k.stamp,p.demand.view.stamp)||k.spacing!=p.spacing)return false;for(unsigned n=0;n<p.count;++n)if(FineWindows::Equal(k,p.keys[n]))return true;return false;}
inline bool SameCell(const FineWindows::Key&a,const FineWindows::Key&b){Placement x,y;return Map(a,x)&&Map(b,y)&&x.region==y.region&&x.cell==y.cell;}
inline bool Companion(const Plan&p,const FineWindows::Key&first,FineWindows::Key&out){
 const unsigned n=9216/(256*first.spacing),x0=(first.x/n)*n,z0=(first.z/n)*n,at=(first.z%n)*n+first.x%n;
 for(unsigned offset=1;offset<n*n;++offset){unsigned i=(at+offset)%(n*n);FineWindows::Key k=first;k.x=x0+i%n;k.z=z0+i/n;if(!Has(p,k)){out=k;return true;}}return false;
}
inline bool Group(Plan&p){
 bool used[Slots]={};p.pairCount=0;
 for(unsigned n=0;n<p.count;++n)if(!used[n]){Pair&q=p.pairs[p.pairCount++];q.key[0]=p.keys[n];q.wanted[0]=true;used[n]=true;Placement place;if(!Map(q.key[0],place))return false;q.region=place.region;q.cell=place.cell;
  unsigned mate=n+1;while(mate<p.count&&(used[mate]||!SameCell(q.key[0],p.keys[mate])))++mate;
  if(mate<p.count){q.key[1]=p.keys[mate];q.wanted[1]=true;used[mate]=true;}else{q.wanted[1]=false;if(!Companion(p,q.key[0],q.key[1]))return false;}
 }return true;
}
}
inline Plan MakePlan(const Demand&r,const Budget&b,Mode mode,const FineWindows::Key*resident=NULL,unsigned residentCount=0){
 Plan p;if(!Internal::DemandValid(r)||(mode!=CenterPair&&mode!=VisibleSet))return p;p.demand=r;
 const FineWindows::Rect&w=r.view.world;FineWindows::Internal::Clip clip={FineWindows::Internal::High(-147456,w.x),FineWindows::Internal::High(-147456,w.z),FineWindows::Internal::Low(147456,w.x+w.w),FineWindows::Internal::Low(147456,w.z+w.h)};
 if(clip.x1<=clip.x0||clip.z1<=clip.z0){p.state=Empty;return p;}
 const double density=FineWindows::Internal::Low(w.w/r.view.screenWidth,w.h/r.view.screenHeight);p.densitySpacing=density<3&&FineWindows::Internal::Worst(r.view,1)<=GsmCacheProfile::OneMetreVisible?1:density<=9?3:9;p.state=Coarse;
 // Planning/cache reuse does not allocate a source bank. A retained visible
 // set may exceed the production phase allowance; CanRender checks it later.
 if(p.densitySpacing==9||!Internal::BudgetValid(b))return p;
 if(resident){if(residentCount!=b.occupied||!Internal::ResidentsValid(resident,residentCount,r.view.stamp))return Plan();}
 else if(residentCount)return Plan();
 unsigned overlap=0,required=0,available=MaxVisible;
 for(unsigned spacing=p.densitySpacing;spacing<9;spacing*=3){unsigned worst=FineWindows::Internal::Worst(r.view,spacing),matching=0;for(unsigned n=0;resident&&n<residentCount;++n){FineWindows::Rect v;if(resident[n].spacing==spacing&&FineWindows::Bounds(resident[n],v)&&v.x<clip.x1&&clip.x0<v.x+v.w&&v.z<clip.z1&&clip.z0<v.z+v.h)++matching;}
  const unsigned actual=(FineWindows::Internal::Last(clip.x1,spacing)-FineWindows::Internal::Index(clip.x0,spacing)+1)*(FineWindows::Internal::Last(clip.z1,spacing)-FineWindows::Internal::Index(clip.z0,spacing)+1);
  if(worst>MaxVisible)continue;
  if(matching<=actual&&actual-matching<=ActiveSlots()-b.occupied&&U64(actual-matching)*WindowBytes<=AtlasView::GpuCap-b.viewerGpu-b.fineGpu){p.spacing=spacing;p.worstVisible=worst;overlap=matching;required=actual;break;}
  // Physical neighborhood pressure must retire old owners, not permanently
  // downgrade a feasible1m demand merely because cached3m tiles already fit.
  return p;}
 if(p.spacing==9)return p;
 FineWindows::Plan sorted;const unsigned x0=FineWindows::Internal::Index(clip.x0,p.spacing),z0=FineWindows::Internal::Index(clip.z0,p.spacing),x1=FineWindows::Internal::Last(clip.x1,p.spacing),z1=FineWindows::Internal::Last(clip.z1,p.spacing);
 if((x1-x0+1)*(z1-z0+1)>p.worstVisible)return Plan();
 for(unsigned z=z0;z<=z1;++z)for(unsigned x=x0;x<=x1;++x){FineWindows::Job j={{p.spacing,x,z,r.view.stamp},FineWindows::Internal::Score(x,z,p.spacing,clip),true};FineWindows::Internal::Insert(sorted,j,0,available);}
 if(!sorted.count)return Plan();
 if(mode==VisibleSet){p.count=sorted.count;for(unsigned n=0;n<p.count;++n)p.keys[n]=sorted.jobs[n].key;}
 else{p.keys[0]=sorted.jobs[0].key;p.count=1;for(unsigned n=1;n<sorted.count;++n)if(Internal::SameCell(p.keys[0],sorted.jobs[n].key)){p.keys[p.count++]=sorted.jobs[n].key;break;}}
 if(!Internal::Group(p))return Plan();p.producerPeak=b.viewerGpu+b.fineGpu+WorldCohort::GpuReservation(2);p.publicationPeak=b.viewerGpu+b.fineGpu+U64(required-overlap)*WindowBytes;p.cpuPeak=b.producerCpu+b.otherCpu;p.state=Detail;return p;
}
inline bool Wanted(const Plan&p,const Demand&now,const FineWindows::Key&key){return Internal::Fresh(p,now)&&Internal::Has(p,key);}
inline bool CanRender(const Plan&p,const Demand&now,const Budget&b,unsigned views=2){return Internal::Fresh(p,now)&&Internal::ProducerFits(b,views);}
// Gate values must come from the real owner AFTER known consumer join and GPU
// drain. These copied values are policy inputs, not fabricated native receipts.
inline bool CanRead(const Plan&p,const Demand&now,const FineWindows::Key&key,const Budget&b,const UploadGate&gate){
 return Wanted(p,now,key)&&Internal::BudgetValid(b)&&gate.consumersJoined&&gate.producerReservedGpu==0;
}
inline bool CanUpload(const Plan&p,const Demand&now,const FineWindows::Key&key,const Budget&b,const UploadGate&gate){
 return Wanted(p,now,key)&&Internal::BudgetValid(b)&&gate.consumersJoined&&gate.producerReservedGpu==0&&b.occupied<ActiveSlots()&&
 b.fineGpu<=U64(ActiveSlots()-1)*WindowBytes&&b.viewerGpu+b.fineGpu<=AtlasView::GpuCap-WindowBytes;
}
// Reuse two existing consecutive pairs; Plan/Pair storage and cache keys stay256.
inline bool CanBatchNext(const Plan&p,unsigned pair){
 if(p.state!=Detail||p.count>Slots||p.pairCount>p.count||pair>=p.pairCount||p.pairCount-pair<2)return false;
 const Pair&a=p.pairs[pair],&b=p.pairs[pair+1];if(a.region!=b.region||a.cell!=b.cell)return false;
 for(unsigned n=0;n<4;++n){const FineWindows::Key&key=p.pairs[pair+n/2].key[n%2];Placement place;if(!Map(key,place)||place.region!=a.region||place.cell!=a.cell||!FineWindows::Same(key.stamp,p.demand.view.stamp))return false;for(unsigned k=0;k<n;++k)if(FineWindows::Equal(key,p.pairs[pair+k/2].key[k%2]))return false;}return true;
}
inline bool MakeJob(const Plan&p,unsigned pair,const Demand&now,const Snapshot&snapshot,const Settings&settings,const WorldRaster::Shaders&shaders,const wchar_t*path,WorldCohort::Job&out,unsigned pairSpan=1){
 if(!Internal::Fresh(p,now)||pair>=p.pairCount||(pairSpan!=1&&pairSpan!=2)||(pairSpan==2&&!CanBatchNext(p,pair))||!FineWindows::Same(snapshot.stamp,p.demand.view.stamp)||!path)return false;
 unsigned length=0;while(length<260&&path[length])++length;if(length<3||length>=260||!((path[0]>=L'A'&&path[0]<=L'Z')||(path[0]>=L'a'&&path[0]<=L'z'))||path[1]!=L':'||(path[2]!=L'/'&&path[2]!=L'\\'))return false;
 const Pair&q=p.pairs[pair];Placement place[4];for(unsigned index=0;index<pairSpan;++index){const Pair&selected=p.pairs[pair+index];if(!selected.wanted[0]||!Internal::Has(p,selected.key[0])||selected.wanted[1]!=Internal::Has(p,selected.key[1])||!Map(selected.key[0],place[index*2])||!Map(selected.key[1],place[index*2+1])||!Internal::SameCell(selected.key[0],selected.key[1])||FineWindows::Equal(selected.key[0],selected.key[1])||selected.region!=place[index*2].region||selected.cell!=place[index*2].cell||selected.region!=q.region||selected.cell!=q.cell)return false;}
 if(snapshot.material.region!=q.region||snapshot.material.source.value!=now.scope.sourceEpoch||!WorldBank::ValidEpoch(snapshot.material.source))return false;
 WorldCohort::Job j={};j.scope=now.scope;j.material=snapshot.material;j.shaders=shaders;j.count=2*pairSpan;std::memcpy(j.heightPath,path,(length+1)*sizeof(wchar_t));
 for(unsigned n=0;n<j.count;++n){const Placement&a=place[n];WorldRaster::Frame&f=j.frame[n];f.source=j.material.source;f.generation=j.scope.job;f.materialGeneration=j.material.generation;f.region=a.region;f.cell=a.cell;f.side=256;f.x=a.x;f.z=a.z;f.spacing=float(p.pairs[pair+n/2].key[n%2].spacing);f.cameraY=settings.cameraY;f.farClip=settings.farClip;f.waterHeight=settings.waterHeight;f.wetness=settings.wetness;j.grid[n]=a.grid;}
 WorldMaterial::Planner planner;if(!planner.Init(j.scope.sourceEpoch)||!planner.Build(q.region,q.cell,j.scope.sourceEpoch,0,j.plan)||!WorldCohort::ValidJob(j))return false;out=j;return true;
}
}
