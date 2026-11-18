from pathlib import Path
import os,subprocess,re
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text();m=re.search(r'^    bool FungibleInventory\([^\n]+\)\n    \{',s,re.M);a=s.index('{',m.start());b=a+1;n=1
while n:n+=(s[b]=='{')-(s[b]=='}');b+=1
fixture=(here/'engineer_surplus_variant_fixture.cpp').read_text().replace('// @PRODUCTION@',s[m.start():b]);out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ['/Od','/O2']:
 cpp=out/('variant'+opt[1:]+'.cpp');cpp.write_text(fixture);exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True);subprocess.run([str(exe)],check=True)
