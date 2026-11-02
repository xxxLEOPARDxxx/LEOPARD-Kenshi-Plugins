"""Compile the production bounded seam-control selector, including refusal paths."""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
build = here / 'build'
build.mkdir(exist_ok=True)
fixture = r'''
#include "AtlasCapturePolicy.h"
#include <cstdio>
static bool Same(const AtlasCapture::Block&a,const AtlasCapture::Block&b){return a.tileX==b.tileX&&a.tileZ==b.tileZ&&a.x==b.x&&a.z==b.z&&a.regionX==b.regionX&&a.regionZ==b.regionZ&&a.centerX==b.centerX&&a.centerZ==b.centerZ&&a.span==b.span;}
int main() {
 AtlasCapture::Block b = {0}; unsigned checks=0;
 if(!AtlasCapture::OverlapControl(b))return 1;
 if(b.tileX!=1||b.tileZ!=1||b.x!=5120||b.z!=6144||b.regionX!=3||b.regionZ!=3||b.span!=18432||b.centerX!=-18432||b.centerZ!=-9216)return 2;
 AtlasCapture::Block original=b;
 for(unsigned controlCase=0;controlCase<2;++controlCase){
 if(!AtlasCapture::OverlapControl(b,controlCase))return 5;
 if(controlCase==0&&!Same(b,original))return 6;
 if(controlCase==1&&(b.tileX!=1||b.tileZ!=1||b.x!=6128||b.z!=6128||b.regionX!=3||b.regionZ!=3||b.span!=18432||b.centerX!=-9360||b.centerZ!=-9360))return 7;
 for(unsigned z=0;z<2048;++z)for(unsigned x=0;x<2048;++x){
  const double actualX=b.centerX-b.span*.5+(x+.5)*9;
  const double actualZ=b.centerZ-b.span*.5+(z+.5)*9;
  if(actualX!=-73728+(b.x+x+.5)*9||actualZ!=-73728+(b.z+z+.5)*9)return 3;
  if(((8192+b.x+x)&1)!=(x&1)||((8192+b.z+z)&1)!=(z&1))return 8;
  ++checks;
 }
 if(controlCase==1&&(b.x+16!=6144||b.z+16!=6144||b.x+2047!=8175||b.z+2047!=8175))return 9;
 ++checks;
 }
 const unsigned invalid[]={2,3,16,256,0xffffffffu};
 for(unsigned i=0;i<5;++i){AtlasCapture::Block before=b;if(AtlasCapture::OverlapControl(b,invalid[i])||!Same(b,before))return 10;++checks;}
 for(unsigned i=0;i<256;++i){AtlasCapture::Block normal;if(!AtlasCapture::Select(i,normal))return 4;++checks;}
 std::printf("PASS %u two-case overlap/global sample/checker alignment, invalid-case refusal and unchanged normal selectors\n",checks);
}
'''
path = build / 'overlap_control.cpp'
path.write_text(fixture)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{here}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for cmd in ([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/Fe'+str(build/'overlap_control.exe'),'/Fo'+str(build/'overlap_control.obj'),str(path)],[str(build/'overlap_control.exe')]):
 r=subprocess.run(cmd,env=env,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
 print(r.stdout+r.stderr,end='')
 if r.returncode: raise SystemExit(r.returncode)

# Compile the actual case-admission/latching methods at explicit fake I/O and
# native-scene boundaries. Runtime INI changes cannot redirect an active run.
run_source=(here/'AtlasRun.h').read_text()
terrain_source=(here/'TerrainExperiment.h').read_text()
caller_source=(here/'CaptureProbe.cpp').read_text()
def extract(source,marker):
 start=source.index(marker);begin=source.index('{',start);end=begin+1;depth=1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
begin=extract(run_source,'void Begin(')
block=extract(run_source,'bool Block(')
state=run_source[run_source.index(' bool active,'):run_source.index(' bool Block(')]
terrain=extract(terrain_source,'void BeginAtlas(')
request=extract(caller_source,'if(!atlasRun.active){')
terrain_call=next(s.strip() for s in caller_source.splitlines() if 'terrainExperiment.BeginAtlas(renderer->root,atlasRun.index' in s)
latching=r'''
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <stdexcept>
#include "AtlasCapturePolicy.h"
static unsigned checks=0,dirs=0,disks=0,caseReads=0,iniCase=0,Side=2048;
static bool overlapRequest=true;
static std::string requestIni="fixture";
static void* ou=(void*)123;
static void Check(bool b,const char* s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
static BOOL ReviewDisk(const char*,ULARGE_INTEGER* bytes,ULARGE_INTEGER*,ULARGE_INTEGER*){++disks;bytes->QuadPart=1ULL<<40;return TRUE;}
static BOOL ReviewDir(const char*,SECURITY_ATTRIBUTES*){++dirs;return TRUE;}
static UINT ReviewIni(const char*,const char* name,INT fallback,const char*){if(std::string(name)=="OverlapCase"){++caseReads;return iniCase;}if(std::string(name)=="Request")return 123;if(std::string(name)=="AtlasBlocks")return 1;return fallback;}
#define GetDiskFreeSpaceExA ReviewDisk
#define CreateDirectoryA ReviewDir
#define GetPrivateProfileIntA ReviewIni
static std::string Folder(){return "owned/";}
namespace Gst2{struct Writer{};struct Win32File{};static unsigned __int64 FileBytes(){return 1ULL<<30;}}
struct AtlasRun{
 struct Pixels{};
STATE
BLOCK
BEGIN
} atlasRun;
namespace Ogre{struct Root{};}
struct Renderer{Ogre::Root*root;};static Renderer rendererValue;static Renderer*renderer=&rendererValue;
static bool Same(const AtlasCapture::Block&a,const AtlasCapture::Block&b){return a.tileX==b.tileX&&a.tileZ==b.tileZ&&a.x==b.x&&a.z==b.z&&a.regionX==b.regionX&&a.regionZ==b.regionZ&&a.centerX==b.centerX&&a.centerZ==b.centerZ&&a.span==b.span;}
struct Terrain{
 void *root,*scene,*terrain,*camera,*workspace;bool active,atlas;AtlasCapture::Block atlasBlock;unsigned calls;
 Terrain():root(NULL),scene(NULL),terrain(NULL),camera(NULL),workspace(NULL),active(false),atlas(false),calls(0){}
 void Begin(Ogre::Root*,int,int,int,const std::string&,bool,bool,bool,const std::string&,bool,bool,bool){++calls;}
TERRAIN
}terrainExperiment;
static void Dispatch(){std::string base="input";REQUEST
TERRAIN_CALL
}
int main(){try{
 iniCase=1;Dispatch();AtlasCapture::Block latched;Check(atlasRun.Block(latched),"latched Block resolves");
 Check(atlasRun.active&&atlasRun.control&&atlasRun.controlCase==1&&atlasRun.limit==1&&atlasRun.world==ou,"case1 and world latch once at admission");
 Check(latched.x==6128&&latched.z==6128&&latched.centerX==-9360&&Same(latched,terrainExperiment.atlasBlock),"terrain and worker Block share case1");
 unsigned reads=caseReads,created=dirs;iniCase=0;overlapRequest=false;Dispatch();AtlasCapture::Block after;Check(atlasRun.Block(after)&&Same(after,latched)&&Same(terrainExperiment.atlasBlock,latched),"runtime INI edits cannot redirect active terrain or worker");Check(caseReads==reads&&dirs==created,"active run never rereads case or creates directory");
 const unsigned invalid[]={2,3,256,0xffffffffu};
 for(unsigned i=0;i<4;++i){atlasRun=AtlasRun();terrainExperiment=Terrain();iniCase=invalid[i];overlapRequest=true;unsigned oldDirs=dirs,oldDisks=disks;bool threw=false;
  try{Dispatch();}catch(...){threw=true;}Check(threw&&!atlasRun.active&&dirs==oldDirs&&disks==oldDisks&&!terrainExperiment.calls,"invalid requested case refuses before disk directory or terrain");}
 atlasRun=AtlasRun();terrainExperiment=Terrain();iniCase=0;overlapRequest=true;Dispatch();AtlasCapture::Block original;Check(atlasRun.Block(original)&&original.x==5120&&original.z==6144&&original.centerX==-18432&&original.centerZ==-9216&&atlasRun.controlCase==0,"explicit case0 reproduces original footprint");
 AtlasRun defaults;defaults.Begin("owned/",7,ou,1,true);AtlasCapture::Block defaultBlock;Check(defaults.Block(defaultBlock)&&Same(defaultBlock,original)&&defaults.controlCase==0,"omitted case defaults to original case0");
 atlasRun=AtlasRun();terrainExperiment=Terrain();iniCase=1;overlapRequest=false;reads=caseReads;Dispatch();Check(!atlasRun.control&&atlasRun.controlCase==0&&caseReads==reads,"normal atlas ignores overlap INI and stays case0");AtlasCapture::Block ordinary;Check(atlasRun.Block(ordinary)&&Same(ordinary,terrainExperiment.atlasBlock),"ordinary atlas terrain and worker stay coherent");
 for(unsigned mode=0;mode<2;++mode){Terrain t;t.atlasBlock=original;bool threw=false;try{t.BeginAtlas(NULL,0,"height",mode!=0,mode?2:1);}catch(...){threw=true;}Check(threw&&!t.calls&&!t.atlas&&Same(t.atlasBlock,original),"direct invalid terrain case refuses without output mutation/resources");}
 AtlasRun normal;unsigned oldDirs=dirs,oldDisks=disks;bool threw=false;try{normal.Begin("owned/",7,ou,1,false,1);}catch(...){threw=true;}Check(threw&&!normal.active&&dirs==oldDirs&&disks==oldDisks,"non-control case1 rejected before I/O");
 std::printf("PASS %u actual Begin/Block/BeginAtlas/request latching and pre-I/O refusal checks\n",checks);return 0;
 }catch(...){std::printf("FAIL unexpected latching exception\n");return 1;}}
'''
latching=latching.replace('STATE',state).replace('BLOCK',block).replace('BEGIN',begin).replace('TERRAIN_CALL',terrain_call).replace('TERRAIN',terrain).replace('REQUEST',request)
def run_latching(name,source,success=True):
 folder=build/'overlap-cases'/name;folder.mkdir(parents=True,exist_ok=True);unit=folder/'test.cpp';unit.write_text(source)
 result=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(unit),'/Fo'+str(folder)+'/', '/Fe'+str(folder/'test.exe')],env=env,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (folder/'build.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
 result=subprocess.run([str(folder/'test.exe')],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (folder/'run.log').write_text(result.stdout+result.stderr);assert (result.returncode==0)==success,(name,result.stdout,result.stderr)
 if not success:assert 'FAIL ' in result.stdout,(name,result.stdout,result.stderr)
 print(name,result.stdout.strip())
run_latching('actual',latching)
for name,old,new in [
 ('forget-latch','controlCase=requestedCase','controlCase=0'),
 ('wrong-worker-case','OverlapControl(out,controlCase)','OverlapControl(out,0)'),
 ('wrong-terrain-case','atlasRun.control,atlasRun.controlCase','atlasRun.control,iniCase'),
 ('ignore-case-admission','requestedCase>1','false'),
 ('reread-case','if(!atlasRun.active){','if(true){'),
 ('allow-normal-case','(!overlapControl&&requestedCase!=0)','false'),
]:
 assert old in latching,name
 run_latching(name,latching.replace(old,new),False)
print('6 compiled case-latching/refusal mutants rejected')
