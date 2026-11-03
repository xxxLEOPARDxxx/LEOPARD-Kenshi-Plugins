from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
if not (OUT/'Cases.h').exists():subprocess.run([sys.executable,str(HERE/'prepare_fixtures.py')],cwd=HERE,check=True)
import hashlib,json
for name,pin in json.loads((OUT/'fixtures.json').read_text())['pins'].items():assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==pin,('fixture/dependency changed',name)
for opt in ['/O2','/Od']:
 name='pixels-'+opt[1:];exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(HERE/'tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
for script in ['test_faults.py']+([]if'--skip-mutants'in sys.argv else['test_mutants.py']):
 p=subprocess.run([sys.executable,str(HERE/script)],cwd=HERE,capture_output=True,text=True);(OUT/(script+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True)
 if p.returncode:sys.exit(p.returncode)
