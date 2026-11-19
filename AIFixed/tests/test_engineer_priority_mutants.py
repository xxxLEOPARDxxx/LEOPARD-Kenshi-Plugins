"""Compile semantic regressions of the actual headers; each must fail ordinary CHECKs."""
from pathlib import Path
import os,subprocess,shutil,re
here=Path(__file__).resolve().parent;out=here/'_generated/engineer_priority';image=out/'steam_mapped.bin'
assert image.exists(),'run test_engineer_priority_native.py first'
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
policy=(here.parent/'EngineerPriorityPolicy.h').read_text(encoding='utf-8-sig');native=(here.parent/'EngineerPriorityNative.h').read_text(encoding='utf-8-sig')
mutants=[
 ('unknown_order',False,'q.bad||(q.unknown&&!q.started)||','q.bad||'),
 ('reverse_age',False,'ordinal<q.ordinal','ordinal>q.ordinal'),
 ('wall_lane',False,'c.productive&&!c.wall','c.productive'),
 ('positive_caller',True,'caller!=image+0x9A60F3||',''),
 ('placement_caller',True,'(caller==image+0x4D6DE5||placementDepth==1)','true'),
 ('recursive_factory',True,'!factoryDepth&&','true&&'),
 ('shared_builder_scope',True,'||placementDepth==1',''),
 ('townclaim_compat',True,'{0x57C1E0,0x20,0x3ff5030ff1d41cf3ULL}','{0x57C1E0,0x2570,0x778f1cf72006b4c1ULL}'),
 ('recycled_identity',True,'!AIFixedEngineerPriority::Equal(live.key,replacement)||',''),
 ('defer_next_machine',True,'if(frame.round.waiting)','if(false)'),
 ('wall_during_wait',True,'if(!frame.round.hasAlternative)return NULL;','return NULL;'),
 ('stale_election',True,'||generation!=expectedGeneration||','||'),
]
match=re.search(r'    static void Maintenance\(unsigned requested=8\)\s*\{',native);assert match
opening=native.index('{',match.start());end=opening+1;depth=1
while depth:depth+=(native[end]=='{')-(native[end]=='}');end+=1
mutants.append(('maintenance',True,native[opening:end],'{ (void)requested; }'))
for label,is_native,old,new in mutants:
 work=out/('mutant_'+label);work.mkdir(exist_ok=True)
 text=native if is_native else policy;assert text.count(old)==1,(label,text.count(old));changed=text.replace(old,new)
 (work/'EngineerPriorityPolicy.h').write_text(policy if is_native else changed,encoding='utf-8')
 (work/'EngineerPriorityNative.h').write_text(changed if is_native else native,encoding='utf-8')
 env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(work);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
 exe=work/'mutant.exe';fixture=here/('engineer_priority_native_fixture.cpp' if is_native else 'engineer_priority_policy_fixture.cpp')
 build=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(fixture),'/Fe'+str(exe),'/Fo'+str(work)+'/'],cwd=work,env=env,capture_output=True,text=True)
 assert build.returncode==0,(label,build.stdout,build.stderr)
 run=subprocess.run([str(exe),str(image)] if is_native else [str(exe)],capture_output=True,text=True)
 (work/'result.txt').write_text(run.stdout+run.stderr,encoding='utf-8')
 assert run.returncode==1 and 'FAIL ' in run.stdout,(label,run.returncode,run.stdout,run.stderr)
 print('PASS rejected compiled semantic mutant:',label)
print('PASS',len(mutants),'production-header semantic negative controls')
