from pathlib import Path
import subprocess,os,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
source=(Path(sys.argv[1]) if len(sys.argv)>1 else HERE/'NativeCensus.h').read_text(encoding='utf-8-sig')
def extract(needle):
 a=source.index(needle);b=source.index('{',a);depth=1;i=b+1
 while depth:
  depth+= (source[i]=='{')-(source[i]=='}');i+=1
 return source[a:i]
body='\n'.join(extract(n) for n in ['inline const void* Pointer','inline unsigned Number','inline size_t Size','inline bool Header','inline bool Same','inline bool Member'])
methods='\n'.join(extract(n) for n in ['bool Current()const','bool CheckFloors()const','PhysicalEntity*Physical(','unsigned Entries('])
test=r'''
#include "../CensusRoute.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
struct Town{};struct Building{};struct PhysicalEntity{};
namespace Ogre {struct SceneManager{};}
struct Renderer{Ogre::SceneManager*scene;};struct Aux{Renderer*render;};Aux*au=NULL;
namespace NativeModels{bool typed=true;unsigned queries=0;inline bool Typed(const void*,HMODULE,SIZE_T){++queries;return typed;}}
const SIZE_T CensusTownVtable=0x1734c08;
using namespace NativeLoadedCensus;
struct FloorHeader{const void*const*data;size_t count,capacity;};
FREE_FUNCTIONS
struct Harness{
 struct Key{Building*building;const void*info;};
 Town*town_;Ogre::SceneManager*scene_;LONG generation_;volatile LONG*live_;Key keys_[1];unsigned currentKey_;bool haveFloor_;FloorHeader floors_[4];mutable unsigned physicalReads_;
 Harness():town_(NULL),scene_(NULL),generation_(1),live_(NULL),currentKey_(0),haveFloor_(false),physicalReads_(0){}
 NATIVE_METHODS
};
unsigned checks=0;void C(bool v,const char*s){++checks;if(!v){std::printf("FAIL %s\n",s);std::exit(1);}}
void P(BYTE*p,const void*v){std::memcpy(p,&v,8);}void U(BYTE*p,unsigned n){std::memcpy(p,&n,4);}
int main(){Harness h;Town t;Ogre::SceneManager scene;Renderer render={&scene};Aux aux={&render};LONG gen=4;h.town_=&t;h.scene_=&scene;h.generation_=4;h.live_=&gen;au=&aux;
 C(h.Current(),"actual native Current accepts coherent world");gen=5;unsigned q=NativeModels::queries;C(!h.Current()&&NativeModels::queries==q,"changed generation refuses before type query");gen=4;au=NULL;C(!h.Current(),"world unload refuses");au=&aux;render.scene=NULL;C(!h.Current(),"scene replaced refuses");render.scene=&scene;NativeModels::typed=false;C(!h.Current(),"wrong Town type refuses");NativeModels::typed=true;h.live_=NULL;C(!h.Current(),"no generation owner refuses");h.live_=&gen;
 BYTE building[0x230]={},pc[0x18]={},part[0x38]={};const void*entries[2]={part,NULL};const void*entity=&t;P(building+0x228,pc);P(pc+16,entries);U(pc+8,1);U(pc+12,2);P(part+8,entity);
 C(h.Physical(reinterpret_cast<Building*>(building),entity)==reinterpret_cast<PhysicalEntity*>(part),"exact exterior physical ownership");C(!h.Physical(reinterpret_cast<Building*>(building),&scene),"unowned movable rejected");U(pc+8,4097);U(pc+12,4097);C(!h.Physical(reinterpret_cast<Building*>(building),entity),"physical count cap before scan");U(pc+8,2);U(pc+12,1);C(!h.Physical(reinterpret_cast<Building*>(building),entity),"physical bad capacity");U(pc+8,1);U(pc+12,2);P(pc+16,reinterpret_cast<BYTE*>(entries)+1);C(!h.Physical(reinterpret_cast<Building*>(building),entity),"physical alignment");P(pc+16,entries);
 h.physicalReads_=0;for(unsigned i=0;i<EntryLimit;++i)h.Physical(reinterpret_cast<Building*>(building),&scene);C(!h.Physical(reinterpret_cast<Building*>(building),entity)&&h.physicalReads_==EntryLimit,"total physical reads bounded across repeated unready candidates");
 FloorHeader raw[4]={};h.keys_[0].info=raw;C(h.Entries(0)==0&&h.CheckFloors(),"empty floor arrays valid");raw[2].data=entries;raw[2].count=raw[2].capacity=2;C(!h.CheckFloors(),"changed floor rejects publication");h.haveFloor_=false;C(h.Entries(0)==2&&h.CheckFloors(),"exact four floor offsets");C(Member(raw[2],0,part)&&!Member(raw[2],2,part)&&!Member(raw[2],0,&scene),"actual selected owner membership");const void*old=entries[0];entries[0]=&scene;C(!Member(raw[2],0,old),"selected floor item changed at same pointer/count");entries[0]=old;raw[2].count=1;C(!h.CheckFloors(),"floor count reloaded");h.haveFloor_=false;raw[2].count=4097;raw[2].capacity=4097;C(h.Entries(0)==4097,"floor count cap");h.haveFloor_=false;raw[2].data=NULL;raw[2].count=raw[2].capacity=1;C(h.Entries(0)==4097,"nonnull floor storage required");h.haveFloor_=false;raw[2].count=raw[2].capacity=0;raw[0].data=raw[1].data=entries;raw[0].count=raw[1].count=raw[0].capacity=raw[1].capacity=4096;C(h.Entries(0)==4097,"four floor aggregate cap");
 std::printf("PASS %u extracted actual native world/floor/physical checks\n",checks);return 0;}
'''.replace('FREE_FUNCTIONS',body).replace('NATIVE_METHODS',methods)
(OUT/'native_extracted.cpp').write_text(test)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{HERE}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/W3','/O2','/MT',str(OUT/'native_extracted.cpp'),'/Fe'+str(OUT/'native_extracted.exe'),'/Fo'+str(OUT/'native_extracted.obj')],check=True,env=env)
subprocess.run([str(OUT/'native_extracted.exe')],check=True)
