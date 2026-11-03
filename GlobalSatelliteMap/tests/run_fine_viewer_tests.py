from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE.parent/'build';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ('Od','O2'):
 name='fine-viewer-'+opt;exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,'/W3',str(HERE/'fine_viewer_gpu.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert p.returncode==0
 p=subprocess.run([str(exe)],env=env,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert p.returncode==0
