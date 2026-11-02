"""Narrow actual RepositionZoom routing/fast-path extraction."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parents[1]/'build/window-integration-tests'
here.mkdir(parents=True,exist_ok=True);base=Path(__file__).resolve().parents[1]
s=(base/'GlobalSatelliteMap.cpp').read_text();a=s.index('void RepositionZoom(');b=s.index('void RequestWindow(',a);helper=s[a:b]
tick=s[s.index('void Tick('):s.index('void UpdateHook(',s.index('void Tick('))]
assert tick.index('if(ready)RepositionZoom(map)')<tick.index('if(!tickClock.Due(')
cpp=r'''
#include "D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
namespace MyGUI{
struct IntSize{int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct IntCoord{int left,top,width,height;IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}bool operator!=(const IntCoord&b)const{return left!=b.left||top!=b.top||width!=b.width||height!=b.height;}};
struct Widget{IntSize size;IntCoord coord;unsigned sizes,coords;Widget():sizes(0),coords(0){}int getWidth(){return size.width;}int getHeight(){return size.height;}IntSize getSize(){return size;}void setSize(IntSize s){size=s;++sizes;}IntCoord getCoord(){return coord;}void setCoord(IntCoord c){coord=c;++coords;}};
}
struct Ref{MyGUI::Widget*p;Ref():p(NULL){}MyGUI::Widget*get(){return p;}}layer,tiles[AtlasView::WindowSlots];
struct Entry{AtlasView::WindowKey key;}cache[AtlasView::WindowSlots];
struct MapScreen{MyGUI::Widget*mapImage;};
int projectedWidth=0,projectedHeight=0;unsigned projections=0;
MyGUI::IntCoord WindowGeometry(MapScreen*m,const AtlasView::WindowKey&k){++projections;return MyGUI::IntCoord(int(k.tile),0,m->mapImage->getWidth()/4,m->mapImage->getHeight()/4);}
HELPER
static unsigned checks=0;static void Check(bool x,const char*s){++checks;if(!x){std::printf("FAIL %s\n",s);throw 1;}}
int main(){try{
 MyGUI::Widget image,container,widgets[AtlasView::WindowSlots];image.size=MyGUI::IntSize(2048,1024);MapScreen map={&image};layer.p=&container;
 for(unsigned i=0;i<AtlasView::WindowSlots;++i){cache[i].key=AtlasView::WindowKey(i%16,i<16?5:0,0,0);tiles[i].p=&widgets[i];}
 RepositionZoom(&map);Check(projections==AtlasView::WindowSlots&&container.sizes==1,"dimension event visits at most72 resident windows");
 Check(projectedWidth==2048&&projectedHeight==1024,"event dimensions cached");
 Check(widgets[AtlasView::WindowSlots-1].coords==1&&widgets[AtlasView::WindowSlots-1].coord.width==512,"last resident coordinate updated");
 RepositionZoom(&map);Check(projections==AtlasView::WindowSlots&&container.sizes==1&&widgets[AtlasView::WindowSlots-1].coords==1,"unchanged per-frame path performs no projection/setter");
 image.size.width=1000001;RepositionZoom(&map);Check(projections==AtlasView::WindowSlots&&projectedWidth==2048,"oversized native image refused before projection");
 image.size.width=0;RepositionZoom(&map);Check(projections==AtlasView::WindowSlots,"zero native image refused");
 image.size.width=4096;tiles[3].p=NULL;cache[5].key=AtlasView::WindowKey();RepositionZoom(&map);
 Check(projections==AtlasView::WindowSlots*2-2&&container.sizes==2,"destroyed widget and invalid key skipped");
 RepositionZoom(NULL);Check(projections==AtlasView::WindowSlots*2-2,"null map harmless");
 map.mapImage=NULL;RepositionZoom(&map);Check(projections==AtlasView::WindowSlots*2-2,"null native image harmless");
 map.mapImage=&image;layer.p=NULL;RepositionZoom(&map);Check(projections==AtlasView::WindowSlots*2-2,"no owned layer harmless");
 std::printf("PASS %u actual RepositionZoom event/fast-path checks\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('HELPER',helper)
(here/'reposition.cpp').write_text(cpp)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
exe=here/'reposition.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',f'/Fo{here}/reposition.obj',f'/Fe{exe}',str(here/'reposition.cpp')],check=True,env=env)
subprocess.run([str(exe)],check=True)
print('PASS ready-only zoom event before streaming cadence gate')
