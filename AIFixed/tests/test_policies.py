from pathlib import Path
import os, subprocess
here=Path(__file__).resolve().parent
out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'policies.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od',str(here/'test_policies.cpp'),'/Fe'+str(exe),'/Fo'+str(out)+'\\'],env=env,cwd=out,check=True)
subprocess.run([str(exe)],check=True)
