"""Actual Root Poll/typed cached completion bodies; no native/GPU/atlas IO."""
from pathlib import Path
import hashlib,json,os,subprocess
BASE=Path(__file__).resolve().parents[1];OUT=BASE/'build/cached-town-caller-tests';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
s=(BASE/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8')
def body(marker):
 a=s.index(marker);b=s.index('{',a);e=b+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
code=r"""
#include <windows.h>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <memory>
namespace AtlasView{typedef unsigned __int64 U64;const unsigned WindowSlots=144;U64 WindowGpuBytes(){return 189006000;}U64 CellBytes(unsigned){return 0;}}
namespace AtlasWater{const AtlasView::U64 PipelinePrivateBytes=3408256;}namespace AtlasLocalLights{const AtlasView::U64 PrivateBytes=526416;}
const AtlasView::U64 ModelGpuBytes=3145936;
namespace GSM{volatile LONG generation=7;}
namespace GsmCachedTown{enum State{Empty,Ready,Unavailable,Cancelled,Refused};struct Request{unsigned townId;float storedXYZ[3];Request():townId(2){storedXYZ[0]=11;storedXYZ[1]=1533;storedXYZ[2]=22;}};struct Result{State state;unsigned bytes;Result():state(Empty),bytes(0){}};}
unsigned deleted=0,failures=0,errors=0,diagnostics=0,adopts=0,invalidations=0,sceneQueries=0,terrainInits=0;
struct Manifest{bool modelVerified,waterVerified;Manifest():modelVerified(false),waterVerified(false){}}manifest;
namespace GsmAtlas{struct Read{volatile LONG cancel;Read():cancel(0){}};struct Job{HANDLE thread;LONG generation;bool cachedTown,modelValidation,waterValidation,validation,ok;Read read;Manifest manifest;char error[128];GsmCachedTown::Request cachedTownRequest;GsmCachedTown::Result cachedTownResult;Job():thread(CreateEvent(NULL,TRUE,TRUE,NULL)),generation(7),cachedTown(false),modelValidation(false),waterValidation(false),validation(false),ok(false){std::strcpy(error,"fixture");}~Job(){++deleted;}};}
struct MapScreen{};GsmAtlas::Job*job=NULL,*completed=NULL;
bool jobQuarantined=false,enabled=true,ready=true,modelOption=true,modelFailed=false,waterFailed=false;LONG observedGeneration=7;
struct Mapping{bool verified;unsigned generation;float sourceSceneOrigin[3];Mapping():verified(true),generation(7){sourceSceneOrigin[0]=100;sourceSceneOrigin[1]=0;sourceSceneOrigin[2]=200;}}modelMapping;
const void*modelProofScene=reinterpret_cast<void*>(8);bool sceneOk=true;const void*nextScene=reinterpret_cast<void*>(8);
bool DiagnosticScene(const void*&scene,const void*&camera){++sceneQueries;scene=nextScene;camera=NULL;return sceneOk;}
struct Device{void*p;Device():p(reinterpret_cast<void*>(16)){}};struct NativeLight{Device device;void Init(){++terrainInits;}}nativeLight;
struct Models{void Invalidate(){++invalidations;}}models;
struct Cached{unsigned revision;bool valid;Cached():revision(1),valid(true){}unsigned Revision()const{return revision;}HRESULT Adopt(const GsmCachedTown::Request&r,const GsmCachedTown::Result&v,LONG g,const float*o,const void*scene,void*device,bool verified,AtlasView::U64 bytes){++adopts;if(r.townId!=2||r.storedXYZ[1]!=1533||g!=7||o!=modelMapping.sourceSceneOrigin||scene!=modelProofScene||device!=nativeLight.device.p||!verified||bytes!=196086608)throw std::runtime_error("wrong adoption args");if(v.state==GsmCachedTown::Ready){if(v.bytes!=108057)throw std::runtime_error("lost owned worker bytes");valid=true;++revision;return S_OK;}if(v.state==GsmCachedTown::Unavailable)return S_FALSE;if(v.state==GsmCachedTown::Refused)return E_FAIL;return S_FALSE;}const char*LastError(){return "optional source refused";}unsigned TownId(){return 2;}AtlasView::U64 OwnedGpuBytes(){return 323550;}SIZE_T CpuPayloadBytes(){return 108057;}}cachedTown;
struct Relighter{HRESULT Init(void*,const void*,SIZE_T,unsigned,AtlasView::U64){++terrainInits;return S_OK;}}relighter;const unsigned char GsmRelightShader[]={0};
bool throwDiagnostic=false,throwError=false;
void ErrorLog(const std::string&){++errors;if(throwError)throw std::runtime_error("fake log allocation failure");}void Diagnostic(const std::string&){++diagnostics;if(throwDiagnostic)throw std::runtime_error("fake diagnostic allocation failure");}void Fail(MapScreen*,const std::string&){++failures;}
ACTUAL
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL %u %s\n",__LINE__,#x);return 1;}}while(0)
void Pending(GsmCachedTown::State state,bool ok){job=new GsmAtlas::Job();job->cachedTown=true;job->ok=ok;job->cachedTownResult.state=state;job->cachedTownResult.bytes=state==GsmCachedTown::Ready?108057:0;}
int main(){MapScreen map;
 Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(!job&&!completed&&adopts==1&&invalidations==1&&deleted==1&&!failures&&!terrainInits);
 CHECK(cachedTown.valid&&diagnostics==1&&!errors);
 Pending(GsmCachedTown::Unavailable,true);Poll(&map);CHECK(adopts==2&&invalidations==1&&cachedTown.valid&&!failures&&!completed&&deleted==2);
 Pending(GsmCachedTown::Refused,false);Poll(&map);CHECK(adopts==3&&errors==1&&!failures&&!completed&&deleted==3&&cachedTown.valid);
 Pending(GsmCachedTown::Cancelled,false);job->read.cancel=1;Poll(&map);CHECK(adopts==3&&!job&&deleted==4&&!failures);
 Pending(GsmCachedTown::Ready,true);job->generation=6;unsigned q=sceneQueries;Poll(&map);CHECK(adopts==3&&deleted==5&&sceneQueries==q&&!failures);
 modelOption=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q&&!completed&&!failures);modelOption=true;
 enabled=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);enabled=true;
 ready=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);ready=true;
 modelFailed=true;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);modelFailed=false;
 manifest.modelVerified=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);manifest.modelVerified=true;
 modelMapping.generation=6;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);modelMapping.generation=7;
 modelMapping.verified=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q);modelMapping.verified=true;
 sceneOk=false;Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q+1);sceneOk=true;
 nextScene=reinterpret_cast<void*>(9);Pending(GsmCachedTown::Ready,true);Poll(&map);CHECK(adopts==3&&sceneQueries==q+2);nextScene=modelProofScene;
 Pending(GsmCachedTown::Ready,true);ResetEvent(job->thread);GsmAtlas::Job*held=job;Poll(&map);CHECK(job==held&&adopts==3);SetEvent(job->thread);Poll(&map);CHECK(!job&&adopts==4&&!completed&&!failures);
 job=new GsmAtlas::Job();job->ok=false;Poll(&map);CHECK(failures==1&&!job&&!completed);
 job=new GsmAtlas::Job();job->ok=true;Poll(&map);CHECK(completed&&!job&&failures==1);delete completed;completed=NULL;
 CHECK(!terrainInits&&deleted==17);
 Pending(GsmCachedTown::Ready,true);throwDiagnostic=true;bool threw=false;try{Poll(&map);}catch(const std::runtime_error&){threw=true;}throwDiagnostic=false;
 CHECK(threw&&!job&&!completed&&deleted==18); // scoped typed owner survives success-log failure
 Pending(GsmCachedTown::Refused,false);throwError=true;threw=false;try{Poll(&map);}catch(const std::runtime_error&){threw=true;}throwError=false;
 CHECK(threw&&!job&&!completed&&deleted==19); // also failure-log allocation exceptions
 printf("Actual cached completion/Poll %u checks PASS\n",checks);return 0;}
"""
# Deliberately compile the real dispatch together with the real completion body.
code=code.replace('ACTUAL','\n'.join(body(x) for x in ('bool ModelsAvailable()', 'void CachedTownCompletion(const GsmAtlas::Job&j){','void Poll(MapScreen*map)')))
code=code.replace('int main(){MapScreen map;','int main(){manifest.modelVerified=true;MapScreen map;')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,c,passes=True):
 cpp=OUT/(name+'.cpp');cpp.write_text(c,encoding='utf-8')
 r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),f'/Fe{OUT/name}.exe',f'/Fo{OUT/name}.obj'],env=env,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(r.stdout+r.stderr)
 if r.returncode:raise RuntimeError(r.stdout+r.stderr)
 r=subprocess.run([str(OUT/(name+'.exe'))],capture_output=True,text=True)
 (OUT/(name+'-run.log')).write_text(r.stdout+r.stderr)
 if (r.returncode==0)!=passes:raise AssertionError(name+': '+r.stdout+r.stderr)
 if not passes and 'FAIL ' not in r.stdout:raise AssertionError(name+' did not fail a runtime assertion: '+r.stdout+r.stderr)
 print(name,r.stdout.strip())
