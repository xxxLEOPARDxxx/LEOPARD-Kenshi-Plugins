"""Execute actual NativeOps passive inspector at fake native getter boundaries."""
from pathlib import Path
import os,subprocess,argparse
HERE=Path(__file__).resolve().parent;MODULE=HERE.parent;OUT=MODULE/'build/cached-town-state-tests';OUT.mkdir(parents=True,exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--header',type=Path);parser.add_argument('--label',default='native-inspector');args=parser.parse_args()
assert args.label.replace('-','').isalnum()
source=(args.header or MODULE/'GsmCachedTownState.h').read_text()
def function(signature):
 start=source.index(signature);a=source.index('{',start);b=a+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[start:b]
current=function('static bool ContextCurrent(')
unsafe=function('static HRESULT InspectUnsafe(')
needle='reinterpret_cast<Manager>(reinterpret_cast<const BYTE*>(c.engine)+0x569960)(town)'
assert unsafe.count(needle)==1
unsafe=unsafe.replace(needle,'FakeManager(town)') # Native leaf-call boundary only.
protected=function('static HRESULT InspectProtected(')
stubs=r'''
#define NOMINMAX
#define GSM_CACHED_TOWN_POLICY_ONLY
#include <Windows.h>
#include <cstdio>
#include <new>
namespace GsmTownValues{struct TownValue{unsigned id;float storedXYZ[3];bool hasProxy;};struct Result{LONG generation;unsigned count;TownValue towns[1];};}
#include "GsmCachedTownState.h"
using GsmCachedTownState::Context;using GsmCachedTownState::Finite;
namespace Ogre{struct SceneManager{};struct Matrix4{float value[16];const float*operator[](unsigned row)const{return value+4*row;}};}
struct Town{unsigned __int64 storage[192];};
struct Map{size_t count;size_t size()const{return count;}};
struct TownBuildingsManager{Map buildingEntities;Town*town;};
struct Renderer{const void*scene;};struct AU{Renderer*render;};
static Renderer renderer;static AU auValue;static AU*au;static void*shou;
static unsigned __int64 sceneStorage[192],rootToken;static Ogre::Matrix4 rootMatrix;static const char*GsmOgreFile="test-no-load.dll";static DWORD ModelSceneNodeVtable=0x694c38;
static Town arena;static Town*resolved;static TownBuildingsManager*manager;
static unsigned queries=0,originCalls=0;static bool profileOk=true,badManager=false,badReturn=false;static unsigned mutation=0;static float nativeOrigin[3]={1000,0,0};static volatile LONG generation=1;
static TownBuildingsManager*FakeManager(Town*){++queries;return badManager?NULL:manager;}
static void*FakeOrigin(const Ogre::SceneManager*,float*out){++originCalls;std::memcpy(out,nativeOrigin,12);BYTE*p=reinterpret_cast<BYTE*>(&arena);
 if(mutation==1)*reinterpret_cast<unsigned*>(p+0x64)=3;
 if(mutation==2)*reinterpret_cast<float*>(p+0x48)=1011;
 if(mutation==3)*reinterpret_cast<void**>(p+0x4f8)=reinterpret_cast<void*>(1);
 if(mutation==4)++manager->buildingEntities.count;
 if(mutation==5)InterlockedIncrement(&generation);
 return badReturn?NULL:out;
}
namespace NativeModels{inline void Profile(unsigned){++queries;}inline FARPROC Pin(const char*){return reinterpret_cast<FARPROC>(FakeOrigin);}inline bool Typed(const void*p,HMODULE,DWORD){return p==&rootToken;}template<class T>T Get(const void*,const char*);template<>inline const Ogre::Matrix4&Get<const Ogre::Matrix4&>(const void*,const char*){return rootMatrix;}inline bool Copy(const void*a,void*b,size_t n){std::memcpy(b,a,n);return true;}}
namespace NativeLoadedCensus{const unsigned EntryLimit=4096;inline bool Profile(){++queries;return profileOk;}inline const void*Pointer(const void*p){return *reinterpret_cast<const void*const*>(p);}inline unsigned Number(const void*p){return *reinterpret_cast<const unsigned*>(p);}}
namespace GsmModels{inline Town*Resolve(const void*,const void*,unsigned,LONG,volatile LONG*){++queries;return resolved;}}
namespace PrivateTownLod{inline int Fault(DWORD c){return c==EXCEPTION_ACCESS_VIOLATION||c==EXCEPTION_IN_PAGE_ERROR||c==EXCEPTION_DATATYPE_MISALIGNMENT;}}
struct Probe{
BODY
static HRESULT Call(const Context&c,const GsmCachedTown::Request&r){return InspectProtected(c,r);}
};
static unsigned checks=0;static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static Context Reset(){std::memset(&arena,0,sizeof(arena));BYTE*p=reinterpret_cast<BYTE*>(&arena);manager=new(p+0x250)TownBuildingsManager;manager->town=&arena;manager->buildingEntities.count=1;resolved=&arena;float xyz[3]={1010,100,0};std::memcpy(p+0x48,xyz,12);*reinterpret_cast<unsigned*>(p+0x64)=2;std::memset(sceneStorage,0,sizeof(sceneStorage));*reinterpret_cast<void**>(reinterpret_cast<BYTE*>(sceneStorage)+0x5f0)=&rootToken;std::memset(&rootMatrix,0,sizeof(rootMatrix));rootMatrix.value[0]=rootMatrix.value[5]=rootMatrix.value[10]=rootMatrix.value[15]=1;rootMatrix.value[3]=-1000;renderer.scene=sceneStorage;auValue.render=&renderer;au=&auValue;shou=reinterpret_cast<void*>(4);generation=1;queries=originCalls=mutation=0;badManager=badReturn=false;profileOk=true;nativeOrigin[0]=1000;nativeOrigin[1]=nativeOrigin[2]=0;
 Context c;c.scene=renderer.scene;c.king=shou;c.engine=GetModuleHandleA(NULL);c.generation=1;c.live=&generation;std::memcpy(c.origin,nativeOrigin,12);return c;}
int main(){try{GsmCachedTown::Request r;r.townId=2;r.storedXYZ[0]=1010;r.storedXYZ[1]=100;r.storedXYZ[2]=0;Context c=Reset();
 Check(Probe::Call(c,r)==S_OK&&originCalls==1,"actual native inspector accepts coherent source");
 c=Reset();generation=2;Check(Probe::Call(c,r)==E_ABORT&&!queries,"stale world prevents any native query");
 c=Reset();c.king=NULL;Check(Probe::Call(c,r)==E_ABORT&&!queries,"wrong SharedKing prevents queries");
 c=Reset();c.scene=NULL;Check(Probe::Call(c,r)==E_ABORT&&!queries,"wrong scene prevents queries");
 c=Reset();c.engine=NULL;Check(Probe::Call(c,r)==E_ABORT&&!queries,"wrong module prevents queries");
 c=Reset();au=NULL;Check(Probe::Call(c,r)==E_ABORT&&!queries,"missing renderer owner prevents queries");
 c=Reset();profileOk=false;Check(Probe::Call(c,r)==E_ACCESSDENIED&&!originCalls,"actual census profile refusal prevents source use");
 c=Reset();resolved=NULL;Check(Probe::Call(c,r)==S_FALSE&&!originCalls,"missing ID refuses before origin getter");
 c=Reset();*reinterpret_cast<void**>(reinterpret_cast<BYTE*>(&arena)+0x4f8)=reinterpret_cast<void*>(1);Check(Probe::Call(c,r)==S_FALSE&&!originCalls,"native proxy blocks duplicate fallback");
 c=Reset();*reinterpret_cast<float*>(reinterpret_cast<BYTE*>(&arena)+0x48)=1011;Check(Probe::Call(c,r)==S_FALSE&&!originCalls,"current native pose must match copied request");
 c=Reset();badManager=true;Check(Probe::Call(c,r)==E_INVALIDARG&&!originCalls,"exact embedded manager address enforced");
 c=Reset();manager->town=NULL;Check(Probe::Call(c,r)==E_INVALIDARG&&!originCalls,"manager back owner enforced");
 c=Reset();manager->buildingEntities.count=0;Check(Probe::Call(c,r)==S_FALSE&&!originCalls,"empty registry cannot count as loaded");
 c=Reset();manager->buildingEntities.count=4097;Check(Probe::Call(c,r)==E_INVALIDARG&&!originCalls,"4097 registry limit refused");
 c=Reset();manager->buildingEntities.count=4096;Check(Probe::Call(c,r)==S_OK,"4096 registry limit accepted");
 c=Reset();nativeOrigin[0]=1001;Check(Probe::Call(c,r)==E_ABORT,"current origin must match proven source frame");
 c=Reset();badReturn=true;Check(Probe::Call(c,r)==E_ABORT,"native origin hidden return pointer checked");
 c=Reset();rootMatrix.value[0]=2;Check(Probe::Call(c,r)==E_INVALIDARG,"nonidentity root basis refused before pose shortcut");
 c=Reset();rootMatrix.value[3]+=1;Check(Probe::Call(c,r)==E_INVALIDARG,"root translation must equal negative origin");
 c=Reset();*reinterpret_cast<void**>(reinterpret_cast<BYTE*>(sceneStorage)+0x5f0)=NULL;Check(Probe::Call(c,r)==E_INVALIDARG,"missing typed root refuses pose shortcut");
 for(unsigned i=1;i<=5;++i){c=Reset();mutation=i;Check(Probe::Call(c,r)==E_ABORT,"post-getter source and world reread rejects mutation");}
 c=Reset();resolved=reinterpret_cast<Town*>(1);Check(Probe::Call(c,r)==E_FAIL,"native access fault is filtered into local refusal");
 std::printf("Actual passive NativeOps inspector PASS %u checks\n",checks);return 0;}catch(...){return 1;}}
'''
cpp=OUT/(args.label+'.cpp');cpp.write_text(stubs.replace('BODY','\n'.join([current,unsafe,protected])))
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{MODULE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
exe=OUT/(args.label+'.exe');r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),'/Fo'+str(OUT/(args.label+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(OUT/(args.label+'-build.log')).write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(args.label+'-run.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');raise SystemExit(r.returncode)
