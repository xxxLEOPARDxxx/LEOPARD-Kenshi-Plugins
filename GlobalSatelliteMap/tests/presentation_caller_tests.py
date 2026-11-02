"""Production-extracted ready-texture presentation controls. No game/UI calls."""
from pathlib import Path
import argparse, hashlib, json, os, subprocess, sys

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--case-source',type=Path)
parser.add_argument('--case-name',default='positive')
parser.add_argument('--skip-mutants',action='store_true')
args=parser.parse_args()
P=Path(__file__).resolve().parents[1]
source=(args.case_source or P/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig')
out=P/'build/presentation-caller-tests';folder=out/args.case_name;folder.mkdir(parents=True,exist_ok=True)
def function(signature):
 start=source.index(signature);opening=source.index('{',start);depth=1;end=opening+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
signatures=['bool Geometry(', 'void WindowBounds(', 'int FindWindow(', 'bool Superseded(', 'MyGUI::IntCoord WindowGeometry(']
if 'void PresentWindows(' in source:signatures+=['void PresentWindows(']
signatures+=['void RepositionZoom(']
functions='\n'.join(function(s) for s in signatures)
# Before the fix this fixture uses the actual old Draw presentation block, so
# the first failing run compiles and fails at the missing immediate coverage.
if 'void PresentWindows(' not in source:
 a=source.index('    for(unsigned i=0;i<AtlasView::WindowSlots;++i){\n        MyGUI::ImageBox*w=',source.index('void Draw('))
 b=source.index('    if(debug&&now-statsAt',a)
 functions+='\nvoid PresentWindows(MapScreen*map,const AtlasView::WindowRect cells[16],const AtlasView::WindowRect&clip,const AtlasView::WindowPlan&plan){\n'+source[a:b]+'}\n'
program=r'''
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <stdexcept>
#include "__POLICY__"
namespace GSM {struct Rect{int x,y,w,h;Rect(int a=0,int b=0,int c=0,int d=0):x(a),y(b),w(c),h(d){}};bool Finite(double x){return _finite(x)!=0;}}
namespace Ogre {struct Vector3{float x,y,z;Vector3(float a,float b,float c):x(a),y(b),z(c){}};}
unsigned projections=0,writes=0,checks=0;
namespace MyGUI {
struct IntPoint{int left,top;IntPoint(int a=0,int b=0):left(a),top(b){}};
struct IntSize{int width,height;IntSize(int a=0,int b=0):width(a),height(b){}bool operator!=(const IntSize&b)const{return width!=b.width||height!=b.height;}};
struct IntCoord{int left,top,width,height;IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}bool operator!=(const IntCoord&b)const{return left!=b.left||top!=b.top||width!=b.width||height!=b.height;}};
struct Widget{IntCoord c;Widget*parent;bool visible,alive;std::string texture;Widget():parent(NULL),visible(false),alive(true){}void Touch(){if(!alive)throw std::runtime_error("dead widget accessed");}int getWidth(){Touch();return c.width;}int getHeight(){Touch();return c.height;}IntSize getSize(){Touch();return IntSize(c.width,c.height);}IntCoord getCoord(){Touch();return c;}Widget*getParent(){Touch();return parent;}IntCoord getAbsoluteCoord(){Touch();return c;}IntPoint getAbsolutePosition(){Touch();return IntPoint(c.left,c.top);}void setCoord(IntCoord x){Touch();c=x;++writes;}void setSize(IntSize x){Touch();c.width=x.width;c.height=x.height;++writes;}bool getVisible(){Touch();return visible;}void setVisible(bool x){Touch();visible=x;++writes;}std::string _getTextureName(){Touch();return texture;}void setImageTexture(const std::string&x){Touch();texture=x;++writes;}void setDepth(int){Touch();++writes;}};
typedef Widget ImageBox;
}
struct MapScreen{MyGUI::Widget*mapImage,*mapScrollView;MyGUI::IntPoint worldToMapCoords(Ogre::Vector3 p){++projections;return MyGUI::IntPoint(int((double(p.x)+147456)/294912*mapImage->getWidth()+.5),int((double(p.z)+147456)/294912*mapImage->getHeight()+.5));}};
struct Ref{MyGUI::Widget*p;Ref():p(NULL){}MyGUI::Widget*get(){return p;}};
struct Entry{AtlasView::WindowKey key;unsigned shaded;std::string name;Entry():shaded(0){}};
struct Manifest{double minX,minZ,maxX,maxZ;Manifest():minX(-147456),minZ(-147456),maxX(147456),maxZ(147456){}}manifest;
Entry cache[AtlasView::WindowSlots];Ref layer,tiles[AtlasView::WindowSlots];int projectedWidth=0,projectedHeight=0;MyGUI::IntCoord projectedClip;bool presentationCurrent=false;
__FUNCTIONS__
void C(bool ok,const char*s){++checks;if(!ok){printf("FAIL %s check%u\n",s,checks);throw 1;}}
void Paint(MapScreen*map){GSM::Rect coords[16];C(Geometry(map,coords),"fixture native geometry");AtlasView::WindowRect cells[16];for(unsigned i=0;i<16;++i)cells[i]=AtlasView::WindowRect(coords[i].x,coords[i].y,coords[i].w,coords[i].h);MyGUI::IntCoord v=map->mapScrollView->getAbsoluteCoord();MyGUI::IntPoint o=map->mapImage->getAbsolutePosition();AtlasView::WindowRect clip(v.left-o.left,v.top-o.top,v.width,v.height);AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,clip);PresentWindows(map,cells,clip,plan);}
bool Covered(int x,int y){for(unsigned i=0;i<AtlasView::WindowSlots;++i){MyGUI::Widget*w=tiles[i].get();if(w&&w->visible&&w->texture==cache[i].name&&w->c.left<=x&&x<w->c.left+w->c.width&&w->c.top<=y&&y<w->c.top+w->c.height)return true;}return false;}
void Setup(MapScreen&m,MyGUI::Widget&image,MyGUI::Widget&scroll,MyGUI::Widget&overlay,MyGUI::Widget*images){image=MyGUI::Widget();scroll=MyGUI::Widget();overlay=MyGUI::Widget();image.c=MyGUI::IntCoord(-5000,-1000,8192,8192);scroll.c=MyGUI::IntCoord(0,0,2160,1044);m.mapImage=&image;m.mapScrollView=&scroll;overlay.parent=&image;layer.p=&overlay;presentationCurrent=false;projectedWidth=projectedHeight=0;projections=writes=0;for(unsigned i=0;i<AtlasView::WindowSlots;++i){images[i]=MyGUI::Widget();tiles[i].p=&images[i];cache[i]=Entry();}for(unsigned i=0;i<16;++i){cache[i].key=AtlasView::WindowKey(i,5,0,0);cache[i].shaded=1;char n[30];sprintf_s(n,"resident%u",i);cache[i].name=n;}}
int main(){try{MapScreen m;MyGUI::Widget image,scroll,overlay,images[AtlasView::WindowSlots];Setup(m,image,scroll,overlay,images);
 Paint(&m);C(Covered(5100,1500)&&!Covered(1000,1500),"old visible phase established with16 ready fallbacks");
 image.c=MyGUI::IntCoord(-1936,-239,4096,4096);RepositionZoom(&m);C(Covered(1950,600)&&Covered(2500,1200)&&Covered(2500,600),"zoom-out exposes ready left/bottom immediately");
 unsigned p=projections,w=writes;for(unsigned i=0;i<100;++i)RepositionZoom(&m);C(projections==p&&writes==w,"unchanged100 frames zero projection or UI writes");
 image.c.left=0;RepositionZoom(&m);C(Covered(10,600),"pan without resize exposes ready texture immediately");
 p=projections;scroll.c.width=2200;RepositionZoom(&m);C(projections>p,"viewport-only change refreshes presentation");
 w=writes;MyGUI::Widget foreign;overlay.parent=&foreign;image.c.left=-20;RepositionZoom(&m);C(writes==w&&!presentationCurrent,"foreign layer owner never receives projection writes");overlay.parent=&image;
 // Reset stamp simulates actual Clear/EnsureLayer lifecycle, checked below.
 for(unsigned i=0;i<AtlasView::WindowSlots;++i)images[i].alive=false;MyGUI::Widget fresh[AtlasView::WindowSlots];for(unsigned i=0;i<AtlasView::WindowSlots;++i)tiles[i].p=&fresh[i];MyGUI::Widget freshOverlay;freshOverlay.parent=&image;layer.p=&freshOverlay;presentationCurrent=false;RepositionZoom(&m);C(Covered(100,600),"new generation/layer with same dimensions refreshes fresh widgets");
 w=writes;tiles[0].p=NULL;presentationCurrent=false;RepositionZoom(&m);C(writes>=w,"destroyed tile WidgetRef skipped without dereference");
 layer.p=NULL;w=writes;RepositionZoom(&m);RepositionZoom(NULL);C(writes==w,"destroyed layer and null map harmless");
 Setup(m,image,scroll,overlay,images);image.c=MyGUI::IntCoord(0,0,2272,2272);cache[0].shaded=0;RepositionZoom(&m);C(!images[0].visible&&images[0].texture.empty()&&!Covered(10,300),"pending fallback remains pending without invented pixels or binding");cache[0].shaded=1;Paint(&m);C(images[0].visible&&images[0].texture==cache[0].name,"Draw refresh after shade binds newly ready source at unchanged geometry");
 cache[16].key=AtlasView::WindowKey(0,0,0,0);cache[16].shaded=1;cache[16].name="fine";cache[17].key=AtlasView::WindowKey(0,4,0,0);cache[17].shaded=1;cache[17].name="ancestor";Paint(&m);C(!images[16].visible&&images[17].visible&&images[0].visible,"current plan supersession and coarse fallback rules preserved");
 w=writes;p=projections;image.c.width=1000001;RepositionZoom(&m);C(writes==w&&projections==p,"oversized native geometry refused before projection/write");
 printf("PASS actual ready presentation %u checks\n",checks);return 0;}catch(...){return 1;}}
'''.replace('__POLICY__',str(P/'atlas-view/AtlasWindowPolicy.h')).replace('__FUNCTIONS__',functions)
(folder/'caller.cpp').write_text(program)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
run=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2',str(folder/'caller.cpp'),'/Fo'+str(folder/'caller.obj'),'/Fe'+str(folder/'caller.exe')],env=env,capture_output=True,text=True)
(folder/'build.log').write_text(run.stdout+run.stderr);assert run.returncode==0,run.stdout+run.stderr
run=subprocess.run([str(folder/'caller.exe')],capture_output=True,text=True);(folder/'run.log').write_text(run.stdout+run.stderr);print(run.stdout+run.stderr,end='');assert run.returncode==0
assert 'presentationCurrent=false;' in function('void Clear(')
assert 'presentationCurrent=false;' in function('void EnsureLayer(')
tick=function('void Tick(');assert tick.index('if(ready)RepositionZoom(map)')<tick.index('if(!tickClock.Due(')<tick.index('fs->busy()')
assert source[source.index('void Draw('):].count('PresentWindows(map,cells,clip,plan);')>=1
assert all(x not in function('void RepositionZoom(') for x in ('RequestWindow','Upload(','.Render(','.Launch(', 'zoomLease'))
assert all(x not in function('void PresentWindows(') for x in ('RequestWindow','Upload(','.Render(','.Launch(', 'zoomLease'))
draw=function('void Draw(');assert draw.index('unsigned coarseValid=')>draw.index('if(debug&&now-statsAt>=5000)')
(folder/'identity.json').write_text(json.dumps(dict(main=hashlib.sha256(source.encode()).hexdigest(),extracted=hashlib.sha256(functions.encode()).hexdigest()),indent=2)+'\n')
print('PASS presentation lifecycle reset / pre-cadence routing / no IO or render edges')
if not args.skip_mutants:
 cases=[
  ('no_immediate_refresh','PresentWindows(map,cells,windowClip,plan);',''),
  ('ignore_clip_change','&&!(clip!=projectedClip)',''),
  ('no_texture_binding','w->setImageTexture(e.name);',''),
  ('foreign_owner_write','||layer.get()->getParent()!=map->mapImage',''),
  ('show_pending_pixels','&&e.shaded!=0',''),
  ('scan_unchanged_frames','if(presentationCurrent&&width==projectedWidth&&height==projectedHeight&&!(clip!=projectedClip))return;',''),
 ]
 results=[]
 for name,old,new in cases:
  assert source.count(old)==1,(name,source.count(old))
  mutantFolder=out/('mutant-'+name);mutantFolder.mkdir(exist_ok=True)
  mutant=mutantFolder/'Main.cpp';mutant.write_text(source.replace(old,new),encoding='utf-8')
  result=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--skip-mutants','--case-source',str(mutant),'--case-name','mutant-'+name],capture_output=True,text=True)
  (mutantFolder/'expected-red.log').write_text(result.stdout+result.stderr)
  assert result.returncode!=0 and 'FAIL ' in result.stdout,(name,result.stdout+result.stderr)
  print('Expected RED',name);results.append(dict(name=name,result=result.stdout.strip()))
 (out/'mutants.json').write_text(json.dumps(results,indent=2)+'\n')
 print('PASS',len(cases),'compiled actual presentation mutants')
