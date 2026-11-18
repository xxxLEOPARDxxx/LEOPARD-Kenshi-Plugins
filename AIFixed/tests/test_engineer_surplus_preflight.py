from pathlib import Path
import os,subprocess,re
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text()
def block(kind,name):
 m=re.search(r'^    '+kind+' '+re.escape(name)+r'\b[^\n;]*\s*\{',s,re.M);a=s.index('{',m.start());b=a+1;n=1
 while n:n+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[m.start():b+(kind=='struct')]
production='\n'.join(block(k,n) for k,n in [('struct','Episode')])+'\nEpisode episodes[Capacity];\n'+block('struct','Guard')+'\n'+'\n'.join(block(k,n) for k,n in [('bool','ReadActor'),('void','Retire'),('struct','SearchSlot')])+'\nSearchSlot searchSlots[Capacity];LONG searchGeneration=-1;DWORD searchWindow=0;unsigned searchCount=0;\n'+block('bool','SearchTicket')+'\n'+block('bool','FungibleInventory')+'\n'+block('bool','CargoReady')+'\n'+block(r'GameData\*','DataIn')+'\n'+block('bool','Prepare')
fixture=(here/'engineer_surplus_preflight_fixture.cpp').read_text();out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
def run(label,text,opt,negative=False):
 cpp=out/(label+'.cpp');cpp.write_text(fixture.replace('// @PRODUCTION@',text));exe=cpp.with_suffix('.exe');subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/'],cwd=out,env=env,check=True);r=subprocess.run([str(exe)],capture_output=True,text=True);print(label,r.stdout);assert r.returncode==(1 if negative else 0),(label,r.returncode,r.stderr)
for opt in ['/Od','/O2']:run('preflight'+opt[1:],production,opt)
mutants={
 'late_guard':production.replace('||!CargoReady(c,data,e)',''),
 'no_budget':production.replace('!mineCount||!SearchTicket(actor,e,GetTickCount(),materialTurn,storageCursor)','!mineCount'),
}
for name,text in mutants.items():assert text!=production;run('preflight_'+name,text,'/O2',True)