run('actual_cached_completion',code)
for name,old,new in (
 ('generic_failure_first','if(j->cachedTown){std::auto_ptr<GsmAtlas::Job>owned(j);CachedTownCompletion(*owned);return;}','if(!j->ok){std::string e=j->error;delete j;Fail(map,e);return;}if(j->cachedTown){std::auto_ptr<GsmAtlas::Job>owned(j);CachedTownCompletion(*owned);return;}'),
 ('ignore_generation','j->generation==InterlockedCompareExchange(&GSM::generation,0,0)','true'),
 ('ignore_cancel','!InterlockedCompareExchange(&j->read.cancel,0,0)','true'),
 ('ignore_option','return modelOption&&!modelFailed','return !modelFailed'),
 ('no_cached_dispatch','if(j->cachedTown){std::auto_ptr<GsmAtlas::Job>owned(j);CachedTownCompletion(*owned);return;}',''),
 ('skip_revision_dirty','if(cachedTown.Revision()!=before)models.Invalidate();',''),
 ('omit_scoped_job_owner','if(j->cachedTown){std::auto_ptr<GsmAtlas::Job>owned(j);CachedTownCompletion(*owned);return;}','if(j->cachedTown){CachedTownCompletion(*j);delete j;return;}'),
):
 assert old in code,name
 run(name,code.replace(old,new),False)
