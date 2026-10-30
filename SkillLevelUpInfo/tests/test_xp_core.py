from pathlib import Path
import os,subprocess
p=Path(__file__).resolve().parent
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
out=p/'_generated';out.mkdir(exist_ok=True)
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',p.parent]))
env['LIB']=';'.join(map(str,[vc/'lib/amd64',sdk/'Lib/x64']))
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(p/'xp_core_fixture.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'xp_core.exe')],env=env,cwd=out,check=True)
subprocess.run([str(out/'xp_core.exe')],env=env,check=True)
