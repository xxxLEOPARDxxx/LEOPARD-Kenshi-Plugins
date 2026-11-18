from pathlib import Path
import os,subprocess,re
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text()
def block(kind,name):
 m=re.search(r'^    '+kind+' '+re.escape(name)+r'\b',s,re.M);start=s.index('{',m.start());end=start+1;n=1
 while n:n+=(s[end]=='{')-(s[end]=='}');end+=1
 if kind=='struct':end+=1
 return s[m.start():end]
production=block('struct','Episode')+'\nEpisode episodes[Capacity];\n'+block('struct','Guard')+'\n'+'\n'.join(block(kind,name) for kind,name in [('bool','ReadActor'),('bool','ReadFill'),('bool','ReadToken'),('void','Retire'),('void','Cancel'),('void','Invalidate'),('void\\*','FillDeleteHook'),('bool','BeginTransaction'),('void','EndTransaction'),('void','Pending'),('void','ClearPending')])
fixture=(here/'engineer_surplus_lifetime_fixture.cpp').read_text();out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ['/Od','/O2']:
 cpp=out/('lifetime'+opt[1:]+'.cpp');cpp.write_text(fixture.replace('// @PRODUCTION@',production));exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True);subprocess.run([str(exe)],check=True)
