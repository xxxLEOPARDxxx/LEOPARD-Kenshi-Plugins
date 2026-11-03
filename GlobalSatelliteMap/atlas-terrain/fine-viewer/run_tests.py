from pathlib import Path
import os,subprocess,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(label,cmd):
 p=subprocess.run(cmd,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(label+'.log')).write_text(p.stdout+p.stderr);print(''.join(x+'\n' for x in (p.stdout+p.stderr).splitlines() if not x.startswith('Note: including file:')) if 'imports' not in label else 'PE imports checked',end='\n',flush=True);return p
for mode in ['Od','O2']:
 exe=OUT/f'tests-{mode}.exe';p=run('compile-'+mode,[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3','/showIncludes',str(HERE/'tests.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','advapi32.lib'])
 if p.returncode:sys.exit(p.returncode)
 p=run('imports-'+mode,[str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)])
 assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
 p=run('run-'+mode,[str(exe)])
 if p.returncode:sys.exit(p.returncode)
