"""Run the exact owning State at a synchronous native Snapshot boundary."""
from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
OUT=Path('D:/DEV/Kenshi/research/gsm-loaded-reuse-20261104');OUT.mkdir(exist_ok=True)
source=(HERE.parent.parent/'GsmLoadedTownState.h').read_text()
start=source.index('class State {');end=source.index('{',start)+1;depth=1
while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
state=source[start:end]+';'
prefix=r'''
#define NOMINMAX
#include <Windows.h>
#include <float.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <cstdio>
#include <stdexcept>
namespace AtlasView{typedef unsigned __int64 U64;}
struct ID3D11Device{};struct Town{};namespace Ogre{struct SceneManager{};}
namespace AtlasModels{inline bool Finite(const float*p,unsigned n){if(!p)return false;for(unsigned i=0;i<n;++i)if(!_finite(p[i]))return false;return true;}}
namespace AtlasModelRegion{struct Bounds{float lo[3],hi[3];Bounds(){std::memset(this,0,sizeof(*this));}};inline bool WorldBounds(const float*,const float*,const float*,Bounds&b){for(unsigned i=0;i<3;++i){b.lo[i]=1;b.hi[i]=2;}return true;}}
namespace GsmTownDiagnostic{const unsigned Limit=4096;}
namespace GsmTownValues{struct TownValue{unsigned id;float storedXYZ[3];};struct Result{LONG generation;unsigned count;TownValue towns[4];Result():generation(7),count(1){std::memset(towns,0,sizeof(towns));towns[0].id=2;}};inline bool Generation(volatile LONG*p,LONG g){return p&&InterlockedCompareExchange(p,0,0)==g;}}
static Town town;namespace GsmModels{inline Town*Resolve(const void*,const void*,unsigned,LONG,volatile LONG*){return &town;}}
namespace LoadedInstances{
static unsigned mode=0,alive=0,reads=0,reuses=0;static float sourceOrigin[3]={};
struct Stats{};
struct Packet{unsigned count,generation;float origin[3],matrix[16],bounds[6];Packet():count(0),generation(0){std::memset(origin,0,12);std::memset(matrix,0,64);std::memset(bounds,0,24);}unsigned Count()const{return count;}unsigned Generation()const{return generation;}const float*SourceSceneOrigin()const{return origin;}const float*RawSceneWorld(unsigned)const{return matrix;}const float*LocalBounds(unsigned)const{return bounds;}AtlasView::U64 BorrowedBuffers()const{return count*32;}AtlasView::U64 BorrowedTextures()const{return count*64;}bool SameContents(const Packet&p)const{return count==p.count&&generation==p.generation&&!std::memcmp(origin,p.origin,12);}};
class Snapshot{Packet packet_;Stats stats_;bool unchanged_;public:Snapshot():unchanged_(false){++alive;}~Snapshot(){--alive;}const Packet&Geometry()const{return packet_;}const Stats&Values()const{return stats_;}const char*LastError()const{return "refused fixture";}bool Unchanged()const{return unchanged_;}
 HRESULT Read(bool,Town*,Ogre::SceneManager*,LONG generation,volatile LONG*live,bool,ID3D11Device*,AtlasView::U64,AtlasView::U64,unsigned,const Packet*previous){++reads;if(mode==4){InterlockedIncrement(live);return E_ABORT;}if(mode==5&&previous==NULL)return E_FAIL;if(mode==1||mode==2||mode==5){unchanged_=true;if(previous)++reuses;return S_FALSE;}if(mode==3)return S_FALSE;packet_.count=1;packet_.generation=unsigned(generation);std::memcpy(packet_.origin,sourceOrigin,12);return S_OK;}
};}
namespace GsmLoadedTown{
'''
tests=r'''
}
static unsigned checks=0;static void C(bool b,const char*why){++checks;if(!b){std::printf("FAIL %s\n",why);throw 1;}}
int main(){try{using namespace LoadedInstances;GsmLoadedTown::State state;GsmTownValues::Result values;float camera[3]={},origin[3]={};Ogre::SceneManager scene;ID3D11Device device;volatile LONG live=7;const void*king=&town;std::string error;
 C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_OK&&state.Geometry()&&state.Geometry()->Count()==1&&alive==1,"fresh State owns one accepted snapshot");const Packet*held=state.Geometry();unsigned revision=state.Revision();
 mode=1;for(unsigned i=0;i<20;++i)C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_FALSE&&state.Geometry()==held&&state.Geometry()->Count()==1&&state.Revision()==revision&&alive==1,"explicit unchanged receipt preserves old geometry bounds and revision");C(reuses==20&&reads==21,"every unchanged State refresh still invokes native capture boundary");
 values.towns[0].id=3;mode=2;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==E_ABORT&&!state.Geometry()&&alive==0,"rogue unchanged result without matching old town cannot retain geometry");values.towns[0].id=2;mode=0;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_OK,"recover fresh geometry");
 mode=3;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_FALSE&&!state.Geometry()&&alive==0,"ordinary empty S_FALSE clears unloaded old town");mode=0;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_OK,"recover after empty capture");
 mode=4;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==E_ABORT&&!state.Geometry()&&alive==0,"world generation change clears old geometry despite previous packet");live=7;mode=0;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_OK,"recover after world change");
 values.count=2;values.towns[1]=values.towns[0];values.towns[1].storedXYZ[0]=100;values.towns[0].id=1;mode=5;C(state.Refresh(values,camera,origin,&scene,king,7,&live,true,&device,0,0,error)==S_FALSE&&state.TownId()==2&&state.Geometry()&&error.find("town=1")!=std::string::npos&&alive==1,"unchanged fallback preserves first nearer refusal diagnostics");
 state.Clear();C(!state.Geometry()&&alive==0,"explicit clear releases only owned snapshot");std::printf("PASS %u actual State unchanged lifecycle controls\n",checks);return 0;}catch(...){return 1;}}
'''
cpp=OUT/'state-reuse.cpp';cpp.write_text(prefix+state+tests)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 name='state-'+opt[1:];exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name,p.stdout+p.stderr,end='',flush=True)
 if p.returncode:raise SystemExit(p.returncode)
