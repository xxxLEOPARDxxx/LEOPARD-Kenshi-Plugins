from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
def verify_dependencies():
 root=HERE.parents[2]
 pins=json.loads((HERE/'dependencies.json').read_text())
 for name,want in pins.items():
  assert hashlib.sha256((root/name).read_bytes()).hexdigest()==want,name
 print('PASS',len(pins),'unchanged frozen borrowed files authenticated',flush=True)
verify_dependencies()
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=env,capture_output=True)
 (OUT/(name+'.log')).write_bytes(p.stdout+p.stderr)
 print((p.stdout+p.stderr).decode('utf8','replace'),end='',flush=True)
 return p.returncode
code=run('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'warp_tests.cpp'),'/Fo'+str(OUT/'warp_tests.obj'),'/Fe'+str(OUT/'warp_tests.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'])
if code:sys.exit(code)
code=run('warp',[str(OUT/'warp_tests.exe')])
if code:sys.exit(code)
code=run('faults',[sys.executable,str(HERE/'test_faults.py')])
if code:sys.exit(code)
code=run('runtime',[sys.executable,str(HERE/'test_runtime.py')])
if code:sys.exit(code)
verify_dependencies()
