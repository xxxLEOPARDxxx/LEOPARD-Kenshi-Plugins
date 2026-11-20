#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <new>
#include <deque>
#include <vector>
#include "../EngineerMaterialsPolicy.h"
static unsigned checks=0,nativeCalls=0,scratchConstructed=0,scratchDestroyed=0,offerCalls=0;static uintptr_t returnAddress=0;
static LONG epoch=3;static bool flipEpochOnCount=false,flipEpochUnderLock=false,flipEpochOnSource=false,dropAdmissionOnSource=false;
static LONG epochCalls=0,admitCalls=0;
static unsigned vanishItem=0,vanishOnRead=0,itemReads=0,lockCalls=0;
static unsigned liveOfferLists=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CHECK line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void Boundary();static void* FixtureReturnAddress(){return reinterpret_cast<void*>(returnAddress);}
static void FixtureEnter(CRITICAL_SECTION* lock){++lockCalls;EnterCriticalSection(lock);}
enum itemType{BUILDING=0,CHARACTER=1,ITEM=4,NULL_ITEM=11};
enum BuildingClassType{BCTYPE_STORAGE,BCTYPE_PRODUCTION};
enum TaskType{BUILD=2,PICKUP=3,ADD_MATERIALS_TO_BUILDING=71,JOB_BUILDER=125,COLLECT_OUTPUT_RESOURCE_BUILD_MATS=247};
enum taskPriority{NORMAL};
enum StatsEnumerated{STAT_STRENGTH=1};
class Character;class Building;class StorageBuilding;class Item;class GameData;class Inventory;class AI;class AITaskSytem;class CharBody;
struct hand
{
    itemType type;unsigned container,containerSerial,index,serial;
    hand():type(NULL_ITEM),container(0),containerSerial(0),index(0),serial(0){}
    hand(unsigned i,unsigned s,itemType t,unsigned c,unsigned cs):type(t),container(c),containerSerial(cs),index(i),serial(s){}
    Character* getCharacter()const;Building* getBuilding()const;Item* getItem()const;
    bool isNull()const{return type==NULL_ITEM;}
    void setNull(){*this=hand();}
};
static std::map<unsigned,Character*> actors;static std::map<unsigned,Building*> buildings;static std::map<unsigned,Item*> items;
template<typename T>struct lektor{std::vector<T> list;unsigned count,maxSize;T* stuff;lektor():count(0),maxSize(0),stuff(NULL){}unsigned size()const{return count;}T operator[](unsigned i)const{return stuff[i];}};
struct GameData{itemType type;std::string stringID;GameData(const char* n):type(ITEM),stringID(n){}};
struct Inventory
{
    std::map<GameData*,int> counts;lektor<Item*> all;
    int countItems(GameData* d){Boundary();if(flipEpochOnCount){++epoch;flipEpochOnCount=false;}return counts[d];}
    const lektor<Item*>& getAllItems(){Boundary();return all;}
};
struct TaskData{TaskType key;};
struct Location{float x,y,z;Location():x(0),y(0),z(0){}};
struct Tasker
{
    uintptr_t vptr;TaskData* taskData;hand subject,currentSubTarget;
    int startTime,endTime,priority;float weight;Location location;
    Tasker():vptr(0),taskData(NULL),startTime(0),endTime(0),priority(0),weight(1){}
    TaskType key()const{return taskData?taskData->key:TaskType(-1);}
};
struct TaskMatch
{
    TaskData* taskData;hand subject,subtarget;
    TaskMatch():taskData(NULL){}
    TaskType key()const{return taskData?taskData->key:TaskType(-1);}
};
struct ActionDeque{std::deque<Tasker*> list;};
struct OrdersReceiver
{
    bool jobsEnabled,orders;TaskMatch goal;lektor<Tasker*> jobs,permajobs;
    OrdersReceiver():jobsEnabled(true),orders(false){}
    bool isJobsEnabled(){Boundary();return jobsEnabled;}
    bool hasPlayerOrders(){Boundary();return orders;}
    const TaskMatch& getCurrentGoal(){Boundary();return goal;}
};
struct ContainerItem{Inventory inv;Inventory* getInventory(){Boundary();return &inv;}};
struct GlobalConstants{float ENCUMBRANCE_BASE,CARRY_WEIGHT_MULT;GlobalConstants():ENCUMBRANCE_BASE(10),CARRY_WEIGHT_MULT(1){}};
static GlobalConstants constants;static GlobalConstants* con=&constants;
struct CharStats
{
    float strength;CharStats():strength(100){}
    float getStat(StatsEnumerated stat,bool unmodified){Boundary();CHECK(stat==STAT_STRENGTH&&!unmodified);return strength;}
};
struct Character
{
    hand handle;Inventory inv;ContainerItem* bag;AI* ai;AITaskSytem* system;bool player,combat,dead;
    CharStats stats;float carriedWeight;
    Character(unsigned id):handle(id,1,CHARACTER,1,1),bag(NULL),ai(NULL),system(NULL),player(true),combat(false),dead(false),carriedWeight(0){actors[id]=this;}
    bool isPlayerCharacter(){Boundary();return player;}
    bool isDead(){Boundary();return dead;}
    bool isInCombatMode(bool,bool){Boundary();return combat;}
    OrdersReceiver* getOrdersReciever();
    AI* getAI(){Boundary();return ai;}
    Inventory* getInventory(){Boundary();return &inv;}
    ContainerItem* hasABackpackOn(){Boundary();return bag;}
    CharStats* getStats(){Boundary();return &stats;}
    float getTotalCarryWeight(){Boundary();return carriedWeight;}
};
struct Building
{
    struct ConstructionState
    {
        struct BuildMaterial{GameData* mat;};
        lektor<BuildMaterial*> mats;std::map<GameData*,int> needs;bool isComplete;
        ConstructionState():isComplete(false){}
        int needsMat(GameData* m){Boundary();return needs[m];}
    } state;
    hand handle;StorageBuilding* storage;
    Building(unsigned id):handle(id,1,BUILDING,1,1),storage(NULL){buildings[id]=this;}
    ConstructionState* getBuildState(){Boundary();return &state;}
    StorageBuilding* getFunctionStuff(){Boundary();return storage;}
    virtual Inventory* getInventory(){Boundary();return NULL;}
};
struct StorageBuilding:Building
{
    Inventory inv;GameData* output;bool producer;int outputUnits;
    StorageBuilding(unsigned n):Building(n),output(NULL),producer(false),outputUnits(0){storage=this;}
    Inventory* getInventory(){Boundary();return &inv;}
    BuildingClassType getBuildingClass(){Boundary();return producer?BCTYPE_PRODUCTION:BCTYPE_STORAGE;}
    GameData* getProductionItemData(){Boundary();return output;}
    int getCurrentProductionQuantity(){Boundary();return outputUnits;}
    bool isProductionEmpty(){Boundary();return outputUnits<=0;}
    void getItemsWeWantRidOf(lektor<GameData*>& out,bool loot)
    {
        Boundary();++offerCalls;CHECK(!loot);
        out.maxSize=static_cast<unsigned>(inv.counts.size());
        if(!out.maxSize)return;
        out.stuff=new GameData*[out.maxSize];
        ++liveOfferLists;
        for(std::map<GameData*,int>::const_iterator i=inv.counts.begin();i!=inv.counts.end();++i)
            if(i->second>0&&(!output||i->first==output))out.stuff[out.count++]=i->first;
    }
};
static void FreeOfferList(void* list)
{Boundary();CHECK(liveOfferLists>0);--liveOfferLists;delete[] static_cast<GameData**>(list);}
struct Item
{
    hand handle,owner;GameData* data;bool isInInventory;int quantity;float weight;
    Item(unsigned n,GameData* d,int q):handle(n,1,ITEM,1,1),data(d),isInInventory(false),quantity(q),weight(1){items[n]=this;}
    GameData* getGameData(){Boundary();return data;}
    float getItemWeightSingle(){Boundary();return weight;}
    const hand& getInventoryWeAreIn(){Boundary();return owner;}
};
static unsigned endedActions=0,moveAttempts=0;
struct CharBody
{
    uintptr_t vptr;AI* ai;Character* actor;Tasker* action;
    CharBody():vptr(0),ai(NULL),actor(NULL),action(NULL){}
    Character* getCharacter(){Boundary();return actor;}
    Tasker* getCurrentAction(){Boundary();return action;}
    void endAction(){Boundary();++endedActions;action=NULL;}
};
struct AI
{
    struct AIResultsCacher
    {
        AI* ai;
        std::map<GameData*,hand,std::less<GameData*>,std::allocator<std::pair<GameData* const,hand> > > resourceSourceCache;
        AIResultsCacher* _CONSTRUCTOR(AI* owner){Boundary();++scratchConstructed;AIResultsCacher* cache=new(this) AIResultsCacher;cache->ai=owner;return cache;}
        void _DESTRUCTOR(){Boundary();++scratchDestroyed;this->~AIResultsCacher();}
    } resultsCache;
    Character* me;AITaskSytem* taskSystemAI;
};
struct AITaskSytem:OrdersReceiver
{
    Character* character;CharBody* body;ActionDeque actions;unsigned replans;hand failedSource;
    AITaskSytem():character(NULL),body(NULL),replans(0){}
    void taskImpossible(){Boundary();++replans;}
    bool isPathImpossible(const hand& source)
    {Boundary();return source.type==failedSource.type&&source.index==failedSource.index&&source.serial==failedSource.serial;}
};
OrdersReceiver* Character::getOrdersReciever(){Boundary();return system;}
Character* hand::getCharacter()const{Boundary();return type==CHARACTER&&actors.count(index)&&actors[index]->handle.serial==serial?actors[index]:NULL;}
Building* hand::getBuilding()const{Boundary();return type==BUILDING&&buildings.count(index)&&buildings[index]->handle.serial==serial?buildings[index]:NULL;}
Item* hand::getItem()const
{Boundary();if(index==vanishItem&&++itemReads==vanishOnRead){items.erase(index);return NULL;}
return type==ITEM&&items.count(index)&&items[index]->handle.serial==serial?items[index]:NULL;}
// @PRODUCTION@
static void Boundary(){++nativeCalls;CHECK(!EngineerMaterialsRuntime::initialized||EngineerMaterialsRuntime::mutex.RecursionCount==0);}
static bool allow=true;
static LONG E(){InterlockedIncrement(&epochCalls);if(flipEpochUnderLock&&EngineerMaterialsRuntime::mutex.RecursionCount){++epoch;flipEpochUnderLock=false;}return epoch;}
static bool Yes(){InterlockedIncrement(&admitCalls);return allow;}static bool No(){return false;}
static unsigned sourceCalls=0,findCalls=0,groundCalls=0,gives=0,takes=0,collects=0,pickups=0,callbacks=0;
static GameData* material=NULL;static GameData* otherMaterial=NULL;static Item* groundA=NULL;static Item* groundB=NULL;static StorageBuilding* storage=NULL;
static bool Safe(AI*,Item*){Boundary();++groundCalls;return true;}
static StorageBuilding* otherStorage=NULL;
static void Machines(AI*,lektor<Building*>& out,unsigned)
{Boundary();static Building* array[2];array[0]=storage;array[1]=otherStorage;out.stuff=array;out.count=otherStorage?2:1;}
struct NativeCaller
{
    uintptr_t saved;
    explicit NativeCaller(unsigned rva):saved(returnAddress){returnAddress=EngineerMaterialsRuntime::image+rva;}
    ~NativeCaller(){returnAddress=saved;}
};
static bool GroundCandidate(AI* ai,Item* item)
{NativeCaller caller(0x59C7B5);return EngineerMaterialsRuntime::GroundSafe(ai,item);}
static float Source(AI::AIResultsCacher* c,hand& out,GameData* d,StorageBuilding*)
{
    Boundary();++sourceCalls;
    if(flipEpochOnSource){flipEpochOnSource=false;++epoch;out=groundB->handle;return 1;}
    if(dropAdmissionOnSource){dropAdmissionOnSource=false;allow=false;out=groundB->handle;return 1;}
    if(c->resourceSourceCache.count(d)){out=c->resourceSourceCache[d];return out.isNull()?0.0f:1.0f;}
    Item* candidates[2]={groundA,groundB};
    for(unsigned i=0;i<2;++i)if(candidates[i]&&candidates[i]->data==d&&GroundCandidate(c->ai,candidates[i]))
    {out=candidates[i]->handle;c->resourceSourceCache[d]=out;return 1;}
    lektor<Building*> machines;
    {NativeCaller caller(0x59C892);EngineerMaterialsRuntime::Machines(c->ai,machines,0x04002436);}
    if(machines.size()){out=machines[0]->handle;c->resourceSourceCache[d]=out;return 1;}
    out.setNull();return 0;
}
static float Find(AI* ai,const hand&,hand& out,bool)
{Boundary();++findCalls;NativeCaller caller(0x5A20D0);return EngineerMaterialsRuntime::Source(&ai->resultsCache,out,material,NULL);}
struct Finder{uintptr_t vptr,pad[3];Character* actor;};
static bool nativePathImpossible=false;
static bool DestinationPath(void*,const hand&){Boundary();return nativePathImpossible;}
static Building* desiredProject=NULL;
static bool IsProductive(Building* project){Boundary();return project==desiredProject;}
static float Destination(AI* ai,const hand&,hand& out,bool)
{
    Boundary();Finder finder;finder.vptr=EngineerMaterialsRuntime::image+0x1740910;finder.actor=ai->me;
    if(EngineerMaterialsRuntime::DestinationPath(&finder,desiredProject->handle)){out.setNull();return 0;}
    out=desiredProject->handle;return 1;
}
static void Selected(AITaskSytem* s,Tasker* t,float,taskPriority){Boundary();s->goal.taskData=t->taskData;s->goal.subject=t->subject;s->goal.subtarget=t->currentSubTarget;}
static void Callback(AITaskSytem*,Tasker*){Boundary();++callbacks;}
static int Quantity(Tasker*,Character*,GameData*,StorageBuilding*){Boundary();return 2;}
static Building* completeOnTake=NULL;
static Item* Take(Inventory* inv,GameData* d)
{
    Boundary();++takes;if(inv->counts[d]<=0)return NULL;--inv->counts[d];
    if(completeOnTake)
    {
        completeOnTake->state.isComplete=true;
        EngineerMaterialsRuntime::CompleteProject(EngineerMaterialsRuntime::Of(completeOnTake->handle),epoch);
        completeOnTake=NULL;
    }
    static Item one(500,d,1);one.data=d;one.quantity=1;return &one;
}
static bool Give(Character* c,Item* item,bool,bool)
{Boundary();++gives;return EngineerMaterialsRuntime::Add(c->getInventory(),item,1);}
static bool Add(Inventory* inv,Item* item,int)
{Boundary();inv->counts[item->data]+=item->quantity;return true;}
static bool probe=false,forceFallback=false;
static void Collect(Tasker* t,CharBody* b)
{
    Boundary();++collects;
    if(forceFallback)b->actor->system->goal.subtarget.getBuilding()->state.needs[material]=0;
    int quantity=EngineerMaterialsRuntime::Quantity(t,b->actor,material,storage);
    if(quantity==0)quantity=1; // native class6 fallback must be stopped before take!
    for(int i=0;i<quantity;++i)
    {
        returnAddress=EngineerMaterialsRuntime::image+0x33CF1C;
        Item* one=EngineerMaterialsRuntime::Take(&storage->inv,material);if(!one)break;
        returnAddress=EngineerMaterialsRuntime::image+0x33CF37;
        ::Give(b->actor,one,true,false);
        if(probe)
        {
            AIFixedEngineer::Request r;r.actor=EngineerMaterialsRuntime::Of(hand(999,1,CHARACTER,1,1));
            r.project=EngineerMaterialsRuntime::Of(b->actor->system->goal.subtarget);r.source=EngineerMaterialsRuntime::Of(storage->handle);
            EngineerMaterialsRuntime::MaterialOf(material,r.material);r.need=32;r.available=storage->inv.counts[material];r.now=GetTickCount();r.epoch=epoch;
            CHECK(EngineerMaterialsRuntime::Preview(r)==0);
        }
    }
}
static void Pickup(Tasker* t,CharBody* b)
{
    Boundary();++pickups;Item* item=t->subject.getItem();returnAddress=EngineerMaterialsRuntime::image+0x34F5AD;
    if(::Give(b->actor,item,false,false)){item->isInInventory=true;item->owner=b->actor->handle;}
}
static void Deliver(Tasker*,CharBody* b){Boundary();--b->actor->inv.counts[material];}
static void Body(CharBody* b,float){Boundary();if(b&&EngineerMaterialsRuntime::MoveClass(b->action))++moveAttempts;}
static void Connect(Character& c,AI& ai,AITaskSytem& s,CharBody& b)
{c.ai=&ai;c.system=&s;ai.me=&c;ai.taskSystemAI=&s;ai.resultsCache.ai=&ai;s.character=&c;s.body=&b;b.actor=&c;b.ai=&ai;b.vptr=EngineerMaterialsRuntime::image+0x16F7938;}
struct Race{AIFixedEngineer::Request request;volatile LONG* total;};
static DWORD WINAPI RaceCommit(void* p)
{Race* r=static_cast<Race*>(p);InterlockedExchangeAdd(r->total,EngineerMaterialsRuntime::Commit(r->request));return 0;}
static void CheckWindows()
{
    // @WINDOWTESTS@
}
int main()
{
    using namespace EngineerMaterialsRuntime;
    Init(Yes,No,E);ready=1;image=reinterpret_cast<uintptr_t>(con)-0x2132D70;
    const Identity sampled=Of(hand(7002,1,CHARACTER,8002,1));
    CHECK(DeliveryCheckDue(sampled,epoch,1000));
    CHECK(!DeliveryCheckDue(sampled,epoch,1999));
    CHECK(DeliveryCheckDue(sampled,epoch,2000));
    CHECK(DeliveryCheckDue(sampled,epoch+1,2001));
    originalFind=::Find;originalSource=::Source;originalGroundSafe=Safe;originalMachines=::Machines;
    originalSelected=::Selected;originalQuantity=::Quantity;originalCollect=::Collect;originalPickup=::Pickup;
    originalAdd=::Add;originalTake=::Take;originalDeliver=::Deliver;originalBody=::Body;SetSelectedCallback(Callback);
    originalDestination=::Destination;originalDestinationPath=::DestinationPath;
    GameData mats("mats"),iron("iron");material=&mats;otherMaterial=&iron;
    Character actor(1),second(2);AI ai,ai2;AITaskSytem system,system2;CharBody body,body2;
    Connect(actor,ai,system,body);Connect(second,ai2,system2,body2);
    Building project(10);StorageBuilding store(20);storage=&store;
    Item sourceStack(21,&mats,100),heavyStack(22,&mats,100);heavyStack.weight=6;
    Item* stocks[2]={&sourceStack,&heavyStack};store.inv.all.stuff=stocks;store.inv.all.count=1;store.inv.all.maxSize=2;
    desiredProject=&project;
    Building::ConstructionState::BuildMaterial need;need.mat=&mats;Building::ConstructionState::BuildMaterial* needs[1]={&need};
    project.state.mats.stuff=needs;project.state.mats.count=1;project.state.needs[&mats]=32;store.inv.counts[&mats]=100;
    TaskData builderData={JOB_BUILDER},collectData={COLLECT_OUTPUT_RESOURCE_BUILD_MATS},moveData={TaskType(80)},deliverData={ADD_MATERIALS_TO_BUILDING};
    Tasker builder,collection,move,delivery;builder.taskData=&builderData;builder.currentSubTarget=project.handle;
    collection.taskData=&collectData;collection.subject=store.handle;collection.vptr=image+0x16BD7E0;
    delivery.taskData=&deliverData;delivery.vptr=image+0x16BDEF8;delivery.subject=project.handle;
    move.taskData=&moveData;move.vptr=image+0x16BCD88;body.action=&move;
    system.actions.list.push_back(&collection);system.actions.list.push_back(&delivery);
    Item first(30,&mats,1),alternate(31,&mats,1);groundA=&first;groundB=&alternate;first.isInInventory=true;
    ai.resultsCache.resourceSourceCache[&mats]=first.handle;ai.resultsCache.resourceSourceCache[&iron]=store.handle;
    hand out;CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1);CHECK(out.index==alternate.handle.index);
    CHECK(sourceCalls==1&&findCalls==1&&ai.resultsCache.resourceSourceCache.count(&iron)==1);
    CHECK(Of(ai.resultsCache.resourceSourceCache[&mats])==Of(first.handle));
    CHECK(EngineerMaterialsRuntime::Source(&ai.resultsCache,out,&mats,NULL)==1&&Of(out)==Of(first.handle));
    ai.resultsCache.resourceSourceCache.erase(&mats);
    groundA=&alternate;
    const unsigned scratchBeforeCold=scratchConstructed,sourceBeforeCold=sourceCalls;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(alternate.handle));
    CHECK(Of(ai.resultsCache.resourceSourceCache[&mats])==Of(alternate.handle));
    CHECK(sourceCalls==sourceBeforeCold+1&&scratchConstructed==scratchBeforeCold);
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(alternate.handle));
    CHECK(sourceCalls==sourceBeforeCold+1&&scratchConstructed==scratchBeforeCold);
    groundA=&first;ai.resultsCache.resourceSourceCache.erase(&mats);
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(alternate.handle));
    CHECK(Of(ai.resultsCache.resourceSourceCache[&mats])==Of(first.handle));
    CHECK(scratchConstructed==scratchBeforeCold+1&&scratchDestroyed==scratchConstructed);
    {
        StorageBuilding other(24);other.inv.counts[&mats]=16;
        otherStorage=&other;groundA=NULL;groundB=NULL;
        system.failedSource=store.handle;
        ai.resultsCache.resourceSourceCache[&mats]=store.handle;
        CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(other.handle));
        CHECK(Of(ai.resultsCache.resourceSourceCache[&mats])==Of(store.handle));
        CHECK(EngineerMaterialsRuntime::Source(&ai.resultsCache,out,&mats,NULL)==1&&Of(out)==Of(store.handle));
        ai.resultsCache.resourceSourceCache.erase(&mats);
        CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(other.handle));
        CHECK(Of(ai.resultsCache.resourceSourceCache[&mats])==Of(store.handle));
        system.failedSource.setNull();groundA=&first;groundB=&alternate;otherStorage=NULL;
    }
    ai.resultsCache.resourceSourceCache[&mats]=hand();
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(alternate.handle));
    CHECK(ai.resultsCache.resourceSourceCache[&mats].isNull());
    flipEpochOnSource=true;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==0&&out.isNull());
    CHECK(scratchDestroyed==scratchConstructed);
    ai.resultsCache.resourceSourceCache.erase(&mats);
    flipEpochOnSource=true;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==0&&out.isNull());
    Invalidate();
    ai.resultsCache.resourceSourceCache[&mats]=first.handle;
    dropAdmissionOnSource=true;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(first.handle));
    CHECK(!allow&&scratchDestroyed==scratchConstructed);allow=true;
    {
        Query outer(&ai,&project,NULL,epoch);
        CHECK(EngineerMaterialsRuntime::Find(&ai,hand(),out,true)==1&&Of(out)==Of(first.handle));
        CHECK(query==&outer);
    }
    ai.resultsCache.resourceSourceCache[&mats]=store.handle;
    const unsigned beforeLarge=offerCalls;
    store.inv.all.count=store.inv.all.maxSize=129;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(store.handle));
    CHECK(offerCalls==beforeLarge);
    store.inv.all.count=1;store.inv.all.maxSize=2;
    ai.resultsCache.resourceSourceCache[&mats]=alternate.handle;
    CHECK(EngineerMaterialsRuntime::Find(&ai,project.handle,out,true)==1&&Of(out)==Of(alternate.handle));
    {
        Query filter(&ai,&project,&mats,epoch);
        NativeCaller caller(0x59D16B);
        ai.resultsCache.resourceSourceCache.erase(&mats);
        CHECK(EngineerMaterialsRuntime::Source(&ai.resultsCache,out,&mats,NULL)==1&&Of(out)==Of(first.handle));
        CHECK(query==&filter);
    }
    Request preview;CHECK(RequestFor(&actor,&project,alternate.handle,&mats,preview));CHECK(Preview(preview)==1); // queries reserve nothing
    groundA=NULL;groundB=NULL;ai.resultsCache.resourceSourceCache.erase(&mats);
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);CHECK(callbacks==1);
    Request competing;CHECK(RequestFor(&second,&project,store.handle,&mats,competing));CHECK(Preview(competing)==0);
    second.inv.counts[&mats]=5;CHECK(EngineerMaterialsRuntime::Destination(&ai2,hand(),out,true)==0&&out.isNull());
    SetProjectPriority(NULL,IsProductive);
    CHECK(EngineerMaterialsRuntime::Destination(&ai2,hand(),out,true)==1&&Of(out)==Of(project.handle));
    nativePathImpossible=true;
    CHECK(EngineerMaterialsRuntime::Destination(&ai2,hand(),out,true)==0&&out.isNull());
    nativePathImpossible=false;SetProjectPriority(NULL,NULL);
    CHECK(EngineerMaterialsRuntime::Destination(&ai2,hand(),out,true)==0&&out.isNull());
    Finder unrelated;unrelated.vptr=image+1;unrelated.actor=&second;
    {DestinationQuery scope(&ai2);CHECK(!EngineerMaterialsRuntime::DestinationPath(&unrelated,project.handle));}
    CHECK(EngineerMaterialsRuntime::Quantity(&collection,&actor,&mats,&store)==2); // outside admitted runAction stays native
    body.action=&collection;probe=true;store.inv.counts[&mats]=32;EngineerMaterialsRuntime::Collect(&collection,&body);probe=false;
    CHECK(actor.inv.counts[&mats]==32&&store.inv.counts[&mats]==0&&takes==32&&gives==32&&collects==1);
    OwnedSurplus rows[128];CHECK(ListOwned(rows,128)==1&&rows[0].owned==32);
    int allowed=0;CHECK(CheckOwned(rows[0],32,allowed)&&allowed==32);
    body.action=&delivery;EngineerMaterialsRuntime::Deliver(&delivery,&body);CHECK(ListOwned(rows,128)==1&&rows[0].owned==31&&rows[0].expectedTotal==31);
    actor.combat=true;EngineerMaterialsRuntime::Body(&body,.1f);CHECK(ListOwned(rows,128)==1);actor.combat=false;
    ++epoch;Invalidate();actor.inv.counts[&mats]=0;store.inv.counts[&mats]=32;
    project.state.needs[&mats]=32;body.action=&collection;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    const unsigned took=takes;forceFallback=true;EngineerMaterialsRuntime::Collect(&collection,&body);forceFallback=false;
    CHECK(takes==took&&actor.inv.counts[&mats]==0&&store.inv.counts[&mats]==32); // fallback cannot remove an unreserved unit
    ++epoch;Invalidate();actor.inv.counts[&mats]=0;project.state.needs[&mats]=3;
    Item pile(40,&mats,100);collection.subject=pile.handle;collection.vptr=image+0x16BDE38;body.action=&collection;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);EngineerMaterialsRuntime::Pickup(&collection,&body);
    CHECK(pickups==1&&actor.inv.counts[&mats]==100&&pile.isInInventory);
    CHECK(ListOwned(rows,128)==1&&rows[0].owned==100&&rows[0].baseline==0);
    CHECK(CheckOwned(rows[0],100,allowed)&&allowed==100);Returned(rows[0],97,3);CHECK(ListOwned(rows,128)==1&&rows[0].owned==3);
    // Native direct stock entry bypasses the virtual observer and remains a
    // normal add. It must not manufacture a managed acquisition receipt.
    const unsigned directOwned=ListOwned(rows,128);Item direct(700,&iron,1);::Add(&store.inv,&direct,1);
    CHECK(store.inv.counts[&iron]==1&&ListOwned(rows,128)==directOwned);
    // Source disappears at the third resolution: afterPlan, before receipt.
    ++epoch;Invalidate();actor.inv.counts[&mats]=0;Item fleeting(41,&mats,1);
    collection.subject=fleeting.handle;EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    vanishItem=41;vanishOnRead=3;itemReads=0;const unsigned picked=pickups,replanned=system.replans;
    EngineerMaterialsRuntime::Pickup(&collection,&body);vanishItem=0;
    CHECK(pickups==picked&&system.replans==replanned+1&&actor.inv.counts[&mats]==0);
    // Old native snapshots cannot acquire a new epoch label mid-RequestFor.
    Request interrupted;const unsigned entryEpoch=epoch;flipEpochOnCount=true;
    CHECK(!RequestFor(&actor,&project,store.handle,&mats,interrupted,entryEpoch));CHECK(Commit(interrupted)==0);
    Invalidate();Request cut=competing;cut.epoch=epoch;cut.now=GetTickCount();
    flipEpochUnderLock=true;CHECK(Commit(cut)==0);CHECK(!Has(cut));Invalidate();
    // Unsupported valid modded recipes preserve native behavior exactly once.
    actor.inv.counts[&mats]=0;store.inv.counts[&mats]=20;collection.subject=store.handle;collection.vptr=image+0x16BD7E0;
    project.state.mats.count=17;const unsigned collected=collects;EngineerMaterialsRuntime::Collect(&collection,&body);
    CHECK(collects==collected+1&&actor.inv.counts[&mats]==2);project.state.mats.count=1;
    actor.player=false;const unsigned beforeLocks=lockCalls;const LONG beforeEpochs=epochCalls,beforeAdmits=admitCalls;
    EngineerMaterialsRuntime::Body(&body,.1f);CHECK(lockCalls==beforeLocks&&epochCalls==beforeEpochs&&admitCalls==beforeAdmits);actor.player=true;
    // Expanded storage batches use actual modified strength, current native
    // weight and the heaviest matching source unit. No future bag discount.
    ++epoch;Invalidate();actor.inv.counts[&mats]=0;store.inv.counts[&mats]=100;project.state.needs[&mats]=100;
    store.inv.all.count=2;actor.stats.strength=30;actor.carriedWeight=10;ContainerItem bag;actor.bag=&bag;
    CHECK(StorageBudget(&actor,&store.inv,&mats)==5);
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);EngineerMaterialsRuntime::Collect(&collection,&body);
    CHECK(actor.inv.counts[&mats]==5&&store.inv.counts[&mats]==95);
    actor.carriedWeight=100;CHECK(StorageBudget(&actor,&store.inv,&mats)==2);
    actor.stats.strength=0;actor.carriedWeight=3;CHECK(StorageBudget(&actor,&store.inv,&mats)==1);
    GlobalConstants* saved=con;con=NULL;CHECK(StorageBudget(&actor,&store.inv,&mats)==2);con=saved;
    actor.bag=NULL;actor.stats.strength=100;actor.carriedWeight=0;store.inv.all.count=1;
    // An actor whose owner issued a direct order receives exact native behavior.
    system.orders=true;CHECK(EngineerMaterialsRuntime::Quantity(&collection,&actor,&mats,&store)==2);system.orders=false;
    // Stale worker snapshots cannot reset the coordinator to an older world.
    Request stale=preview;stale.epoch=3;CHECK(Commit(stale)==0&&generation==unsigned(epoch));
    Material longId;mats.stringID=std::string(160,'x');CHECK(!MaterialOf(&mats,longId));mats.stringID="mats";
    // Restore one actual owned receipt, then prune its dead actor metadata.
    ++epoch;Invalidate();actor.inv.counts[&mats]=0;project.state.needs[&mats]=3;
    collection.subject=pile.handle;pile.isInInventory=false;pile.owner.setNull();collection.vptr=image+0x16BDE38;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);EngineerMaterialsRuntime::Pickup(&collection,&body);
    actor.dead=true;maintenanceAt=0;Maintenance();CHECK(ListOwned(rows,128)==0&&actor.inv.counts[&mats]==100);
    actor.dead=false;++epoch;Invalidate();volatile LONG total=0;Race races[8];HANDLE threads[8];
    for(unsigned i=0;i<8;++i)
    {
        races[i].request=competing;races[i].request.actor=Of(hand(i+100,1,CHARACTER,1,1));
        races[i].request.epoch=epoch;races[i].request.now=GetTickCount();races[i].request.need=100;
        races[i].request.available=100;races[i].request.carried=0;races[i].total=&total;
        threads[i]=CreateThread(NULL,0,RaceCommit,&races[i],0,NULL);CHECK(threads[i]!=NULL);
    }
    CHECK(WaitForMultipleObjects(8,threads,TRUE,10000)==WAIT_OBJECT_0);
    for(unsigned i=0;i<8;++i)CloseHandle(threads[i]);CHECK(total==100);
    // Native actors can query the same single ground item before either goal
    // commits. The first commitment wins; neither loser executes its move,
    // including after the item has entered the winning engineer's inventory.
    ++epoch;Invalidate();project.state.needs[&mats]=1;Item lone(901,&mats,1);
    Character win(300),lose1(301),lose2(302);AI winAi,loseAi1,loseAi2;AITaskSytem winSys,loseSys1,loseSys2;
    CharBody winBody,loseBody1,loseBody2;Connect(win,winAi,winSys,winBody);Connect(lose1,loseAi1,loseSys1,loseBody1);Connect(lose2,loseAi2,loseSys2,loseBody2);
    Tasker moves[3],collections[3];CharBody* bodies[3]={&winBody,&loseBody1,&loseBody2};AITaskSytem* systems[3]={&winSys,&loseSys1,&loseSys2};
    for(unsigned i=0;i<3;++i)
    {
        moves[i].vptr=image+0x16BCD88;moves[i].taskData=&moveData;moves[i].subject=lone.handle;
        collections[i].vptr=image+0x16BDE38;collections[i].taskData=&collectData;collections[i].subject=lone.handle;
        bodies[i]->action=&moves[i];systems[i]->actions.list.push_back(&collections[i]);systems[i]->actions.list.push_back(&delivery);
        EngineerMaterialsRuntime::Selected(systems[i],&builder,1,NORMAL);
    }
    CHECK(deniedCount==2&&loseSys1.replans==0&&loseSys2.replans==0); // no destruction inside native runGoals
    winBody.action=&collections[0];const unsigned beforePickup=pickups;
    EngineerMaterialsRuntime::Pickup(&collections[0],&winBody);CHECK(pickups==beforePickup+1&&win.inv.counts[&mats]==1&&lone.isInInventory);
    const unsigned beforeMoves=moveAttempts,beforeEnds=endedActions;
    EngineerMaterialsRuntime::Body(&loseBody1,.1f);EngineerMaterialsRuntime::Body(&loseBody2,.1f);
    CHECK(moveAttempts==beforeMoves&&endedActions==beforeEnds+2&&loseSys1.replans==1&&loseSys2.replans==1);
    CHECK(!loseBody1.action&&!loseBody2.action&&deniedCount==0);
    // The source is already in another inventory BEFORE native setCurrentGoal
    // completes. It is managed-unavailable, not native capacity fallback.
    ++epoch;Invalidate();lone.isInInventory=true;loseBody1.action=&moves[1];
    EngineerMaterialsRuntime::Selected(&loseSys1,&builder,1,NORMAL);
    const unsigned heldMoves=moveAttempts,heldEnds=endedActions;
    EngineerMaterialsRuntime::Body(&loseBody1,.1f);
    CHECK(moveAttempts==heldMoves&&endedActions==heldEnds+1&&!loseBody1.action);
    // A source can become held, or its project's final demand can disappear,
    // during the walk after a successful commitment. Revalidate only that
    // committed witness before its next native move, at most once per250ms.
    ++epoch;Invalidate();lone.isInInventory=false;lone.owner.setNull();loseBody1.action=&moves[1];
    EngineerMaterialsRuntime::Selected(&loseSys1,&builder,1,NORMAL);CHECK(deniedCount==0);
    lone.isInInventory=true;const unsigned changedMoves=moveAttempts,changedEnds=endedActions;
    EngineerMaterialsRuntime::Body(&loseBody1,.1f);
    CHECK(moveAttempts==changedMoves&&endedActions==changedEnds+1&&!loseBody1.action);
    ++epoch;Invalidate();lone.isInInventory=false;lone.owner.setNull();loseBody1.action=&moves[1];
    EngineerMaterialsRuntime::Selected(&loseSys1,&builder,1,NORMAL);
    moves[1].subject=project.handle; // leftover collect does not own this unrelated Move
    lone.isInInventory=true;const unsigned unrelatedMoves=moveAttempts;
    EngineerMaterialsRuntime::Body(&loseBody1,.1f);
    CHECK(moveAttempts==unrelatedMoves+1&&loseBody1.action==&moves[1]);
    moves[1].subject=lone.handle;
    ++epoch;Invalidate();lone.isInInventory=false;lone.owner.setNull();loseBody1.action=&moves[1];
    EngineerMaterialsRuntime::Selected(&loseSys1,&builder,1,NORMAL);CHECK(deniedCount==0);
    project.state.needs[&mats]=0;const unsigned completedMoves=moveAttempts,completedEnds=endedActions;
    EngineerMaterialsRuntime::Body(&loseBody1,.1f);
    CHECK(moveAttempts==completedMoves&&endedActions==completedEnds+1&&!loseBody1.action);
    // No useful demand at selection time has the same managed-unavailable
    // outcome; it must never receive the native2 table-capacity fallback.
    ++epoch;Invalidate();loseBody1.action=&moves[1];
    EngineerMaterialsRuntime::Selected(&loseSys1,&builder,1,NORMAL);
    const unsigned emptyMoves=moveAttempts;EngineerMaterialsRuntime::Body(&loseBody1,.1f);
    CHECK(moveAttempts==emptyMoves&&!loseBody1.action);project.state.needs[&mats]=1;
    // A native delivery-only plan reserves all already carried types without
    // treating that personal inventory as newly acquired module-owned stock.
    ++epoch;Invalidate();GameData steel("steel");Building cargoProject(12);
    Building::ConstructionState::BuildMaterial ni,ns;ni.mat=&iron;ns.mat=&steel;
    Building::ConstructionState::BuildMaterial* mixedNeeds[3]={&need,&ni,&ns};
    cargoProject.state.mats.stuff=mixedNeeds;cargoProject.state.mats.count=cargoProject.state.mats.maxSize=3;
    cargoProject.state.needs[&mats]=10;cargoProject.state.needs[&iron]=8;cargoProject.state.needs[&steel]=4;
    win.inv.counts[&mats]=3;win.inv.counts[&iron]=5;win.inv.counts[&steel]=4;
    // The original goal can still name the previous machine when native GOAP
    // has already committed the delivery for the next one.
    Tasker cargoGoal,cargoDelivery;cargoGoal.taskData=&builderData;cargoGoal.currentSubTarget=project.handle;
    cargoDelivery.taskData=&deliverData;cargoDelivery.vptr=image+0x16BDEF8;cargoDelivery.subject=cargoProject.handle;
    winSys.actions.list.clear();winSys.actions.list.push_back(&cargoDelivery);winBody.action=&moves[0];moves[0].subject=cargoProject.handle;
    EngineerMaterialsRuntime::Selected(&winSys,&cargoGoal,1,NORMAL);
    Material mi,ms,mm;MaterialOf(&mats,mm);MaterialOf(&iron,mi);MaterialOf(&steel,ms);
    const unsigned when=GetTickCount();CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),mm,10,when,epoch)==7);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),mi,8,when,epoch)==3);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),ms,4,when,epoch)==0);CHECK(ListOwned(rows,128)==0);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(project.handle),mm,10,when,epoch)==10);
    Tasker build=cargoDelivery;TaskData buildData={BUILD};build.taskData=&buildData;build.vptr=image+0x16BDE78;
    winBody.action=&build;CHECK(SelectedProject(&winBody,&win)==&cargoProject);
    winBody.action=&cargoDelivery;CHECK(SelectedProject(&winBody,&win)==&cargoProject);
    build.subject=project.handle;winBody.action=&moves[0];
    winSys.actions.list.push_back(&build);CHECK(!SelectedProject(&winBody,&win));winSys.actions.list.pop_back();
    winSys.actions.list.push_front(&cargoGoal);CHECK(!SelectedProject(&winBody,&win));winSys.actions.list.pop_front();
    collections[0].subject=store.handle;collections[0].vptr=image+0x16BD7E0;
    store.inv.counts[&mats]=100;store.inv.counts[&iron]=0;
    winSys.actions.list.push_front(&collections[0]);EngineerMaterialsRuntime::Selected(&winSys,&cargoGoal,1,NORMAL);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),mm,10,when,epoch)==0);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),mi,8,when,epoch)==3);
    CHECK(coordinator.Demand(Of(lose1.handle),Of(cargoProject.handle),ms,4,when,epoch)==0);
    // Capacity exhaustion preserves ordinary movement and ordinary native2
    // collection; it neither bricks the job nor evicts another actor's lease.
    ++epoch;Invalidate();
    for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)
    {
        Request full;full.actor=Of(hand(1000+i,1,CHARACTER,1,1));full.project=Of(project.handle);full.source=full.actor;
        full.material=mm;full.need=1000000;full.carried=1;full.epoch=epoch;full.now=GetTickCount();
        CHECK(Cargo(full)==1);
    }
    winBody.action=&moves[0];EngineerMaterialsRuntime::Selected(&winSys,&cargoGoal,1,NORMAL);
    CHECK(deniedCount==0);const unsigned capacityMoves=moveAttempts;EngineerMaterialsRuntime::Body(&winBody,.1f);CHECK(moveAttempts==capacityMoves+1);
    winBody.action=&collections[0];const int cargoBefore=win.inv.counts[&mats];EngineerMaterialsRuntime::Collect(&collections[0],&winBody);
    CHECK(win.inv.counts[&mats]==cargoBefore+2);
    // Each machine contains the other recipe material as an input. Both
    // engineers must collect their source's output instead of pledging inputs.
    ++epoch;Invalidate();
    Building mixedSite(998);
    mixedSite.state.mats.stuff=mixedNeeds;mixedSite.state.mats.count=mixedSite.state.mats.maxSize=2;
    mixedSite.state.needs[&mats]=6;mixedSite.state.needs[&iron]=6;
    StorageBuilding ironOutput(992),matsOutput(993);
    ironOutput.output=&iron;matsOutput.output=&mats;
    ironOutput.inv.counts[&iron]=ironOutput.inv.counts[&mats]=6;
    matsOutput.inv.counts[&iron]=matsOutput.inv.counts[&mats]=6;
    Item ironStack(994,&iron,6);
    Item* machineStocks[]={&sourceStack,&ironStack};
    ironOutput.inv.all.stuff=matsOutput.inv.all.stuff=machineStocks;
    ironOutput.inv.all.count=ironOutput.inv.all.maxSize=2;
    matsOutput.inv.all.count=matsOutput.inv.all.maxSize=2;
    actor.inv.counts.clear();second.inv.counts.clear();
    builder.currentSubTarget=mixedSite.handle;
    Tasker collectIron=collection,collectMats=collection;
    collectIron.subject=ironOutput.handle;collectMats.subject=matsOutput.handle;
    collectIron.vptr=collectMats.vptr=image+0x16BD7E0;
    Tasker mixedDelivery=delivery;mixedDelivery.subject=mixedSite.handle;
    system.actions.list.clear();system.actions.list.push_back(&mixedDelivery);
    system2.actions.list.push_back(&mixedDelivery);
    body.action=&collectIron;body2.action=&collectMats;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    EngineerMaterialsRuntime::Selected(&system2,&builder,1,NORMAL);
    material=&iron;storage=&ironOutput;EngineerMaterialsRuntime::Collect(&collectIron,&body);
    CHECK(actor.inv.counts[&iron]==6&&ironOutput.inv.counts[&mats]==6);
    material=&mats;storage=&matsOutput;EngineerMaterialsRuntime::Collect(&collectMats,&body2);
    CHECK(second.inv.counts[&mats]==6&&matsOutput.inv.counts[&iron]==6);
    CHECK(liveOfferLists==0);
    // Large modded machine inventories stay on the native collection path.
    ++epoch;Invalidate();second.inv.counts.clear();matsOutput.inv.counts[&mats]=6;
    Item* oversized[129];for(unsigned i=0;i<129;++i)oversized[i]=&sourceStack;
    matsOutput.inv.all.stuff=oversized;matsOutput.inv.all.count=matsOutput.inv.all.maxSize=129;
    EngineerMaterialsRuntime::Selected(&system2,&builder,1,NORMAL);
    EngineerMaterialsRuntime::Collect(&collectMats,&body2);
    CHECK(second.inv.counts[&mats]==2&&matsOutput.inv.counts[&mats]==4);
    CHECK(deniedCount==0&&liveOfferLists==0);
    // Haulers can consume stock while the engineer approaches. As long as a
    // few units remain the trip is still useful, and other sources can serve
    // the part that is no longer covered.
    ++epoch;Invalidate();actor.inv.counts.clear();second.inv.counts.clear();
    matsOutput.inv.all.stuff=machineStocks;
    matsOutput.inv.all.count=matsOutput.inv.all.maxSize=2;
    matsOutput.inv.counts[&mats]=6;store.inv.counts[&mats]=6;
    move.subject=matsOutput.handle;body.action=&move;
    system.actions.list.clear();system.actions.list.push_back(&collectMats);system.actions.list.push_back(&mixedDelivery);
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    Request unpledged;
    CHECK(RequestFor(&second,&mixedSite,store.handle,&mats,unpledged));
    CHECK(Preview(unpledged)==0);
    matsOutput.inv.counts[&mats]=2;
    EngineerMaterialsRuntime::Body(&body,.1f);
    CHECK(body.action==&move&&Preview(unpledged)==4);
    // A delivery that can already reach the site supersedes an incoming
    // promise. Collection still fetches the uncovered part when it arrives.
    ++epoch;Invalidate();matsOutput.inv.counts[&mats]=6;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    second.inv.counts[&mats]=4;
    Tasker readyDelivery=delivery,deliveryWalk=move;
    readyDelivery.vptr=image+0x16BDEF8;
    readyDelivery.subject=deliveryWalk.subject=mixedSite.handle;
    body2.action=&deliveryWalk;
    system2.actions.list.clear();system2.actions.list.push_back(&readyDelivery);
    EngineerMaterialsRuntime::Selected(&system2,&builder,1,NORMAL);
    EngineerMaterialsRuntime::Body(&body2,.1f);
    CHECK(body2.action==&deliveryWalk);
    body.action=&collectMats;EngineerMaterialsRuntime::Collect(&collectMats,&body);
    CHECK(actor.inv.counts[&mats]==2&&matsOutput.inv.counts[&mats]==4);
    // A parked engineer's real cargo can fill the reservation table for the
    // last ingredient. Another engineer with that ingredient must still be
    // allowed to execute the delivery vanilla selected.
    ++epoch;Invalidate();
    actor.inv.counts[&mats]=6;second.inv.counts[&mats]=1;
    system.actions.list.clear();system.actions.list.push_back(&readyDelivery);body.action=&deliveryWalk;
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    body2.action=&deliveryWalk;
    EngineerMaterialsRuntime::Selected(&system2,&builder,1,NORMAL);
    EngineerMaterialsRuntime::Body(&body2,.1f);
    CHECK(body2.action==&deliveryWalk); // was cancelled and reelected in a loop
    Tasker doorWalk=move;doorWalk.vptr=image+0x16BCEA8;
    body2.action=&doorWalk;
    system2.actions.list.push_back(&collectMats);
    EngineerMaterialsRuntime::Selected(&system2,&builder,1,NORMAL);
    EngineerMaterialsRuntime::Body(&body2,.1f);
    CHECK(body2.action==&doorWalk&&DeliveryNext(&system2,Of(mixedSite.handle)));
    system2.actions.list.pop_front();
    CHECK(!DeliveryNext(&system2,Of(mixedSite.handle)));
    system.actions.list.clear();system.actions.list.push_back(&collectMats);system.actions.list.push_back(&mixedDelivery);
    // A producer's physical inventory can still contain the same material
    // elsewhere after the amount in the output section drops.
    ++epoch;Invalidate();actor.inv.counts.clear();second.inv.counts.clear();
    matsOutput.producer=true;matsOutput.outputUnits=6;matsOutput.inv.counts[&mats]=6;
    body.action=&move;EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    CHECK(RequestFor(&second,&mixedSite,store.handle,&mats,unpledged));
    CHECK(Preview(unpledged)==0);
    matsOutput.outputUnits=2;
    EngineerMaterialsRuntime::Body(&body,.1f);
    CHECK(body.action==&move&&Preview(unpledged)==4);
    // The multi-output provider no longer advertises the required material.
    // An abandoned construction source leaves both native haul cache rows intact.
    ++epoch;Invalidate();matsOutput.producer=false;matsOutput.output=&iron;
    mixedSite.state.needs[&iron]=0;
    ai.resultsCache.resourceSourceCache[&mats]=matsOutput.handle;
    ai.resultsCache.resourceSourceCache[&iron]=store.handle;
    body.action=&collectMats;EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    CHECK(ai.resultsCache.resourceSourceCache.count(&mats)==1&&Of(ai.resultsCache.resourceSourceCache[&mats])==Of(matsOutput.handle));
    CHECK(ai.resultsCache.resourceSourceCache.count(&iron)==1);
    // The native goal and move for the completed project can still be live
    // when another engineer starts the next machine from the same store.
    ++epoch;Invalidate();material=&mats;storage=&store;store.inv.counts[&mats]=4;
    actor.inv.counts.clear();second.inv.counts.clear();
    Building finishedSite(998),nextSite(999);
    finishedSite.state.mats.stuff=nextSite.state.mats.stuff=needs;
    finishedSite.state.mats.count=nextSite.state.mats.count=1;
    finishedSite.state.needs[&mats]=nextSite.state.needs[&mats]=4;
    Tasker nextBuilder=builder,storeCollection=collectMats,finishedDelivery=delivery,nextDelivery=delivery;
    builder.currentSubTarget=finishedSite.handle;nextBuilder.currentSubTarget=nextSite.handle;
    finishedDelivery.subject=finishedSite.handle;nextDelivery.subject=nextSite.handle;
    storeCollection.subject=move.subject=store.handle;
    body.action=&move;system.actions.list.clear();system.actions.list.push_back(&storeCollection);system.actions.list.push_back(&finishedDelivery);
    EngineerMaterialsRuntime::Selected(&system,&builder,1,NORMAL);
    CHECK(EngineerMaterialsRuntime::Find(&ai2,nextSite.handle,out,true)==0&&out.isNull());
    Request staleFetch,unrelatedFetch;
    CHECK(RequestFor(&second,&finishedSite,store.handle,&mats,staleFetch));
    CHECK(RequestFor(&second,&nextSite,store.handle,&mats,unrelatedFetch));
    finishedSite.state.isComplete=true;
    CompleteProject(Of(finishedSite.handle),epoch);
    const unsigned completion=ProjectRevision(Of(finishedSite.handle));
    CompleteProject(Of(finishedSite.handle),epoch);
    CHECK(ProjectRevision(Of(finishedSite.handle))==completion);
    CHECK(Commit(staleFetch)==0); // this worker had no claim at completion
    CHECK(Commit(unrelatedFetch)==4);
    Cancel(second.handle);
    CHECK(system.goal.subtarget.index==finishedSite.handle.index&&body.action==&move);
    CHECK(EngineerMaterialsRuntime::Find(&ai2,nextSite.handle,out,true)==1&&out.index==store.handle.index);
    body2.action=&storeCollection;system2.actions.list.clear();system2.actions.list.push_back(&nextDelivery);
    EngineerMaterialsRuntime::Selected(&system2,&nextBuilder,1,NORMAL);
    completeOnTake=&nextSite;
    EngineerMaterialsRuntime::Collect(&storeCollection,&body2);
    CHECK(!completeOnTake&&store.inv.counts[&mats]==3&&second.inv.counts[&mats]==1);
    CHECK(ListOwned(rows,128)==1&&rows[0].project==Of(nextSite.handle)&&rows[0].owned==1);
    const unsigned finishEnd=endedActions;
    EngineerMaterialsRuntime::Body(&body,.1f);
    CHECK(body.action==NULL&&endedActions==finishEnd+1&&store.inv.counts[&mats]==3);
    Building unclaimedSite(1000);
    unclaimedSite.state.mats.stuff=needs;unclaimedSite.state.mats.count=1;
    unclaimedSite.state.needs[&mats]=3;
    CHECK(RequestFor(&actor,&unclaimedSite,store.handle,&mats,staleFetch));
    unclaimedSite.state.isComplete=true;
    CompleteProject(Of(unclaimedSite.handle),epoch,true); // registry observed genuine completion with no claims
    CHECK(Commit(staleFetch)==0);
    CheckWindows();
    CHECK(nativeCalls>100);std::printf("engineer materials runtime: %u checks, %u native boundaries PASS\n",checks,nativeCalls);
    return 0;
}
