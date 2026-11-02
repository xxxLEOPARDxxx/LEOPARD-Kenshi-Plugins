"""Compile actual map knowledge/refresh bodies at fake native/UI boundaries.
Only owned test outputs; never load a game DLL or write player settings.
"""
from pathlib import Path
import os,subprocess
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ItemMarkers.cpp'
text=SOURCE.read_text(encoding='utf-8-sig')
assert '// BEGIN known location maps' in text,'RED: location-map knowledge implementation is absent'
def body(marker):
 a=text.index(marker);b=text.index('{',a);e=b+1;depth=1
 while depth:depth+=(text[e]=='{')-(text[e]=='}');e+=1
 return text[a:e]
knowledge=text.split('// BEGIN known location maps',1)[1].split('// END known location maps',1)[0]
refresh=body('void RecheckDynamicMarks(')
code=r'''
#include <Windows.h>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <new>
#include <cstdlib>
typedef UINT_PTR uintptr_t;
static DWORD testTick=1000;DWORD TestNow(){return testTick;}void TestAdvance(DWORD n){testTick+=n;}
#define GetTickCount TestNow
#define Sleep TestAdvance
static long allocs=0,checks=0,errors=0,writes=0,queries=0,worldScans=0;static bool nativeFault=false;
void*operator new(size_t n){++allocs;void*p=malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete(void*p){free(p);}
void*operator new[](size_t n){++allocs;void*p=malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete[](void*p){free(p);}
enum itemType{TOWN=13,MAP_ITEM=102,BLUEPRINT=90,OTHER=4};
struct Item;struct GameData;struct GameDataReference{std::string sid;GameDataReference(const char*s):sid(s){}};
namespace Ogre{template<class T>struct vector{typedef std::vector<T>type;};}
template<class T>struct lektor:std::vector<T>{};
struct GameData{itemType type;std::vector<GameDataReference>refs;bool noRefs;GameData(itemType t=OTHER):type(t),noRefs(false){}const std::vector<GameDataReference>*getReferenceListIfExists(const std::string&)const{return noRefs?NULL:&refs;}};
struct hand{itemType type;Item*live;hand():type(OTHER),live(NULL){}Item*getItem()const{++queries;return live;}};
struct RootObject{hand h;GameData*data;RootObject(GameData*d=NULL):data(d){h.type=TOWN;}const hand&getHandle(){return h;}};
struct Town:RootObject{bool discovered,genuine;Town(GameData*d,bool b):RootObject(d),discovered(b),genuine(true){}Town*isTown(){return genuine?this:NULL;}bool isDiscovered()const{if(nativeFault)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);return discovered;}};
typedef Town TownBase;
struct TownList{lektor<RootObject*>all;lektor<RootObject*>&getAllTowns(){++worldScans;return all;}};
struct SharedKing{TownList*townList;};static SharedKing*shou=NULL;
struct DataContainer{std::map<std::string,GameData*>data;GameData*getData(const std::string&sid){std::map<std::string,GameData*>::iterator i=data.find(sid);return i==data.end()?NULL:i->second;}};
struct World{DataContainer gamedata;};static World*ou=NULL;
struct Item{GameData*data;bool learned,traded;Item(GameData*d):data(d),learned(false),traded(false){}itemType getClassType(){return data?data->type:OTHER;}};
static bool g_enabled=true,g_tradeBags=true,g_blueprint=true,g_knownMaps=true,g_shown=true;
bool BlueprintLearned(Item*i){return i&&i->getClassType()==BLUEPRINT&&i->learned;}
void ErrorLog(const char*){++errors;}
namespace MyGUI{struct Widget{bool visible;Widget():visible(true){}bool getInheritedVisible(){return visible;}};struct IntSize{int width,height;IntSize(int w,int h):width(w),height(h){}};struct IntCoord{int left,top,width,height;IntCoord(int l,int t,int w,int h):left(l),top(t),width(w),height(h){}};}
KNOWLEDGE
struct IconEntry{std::vector<MyGUI::Widget*>marks;Item*item;hand handle;bool bag,known,knowledgeItem;MyGUI::Widget*eye;IconEntry():item(NULL),bag(false),known(false),knowledgeItem(false),eye(NULL){}};
std::map<MyGUI::Widget*,IconEntry>g_icons;
bool TradedNow(Item*i){return i->traded;}
Item*ResolveIconItem(IconEntry&entry){Item*i=entry.handle.getItem();return i==entry.item?i:NULL;}
void Undecorate(IconEntry&e){++writes;e.eye=NULL;e.known=false;e.marks.clear();}
bool SafeDecorate(MyGUI::Widget*w,IconEntry*e,MapKnowledgeBatch*maps){++writes;e->bag=TradedNow(e->item);e->known=WantedKnownEye(e->item,*maps);e->eye=e->known?w:NULL;if(e->known)e->marks.push_back(w);return true;}
struct FakeTrade{void*root;FakeTrade():root(NULL){}void*getRootObject(){return root;}};struct ForgottenGUI{FakeTrade tradeA,tradeB;};
REFRESH
void Check(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
int main(){try{World world;ou=&world;TownList towns;SharedKing shared={&towns};shou=&shared;GameData a(TOWN),b(TOWN),map(MAP_ITEM),plain(MAP_ITEM),wrong(OTHER),blue(BLUEPRINT);world.gamedata.data["a"]=&a;world.gamedata.data["b"]=&b;Town ta(&a,true),tb(&b,false);towns.all.push_back(&ta);towns.all.push_back(&tb);map.refs.push_back(GameDataReference("a"));map.refs.push_back(GameDataReference("b"));Item item(&map),ordinary(&plain),bp(&blue);MapKnowledgeBatch batch;
 MapKnowledgeBatch deferred(false);unsigned deferredScans=worldScans;Check(!MapLocationsKnown(&item,deferred)&&worldScans==deferredScans&&!deferred.built,"icon creation and MCM defer map world scan to periodic batch");
 Check(!MapLocationsKnown(&ordinary,batch)&&!batch.built,"ordinary map with zero towns never vacuously known or scans world");Check(!MapLocationsKnown(&item,batch),"multi-location map remains unknown while one location is unknown");tb.discovered=true;MapKnowledgeBatch all;Check(MapLocationsKnown(&item,all),"all current listed towns discovered gets eye");
 map.refs.push_back(GameDataReference("missing"));MapKnowledgeBatch missing;Check(!MapLocationsKnown(&item,missing),"unresolved reference vetoes eye even after genuine towns");map.refs.pop_back();world.gamedata.data["b"]=&wrong;MapKnowledgeBatch invalid;Check(!MapLocationsKnown(&item,invalid),"non-town reference never counts as genuine");world.gamedata.data["b"]=&b;
 towns.all.pop_back();MapKnowledgeBatch destroyed;Check(!MapLocationsKnown(&item,destroyed),"missing current town vetoes old discovery");towns.all.push_back(&tb);Town extra(&a,false);towns.all.push_back(&extra);MapKnowledgeBatch dup;Check(!MapLocationsKnown(&item,dup),"every instance of a listed location must be discovered");towns.all.pop_back();
 plain.noRefs=true;MapKnowledgeBatch absent;Check(!MapLocationsKnown(&ordinary,absent)&&!absent.built,"absent reference list refuses without traversal");plain.noRefs=false;
 map.refs.resize(257,GameDataReference("a"));MapKnowledgeBatch manyRefs;Check(!MapLocationsKnown(&item,manyRefs)&&!manyRefs.built,"reference cap refuses before traversal");map.refs.resize(2,GameDataReference("b"));
 SharedKing* saved=shou;shou=NULL;MapKnowledgeBatch noWorld;Check(!MapLocationsKnown(&item,noWorld),"missing shared world refuses");shou=saved;
 size_t townCount=towns.all.size();towns.all.resize(8193,&ta);MapKnowledgeBatch tooMany;Check(!MapLocationsKnown(&item,tooMany)&&!tooMany.valid,"town cap refuses whole batch");towns.all.resize(townCount);
 std::vector<GameData> unique(2049,GameData(TOWN));std::vector<Town> uniqueTowns;uniqueTowns.reserve(2049);for(unsigned n=0;n<2049;++n){uniqueTowns.push_back(Town(&unique[n],true));towns.all.push_back(&uniqueTowns.back());}MapKnowledgeBatch overflow;Check(!MapLocationsKnown(&item,overflow)&&!overflow.valid,"hash saturation refuses whole batch boundedly");towns.all.resize(townCount);
 tb.genuine=false;MapKnowledgeBatch refusedTown;Check(!MapLocationsKnown(&item,refusedTown),"native isTown refusal never counts as location");tb.genuine=true;extra.discovered=true;extra.genuine=false;towns.all.push_back(&extra);MapKnowledgeBatch refusedDuplicate;Check(!MapLocationsKnown(&item,refusedDuplicate),"refused town invalidates batch even with same-data genuine known instance");towns.all.pop_back();
 nativeFault=true;MapKnowledgeBatch faulty;Check(!SafeKnownEye(&item,faulty)&&!faulty.valid&&errors==1,"native knowledge fault fails closed and logs with Debug off");nativeFault=false;Check(!SafeKnownEye(&item,faulty),"faulted snapshot is never reused as valid");MapKnowledgeBatch recovered;Check(SafeKnownEye(&item,recovered),"next ephemeral batch recovers without persistent poisoned state");
 bp.learned=true;MapKnowledgeBatch toggles;g_knownMaps=false;Check(!WantedKnownEye(&item,toggles)&&WantedKnownEye(&bp,toggles)&&!toggles.built,"map toggle independent of blueprint and avoids all map work");g_knownMaps=true;g_blueprint=false;Check(WantedKnownEye(&item,toggles)&&!WantedKnownEye(&bp,toggles),"blueprint toggle independent of maps");g_blueprint=true;
 unsigned beforeAllocs=allocs,beforeScans=worldScans;LARGE_INTEGER freq,start,end;QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&start);for(unsigned n=0;n<10000;++n)Check(MapLocationsKnown(&item,all),"unchanged scratch lookup stays known");QueryPerformanceCounter(&end);std::printf("Host fixture 10000 actual policy lookups %.3f ms; allocations %ld; new scans %ld\n",(end.QuadPart-start.QuadPart)*1000.0/freq.QuadPart,allocs-beforeAllocs,worldScans-beforeScans);Check(allocs==beforeAllocs&&worldScans==beforeScans,"unchanged lookups allocate nothing and reuse single ephemeral snapshot");
 for(int n=1;n<=512;++n){MyGUI::IntCoord c=KnownEyeCoord(MyGUI::IntSize(n,n+1));int side=n*2/5;if(side<14)side=14;if(side>28)side=28;Check(c.left==2&&c.top==n+1-side-2&&c.width==side&&c.height==side,"map and blueprint use exact original eye geometry");}
 MyGUI::Widget root;IconEntry e;e.item=&item;e.handle.live=&item;e.knowledgeItem=true;g_icons[&root]=e;ForgottenGUI gui;root.visible=false;beforeScans=worldScans;Sleep(410);RecheckDynamicMarks(&gui);Check(worldScans==beforeScans,"hidden maps never scan world");root.visible=true;Sleep(410);RecheckDynamicMarks(&gui);Check(g_icons[&root].known&&g_icons[&root].eye==&root,"existing inventory map refreshes outside trade");beforeAllocs=allocs;unsigned beforeWrites=writes;beforeScans=worldScans;RecheckDynamicMarks(&gui);Check(writes==beforeWrites&&allocs==beforeAllocs&&worldScans==beforeScans,"same-frame cadence is zero allocation and UI writes");Sleep(410);RecheckDynamicMarks(&gui);Check(writes==beforeWrites&&allocs==beforeAllocs,"unchanged periodic state is zero allocation and UI writes");
 tb.discovered=false;Sleep(410);RecheckDynamicMarks(&gui);Check(!g_icons[&root].known&&!g_icons[&root].eye,"new world at same addresses removes previously known eye");tb.discovered=true;Sleep(410);RecheckDynamicMarks(&gui);Check(g_icons[&root].known,"new discovery refreshes already-open map icon");
 MyGUI::Widget root2;g_icons[&root2]=e;beforeScans=worldScans;Sleep(410);RecheckDynamicMarks(&gui);Check(worldScans==beforeScans+1&&g_icons[&root2].known,"two visible maps share exactly one periodic world snapshot");g_icons.erase(&root2);
 MyGUI::Widget tradeRoot;Item weapon(&wrong);IconEntry tradeEntry;tradeEntry.item=&weapon;tradeEntry.handle.live=&weapon;g_icons[&tradeRoot]=tradeEntry;weapon.traded=true;gui.tradeA.root=&weapon;Sleep(410);RecheckDynamicMarks(&gui);Check(g_icons[&tradeRoot].bag,"ordinary item trade bag refresh remains active");g_icons.erase(&tradeRoot);gui.tradeA.root=NULL;
 g_icons[&root].eye=NULL;Sleep(410);RecheckDynamicMarks(&gui);Check(g_icons[&root].eye==&root,"native child unlink permits eye restoration");g_icons[&root].handle.live=NULL;Sleep(410);RecheckDynamicMarks(&gui);Check(!g_icons[&root].eye&&!g_icons[&root].known,"destroyed item handle removes stale mark without item dereference");
 std::printf("Known map production %ld checks PASS\n",checks);return 0;}catch(...){return 1;}}
'''.replace('KNOWLEDGE',knowledge).replace('REFRESH',refresh)
OUT=ROOT/'tests/_build-known-maps';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,source,success=True):
 folder=OUT/name;folder.mkdir(exist_ok=True);unit=folder/'test.cpp';unit.write_text(source)
 r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(unit),'/Fo'+str(folder)+'/', '/Fe'+str(folder/'test.exe')],env=env,capture_output=True,text=True);(folder/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stdout+r.stderr
 r=subprocess.run([str(folder/'test.exe')],capture_output=True,text=True);(folder/'run.log').write_text(r.stdout+r.stderr);assert (r.returncode==0)==success,(name,r.stdout+r.stderr)
 if not success:assert 'FAIL ' in r.stdout,(name,r.stdout+r.stderr)
 print(name,r.stdout.strip())
run('actual',code)
for name,old,new in [
 ('any-known','fact.allKnown = fact.allKnown && town->isDiscovered();','fact.allKnown = true;'),
 ('vacuous','refs->empty()','false'),
 ('unresolved','data == NULL || data->type != TOWN','data != NULL && data->type != TOWN'),
 ('no-cadence','now - s_last < 400','false'),
 ('trade-only','const bool trade = g_tradeBags','if (gui->tradeA.getRootObject() == NULL && gui->tradeB.getRootObject() == NULL) return;const bool trade = g_tradeBags'),
 ('redraw-unchanged','known == entry.known && (!known || entry.eye != NULL)','false'),
 ('map-toggle','g_knownMaps && MapLocationsKnown(item, batch)','MapLocationsKnown(item, batch)'),
 ('missing-child','(!known || entry.eye != NULL)','true'),
 ('wrong-eye-size','if (side > 28) side = 28;','if (side > 30) side = 30;'),
 ('creation-scan','!batch.built && !batch.allowBuild','false'),
 ('town-cap','towns.size() > MapKnowledgeBatch::MAX_TOWNS','false'),
 ('reference-cap','refs->size() > MapKnowledgeBatch::MAX_REFERENCES','false'),
 ('ignore-fault','batch.valid = false;\n            static bool told','batch.valid = true;\n            static bool told'),
 ('hidden-scan','!it->first->getInheritedVisible()','false'),
 ('refusal-skip','return;                     // genuine-town read refused','continue;                     // genuine-town read refused'),
]:
 assert old in code,name
 if name=='unresolved':
  # Refusing to resolve a missing SID must fail a normal assertion, not crash
  # the test process at the fake native boundary.
  altered=code.replace(old,new).replace('unsigned slot = MapFactSlot(data), probe = 0;','if (data == NULL) continue;unsigned slot = MapFactSlot(data), probe = 0;')
 else:altered=code.replace(old,new)
 run(name,altered,False)
assert 'KnownEyeCoord(size)' in body('void Decorate(')
assert 'Texture("itemmarkers_known.png")' in body('void Decorate(')
assert 'townsDiscoveredCount' not in body('bool MapLocationsKnown(') and 'selectTowns(' not in body('bool MapLocationsKnown(')
assert text.index('"MapsKnown", Tr(')<text.index('"Debug", Tr(')
assert 'ReadInt(ini, "MapsKnown", 1, 0, 1)' in text
assert 'MapKnowledgeBatch maps(false);' in body('void OnIcon(')
assert 'MapKnowledgeBatch maps(false);' in body('void RefreshAll()\n    {')
assert 'MapKnowledgeBatch maps;' in refresh
print('15 compiled functional mutants rejected; actual overlay asset/default/toggle placement checked')
