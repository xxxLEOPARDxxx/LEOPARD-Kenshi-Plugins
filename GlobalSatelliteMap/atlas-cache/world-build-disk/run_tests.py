from pathlib import Path
import json,os,re,subprocess,sys,uuid
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def inventory(root):
 root=root.absolute();assert root.parent==OUT and re.fullmatch(r'fixtures-[0-9a-f]{32}',root.name)
 for p in [root,*root.parents]:assert not (p.lstat().st_file_attributes&0x400),'Reparse ancestor of cleanup scope'
 result=[]
 def scan(path):
  info=path.lstat();rel=path.relative_to(root).as_posix();attrs=info.st_file_attributes
  if attrs&0x400:
   assert rel in ['linked','unheld','fresh'],'Unknown reparse object in owned fixture'
   target=os.readlink(path).removeprefix('\\\\?\\');assert Path(target).absolute()==root/'target','Unexpected junction target'
   result.append(dict(path=str(path),kind='junction',bytes=0,target=str(target)));return
  if path.is_dir():
   assert rel in ['.','fresh','moved','collision','target','target/escape'],'Unexpected fixture directory'
   result.append(dict(path=str(path),kind='directory',bytes=0))
   for child in sorted(path.iterdir()):scan(child)
  else:
   assert re.fullmatch(r'(?:fresh|moved|collision|target)/tile_[0-9]{1,10}_[0-9]{1,10}\.gst\.partial',rel),'Unknown fixture file'
   assert info.st_size<=4096,'Unexpected large fixture'
   result.append(dict(path=str(path),kind='file',bytes=info.st_size))
 scan(root);return result
def cleanup(root,name):
 entries=inventory(root);manifest=OUT/(name+'-cleanup-'+root.name+'.json');manifest.write_text(json.dumps(dict(scope=str(root),bytes=sum(x['bytes']for x in entries),entries=entries),indent=2)+'\n')
 assert inventory(root)==entries,'Fixture changed after cleanup manifest'
 for item in entries:
  if item['kind']=='file':Path(item['path']).unlink()
 for item in entries:
  if item['kind']=='junction':os.rmdir(item['path'])
 for item in sorted(entries,key=lambda x:len(Path(x['path']).parts),reverse=True):
  if item['kind']=='directory':os.rmdir(item['path'])
 assert not root.exists();print('PASS manifest cleanup',sum(x['bytes']for x in entries),'bytes',flush=True)
def build(name,cpp,opt='/O2'):
 exe=OUT/(name+'.exe');cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc',opt,'/MT','/W3','/DUNICODE','/D_UNICODE','/showIncludes',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib']
 p=subprocess.run(cmd,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);print('\n'.join(x for x in(p.stdout+p.stderr).splitlines()if'Note: including file:'not in x),flush=True)
 if p.returncode:sys.exit(p.returncode)
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(p.stdout+p.stderr);assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi|d3d|dxgi|user32|gdi32)\S*\.dll\s*$',p.stdout),'Unexpected UI/GPU/native import'
 return exe
def execute(name,exe,green=True,reason=None):
 fixture=OUT/('fixtures-'+uuid.uuid4().hex);fixture.mkdir();target=fixture/'target';target.mkdir();link=fixture/'linked';junction=subprocess.run(['cmd.exe','/d','/c','mklink','/J',str(link),str(target)],capture_output=True,text=True);(OUT/(name+'-junction.log')).write_text(junction.stdout+junction.stderr);assert junction.returncode==0,'Actual reparse fixture required'
 p=subprocess.run([str(exe),str(fixture)],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 cleanup(fixture,name)
 if green and p.returncode:sys.exit(p.returncode)
 if not green:assert p.returncode==1 and reason in p.stderr,(name,p.returncode,p.stdout,p.stderr)
def run(opt='/O2'):
 name='tests'+opt[1:];execute(name,build(name,HERE/'tests.cpp',opt))
if __name__=='__main__':
 run('/Od');run('/O2')
 p=subprocess.run([sys.executable,str(HERE/'test_faults.py')],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/'faults.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
