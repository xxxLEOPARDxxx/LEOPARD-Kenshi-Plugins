"""Extract production refresh, handle resolution and decoration prefix.
Fake native/UI boundaries only; no game DLL, settings or real widgets.
"""
from pathlib import Path
import os,subprocess,hashlib,json
HERE=Path(__file__).resolve().parent;ROOT=HERE.parent;OUT=HERE/'_build-extensions'/'refresh';OUT.mkdir(parents=True,exist_ok=True)
text=(ROOT/'ItemMarkers.cpp').read_text(encoding='utf-8-sig')
def function(marker):
 a=text.index(marker);b=text.index('{',a);e=b+1;depth=1
 while depth:
  depth+=(text[e]=='{')-(text[e]=='}');e+=1
 return text[a:e]
refresh=function('void RefreshExtensionMarks()');resolve=function('Item* ResolveIconItem(')
a=text.index('void Decorate(');b=text.index('const int grade = entry.grade.rank;',a)
decorate=text[a:b]+'}\n'
prefix=r"""
#include <Windows.h>
#include <stdint.h>
#include <map>
#include <cstdio>
#include <cstdlib>
static unsigned checks=0,queries=0,handles=0,undecorates=0,decorates=0,reloads=0,revisionReads=0;static DWORD tick=0;static bool handleFault=false;
DWORD Now(){return tick;}
#define GetTickCount Now
void Check(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);std::exit(1);}}
struct ResolvedGrade{int rank;unsigned colour;uint64_t revision;ResolvedGrade():rank(2),colour(0x123456),revision(1){}};
struct Item{void*data;ResolvedGrade grade;Item():data(this){}};
struct hand{Item*live;hand():live(NULL){}Item*getItem(){++handles;if(handleFault)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);return live;}};
namespace MyGUI{struct IntSize{int width,height;};struct Widget{bool visible;IntSize size;Widget():visible(true){size.width=size.height=20;}bool getInheritedVisible(){return visible;}IntSize getSize(){return size;}};}
struct MapKnowledgeBatch{MapKnowledgeBatch(bool b){Check(!b,"refresh must defer map-world traversal");}};
struct IconEntry{Item*item;hand handle;ResolvedGrade grade;IconEntry():item(NULL){}};
std::map<MyGUI::Widget*,IconEntry>g_icons;bool g_enabled=true,g_rarity=true,g_shown=true;
namespace IMRuntime{struct State{uint64_t revision;State():revision(1){}void ProcessReload(){++reloads;}uint64_t Revision(){++revisionReads;return revision;}void Notify(){++revision;}};State state;State&Get(){return state;}}
ResolvedGrade ResolveGrade(Item*i){++queries;ResolvedGrade g=i->grade;g.revision=IMRuntime::Get().revision;return g;}
void Undecorate(IconEntry&){++undecorates;}
"""
bridge=r"""
bool SafeDecorate(MyGUI::Widget*w,IconEntry*e,MapKnowledgeBatch*m,const ResolvedGrade*supplied=NULL){++decorates;Decorate(w,*e,*m,supplied);return true;}
"""
suffix=r"""
void Step(){tick+=400;RefreshExtensionMarks();}
void CleanCounts(){queries=handles=undecorates=decorates=reloads=revisionReads=0;}
int main(){
 Item item,other;MyGUI::Widget widget;IconEntry entry;entry.item=&item;entry.handle.live=&item;g_icons[&widget]=entry;
 tick=399;RefreshExtensionMarks();Check(!queries&&!handles&&!reloads&&!revisionReads,"400ms cadence refuses early tick");
 tick=400;RefreshExtensionMarks();Check(!queries&&!handles&&!undecorates&&!decorates&&reloads==1,"unchanged revision avoids provider/handle/widget work");
 IMRuntime::Get().Notify();CleanCounts();tick+=399;RefreshExtensionMarks();Check(!queries&&!reloads,"notification still respects400ms cadence");++tick;RefreshExtensionMarks();Check(queries==1&&handles==1&&!undecorates&&!decorates,"unchanged appearance updates revision without widget churn");Check(g_icons[&widget].grade.revision==IMRuntime::Get().revision,"unchanged appearance acknowledges current revision");
 CleanCounts();Step();Check(!queries&&!handles&&!undecorates&&!decorates,"acknowledged revision skips next tick");
 item.grade.rank=4;for(unsigned n=0;n<100;++n)IMRuntime::Get().Notify();CleanCounts();Step();Check(queries==1&&handles==1&&undecorates==1&&decorates==1,"coalesced notifications change appearance exactly once");Check(g_icons[&widget].grade.rank==4&&g_icons[&widget].grade.revision==IMRuntime::Get().revision,"supplied resolved appearance reused by actual decoration prefix");
 CleanCounts();Step();Check(!queries&&!undecorates&&!decorates,"changed appearance does not churn again");
 item.grade.colour=0xaabbcc;IMRuntime::Get().Notify();CleanCounts();Step();Check(queries==1&&undecorates==1&&decorates==1&&g_icons[&widget].grade.colour==0xaabbcc,"colour-only change redraws once with one ResolveGrade");
 widget.visible=false;IMRuntime::Get().Notify();CleanCounts();Step();Check(!queries&&!handles&&!decorates,"hidden widget avoids provider and handle query");widget.visible=true;CleanCounts();Step();Check(queries==1&&!undecorates&&!decorates,"revealed widget catches pending unchanged revision");
 bool*flags[]={&g_enabled,&g_rarity,&g_shown};for(unsigned n=0;n<3;++n){*flags[n]=false;IMRuntime::Get().Notify();CleanCounts();Step();Check(!queries&&!handles&&!decorates,"disabled feature avoids icon work");*flags[n]=true;}CleanCounts();Step();Check(queries==1&&!undecorates,"reenabled view catches coalesced revision");
 g_icons[&widget].handle.live=NULL;IMRuntime::Get().Notify();CleanCounts();Step();Check(handles==1&&!queries&&!undecorates&&!decorates,"deleted item handle is never queried/decorated");
 g_icons[&widget].handle.live=&other;CleanCounts();Step();Check(handles==1&&!queries&&!decorates,"reused handle identity refuses different Item pointer");
 g_icons[&widget].handle.live=&item;handleFault=true;CleanCounts();Step();Check(handles==1&&!queries&&!decorates,"native handle access fault safely refuses item");handleFault=false;
 CleanCounts();Step();Check(queries==1&&!undecorates&&!decorates,"valid restored handle catches pending revision");
 g_icons.clear();CleanCounts();Step();Check(!queries&&!handles&&!revisionReads,"empty icon set avoids revision/provider work");
 std::printf("PASS %u extracted extension refresh controls\n",checks);return 0;
}
"""
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,parts,opt,red=False):
 source=OUT/(name+'.cpp');source.write_text(prefix+parts[0]+parts[1]+bridge+parts[2]+suffix);exe=source.with_suffix('.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,'/W3',str(source),'/Fo'+str(source.with_suffix('.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name+': '+p.stdout.strip());assert (p.returncode!=0)==red,(name,p.stdout,p.stderr)
