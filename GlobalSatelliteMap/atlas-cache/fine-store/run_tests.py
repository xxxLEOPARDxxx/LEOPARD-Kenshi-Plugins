from pathlib import Path
import json,os,re,subprocess,sys,uuid
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def cleanup(root,name):
    root=root.absolute();assert root.parent==OUT and re.fullmatch('fixtures-[0-9a-f]{32}',root.name)
    for p in (root,*root.parents):assert not p.lstat().st_file_attributes&0x400
    entries=[]
    for p in sorted(root.rglob('*')):
        a=p.lstat();assert not a.st_file_attributes&0x400
        if p.is_dir():assert p.parent==root and (p.name in ('cache','main-refuse') or re.fullmatch('fault_[1-8]',p.name))
        else:assert p.parent.parent==root and (p.parent.name=='cache' or re.fullmatch('fault_[1-8]',p.parent.name)) and re.fullmatch('fine_[0-9]{3}.gsf',p.name) and a.st_size<=1049600
        entries.append(dict(path=str(p),directory=p.is_dir(),bytes=0 if p.is_dir() else a.st_size))
    (OUT/(name+'-cleanup.json')).write_text(json.dumps(dict(root=str(root),entries=entries),indent=2)+'\n')
    for x in entries:
        p=Path(x['path']);assert not p.lstat().st_file_attributes&0x400
        if not x['directory']:p.unlink()
    for x in reversed(entries):
        if x['directory']:Path(x['path']).rmdir()
    root.rmdir()
def build(opt,source='tests.cpp'):
    name='fine-store-'+Path(source).stem+'-'+opt[1:];exe=OUT/(name+'.exe')
    p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3','/DUNICODE','/D_UNICODE',str(HERE/source),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=ENV,capture_output=True,text=True)
    (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode
    p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],env=ENV,capture_output=True,text=True);(OUT/(name+'-imports.log')).write_text(p.stdout+p.stderr);assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi|d3d|dxgi|user32|gdi32)\S*\.dll\s*$',p.stdout)
    root=OUT/('fixtures-'+uuid.uuid4().hex);root.mkdir()
    p=subprocess.run([str(exe),str(root)],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
    cleanup(root,name);assert not p.returncode
if __name__=='__main__':
    build('/Od');build('/O2');build('/Od','faults.cpp');build('/O2','faults.cpp')
