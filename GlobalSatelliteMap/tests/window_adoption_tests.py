"""Actual completion-adoption, cancel and generation guards extracted read-only."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parents[1]/'build/window-integration-tests'
here.mkdir(parents=True,exist_ok=True)
base=Path(__file__).resolve().parents[1]
source=(base/'GlobalSatelliteMap.cpp').read_text()
def fun(marker):
 a=source.index(marker);b=source.index('{',a);depth=0
 for p in range(b,len(source)):
  if source[p]=='{':depth+=1
  if source[p]=='}':
   depth-=1
   if not depth:return source[a:p+1]
 raise ValueError(marker)
draw=source.index('void Draw(')
assert source.index('    if(completed){') > draw
adopt=fun('    if(completed){')
a=source.index('bool current=j->generation==');b=source.index(';',a)+1
guard=source[a:b]
cpp=r'''
#define NOMINMAX
#include <Windows.h>
#include "D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
static unsigned checks=0,uploads=0,destroyed=0;static int selected=-1;
static void Check(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
namespace GSM{volatile LONG generation=7;}
namespace GsmAtlas{
struct Read{volatile LONG cancel;AtlasView::U64 readBytes;Read():cancel(0),readBytes(1049600){}};
struct Job{LONG generation;Read read;AtlasView::WindowKey window;Job():generation(7){}~Job(){++destroyed;}};
}
struct Entry{AtlasView::WindowKey key;unsigned age,shaded;Entry():age(0),shaded(0){}}cache[AtlasView::WindowSlots];
GsmAtlas::Job*job=NULL,*completed=NULL;unsigned windowReads=0;AtlasView::U64 windowBytes=0;
FIND
CANCEL
static void Upload(int slot,const AtlasView::WindowKey&key,GsmAtlas::Job&){++uploads;selected=slot;cache[slot].key=key;}
static void Adopt(const AtlasView::WindowPlan&plan){ADOPT}
static bool Current(GsmAtlas::Job*j){GUARD return current;}
static void Reset(){for(unsigned i=0;i<AtlasView::WindowSlots;++i)cache[i]=Entry();uploads=destroyed=0;selected=-1;windowReads=0;windowBytes=0;}
static void Completion(AtlasView::WindowKey key){completed=new GsmAtlas::Job();completed->window=key;}
int main(){try{
 AtlasView::WindowPlan plan;AtlasView::WindowKey fine(0,0,7,3),coarse(0,1,3,1);
 Reset();plan.count=1;plan.level=1;plan.keys[0]=coarse;plan.visible[0]=true;Completion(fine);Adopt(plan);
 Check(!completed&&destroyed==1&&uploads==0,"stale zoom result deleted without replacing requested mapping");
 Check(windowReads==1&&windowBytes==1049600,"discarded zoom IO remains counted");
 Reset();plan.keys[0]=fine;cache[16].key=fine;Completion(fine);Adopt(plan);
 Check(!completed&&destroyed==1&&uploads==0&&cache[16].key==fine,"duplicate completed key does not recreate live texture");
 Reset();plan=AtlasView::WindowPlan();plan.visible[9]=true;Completion(AtlasView::WindowKey(9,5,0,0));Adopt(plan);
 Check(uploads==1&&selected==9,"coarse window uses its pinned fallback slot");
 Reset();plan=AtlasView::WindowPlan();plan.level=0;plan.count=AtlasView::DetailSlots;
 for(unsigned i=0;i<16;++i)cache[i].key=AtlasView::WindowKey(i,5,0,0);
 for(unsigned i=0;i<AtlasView::DetailSlots-1;++i){plan.keys[i]=AtlasView::WindowKey(i%16,0,i/16,0);cache[16+i].key=plan.keys[i];cache[16+i].age=i+1;}
 cache[AtlasView::WindowSlots-1].key=AtlasView::WindowKey(15,4,1,1);cache[AtlasView::WindowSlots-1].age=0;plan.keys[AtlasView::DetailSlots-1]=fine;Completion(fine);Adopt(plan);
 Check(uploads==1&&selected==int(AtlasView::WindowSlots-1)&&cache[AtlasView::WindowSlots-1].key==fine,"new key evicts only obsolete detail after current-plan check");
 for(unsigned i=0;i<16;++i)Check(cache[i].key==AtlasView::WindowKey(i,5,0,0),"pinned fallback retained during detail admission");
 Reset();plan=AtlasView::WindowPlan();Completion(AtlasView::WindowKey(9,5,0,0));Adopt(plan);
 Check(uploads==0&&!completed&&destroyed==1,"offscreen fallback completion cannot overwrite current view");
 GsmAtlas::Job control;Check(Current(&control),"matching generation accepted");
 control.generation=6;Check(!Current(&control),"old world result rejected");control.generation=7;control.read.cancel=1;Check(!Current(&control),"cancelled matching-generation result rejected");
 Reset();job=new GsmAtlas::Job();Completion(fine);Cancel();
 Check(job&&job->read.cancel&&destroyed==1&&!completed,"Cancel retains pending worker ownership and frees only completed result");
 delete job;job=NULL;
 std::printf("PASS %u extracted production adoption/cancel/generation checks\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('FIND',fun('int FindWindow(')).replace('CANCEL',fun('void Cancel(')).replace('ADOPT',adopt).replace('GUARD',guard)
(here/'adoption.cpp').write_text(cpp)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
exe=here/'adoption.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',f'/Fo{here}/adoption.obj',f'/Fe{exe}',str(here/'adoption.cpp')],check=True,env=env)
subprocess.run([str(exe)],check=True)
