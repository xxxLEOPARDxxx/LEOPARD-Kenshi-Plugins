#pragma once
#include "../../atlas-view/AtlasPolicy.h"
#include <cmath>
#include <float.h>
namespace FineWindows {
typedef AtlasView::U64 U64;
static const U64&GpuCap=AtlasView::GpuCap;
const U64 RawWindowBytes=256ULL*256*16;
enum{MaxJobs=GsmCacheProfile::MaxVisible,MaxScreen=16384,WorldExtent=294912,WorldOrigin=-147456};
struct Stamp{U64 source,material;};
struct Rect{double x,z,w,h;};
struct Key{unsigned spacing,x,z;Stamp stamp;};
struct Request{Rect world;unsigned screenWidth,screenHeight;Stamp stamp;bool apron;};
// All fields are explicit: source/raster peak are ADDITIONAL to retained bytes.
// windowBytes includes the three raw planes and any per-window display owner.
struct Budget{U64 limit,retained,sourcePeak,rasterPeak,windowBytes;unsigned capacity,unavailable;Stamp stamp;bool sourceAdmitted;};
enum Status{Invalid,Empty,Coarse,Detail};
enum Reason{BadInput,OutsideWorld,DensityEnough,SourceUnproved,StaleSource,NoCapacity,Admitted};
struct Job{Key key;double priority;bool visible;};
struct Plan{
 Job jobs[MaxJobs];unsigned count,visibleCount,spacing,densitySpacing,worstVisible,usableSlots;U64 peakBytes;Status status;Reason reason;
 Plan():count(0),visibleCount(0),spacing(9),densitySpacing(9),worstVisible(0),usableSlots(0),peakBytes(0),status(Invalid),reason(BadInput){std::memset(jobs,0,sizeof(jobs));}
};
inline bool Same(const Stamp&a,const Stamp&b){return a.source&&a.material&&a.source==b.source&&a.material==b.material;}
inline unsigned GridSide(unsigned spacing){return spacing==1||spacing==3||spacing==9?WorldExtent/(256*spacing):0;}
inline bool Valid(const Key&k){const unsigned n=GridSide(k.spacing);return n&&k.x<n&&k.z<n&&k.stamp.source&&k.stamp.material;}
inline bool Equal(const Key&a,const Key&b){return a.spacing==b.spacing&&a.x==b.x&&a.z==b.z&&Same(a.stamp,b.stamp);}
inline bool Bounds(const Key&k,Rect&out){if(!Valid(k))return false;const unsigned span=256*k.spacing;out.x=WorldOrigin+int(k.x*span);out.z=WorldOrigin+int(k.z*span);out.w=out.h=span;return true;}
inline bool PixelCenter(const Key&k,unsigned x,unsigned z,double&wx,double&wz){Rect r;if(x>=256||z>=256||!Bounds(k,r))return false;wx=r.x+(double(x)+.5)*k.spacing;wz=r.z+(double(z)+.5)*k.spacing;return true;}
inline bool TileLocal(const Key&k,unsigned&tile,unsigned&x,unsigned&z){if(!Valid(k))return false;const unsigned n=GridSide(k.spacing)/4;tile=k.x/n+4*(k.z/n);x=k.x%n;z=k.z%n;return true;}
inline bool Contains(const Plan&p,const Key&k,bool visibleOnly=false){if(p.count>MaxJobs||p.visibleCount>p.count)return false;for(unsigned i=0;i<(visibleOnly?p.visibleCount:p.count);++i)if(Equal(p.jobs[i].key,k))return true;return false;}
inline bool CanPublish(const Plan&p,const Key&requested,const Key&completed,bool success){return p.status==Detail&&success&&Valid(requested)&&Equal(requested,completed)&&Contains(p,requested);}
inline bool UseFine(const Plan&p,const Key&wanted,const Key&completed,bool success){return CanPublish(p,wanted,completed,success)&&Contains(p,wanted,true);}
namespace Internal {
struct Clip{double x0,z0,x1,z1;};
inline double Low(double a,double b){return a<b?a:b;}inline double High(double a,double b){return a>b?a:b;}
inline unsigned WorstAxis(double extent,unsigned spacing){if(extent>=WorldExtent)return GridSide(spacing);double n=std::ceil(extent/(256*spacing));if(n<1)n=1;return unsigned(Low(GridSide(spacing),n+1));}
inline unsigned Worst(const Request&r,unsigned spacing){return WorstAxis(r.world.w,spacing)*WorstAxis(r.world.h,spacing);}
inline unsigned Index(double x,unsigned spacing){
 const unsigned n=GridSide(spacing),span=256*spacing;unsigned q=unsigned(std::floor((x-WorldOrigin)/span));if(q>=n)q=n-1;
 // Comparing exact integer world boundaries repairs quotient rounding at zero
 // (including subnormal +/- world coordinates), without an arbitrary epsilon.
 if(q&&x<WorldOrigin+int(q*span))--q;if(q+1<n&&x>=WorldOrigin+int((q+1)*span))++q;return q;
}
inline unsigned Last(double x,unsigned spacing){unsigned q=Index(x,spacing);if(q&&x==WorldOrigin+int(q*256*spacing))--q;return q;}
inline double Score(unsigned x,unsigned z,unsigned spacing,const Clip&c){double dx=WorldOrigin+int(x*256*spacing)+128.0*spacing-(c.x0+c.x1)*.5,dz=WorldOrigin+int(z*256*spacing)+128.0*spacing-(c.z0+c.z1)*.5;return dx*dx+dz*dz;}
inline bool Before(const Job&a,const Job&b){return a.priority<b.priority||(a.priority==b.priority&&(a.key.z<b.key.z||(a.key.z==b.key.z&&a.key.x<b.key.x)));}
inline void Insert(Plan&p,const Job&j,unsigned first,unsigned limit){
 if(p.count>=limit&&!Before(j,p.jobs[p.count-1]))return;
 if(p.count<limit)++p.count;unsigned at=p.count-1;while(at>first&&Before(j,p.jobs[at-1])){p.jobs[at]=p.jobs[at-1];--at;}p.jobs[at]=j;
}
}
inline Plan MakePlan(const Request&r,const Budget&b){
 Plan p;const Rect&w=r.world;
 if(!_finite(w.x)||!_finite(w.z)||!_finite(w.w)||!_finite(w.h)||w.w<=0||w.h<=0||!_finite(w.x+w.w)||!_finite(w.z+w.h)||w.x+w.w<=w.x||w.z+w.h<=w.z||!r.screenWidth||!r.screenHeight||r.screenWidth>MaxScreen||r.screenHeight>MaxScreen||!r.stamp.source||!r.stamp.material||!b.limit||b.limit>GpuCap||b.capacity>MaxJobs||b.unavailable>b.capacity||b.windowBytes<RawWindowBytes||b.windowBytes>GpuCap||b.retained<U64(b.unavailable)*b.windowBytes)return p;
 Internal::Clip c={Internal::High(WorldOrigin,w.x),Internal::High(WorldOrigin,w.z),Internal::Low(-WorldOrigin,w.x+w.w),Internal::Low(-WorldOrigin,w.z+w.h)};
 if(c.x1<=c.x0||c.z1<=c.z0){p.status=Empty;p.reason=OutsideWorld;return p;}
 const double mpp=Internal::Low(w.w/r.screenWidth,w.h/r.screenHeight);p.densitySpacing=mpp<3?1:mpp<9?3:9;p.status=Coarse;p.reason=DensityEnough;
 if(p.densitySpacing==9)return p;
 if(!b.sourceAdmitted||!b.sourcePeak||!b.rasterPeak){p.reason=SourceUnproved;return p;}
 if(!Same(r.stamp,b.stamp)){p.reason=StaleSource;return p;}
 p.reason=NoCapacity;U64 head=b.limit;
 if(b.retained>head)return p;head-=b.retained;if(b.sourcePeak>head)return p;head-=b.sourcePeak;if(b.rasterPeak>head)return p;head-=b.rasterPeak;
 p.usableSlots=unsigned(Internal::Low(b.capacity-b.unavailable,double(head/b.windowBytes)));
 for(unsigned spacing=p.densitySpacing;spacing<9;spacing*=3){const unsigned worst=Internal::Worst(r,spacing);if(worst<=p.usableSlots){p.spacing=spacing;p.worstVisible=worst;break;}}
 if(p.spacing==9)return p;
 const unsigned x0=Internal::Index(c.x0,p.spacing),z0=Internal::Index(c.z0,p.spacing),x1=Internal::Last(c.x1,p.spacing),z1=Internal::Last(c.z1,p.spacing);
 const unsigned visible=(x1-x0+1)*(z1-z0+1);if(visible>p.worstVisible||visible>p.usableSlots){p.spacing=9;return p;}
 for(unsigned z=z0;z<=z1;++z)for(unsigned x=x0;x<=x1;++x){Job j={{p.spacing,x,z,r.stamp},Internal::Score(x,z,p.spacing,c),true};Internal::Insert(p,j,0,p.usableSlots);}
 p.visibleCount=p.count;
 if(r.apron&&p.usableSlots>p.worstVisible){const unsigned n=GridSide(p.spacing),endX=x1+1<n?x1+1:x1,endZ=z1+1<n?z1+1:z1,limit=p.visibleCount+p.usableSlots-p.worstVisible;
  for(unsigned z=z0?z0-1:0;z<=endZ;++z)for(unsigned x=x0?x0-1:0;x<=endX;++x){if(x>=x0&&x<=x1&&z>=z0&&z<=z1)continue;Job j={{p.spacing,x,z,r.stamp},Internal::Score(x,z,p.spacing,c),false};Internal::Insert(p,j,p.visibleCount,limit);}
 }
 p.peakBytes=b.retained+b.sourcePeak+b.rasterPeak+U64(p.worstVisible+p.count-p.visibleCount)*b.windowBytes;p.status=Detail;p.reason=Admitted;return p;
}
}
