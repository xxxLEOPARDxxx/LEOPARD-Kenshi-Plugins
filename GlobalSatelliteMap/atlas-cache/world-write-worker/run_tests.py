from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,cpp,opt='/O2'):
 exe=OUT/(name+'.exe')
 for label,args in [('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib']),('run',[str(exe)])]:
  if label=='run':
   imports=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(imports.stdout+imports.stderr)
   assert imports.returncode==0 and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',imports.stdout),'Unsafe host imports'
  p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-'+label+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
  if p.returncode:sys.exit(p.returncode)
if __name__=='__main__':
 for opt in ['/Od','/O2']:run('worker'+opt[1:],HERE/'worker_tests.cpp',opt)
 subprocess.run([sys.executable,str(HERE/'fault_tests.py')],cwd=HERE,check=True)
