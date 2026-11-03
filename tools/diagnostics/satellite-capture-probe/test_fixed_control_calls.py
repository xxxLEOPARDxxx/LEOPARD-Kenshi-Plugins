"""Compile actual private request/guard/callback bodies with owned fake engine values."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'build';source=(HERE/'CaptureProbe.cpp').read_text()
def block(start):
 at=source.index('{',start);end=at+1;depth=1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 return source[start:end]
context=block(source.index('bool FixedContext('))
request=block(source.index('if(fixedControlEnabled)',source.index('void Update(')))
callback=block(source.index('if(fixedControl.Active()&&!fixedControl.Stopping()'))
startup=source[source.index('fixedControlEnabled=GetPrivateProfileIntA'):source.index('RasterPolicy::Layout raster;',source.index('fixedControlEnabled=GetPrivateProfileIntA'))]
assert '"FixedTerrainControl",0' in startup and '"FixedTerrainGsmStartupDisabled",0' in startup
assert 'fixedRequestLatch.Changed(GetPrivateProfileIntA("Probe","FixedTerrainRequest",0' in startup
assert source.count('KenshiLib::AddHook(')==1
assert 'Side' not in request and 'atlasRun.Begin' not in request and 'TerrainExperiment' not in request
prefix=r'''
#include <cstdio>
#include <string>
#include <stdexcept>
#include "PROBE_POLICY"
typedef unsigned long DWORD;struct ID3D11Device{};static ID3D11Device device;
namespace AtlasView{template<class T>struct Com{T*p;Com():p(0){}};}
namespace Ogre{struct CompositorWorkspace{void*listener;CompositorWorkspace():listener(0){}void*getListener(){return listener;}void setListener(void*p){listener=p;}};}
struct Renderer{int*root,*scene;Ogre::CompositorWorkspace*workspace;};struct AU{Renderer*render;};struct MapScreen{bool visible;bool getVisible(){return visible;}};
static unsigned filesystemCalls=0,censusCalls=0;struct SaveFileSystem{bool saving;bool busy(){return saving;}static SaveFileSystem*getSingleton();};static SaveFileSystem fs;SaveFileSystem*SaveFileSystem::getSingleton(){++filesystemCalls;return &fs;}
static bool fixedControlEnabled=true,fixedGsmStartupOff=true,currentOff=true,noTextures=true,profileOK=true,mainOk=true,loadedOk=true,abiOk=true,restorationSafe=true,chainSafe=true;
static bool inside=false,pending=false,cleanup=false;static unsigned fixedEpoch=0;static DWORD tick=300,fixedPollAt=0,token=10,cancelKey=0;static std::string requestIni="fake.ini";
static int world;static int*ou=&world,*requestedWorld=&world;static Ogre::CompositorWorkspace workspace;static Renderer value={&world,&world,&workspace};static Renderer*renderer=&value;static AU auValue={&value};static AU*au=&auValue;static Ogre::CompositorWorkspace*gameWorkspace=&workspace;static void*previous=0,*ourListener=0;static int listener=0;
static struct{void*root;}terrainExperiment={0};static struct{bool active;void*thread,*pixels;}atlasRun={false,0,0};static ProbePolicy::RequestLatch fixedRequestLatch;
static unsigned checks=0,errors=0,drops=0,logs=0;static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
static bool OnMain(){return mainOk;}static bool LoadedGuard(){return loadedOk;}static bool RenderAbiMatches(){return abiOk;}static bool FixedViewerOff(){return currentOff;}static bool FixedNoViewerTextures(){++censusCalls;return noTextures;}static DWORD GetTickCount(){return tick;}static unsigned GetPrivateProfileIntA(const char*,const char*key,unsigned,const char*){return std::string(key)=="FixedTerrainCancel"?cancelKey:token;}
static void Log(const std::string&){++logs;}static void ErrorLog(const std::string&){++errors;}static void Drop(){++drops;cleanup=false;}static void EnsureRenderAbi(){if(!abiOk)throw std::runtime_error("ABI");}static std::string Folder(){return "owned/";}static void FixedDevice(Ogre::CompositorWorkspace*,ID3D11Device**out){*out=&device;}
static struct FakeControl{bool active,stopping,same,quarantined;unsigned begins,cancels,steps;FakeControl():active(false),stopping(false),same(true),quarantined(false),begins(0),cancels(0),steps(0){}bool Active(){return active;}bool Stopping(){return stopping;}bool Quarantined(){return quarantined;}bool SameDevice(ID3D11Device*){return same;}void Cancel(){if(active){++cancels;stopping=true;}}bool Poll(DWORD&,std::string&){return false;}void Begin(ID3D11Device*,const std::string&,unsigned,unsigned,bool){C(workspace.listener!=&listener,"listener installed only after admission succeeds");++begins;active=true;stopping=false;}void Step(bool b){C(b,"Step fully admitted");++steps;}}fixedControl;
namespace FixedTerrainControl{static bool Guard(bool a,bool b,bool c,bool d,bool e,bool f,bool g,bool h){return a&&b&&c&&d&&e&&f&&g&&h;}}
'''.replace('PROBE_POLICY',(HERE/'ProbePolicy.h').as_posix())
main=r'''
static void Request(MapScreen*map){tick+=300;REQUEST_BODY}
static void Callback(Ogre::CompositorWorkspace*w){CALLBACK_BODY}
int main(){try{MapScreen map={true};fixedRequestLatch.Changed(token);Request(&map);C(!fixedControl.begins,"startup existing token never replays");token=11;Request(&map);C(fixedControl.begins==1&&workspace.listener==&listener,"one new token installs existing listener bridge");Callback(&workspace);C(fixedControl.steps==1&&!inside,"actual callback draws once and clears recursion flag");
 token=12;Request(&map);C(fixedControl.begins==1,"busy new token consumed without restart");fixedControl.active=false;workspace.listener=0;Request(&map);C(fixedControl.begins==1,"busy token not replayed when idle");token=13;Request(&map);C(fixedControl.begins==2,"next explicit token accepted");fixedControl.active=false;workspace.listener=0;
 cancelKey=1;token=14;Request(&map);C(fixedControl.begins==2,"cancel flag blocks new I/O");cancelKey=0;Request(&map);C(fixedControl.begins==2,"cancelled token never replays");
 for(unsigned bad=0;bad<5;++bad){token++;currentOff=bad!=0;noTextures=bad!=1;fs.saving=bad==2;ou=bad==3?0:&world;profileOK=bad!=4;bool caught=false;try{Request(&map);}catch(const std::runtime_error&){caught=true;}C(caught&&fixedControl.begins==2&&workspace.listener==0,"viewer/save/world/profile failure before worker or listener mutation");currentOff=noTextures=profileOK=true;fs.saving=false;ou=&world;}
 map.visible=false;token++;Request(&map);C(fixedControl.begins==2,"hidden map consumes token without I/O");map.visible=true;Request(&map);C(fixedControl.begins==2,"hidden-map token cannot replay");token++;Request(&map);C(fixedControl.begins==3,"fresh eligible token succeeds");
 fixedControl.same=false;unsigned steps=fixedControl.steps;Callback(&workspace);C(fixedControl.stopping&&fixedControl.steps==steps&&!inside,"changed actual device cancels before GPU step");fixedControl.same=true;fixedControl.stopping=false;requestedWorld=0;Callback(&workspace);C(fixedControl.stopping&&fixedControl.steps==steps,"changed world cancels before GPU step");requestedWorld=&world;fixedControl.stopping=false;inside=true;Callback(&workspace);C(fixedControl.steps==steps,"nested callback never executes pass");inside=false;
 for(unsigned mask=0;mask<8;++mask){fixedGsmStartupOff=(mask&1)!=0;currentOff=(mask&2)!=0;noTextures=(mask&4)!=0;C(FixedContext(&workspace,true)==(mask==7),"actual native guard includes startup/current/texture exclusion");}fixedGsmStartupOff=currentOff=noTextures=true;
 unsigned fsBefore=filesystemCalls,censusBefore=censusCalls;requestedWorld=0;C(!FixedContext(&workspace,true),"world mismatch refuses actual context");C(filesystemCalls==fsBefore&&censusCalls==censusBefore,"world mismatch before filesystem and resource census");requestedWorld=&world;
 fixedControl.quarantined=true;cleanup=true;Request(&map);C(drops==1&&!cleanup,"quarantine detaches listener exactly once");token++;Request(&map);C(drops==1&&fixedControl.begins==3,"quarantine permanently refuses more jobs without repeated cleanup");
 std::printf("PASS %u actual request/context/callback checks\n",checks);return 0;}catch(...){return 1;}}
'''.replace('REQUEST_BODY',request).replace('CALLBACK_BODY',callback)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,ctx=context,req=request,call=callback,green=True):
 body=prefix+ctx+main.replace(request,req).replace(callback,call);cpp=OUT/(name+'.cpp');cpp.write_text(body);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,('compile failure not kill',name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (p.returncode==0 if green else p.returncode==1 and 'FAIL ' in p.stdout),(name,p.returncode,p.stdout,p.stderr);print(('PASS 'if green else 'Expected RED ')+name+': '+p.stdout.strip())
run('fixed-call-actual')
run('fixed-call-startup',ctx=context.replace('!fixedGsmStartupOff','false'),green=False)
run('fixed-call-cancel',req=request.replace('if(cancelled||fixedControl.Active()','if(false||fixedControl.Active()'),green=False)
run('fixed-call-device',call=callback.replace('!fixedControl.SameDevice(d.p)','false'),green=False)
run('fixed-call-world',ctx=context.replace('ou!=requestedWorld','false'),green=False)
run('fixed-call-early-census',ctx=context.replace('{\n    if','{\n    SaveFileSystem::getSingleton();\n    if',1),green=False)
print('PASS five compiled caller refusal mutants rejected')
