from pathlib import Path
import os, subprocess, sys
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 name='lifecycle'+opt[1:]
 args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc',opt,'/MT','/W3',str(HERE/'lifecycle_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(OUT/(name+'.exe')),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']
 for label,cmd in [('build',args),('run',[str(OUT/(name+'.exe'))])]:
  p=subprocess.run(cmd,cwd=HERE,env=env,capture_output=True,text=True)
  (OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr)
  print(p.stdout+p.stderr,end='',flush=True)
  if p.returncode:sys.exit(p.returncode)
name='flow';args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'flow_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(OUT/(name+'.exe')),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']
for label,cmd in [('build',args),('run',[str(OUT/(name+'.exe'))])]:
 p=subprocess.run(cmd,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
