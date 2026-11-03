from pathlib import Path
import os,re,subprocess,uuid,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ('Od','O2'):
 exe=OUT/('native-'+opt+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,'/DUNICODE','/D_UNICODE',str(HERE/'native.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib'],env=env,capture_output=True,text=True);(OUT/('native-'+opt+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,p.stdout+p.stderr
 root=OUT/('fixtures-'+uuid.uuid4().hex);root.mkdir();p=subprocess.run([str(exe),str(root)],env=env,capture_output=True,text=True);(OUT/('native-'+opt+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
 # Test-generated files only; keep an inventory before known-path cleanup.
 assert root.parent==OUT and re.fullmatch('fixtures-[0-9a-f]{32}',root.name)
 entries=[]
 for f in (root,*root.rglob('*')):
  st=f.lstat();assert not st.st_file_attributes&0x400
  if f.is_file():assert f.parent==root/'normal' and re.fullmatch('fine_[0-9]{3}.gsf',f.name) and st.st_size<=1049600
  else:assert f==root or f==root/'normal'
  entries.append({'path':str(f),'directory':f.is_dir(),'bytes':0 if f.is_dir() else st.st_size})
 (OUT/('native-'+opt+'-cleanup.json')).write_text(json.dumps(entries,indent=2)+'\n')
 for item in reversed(entries):
  f=Path(item['path']);assert not f.lstat().st_file_attributes&0x400
  if item['directory']:f.rmdir()
  else:f.unlink()
 assert not p.returncode,p.stdout+p.stderr
