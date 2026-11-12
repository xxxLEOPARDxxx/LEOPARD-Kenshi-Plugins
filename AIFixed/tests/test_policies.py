from pathlib import Path
import os, subprocess
import sys
here=Path(__file__).resolve().parent
out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'policies.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od',str(here/'test_policies.cpp'),'/Fe'+str(exe),'/Fo'+str(out)+'\\'],env=env,cwd=out,check=True)
subprocess.run([str(exe)],check=True)
runtime=(here.parent/'UnstuckRuntime.h').read_text(encoding='utf-8')
start=runtime.index('    Building* ReadGuardCycle(')
end=runtime.index('    bool SameIntent(',start)
fixture=(here/'guard_cycle_runtime_fixture.cpp').read_text(encoding='utf-8')
generated=out/'guard_cycle_runtime.cpp'
generated.write_text(fixture.replace('// @PRODUCTION@',runtime[start:end]),encoding='utf-8')
guard_exe=out/'guard_cycle_runtime.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od','/I'+str(here.parent),
    str(generated),'/Fe'+str(guard_exe),'/Fo'+str(out/'guard_cycle_runtime.obj')],env=env,cwd=out,check=True)
subprocess.run([str(guard_exe)],check=True)
subprocess.run([sys.executable,str(here/'test_guard_trace.py')],check=True)
