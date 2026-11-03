"""Exercise actual private caller functions with CPU-only native-data stand-ins."""
from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'build/world-build-main';OUT.mkdir(parents=True,exist_ok=True)
ROOT=HERE.parents[2]
prefix=r'''
#define _CRT_SECURE_NO_WARNINGS
#include "%s/GlobalSatelliteMap/atlas-terrain/world-cohort/WorldCohort.h"
#include "%s/GlobalSatelliteMap/atlas-terrain/world-layout/WorldLayout.h"
#include "%s/GlobalSatelliteMap/atlas-cache/world-build-disk/WorldBuildDisk.h"
#include "%s/GlobalSatelliteMap/atlas-terrain/compact-material/_generated/Shaders.h"
#include "%s/GlobalSatelliteMap/atlas-terrain/staged-material/_generated/StagedShaders.h"
#include <sstream>
#include <stdexcept>
static unsigned checks=0,caseValue=0,geoCalls=0;
static void C(bool ok,const char*message){++checks;if(!ok){fprintf(stderr,"FAIL %%s\n",message);throw 1;}}
struct FakeBuild{unsigned beginCalls,closeCalls,windows;bool closeOK;SourceReceipts::Scope captured;FakeBuild():beginCalls(0),closeCalls(0),windows(0),closeOK(true){}bool Begin(const SourceReceipts::Scope&s,WorldWindow::U64,const WorldBuildDisk::Config&,const WorldWindow::Ledger&l){++beginCalls;captured=s;return l.other==512*1024;}bool Complete(){++closeCalls;return closeOK;}unsigned Windows()const{return windows;}HRESULT Error()const{return S_OK;}bool Quarantined()const{return false;}};
typedef FakeBuild NativeWorldBuild;
NativeWorldBuild*worldBuild=NULL;SourceReceipts::Scope worldBuildScope={};WorldCohort::Job worldBuildJob;WorldRaster::Material worldBuildMaterial;
unsigned worldBuildMaterialRegion=64,worldBuildCount=0,worldBuildDone=0,worldBuildCase=0,worldFrameEpoch=0;DWORD worldBuildToken=0;LONG worldBuildCapturedLoad=0;volatile LONG worldFrameLoadGeneration=0;
void*ou=reinterpret_cast<void*>(0x1234);std::string worldBuildRoot,worldBuildDirectory,requestIni;bool worldBuildStop=false,cleanup=false;unsigned logs=0,errors=0;
void Log(const std::string&){++logs;}void ErrorLog(const std::string&){++errors;}
std::string Folder(){return "D:/SteamLibrary/steamapps/common/Kenshi/mods/Codex Satellite Capture Probe/";}
struct PrivateGeography{const SatelliteRegion::Region*selectedRegion;float parameters[900],distortion[60];void ParametersRegion(const std::string&,bool fade){C(!fade&&selectedRegion,"caller requests unmodified whole-region parameter snapshot");++geoCalls;memcpy(parameters,CompactMaterial::PinnedParameters,sizeof(parameters));memcpy(distortion,CompactMaterial::PinnedDistortion,sizeof(distortion));}};
unsigned CaseValue(const char*,const char*,unsigned,const char*){return caseValue;}
#define GetPrivateProfileIntA CaseValue
#include "%s/WorldBuildMain.inl"
#undef GetPrivateProfileIntA
int main(){try{
 caseValue=2;bool rejected=false;try{PrepareWorldBuild(9);}catch(...){rejected=true;}C(rejected&&!worldBuild&&!geoCalls,"invalid case refused before constructor/data capture");
 caseValue=0;PrepareWorldBuild(1709);C(worldBuildCount==1&&!worldBuildDone&&!worldBuildStop&&worldBuild->beginCalls==1,"default requests one private cell and fixed owned config");PrepareWorldBuildJob();C(geoCalls==1&&WorldCohort::ValidJob(worldBuildJob)&&worldBuildJob.frame[0].region==27&&worldBuildJob.frame[0].cell==10,"single native caller job has actual valid K4 geometry/profile");
 SourceReceipts::Scope now=WorldBuildNow();C(SourceReceipts::SameScope(now,worldBuildScope),"fresh caller scope accepted");InterlockedIncrement(&worldFrameLoadGeneration);C(!SourceReceipts::SameScope(WorldBuildNow(),worldBuildScope),"same-address save reload invalidates source epoch");InterlockedDecrement(&worldFrameLoadGeneration);ou=reinterpret_cast<void*>(0x100001234ULL);C(!SourceReceipts::SameScope(WorldBuildNow(),worldBuildScope),"full-width world replacement invalidates caller scope");ou=reinterpret_cast<void*>(0x1234);
 caseValue=1;PrepareWorldBuild(1710);geoCalls=0;for(unsigned ordinal=0;ordinal<1024;++ordinal){worldBuildDone=ordinal;PrepareWorldBuildJob();const WorldCohort::Job&j=worldBuildJob;C(j.count==4&&j.scope.job==worldBuildScope.job&&WorldCohort::ValidJob(j),"all1024 real production jobs valid and share capture identity");C(geoCalls==ordinal/16+1,"parameter snapshot cached exactly once per native region");for(unsigned n=0;n<4;++n){unsigned region=ordinal/16,cell=ordinal%%16,x=(region%%8)*8+(cell%%4)*2+n%%2,z=(region/8)*8+(cell/4)*2+n/2;C(j.frame[n].x==-145152+4608*int(x)&&j.frame[n].z==-145152+4608*int(z)&&j.frame[n].region==region&&j.frame[n].cell==cell,"world-region-major placement agrees with independent base-window grid");}}
 C(geoCalls==64,"all world regions captured without per-cell rereads");worldBuildDone=0;worldBuildCount=1;worldBuild->windows=4;logs=errors=0;WorldBuildFinished(1);C(worldBuildDone==1&&worldBuild->closeCalls==1&&!cleanup,"one cohort ack requests graceful close, not atlas publication");WorldBuildFinished(2);C(cleanup&&!errors,"matching closed control reports success");cleanup=false;worldBuild->windows=3;WorldBuildFinished(2);C(cleanup&&errors==1,"mismatched final coverage reports error");
 delete worldBuild;worldBuild=NULL;printf("PASS %%u actual caller-function controls; fake native material values, no game calls\n",checks);return 0;
}catch(...){return 1;}}
'''
text=prefix%tuple([ROOT.as_posix()]*5+[HERE.as_posix()]);cpp=OUT/'caller.cpp';cpp.write_text(text)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 exe=OUT/('caller'+opt[1:]+'.exe')
 for label,cmd in [('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(cpp),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','advapi32.lib']),('run',[str(exe)])]:
  if label=='run':
   p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(exe.stem+'-imports.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',p.stdout)
  p=subprocess.run(cmd,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(exe.stem+'-'+label+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
  assert p.returncode==0,(label,p.returncode)
