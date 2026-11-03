from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od','/O2']:
 name='build'+opt[1:];exe=OUT/(name+'.exe')
 for label,args in [('compile',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(HERE/'build_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']),('run',[str(exe)])]:
  if label=='run':
   p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',p.stdout)
  p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
  if p.returncode:sys.exit(p.returncode)
subprocess.run([sys.executable,str(HERE/'test_graceful_error.py')],cwd=HERE,check=True)
