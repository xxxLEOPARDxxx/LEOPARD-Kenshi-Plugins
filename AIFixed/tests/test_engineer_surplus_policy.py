from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ['/Od','/O2']:
 exe=out/('policy'+opt[1:]+'.exe')
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(here/'engineer_surplus_policy_fixture.cpp'),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True)
 subprocess.run([str(exe)],check=True)
