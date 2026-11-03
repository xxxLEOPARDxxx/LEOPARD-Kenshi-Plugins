from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;out=here/'_generated/listener-controls';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
base=(here/'MainGate.h').read_text()
mutants=[('listener64','a.listener==b.listener','DWORD(a.listener)==DWORD(b.listener)'),('semantics','a.semantics==b.semantics','true'),('unknown-mode','semantics==OwnedRawIndexedFile||','true||'),('borrowed-evidence','semantics==OwnedRawIndexedFile&&ownedNamedFile','semantics==OwnedRawIndexedFile')]
for name,old,new in [('O2','',''),('Od','','')]+mutants:
 folder=out/name;folder.mkdir(exist_ok=True)
 for file in ['ReceiptPolicy.h','WorkerFileLease.h','MainGate.h','listener_tests.cpp']:
  text=(here/file).read_text()
  if file=='MainGate.h' and old:
   assert text.count(old)==1,(name,text.count(old));text=text.replace(old,new)
  (folder/file).write_text(text)
 exe=folder/'test.exe';p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od'if name=='Od'else'/O2',str(folder/'listener_tests.cpp'),'/Fo'+str(folder/'test.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=here,env=env,capture_output=True,text=True);(folder/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr)
 assert (p.returncode==1 and 'FAIL 'in p.stderr)if old else p.returncode==0,(name,p.returncode,p.stdout,p.stderr)
 print(name,(p.stdout+p.stderr).strip())
