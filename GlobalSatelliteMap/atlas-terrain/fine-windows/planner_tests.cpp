#define _CRT_SECURE_NO_WARNINGS
#include "FineWindowPlanner.h"
#include <cstdio>
#include <limits>
#include <new>
#include <cstdlib>
static unsigned heapCalls=0;void*operator new(size_t n){++heapCalls;void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}
void operator delete(void*p)throw(){std::free(p);}void*operator new[](size_t n){return ::operator new(n);}void operator delete[](void*p)throw(){::operator delete(p);}
using namespace FineWindows;
static unsigned checks=0;static void C(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static Request Req(double x,double z,double w,double h,unsigned sx=1920,unsigned sy=1080){Request r={{x,z,w,h},sx,sy,{0x100000007ULL,0x200000009ULL},true};return r;}
static Budget Money(const Request&r){Budget b={192ULL*1024*1024,20ULL*1024*1024,80ULL*1024*1024,2ULL*1024*1024,1310720,128,0,r.stamp,true};return b;}
static void CheckPlan(const Request&r,const Budget&b,const Plan&p){
 C(p.count<=128&&p.visibleCount<=p.count,"bounded counts");if(p.status!=Detail){C(p.count==0,"fallback has no partial fine job");return;}
 C(p.spacing==1||p.spacing==3,"only worthwhile fine jobs");C(p.worstVisible>=p.visibleCount,"all-phase visible reservation");C(p.count-p.visibleCount<=p.usableSlots-p.worstVisible,"apron uses worst-phase spare");
 const double loX=r.world.x< -147456?-147456:r.world.x,loZ=r.world.z< -147456?-147456:r.world.z,hiX=r.world.x+r.world.w>147456?147456:r.world.x+r.world.w,hiZ=r.world.z+r.world.h>147456?147456:r.world.z+r.world.h;
 const unsigned span=p.spacing*256,fx=unsigned(std::floor((loX+147456)/span)),fz=unsigned(std::floor((loZ+147456)/span)),tx=unsigned(std::ceil((hiX+147456)/span)),tz=unsigned(std::ceil((hiZ+147456)/span));
 C(p.visibleCount==(tx-fx)*(tz-fz),"independent clipped half-open reference count");
 for(unsigned z=fz;z<tz;++z)for(unsigned x=fx;x<tx;++x){Key k={p.spacing,x,z,r.stamp};C(Contains(p,k,true),"every independently visible reference key covered");}
 C(p.peakBytes<=b.limit,"explicit source-aware GPU admission");C(p.peakBytes==b.retained+b.sourcePeak+b.rasterPeak+U64(p.worstVisible+p.count-p.visibleCount)*b.windowBytes,"full actual GPU ledger");
 for(unsigned i=0;i<p.count;++i){C(Valid(p.jobs[i].key)&&Same(p.jobs[i].key.stamp,r.stamp),"valid complete stamped key");C(p.jobs[i].visible==(i<p.visibleCount),"all visible jobs precede apron");Rect box;C(Bounds(p.jobs[i].key,box),"priority bounds");const double dx=box.x+box.w*.5-(loX+hiX)*.5,dz=box.z+box.h*.5-(loZ+hiZ)*.5;C(p.jobs[i].priority==dx*dx+dz*dz,"priority is actual world-center distance including negative coordinates");if(i&&i!=p.visibleCount)C(p.jobs[i-1].priority<=p.jobs[i].priority,"near-center deterministic priority");for(unsigned j=0;j<i;++j)C(!Equal(p.jobs[i].key,p.jobs[j].key),"unique keys");}
}
int main(){try{
 C(GridSide(9)==128&&GridSide(3)==384&&GridSide(1)==1152&&!GridSide(2),"actual independent global grids");
 for(unsigned step=1;step<=9;step*=3){unsigned n=GridSide(step);for(unsigned t=0;t<4;++t){Key k={step,t*(n/4),0,{7,9}};Rect a;C(Bounds(k,a)&&a.x==-147456.0+t*73728&&a.w==256*step,"logical tiles exact at every spacing");unsigned tile,x,z;C(TileLocal(k,tile,x,z)&&tile==t&&x==0&&z==0,"global to tile-local mapping");}Key last={step,n-1,n-1,{7,9}};Rect a;C(Bounds(last,a)&&a.x+a.w==147456&&a.z+a.h==147456,"positive world edge exact");}
 // The ratio is odd: every9m pixel center exists exactly on3m and1m grids.
 for(unsigned g=0;g<32768;g+=97){unsigned p9=g%256,x9=g/256;Key a={9,x9,0,{7,9}},b={3,(g*3+1)/256,0,{7,9}},c={1,(g*9+4)/256,0,{7,9}};double ax,az,bx,bz,cx,cz;C(PixelCenter(a,p9,0,ax,az)&&PixelCenter(b,(g*3+1)%256,1,bx,bz)&&PixelCenter(c,(g*9+4)%256,4,cx,cz),"common-world center lookup");C(ax==-147456+9.0*g+4.5&&az==-147456+4.5,"absolute native pixel-center formula");C(ax==bx&&bx==cx&&az==bz&&bz==cz,"1m/3m/9m common centers have no camera phase");}
 Request r=Req(-1000,-500,1000,500);Budget b=Money(r);Plan p=MakePlan(r,b);C(p.status==Detail&&p.spacing==1&&p.worstVisible==15,"fine1 useful and all-phase fit");CheckPlan(r,b,p);
 for(int pan=-300;pan<=300;++pan){Request q=r;q.world.x+=pan*.37;q.world.z+=pan*.29;Plan v=MakePlan(q,b);C(v.status==Detail&&v.spacing==1&&v.worstVisible==p.worstVisible,"translation cannot phase-flip zoom");CheckPlan(q,b,v);}
 r=Req(-4096,-2304,4096,2304);b=Money(r);p=MakePlan(r,b);C(p.status==Detail&&p.densitySpacing==1&&p.spacing==3&&p.worstVisible==28,"capacity promotes1 to bounded3");CheckPlan(r,b,p);
 for(int pan=-50;pan<=50;++pan){Request q=r;q.world.x+=pan*37.1;q.world.z+=pan*19.3;Plan v=MakePlan(q,b);C(v.spacing==p.spacing,"coarser admission also stable under pan");}
 r=Req(-1000,-500,1920*3.0,1080*3.0);b=Money(r);p=MakePlan(r,b);C(p.densitySpacing==3,"density equality3m does not request1m");r.world.w=1920*9.0;r.world.h=1080*9.0;p=MakePlan(r,b);C(p.status==Coarse&&p.spacing==9&&!p.count,"density equality9m uses base only");
 r=Req(-18432,-10368,294912.0/8,165888.0/8);b=Money(r);p=MakePlan(r,b);C(p.status==Coarse&&p.densitySpacing==9,"actual zoom8 screen density does not promise universal1m");
 r=Req(-147500,-147500,1000,500);b=Money(r);p=MakePlan(r,b);C(p.status==Detail&&p.spacing==1,"world clipping does not promote/change density");CheckPlan(r,b,p);for(unsigned i=0;i<p.count;++i)C(Valid(p.jobs[i].key),"negative world-edge clipping");
 r.world.x=147000;r.world.z=147200;p=MakePlan(r,b);C(p.spacing==1&&p.worstVisible==15,"positive edge retains worst-phase reservation");CheckPlan(r,b,p);r.world.x=147456;p=MakePlan(r,b);C(p.status==Empty&&!p.count,"half-open positive world boundary empty");
 r=Req(-1e-300,-1e-300,2e-300,2e-300);b=Money(r);r.apron=false;p=MakePlan(r,b);C(p.status==Detail&&p.visibleCount==4,"tiny crossing at exact global boundary cannot disappear to rounding");
 r=Req(0,0,256,256,256,256);b=Money(r);r.apron=false;p=MakePlan(r,b);C(p.status==Detail&&p.visibleCount==1&&p.worstVisible==4,"half-open exact window not duplicated");
 r=Req(-1000,-500,1000,500);b=Money(r);p=MakePlan(r,b);Key wanted=p.jobs[0].key;C(CanPublish(p,wanted,wanted,true)&&UseFine(p,wanted,wanted,true),"exact successful visible completion uses fine");
 Key wrong=wanted;wrong.stamp.source^=1ULL<<32;C(!CanPublish(p,wanted,wrong,true)&&!UseFine(p,wanted,wrong,true),"stale source high32 keeps coarse");wrong=wanted;wrong.stamp.material^=1ULL<<32;C(!UseFine(p,wanted,wrong,true),"stale material high32 keeps coarse");wrong=wanted;wrong.x++;C(!UseFine(p,wanted,wrong,true)&&!UseFine(p,wanted,wanted,false),"wrong window and incomplete job keep coarse");
 C(p.count>p.visibleCount,"optional apron present with spare budget");Key apron=p.jobs[p.visibleCount].key;C(CanPublish(p,apron,apron,true)&&!UseFine(p,apron,apron,true),"apron may cache but cannot replace visible fallback");
 Plan malformed=p;malformed.count=129;C(!CanPublish(malformed,wanted,wanted,true),"malformed external plan cannot publish");
 Request moved=r;moved.world.x=100000;moved.world.z=100000;Plan newer=MakePlan(moved,b);C(!CanPublish(newer,wanted,wanted,true),"old view completion not published to unrelated plan");
 b.sourceAdmitted=false;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"unproved source never assumes free192MiB");b=Money(r);b.stamp.material++;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"stale material budget proof refuses fine");
 b=Money(r);b.sourcePeak=0;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"missing actual source peak refuses fine");b=Money(r);b.retained=189006000;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"old144-window reservation plus actual sources cannot fit");
 b=Money(r);b.capacity=7;p=MakePlan(r,b);CheckPlan(r,b,p);C(p.spacing==3&&p.count-p.visibleCount==1,"one spare beyond worst phase permits exactly one apron");
 b=Money(r);b.capacity=6;p=MakePlan(r,b);C(p.status==Detail&&p.spacing==3&&p.worstVisible==6&&p.count==p.visibleCount,"capacity bound chooses3 without opportunistic overcommit");b.capacity=5;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"no partial visible coverage on capacity failure");
 b=Money(r);b.capacity=20;b.unavailable=5;b.retained=20ULL*1024*1024;p=MakePlan(r,b);C(p.spacing==1&&p.usableSlots==15&&p.count==p.visibleCount,"unavailable slots protected before apron");
 b=Money(r);b.limit=b.retained+b.sourcePeak+b.rasterPeak+15*b.windowBytes;p=MakePlan(r,b);C(p.spacing==1&&p.usableSlots==15,"exact GPU boundary fits finest");b.limit--;p=MakePlan(r,b);C(p.spacing==3,"GPU boundary minus1 falls back safely");
 b=Money(r);b.limit=90ULL*1024*1024;p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"unchanged90MiB renderer cap plus real sources/fallback refuses fine");
 b=Money(r);b.sourcePeak=~U64(0);p=MakePlan(r,b);C(p.status==Coarse&&!p.count,"source sum cannot wrap into cap");b=Money(r);b.windowBytes=1572864;p=MakePlan(r,b);CheckPlan(r,b,p);C(p.status==Detail,"explicit HDR presentation window ledger supported");
 for(unsigned bad=0;bad<12;++bad){Request q=r;Budget m=Money(r);switch(bad){case 0:q.world.x=std::numeric_limits<double>::quiet_NaN();break;case 1:q.world.w=std::numeric_limits<double>::infinity();break;case 2:q.world.h=-1;break;case 3:q.screenWidth=0;break;case 4:q.screenHeight=16385;break;case 5:q.stamp.source=0;break;case 6:m.limit=GpuCap+1;break;case 7:m.capacity=129;break;case 8:m.unavailable=129;break;case 9:m.windowBytes=0;break;case 10:q.world.x=1e308;q.world.w=1e308;break;case 11:q.world.w=0;break;}p=MakePlan(q,m);C(p.status==Invalid&&!p.count,"invalid input returns no jobs");}
 C(heapCalls==0,"production planning and tests allocate no C++ heap");printf("PASS %u production fine-window checks\n",checks);return 0;
}catch(...){return 1;}}
