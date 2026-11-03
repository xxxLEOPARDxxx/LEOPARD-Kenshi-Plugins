from pathlib import Path
import datetime,json,os, subprocess, sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
commands=[]
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(name=name,args=args,exitCode=p.returncode));print('\n'.join(line for line in (p.stdout+p.stderr).splitlines()if'Note: including file:'not in line),flush=True)
 if p.returncode:sys.exit(p.returncode)
run('dependencies-before',[sys.executable,str(HERE/'freeze.py'),'--snapshot'])
for opt in ['/Od','/O2']:
 name='host'+opt[1:]
 run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,'/W3','/DNDEBUG','/DWIN32',str(HERE/'tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(OUT/(name+'.exe')),'/link','advapi32.lib'])
 sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
 import pefile
 pe=pefile.PE(str(OUT/(name+'.exe')))
 assert all('ogre' not in d.dll.decode().lower() and 'kenshi' not in d.dll.decode().lower() for d in pe.DIRECTORY_ENTRY_IMPORT),'host must not load game modules'
 run(name,[str(OUT/(name+'.exe'))])
if '--host-only' not in sys.argv:
 run('native-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/LD','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32','/showIncludes',str(HERE/'native_compile.cpp'),'/Fo'+str(OUT/'native.obj'),'/Fe'+str(OUT/'native-contract.dll'),'/link','OgreMain_x64.lib','advapi32.lib'])
if '--skip-mutants' not in sys.argv:run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
run('dependencies-after',[sys.executable,str(HERE/'freeze.py'),'--check-snapshot'])
(OUT/'run-session.json').write_text(json.dumps(dict(completedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),python=sys.executable,fullRun='--host-only'not in sys.argv and'--skip-mutants'not in sys.argv,commands=commands),indent=2)+'\n')
