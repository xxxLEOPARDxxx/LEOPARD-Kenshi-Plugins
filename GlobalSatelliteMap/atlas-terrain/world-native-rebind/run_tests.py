from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
def run(name,args,execute=False):
 p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print('\n'.join(x for x in(p.stdout+p.stderr).splitlines()if'Note: including file:'not in x),flush=True)
 if p.returncode:sys.exit(p.returncode)
def host(stem='tests',opt='/O2'):
 name=stem+opt[1:];exe=OUT/(name+'.exe');run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,'/W3','/DNDEBUG','/DWIN32','/showIncludes',str(HERE/(stem+'.cpp')),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib'])
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',p.stdout),'Native import in host binary'
 run(name,[str(exe)],True)
def native():
 run('native-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/LD','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32','/showIncludes',str(HERE/'native_compile.cpp'),'/Fo'+str(OUT/'native.obj'),'/Fe'+str(OUT/'native-contract.dll'),'/link','OgreMain_x64.lib','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib'])
 run('native-imports',[str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(OUT/'native-contract.dll')])
if __name__=='__main__':
 if '--native-only'in sys.argv:native();sys.exit(0)
 host()
 if '--all'in sys.argv:
  host('tests','/Od');host('flow')
  native()
  p=subprocess.run([sys.executable,str(HERE/'test_mutants.py')],cwd=HERE,env=env,capture_output=True,text=True);(OUT/'mutants.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
  if p.returncode:sys.exit(p.returncode)
