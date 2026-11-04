#define _CRT_SECURE_NO_WARNINGS
// MODE
#ifdef LEGACY
#include "../../../atlas-terrain/fine-viewer/FineJob.h"
namespace GsmFineCoverage {template<class Ready>bool Covered(const FineJob::Plan&,const FineJob::Demand&,bool,bool,bool,unsigned,const Ready&){return false;}}
#else
#include "../../../GsmFineCoverage.h"
#endif
#include <cstdio>
static unsigned checks=0;
static void C(bool value,const char*name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);throw 1;}std::printf("PASS %s\n",name);}
struct Entries {
 bool valid[4],shaded[4],opaque[4];mutable unsigned visits;
 Entries():visits(0){for(unsigned n=0;n<4;++n)valid[n]=shaded[n]=opaque[n]=true;}
 bool operator()(const FineWindows::Key&key)const{++visits;unsigned n=key.z*2+key.x;return n<4&&valid[n]&&shaded[n]&&opaque[n];}
};
static FineJob::Plan Plan(){FineJob::Plan p;p.state=FineJob::Detail;p.spacing=1;p.count=4;p.pairCount=2;p.demand.revision=2;p.demand.scope.thread=GetCurrentThreadId();p.demand.scope.world=0x100000001ULL;p.demand.scope.sourceEpoch=0x200000002ULL;p.demand.scope.job=0x300000003ULL;p.demand.view.stamp.source=p.demand.scope.sourceEpoch;p.demand.view.stamp.material=7;p.demand.view.world.x=p.demand.view.world.z=-147456;p.demand.view.world.w=p.demand.view.world.h=512;p.demand.view.screenWidth=p.demand.view.screenHeight=512;for(unsigned n=0;n<4;++n){FineWindows::Key k={1,n%2,n/2,p.demand.view.stamp};p.keys[n]=k;}return p;}
int main(){try{FineJob::Plan p=Plan();Entries entries;C(GsmFineCoverage::Covered(p,p.demand,true,true,false,4,entries),"complete opaque fine viewport suppresses redundant coarse rendering");C(entries.visits==4,"complete viewport visits each ready entry exactly once");
 for(unsigned n=0;n<4;++n){Entries e;e.valid[n]=false;C(!GsmFineCoverage::Covered(p,p.demand,true,true,false,4,e),"one missing GPU entry resumes coarse fallback");e=Entries();e.shaded[n]=false;C(!GsmFineCoverage::Covered(p,p.demand,true,true,false,4,e),"one unshaded entry resumes coarse fallback");e=Entries();e.opaque[n]=false;C(!GsmFineCoverage::Covered(p,p.demand,true,true,false,4,e),"one translucent entry resumes coarse fallback");}
 C(!GsmFineCoverage::Covered(p,p.demand,false,true,false,4,entries),"invalid context resumes coarse fallback");C(!GsmFineCoverage::Covered(p,p.demand,true,false,false,4,entries),"changed map/widget resumes coarse fallback");C(!GsmFineCoverage::Covered(p,p.demand,true,true,true,4,entries),"center-first work never hides coarse fallback");C(!GsmFineCoverage::Covered(p,p.demand,true,true,false,2,entries),"active drain never hides coarse fallback");
 FineJob::Demand now=p.demand;now.view.world.x+=1;C(!GsmFineCoverage::Covered(p,now,true,true,false,4,entries),"pan with same revision resumes coarse fallback");now=p.demand;now.view.screenWidth++;C(!GsmFineCoverage::Covered(p,now,true,true,false,4,entries),"viewport resize resumes coarse fallback");now=p.demand;now.scope.world^=1ULL<<32;C(!GsmFineCoverage::Covered(p,now,true,true,false,4,entries),"full64-bit world change resumes coarse fallback");now=p.demand;now.scope.sourceEpoch^=1ULL<<32;C(!GsmFineCoverage::Covered(p,now,true,true,false,4,entries),"source generation change resumes coarse fallback");
 FineJob::Plan bad=p;bad.keys[2].stamp.material^=1ULL<<32;C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"stale material entry cannot prove opaque coverage");bad=p;bad.keys[3]=bad.keys[0];C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"duplicate keys cannot conceal a missing viewport tile");bad=p;bad.count=2;bad.pairCount=1;C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"center-only plan cannot masquerade as complete visible plan");bad=p;bad.keys[0].x=3;C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"offscreen key cannot conceal viewport hole");bad=p;bad.count=FineJob::Slots+1;C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"overflow count refuses before entry array access");
 bad=p;bad.spacing=3;bad.demand.view.world.w=bad.demand.view.world.h=1536;for(unsigned n=0;n<4;++n)bad.keys[n].spacing=3;C(GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"actual three-metre detail uses exact full viewport coverage");
 bad=p;bad.demand.view.world.x-=100;bad.demand.view.world.w+=100;C(GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"world-edge clipping requires only visible world coverage");
 bad=p;bad.demand.view.world.x+=.5;bad.demand.view.world.w-=.5;C(GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"fractional clipped viewport preserves exact anchored coverage");
 bad=p;bad.state=FineJob::Coarse;C(!GsmFineCoverage::Covered(bad,bad.demand,true,true,false,4,entries),"coarse plan never proves fine coverage");now=p.demand;now.scope.job^=1ULL<<32;C(!GsmFineCoverage::Covered(p,now,true,true,false,4,entries),"full64-bit capture generation change resumes coarse fallback");
 std::printf("PASS %u coverage controls; no game/native/GPU calls\n",checks);return 0;}catch(...){return 1;}}
