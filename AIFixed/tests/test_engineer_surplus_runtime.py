from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;source=(here.parent/'EngineerSurplusRuntime.h').read_text()
production=source.split('// @SURPLUS_NATIVE_TRANSFER_BEGIN@')[1].split('// @SURPLUS_NATIVE_TRANSFER_END@')[0]
fixture=(here/'engineer_surplus_runtime_fixture.cpp').read_text();out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
def run(label,text,opt,negative=False):
 cpp=out/(label+'.cpp');cpp.write_text(fixture.replace('// @PRODUCTION@',text));exe=out/(label+'.exe')
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'/', '/link','kernel32.lib'],cwd=out,env=env,check=True)
 r=subprocess.run([str(exe)],capture_output=True,text=True);print(label,r.stdout)
 assert r.returncode==(1 if negative else 0),(label,r.returncode,r.stderr)
 if negative:assert 'FAIL ' in r.stdout
for opt in ['/Od','/O2']:run('runtime'+opt[1:],production,opt)
mutants={
 'whole_stack':production.replace('removeItemDontDestroy_returnsItem(item,1,true)','removeItemDontDestroy_returnsItem(item,item->quantity,true)'),
 'drop_rollback':production.replace('source->addItem(piece,1,false,false)','source->addItem(piece,1,true,true)'),
 'false_receipt':production.replace('return source->addItem(piece,1,false,false);','source->addItem(piece,1,false,false);Commit();return true;'),
 'foreign_release':production.replace('if(op.reserved)EndTransaction','EndTransaction'),
}
for name,text in mutants.items():assert text!=production;run('mutant_'+name,text,'/O2',True)
