from pathlib import Path
import os,subprocess,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
def run(name,cmd):
 p=subprocess.run(cmd,cwd=OUT,env=ENV,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print('PE imports inspected' if 'imports' in name else '\n'.join(x for x in (p.stdout+p.stderr).splitlines() if 'Note: including file:' not in x),flush=True)
 if p.returncode:sys.exit(p.returncode)
 return p
run('prepare',[sys.executable,str(HERE/'prepare_tests.py')])
for mode in ['Od','O2']:
 exe=OUT/f'tests-{mode}.exe';run('build-'+mode,[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/'+mode,'/W3','/showIncludes',str(HERE/'tests.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','advapi32.lib'])
 p=run('imports-'+mode,[str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)]);assert not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
 run('run-'+mode,[str(exe)])
run('native-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/LD','/EHsc','/MD','/O2','/GL','/W3','/DNDEBUG','/DWIN32','/DUNICODE','/D_UNICODE','/showIncludes',str(HERE/'native_compile.cpp'),'/Fo'+str(OUT/'native.obj'),'/Fe'+str(OUT/'native-contract.dll'),'/link','/LTCG','KenshiLib.lib','OgreMain_x64.lib','advapi32.lib'])
run('native-imports',[str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(OUT/'native-contract.dll')])
