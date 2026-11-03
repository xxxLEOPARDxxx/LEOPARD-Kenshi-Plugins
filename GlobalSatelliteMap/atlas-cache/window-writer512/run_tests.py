from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'writer_tests.cpp'),'/Fo'+str(OUT/'writer.obj'),'/Fe'+str(OUT/'writer.exe')]
for name,command in [('build',args),('writer',[str(OUT/'writer.exe')])]:
 p=subprocess.run(command,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:raise SystemExit(p.returncode)
if '--skip-mutants' not in sys.argv:
 p=subprocess.run([sys.executable,str(HERE/'test_mutants.py')],cwd=HERE,capture_output=True,text=True);(OUT/'mutants.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:raise SystemExit(p.returncode)
