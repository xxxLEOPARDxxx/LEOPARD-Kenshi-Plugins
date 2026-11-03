from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode,(name,p.returncode)
if not (OUT/'DecodedPins.h').exists():run('prepare',[sys.executable,str(HERE/'generate_pins.py')])
pins=json.loads((OUT/'decoded-pins.json').read_text())
for i in [27,54,91,118,128]:assert hashlib.sha256((OUT/f'rgba-{i}.bin').read_bytes()).hexdigest()==pins['pins'][i]['decodedSha256']
run('build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'warp_tests.cpp'),'/Fo'+str(OUT/'warp_tests.obj'),'/Fe'+str(OUT/'warp_tests.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'])
run('warp',[str(OUT/'warp_tests.exe')])
run('faults',[sys.executable,str(HERE/'test_faults.py')])
if '--skip-mutants'not in sys.argv:run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
