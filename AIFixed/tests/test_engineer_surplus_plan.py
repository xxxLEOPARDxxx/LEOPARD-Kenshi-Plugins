from pathlib import Path
import os,subprocess,re
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text()
def block(kind,name):
 m=re.search(r'^    '+kind+' '+name+r'\b',s,re.M);a=s.index('{',m.start());b=a+1;n=1
 while n:n+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[m.start():b+(kind=='struct')]
production=block('bool','SetTaskHook');budget=s.split('// @SURPLUS_SEARCH_BUDGET_BEGIN@')[1].split('// @SURPLUS_SEARCH_BUDGET_END@')[0];types=block('struct','SearchSlot')+'\nSearchSlot searchSlots[Capacity];LONG searchGeneration=-1;DWORD searchWindow=0;unsigned searchCount=0;\n'
fixture=(here/'engineer_surplus_plan_fixture.cpp').read_text().replace('// @SET_TASK@',production).replace('// @BUDGET_TYPES@',types).replace('// @BUDGET@',budget);out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ['/Od','/O2']:
 cpp=out/('plan'+opt[1:]+'.cpp');cpp.write_text(fixture);exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True);subprocess.run([str(exe)],check=True)

mutants={
 'untagged_fill':fixture.replace('*reinterpret_cast<ULONG_PTR*>(fills[i])=OwnedFillVtable();','(void)fills[i];'),
 'skip_before_start_gate':fixture.replace('if(!planningToken||planningSystem!=self)return originalSetTask(self,from);','return originalSetTask(self,from);'),
 'unbudgeted_queries':fixture.replace('if(searchCount>=4)return false;','if(searchCount>=4000)return false;'),
 'no_actor_backoff':fixture.replace('if(slot&&static_cast<LONG>(now-slot->next)<0)return false;','if(false)return false;'),
}
for name,text in mutants.items():
 assert text!=fixture
 cpp=out/('plan_mutant_'+name+'.cpp');cpp.write_text(text);exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True)
 r=subprocess.run([str(exe)],capture_output=True,text=True);assert r.returncode==1 and 'FAIL ' in r.stdout,(name,r.returncode,r.stdout);print(name,'ordinary CHECK rejection verified')
