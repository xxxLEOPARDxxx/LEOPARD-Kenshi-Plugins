from pathlib import Path
import re,subprocess,sys
import run_tests as runner
HERE=runner.HERE;OUT=runner.OUT/'faults';OUT.mkdir(exist_ok=True)
def absolute_includes(text,root):return re.sub(r'^\s*#include\s+"([^"\n]+)"',lambda m:'#include "'+(root/m[1]).resolve().as_posix()+'"',text,flags=re.M)
stream=absolute_includes((HERE.parent/'world-stream/WorldStream.h').read_text(),HERE.parent/'world-stream')
prefix='namespace Review {static volatile LONG failWakeAfter=0,failSend=0;static bool forceFence=false;static DWORD Wake(HANDLE h){DWORD result=::WaitForSingleObject(h,INFINITE);return InterlockedExchange(&failWakeAfter,0)?WAIT_FAILED:result;}static BOOL Send(HANDLE h){return InterlockedExchange(&failSend,0)?FALSE:::SetEvent(h);}}\n'
# Separate if statement avoids a token-adjacent C++03 conditional/scope operator.
prefix=prefix.replace('return InterlockedExchange(&failSend,0)?FALSE:::SetEvent(h);','if(InterlockedExchange(&failSend,0))return FALSE;return ::SetEvent(h);')
stream=stream.replace('namespace WorldStream {',prefix+'namespace WorldStream {').replace('WaitForSingleObject(w.wake,INFINITE)','Review::Wake(w.wake)')
stream_path=OUT/'stream.h';stream_path.write_text(stream)
base=absolute_includes((HERE.parent/'world-stream/flow_tests.cpp').read_text(),HERE.parent/'world-stream').replace((HERE.parent/'world-stream/WorldStream.h').as_posix(),stream_path.as_posix())
base_path=OUT/'stream-fixture.cpp';base_path.write_text(base)
header=absolute_includes((HERE/'WorldCohort.h').read_text(),HERE).replace((HERE.parent/'world-stream/WorldStream.h').as_posix(),stream_path.as_posix())
header=header.replace(' explicit Controller(Provider&p)',' Work*ReviewWork(){return work_;}GpuOwners*ReviewGpu(){return gpu_;}HANDLE ReviewThread(){return thread_;}\n explicit Controller(Provider&p)')
header=header.replace('if(SetEvent(work_->wake))return true;','if(Review::Send(work_->wake))return true;').replace('gpu_->context.p->GetData(gpu_->query.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);','(Review::forceFence?S_FALSE:gpu_->context.p->GetData(gpu_->query.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH));').replace('GetTickCount()-start<5000','GetTickCount()-start<25')
header_path=OUT/'cohort.h';header_path.write_text(header)
fixture=absolute_includes((HERE/'flow_tests.cpp').read_text(),HERE).replace((HERE.parent/'world-stream/flow_tests.cpp').as_posix(),base_path.as_posix()).replace((HERE/'WorldCohort.h').as_posix(),header_path.as_posix())
fixture_path=OUT/'cohort-fixture.cpp';fixture_path.write_text(fixture)
code=absolute_includes((HERE/'fault_tests.cpp').read_text(),HERE).replace((HERE/'flow_tests.cpp').as_posix(),fixture_path.as_posix());cpp=OUT/'fault.cpp';cpp.write_text(code);exe=OUT/'fault.exe'
cmd=[str(runner.VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'fault.obj'),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']
p=subprocess.run(cmd,cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/'build.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert p.returncode==0
runner.imports(exe,OUT/'imports.log')
for scenario in ([]if'--mutant'in sys.argv else [sys.argv[2]]if len(sys.argv)==3 and sys.argv[1]=='--only'else ['cancel','lease','opened','decoded','complete','released','send','borrow-budget','quarantine']):
 p=subprocess.run([str(exe),scenario],cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(scenario+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert p.returncode==0,(scenario,p.returncode)
if '--only'in sys.argv:sys.exit(0)
mutants=[('off-result','if(!enabled||!OnMain()||state_!=Complete','if(!OnMain()||state_!=Complete','complete','OFF refuses complete accepted result'),('exit-error-latch','if(exited){error_=FAILED(work_->error)?work_->error:E_FAIL;','if(exited&&state_!=Cancelling){error_=FAILED(work_->error)?work_->error:E_FAIL;','complete','explicit Cancel cannot hide abnormal worker failure'),('quarantine-debt','q.gpuBytes=MaximumGpuReservation();','q.gpuBytes=WorldStream::Controller<Provider>::MaximumGpuReservation();','quarantine','quarantine debt includes all four rasters'),('quarantine-destructor','if(!quarantined_)delete gpu_;','delete gpu_;','quarantine','quarantined destructor releases no GPU owner')]
mutants.append(('raw-borrow-budget','raw>512ULL*512*16||','','borrow-budget','inconsistent raw owner capacity refuses CPU borrow'))
for name,old,new,scenario,reason in mutants:
 if '--mutant'in sys.argv and name!=sys.argv[2]:continue
 assert header.count(old)==1,(name,'mutation match')
 altered=OUT/(name+'.h');altered.write_text(header.replace(old,new));f=OUT/(name+'-fixture.cpp');f.write_text(fixture.replace(header_path.as_posix(),altered.as_posix()));test=OUT/(name+'.cpp');test.write_text(code.replace(fixture_path.as_posix(),f.as_posix()));binary=OUT/(name+'.exe')
 cmd=[str(runner.VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(test),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(binary),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']
 p=subprocess.run(cmd,cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 runner.imports(binary,OUT/(name+'-imports.log'));p=subprocess.run([str(binary),scenario],cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and'FAIL '+reason in p.stderr,(name,p.returncode,p.stdout,p.stderr);print('KILLED',name,(p.stdout+p.stderr).strip(),flush=True)