parts=(resolve,decorate,refresh)
for opt in ('/Od','/O2'):run('refresh-'+opt[1:],parts,opt)
mutants={
 'cadence':(resolve,decorate,refresh.replace('if (now - last < 400) return;','')),
 'unchanged-revision':(resolve,decorate,refresh.replace('entry.grade.revision == revision || ','')),
 'hidden':(resolve,decorate,refresh.replace(' || !it->first->getInheritedVisible()','')),
 'deleted-handle':(resolve.replace('return live == entry.item ? live : NULL;','return entry.item;'),decorate,refresh),
 'supplied-reuse':(resolve,decorate,refresh.replace('&maps, &grade','&maps, NULL')),
 'unchanged-churn':(resolve,decorate,refresh.replace('continue;                       // notification without a visual change: no widget churn',';')),
}
for name,mutant in mutants.items():
 assert mutant!=parts,name;run('mutant-'+name,mutant,'/Od',True)
(OUT/'verification.json').write_text(json.dumps({'scope':'extracted production RefreshExtensionMarks, ResolveIconItem and Decorate prefix; mocked native/UI/provider boundaries; no actual game widgets or FPS claim','productionSha256':hashlib.sha256((ROOT/'ItemMarkers.cpp').read_bytes()).hexdigest(),'mutantsKilled':list(mutants),'optimizations':['Od','O2']},indent=2)+'\n')
