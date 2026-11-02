"""Compile actual local main methods/shade loop with bounded CPU source/GPU-call fixtures."""
from pathlib import Path
import os,subprocess
BASE=Path(__file__).resolve().parents[1];OUT=BASE/'build/local-composition-tests'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
def function(s,marker):
 a=s.index(marker);b=s.index('{',a);d=1;e=b+1
 while d:
  d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
def run():
 s=(BASE/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig');layer=(BASE/'atlas-lights/terrain-hdr/AtlasLocalLights.h').read_text();snap=(BASE/'atlas-lights/NativeLocalLightSnapshot.h').read_text()
 localtypes=snap[snap.index('struct LightValue {'):snap.index('inline bool Power(')]
 helpers='\n'.join(function(layer,k)+';' for k in ['struct GpuLight','struct ProvenMapping','struct Region'])+'\n'+'\n'.join(function(layer,k) for k in ['inline bool Pack(','inline bool Intersects('])
 bodies='\n'.join(function(s,k) for k in ['void ResetLocal()','void LocalDiagnostic(','bool WantsLocal(','HRESULT ApplyLocal(','HRESULT RenderLayers(','void WaterFailure('])
 a=s.index('    unsigned shaded=0,next=shadeCursor;');b=s.index('}shadeCursor=next;',a)+len('}shadeCursor=next;');shade='unsigned Shade(){AtlasView::WindowRect cells[16],clip(0,0,8192,8192);for(unsigned i=0;i<16;++i)cells[i]=AtlasView::WindowRect(0,0,8192,8192);Plan plan;'+s[a:b]+'return shaded;}'
 code=r'''
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <float.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include "atlas-view/AtlasWindowPolicy.h"
#include "GsmLocalRevision.h"
#include "GsmLocalDiagnostic.h"
static unsigned checks=0,errors=0,reads=0,intersections=0,localInit=0,pipelineInit=0,localCalls=0,terrainStage=0,waterStage=0,terrainFallback=0,writes=0;
static HRESULT nextRead=S_OK,nextLocalInit=S_OK,nextPipelineInit=S_OK,nextLocal=S_OK,nextWater=S_OK;
static AtlasView::U64 lastLocalCaller=0,lastPipelineCaller=0,lastApplyCaller=0;
void Check(bool x,const char*s){++checks;if(!x){std::printf("FAIL %s\n",s);throw 1;}}
namespace GSM{volatile LONG generation=7;}
namespace AtlasView{struct Uniforms{float tileBounds[4],referenceSide[4],worldOffset[4];};struct View{ID3D11ShaderResourceView*p;View():p(NULL){}};struct Cell{View view[3];};}
namespace NativeLocalLights{
const unsigned Limit=4096;
LOCAL_TYPES
struct Information{float SourceSceneOrigin[3];unsigned registered,hidden,daySuppressed;float rendererGain;struct{float time;}clock;Information(){std::memset(this,0,sizeof(*this));}};
static LightValue lights[4097];static Information nextInfo;static unsigned nextCount=1;static LONG nextGeneration=7;
struct Snapshot{bool valid;LONG gen;unsigned count;Information info;Snapshot():valid(false),gen(0),count(0){}void Reset(){valid=false;count=0;}bool Valid(LONG g)const{return valid&&gen==g;}unsigned Count()const{return valid?count:0;}const LightValue*Light(unsigned i)const{return valid&&i<count?lights+i:NULL;}const Information&Info()const{return info;}const char*LastError()const{return "fixture source refused";}HRESULT Read(bool on,LONG g,const void*scene,bool verified,const volatile LONG*live,DWORD){++reads;if(!on||!verified||!scene||!live||*live!=g||nextCount>Limit||FAILED(nextRead)){Reset();return E_FAIL;}valid=true;gen=nextGeneration;count=nextCount;info=nextInfo;return S_OK;}};typedef Snapshot NativeSnapshot;
}
namespace AtlasLocalLights{
const unsigned MaxBlock=256;const AtlasView::U64 PrivateBytes=526416;
LOCAL_HELPERS
struct Layer{unsigned LastBatches()const{return 1;}void Clear(){}HRESULT Init(ID3D11Device*,const void*,SIZE_T,AtlasView::U64 caller){++localInit;lastLocalCaller=caller;return nextLocalInit;}HRESULT Render(bool,const NativeLocalLights::Snapshot&,LONG,const ProvenMapping&,const Region&,ID3D11ShaderResourceView*const source[4],ID3D11Texture2D*,AtlasView::U64 caller){++localCalls;lastApplyCaller=caller;Check(source[0]==reinterpret_cast<ID3D11ShaderResourceView*>(99)&&source[1]==reinterpret_cast<ID3D11ShaderResourceView*>(1)&&source[2]==reinterpret_cast<ID3D11ShaderResourceView*>(2)&&source[3]==reinterpret_cast<ID3D11ShaderResourceView*>(3),"actual ApplyLocal passes HDR/material/normal/height slots in order");return nextLocal;}};
}
namespace AtlasWater{const AtlasView::U64 PipelinePrivateBytes=3408256;struct Uniforms{float tileBounds[4],sourceWindow[4],waterPlane[4];};struct HdrStage{void*context;HRESULT(*apply)(void*,const AtlasView::Cell&,const AtlasView::Uniforms&,ID3D11ShaderResourceView*,ID3D11Texture2D*);};}
static bool debug=false,localOption=true,localFailed=false,localLayerReady=false,ready=true,localDiagnosticLogged=false,originDiagnosticLogged=false,waterFailed=false,waterPipelineReady=false;
static DWORD localDiagnosticAt=0;static LONG observedGeneration=7;static unsigned localVersion=1,lightVersion=2,waterVersion=1,shadeCursor=0,localRenders=0,localBatches=0;
static const void*localProofScene=NULL;static const void*fakeScene=reinterpret_cast<void*>(11);
static AtlasLocalLights::Layer localLayer;static AtlasLocalLights::ProvenMapping localMapping;
static GsmLocalRevision::State<NativeLocalLights::LightValue,NativeLocalLights::Limit> localRevision;
static GsmLocalDiagnostic::Bridge<NativeLocalLights::NativeSnapshot> localDiagnostic;
struct Output{ID3D11Texture2D*p;Output():p(NULL){}};struct Entry{AtlasView::Cell gpu;Output output;AtlasView::WindowKey key;unsigned shaded,waterStamp,localStamp,localChecked,modelStamp;bool localAffects,localApplied,waterApplied,modelApplied;Entry():shaded(0),waterStamp(0),localStamp(0),localChecked(0),modelStamp(0),localAffects(false),localApplied(false),waterApplied(false),modelApplied(false){}};
static Entry cache[AtlasView::WindowSlots];static AtlasView::Uniforms lighting={};
static bool modelGpuReady=false,modelsOn=false,modelError=false;
static const AtlasView::U64 ModelGpuBytes=3145936;
static AtlasView::Cell modelCell;
struct ModelFixture{unsigned revision;ModelFixture():revision(1){}unsigned Revision()const{return revision;}}models;
bool ModelsAvailable(){return modelsOn;}
HRESULT PrepareModelCell(const Entry&e,const AtlasView::Uniforms&,bool&applied){modelCell=e.gpu;applied=modelsOn&&!modelError;return modelError?E_FAIL:S_OK;}
void ModelFailure(const char*){modelsOn=false;modelError=false;}

static void ErrorLog(const std::string&){++errors;}static void Diagnostic(const std::string&){}
bool DiagnosticScene(const void*&scene,const void*&camera){scene=fakeScene;camera=reinterpret_cast<void*>(12);return scene!=NULL;}
void WindowBounds(const AtlasView::WindowKey&,double&x,double&z,double&sx,double&sz){x=z=0;sx=sz=2304;}
REGION_BODY
struct Device{ID3D11Device*p;Device():p(NULL){}};struct NativeLight{Device device;ID3D11ShaderResourceView*views[3];ID3D11SamplerState*samplers[3];}nativeLight;
static unsigned char GsmLocalShader[1]={},AtlasTerrainHdrShader[1]={},AtlasWaterMaterialShader[1]={},AtlasWaterToneShader[1]={};
struct WaterSnapshot{AtlasWater::Uniforms values;void Clear(){}const AtlasWater::Uniforms&Values()const{return values;}const WaterSnapshot&Inputs()const{return *this;}ID3D11ShaderResourceView*View(unsigned)const{return NULL;}ID3D11SamplerState*Sampler(unsigned)const{return NULL;}}waterSnapshot;
struct Pipeline{void Clear(){}HRESULT Init(ID3D11Device*,const void*,SIZE_T,const void*,SIZE_T,const void*,SIZE_T,AtlasView::U64 caller){++pipelineInit;lastPipelineCaller=caller;return nextPipelineInit;}HRESULT Run(const AtlasView::Cell&c,const AtlasView::Uniforms&u,ID3D11Texture2D*o,const AtlasWater::HdrStage*s,bool water){HRESULT hr=S_OK;if(s)hr=s->apply(s->context,c,u,reinterpret_cast<ID3D11ShaderResourceView*>(99),o);if(FAILED(hr))return hr;if(water&&FAILED(nextWater))return nextWater;++writes;return S_OK;}HRESULT RenderTerrainWithStage(const AtlasView::Cell&c,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*const*,ID3D11SamplerState*const*,ID3D11Texture2D*o,AtlasView::U64 caller,const AtlasWater::HdrStage*s){++terrainStage;lastPipelineCaller=caller;return Run(c,u,o,s,false);}HRESULT RenderWithStage(const AtlasView::Cell&c,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*const*,ID3D11SamplerState*const*,const AtlasWater::Uniforms&w,ID3D11ShaderResourceView*const*,ID3D11SamplerState*const*,ID3D11Texture2D*o,AtlasView::U64 caller,const AtlasWater::HdrStage*s){++waterStage;lastPipelineCaller=caller;Check(w.sourceWindow[0]==256&&w.tileBounds[0]==u.tileBounds[0],"actual water stage uses same256 window bounds");return Run(c,u,o,s,true);}}waterPipeline;
struct Relighter{HRESULT Render(const AtlasView::Cell&,const AtlasView::Uniforms&,ID3D11ShaderResourceView*const*,ID3D11SamplerState*const*,ID3D11Texture2D*,unsigned*){++terrainFallback;++writes;return S_OK;}}relighter;
BODIES
static bool useWaterFixture=false;bool WantsWater(const Entry&){return useWaterFixture&&!waterFailed;}bool Superseded(const AtlasView::WindowKey&,unsigned){return false;}struct Plan{unsigned level;Plan():level(0){}bool Contains(const AtlasView::WindowKey&)const{return true;}};
SHADE
void Reset(){modelGpuReady=modelsOn=modelError=false;models.revision=1;errors=reads=localInit=pipelineInit=localCalls=terrainStage=waterStage=terrainFallback=writes=0;nextRead=nextLocalInit=nextPipelineInit=nextLocal=nextWater=S_OK;debug=false;localOption=true;localFailed=localLayerReady=localDiagnosticLogged=originDiagnosticLogged=waterFailed=waterPipelineReady=useWaterFixture=false;ready=true;observedGeneration=7;GSM::generation=7;localVersion=1;lightVersion=2;waterVersion=1;shadeCursor=0;localProofScene=NULL;fakeScene=reinterpret_cast<void*>(11);localMapping=AtlasLocalLights::ProvenMapping();localRevision.Reset();localDiagnostic.Reset();NativeLocalLights::nextCount=1;NativeLocalLights::nextGeneration=7;NativeLocalLights::nextInfo=NativeLocalLights::Information();std::memset(NativeLocalLights::lights,0,sizeof(NativeLocalLights::lights));NativeLocalLights::LightValue&v=NativeLocalLights::lights[0];v.worldPosition[0]=v.worldPosition[2]=100;v.range=200;v.basePower=v.effectivePower=1;v.diffuse[0]=1;v.type=0;
 for(unsigned i=0;i<AtlasView::WindowSlots;++i){cache[i]=Entry();cache[i].key=AtlasView::WindowKey(i%16,5,0,0);cache[i].shaded=lightVersion;cache[i].output.p=reinterpret_cast<ID3D11Texture2D*>(ULONG_PTR(i+1));for(unsigned j=0;j<3;++j)cache[i].gpu.view[j].p=reinterpret_cast<ID3D11ShaderResourceView*>(ULONG_PTR(j+1));}
}
void Admit(){LocalDiagnostic(1000);localMapping.verified=true;localMapping.generation=7;std::memcpy(localMapping.sourceSceneOrigin,NativeLocalLights::nextInfo.SourceSceneOrigin,12);localProofScene=fakeScene;}
int main(){try{
 Reset();localOption=false;LocalDiagnostic(1000);Check(reads==0,"OFF notdebug has no native snapshot queries");Check(!WantsLocal(cache[0]),"OFF refuses local draw");
 Reset();ready=false;LocalDiagnostic(1000);Check(localFailed&&!localMapping.verified&&errors==1,"unverified read fails closed");LocalDiagnostic(2000);Check(errors==1&&reads==1,"read failure latches without repeated query/errors");
 Reset();Admit();Check(localVersion==2&&WantsLocal(cache[0]),"first copied snapshot dirties and admits intersecting cell");unsigned version=localVersion;LocalDiagnostic(2000);Check(localVersion==version,"paused identical revision remains clean");NativeLocalLights::lights[0].diffuse[0]=.5f;LocalDiagnostic(3000);Check(localVersion!=version&&WantsLocal(cache[0]),"samecount paused changed material light dirties actual caller");
 Reset();Admit();for(unsigned i=0;i<AtlasView::WindowSlots;++i){cache[i].localApplied=true;cache[i].localStamp=localVersion;cache[i].localChecked=localVersion;cache[i].localAffects=true;}NativeLocalLights::lights[0].effectivePower=.25f;LocalDiagnostic(2000);Check(Shade()==4&&cache[0].localStamp==localVersion,"paused exact revision refresh dirties already shaded samecount windows");
 Reset();Admit();localMapping.verified=false;Check(!WantsLocal(cache[0]),"unproven mapping refuses");localMapping.verified=true;++observedGeneration;Check(!WantsLocal(cache[0]),"stale generation refuses mapping");observedGeneration=7;localMapping.sourceSceneOrigin[0]=1;Check(!WantsLocal(cache[0]),"source origin mismatch refuses");
 Reset();Admit();fakeScene=reinterpret_cast<void*>(13);LocalDiagnostic(2000);Check(!localMapping.verified&&!localProofScene,"scene replacement invalidates coordinate proof before reuse");
 Reset();Admit();NativeLocalLights::lights[0].worldPosition[0]=100000;NativeLocalLights::lights[0].worldPosition[2]=100000;LocalDiagnostic(2000);Check(!WantsLocal(cache[0]),"outside radius skips local stage");
 Reset();Admit();AtlasView::Uniforms u={};u.tileBounds[2]=u.tileBounds[3]=2304;Check(RenderLayers(cache[0],cache[0].gpu,u,false,true)==S_OK&&localInit==1&&pipelineInit==1&&localCalls==1&&terrainStage==1&&!waterStage,"waterOFF locals use terrain HDR stage");Check(lastLocalCaller==AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes&&lastPipelineCaller==AtlasView::WindowGpuBytes()+AtlasLocalLights::PrivateBytes&&lastApplyCaller==lastLocalCaller,"actual Init/Render ledgers include external private resources once");
 Reset();Admit();Check(RenderLayers(cache[0],cache[0].gpu,u,true,false)==S_OK&&!localInit&&!localCalls&&waterStage==1,"localOFF water uses null HDR callback");
 Reset();Admit();Check(RenderLayers(cache[0],cache[0].gpu,u,true,true)==S_OK&&localCalls==1&&waterStage==1,"waterON locals pass callback once");
 Reset();Admit();nextLocalInit=E_FAIL;Check(FAILED(RenderLayers(cache[0],cache[0].gpu,u,false,true))&&!pipelineInit&&!writes,"local init failure stops before display/pipeline");
 Reset();Admit();nextPipelineInit=E_FAIL;Check(FAILED(RenderLayers(cache[0],cache[0].gpu,u,false,true))&&!localCalls&&!writes,"pipeline init failure stops before stage/display");
 Reset();Admit();Check(Shade()<=4&&writes==4&&cache[0].localApplied,"actual Draw four-window local budget");localOption=false;unsigned total=0;for(unsigned i=0;i<(AtlasView::WindowSlots+3)/4;++i)total+=Shade();Check(total==4&&terrainFallback==4&&!cache[0].localApplied,"OFF dirties old applied locals then restores only affected outputs");
 Reset();Admit();nextLocal=E_FAIL;Check(Shade()<=4&&localFailed&&errors==1&&terrainFallback>0&&!cache[0].localApplied,"stage failure latches and restores terrain sameiteration");
 Reset();Admit();useWaterFixture=true;nextLocal=E_FAIL;Check(Shade()<=4&&localFailed&&!waterFailed&&waterStage>1&&cache[0].waterApplied&&!cache[0].localApplied,"local failure retries water with null stage retaining water");
 Reset();Admit();useWaterFixture=true;nextWater=E_FAIL;Check(Shade()<=4&&localFailed&&waterFailed&&terrainFallback>0&&!cache[0].localApplied&&!cache[0].waterApplied,"all composition failures fall back bounded terrain");
 Reset();Admit();modelsOn=true;modelGpuReady=true;Check(Shade()==4&&cache[0].modelApplied&&cache[0].modelStamp==models.Revision(),"model revision shades actual caller with reused model cell");Check(lastPipelineCaller==AtlasView::WindowGpuBytes()+AtlasLocalLights::PrivateBytes+ModelGpuBytes,"model working resources charged to pipeline");
 modelsOn=false;shadeCursor=0;Check(Shade()==4&&!cache[0].modelApplied,"disabling models redraws cached outputs");
 Reset();Admit();modelsOn=true;modelError=true;Check(Shade()==4&&!cache[0].modelApplied&&!modelsOn,"model preparation failure restores pristine terrain and keeps light layer");
 Check(AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes+AtlasLocalLights::PrivateBytes<AtlasView::GpuCap,"combined declared private ledger remains under shared cap");
 std::printf("PASS %u extracted production local-composition checks\n",checks);return 0;
 }catch(...){return 1;}}
'''.replace('LOCAL_TYPES',localtypes).replace('LOCAL_HELPERS',helpers).replace('REGION_BODY',function(s,'AtlasLocalLights::Region LocalRegion(')).replace('BODIES',bodies).replace('SHADE',shade)
 OUT.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;{BASE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
 def compile_run(name,text):
  cpp=OUT/(name+'.cpp');cpp.write_text(text,encoding='utf-8');exe=OUT/(name+'.exe');r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),'/Fe'+str(exe),'/Fo'+str(OUT/(name+'.obj')),'/link','user32.lib'],env=env,capture_output=True,text=True)
  if r.returncode:raise RuntimeError(r.stdout+r.stderr)
  return subprocess.run([str(exe)],capture_output=True,text=True)
 r=compile_run('fixture',code);print(r.stdout+r.stderr,end='');assert r.returncode==0,'actual caller regression'
 mutations=[
 ('ignore local OFF','!localOption||localFailed||','localFailed||'),
 ('ignore exact revision','localRevision.Observe(values,observedGeneration)','false'),
 ('reuse stale cell eligibility','if(e.localChecked==localVersion)','if(true)'),
 ('omit local init','if(useLocal&&!localLayerReady)','if(false)'),
 ('omit HDR callback','useLocal?&stage:NULL','NULL'),
 ('omit external caller ledger','(localLayerReady?AtlasLocalLights::PrivateBytes:0)','0'),
 ('omit pipeline in stage ledger','AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes','AtlasView::WindowGpuBytes()'),
 ('omit local failure fallback','if(FAILED(hr)&&useLocal)','if(false)'),
 ('omit water retry','if(useWater)hr=RenderLayers(e,cell,u,true,false);',''),
 ('ignore paused version','(useLocal&&e.localStamp!=localVersion)','false'),
 ('inherit stale scene proof','if(localProofScene&&localProofScene!=scene)','if(false)'),
 ('exceed four window budget','shaded<4','shaded<5')]
 for n,old,new in mutations:
  assert old in code,n
  mutated=code.replace(old,new) if n=='exceed four window budget' else code.replace(old,new,1)
  result=compile_run('mutant',mutated);assert result.returncode!=0,(n,'SURVIVED');print('PASS caller mutant',n)
 print('PASS',len(mutations),'compiled actual caller mutants')
 return code,compile_run
if __name__=='__main__':run()
