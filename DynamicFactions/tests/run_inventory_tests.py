"""Compile the real production InventoryView with a small native-port fixture."""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root.parents[1] / 'build/DynamicFactionsInventoryCheck'
out.mkdir(parents=True, exist_ok=True)
text = (root / 'game/DfMine.cpp').read_text(encoding='utf-8-sig')
start = text.index('    struct InventoryView\n')
end = text.index('\n    };', start) + len('\n    };')
production = text[start:end]
def function(signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]
code = r'''
#include <vector>
#include <cstdio>
#include <cstddef>
#include <cmath>
#include <float.h>
#include <limits>
typedef unsigned int uint32_t;
template<class T> struct lektor {
    std::vector<T> data; T* stuff;
    lektor():stuff(NULL){} unsigned int size()const{return (unsigned int)data.size();}
    void sync(){stuff=data.empty()?NULL:&data[0];}
};
namespace Ogre {template<class T>struct vector{typedef std::vector<T> type;};}
struct Inventory;
struct GameData {};
const int ITEM_FOOD=3,ITEM_FOOD_RESTRICTED=15;
struct Item {Inventory* child;bool isEquipped,deathItem,edible;int itemFunction,quantity;
float chargesLeft,originalFullChargeAmount;
Item(Inventory* p=NULL,bool e=false):child(p),isEquipped(e),deathItem(false),edible(true),itemFunction(0),quantity(1),chargesLeft(1),originalFullChargeAmount(1){}
Inventory* getInventory(){return child;}};
struct InventorySection {struct SectionItem{Item* item;SectionItem(Item* p=NULL):item(p){}};
std::vector<SectionItem> items; Inventory* parent;
InventorySection(Inventory* p):parent(p){}
const std::vector<SectionItem>& getItems(){return items;} Inventory* getInventory(){return parent;}};
int graphReads=0,fitCalls=0;
struct Inventory {lektor<Item*> all;lektor<InventorySection*> sections;GameData* allowed;
Inventory():allowed(NULL){}
const lektor<Item*>& getAllItems(){++graphReads;all.sync();return all;}
const lektor<InventorySection*>& getAllSections(){sections.sync();return sections;}
bool hasRoomForItem(GameData* d){++fitCalls;return d==allowed;}};
struct RaceData {bool robot;float hungerRate;RaceData():robot(false),hungerRate(1){}bool canEat(Item* p,bool){return p->edible;}};
struct Medical {float hunger;Medical():hunger(3){}};
struct Character {RaceData race;Medical medical;Inventory* inventory;
Character(Inventory* p):inventory(p){}RaceData* getRace(){return &race;}Medical* getMedical(){return &medical;}
Inventory* getInventory(){return inventory;}void* isAnimal(){return NULL;}};
int checks=0,failed=0;
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
'''+production+'\n'+function('    bool ReadFood(')+'\n'+function('    int FirstFit(')+r'''
int main(){
CHECK(InventoryView(NULL).count==0);
Inventory root,bag;Item equipped(&bag,true);InventorySection worn(&root);
worn.items.push_back(&equipped);root.sections.data.push_back(&worn);
InventoryView physical(&root);CHECK(physical.count==2);CHECK(physical.inv[1]==&bag);
// The same container published in both views is traversed once.
root.all.data.push_back(&equipped);CHECK(InventoryView(&root).count==2);
// No graph cycle and no duplicate equipped reference may grow the view.
Item cycle(&root,true);InventorySection nested(&bag);nested.items.push_back(&cycle);
bag.sections.data.push_back(&nested);CHECK(InventoryView(&root).count==2);
Inventory malformed;malformed.sections.data.push_back(NULL);CHECK(InventoryView(&malformed).count==0);
Inventory foreign;InventorySection wrong(&bag);foreign.sections.data.push_back(&wrong);
CHECK(InventoryView(&foreign).count==0);
Inventory overflow;std::vector<Inventory> children(16);std::vector<Item> boxes;
for(int i=0;i<16;++i)boxes.push_back(Item(&children[i]));
for(int i=0;i<15;++i)overflow.all.data.push_back(&boxes[i]);
CHECK(InventoryView(&overflow).count==16);overflow.all.data.push_back(&boxes[15]);
CHECK(InventoryView(&overflow).count==0);
Inventory many;many.all.data.assign(4097,NULL);CHECK(InventoryView(&many).count==0);
Inventory tooManySections;InventorySection section(&tooManySections);
tooManySections.sections.data.assign(129,&section);CHECK(InventoryView(&tooManySections).count==0);
Inventory tooManyPhysical;InventorySection huge(&tooManyPhysical);
huge.items.assign(4097,InventorySection::SectionItem(NULL));tooManyPhysical.sections.data.push_back(&huge);
CHECK(InventoryView(&tooManyPhysical).count==0);
// Real production food arithmetic sees a partly used stack in worn storage.
Item meal;meal.itemFunction=ITEM_FOOD;meal.quantity=3;meal.chargesLeft=10.0f;meal.originalFullChargeAmount=30.0f;
bag.all.data.push_back(&meal);Character worker(&root);float rate=0,hunger=0,nutrition=0;
CHECK(ReadFood(&worker,rate,hunger,nutrition));CHECK(rate==1 && hunger==3);
CHECK(std::fabs(nutrition-0.7f)<0.00001f);
worker.race.robot=true;CHECK(ReadFood(&worker,rate,hunger,nutrition));CHECK(rate==0 && nutrition==0);
worker.race.robot=false;meal.edible=false;CHECK(ReadFood(&worker,rate,hunger,nutrition));CHECK(nutrition==0);
meal.edible=true;meal.quantity=0;CHECK(!ReadFood(&worker,rate,hunger,nutrition));meal.quantity=3;
meal.chargesLeft=std::numeric_limits<float>::quiet_NaN();CHECK(!ReadFood(&worker,rate,hunger,nutrition));meal.chargesLeft=10.0f;
// A ranked offer batch must traverse containers once, not once per meal.
GameData a,b;bag.allowed=&b;std::vector<GameData*> offers(63,&a);offers.push_back(&b);
graphReads=fitCalls=0;CHECK(FirstFit(&root,offers)==63);CHECK(graphReads==2);CHECK(fitCalls==128);
offers.push_back(&a);graphReads=0;CHECK(FirstFit(&root,offers)==-1);CHECK(graphReads==0);
std::printf("%d inventory checks, %d failed\n",checks,failed);return failed;}
'''
(out / 'inventory_tests.cpp').write_text(code, encoding='utf-8')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env = dict(os.environ)
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include')
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2',
                str(out / 'inventory_tests.cpp'), '/Fo' + str(out / 'inventory_tests.obj'),
                '/Fe' + str(out / 'inventory_tests.exe')], env=env, check=True)
subprocess.run([str(out / 'inventory_tests.exe')], check=True)
