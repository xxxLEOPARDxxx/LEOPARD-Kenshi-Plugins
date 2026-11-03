from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for name,args in [('build',[str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(here/'decode_tests.cpp'),str(here/'cross_pin.cpp'),'/Fo'+str(out)+'/', '/Fe'+str(out/'decode.exe'),'/link','ole32.lib','windowscodecs.lib','advapi32.lib']),('decode',[str(out/'decode.exe')]),('mutants',[sys.executable,str(here/'test_mutants.py')])]:
 p=subprocess.run(args,cwd=here,env=env,capture_output=True,text=True);(out/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
