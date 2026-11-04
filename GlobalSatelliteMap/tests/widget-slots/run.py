from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
for opt in ['Od','O2']:
 r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(here/'slots.cpp'),'/Fo'+str(out/'slots.obj'),'/Fe'+str(out/'slots.exe')],cwd=out,env=env,capture_output=True,text=True);(out/('build-'+opt+'.log')).write_text(r.stdout+r.stderr)
 if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
 r=subprocess.run([str(out/'slots.exe')],cwd=out,capture_output=True,text=True);(out/('run-'+opt+'.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='')
 if r.returncode:raise SystemExit(r.returncode)
