from pathlib import Path
import os,subprocess,sys
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True);(OUT/(name+'.log')).write_bytes(p.stdout+p.stderr);print((p.stdout+p.stderr).decode('utf8','replace'),end='',flush=True);return p.returncode
for name in ['prepare_profile','build_native']:
 code=run(name,[sys.executable,str(HERE/(name+'.py'))])
 if code:sys.exit(code)
for name in ['tests','abi_tests','listener_tests']:
 code=run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32',str(HERE/(name+'.cpp')),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(OUT/(name+'.exe')),'/link','advapi32.lib'])
 if code:sys.exit(code)
 pe=pefile.PE(str(OUT/(name+'.exe')))
 assert all('ogre' not in d.dll.decode().lower() and 'kenshi' not in d.dll.decode().lower() for d in pe.DIRECTORY_ENTRY_IMPORT),'host tests must not load native game modules'
 code=run(name,[str(OUT/(name+'.exe'))])
 if code:sys.exit(code)
code=run('controls',[sys.executable,str(HERE/'test_controls.py')])
if code:sys.exit(code)

code=run("listener-controls",[sys.executable,str(HERE/"test_listener_controls.py")])
if code:sys.exit(code)
