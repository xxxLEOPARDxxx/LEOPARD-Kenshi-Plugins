#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../../atlas-view/AtlasWindowPolicy.h"
namespace MyGUI {struct IntCoord{int left,top,width,height;IntCoord(int x,int y,int w,int h):left(x),top(y),width(w),height(h){}};struct IntPoint{int left,top;IntPoint(int x,int y):left(x),top(y){}};}
namespace GSM {struct Rect{int x,y,w,h;};}
struct View{MyGUI::IntCoord getAbsoluteCoord(){return MyGUI::IntCoord(0,0,2048,1100);}};
struct Image{MyGUI::IntPoint getAbsolutePosition(){return MyGUI::IntPoint(0,0);}};
struct MapScreen{View*mapScrollView;Image*mapImage;};
struct Entry{AtlasView::WindowKey key;bool shaded;};Entry cache[AtlasView::WindowSlots];unsigned dropped=0,fallbackDrops=0;bool qualified=true;
struct Demand{struct Window{struct World{double w;}world;double screenWidth;}view;}fineDemand;
bool Geometry(MapScreen*,GSM::Rect*out){for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){GSM::Rect&r=out[z*4+x];r.x=x*9216;r.y=z*9216;r.w=r.h=9216;}return true;}
bool FineQualified(){return qualified;}
void DropEntry(unsigned n){if(n<AtlasView::FallbackSlots)++fallbackDrops;++dropped;cache[n].key=AtlasView::WindowKey();cache[n].shaded=false;}
#include "_generated/Residency.inc"
unsigned checks=0;void C(bool ok,const char*why){++checks;if(!ok){printf("FAIL %s\n",why);throw 1;}}
void Warm(){dropped=fallbackDrops=0;for(unsigned n=0;n<AtlasView::FallbackSlots;++n){cache[n].key=AtlasView::WindowKey(n,5,0,0);cache[n].shaded=true;}for(unsigned n=AtlasView::FallbackSlots;n<AtlasView::WindowSlots;++n){cache[n].key=AtlasView::WindowKey((n-16)/8,1,(n-16)%8,0);cache[n].shaded=true;}}
int main(){try{View v;Image i;MapScreen map={&v,&i};fineDemand.view.world.w=16384;fineDemand.view.screenWidth=2048;Warm();qualified=true;Maintain(&map);
 C(fallbackDrops==0,"fine residency never evicts fixed overview fallback");for(unsigned n=0;n<16;++n)C(cache[n].key==AtlasView::WindowKey(n,5,0,0)&&cache[n].shaded,"all sixteen ready fallback cells remain resident");C(dropped==128,"fine admission retires detail only and keeps all overview");
 for(unsigned zoom=0;zoom<8;++zoom)for(unsigned pan=0;pan<16;++pan){double side=512.0*(1u<<zoom);AtlasView::WindowRect cells[16];for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x)cells[z*4+x]=AtlasView::WindowRect(x*side,z*side,side,side);AtlasView::WindowRect clip((pan%4)*side,(pan/4)*side,2048,1100);AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,clip,4);for(unsigned t=0;t<16;++t)if(plan.visible[t])C(cache[t].key==AtlasView::WindowKey(t,5,0,0)&&cache[t].shaded,"fast pan/zoom visible primary has immediate ready overview");}
 qualified=false;Warm();Maintain(&map);C(!dropped,"blocked fine restores normal detail capacity without deleting overview");printf("PASS %u actual residency-tail checks\n",checks);return 0;}catch(...){return 1;}}
