from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 name='layout'+opt[1:];exe=OUT/(name+'.exe')
 for label,args in [('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(HERE/'layout_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)]),('run',[str(exe)])]:
  p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
  if p.returncode:sys.exit(p.returncode)
