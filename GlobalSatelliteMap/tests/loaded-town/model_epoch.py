"""Exercise the actual completed-worker gate with real Win32 event handles."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
source=(here.parents[1]/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig')
a=source.index('void Poll(MapScreen*map){');b=source.index('    if(j->cachedTown)',a)
poll=source[a:b]+'    ++accepted;delete j;\n}\n'
a=source.index('void ResetModels(){')+len('void ResetModels(){');b=source.index('RetireGpuCharge(',a)
advance=source[a:b]
code=r'''
#define NOMINMAX
#include <Windows.h>
#include <stdexcept>
#include <cstdio>
static unsigned checks=0,accepted=0,deleted=0;void C(bool v,const char*s){++checks;if(!v){std::printf("FAIL %s\n",s);throw 1;}}
namespace GSM{volatile LONG generation=7;}
namespace GsmAtlas{struct Job{HANDLE thread;LONG generation;unsigned modelEpoch;bool modelValidation,instanceValidation,cachedTown;struct{volatile LONG cancel;}read;Job():thread(CreateEvent(NULL,TRUE,TRUE,NULL)),generation(7),modelEpoch(1),modelValidation(false),instanceValidation(false),cachedTown(false){read.cancel=0;}~Job(){++deleted;}};}
struct MapScreen{};GsmAtlas::Job*job=NULL;bool jobQuarantined=false,modelOption=true;unsigned modelEpoch=1;
POLL
void ResetEpoch(){ADVANCE}
void Run(unsigned kind,unsigned epoch,bool on,bool expect,LONG world=7,bool cancel=false){
 modelOption=on;unsigned before=accepted,freed=deleted;job=new GsmAtlas::Job;job->modelEpoch=epoch;job->generation=world;job->read.cancel=cancel;job->modelValidation=kind==1;job->instanceValidation=kind==2;job->cachedTown=kind==3;HANDLE old=job->thread;
 Poll(NULL);C(job==NULL&&deleted==freed+1,"completion is consumed exactly once");C(accepted==before+(expect?1:0),"only current enabled model jobs reach result handlers");DWORD flags=0;C(!GetHandleInformation(old,&flags)&&GetLastError()==ERROR_INVALID_HANDLE,"completed native handle is closed");
}
int main(){try{
 ResetEpoch();C(modelEpoch==2,"model reset advances option epoch");Run(2,1,true,false);Run(1,1,true,false);Run(3,1,true,false);Run(2,2,false,false);Run(2,2,true,true);Run(1,2,true,true);Run(0,1,false,true);Run(2,2,true,false,6);Run(2,2,true,false,7,true);
 modelEpoch=~0u;ResetEpoch();C(modelEpoch==1,"nonzero epoch after wrap");Run(2,~0u,true,false);Run(2,1,true,true);
 std::printf("PASS %u actual model-worker completion controls\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('POLL',poll).replace('ADVANCE',advance)
(out/'epoch.cpp').write_text(code)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
for opt in ['Od','O2']:
    r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(out/'epoch.cpp'),'/Fo'+str(out/'epoch.obj'),'/Fe'+str(out/'epoch.exe')],env=env,cwd=out,capture_output=True,text=True)
    if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
    r=subprocess.run([str(out/'epoch.exe')],cwd=out,capture_output=True,text=True)
    (out/('epoch-'+opt+'.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='')
    if r.returncode:raise SystemExit(r.returncode)
