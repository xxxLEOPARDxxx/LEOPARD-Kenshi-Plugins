from pathlib import Path
import subprocess,os,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{HERE}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
source=(HERE/'CensusRoute.h').read_text(encoding='utf-8-sig');test=(HERE/'route_tests.cpp').read_text(encoding='utf-8-sig').replace('"CensusRoute.h"','"mutant_route.h"')
mutations=[('entity cap','r.selected>=EntityLimit','false'),('subentity cap','!count||count>SubLimit','false'),('material cap','r.materialCount>MaterialLimit||count>MaterialLimit-r.materialCount','false'),('OFF queries','if(!enabled)return S_FALSE;','if(false)return S_FALSE;'),('unverified query','!verified||!generation','!generation'),('zero generation','!verified||!generation','!verified'),('key cap','keys>EntryLimit','false'),('floor count cap','count>EntryLimit)return E_INVALIDARG','false)return E_INVALIDARG'),('total entry cap','next.visited==EntryLimit','next.visited==EntryLimit+1'),('publish stale','if(!ops.End(chosen))','if(false)'),('array capacity','capacity<count','false'),('array alignment','pointer%8==0','true'),('name controls','static_cast<unsigned char>(name[i])<32||name[i]==127','false')]
for name,old,new in mutations:
 assert old in source,name
 (OUT/'mutant_route.h').write_text(source.replace(old,new,1));(OUT/'mutant_route.cpp').write_text(test)
 r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/W3','/O2','/MT',str(OUT/'mutant_route.cpp'),'/Fe'+str(OUT/'mutant_route.exe'),'/Fo'+str(OUT/'mutant_route.obj')],env=env,capture_output=True,text=True);assert r.returncode==0,(name,r.stdout,r.stderr)
 r=subprocess.run([str(OUT/'mutant_route.exe')],capture_output=True,text=True);assert r.returncode!=0,(name,'SURVIVED');print('PASS mutant',name,':',r.stdout.strip())
native=(HERE/'NativeCensus.h').read_text(encoding='utf-8-sig')
for name,old,new in [('ignore native generation','InterlockedCompareExchange(live_,0,0)==generation_','true'),('ignore native world','au->render->scene==scene_','true'),('wrong physical owner','Pointer(part+8)==entity','true'),('ignore floor change','!Same(floors_[f],after)','false'),('ignore selected membership','index<h.count&&Pointer(h.data+index)==entity','index<h.count'),('omit global physical budget','if(physicalReads_==EntryLimit)return NULL;','if(false)return NULL;')]:
 assert old in native
 (OUT/'mutant_native.h').write_text(native.replace(old,new,1))
 r=subprocess.run([sys.executable,str(HERE/'test_native_extracted.py'),str(OUT/'mutant_native.h')],capture_output=True,text=True);assert r.returncode!=0 and 'FAIL ' in r.stdout,(name,r.stdout,r.stderr);print('PASS mutant',name)
print('PASS 19 compiled actual-production functional mutants')
