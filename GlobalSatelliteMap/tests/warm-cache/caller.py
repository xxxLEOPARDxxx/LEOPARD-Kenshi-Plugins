from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1];OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
s=(ROOT/'GsmFineViewer.h').read_text();a=s.index(' FineJob::Demand next={};');b=s.index(' // Offscreen same-scope',a);observe=s[a:b];a=s.index('int FineFind(');b=s.index('// Reserve requested layers',a);find=s[a:b];a=s.index(' if(finePhase==3){unsigned count=');b=s.index(' if(finePhase==1&&',a);step=s[a:b]
prefix=r"""
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <new>
#include <cstdlib>
#include "../../../GsmFinePolicy.h"
#include "../../../atlas-terrain/fine-viewer/FineJob.h"
unsigned allocations=0,requests=0,retired=0,checks=0;void*operator new(size_t n){++allocations;void*p=malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete(void*p){free(p);}void*operator new[](size_t n){++allocations;void*p=malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete[](void*p){free(p);}
void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
struct Entry{FineWindows::Key key;bool valid;FineWindows::U64 touch;Entry():valid(false),touch(0){}}fineCache[FineJob::CacheSlots];
FineJob::Demand fineDemand={};FineJob::Plan finePlan,fineAheadPlan;FineWindows::U64 fineUseClock=0;double fineMotionX=0,fineMotionZ=0;DWORD fineChangedAt=0;unsigned fineAhead=0,finePhase=0,finePair=0,fineRead=0;bool fineCenter=false,fineSpeculative=false,fineMiss=false;int fineIdentity=1;
struct MapScreen{FineJob::Demand demand;};bool FineGeometry(MapScreen*map,FineJob::Demand&next){next=map->demand;next.revision=fineDemand.revision;return true;}
int FineResidency(){return 1;}int FineCpuLedger(){return 1;}
struct Owner{bool ReadReady(){return false;}bool ReleaseRead(){return true;}bool RequestRead(const FineJob::Plan&,const FineWindows::Key&,const FineJob::Demand&,int,int,int){++requests;return true;}}owner,*fineOwner=&owner;
void FineRetire(unsigned n){fineCache[n].valid=false;++retired;}
"""
suffix=r"""
int main(){try{
 FineJob::Demand d={};d.scope.thread=GetCurrentThreadId();d.scope.world=7;d.scope.sourceEpoch=0x100000007ULL;d.scope.job=0x200000007ULL;d.revision=1;d.view.stamp.source=d.scope.sourceEpoch;d.view.stamp.material=0x300000007ULL;d.view.world.x=d.view.world.z=-14336;d.view.world.w=2048;d.view.world.h=1024;d.view.screenWidth=2048;d.view.screenHeight=1024;FineJob::Budget budget={};budget.producerCpu=FineJob::Internal::ProducerCpuFloor();FineJob::Plan first=FineJob::MakePlan(d,budget,FineJob::VisibleSet);C(first.state==FineJob::Detail,"actual original visible keys");for(unsigned n=0;n<first.count;++n){fineCache[n].key=first.keys[n];fineCache[n].valid=true;fineCache[n].touch=1;}
 fineDemand=d;MapScreen map;map.demand=d;map.demand.view.world.x+=8192;unsigned before=allocations;Observe(&map);for(unsigned n=0;n<100;++n)Observe(&map);C(!retired&&requests==0&&allocations==before,"pan plus unchanged frames retain old neighborhood with no IO request/allocation");for(unsigned n=0;n<first.count;++n)C(FineFind(first.keys[n])>=0,"all previous view GPU keys still owned offscreen");
 map.demand=d;Observe(&map);C(!retired&&allocations==before,"immediate backpan preserves owners without allocations");for(unsigned n=0;n<first.count;++n)C(FineFind(first.keys[n])>=0&&fineCache[n].touch==fineUseClock,"backpan touches ready existing keys");
 budget.occupied=first.count;budget.fineGpu=first.count*FineJob::WindowBytes;FineWindows::Key held[256];for(unsigned n=0;n<first.count;++n)held[n]=fineCache[n].key;finePlan=FineJob::MakePlan(fineDemand,budget,FineJob::VisibleSet,held,first.count);C(finePlan.state==FineJob::Detail,"actual warm latest-demand plan");fineCenter=false;finePhase=3;finePair=fineRead=0;before=allocations;for(unsigned n=0;n<256&&finePhase!=4;++n)Step();C(finePhase==4&&!requests&&!retired&&allocations==before,"actual caller warm pair/final passes use no Get/Queue/allocation and retain sourceGPU0 boundary");
 printf("PASS %u extracted warm-pan caller controls\n",checks);return 0;}catch(...){return 1;}}
"""
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,o,body,opt,red=False):
 f=OUT/(name+'.cpp');f.write_text(prefix+find+'void Observe(MapScreen*map){'+o+'}\nvoid Step(){'+body+'}\n'+suffix);exe=f.with_suffix('.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(f),'/Fo'+str(f.with_suffix('.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,p.stdout+p.stderr
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name+': '+p.stdout.strip());assert (p.returncode!=0)==red,p.stdout+p.stderr
for opt in ('/Od','/O2'):run('caller-'+opt[1:],observe,step,opt)
old=observe+'for(unsigned n=0;n<FineJob::CacheSlots;++n)if(fineCache[n].valid&&!GsmFinePolicy::Intersects(fineCache[n].key,fineDemand.view))FineRetire(n);'
run('mutant-eager-purge',old,step,'/Od',True)
run('mutant-no-gpu-reuse',observe,step.replace('FineFind(key)>=0','false'),'/Od',True)
