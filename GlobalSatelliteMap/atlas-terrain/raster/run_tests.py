from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
compact=HERE.parent/'compact-material'
contract=json.loads((compact/'_generated/contract.json').read_text())
assert hashlib.sha256((compact/'_generated/contract.json').read_bytes()).hexdigest()=='afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9'
pins=json.loads((compact/'verification.json').read_text())
for name in ['Case.h','Shaders.h','height.u16']+[f'{case}-gbuffer{plane}.bin' for case in ['control3','control9','shift3','control1'] for plane in range(3)]:
 assert hashlib.sha256((compact/'_generated'/name).read_bytes()).hexdigest()==pins['artifacts'][name],name
for stage in contract['shaders']:
 assert stage['constantBytes']=={'Vertex':448,'Pixel':3856}[stage['name']]
 assert hashlib.sha256((compact/'_generated'/(stage['name']+'.cso')).read_bytes()).hexdigest()==stage['sha256']
print('PASS frozen compact shader ABI/default parameters/source geometry/manual controls authenticated',flush=True)
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
if '--skip-mutants' not in sys.argv:sys.exit(run('mutants',[sys.executable,str(HERE/'test_mutants.py')]))
