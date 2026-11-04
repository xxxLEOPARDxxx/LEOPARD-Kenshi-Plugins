from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
OUT=Path('D:/DEV/Kenshi/research/gsm-loaded-reuse-20261104');OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 name='reuse-'+opt[1:];exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(HERE/'reuse_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name,p.stdout+p.stderr,end='',flush=True)
 if p.returncode:raise SystemExit(p.returncode)
