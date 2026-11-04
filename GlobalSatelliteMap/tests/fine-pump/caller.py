"""Execute the actual Main progress batching adapter with controlled clock/I/O."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
s=(root/'GsmFineViewer.h').read_text(encoding='utf-8-sig')
a=s.index('struct FineBatchProgress {');b=s.index('struct FineListener:',a);adapter=s[a:b]
assert 'try{FineRenderBatch();}' in s
assert 'void FinePresent(MapScreen*map){if(fineDeferPaint)return;' in s
assert 'void FineShade(){if(fineDeferPaint||!haveLighting)return;' in s
code=r"""
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <stdexcept>
#include "../../../GsmFinePump.h"
namespace FineWindows {typedef unsigned long long U64;}
namespace FineCoordinator {struct Progress {unsigned state;Progress():state(0){}};bool SameProgress(const Progress&a,const Progress&b){return a.state==b.state;}}
struct FineController {FineCoordinator::Progress state;FineCoordinator::Progress ObserveProgress(){return state;}}owner;
FineController*fineOwner=&owner;
struct Demand{FineWindows::U64 revision;}fineDemand={1};
struct Index{FineWindows::U64 revision;FineWindows::U64 Revision(){return revision;}}fineIndex={1};
struct Plan{unsigned state,count,pairCount,spacing;Demand demand;}finePlan={1,2,1,3,{1}};
unsigned finePhase=0,finePair=0,fineRead=0,fineAhead=0,fineAheadRead=0;
bool fineMiss=false,fineBlocked=false,fineClosing=false,fineCenter=false,fineSpeculative=false,fineAheadMiss=false,fineDeferPaint=false;
bool context=true,clockOk=true,throwTick=false,nested=false;unsigned ticks=0,shades=0,presents=0,ready=0,mode=0;LONGLONG clockNow=0,cost=0;
int fineMap=1;bool FineContext(){return context;}
void FineShade(){if(!fineDeferPaint)++shades;}void FinePresent(int){if(!fineDeferPaint)++presents;}
void FineSeamTick(){}
void FineRenderBatch();
void FineRenderTick(){++ticks;clockNow+=cost;if(throwTick)throw std::runtime_error("fixture");if(nested)FineRenderBatch();if(ready){--ready;switch(mode){case 0:++finePhase;break;case 1:++owner.state.state;break;case 2:++fineIndex.revision;break;case 3:++fineDemand.revision;break;case 4:fineSpeculative=!fineSpeculative;break;case 5:++fineRead;break;case 6:++finePair;break;case 7:++fineAhead;break;case 8:++fineAheadRead;break;case 9:++finePlan.count;break;}}FineShade();FinePresent(fineMap);}
BOOL Clock(LARGE_INTEGER*p){p->QuadPart=clockNow;return clockOk;}
BOOL Frequency(LARGE_INTEGER*p){p->QuadPart=1000000;return clockOk;}
#define QueryPerformanceCounter Clock
#define QueryPerformanceFrequency Frequency
ADAPTER
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
void Reset(){context=clockOk=true;throwTick=nested=false;ticks=shades=presents=ready=mode=0;clockNow=cost=0;fineDeferPaint=false;}
int main(){try{
 Reset();FineRenderBatch();C(ticks==1,"unchanged async state stops immediately");C(shades==1&&presents==1,"waiting paints once");
 for(unsigned m=0;m<10;++m){Reset();mode=m;ready=100;FineRenderBatch();C(ticks==8,"actual snapshot ready transitions capped at eight");C(shades==1&&presents==1,"ready batch paints once");}
 Reset();ready=100;cost=1500;FineRenderBatch();C(ticks==1,"exact time limit prevents second entered operation");
 Reset();ready=100;cost=400;FineRenderBatch();C(ticks==4,"clock cap bounds ready chain");
 Reset();ready=3;FineRenderBatch();C(ticks==4,"ready chain ends on first unchanged async wait");
 Reset();ready=100;context=false;FineRenderBatch();C(!shades&&!presents,"hidden owner advance creates no UI pass");
 Reset();throwTick=true;try{FineRenderBatch();}catch(const std::runtime_error&){}C(ticks==1&&!fineDeferPaint,"exception restores paint state");
 Reset();nested=true;FineRenderBatch();C(ticks==1&&shades==1&&presents==1,"nested callback cannot multiply pump");
 Reset();clockOk=false;FineRenderBatch();C(ticks==1&&shades==1&&presents==1&&!fineDeferPaint,"clock failure preserves single original tick");
 printf("PASS %u actual Main pump caller controls\n",checks);return 0;
}catch(...){return 1;}}
""".replace('ADAPTER',adapter)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ['Od','O2']:
 f=out/'caller.cpp';f.write_text(code);exe=out/'caller.exe'
 r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(f),'/Fo'+str(out/'caller.obj'),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(out/('caller-build-'+opt+'.log')).write_text(r.stdout+r.stderr)
 assert r.returncode==0,r.stdout+r.stderr
 r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,end='');assert r.returncode==0,r.stdout+r.stderr
