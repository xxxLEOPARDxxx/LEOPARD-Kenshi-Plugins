"""Compile actual production window geometry and ancestor selection."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parents[1]/'build/window-integration-tests'
here.mkdir(parents=True,exist_ok=True)
base=Path(__file__).resolve().parents[1]
source=(base/'GlobalSatelliteMap.cpp').read_text()
def extract(marker):
 a=source.index(marker); b=source.index('{',a);depth=0
 for p in range(b,len(source)):
  if source[p]=='{':depth+=1
  if source[p]=='}':
   depth-=1
   if not depth:return source[a:p+1]
 raise ValueError(marker)
parts='\n'.join(extract(k) for k in ['int FindWindow(','bool Superseded(','void WindowBounds(','MyGUI::IntCoord WindowGeometry('])
cpp=r'''
#include "D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
namespace Ogre {struct Vector3 {float x,y,z;Vector3(float a,float b,float c):x(a),y(b),z(c){}};}
namespace MyGUI {struct IntPoint {int left,top;IntPoint(int a,int b):left(a),top(b){}};struct IntCoord {int left,top,width,height;IntCoord(int a,int b,int c,int d):left(a),top(b),width(c),height(d){}};}
struct MapScreen {double a,b,sx,sz;MyGUI::IntPoint worldToMapCoords(const Ogre::Vector3&v){return MyGUI::IntPoint(int(std::floor(a+v.x*sx)),int(std::floor(b+v.z*sz)));}};
struct Manifest {double minX,minZ,maxX,maxZ;Manifest():minX(-147456),minZ(-147456),maxX(147456),maxZ(147456){}}manifest;
struct Entry {AtlasView::WindowKey key;unsigned shaded;Entry():shaded(0){}}cache[AtlasView::WindowSlots];
PARTS
static unsigned checks=0;static void Check(bool value,const char*msg){++checks;if(!value){std::printf("FAIL %s\n",msg);throw 1;}}
int main(){try {
 MapScreen map={13.7,22.3,.0234567,.0345678};
 for(unsigned tile=0;tile<16;++tile)for(unsigned level=0;level<6;++level){
  unsigned n=AtlasView::Side(level)/256;double span=2304.0*double(1U<<level);
  for(unsigned z=0;z<n;++z)for(unsigned x=0;x<n;++x){
   AtlasView::WindowKey key(tile,level,x,z);double wx,wz,sx,sz;WindowBounds(key,wx,wz,sx,sz);
   Check(sx==span&&sz==span,"window span retains selected native texel density");
   Check(wx==-147456.0+(tile%4)*73728.0+x*span&&wz==-147456.0+(tile/4)*73728.0+z*span,"absolute source window origin");
   double step=9.0*(1U<<level);
   Check(wx+.5*step==-147456.0+(tile%4)*73728.0+(x*256+.5)*step,"first source-pixel X center");
   Check(wz+255.5*step==-147456.0+(tile/4)*73728.0+(z*256+255.5)*step,"last source-pixel Z center");
   MyGUI::IntCoord rect=WindowGeometry(&map,key);MyGUI::IntPoint a=map.worldToMapCoords(Ogre::Vector3(float(wx),0,float(wz))),b=map.worldToMapCoords(Ogre::Vector3(float(wx+span),0,float(wz+span)));
   Check(rect.left==a.left&&rect.top==a.top&&rect.width==b.left-a.left&&rect.height==b.top-a.top,"native-corner projection used directly");
   if(x+1<n){MyGUI::IntCoord next=WindowGeometry(&map,AtlasView::WindowKey(tile,level,x+1,z));Check(rect.left+rect.width==next.left,"intra-cell shared projected X edge");}
   else if(tile%4<3){MyGUI::IntCoord next=WindowGeometry(&map,AtlasView::WindowKey(tile+1,level,0,z));Check(rect.left+rect.width==next.left,"cross-primary projected X edge");}
   if(z+1<n){MyGUI::IntCoord next=WindowGeometry(&map,AtlasView::WindowKey(tile,level,x,z+1));Check(rect.top+rect.height==next.top,"intra-cell shared projected Z edge");}
   else if(tile/4<3){MyGUI::IntCoord next=WindowGeometry(&map,AtlasView::WindowKey(tile+4,level,x,0));Check(rect.top+rect.height==next.top,"cross-primary projected Z edge");}
   for(unsigned desired=level+1;desired<6;++desired){
    unsigned shift=desired-level;
    AtlasView::WindowKey ancestor(tile,desired,x>>shift,z>>shift);
    cache[17].key=AtlasView::WindowKey();cache[17].shaded=0;
    Check(!Superseded(key,desired),"keep old finer until actual ancestor loaded");
    cache[17].key=ancestor;
    Check(!Superseded(key,desired),"unshaded ancestor must not hide rendered fine");
    cache[17].shaded=7;
    Check(Superseded(key,desired),"ready aligned coarser ancestor supersedes fine");
    Check(!Superseded(key,level),"current selected level is not superseded");
    cache[17].key.tile=(tile+1)%16;
    Check(!Superseded(key,desired),"other primary ancestor does not supersede");
    cache[17].key=AtlasView::WindowKey();cache[17].shaded=0;
   }
  }
 }
 Check(AtlasView::WindowGpuBytes()==94634160ULL,"72window payload ledger");
 std::printf("PASS %u extracted production bounds/geometry/supersession checks\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('PARTS',parts)
(here/'fixture.cpp').write_text(cpp)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
exe=here/'fixture.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',f'/Fo{here}/fixture.obj',f'/Fe{exe}',str(here/'fixture.cpp')],check=True,env=env)
subprocess.run([str(exe)],check=True)
assert source.count('Superseded(e.key,plan.level)')==2
print('PASS supersession used for both rendering and widget visibility')
