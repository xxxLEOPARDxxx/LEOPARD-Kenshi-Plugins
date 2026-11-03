from pathlib import Path
import hashlib,os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def imports(exe,log):
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=env,capture_output=True,text=True);log.write_text(p.stdout+p.stderr)
 if p.returncode or re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',p.stdout):raise RuntimeError('Unexpected native host import or PE inspection failure')
def fixture():
 path=OUT/'height-cell.u16';pin='0c918b284a31b95d90a6931e17faadd95277209863bba10069620f16dfdb8c83'
 if not path.exists():
  with Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif').open('rb')as stream:
   data=bytearray()
   for z in range(517):stream.seek(43686+2*((7166+z)*16385+7166));data.extend(stream.read(517*2))
  if hashlib.sha256(data).hexdigest()!=pin:raise RuntimeError('Whole-cell fixture differs from pinned input')
  path.write_bytes(data)
 if hashlib.sha256(path.read_bytes()).hexdigest()!=pin:raise RuntimeError('Changed whole-cell fixture')
def execute(stem,opt='/O2'):
 name=stem+opt[1:];exe=OUT/(name+'.exe')
 for label,cmd in [('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/showIncludes','/EHsc',opt,'/MT','/W3',str(HERE/(stem+'_tests.cpp')),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib']),('run',[str(exe)])]:
  if label=='run':imports(exe,OUT/(name+'-imports.log'))
  p=subprocess.run(cmd,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr);print('\n'.join(x for x in (p.stdout+p.stderr).splitlines()if'Note: including file:'not in x),flush=True)
  if p.returncode:sys.exit(p.returncode)
if __name__=='__main__':
 fixture()
 for opt in ['/Od','/O2']:execute('contract',opt)
 execute('flow')
 p=subprocess.run([sys.executable,str(HERE/'fault_tests.py')],cwd=HERE,env=env,capture_output=True,text=True);(OUT/'faults.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
