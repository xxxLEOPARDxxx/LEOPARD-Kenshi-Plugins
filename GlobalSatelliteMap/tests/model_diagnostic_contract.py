from pathlib import Path
import os,subprocess
base=Path(__file__).resolve().parents[1];out=base/'build/model-diagnostic-review';out.mkdir(exist_ok=True)
header=(base/'GsmModelDiagnostic.h').read_text()
for banned in ['AtlasModels::Raster','.Render(','.Init(','CreateTexture','CreateBuffer','CreateShader','Draw(','ErrorLog(','std::string','_getTexturePtr','_notifyResourceLoaded']:
    assert banned not in header,banned
assert 'VertexCount(' not in header
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
def build(cpp,name):
    exe=out/(name+'.exe')
    with (out/(name+'-compile.log')).open('w') as log:
        subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/Fe'+str(exe),'/Fo'+str(out/(name+'.obj')),str(cpp)],env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
    return exe
native_header=header.replace('#include "atlas-models/loaded-census/NativeCensus.h"','// Native sources replaced only by owned fixture seams.').replace('"atlas-models/loaded-census/CensusRoute.h"','"'+(base/'atlas-models/loaded-census/CensusRoute.h').as_posix()+'"')
(out/'native_fixture.h').write_text(native_header)
source=r'''
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include "ROUTE"
static unsigned checks=0,constructors=0,models=0,censuses=0,held=0,clears=0;
static HRESULT modelResult=S_OK,censusResult=S_OK;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Town{};namespace Ogre{struct SceneManager{};}
static Town town;static Ogre::SceneManager scene;
namespace AtlasModels {struct Packet {float world[16],origin[3];Packet(){for(unsigned i=0;i<16;++i)world[i]=float(10+i);for(unsigned i=0;i<3;++i)origin[i]=float(40+i);}unsigned Count()const{return 2;}const float*RawSceneWorld()const{return world;}const float*SourceSceneOrigin()const{return origin;}};}
namespace NativeModels {class Snapshot {AtlasModels::Packet packet;unsigned generation;float bounds[6];public:
Snapshot():generation(0){++constructors;for(unsigned i=0;i<6;++i)bounds[i]=float(60+i);}
HRESULT Read(bool enabled,Town*t,Ogre::SceneManager*s,unsigned g,bool verified){C(enabled&&verified&&t==&town&&s==&scene&&g!=0,"actual NativeOps preserves enabled/profile/town/scene/generation arguments");++models;held=1;generation=g;return modelResult;}
void Clear(){++clears;held=0;}const char*LastError()const{return "actual source message";}bool Valid(unsigned g)const{return SUCCEEDED(modelResult)&&g==generation;}const AtlasModels::Packet&Geometry()const{return packet;}const float*LocalMeshBounds()const{return bounds;}
};}
namespace NativeLoadedCensus {HRESULT Read(bool enabled,Town*t,Ogre::SceneManager*s,LONG generation,volatile LONG*live,bool verified,Result&r){C(enabled&&verified&&t==&town&&s==&scene&&generation==*live,"actual NativeOps preserves loaded census inputs");C(!held,"actual native packet Clear precedes census");++censuses;r.selected=4;strcpy_s(r.materials[0].name,"copied native material");return censusResult;}}
#include "native_fixture.h"
int main(){try{GsmModelDiagnostic::Once once;volatile LONG live=7;bool fresh=true;
C(once.Capture(false,reinterpret_cast<Town*>(1),reinterpret_cast<Ogre::SceneManager*>(1),7,reinterpret_cast<volatile LONG*>(1),false,&fresh)==S_FALSE&&!fresh&&!constructors,"actual facade OFF does not construct native snapshot");
C(once.Capture(true,&town,&scene,7,&live,false,&fresh)==E_ACCESSDENIED&&!constructors,"actual facade unverified does not construct snapshot");
C(once.Capture(true,NULL,&scene,7,&live,true,&fresh)==E_INVALIDARG&&!constructors,"actual facade missing town does not construct snapshot");
C(once.Capture(true,&town,&scene,7,&live,true,&fresh)==S_OK&&fresh&&constructors==1&&models==1&&censuses==1&&!held&&clears==1,"actual native adapter captures once and releases packet");
const GsmModelDiagnostic::Report&r=once.Values();C(r.modelHr==S_OK&&r.censusHr==S_OK&&r.packets==2&&r.modelValid&&!r.modelShadersVerified,"actual native model source codes and no shader proof preserved");
for(unsigned i=0;i<16;++i)C(r.rawSceneWorld[i]==float(10+i),"actual adapter copies full raw world matrix unchanged");
for(unsigned i=0;i<3;++i)C(r.sourceSceneOrigin[i]==float(40+i),"actual adapter copies same snapshot origin unchanged");
for(unsigned i=0;i<6;++i)C(r.localBounds[i]==float(60+i),"actual adapter copies six local bounds unchanged");
C(r.loaded.selected==4&&!std::strcmp(r.loaded.materials[0].name,"copied native material"),"actual native census numeric/name result retained");
C(once.Capture(true,&town,&scene,7,&live,true,&fresh)==S_FALSE&&!fresh&&constructors==1&&models==1,"repeat facade avoids even native snapshot construction");
once.Reset();modelResult=E_NOINTERFACE;C(once.Capture(true,&town,&scene,7,&live,true,&fresh)==E_NOINTERFACE&&fresh&&!once.Values().modelValid&&!once.Values().packets&&!held,"actual source unsupported HRESULT retained and packet released");
C(!std::strcmp(once.Values().modelError,"actual source message")&&once.Values().censusHr==S_OK,"actual LastError copied before release, separate census still recorded");
std::printf("actual model diagnostic adapter: %u checks PASS\n",checks);return 0;}catch(...){return 1;}}
'''.replace('ROUTE',(base/'atlas-models/loaded-census/CensusRoute.h').as_posix())
cpp=out/'native_fixture.cpp';cpp.write_text(source);subprocess.run([str(build(cpp,'native_fixture'))],check=True)
fixture=(base/'tests/model_diagnostic_tests.cpp').read_text()
mutants=[
('omit-packet-clear','ops.Clear(); // Retire','; // Retire'),
('repeat-source-query','return epoch_==generation&&attempted_?S_FALSE:S_OK;','return S_OK;'),
('query-off','if(!enabled)return S_FALSE;','if(false)return S_FALSE;'),
('query-unverified','if(!verified)return E_ACCESSDENIED;','if(false)return E_ACCESSDENIED;'),
('keep-failed-model','if(FAILED(next.modelHr))EraseModel(next);',''),
('keep-failed-census','if(FAILED(next.censusHr))next.loaded=NativeLoadedCensus::Result();',''),
('skip-world-postcheck','if(!Generation(live,generation))return Changed(generation);',''),
('invent-model-shader-proof','modelShadersVerified(false)','modelShadersVerified(true)'),
]
for name,before,after in mutants:
    assert before in header,name
    mutated=header.replace(before,after,1).replace('"atlas-models/loaded-census/CensusRoute.h"','"'+(base/'atlas-models/loaded-census/CensusRoute.h').as_posix()+'"')
    path=out/(name+'.h');path.write_text(mutated);cpp=out/(name+'.cpp');cpp.write_text(fixture.replace('#include "../GsmModelDiagnostic.h"','#include "'+path.as_posix()+'"'))
    result=subprocess.run([str(build(cpp,name))],capture_output=True,text=True);(out/(name+'-run.log')).write_text(result.stdout+result.stderr)
    assert result.returncode and 'FAIL ' in result.stderr,(name,result.stdout,result.stderr)
    print('Model diagnostic compiled mutant rejected:',name)
