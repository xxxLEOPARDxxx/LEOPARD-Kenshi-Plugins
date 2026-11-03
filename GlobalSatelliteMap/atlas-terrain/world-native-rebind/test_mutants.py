from pathlib import Path
import re,subprocess
import run_tests as runner
HERE=runner.HERE;OUT=runner.OUT/'mutants';OUT.mkdir(exist_ok=True)
def absolute(text,root):return re.sub(r'^\s*#include\s+"([^"\n]+)"',lambda m:'#include "'+(root/m[1]).resolve().as_posix()+'"',text,flags=re.M)
header=absolute((HERE/'NativeRebind.h').read_text(),HERE);test=absolute((HERE/'tests.cpp').read_text(),HERE);flow=absolute((HERE/'flow.cpp').read_text(),HERE)
cases=[('binding-high64','out.profile=binding_;','out.profile=DWORD(binding_);',False,'old same-scope MainToken never reaches freshly bound provider'),('capture-open','std::memset(&out,0,sizeof(out));if(!capturing_','std::memset(&out,0,sizeof(out));if(!capturing_&&provider_)return provider_->Capture(r,out);if(!capturing_',False,'old same-scope source request cannot recapture outside owned transaction'),('idle-admission','!initialized_||rebinding_||bankActive_||cohort_.State()!=WorldCohort::Idle','!initialized_||rebinding_||bankActive_',True,'unreleased CPU/plane view forbids provider rebinding without Pump'),('failed-provider-ready','provider_=NULL;delete fresh;Withdraw();Clear(tokens);','provider_=fresh;ready_=true;Clear(tokens);',False,'missing native source invalidates/deletes old and failed candidate'),('provider-peak','2ULL*sizeof(Owned)','sizeof(Owned)',False,'old/new provider peak over64MiB refuses before worker allocation')]
for name,old,new,use_flow,reason in cases:
 assert header.count(old)==1,(name,header.count(old));h=OUT/(name+'.h');h.write_text(header.replace(old,new));t=OUT/(name+'-tests.cpp');t.write_text(test.replace((HERE/'NativeRebind.h').as_posix(),h.as_posix()));cpp=t
 if use_flow:cpp=OUT/(name+'-flow.cpp');cpp.write_text(flow.replace((HERE/'tests.cpp').as_posix(),t.as_posix()))
 exe=OUT/(name+'.exe');cmd=[str(runner.VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']
 p=subprocess.run(cmd,cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(runner.VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',p.stdout)
 p=subprocess.run([str(exe)],cwd=HERE,env=runner.env,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL '+reason in p.stderr,(name,p.returncode,p.stdout,p.stderr);print('KILLED',name,(p.stdout+p.stderr).strip(),flush=True)
