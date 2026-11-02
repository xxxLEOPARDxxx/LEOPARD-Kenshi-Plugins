"""Real model-only Job::Run/HashFile against owned copies of the three pins.

No Ogre/game calls or game-file writes. Outputs are isolated from other runners.
"""
from pathlib import Path
import os,re,subprocess

BASE=Path(__file__).resolve().parents[1]
OUT=BASE/'build/model-job-tests'
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
OUT.mkdir(parents=True,exist_ok=True)
ROOT=OUT/'sources'
pins=(BASE/'build/GsmNativeProfile.h').read_text()
pin_section=pins[pins.index('GsmModelShaderInputs[]={'):]
pin_section=pin_section[:pin_section.index('};')]
paths=re.findall(r'\{"([^"\n]+)","[0-9a-f]{64}"\}',pin_section)
assert len(paths)==3,paths
original={p:(GAME/p).read_bytes() for p in paths}
def restore():
    for p,data in original.items():
        target=ROOT/p;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)

fixture=r'''
#ifndef MODEL_JOB_HEADER
#define MODEL_JOB_HEADER "GsmAtlasJob.h"
#endif
#include MODEL_JOB_HEADER
#include <cstdio>
namespace GSM{volatile LONG generation=7;}
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(int argc,char**argv){try{
 C(argc==3,"fixture arguments");const unsigned mode=unsigned(std::atoi(argv[2]));GsmAtlas::Job j;
 j.game=std::string(argv[1])+"/";j.generation=mode==3?6:7;j.modelValidation=true;
 j.manifest.modelVerified=true;j.manifest.header[0][0]=77;
 if(mode==2)j.read.cancel=1;
 if(mode==4)j.validation=true;
 if(mode==6)j.waterValidation=true;
 const DWORD result=GsmAtlas::Run(&j);
 C(j.manifest.header[0][0]==77,"model validation does not replace terrain manifest");
 if(mode==0){C(result==0&&j.ok&&j.manifest.modelVerified&&j.read.readBytes>0,"actual model-only worker validates all generated source pins");C(!j.error[0],"valid source no false error");}
 else{C(result!=0&&!j.ok&&!j.manifest.modelVerified&&j.error[0],"model refusal never leaves stale acceptance");
  if(mode==2||mode==3)C(j.read.cancel&&j.read.readBytes==0,"cancel or stale generation refuses before file IO");
  if(mode==4||mode==6)C(j.read.readBytes==0&&std::strstr(j.error,"ambiguous"),"exclusive validation flags refuse before source IO");
  if(mode==5)C(j.read.readBytes==0,"sixteen MiB model source cap refuses before oversized file IO");
  if(mode==1)C(j.read.readBytes>0&&std::strstr(j.error,"source differs"),"corrupt final source detected by actual SHA");
 }
 std::printf("model worker mode%u: %u checks PASS\n",mode,checks);return 0;
}catch(...){return 1;}}
'''
cpp=OUT/'fixture.cpp';cpp.write_text(fixture)
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/deps/boost_1_60_0;{BASE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def build(name,header=None):
    exe=OUT/(name+'.exe')
    cmd=[VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W3']
    if header:cmd+=['/DMODEL_JOB_HEADER="'+header.as_posix()+'"']
    cmd+=['/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),cpp,'/link','advapi32.lib','user32.lib']
    p=subprocess.run([str(v) for v in cmd],env=env,capture_output=True,text=True)
    (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr)
    assert p.returncode==0,p.stdout+p.stderr
    return exe
def execute(exe,mode,reason=None):
    restore()
    if mode==1:(ROOT/paths[-1]).write_bytes(original[paths[-1]]+b'corrupt')
    if mode==5:(ROOT/paths[0]).write_bytes(b'\0'*(16*1024*1024+1))
    try:p=subprocess.run([str(exe),ROOT.as_posix(),str(mode)],capture_output=True,text=True)
    finally:restore()
    log=p.stdout+p.stderr;(OUT/(exe.stem+'-'+str(mode)+'.log')).write_text(log)
    if reason:
        assert p.returncode and 'FAIL '+reason in log,(exe.stem,log)
        print('Expected RED:',exe.stem,'->',reason)
    else:
        assert p.returncode==0,log
        print(log.strip())

exe=build('actual-model-job')
for mode in range(7):execute(exe,mode)
source=(BASE/'GsmAtlasJob.h').read_text()
cases=[
 ('skip-model-mode','if(job.modelValidation){ValidateModelSources(job);','if(false){ValidateModelSources(job);',0,'actual model-only worker validates all generated source pins'),
 ('allow-mixed-mode','if(unsigned(job.validation)+unsigned(job.waterValidation)+unsigned(job.modelValidation)>1)','if(false)',4,'model refusal never leaves stale acceptance'),
 ('expand-model-cap','16ULL*1024*1024','64ULL*1024*1024',5,'sixteen MiB model source cap refuses before oversized file IO'),
 ('stale-proof',None,None,2,'model refusal never leaves stale acceptance'),
]
for name,old,new,mode,reason in cases:
    if name=='stale-proof':
        text=source.replace('if(job.modelValidation)job.manifest.modelVerified=false;','').replace('if(!job.ok&&job.modelValidation)job.manifest.modelVerified=false;','')
    else:
        assert old in source,name
        text=source.replace(old,new)
    header=OUT/(name+'.h');header.write_text(text)
    execute(build(name,header),mode,reason)
print('PASS seven actual model worker modes and four compiled functional mutants')
