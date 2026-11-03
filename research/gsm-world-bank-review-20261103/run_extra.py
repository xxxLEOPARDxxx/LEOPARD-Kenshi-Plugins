from pathlib import Path
import subprocess,os
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
commands=[('build',[str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(here/'extra.cpp'),'/Fo'+str(out/'extra.obj'),'/Fe'+str(out/'extra.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib']),('extra',[str(out/'extra.exe')])]
for name,args in commands:
 p=subprocess.run(args,cwd=root/'GlobalSatelliteMap/atlas-terrain/world-bank',env=env,capture_output=True,text=True)
 (out/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
