from pathlib import Path
import os,subprocess,re
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text()
def block(kind,name):
 m=re.search(r'^    '+kind+' '+re.escape(name)+r'\b',s,re.M);a=s.index('{',m.start());b=a+1;n=1
 while n:n+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[m.start():b+(kind=='struct')]
production=block('struct','Episode')+'\nEpisode episodes[Capacity];\n'+block('struct','Guard')+'\n'+'\n'.join(block(k,n) for k,n in [('bool','ReadToken'),('void','EndTransaction'),('void','Pending'),('void','ClearPending')])+'\nLONG maintenanceGeneration=-1;DWORD maintenanceWindow=0;unsigned maintenanceCursor=0;bool maintenanceStarted=false;\n'+block('void','Maintenance')
fixture=(here/'engineer_surplus_maintenance_fixture.cpp').read_text();out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
def run(label,text,opt,negative=False):
 cpp=out/(label+'.cpp');cpp.write_text(fixture.replace('// @PRODUCTION@',text));exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True);r=subprocess.run([str(exe)],capture_output=True,text=True);print(label,r.stdout);assert r.returncode==(1 if negative else 0),(label,r.returncode,r.stderr)
for opt in ['/Od','/O2']:run('maintenance'+opt[1:],production,opt)
mutants={
 'off_gate':production.replace('if(!worldSafe||!ready||!epoch)return;','if(!worldSafe||!ready||!epoch||!feature||!hasBuilderJob)return;'),
 'unbounded_window':production.replace('if(maintenanceStarted&&now-maintenanceWindow<250)return;',''),
 'unbounded_batch':production.replace('count<4','count<8').replace('Episode batch[4]','Episode batch[8]'),
 'missing_serial':production.replace('Of(pack->getHandle())==e.sourceInventory','true').replace('Of(body->getHandle())==e.sourceInventory','true'),
 'missing_backoff':production.replace('if(e.retryAt&&static_cast<LONG>(now-e.retryAt)<0)continue;',''),
}
for name,text in mutants.items():assert text!=production;run('maintenance_'+name,text,'/O2',True)
