"""Focused production timing header controls, fake clock and real QPC smoke."""
from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
legacy=not(HERE.parent.parent/'GsmCpuTiming.h').exists()
code=(HERE/'controls.cpp').read_text().replace('// MODE','#define LEGACY' if legacy else '')
cpp=OUT/'controls.cpp';cpp.write_text(code);exe=OUT/'controls.exe'
cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'controls.obj'),'/Fe'+str(exe)]
p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'build.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout+p.stderr);sys.exit(p.returncode)
p=subprocess.run([str(exe)],cwd=OUT,env=env,capture_output=True,text=True);(OUT/('legacy-red.log' if legacy else 'run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');sys.exit(p.returncode)
