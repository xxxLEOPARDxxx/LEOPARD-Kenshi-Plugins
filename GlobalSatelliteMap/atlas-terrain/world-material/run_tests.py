from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode,(name,p.returncode)
run('authenticate',[sys.executable,str(HERE/'authenticate.py')])
run('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'plan_tests.cpp'),'/Fo'+str(OUT/'plan_tests.obj'),'/Fe'+str(OUT/'plan_tests.exe')])
run('plan',[str(OUT/'plan_tests.exe')])
run('debug-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/Od','/MT','/W3',str(HERE/'plan_tests.cpp'),'/Fo'+str(OUT/'plan_debug.obj'),'/Fe'+str(OUT/'plan_debug.exe')])
run('debug',[str(OUT/'plan_debug.exe')])
if '--skip-mutants'not in sys.argv:run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
