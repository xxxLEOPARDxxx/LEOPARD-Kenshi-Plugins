from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for mode in ('Od','O2'):
 exe=OUT/(mode+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,str(HERE/'tests.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,capture_output=True,text=True);(OUT/(mode+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,p.stdout+p.stderr
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(mode+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode,p.stdout+p.stderr
