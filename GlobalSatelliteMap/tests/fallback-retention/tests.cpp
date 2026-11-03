#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../../atlas-view/AtlasWindowPolicy.h"
#include "../../GsmFinePolicy.h"
#include "../../atlas-terrain/fine-viewer/FineJob.h"
namespace MyGUI {struct IntCoord{int left,top,width,height;IntCoord(int x,int y,int w,int h):left(x),top(y),width(w),height(h){}};struct IntPoint{int left,top;IntPoint(int x,int y):left(x),top(y){}};}
namespace GSM {struct Rect{int x,y,w,h;};}
struct View{MyGUI::IntCoord getAbsoluteCoord(){return MyGUI::IntCoord(0,0,2048,1100);}};
struct Image{MyGUI::IntPoint getAbsolutePosition(){return MyGUI::IntPoint(0,0);}};
struct MapScreen{View*mapScrollView;Image*mapImage;};
struct Entry{AtlasView::WindowKey key;bool shaded;};Entry cache[AtlasView::WindowSlots];unsigned dropped=0,fallbackDrops=0;bool qualified=true,producing=false;
FineJob::Demand fineDemand={};void*fineOwner=reinterpret_cast<void*>(1);
void SetDensity(double d){fineDemand.view.world.w=d*fineDemand.view.screenWidth;fineDemand.view.world.h=d*fineDemand.view.screenHeight;}
bool Geometry(MapScreen*,GSM::Rect*out){for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){GSM::Rect&r=out[z*4+x];r.x=x*9216;r.y=z*9216;r.w=r.h=9216;}return true;}
bool FineQualified(){return qualified;}
AtlasView::U64 ProducerGpuBytes(){return producing?1:0;}
AtlasView::U64 FineGpuBytes(){return 0;}
void DropEntry(unsigned n){if(n<AtlasView::FallbackSlots)++fallbackDrops;++dropped;cache[n].key=AtlasView::WindowKey();cache[n].shaded=false;}
#include "_generated/Residency.inc"
unsigned checks=0;void C(bool ok,const char*why){++checks;if(!ok){printf("FAIL %s\n",why);throw 1;}}
void Warm(){dropped=fallbackDrops=0;for(unsigned n=0;n<AtlasView::FallbackSlots;++n){cache[n].key=AtlasView::WindowKey(n,5,0,0);cache[n].shaded=true;}for(unsigned n=AtlasView::FallbackSlots;n<AtlasView::WindowSlots;++n){cache[n].key=AtlasView::WindowKey((n-16)/8,1,(n-16)%8,0);cache[n].shaded=true;}}
int main(){try{View v;Image i;MapScreen map={&v,&i};fineDemand.scope.thread=GetCurrentThreadId();fineDemand.scope.world=1;fineDemand.scope.job=2;fineDemand.scope.sourceEpoch=3;fineDemand.view.stamp.source=3;fineDemand.view.stamp.material=4;fineDemand.revision=1;fineDemand.view.screenWidth=2048;fineDemand.view.screenHeight=1100;SetDensity(3);Warm();qualified=true;Maintain(&map);
 C(fallbackDrops==0,"fine residency never evicts fixed overview fallback");for(unsigned n=0;n<16;++n)C(cache[n].key==AtlasView::WindowKey(n,5,0,0)&&cache[n].shaded,"all sixteen ready fallback cells remain resident");C(dropped==128,"fine admission retires detail only and keeps all overview");
 for(unsigned zoom=0;zoom<8;++zoom)for(unsigned pan=0;pan<16;++pan){double side=512.0*(1u<<zoom);AtlasView::WindowRect cells[16];for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x)cells[z*4+x]=AtlasView::WindowRect(x*side,z*side,side,side);AtlasView::WindowRect clip((pan%4)*side,(pan/4)*side,2048,1100);AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,clip,4);for(unsigned t=0;t<16;++t)if(plan.visible[t])C(cache[t].key==AtlasView::WindowKey(t,5,0,0)&&cache[t].shaded,"fast pan/zoom visible primary has immediate ready overview");}
 qualified=true;SetDensity(9);Warm();Maintain(&map);C(!dropped&&BaseDetailCapacity()==128,"coarse zoom restores ordinary intermediate LOD");SetDensity(3);Warm();Maintain(&map);C(dropped==128&&BaseDetailCapacity()==0,"fine zoom reclaims detail budget again");SetDensity(9);producing=true;Warm();Maintain(&map);C(dropped==128&&BaseDetailCapacity()==0,"active producer keeps allowance through coarse-zoom drain");producing=false;SetDensity(73728.0/9207);Warm();Maintain(&map);C(!dropped&&BaseDetailCapacity()==128,"actual8.00782 density with288 required fine slots restores ordinary intermediate LOD");SetDensity(3);fineOwner=NULL;Warm();Maintain(&map);C(dropped==126&&BaseDetailCapacity()==2,"feasible startup keeps bounded ordinary details until real producer ownership");C(28637998ULL+BaseDetailCapacity()*FineJob::WindowBytes<=FineJob::ViewerAllowance,"actual baseline plus startup details fits32MiB");fineOwner=reinterpret_cast<void*>(1);fineDemand.scope.world=0;Warm();Maintain(&map);C(!dropped&&BaseDetailCapacity()==128,"invalid scope cannot reserve fine budget");fineDemand.scope.world=1;qualified=false;Warm();Maintain(&map);C(!dropped,"blocked fine restores normal detail capacity without deleting overview");printf("PASS %u actual residency-tail checks\n",checks);return 0;}catch(...){return 1;}}
