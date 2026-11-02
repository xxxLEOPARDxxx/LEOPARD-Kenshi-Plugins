"""Mutate only generated review fixtures; production files are read-only."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parents[1]/'build/window-integration-tests'
here.mkdir(parents=True,exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
cases=[
 ('hide-without-ready','fixture.cpp','ancestor>=0&&cache[ancestor].shaded!=0','ancestor>=0'),
 ('never-hide-old-fine','fixture.cpp','return ancestor>=0&&cache[ancestor].shaded!=0;','return false;'),
 ('full-primary-window-bounds','fixture.cpp','double n=AtlasView::Side(key.level)/AtlasView::WindowSide;','double n=1;'),
 ('wrong-parent-source-code','fixture.cpp','key.x>>shift,key.z>>shift','key.x,key.z'),
 ('adopt-stale-zoom','adoption.cpp','plan.Contains(key)&&FindWindow(key)<0','key.Valid()&&FindWindow(key)<0'),
 ('ignore-world-generation','adoption.cpp','j->generation==InterlockedCompareExchange(&GSM::generation,0,0)','true'),
]
for name,file,old,new in cases:
 text=(here/file).read_text();assert text.count(old)==1,(name,text.count(old));text=text.replace(old,new)
 cpp=here/(name+'.cpp');cpp.write_text(text);exe=here/(name+'.exe')
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',f'/Fo{here}/{name}.obj',f'/Fe{exe}',str(cpp)],check=True,env=env,capture_output=True)
 result=subprocess.run([str(exe)],capture_output=True,text=True)
 assert result.returncode!=0 and 'FAIL ' in result.stdout,(name,result.returncode,result.stdout,result.stderr)
 print('REJECTED',name,result.stdout.strip())
print('PASS six independently compiled production-extraction mutation controls')
