from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def verify():
 for name,want in json.loads((HERE/'dependencies.json').read_text()).items():
  assert hashlib.sha256((HERE.parents[2]/name).read_bytes()).hexdigest()==want,name
 print('PASS unchanged authenticated borrowed closure',flush=True)
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True);(OUT/(name+'.log')).write_bytes(p.stdout+p.stderr);print((p.stdout+p.stderr).decode('utf8','replace'),end='',flush=True);return p.returncode
verify()
if '--cpp-only' not in sys.argv:
 for name in ['generate','test_contract']:
  code=run(name,[sys.executable,str(HERE/(name+'.py'))])
  if code:sys.exit(code)
code=run('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'warp_tests.cpp'),'/Fo'+str(OUT/'warp_tests.obj'),'/Fe'+str(OUT/'warp_tests.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'])
if code:sys.exit(code)
code=run('warp',[str(OUT/'warp_tests.exe')])
if code:sys.exit(code)
if '--skip-controls' not in sys.argv:
 code=run('controls',[sys.executable,str(HERE/'test_controls.py')])
 if code:sys.exit(code)
verify()
