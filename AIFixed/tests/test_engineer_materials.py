from pathlib import Path
import os, subprocess, json
here=Path(__file__).resolve().parent
out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
for opt in ('Od','O2'):
    exe=out/('engineer_materials_'+opt+'.exe')
    subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/'+opt,
        str(here/'engineer_material_policy.cpp'),'/Fe'+str(exe),'/Fo'+str(out/('engineer_materials_'+opt+'.obj'))],env=env,cwd=out,check=True)
    subprocess.run([str(exe)],check=True)
    runtime=(here.parent/'EngineerMaterialsRuntime.h').read_text()
    start=runtime.index('namespace EngineerMaterialsRuntime\n{')
    end=runtime.index('    // Native contract table and Install')
    production=runtime[start:end]+'\n}\n'
    a=runtime.index('    static bool WindowMatches(');b=runtime.index('    static bool NativeAdmission()',a)
    production+='namespace EngineerMaterialsRuntime { struct NativeWindow{unsigned rva,size;unsigned __int64 fnv;};\n'+runtime[a:b]+'}\n'
    # Only the hardware return-address boundary is injected; scope, epoch,
    # eligibility, cache, reservation and transfer code are the actual header.
    production=production.replace('_ReturnAddress()','FixtureReturnAddress()')
    production=production.replace('EnterCriticalSection(&mutex)','FixtureEnter(&mutex)')
    cpp=out/'engineer_material_runtime.cpp'
    window_tests=[]
    proof=json.loads((here/'engineer_material_native_contracts.json').read_text())
    for n,row in enumerate(proof['windows']):
        raw=bytes.fromhex(row['bytes']);size=len(raw)
        change='bytes[0]^=0x80' if raw[0]==0xE9 else 'bytes[0]=0xE9'
        window_tests.append('{ using namespace EngineerMaterialsRuntime; unsigned char bytes[]={%s}; NativeWindow w={0x%X,%d,0x%016Xui64}; CHECK(WindowMatches(w,bytes)); %s; CHECK(WindowMatches(w,bytes)==%s); bytes[%d]^=1; CHECK(!WindowMatches(w,bytes)); }'%(
            ','.join('0x%02X'%x for x in raw),row['rva'],size,int(row['fnv'],16),
            change,'true' if row['rva'] in (0x5C6290,0x998190,0x5C7F40) else 'false',size-1))
    fixture=(here/'engineer_material_runtime_fixture.cpp').read_text().replace('// @PRODUCTION@',production).replace('// @WINDOWTESTS@','\n'.join(window_tests))
    cpp.write_text(fixture)
    exe=out/('engineer_material_runtime_'+opt+'.exe')
    subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/'+opt,'/I'+str(here),
        str(cpp),'/Fe'+str(exe),'/Fo'+str(out/('engineer_material_runtime_'+opt+'.obj'))],env=env,cwd=out,check=True)
    subprocess.run([str(exe)],check=True)

mutations={
    'stale_source_cache':('self->resourceSourceCache.erase(it);','(void)0;'),
    'no_commit':('else if(!planned||!Commit(r))DenyMove(system,project,collection,e);',
                 'else if(!planned)DenyMove(system,project,collection,e);'),
    'tiny_batch':('int quantity=Grant(r);','int quantity=2;'),
    'unreserved_fallback':('if(scope->epoch!=Epoch()||!scope->hasRequest||!Matches(material,scope->request.material)||!Has(scope->request))return NULL;',
                          'if(!scope->hasRequest)return originalTake(inventory,material);'),
    'unowned_credit':('coordinator.Acquired(scope->request,moved,before,after);','(void)0;'),
    'no_destination_claim':('return hasNeed;','return impossible;'),
    'reject_shared_entry':('if(hash==w.fnv)return true;','if(hash==w.fnv)return true;return false;'),
    'no_lock_epoch_fence':('if(epoch!=Epoch())return false;','if(false)return false;'),
}
for name,(old,new) in mutations.items():
    assert fixture.count(old)==1,(name,fixture.count(old))
    cpp=out/('engineer_material_mutant_'+name+'.cpp');cpp.write_text(fixture.replace(old,new))
    exe=out/('engineer_material_mutant_'+name+'.exe')
    subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/I'+str(here),
        str(cpp),'/Fe'+str(exe),'/Fo'+str(out/('engineer_material_mutant_'+name+'.obj'))],env=env,cwd=out,check=True,stdout=subprocess.DEVNULL)
    result=subprocess.run([str(exe)],capture_output=True,text=True)
    assert result.returncode==1 and 'CHECK line' in result.stderr,(name,result.returncode,result.stderr)
    print('ordinary CHECK rejected mutant:',name)
policy=(here.parent/'EngineerMaterialsPolicy.h').read_text()
old='claims[c].sourceUnits=Sub(claims[c].sourceUnits,moved);'
assert policy.count(old)==1
cpp=out/'engineer_material_mutant_early_release.cpp'
cpp.write_text(fixture.replace('#include "../EngineerMaterialsPolicy.h"',policy.replace(old,'claims[c].sourceUnits=0;')))
exe=out/'engineer_material_mutant_early_release.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/I'+str(here),str(cpp),
    '/Fe'+str(exe),'/Fo'+str(out/'engineer_material_mutant_early_release.obj')],env=env,cwd=out,check=True,stdout=subprocess.DEVNULL)
result=subprocess.run([str(exe)],capture_output=True,text=True)
assert result.returncode==1 and 'CHECK line' in result.stderr,(result.returncode,result.stderr)
print('ordinary CHECK rejected mutant: early source release')
