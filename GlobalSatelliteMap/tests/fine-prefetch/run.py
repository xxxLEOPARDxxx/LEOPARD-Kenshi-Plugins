from pathlib import Path
import os,re,subprocess,json,hashlib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1];OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
s=(ROOT/'GsmFineViewer.h').read_text();a=s.index(' // Bounded cache-ahead');b=s.index(' if(finePhase==3){unsigned count=',a);caller=s[a:b]
event=next(line.strip() for line in s.splitlines() if line.strip().startswith('if(result==1)'))
prefix=r"""
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../../GsmFinePrefetch.h"
namespace FineStore{struct View{int dummy;};}
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
DWORD clockAt=1000;DWORD Now(){return clockAt;}
#define GetTickCount Now
FineJob::Demand fineDemand={};FineJob::Plan finePlan,fineAheadPlan;FineJob::Budget budget={};unsigned finePair=0,finePhase=3,fineRead=0,fineAhead=0,fineAheadRead=0;DWORD fineChangedAt=0;bool fineCenter=false,fineSpeculative=false,fineAheadMiss=false,fineMiss=false;double fineMotionX=1,fineMotionZ=0;int fineMap=1,fineIdentity=1;
bool queueAllowed=true;unsigned queued=0,uploads=0,shades=0;FineJob::Plan submitted;
int FineResidency(){return 1;}int FineCpuLedger(){return 1;}
struct Owner{bool ready,readOk;unsigned reads,releases,drains;Owner():ready(true),readOk(true),reads(0),releases(0),drains(0){}bool Ready(){return ready;}bool RequestRead(const FineJob::Plan&p,const FineWindows::Key&key,const FineJob::Demand&d,int,int,int){C(FineJob::Wanted(p,d,key),"actual cache preflight uses exact plan/current demand");++reads;return true;}bool ReadReady(){return readOk;}bool PeekRead(bool,const FineJob::Demand&,int,int,int,FineStore::View&){return readOk;}bool ReleaseRead(){++releases;return true;}bool Drain(){++drains;return true;}}owner;Owner*fineOwner=&owner;
bool FineQueue(const FineJob::Plan&p,unsigned pair,bool required=true){if(!queueAllowed)return false;C(pair==0&&FineJob::Internal::Fresh(p,fineDemand)&&p.pairCount==1,"actual prefetch caller submits one bounded fresh pair");++queued;submitted=p;return true;}
bool FineUpload(const FineStore::View&){++uploads;return true;}void FineShade(){++shades;}void FinePresent(int){}
"""
suffix=r"""
void Reset(){queueAllowed=true;owner=Owner();finePhase=3;fineRead=fineAhead=fineAheadRead=queued=uploads=shades=0;fineCenter=fineSpeculative=fineAheadMiss=fineMiss=false;fineMotionX=1;fineMotionZ=0;fineChangedAt=0;clockAt=1000;finePair=finePlan.pairCount;fineAheadPlan=FineJob::Plan();}
int main(){try{
 fineDemand.scope.thread=GetCurrentThreadId();fineDemand.scope.world=7;fineDemand.scope.sourceEpoch=0x100000007ULL;fineDemand.scope.job=0x300000005ULL;fineDemand.revision=0x100000009ULL;fineDemand.view.stamp.source=fineDemand.scope.sourceEpoch;fineDemand.view.stamp.material=0x700000008ULL;fineDemand.view.world.x=fineDemand.view.world.z=-14336;fineDemand.view.world.w=512;fineDemand.view.world.h=256;fineDemand.view.screenWidth=512;fineDemand.view.screenHeight=256;budget.producerCpu=FineJob::Internal::ProducerCpuFloor();finePlan=FineJob::MakePlan(fineDemand,budget,FineJob::VisibleSet);C(finePlan.state==FineJob::Detail,"real policy visible plan ready");Reset();
 FineJob::Plan first=GsmFinePrefetch::Make(finePlan,fineDemand,budget,1,0,0),next=GsmFinePrefetch::Make(finePlan,fineDemand,budget,1,0,1);C(first.state==FineJob::Detail&&first.count==2&&first.pairCount==1,"bounded adjacent same-cell two keys");C(next.state==FineJob::Detail&&!FineWindows::Equal(first.keys[0],next.keys[0]),"second forward column is distinct");FineWindows::Rect bounds;for(unsigned n=0;n<2;++n){C(FineWindows::Bounds(first.keys[n],bounds)&&bounds.x>=fineDemand.view.world.x+fineDemand.view.world.w,"prefetch stays beyond forward edge");C(FineJob::Internal::SameCell(first.keys[0],first.keys[n]),"native K2 cell ownership preserved");}C(!GsmFinePrefetch::Make(finePlan,fineDemand,budget,1,0,2).count,"strict two-pair cap");C(!GsmFinePrefetch::Make(finePlan,fineDemand,budget,0,0,0).count,"stationary initial view never fabricates direction");FineJob::Demand stale=fineDemand;stale.revision^=1ULL<<32;C(!GsmFinePrefetch::Make(finePlan,stale,budget,1,0,0).count,"stale64bit demand refuses");
 for(unsigned axis=0;axis<4;++axis){double dx=axis<2?(axis? -1:1):0,dz=axis>=2?(axis==2?1:-1):0;FineJob::Plan p=GsmFinePrefetch::Make(finePlan,fineDemand,budget,dx,dz,0);C(p.state==FineJob::Detail,"all four pan directions have actual adjacent pair");}
 clockAt=599;Caller();C(!fineSpeculative&&!queued,"600ms stability guard");clockAt=600;Caller();C(fineSpeculative&&!queued&&!owner.reads,"eligible tail schedules only one pair");Caller();C(owner.reads==1&&!queued,"preflight is async worker Get before native source");owner.readOk=false;Event(4);owner.readOk=true;Caller();C(owner.reads==2&&!queued,"both keys probed before source");owner.readOk=false;Event(4);owner.readOk=true;Caller();C(queued==1&&fineSpeculative&&finePhase==2&&!uploads,"cache miss production submits bounded pair without GPU upload");Event(1);C(owner.drains==1,"producer Put acknowledgement requests real bank drain");Event(2);C(!fineSpeculative&&fineAhead==1&&fineRead==0&&finePhase==3,"drain acknowledgement resumes visible publication position");Caller();C(fineSpeculative,"secondpair allowed at tail");fineAheadRead=2;fineAheadMiss=false;Caller();C(fineAhead==2&&!fineSpeculative,"cache hit avoids native source queue");unsigned old=queued;Caller();C(!fineSpeculative&&queued==old,"no third speculativepair");
 Reset();fineSpeculative=true;fineAheadPlan=first;Event(3);C(owner.releases==1&&fineAheadRead==1&&!uploads&&!fineRead,"prefetch CPU cache hit released without fine GPU image");Reset();Event(3);C(uploads==1&&fineRead==1&&owner.releases==1,"ordinary visible publication remains unchanged");
 Reset();finePair=0;Caller();C(!fineSpeculative&&!queued,"all visible source pairs strictly precede speculation");Reset();fineCenter=true;Caller();C(!fineSpeculative,"center stage never delays for speculation");Reset();finePhase=4;clockAt=599;Caller();C(!fineSpeculative,"warm completed view respects remaining stability delay");clockAt=600;Caller();C(fineSpeculative,"warm completed view can prefetch once stable if actual headroom fits");
 Reset();FineJob::Budget saved=budget;budget.viewerGpu=28637998;budget.fineGpu=72089600;budget.occupied=55;Caller();C(!fineSpeculative&&!queued,"actual55tile complete-view budget refuses instead of evicting sharp images");budget=saved;Reset();owner.ready=false;Caller();C(!fineSpeculative&&!queued,"busy consumer/source cannot queue more work");Reset();fineMotionX=0;Caller();C(!fineSpeculative&&!queued,"missing direction immediately leaves visible publication free");
 Reset();Caller();fineAheadRead=2;fineAheadMiss=true;queueAllowed=false;Caller();C(!fineSpeculative&&fineAhead==2&&fineRead==0&&finePhase==3,"invalid optional source/profile immediately resumes visible publication");
 printf("PASS %u actual policy/extracted prefetch caller controls\n",checks);return 0;}catch(...){return 1;}}
"""
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,c,e,opt,red=False):
 f=OUT/(name+'.cpp');f.write_text(prefix+'void Event(int result){'+e+'}\nvoid Caller(){'+c+'}\n'+suffix);exe=f.with_suffix('.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(f),'/Fo'+str(f.with_suffix('.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name+': '+p.stdout.strip());assert (p.returncode!=0)==red,p.stdout+p.stderr
for opt in ('/Od','/O2'):run(opt[1:],caller,event,opt)
mutants={'visible-first':caller.replace('finePair>=finePlan.pairCount&&',''),'bounded':caller.replace('fineAhead,fineOwner->Ready()','0,fineOwner->Ready()')}
for name,c in mutants.items():assert c!=caller;run('mutant-'+name,c,event,'/Od',True)
run('mutant-no-display',caller,event.replace('if(!fineSpeculative&&!FineUpload(view))','if(!FineUpload(view))'),'/Od',True)
(OUT/'verification.json').write_text(json.dumps({'source':hashlib.sha256((ROOT/'GsmFineViewer.h').read_bytes()).hexdigest(),'policy':hashlib.sha256((ROOT/'GsmFinePrefetch.h').read_bytes()).hexdigest(),'scope':'Actual pure adjacent policy and extracted real production prefetch/event caller; fake owner boundary, no native engine/FPS claim','mutantsKilled':['visible-first','bounded','no-display']},indent=2)+'\n')



