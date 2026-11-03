from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'build';GS=HERE.parents[2]/'GlobalSatelliteMap';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'FixedTerrainControl.h').read_text();test=(HERE/'fixed_control_tests.cpp').read_text()
def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def run(name,header,body=test,green=False):
 hp=OUT/(name+'.h');hp.write_text(header.replace('"../../../GlobalSatelliteMap/','"'+GS.as_posix()+'/'))
 body=body.replace('"FixedTerrainControl.h"','"'+hp.as_posix()+'"').replace('"../../../GlobalSatelliteMap/','"'+GS.as_posix()+'/');cpp=OUT/(name+'.cpp');cpp.write_text(body);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=HERE,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (p.returncode==0 if green else p.returncode==1 and 'FAIL ' in p.stderr),(name,p.returncode,p.stdout,p.stderr);print(('PASS fault 'if green else 'Expected RED ')+name+': '+(p.stdout+p.stderr).strip(),flush=True)
for name,a,b in [
 ('fixed-mut-guard','return startupOff&&currentOff&&texturesAbsent&&world&&map&&saveIdle&&profile&&context;','return true;'),
 ('fixed-mut-hash','!std::memcmp(actual,pin,32)','true'),
 ('fixed-mut-epoch','h.epoch=epoch_;','h.epoch=0;'),
 ('fixed-mut-case','spacing[4]={1,3,9,3}','spacing[4]={3,3,9,3}'),
 ('fixed-mut-predicate','CompactMaterial::SavedPredicate predicate(c.p);',''),
 ('fixed-mut-overwrite','MoveFileExA(path.c_str(),(j.output+name).c_str(),0)','MoveFileExA(path.c_str(),(j.output+name).c_str(),MOVEFILE_REPLACE_EXISTING)'),
 ('fixed-mut-cancel','InterlockedExchange(&job_->cancel,1);',''),
 ('fixed-mut-charge','StageBytes=524288','StageBytes=262144'),
]:run(name,edit(source,a,b))
prefix=test[:test.index('int main()')]
fault=prefix+r'''
static ULONG Refs(IUnknown*p){ULONG n=p->AddRef();p->Release();return n;}
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;C(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p)),"owned fault WARP");ULONG before=Refs(d.p);Controller controller;controller.Begin(d.p,"build/",81,9,true);std::string output;C(Finish(controller,c.p,output)!=0,"failed staging allocation cannot complete");C(before==Refs(d.p),"failed controller releases all GPU/device refs after join");C(GetFileAttributesA((output+"complete.txt").c_str())==INVALID_FILE_ATTRIBUTES,"allocation refusal cannot publish completion");std::puts("PASS exact staging failure/controller cleanup");return 0;}catch(...){return 1;}}
'''
for index in range(2):
 mutated=edit(source,'Hr(device_.p->CreateTexture2D(&td,NULL,&stage_[i].p),"fixed staging allocation")','Hr(i==%d?E_OUTOFMEMORY:device_.p->CreateTexture2D(&td,NULL,&stage_[i].p),"fixed staging allocation")'%index)
 run('fixed-fault-stage'+str(index),mutated,fault,True)
 if index==1:run('fixed-mut-partial-cleanup',edit(mutated,'stage_[0].Reset();stage_[1].Reset();','stage_[1].Reset();'),fault)
waitFault=edit(source,'DWORD wait=WaitForSingleObject(thread_,0);','DWORD wait=(SetLastError(ERROR_INVALID_HANDLE),WAIT_FAILED);')
waitTest=prefix+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;C(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p)),"owned wait-fault WARP");Controller controller;controller.Begin(d.p,"build/",82,10,true);DWORD error=0;std::string output;C(controller.Poll(error,output)&&error==ERROR_INVALID_HANDLE&&controller.Active()&&controller.Quarantined()&&controller.Stopping(),"unknown worker identity quarantines one bounded job and emits one error");C(!controller.Poll(error,output),"quarantine never repeats error or touches unknown handle");bool caught=false;try{controller.Begin(d.p,"build/",83,11,true);}catch(...){caught=true;}C(caught,"quarantine cannot create replacement job");std::puts("PASS actual Poll WAIT_FAILED quarantines once without deleting worker");return 0;}catch(...){return 1;}}
'''
run('fixed-fault-wait',waitFault,waitTest,True)
run('fixed-mut-quarantine',edit(waitFault,'quarantined_=true;','quarantined_=false;'),waitTest)
print('PASS10 compiled runtime worker/control mutants +2 actual staging failures +WAIT_FAILED quarantine')
