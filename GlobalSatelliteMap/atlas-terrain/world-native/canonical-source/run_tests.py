from pathlib import Path
import os,subprocess,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for mode in ['Od','O2']:
 exe=OUT/f'test-{mode}.exe'
 for label,cmd in [('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3','/showIncludes',str(HERE/'tests.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','advapi32.lib']),('imports',[str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)]),('run',[str(exe)])]:
  p=subprocess.run(cmd,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(label+'-'+mode+'.log')).write_text(p.stdout+p.stderr)
  if label=='imports':assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
  else:print('\n'.join(x for x in (p.stdout+p.stderr).splitlines() if 'Note: including file:' not in x),flush=True)
  if p.returncode:sys.exit(p.returncode)