print('7 compiled cached completion/Poll mutants rejected')

# Compile the exact selection result/logging branch separately. Native catalog
# acquisition/selection are boundaries; diagnostics can be OFF in this fixture.
a=s.index('            if(FAILED(selected)&&selected!=E_ABORT)',s.index('void ModelState('))
b=s.index('            if(cachedTown.Revision()!=before)',a)
selection=s[a:b]
reset=body('void ResetModels()');assert reset.count('cachedSelectionError.clear();')==1
selectionFixture=r'''
#include <windows.h>
#include <string>
#include <cstdio>
bool debug=false;unsigned errors=0,diagnostics=0,checks=0;std::string cachedSelectionError;
struct Cached {std::string why;const char*LastError(){return why.c_str();}}cachedTown;
void ErrorLog(const std::string&){++errors;}void Diagnostic(const std::string&){if(debug)++diagnostics;}
void Selection(HRESULT selected){__SELECTION__}
void ResetContext(){__RESET__}
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL selection line%u %s\n",__LINE__,#x);return 1;}}while(0)
int main(){cachedTown.why="profile refused";Selection(E_FAIL);CHECK(errors==1&&diagnostics==0&&cachedSelectionError==cachedTown.why);
 Selection(E_FAIL);CHECK(errors==1);cachedTown.why="cleanup refused";Selection(E_FAIL);CHECK(errors==2&&cachedSelectionError==cachedTown.why);
 Selection(E_ABORT);CHECK(errors==2&&cachedSelectionError.empty());Selection(E_FAIL);CHECK(errors==3);
 Selection(S_FALSE);CHECK(errors==3&&cachedSelectionError.empty());Selection(E_FAIL);CHECK(errors==4);
 Selection(S_OK);CHECK(errors==4&&cachedSelectionError.empty());cachedSelectionError="stale context";ResetContext();CHECK(cachedSelectionError.empty());
 Selection(E_FAIL);CHECK(errors==5&&diagnostics==0);
 printf("Actual cached selection logging %u checks PASS (Debug OFF)\n",checks);return 0;}
'''.replace('__RESET__','cachedSelectionError.clear();')
run('actual_selection_logging',selectionFixture.replace('__SELECTION__',selection))
for name,old,new in (
 ('debug_only_selection_errors','ErrorLog(','Diagnostic('),
 ('repeat_same_selection_error','if(cachedSelectionError!=why)','if(true)'),
 ('retain_selection_error_context','}else cachedSelectionError.clear();','}'),
):
 assert selection.count(old)==1,(name,selection.count(old))
 run(name,selectionFixture.replace('__SELECTION__',selection.replace(old,new)),False)
run('forget_reset_selection_context',selectionFixture.replace('__SELECTION__',selection).replace('void ResetContext(){cachedSelectionError.clear();}','void ResetContext(){}'),False)
print('4 compiled actual selection logging/context mutants rejected')
(OUT/'identity.json').write_text(json.dumps(dict(main=hashlib.sha256(s.encode()).hexdigest(),selection=hashlib.sha256(selection.encode()).hexdigest(),poll=hashlib.sha256(body('void Poll(MapScreen*map)').encode()).hexdigest(),completionChecks=22,completionMutants=7,selectionChecks=10,selectionMutants=4),indent=2)+'\n')
