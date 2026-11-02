"""Extract real main water/Poll/settings/shade control flow into a CPU fixture."""
from pathlib import Path
import os,subprocess
BASE=Path(__file__).resolve().parents[1];OUT=BASE/'build/water-caller-tests'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')

def function(text,marker):
 a=text.index(marker);b=text.index('{',a);d=1;e=b+1
 while d:
  if text[e]=='{':d+=1
  elif text[e]=='}':d-=1
  e+=1
 return text[a:e]

def run():
 s=(BASE/'GlobalSatelliteMap.cpp').read_text()
 bodies='\n'.join(function(s,m) for m in ('void Settings()','void Poll(MapScreen*map)','void WaterFailure(','void WaterState(','bool WantsWater('))
 a=s.index('    unsigned shaded=0,next=shadeCursor;');b=s.index('}shadeCursor=next;',a)+len('}shadeCursor=next;')
 shade='static unsigned ShadeSlice(){AtlasView::WindowRect cells[16],clip(0,0,8192,8192);for(unsigned i=0;i<16;++i)cells[i]=AtlasView::WindowRect(0,0,8192,8192);Plan plan;'+s[a:b]+'return shaded;}'
 code=r'''
#include "GsmAtlasManifest.h"
#include "GsmViewerClock.h"
#include "atlas-water/AtlasWater.h"
#include <cstdio>
static unsigned checks=0,errors=0,terrainFailures=0,waterRenders[AtlasView::WindowSlots]={0},terrainRenders[AtlasView::WindowSlots]={0};
static const char*ID="GlobalSatelliteMap";
static void Check(bool x,const char*s){++checks;if(!x){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
namespace GSM {volatile LONG generation=7;}
namespace GsmAtlas {struct Job{HANDLE thread;LONG generation;bool waterValidation,validation,ok;char error[512];Manifest manifest;AtlasView::ReadRequest read;Job():thread(reinterpret_cast<HANDLE>(1)),generation(7),waterValidation(false),validation(false),ok(false){std::strcpy(error,"fixture source error");}};}
struct MapScreen{};struct Output{ID3D11Texture2D*p;Output():p(NULL){}};
struct Entry{AtlasView::WindowKey key;unsigned shaded,waterStamp;bool waterApplied,hasHeight;float minHeight;int gpu;Output output;Entry():shaded(0),waterStamp(0),waterApplied(false),hasHeight(true),minHeight(90),gpu(0){}};
static Entry cache[AtlasView::WindowSlots];static unsigned shadeCursor=0,lightVersion=5,waterVersion=1;
static bool enabled=true,hookFailure=false,sunlight=true,ambient=true,debug=false,haveLighting=true,ready=true,waterOption=true,waterFailed=false,waterAttempted=true,waterPipelineReady=true,jobQuarantined=false;
static LONG observedGeneration=7;static GsmViewer::LightClock lightClock,waterClock;
static GsmAtlas::Manifest manifest;static GsmAtlas::Job*job=NULL,*completed=NULL;
static HRESULT nextRead=S_OK,nextRender=S_OK;static unsigned readCalls=0,pipelineClears=0;
namespace NativeWater {
struct Resources{ID3D11ShaderResourceView*v[5];ID3D11SamplerState*s[5];Resources(){for(unsigned i=0;i<5;++i){v[i]=reinterpret_cast<ID3D11ShaderResourceView*>(100+i);s[i]=reinterpret_cast<ID3D11SamplerState*>(200+i);}}ID3D11ShaderResourceView*View(unsigned i)const{return v[i];}ID3D11SamplerState*Sampler(unsigned i)const{return s[i];}unsigned Mips(unsigned)const{return 1;}};
struct Snapshot{AtlasWater::Uniforms value,next;Resources resources,nextResources;bool valid;unsigned gen;Snapshot():valid(false),gen(7){std::memset(&value,0,sizeof(value));value.waterPlane[0]=100;value.waterPlane[1]=1;next=value;}void Clear(){valid=false;}bool Valid(unsigned g)const{return valid&&g==gen;}HRESULT Read(bool on,bool sun,bool ambient,unsigned g,bool verified){++readCalls;if(FAILED(nextRead)){Clear();return nextRead;}Check(on&&verified,"caller passes enabled and verified source flags");value=next;resources=nextResources;valid=true;gen=g;return S_OK;}const AtlasWater::Uniforms&Values()const{return value;}const Resources&Inputs()const{return resources;}const char*LastError()const{return "fixture native failure";}float DepthBiasConstant()const{return -1;}float DepthBiasSlope()const{return -1;}};
}
static NativeWater::Snapshot waterSnapshot;
struct Pipeline{void Clear(){++pipelineClears;}};static Pipeline waterPipeline;
struct Device{ID3D11Device*p;Device():p(NULL){}};
struct NativeLight{Device device;ID3D11ShaderResourceView*views[3];ID3D11SamplerState*samplers[3];void Init(){}};static NativeLight nativeLight;
struct Relighter{HRESULT Init(ID3D11Device*,const void*,size_t,unsigned,AtlasView::U64){return S_OK;}HRESULT Render(int,const AtlasView::Uniforms&,ID3D11ShaderResourceView*const*,ID3D11SamplerState*const*,ID3D11Texture2D*p,unsigned*){++terrainRenders[reinterpret_cast<ULONG_PTR>(p)-1];return S_OK;}};static Relighter relighter;
static unsigned char GsmRelightShader[1]={0};static AtlasView::Uniforms lighting={};
static void ErrorLog(const std::string&){++errors;}static void Diagnostic(const std::string&){}
static void Fail(MapScreen*,const std::string&){++terrainFailures;ready=false;}
static DWORD FakeWait(HANDLE,DWORD){return WAIT_OBJECT_0;}static BOOL FakeClose(HANDLE){return TRUE;}
static std::string Ini(){return "fixture-only";}static bool iniWater=true;
static UINT FakeIni(const char*,const char*key,int def,const char*){return !std::strcmp(key,"Water")?iniWater:!std::strcmp(key,"Enabled")||!std::strcmp(key,"Sunlight")||!std::strcmp(key,"AmbientLight")?1:def;}
namespace KLocLog{static void SetEnabled(bool){}}
#define WaitForSingleObject FakeWait
#define CloseHandle FakeClose
#define GetPrivateProfileIntA FakeIni
BODIES
static bool Superseded(const AtlasView::WindowKey&,unsigned){return false;}
static void WindowBounds(const AtlasView::WindowKey&,double&x,double&z,double&sx,double&sz){x=z=0;sx=sz=2304;}
struct Plan{unsigned level;Plan():level(0){}bool Contains(const AtlasView::WindowKey&)const{return true;}};
static HRESULT RenderWater(Entry&e,const AtlasView::Uniforms&){++waterRenders[reinterpret_cast<ULONG_PTR>(e.output.p)-1];return nextRender;}
SHADE
static void Reset(){
 errors=terrainFailures=readCalls=pipelineClears=0;enabled=ready=sunlight=ambient=haveLighting=waterOption=waterAttempted=waterPipelineReady=true;hookFailure=debug=waterFailed=jobQuarantined=false;nextRead=nextRender=S_OK;iniWater=true;shadeCursor=0;lightVersion=5;waterVersion=1;observedGeneration=7;InterlockedExchange(&GSM::generation,7);waterClock.Reset();lightClock.Reset();manifest=GsmAtlas::Manifest();manifest.waterVerified=true;manifest.header[0][0]=42;waterSnapshot=NativeWater::Snapshot();waterSnapshot.valid=true;
 for(unsigned i=0;i<AtlasView::WindowSlots;++i){cache[i]=Entry();cache[i].key=i<16?AtlasView::WindowKey(i,5,0,0):AtlasView::WindowKey((i-16)%16,0,(i-16)/16,0);cache[i].shaded=lightVersion;cache[i].waterStamp=waterVersion;cache[i].waterApplied=true;cache[i].output.p=reinterpret_cast<ID3D11Texture2D*>(ULONG_PTR(i+1));waterRenders[i]=terrainRenders[i]=0;}
}
int main(){try{
 Reset();waterOption=false;WaterState(200);Check(readCalls==0,"OFF does not query snapshot");waterOption=true;manifest.waterVerified=false;WaterState(400);Check(readCalls==0,"unverified profile never queries native snapshot");manifest.waterVerified=true;waterFailed=true;WaterState(600);Check(readCalls==0,"failed water remains latched without retries");
 Reset();WaterState(200);Check(readCalls==1&&waterVersion==1,"unchanged paused snapshot does not dirty water");WaterState(250);Check(readCalls==1,"caller snapshot cadence bounded200ms");waterSnapshot.nextResources.s[2]=reinterpret_cast<ID3D11SamplerState*>(999);WaterState(400);Check(waterVersion==2,"sampler-only paused refresh dirties existing water");waterSnapshot.nextResources.v[1]=reinterpret_cast<ID3D11ShaderResourceView*>(998);WaterState(600);Check(waterVersion==3,"current native SRV identity change dirties water");waterSnapshot.next.waterTime[0]=1;WaterState(800);Check(waterVersion==4,"native gameTime animates water dirtiness");
 Reset();nextRead=E_FAIL;WaterState(200);Check(waterFailed&&!waterSnapshot.Valid(7)&&!waterPipelineReady&&pipelineClears==1&&errors==1&&ready,"native read failure retires only water");WaterState(400);Check(readCalls==1&&errors==1,"native water failure latches without repeat errors");for(unsigned i=0;i<(AtlasView::WindowSlots+3)/4;++i)Check(ShadeSlice()<=4,"read failure restoration remains bounded");for(unsigned i=0;i<AtlasView::WindowSlots;++i)Check(!cache[i].waterApplied&&terrainRenders[i]==1,"read failure restores previously visible water to terrain");
 Reset();iniWater=false;Settings();Check(!waterOption&&!waterSnapshot.Valid(7)&&!waterPipelineReady&&pipelineClears==1&&ready,"actual Settings OFF retires water while retaining terrain");for(unsigned i=0;i<(AtlasView::WindowSlots+3)/4;++i)Check(ShadeSlice()<=4,"OFF restoration remains four-window bounded");for(unsigned i=0;i<AtlasView::WindowSlots;++i)Check(!cache[i].waterApplied&&terrainRenders[i]==1,"visible old water is restored to terrain exactly once");
 Reset();for(unsigned tick=0;tick<(AtlasView::WindowSlots+3)/4;++tick){++waterVersion;Check(ShadeSlice()<=4,"animated refresh remains four-window bounded");}for(unsigned i=0;i<AtlasView::WindowSlots;++i)Check(waterRenders[i]>0,"actual round-robin shader loop never starves a visible wet window");
 Reset();nextRender=E_FAIL;++waterVersion;Check(ShadeSlice()<=4&&waterFailed&&errors==1&&ready&&terrainRenders[0]>0&&!cache[0].waterApplied,"composition failure falls back to terrain within same bounded iteration");
 Reset();cache[0].minHeight=101;Check(!WantsWater(cache[0]),"dry minimum skips water");cache[0].minHeight=100;Check(WantsWater(cache[0]),"equal actual water height remains eligible");cache[0].hasHeight=false;Check(!WantsWater(cache[0]),"empty height coverage skips water");
 Reset();job=new GsmAtlas::Job;job->waterValidation=true;job->ok=false;Poll(NULL);Check(!job&&waterFailed&&!manifest.waterVerified&&ready&&terrainFailures==0&&manifest.header[0][0]==42,"water validation failure precedes generic terrain failure and preserves cache manifest");
 Reset();job=new GsmAtlas::Job;job->waterValidation=true;job->ok=job->manifest.waterVerified=true;Poll(NULL);Check(!job&&manifest.waterVerified&&manifest.header[0][0]==42&&ready&&terrainFailures==0,"water validation success merges acceptance flag only");
 Reset();job=new GsmAtlas::Job;job->waterValidation=true;job->generation=6;Poll(NULL);Check(!job&&!waterFailed&&manifest.waterVerified&&manifest.header[0][0]==42,"old generation completion cannot overwrite current state");
 Reset();waterFailed=true;iniWater=false;Settings();Check(!waterAttempted&&!manifest.waterVerified&&!waterFailed,"explicit toggle resets failed water validation for later bounded retry");
 std::printf("Extracted main water caller PASS %u checks\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('BODIES',bodies).replace('SHADE',shade)
 OUT.mkdir(parents=True,exist_ok=True);cpp=OUT/'fixture.cpp';cpp.write_text(code)
 env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/deps/boost_1_60_0;{BASE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
 def compile_run(name,text):
  f=OUT/(name+'.cpp');f.write_text(text);exe=OUT/(name+'.exe')
  build=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',f'/Fo{OUT}/{name}.obj',f'/Fe{exe}',str(f),'/link','user32.lib'],env=env,capture_output=True,text=True)
  if build.returncode:raise RuntimeError(build.stdout+build.stderr)
  return subprocess.run([str(exe)],capture_output=True,text=True)
 result=compile_run('fixture',code);print(result.stdout+result.stderr,end='')
 if result.returncode:raise RuntimeError('main caller regression failed')
 anchor='||beforeSampler[i]!=waterSnapshot.Inputs().Sampler(i)'
 if code.count(anchor)!=1:raise ValueError('sampler dirty-control anchor changed')
 old=compile_run('omit-sampler-dirty',code.replace(anchor,''));print(old.stdout+old.stderr,end='')
 assert old.returncode and 'FAIL sampler-only paused refresh dirties existing water' in old.stdout+old.stderr
 print('PASS compiled sampler-only functional negative control rejected')

if __name__=='__main__':run()
