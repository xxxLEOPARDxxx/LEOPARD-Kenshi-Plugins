"""Actual boot/settings and hidden-map result prefixes; no native engine calls."""
from pathlib import Path
import os,re,subprocess
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
main=(root/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig')
fine=(root/'GsmFineViewer.h').read_text(encoding='utf-8-sig')
def body(source,signature):
    a=source.index('{',source.index(signature));b=a+1;depth=1
    while depth:
        depth+=(source[b]=='{')-(source[b]=='}');b+=1
    return source[a+1:b-1]
settings=body(main,'void MemorySettings()')
hidden='if(!admitted){'+body(fine,'if(!admitted){if(result==1)')+'}'
guard=fine[fine.index('if(!enabled||!ready||!FineQualified()',fine.index('void FineObserve(')):fine.index('fineMap=map;',fine.index('void FineObserve('))]
assert 'MemorySettings()' not in body(main,'void Settings()')
assert 'void startPlugin(){MemorySettings();Settings();' in main
for key,default,upper in [('VramCacheGiB',1,8),('RamCacheGiB',2,32)]:
    assert re.search(r'api->integer\(api,ID,"'+key+r'",.*?,'+str(default)+r',1,'+str(upper)+r',MCM_RESTART\)',main)
    assert key not in body(main,'void Settings()')
code=r'''
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include "PROFILE"
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
const char*ID="GlobalSatelliteMap";int requestedGpu=1,requestedRam=2,reads=0;
std::string Ini(){return "fixture.ini";}
int FakeRead(const char*,const char*key,int,const char*){++reads;return std::string(key)=="VramCacheGiB"?requestedGpu:requestedRam;}
#define GetPrivateProfileIntA FakeRead
void MemorySettings(){SETTINGS}
struct Owner {unsigned drains,releases,cancels;bool held;Owner():drains(0),releases(0),cancels(0),held(false){}bool Drain(){++drains;return true;}bool ReadReady(){return held;}bool ReleaseRead(){held=false;++releases;return true;}void Cancel(){++cancels;}}owner;
Owner*fineOwner=&owner;unsigned finePhase=0,fineRead=4,fineAhead=0,visible=0;bool fineSpeculative=false;
void Hidden(int result,bool admitted){HIDDEN ++visible;}
bool enabled=true,ready=true,qualified=true,context=true;
bool FineQualified(){return qualified;}bool FineOwnerContext(){return context;}
struct{unsigned revision;}fineDemand={1};
struct Map{bool shown;bool getVisible(){return shown;}};
void Observe(Map*map){GUARD ++visible;}
int main(int argc,char**argv){try{
 requestedGpu=atoi(argv[1]);requestedRam=atoi(argv[2]);MemorySettings();
 unsigned gpu=unsigned((std::max)(1,(std::min)(8,requestedGpu))),ram=unsigned((std::max)(1,(std::min)(32,requestedRam)));
 C(GsmCacheProfile::GpuBudgetBytes()==gpu*GsmCacheProfile::GiB&&GsmCacheProfile::RamBudgetBytes()==ram*GsmCacheProfile::GiB,"actual boot INI values/clamps configure both budgets");
 C(reads==2,"budget INI read only during boot");
 Hidden(1,false);C(owner.drains==1&&finePhase==2&&visible==0,"hidden map drains acknowledged producer");
 fineSpeculative=true;Hidden(2,false);C(!fineSpeculative&&fineAhead==1&&finePhase==3&&fineRead==0,"hidden map preserves completion bookkeeping");
 owner.held=true;Hidden(3,false);C(owner.releases==1&&!owner.held&&visible==0,"hidden completed read released without upload");
 Hidden(0,true);C(visible==1,"visible path remains available");
 Map map={false};Observe(&map);Observe(NULL);C(owner.cancels==0,"ordinary map close keeps RAM worker warm");
 map.shown=true;Observe(&map);C(visible==2&&reads==2,"reopen uses same worker and no INI poll");
 context=false;Observe(&map);C(owner.cancels==1,"world identity mismatch cancels retained worker");
 context=true;enabled=false;Observe(&map);C(owner.cancels==2,"plugin disable cancels retained worker");
 printf("PASS %u actual memory-settings/hidden-map controls\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('PROFILE',str(root/'GsmCacheProfile.h').replace('\\','/')).replace('SETTINGS',settings).replace('HIDDEN',hidden).replace('GUARD',guard)
(out/'main.cpp').write_text(code)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for mode in ('Od','O2'):
    exe=out/'main.exe'
    p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,str(out/'main.cpp'),'/Fo'+str(out/'main.obj'),'/Fe'+str(exe)],cwd=out,env=env,capture_output=True,text=True)
    assert p.returncode==0,p.stdout+p.stderr
    for gpu,ram in [(n,9-n)for n in range(1,9)]+[(8,32),(1,16),(8,31),(4,9),(-3,99),(99,-3)]:
        p=subprocess.run([str(exe),str(gpu),str(ram)],capture_output=True,text=True)
        assert p.returncode==0,p.stdout+p.stderr
    print('PASS actual MCM/boot/hidden-map '+mode+' (14 profiles)')
