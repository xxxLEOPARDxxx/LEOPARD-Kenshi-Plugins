from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'build';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert p.returncode==0,(name,p.returncode)
run('fixed-prepare',[sys.executable,str(HERE/'prepare_fixed_terrain.py')])
bad=OUT/'fixed-bad/fixed-terrain-inputs';bad.mkdir(parents=True,exist_ok=True);height=bytearray((OUT/'fixed-terrain-inputs/height.u16').read_bytes());height[0]^=1;(bad/'height.u16').write_bytes(height)
for p in (OUT/'fixed-terrain-inputs').iterdir():
 if p.name!='height.u16' and not (bad/p.name).exists():os.link(p,bad/p.name)
run('fixed-tests-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/'fixed_control_tests.cpp'),'/Fo'+str(OUT/'fixed_control_tests.obj'),'/Fe'+str(OUT/'fixed_control_tests.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'])
run('fixed-tests',[str(OUT/'fixed_control_tests.exe')])
