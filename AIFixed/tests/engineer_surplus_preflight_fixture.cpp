#include <Windows.h>
#include <cstdio>
#include <cfloat>
#include <vector>
#include <EngineerMaterialsPolicy.h>
static unsigned checks=0,failures=0,queries=0,scans=0,serial=0;
static void Check(bool b,const char*s){++checks;if(!b){++failures;printf("FAIL %s\n",s);}}
typedef AIFixedEngineer::Identity Identity;typedef AIFixedEngineer::Owned Owned;
static Identity Id(unsigned n){Identity a;a.type=1;a.index=n;a.serial=4;a.container=2;a.containerSerial=9;return a;}
struct hand{Identity id;hand(const Identity&i=Identity()):id(i){}};static Identity Of(const hand&h){return h.id;}
struct GameData{};static GameData data;
struct Item{GameData*data;bool isEquipped,isUnique;int quantity,owner;float chargesLeft,originalFullChargeAmount,quality;void*manufacturerData;void*materialData;void*coloriseData;Item():data(&::data),isEquipped(false),isUnique(false),quantity(1),owner(1),chargesLeft(1),originalFullChargeAmount(1),quality(1),manufacturerData(0),materialData(0),coloriseData(0){}GameData*getGameData(){return data;}bool canStackWith(Item*p){return owner==p->owner;}};
template<class T>struct lektor:std::vector<T>{};struct Inventory{lektor<Item*>items;const lektor<Item*>&getAllItems(){++scans;return items;}};static Inventory body,pack;
struct Character{hand handle;Inventory*getInventory(){return &body;}};static Character actor;
struct Tasker{hand subject;};struct AITaskSytem{Character*character;};struct StorageBuilding{hand handle;};static StorageBuilding store;
static LONG generation=3;static DWORD now=100;static bool enabled=true,storeAvailable=true;static SRWLOCK metadata=SRWLOCK_INIT;enum{Capacity=128,OwnedCapacity=128,ItemLimit=1024};
static LONG Epoch(){return generation;}static DWORD Clock(){return now;}
#define GetTickCount Clock
static bool Live(LONG e){return enabled&&generation==e;}static bool Eligible(Character*,LONG e){return Live(e);}static Inventory*Backpack(Character*){return &pack;}static int BuilderSlot(AITaskSytem*,Tasker*){return 2;}
static Owned owned;static bool Returnable(Character*,const Owned&,GameData*,LONG,int&units){units=2;return true;}
static StorageBuilding*FindStorage(Character*,GameData*,LONG,unsigned){++queries;return storeAvailable?&store:NULL;}
namespace EngineerMaterialsRuntime{static unsigned ListOwned(Owned*out,unsigned cap){if(cap)out[0]=owned;return 1;}static bool Matches(GameData*p,const AIFixedEngineer::Material&){return p==&data;}}
// @PRODUCTION@
static void Reset(Item&managed,Item&personal){managed=Item();personal=Item();body.items.clear();pack.items.clear();body.items.push_back(&managed);body.items.push_back(&personal);for(unsigned i=0;i<Capacity;++i){episodes[i]=Episode();searchSlots[i]=SearchSlot();}searchGeneration=-1;searchWindow=searchCount=0;generation=3;now=100;queries=scans=serial=0;enabled=storeAvailable=true;actor.handle=hand(Id(200));store.handle=hand(Id(201));owned=Owned();owned.actor=Id(200);owned.epoch=3;}
int main(){Item managed,personal;Tasker builder;builder.subject=hand(Id(202));AITaskSytem tasks;tasks.character=&actor;Episode out;
Reset(managed,personal);Check(Prepare(&tasks,&builder,2,out)&&queries==1&&out.used,"supported owned surplus admits a native-storage search and builder episode");
Reset(managed,personal);personal.quality=2;Check(!Prepare(&tasks,&builder,2,out)&&queries==0&&!episodes[0].used,"mixed quality refuses synthetic builder before any storage search");unsigned firstScans=scans;bool allRefused=true;for(unsigned i=0;i<1000;++i)if(Prepare(&tasks,&builder,2,out))allRefused=false;Check(allRefused,"mixed quality remains unadmitted during cooldown");Check(scans==firstScans&&queries==0,"unsupported cargo scan occurs only inside search-ticket budget");
Reset(managed,personal);personal.chargesLeft=personal.originalFullChargeAmount=5;Check(!Prepare(&tasks,&builder,2,out)&&queries==0,"different full charge capacity refuses builder before travel");
Reset(managed,personal);body.items.assign(129,&managed);Check(!Prepare(&tasks,&builder,2,out)&&queries==0,"129-item source refuses builder before travel");
Reset(managed,personal);pack.items.assign(129,&personal);Check(!Prepare(&tasks,&builder,2,out)&&queries==0,"oversized worn backpack also refuses builder before travel");
Reset(managed,personal);body.items.assign(1025,&managed);Check(!Prepare(&tasks,&builder,2,out)&&queries==0,"ItemLimit source refuses synthetic builder without storage search");
Reset(managed,personal);storeAvailable=false;Check(!Prepare(&tasks,&builder,2,out)&&queries==1,"full or unavailable storage keeps native builder failure");allRefused=true;for(unsigned i=0;i<1000;++i)if(Prepare(&tasks,&builder,2,out))allRefused=false;Check(allRefused,"full storage native fallback remains unadmitted");Check(queries==1,"paused timestamp full-storage retries have bounded native queries");now+=250;Check(!Prepare(&tasks,&builder,2,out)&&queries==1,"actor cooldown spans global budget refill");now+=1250;Check(!Prepare(&tasks,&builder,2,out)&&queries==2,"full storage periodically retries after bounded backoff");
printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
