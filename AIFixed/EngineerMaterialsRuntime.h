#pragma once
#include "EngineerMaterialsPolicy.h"
#include <Windows.h>
#include <stdint.h>
#include <cmath>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/StorageBuilding.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Character.h>
#include <kenshi/CharBody.h>
#include <kenshi/GameData.h>
#include <kenshi/Globals.h>
#include <kenshi/GlobalConstants.h>
#include <kenshi/CharStats.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Tasker.h>
#include <core/Functions.h>
#include <Debug.h>
#include <intrin.h>

// Native callbacks execute in Character::threadedUpdate, not the GUI phase.
// The lock below protects only fixed POD tables. Every native call is outside it.
namespace EngineerMaterialsRuntime
{
    typedef AIFixedEngineer::Identity Identity;
    typedef AIFixedEngineer::Material Material;
    typedef AIFixedEngineer::Request Request;
    typedef AIFixedEngineer::Owned OwnedSurplus;
    typedef bool (*Predicate)();
    static Predicate admitted=NULL,debugging=NULL;
    // Host supplies a pure atomic LONG read. This is the sole callback allowed
    // under our table lock; it must never call game code, allocate or log.
    static LONG (*readEpoch)()=NULL;
    static void (*selectedCallback)(AITaskSytem*,Tasker*)=NULL;
    static void (*projectStarted)(Building*,unsigned)=NULL;
    static bool (*productiveProject)(Building*)=NULL;
    static CRITICAL_SECTION mutex;
    static volatile LONG initialized=0,ready=0;
    enum { CompletionStripes=64 };
    static volatile LONG completionRevision[CompletionStripes]={};
    static volatile LONG64 deliveryChecks[64]={};
    static unsigned generation=0;
    static AIFixedEngineer::Coordinator coordinator;
    struct DeniedMove
    {
        Identity actor,project,source,subject;unsigned epoch,created,moveClass,pendingClass;
        int key,pendingKey,start,end,priority;float x,y,z,weight;bool used;
        DeniedMove():epoch(0),created(0),moveClass(0),pendingClass(0),key(0),pendingKey(0),start(0),end(0),priority(0),x(0),y(0),z(0),weight(0),used(false){}
    };
    static DeniedMove denied[AIFixedEngineer::Coordinator::Capacity];
    static volatile LONG deniedCount=0;
    static uintptr_t image=0;
    static unsigned expiredAt=0;
    static unsigned maintenanceAt=0,maintenanceCursor=0;
    struct Guard
    {
        Guard(){EnterCriticalSection(&mutex);}~Guard(){LeaveCriticalSection(&mutex);}
    private:Guard(const Guard&);Guard& operator=(const Guard&);
    };
    inline Identity Of(const hand& h)
    {
        Identity x;
        if(h.type==NULL_ITEM)return x;
        x.type=h.type;x.container=h.container;x.containerSerial=h.containerSerial;x.index=h.index;x.serial=h.serial;
        return x;
    }
    inline hand Handle(const Identity& h)
    {return hand(h.index,h.serial,static_cast<itemType>(h.type),h.container,h.containerSerial);}
    inline bool MaterialOf(GameData* data,Material& out)
    {
        if(!data||data->stringID.empty()||data->stringID.size()>=sizeof(out.id)||
            data->stringID.find('\0')!=std::string::npos)return false;
        out=Material();out.type=data->type;
        std::memcpy(out.id,data->stringID.c_str(),data->stringID.size());return true;
    }
    inline bool Matches(GameData* data,const Material& material)
    {Material x;return MaterialOf(data,x)&&x==material;}
    static volatile LONG* CompletionStripe(const Identity& h)
    {
        const unsigned hash=(h.index*0x9e3779b9u)^(h.container*0x85ebca6bu)^
            h.serial^(h.containerSerial*31u)^h.type;
        return &completionRevision[(hash^(hash>>16))&(CompletionStripes-1)];
    }
    static unsigned ProjectRevision(const Identity& h)
    {return static_cast<unsigned>(InterlockedCompareExchange(CompletionStripe(h),0,0));}
    static bool DeliveryCheckDue(const Identity& actor,unsigned e,unsigned now)
    {
        // This table throttles observations only. It owns no claims, and a
        // collision can at most cause an extra check or a one-second delay.
        const unsigned key=(actor.index*0x9e3779b9u)^(actor.container*0x85ebca6bu)^
            actor.serial^(actor.containerSerial*31u)^(e*0x9e3779b9u);
        volatile LONG64* slot=&deliveryChecks[(key^(key>>16))&63];
        const LONG64 previous=InterlockedCompareExchange64(slot,0,0);
        const unsigned __int64 bits=static_cast<unsigned __int64>(previous);
        if(previous&&static_cast<unsigned>(bits>>32)==key&&static_cast<int>(now-static_cast<unsigned>(bits))<1000)
            return false;
        const LONG64 next=static_cast<LONG64>((static_cast<unsigned __int64>(key)<<32)|now);
        return InterlockedCompareExchange64(slot,next,previous)==previous;
    }
    static unsigned Epoch(){return readEpoch?static_cast<unsigned>(readEpoch()):0;}
    static void ClearDenied()
    {for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)denied[i].used=false;InterlockedExchange(&deniedCount,0);}
    static void ClearDenied(const Identity& actor)
    {for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)if(denied[i].used&&denied[i].actor==actor)
        {denied[i].used=false;InterlockedDecrement(&deniedCount);}}
    static void ExpireDenied(unsigned now)
    {for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)if(denied[i].used&&unsigned(now-denied[i].created)>=120000)
        {denied[i].used=false;InterlockedDecrement(&deniedCount);}}
    static bool Active()
    {return InterlockedCompareExchange(&ready,0,0)&&admitted&&readEpoch&&admitted();}
    static bool Sync(unsigned epoch)
    {
        // A hook that was on-stack during load must never rewind the state
        // published by a newer generation. The host epoch is monotonic LONG.
        if(epoch!=Epoch())return false;
        if(generation!=epoch&&static_cast<LONG>(epoch-generation)>0)
        {coordinator.Reset(epoch);ClearDenied();generation=epoch;}
        return generation==epoch;
    }
    inline void Invalidate()
    {
        if(!InterlockedCompareExchange(&initialized,0,0))return;
        const unsigned e=Epoch();Guard lock;
        if(e==Epoch()&&(e==generation||static_cast<LONG>(e-generation)>0)){coordinator.Reset(e);ClearDenied();generation=e;}
    }
    inline void Cancel(const hand& actor)
    {if(InterlockedCompareExchange(&initialized,0,0)){const unsigned e=Epoch();const Identity id=Of(actor);Guard lock;if(Sync(e)){coordinator.Cancel(id);ClearDenied(id);}}}
    static void CancelAt(const Identity& actor,unsigned e)
    {Guard lock;if(Sync(e)){coordinator.Cancel(actor);ClearDenied(actor);}}
    inline void CompleteProject(const Identity& project,unsigned e,bool newlyObserved=false)
    {
        Guard lock;
        if(!Sync(e)||!project.Valid())return;
        const bool retired=coordinator.CompleteProject(project,e);
        if(retired||newlyObserved)InterlockedIncrement(CompletionStripe(project));
    }
    inline unsigned ListOwned(OwnedSurplus* out,unsigned capacity)
    {
        if(!out||!capacity||!InterlockedCompareExchange(&initialized,0,0))return 0;
        const unsigned e=Epoch();Guard lock;return Sync(e)?coordinator.ListOwned(out,capacity):0;
    }
    inline unsigned ListOwnedFor(const Identity& actor,OwnedSurplus* out,unsigned capacity,unsigned e)
    {
        if(!Active())return 0;
        Guard lock;return Sync(e)?coordinator.ListOwnedFor(actor,out,capacity):0;
    }
    inline bool CheckOwned(const OwnedSurplus& row,int total,int& allowed)
    {
        allowed=0;if(!Active())return false;const unsigned e=Epoch();Guard lock;if(!Sync(e))return false;
        return coordinator.CheckOwned(row,total,allowed);
    }
    inline void Returned(const OwnedSurplus& row,int moved,int afterTotal)
    {const unsigned e=Epoch();Guard lock;if(Sync(e))coordinator.Returned(row,moved,afterTotal);}
    inline void ForgetOwned(const OwnedSurplus& row)
    {if(InterlockedCompareExchange(&initialized,0,0)){Guard lock;coordinator.ForgetOwned(row);}}
    inline void SetSelectedCallback(void (*callback)(AITaskSytem*,Tasker*))
    {selectedCallback=callback;}
    inline void SetProjectPriority(void (*started)(Building*,unsigned),bool (*productive)(Building*))
    {projectStarted=started;productiveProject=productive;}
    inline void Init(Predicate allow,Predicate debug,LONG (*epoch)())
    {
        // Plugin initialization happens once, before hooks become reachable.
        if(InterlockedCompareExchange(&initialized,1,0)==0)InitializeCriticalSection(&mutex);
        admitted=allow;debugging=debug;readEpoch=epoch;Invalidate();
    }

    static int InventoryCount(Character* actor,GameData* material)
    {
        if(!actor||!material)return -1;Inventory* inv=actor->getInventory();if(!inv)return -1;
        int count=inv->countItems(material);if(count<0||count>1000000)return -1;
        ContainerItem* bag=actor->hasABackpackOn();
        if(bag)
        {
            Inventory* other=bag->getInventory();
            if(other&&other!=inv){int n=other->countItems(material);if(n<0||n>1000000-count)return -1;count+=n;}
        }
        return count;
    }
    inline int Count(Character* actor,GameData* material){return InventoryCount(actor,material);}
    static GameData* FindOwnedMaterial(Character* actor,const Material& material)
    {
        Inventory* lists[2]={actor->getInventory(),NULL};ContainerItem* bag=actor->hasABackpackOn();
        if(bag)lists[1]=bag->getInventory();
        for(unsigned j=0;j<2;++j)if(lists[j]&&(j==0||lists[j]!=lists[0]))
        {
            const lektor<Item*>& all=lists[j]->getAllItems();if(all.size()>1024)return NULL;
            for(unsigned i=0;i<all.size();++i)
            {Item* item=all[i];if(item&&Matches(item->getGameData(),material))return item->getGameData();}
        }
        return NULL;
    }
    static bool HasEngineerJob(Character* actor)
    {
        OrdersReceiver* orders=actor->getOrdersReciever();if(!orders)return false;
        if(orders->getCurrentGoal().key()==JOB_BUILDER)return true;
        if(orders->jobs.size()>64||orders->permajobs.size()>64)return false;
        for(unsigned i=0;i<orders->jobs.size();++i)if(orders->jobs[i]&&orders->jobs[i]->key()==JOB_BUILDER)return true;
        for(unsigned i=0;i<orders->permajobs.size();++i)if(orders->permajobs[i]&&orders->permajobs[i]->key()==JOB_BUILDER)return true;
        return false;
    }
    inline void Maintenance()
    {
        // Host calls this in its admitted owner/main phase. At most8 distinct
        // metadata rows per250ms; no new world/character census, no item moves.
        if(!Active())return;const unsigned now=GetTickCount();
        if(unsigned(now-maintenanceAt)<250)return;maintenanceAt=now;
        const unsigned e=Epoch();OwnedSurplus rows[8];unsigned count=0;
        {Guard lock;if(Sync(e))count=coordinator.ListOwnedSlice(rows,8,maintenanceCursor);}
        for(unsigned i=0;i<count;++i)
        {
            if(e!=Epoch())return;Character* actor=Handle(rows[i].actor).getCharacter();
            if(!actor||actor->isDead()||!actor->isPlayerCharacter()||!HasEngineerJob(actor)){ForgetOwned(rows[i]);continue;}
            GameData* material=FindOwnedMaterial(actor,rows[i].material);const int total=material?Count(actor,material):0;
            int ignored=0;CheckOwned(rows[i],total,ignored);
        }
    }
    static bool Eligible(Character* actor)
    {
        if(!Active()||!actor||!actor->isPlayerCharacter()||actor->isInCombatMode(true,true))return false;
        OrdersReceiver* orders=actor->getOrdersReciever();
        return orders&&orders->isJobsEnabled()&&!orders->hasPlayerOrders();
    }
    static bool ExactClass(Tasker* task,bool ground)
    {
        if(!task||!image||!task->taskData||task->taskData->key!=COLLECT_OUTPUT_RESOURCE_BUILD_MATS)return false;
        const uintptr_t expected=image+(ground?0x16BDE38:0x16BD7E0);
        return *reinterpret_cast<uintptr_t*>(task)==expected;
    }
    static bool SupportedProject(Building* project)
    {
        Building::ConstructionState* state=project?project->getBuildState():NULL;
        if(!state||state->mats.size()>16)return false;
        for(unsigned i=0;i<state->mats.size();++i)
        {Material m;if(!state->mats[i]||!MaterialOf(state->mats[i]->mat,m))return false;}
        return true;
    }
    static int StorageBudget(Character* actor,Inventory* source,GameData* material)
    {
        // Exact native8838B0 uses these live constants and modified strength.
        // Current weight is the native getter (including carried people and
        // any admitted KEO correction). Future units get NO backpack discount.
        if(!actor||!source||!con||reinterpret_cast<uintptr_t>(con)!=image+0x2132D70)return 2;
        CharStats* stats=actor->getStats();if(!stats)return 2;
        const float strength=stats->getStat(STAT_STRENGTH,false);
        const float capacity=con->ENCUMBRANCE_BASE+con->CARRY_WEIGHT_MULT*strength;
        const float carried=actor->getTotalCarryWeight();float unit=0;
        const lektor<Item*>& items=source->getAllItems();
        if(items.size()>1024||items.size()>items.maxSize||(items.size()&&!items.stuff))return 2;
        for(unsigned i=0;i<items.size();++i)
        {
            Item* item=items[i];if(!item||item->quantity<=0||item->getGameData()!=material)continue;
            const float weight=item->getItemWeightSingle();if(!_finite(weight)||weight<=0)return 2;
            if(weight>unit)unit=weight;
        }
        return AIFixedEngineer::StorageCapacity(capacity,carried,unit);
    }
    static bool ExactSingleOutput(StorageBuilding* storage)
    {
        const uintptr_t* table=*reinterpret_cast<const uintptr_t* const*>(storage);
        static const unsigned char thunk[]={0xe9,0xe6,0x18,0x25,0x00};
        static const unsigned char offerSingle[]={
            0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x01,0x48,0x8b,0xda,
            0x48,0x8b,0xf9,0xff,0x90,0x80,0x05,0x00,0x00,0x84,0xc0,0x75,0x2f,0x48,0x8b,0x07,
            0x48,0x8b,0xcf,0xff,0x90,0x50,0x05,0x00,0x00,0x8b,0x53,0x0c,0x48,0x8b,0xf8,0x39,
            0x53,0x08,0x72,0x0a,0x03,0xd2,0x48,0x8b,0xcb,0xe8,0xca,0xf1,0xd7,0xff,0x8b,0x53,
            0x08,0x48,0x8b,0x4b,0x10,0x48,0x89,0x3c,0xd1,0xff,0x43,0x08,0x48,0x8b,0x5c,0x24,
            0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3
        };
        return table[0x548/sizeof(uintptr_t)]==image+0x481D5&&
            !std::memcmp(reinterpret_cast<const void*>(image+0x481D5),thunk,sizeof(thunk))&&
            !std::memcmp(reinterpret_cast<const void*>(image+0x299AC0),offerSingle,sizeof(offerSingle));
    }
    static bool RequestFor(Character* actor,Building* project,const hand& source,GameData* material,Request& r,
                           unsigned expectedEpoch=~0u,bool weigh=true,bool verifyOutput=false)
    {
        const unsigned e=Epoch();if(expectedEpoch!=~0u&&expectedEpoch!=e)return false;
        if(!actor||!project||!material||!MaterialOf(material,r.material))return false;
        r.project=Of(project->handle);
        // Only actual acquisition requests need a fence. Bulk native
        // previews do not need an atomic revision read for every source.
        if(weigh)r.completionRevision=ProjectRevision(r.project);
        Building::ConstructionState* state=project->getBuildState();if(!state||state->isComplete)return false;
        r.need=state->needsMat(material);if(r.need<=0)return false;
        r.carried=InventoryCount(actor,material);if(r.carried<0)return false;
        r.actor=Of(actor->handle);r.source=Of(source);
        r.epoch=e;r.now=GetTickCount();r.ground=false;r.capacity=32;
        if(source.type==BUILDING)
        {
            Building* building=source.getBuilding();if(!building)return false;
            StorageBuilding* storage=building->getFunctionStuff();if(!storage)return false;
            // Task247 takes items from the subject building's inventory.
            Inventory* inventory=building->getInventory();if(!inventory)return false;
            r.available=inventory->countItems(material);
            if(verifyOutput&&r.available>0&&ExactSingleOutput(storage))
            {
                if(storage->isProductionEmpty()||storage->getProductionItemData()!=material)return false;
                const int units=storage->getCurrentProductionQuantity();
                if(units<=0)return false;
                if(units<r.available)r.available=units;
            }
            if(weigh)r.capacity=StorageBudget(actor,inventory,material);
        }
        else
        {
            Item* item=source.getItem();
            if(!item||item->isInInventory||!item->getInventoryWeAreIn().isNull()||
                item->getGameData()!=material)return false;
            r.ground=true;r.available=item->quantity;
        }
        return e==Epoch()&&r.available>0&&r.available<=1000000;
    }
    static int Preview(const Request& r)
    {Guard lock;return Sync(r.epoch)?coordinator.Preview(r):0;}
    static bool RequestCurrent(const Request& r)
    {return Sync(r.epoch)&&r.completionRevision==ProjectRevision(r.project);}
    static int Commit(const Request& r,bool acquiring=false)
    {Guard lock;return RequestCurrent(r)?coordinator.Commit(r,acquiring):0;}
    static int Grant(const Request& r)
    {Guard lock;return RequestCurrent(r)?coordinator.Grant(r):0;}
    static bool Has(const Request& r)
    {Guard lock;return Sync(r.epoch)&&coordinator.Has(r);}
    static bool HasReceipt(const Request& r)
    {Guard lock;return Sync(r.epoch)&&coordinator.HasReceipt(r);}
    static bool CanTrack(const Request& r)
    {Guard lock;return Sync(r.epoch)&&coordinator.CanTrack(r);}
    static int Cargo(const Request& r)
    {Guard lock;return Sync(r.epoch)?coordinator.Cargo(r):0;}
    static bool SourceWatch(const Identity& actor,const Identity& project,const Identity& source,unsigned now,unsigned e,Request& out)
    {Guard lock;return Sync(e)&&coordinator.SourceWatch(actor,project,source,now,e,out);}
    static int RevalidateSource(const Request& previous,const Request& current)
    {Guard lock;return Sync(current.epoch)?coordinator.RevalidateSource(previous,current):0;}
    static bool ProjectNeedsCovered(Character* actor,Building* project,unsigned e)
    {
        if(e!=Epoch())return false;
        Building::ConstructionState* state=project->getBuildState();
        if(!state||state->isComplete||state->mats.size()>16||e!=Epoch())return false;
        const Identity who=Of(actor->handle),where=Of(project->handle);
        if(!who.Valid()||!where.Valid())return false;
        const unsigned now=GetTickCount();
        bool hasNeed=false;
        for(unsigned i=0;i<state->mats.size();++i)
        {
            Building::ConstructionState::BuildMaterial* m=state->mats[i];
            Material material;
            if(!m||!MaterialOf(m->mat,material))return false;
            const int units=state->needsMat(m->mat);
            if(units>1000000)return false;
            if(units<=0)continue;
            hasNeed=true;
            Guard lock;
            if(!Sync(e)||coordinator.Demand(who,where,material,units,now,e)>0)return false;
        }
        return hasNeed&&e==Epoch();
    }
    struct Query
    {
        AI* ai;Building* project;GameData* material;Query* previous;unsigned epoch;bool unmanaged;
        Query(AI* a,Building* p,GameData* m,unsigned e);
        ~Query();
    };
    // Borrowed native pointers exist only on this synchronous hook stack.
    static __declspec(thread) Query* query=NULL;
    inline Query::Query(AI* a,Building* p,GameData* m,unsigned e):ai(a),project(p),material(m),previous(query),epoch(e),unmanaged(false){query=this;}
    inline Query::~Query(){query=previous;}
    struct DestinationQuery
    {
        AI* ai;DestinationQuery* previous;unsigned epoch;
        DestinationQuery(AI* a,unsigned e=~0u);~DestinationQuery();
    };
    static __declspec(thread) DestinationQuery* destinationQuery=NULL;
    inline DestinationQuery::DestinationQuery(AI* a,unsigned e):ai(a),previous(destinationQuery),epoch(e==~0u?Epoch():e){destinationQuery=this;}
    inline DestinationQuery::~DestinationQuery(){destinationQuery=previous;}
    struct Transfer
    {
        Tasker* action;CharBody* body;Building* project;GameData* material;Transfer* previous;Request request;
        Identity receipt;int receiptBefore,receiptUnits;bool hasRequest,hasReceipt;unsigned epoch;
        Transfer(Tasker* t,CharBody* b,Building* p,unsigned e);
        ~Transfer();
    };
    static __declspec(thread) Transfer* transfer=NULL;
    inline Transfer::Transfer(Tasker* t,CharBody* b,Building* p,unsigned e):action(t),body(b),project(p),material(NULL),previous(transfer),
        receiptBefore(-1),receiptUnits(0),hasRequest(false),hasReceipt(false),epoch(e){transfer=this;}
    inline Transfer::~Transfer(){transfer=previous;}

    static float (*originalFind)(AI*,const hand&,hand&,bool)=NULL;
    static float (*originalDestination)(AI*,const hand&,hand&,bool)=NULL;
    static bool (*originalDestinationPath)(void*,const hand&)=NULL;
    static float (*originalSource)(AI::AIResultsCacher*,hand&,GameData*,StorageBuilding*)=NULL;
    static bool (*originalGroundSafe)(AI*,Item*)=NULL;
    static void (*originalMachines)(AI*,lektor<Building*>&,unsigned)=NULL;
    static void (*originalSelected)(AITaskSytem*,Tasker*,float,taskPriority)=NULL;
    static int (*originalQuantity)(Tasker*,Character*,GameData*,StorageBuilding*)=NULL;
    static void (*originalCollect)(Tasker*,CharBody*)=NULL;
    static void (*originalPickup)(Tasker*,CharBody*)=NULL;
    static bool (*originalAdd)(Inventory*,Item*,int)=NULL;
    static Item* (*originalTake)(Inventory*,GameData*)=NULL;
    static void (*originalDeliver)(Tasker*,CharBody*)=NULL;
    static void (*originalBody)(CharBody*,float)=NULL;

    static float Destination(AI* ai,const hand& in,hand& out,bool asking)
    {
        const unsigned e=Epoch();
        if(!ai||!Eligible(ai->me))return originalDestination(ai,in,out,asking);
        DestinationQuery scope(ai,e);return originalDestination(ai,in,out,asking);
    }
    static bool DestinationPath(void* finder,const hand& target)
    {
        const bool impossible=originalDestinationPath(finder,target);
        if(impossible||!destinationQuery||destinationQuery->epoch!=Epoch()||!finder||!Active()||
            *reinterpret_cast<uintptr_t*>(finder)!=image+0x1740910)return impossible;
        Character* actor=destinationQuery->ai->me;
        if(!actor||*reinterpret_cast<Character**>(static_cast<unsigned char*>(finder)+0x20)!=actor)return impossible;
        Building* project=target.getBuilding();if(!project)return impossible;
        // The productive-building selector handles waiting after native site
        // checks. Keep its candidate visible even when all materials are booked.
        if(!ProjectNeedsCovered(actor,project,destinationQuery->epoch))return impossible;
        return !productiveProject||!productiveProject(project);
    }

    static float FindUnfiltered(AI* ai,const hand& in,hand& out,bool asking)
    {
        if(!query)return originalFind(ai,in,out,asking);
        Query withoutParent(NULL,NULL,NULL,query->epoch);
        return originalFind(ai,in,out,asking);
    }
    static float Find(AI* ai,const hand& in,hand& out,bool asking)
    {
        const unsigned e=Epoch();
        if(!ai||!Eligible(ai->me))return FindUnfiltered(ai,in,out,asking);
        Building* project=in.getBuilding();if(e!=Epoch()||!project||!SupportedProject(project))return FindUnfiltered(ai,in,out,asking);
        Query scope(ai,project,NULL,e);return originalFind(ai,in,out,asking);
    }
    static bool GroundSafe(AI* ai,Item* item)
    {
        const uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
        const bool native=originalGroundSafe(ai,item);
        if(!native||caller!=image+0x59C7B5||!query||query->ai!=ai||query->epoch!=Epoch()||!query->material||!Active())return native;
        Request r;return RequestFor(ai->me,query->project,item->handle,query->material,r,query->epoch,false)&&Preview(r)>0;
    }
    static bool SourcePathFailed(AI* ai,const hand& source)
    {
        return source.type==BUILDING&&ai->taskSystemAI&&ai->taskSystemAI->isPathImpossible(source);
    }
    static void Machines(AI* ai,lektor<Building*>& out,unsigned flags)
    {
        const uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
        originalMachines(ai,out,flags);
        if(caller!=image+0x59C892||!query||query->ai!=ai||query->epoch!=Epoch()||!query->material||flags!=0x04002436||!Active())return;
        if(out.size()>256){query->unmanaged=true;return;}
        unsigned kept=0;
        for(unsigned i=0;i<out.size();++i)
        {
            Building* b=out[i];Request r;
            if(b&&RequestFor(ai->me,query->project,b->handle,query->material,r,query->epoch,false)&&
                !SourcePathFailed(ai,b->handle)&&Preview(r)>0)out.stuff[kept++]=b;
        }
        out.count=kept;
    }
    typedef std::map<GameData*,hand,std::less<GameData*>,std::allocator<std::pair<GameData* const,hand> > > ResourceSources;
    struct MachineOutputs
    {
        lektor<GameData*> offered;
        ~MachineOutputs()
        {
            // The SDK lektor does not release its storage. Pair the native
            // allocation with the same game delete[] used by task247.
            if(offered.stuff)reinterpret_cast<void (*)(void*)>(image+0xED562E)(offered.stuff);
        }
    };
    static bool ReadMachineOffers(StorageBuilding* storage,MachineOutputs& outputs)
    {
        Inventory* inventory=storage->getInventory();if(!inventory)return false;
        const lektor<Item*>& stock=inventory->getAllItems();
        const unsigned limit=128;
        if(stock.size()>limit||stock.size()>stock.maxSize||(stock.size()&&!stock.stuff))return false;
        storage->getItemsWeWantRidOf(outputs.offered,false);
        const lektor<GameData*>& offers=outputs.offered;
        return offers.size()<=limit&&offers.size()<=offers.maxSize&&(!offers.size()||offers.stuff);
    }
    static bool CachedSourceOffered(const hand& source,GameData* material,unsigned e)
    {
        if(source.type!=BUILDING)return true;
        Building* building=source.getBuilding();StorageBuilding* storage=building?building->getFunctionStuff():NULL;
        if(!storage||e!=Epoch())return false;
        if(ExactSingleOutput(storage))return true; // RequestFor checks the exact single output and quantity.
        MachineOutputs outputs;
        if(!ReadMachineOffers(storage,outputs))return e==Epoch(); // Preserve an uninspectable native source.
        if(e!=Epoch())return false;
        for(unsigned i=0;i<outputs.offered.size();++i)if(outputs.offered[i]==material)return true;
        return false;
    }
    struct SourceScratch
    {
        __declspec(align(16)) unsigned char memory[sizeof(AI::AIResultsCacher)];
        AI::AIResultsCacher* cache;
        explicit SourceScratch(AI* ai):cache(reinterpret_cast<AI::AIResultsCacher*>(memory)->_CONSTRUCTOR(ai)){}
        ~SourceScratch(){cache->_DESTRUCTOR();}
    private:
        SourceScratch(const SourceScratch&);SourceScratch& operator=(const SourceScratch&);
    };
    static float Source(AI::AIResultsCacher* self,hand& out,GameData* material,StorageBuilding* skip)
    {
        const uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
        Material key;
        if(caller!=image+0x5A20D0||!self||!query||query->material||query->ai!=self->ai||
            query->epoch!=Epoch()||!Eligible(self->ai->me)||!MaterialOf(material,key))
        {
            if(!query||!query->material)return originalSource(self,out,material,skip);
            // If native code re-enters an ordinary source lookup while a
            // construction search is active, hide the outer candidate filter.
            Query unfiltered(NULL,NULL,NULL,query->epoch);
            return originalSource(self,out,material,skip);
        }
        // This cache belongs to the actor, and native hauling reads it too.
        // A cold key gets the same unfiltered native answer as vanilla.
        // Construction may reuse it, but must never overwrite an entry or
        // publish its reservation-filtered alternative in the native cache.
        const unsigned e=query->epoch;
        const ResourceSources::const_iterator it=self->resourceSourceCache.find(material);
        if(!skip)
        {
            const bool cold=it==self->resourceSourceCache.end();
            float native=1.0f;
            if(cold)
            {
                native=originalSource(self,out,material,skip);
                if(e!=Epoch()){out.setNull();return 0;}
                if(native<=0)return native;
            }
            const hand source=cold?out:it->second;
            Request cached;
            if(!SourcePathFailed(self->ai,source)&&RequestFor(self->ai->me,query->project,source,material,cached,e,false,true)&&
                Preview(cached)>0&&(cold||CachedSourceOffered(source,material,e))&&e==Epoch())
            {out=source;return native;}
        }
        if(e!=Epoch()){out.setNull();return 0;}
        float score=0;bool unmanaged=false;
        {
            SourceScratch scratch(self->ai);
            if(e!=Epoch()){out.setNull();return 0;}
            Query scope(self->ai,query->project,material,e);
            score=originalSource(scratch.cache,out,material,skip);
            unmanaged=scope.unmanaged;
        }
        if(e!=Epoch()){out.setNull();return 0;}
        if(!unmanaged&&score>0)
        {
            Request r;
            if(SourcePathFailed(self->ai,out)||!RequestFor(self->ai->me,query->project,out,material,r,e,false)||Preview(r)<=0)
            {out.setNull();score=0;}
        }
        if(e!=Epoch()){out.setNull();return 0;}
        if(!Active())
        {
            Query unfiltered(NULL,NULL,NULL,e);
            score=originalSource(self,out,material,skip);
            if(e!=Epoch()){out.setNull();return 0;}
            return score;
        }
        return score;
    }
    static Tasker* PendingCollection(AITaskSytem* system)
    {
        if(!system||!system->body)return NULL;Tasker* action=system->body->getCurrentAction();
        if(action&&action->key()==COLLECT_OUTPUT_RESOURCE_BUILD_MATS)return action;
        const size_t count=system->actions.list.size();if(count>8)return NULL;
        for(size_t i=0;i<count;++i)
        {
            Tasker* task=system->actions.list[i];
            if(task&&task->key()==COLLECT_OUTPUT_RESOURCE_BUILD_MATS)return task;
        }
        return NULL;
    }
    static bool Plan(Character* actor,Building* project,Tasker* collection,Request& r,unsigned e,bool* unmanaged=NULL)
    {
        if(!actor||!project||!collection||!collection->taskData||
            collection->taskData->key!=COLLECT_OUTPUT_RESOURCE_BUILD_MATS)return false;
        const hand source=collection->subject;
        if(ExactClass(collection,true))
        {
            Item* item=source.getItem();return item&&RequestFor(actor,project,source,item->getGameData(),r,e);
        }
        if(!ExactClass(collection,false))return false;
        Building* building=source.getBuilding();
        StorageBuilding* storage=building?building->getFunctionStuff():NULL;
        if(!storage||e!=Epoch())return false;
        // A machine's inventory also contains inputs. The native collection
        // action can pick only from the materials the source offers to it.
        MachineOutputs outputs;
        if(!ReadMachineOffers(storage,outputs))
        {if(unmanaged)*unmanaged=true;return false;}
        for(unsigned i=0;i<outputs.offered.size()&&e==Epoch();++i)
        {
            if(RequestFor(actor,project,source,outputs.offered[i],r,e,true,true)&&Preview(r)>0)return true;
        }
        return false;
    }
    static void Replan(CharBody* body)
    {
        // Native owns action destruction and GOAP priority. Never clear jobs,
        // insert a higher-rank job, or call into GOAP while holding our lock.
        if(body&&body->ai&&body->ai->taskSystemAI)body->ai->taskSystemAI->taskImpossible();
    }
    static void CommitCargo(Character* actor,Building* project,unsigned e)
    {
        Building::ConstructionState* state=project->getBuildState();
        if(!state||state->mats.size()>16)return;
        const Identity who=Of(actor->handle),where=Of(project->handle);
        if(state->isComplete){CompleteProject(where,e);return;}
        for(unsigned i=0;i<state->mats.size()&&e==Epoch();++i)
        {
            Building::ConstructionState::BuildMaterial* needed=state->mats[i];if(!needed)continue;
            Request r;if(!MaterialOf(needed->mat,r.material))continue;
            r.need=state->needsMat(needed->mat);r.carried=InventoryCount(actor,needed->mat);
            if(r.need<=0||r.carried<=0)continue;
            r.actor=who;r.project=where;r.source=who;r.available=r.carried;r.epoch=e;r.now=GetTickCount();
            Cargo(r);
        }
    }
    static unsigned MoveClass(Tasker* action)
    {
        if(!action)return 0;const uintptr_t v=*reinterpret_cast<uintptr_t*>(action);
        const unsigned allowed[]={0x16BCD88,0x16BCEA8,0x16BCF28,0x16BCFA8,0x16BD028,0x16BD0A8};
        for(unsigned i=0;i<sizeof(allowed)/sizeof(allowed[0]);++i)if(v==image+allowed[i])return allowed[i];
        return 0; // In particular, never cancel the combat MoveBowRange class.
    }
    static bool NativePending(Tasker* pending)
    {
        return ExactClass(pending,true)||ExactClass(pending,false);
    }
    static bool ConstructionWork(Tasker* action)
    {
        if(!action||!action->taskData)return false;
        const uintptr_t table=*reinterpret_cast<const uintptr_t*>(action);
        return (table==image+0x16BDE78&&action->taskData->key==BUILD)||
            (table==image+0x16BDEF8&&action->taskData->key==ADD_MATERIALS_TO_BUILDING);
    }
    inline Building* SelectedProject(CharBody* body,Character* actor)
    {
        AITaskSytem* system=body&&body->ai?body->ai->taskSystemAI:NULL;
        if(!system||system->body!=body||system->character!=actor||!Eligible(actor)||
            actor->getOrdersReciever()->getCurrentGoal().key()!=JOB_BUILDER)return NULL;
        // The goal's subtarget can still name an earlier site. Native BUILD
        // and ADD execute against their own subject in the committed plan.
        Tasker* active=body->getCurrentAction();
        const hand* target=NULL;
        if(ConstructionWork(active))target=&active->subject;
        else if(MoveClass(active)||NativePending(active))
        {
            const size_t count=system->actions.list.size();
            if(count>8)return NULL;
            for(size_t i=0;i<count;++i)
            {
                Tasker* step=system->actions.list[i];
                if(ConstructionWork(step))
                {
                    if(target&&Of(*target)!=Of(step->subject))return NULL;
                    target=&step->subject;
                }
                else if(!MoveClass(step)&&!NativePending(step))break;
            }
        }
        if(!target||target->type!=BUILDING)return NULL;
        Building* project=target->getBuilding();
        return project&&Of(project->handle)==Of(*target)?project:NULL;
    }
    static bool DeliveryNext(AITaskSytem* system,const Identity& project)
    {
        if(!system||system->actions.list.size()>8)return false;
        for(size_t i=0;i<system->actions.list.size();++i)
        {
            Tasker* t=system->actions.list[i];
            if(NativePending(t))return false;
            if(t&&t->key()==ADD_MATERIALS_TO_BUILDING&&Of(t->subject)==project&&
               *reinterpret_cast<uintptr_t*>(t)==image+0x16BDEF8)return true;
        }
        return false;
    }
    static bool DeliveryMove(AITaskSytem* system,const Identity& site,Tasker* action,unsigned kind)
    {
        return kind&&(kind!=0x16BCD88||Of(action->subject)==site)&&DeliveryNext(system,site);
    }
    static void DenyMove(AITaskSytem* system,Building* project,Tasker* pending,unsigned e)
    {
        if(!system||!system->character||!system->body||!NativePending(pending)||e!=Epoch())return;
        CharBody* body=system->body;Tasker* action=body->getCurrentAction();const unsigned kind=MoveClass(action);
        if(!kind||*reinterpret_cast<uintptr_t*>(body)!=image+0x16F7938)return;
        if(kind==0x16BCD88&&Of(action->subject)!=Of(pending->subject))return;
        DeniedMove mark;mark.actor=Of(system->character->handle);mark.project=Of(project->handle);
        if(DeliveryMove(system,mark.project,action,kind))return;
        mark.source=Of(pending->subject);mark.subject=Of(action->subject);mark.moveClass=kind;
        mark.pendingClass=static_cast<unsigned>(*reinterpret_cast<uintptr_t*>(pending)-image);
        mark.key=action->key();mark.pendingKey=pending->key();mark.start=action->startTime;mark.end=action->endTime;
        mark.priority=action->priority;mark.weight=action->weight;
        mark.x=action->location.x;mark.y=action->location.y;mark.z=action->location.z;mark.epoch=e;mark.created=GetTickCount();
        if(!mark.source.Valid()||!_finite(mark.x)||!_finite(mark.y)||!_finite(mark.z)||!_finite(mark.weight))return;
        Guard lock;if(!Sync(e))return;ExpireDenied(mark.created);
        for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)
            if(!denied[i].used||denied[i].actor==mark.actor)
            {if(!denied[i].used)InterlockedIncrement(&deniedCount);mark.used=true;denied[i]=mark;return;}
        // Bounded metadata exhaustion retains ordinary native movement.
    }
    static void StopDeniedMove(CharBody* body,Character* actor,Building* project,unsigned e)
    {
        if(!project||!InterlockedCompareExchange(&deniedCount,0,0))return;
        const Identity who=Of(actor->handle);DeniedMove mark;
        {
            Guard lock;if(!Sync(e))return;
            for(unsigned i=0;i<AIFixedEngineer::Coordinator::Capacity;++i)if(denied[i].used&&denied[i].actor==who)
            {mark=denied[i];denied[i].used=false;InterlockedDecrement(&deniedCount);break;}
        }
        if(!mark.used||mark.epoch!=e||e!=Epoch()||mark.project!=Of(project->handle)||
            *reinterpret_cast<uintptr_t*>(body)!=image+0x16F7938)return;
        AITaskSytem* system=body->ai?body->ai->taskSystemAI:NULL;
        Tasker* pending=PendingCollection(system);
        Tasker* action=body->getCurrentAction();
        if(!NativePending(pending)||pending->key()!=mark.pendingKey||Of(pending->subject)!=mark.source||
            *reinterpret_cast<uintptr_t*>(pending)!=image+mark.pendingClass||MoveClass(action)!=mark.moveClass||
            action->key()!=mark.key||Of(action->subject)!=mark.subject||action->startTime!=mark.start||
            action->endTime!=mark.end||action->priority!=mark.priority||action->weight!=mark.weight||
            action->location.x!=mark.x||action->location.y!=mark.y||action->location.z!=mark.z)return;
        if(e!=Epoch()||body->getCurrentAction()!=action||SelectedProject(body,actor)!=project)return;
        if(DeliveryMove(system,mark.project,action,mark.moveClass))return;
        // Defer native destruction until the owning body phase. Native
        // endAction calls the admitted Move end/stop and deletes only this
        // ownership-confirmed current action; jobs and goal rank stay native.
        body->endAction();if(e==Epoch()){CancelAt(who,e);Replan(body);}
    }
    static void ValidateSourceMove(CharBody* body,Character* actor,Building* project,unsigned e)
    {
        Tasker* action=body->getCurrentAction();const unsigned kind=MoveClass(action);
        if(!kind||!body->ai)return;
        AITaskSytem* system=body->ai->taskSystemAI;const Identity site=Of(project->handle);
        if(DeliveryMove(system,site,action,kind))
        {
            const unsigned now=GetTickCount();const Identity who=Of(actor->handle);
            if(DeliveryCheckDue(who,e,now))
            {Guard lock;if(Sync(e))coordinator.ObserveDelivery(who,site,now,e);}
            return;
        }
        Tasker* pending=PendingCollection(system);
        if(!NativePending(pending)||(kind==0x16BCD88&&Of(action->subject)!=Of(pending->subject)))return;
        // A plain Move targets the source itself. The Door variants are
        // transitions on the admitted collection's native route.
        Request previous;
        if(!SourceWatch(Of(actor->handle),Of(project->handle),Of(pending->subject),GetTickCount(),e,previous))return;
        Building::ConstructionState* state=project->getBuildState();GameData* material=NULL;
        if(state&&state->mats.size()>16)return; // Unsupported modded shape retains native ownership.
        if(!state||state->isComplete)
        {
            if(state)CompleteProject(previous.project,e);
            DenyMove(system,project,pending,e);StopDeniedMove(body,actor,project,e);return;
        }
        for(unsigned i=0;i<state->mats.size();++i)
        {Building::ConstructionState::BuildMaterial* m=state->mats[i];if(m&&Matches(m->mat,previous.material)){material=m->mat;break;}}
        Request current;
        if(e==Epoch()&&(!material||SourcePathFailed(body->ai,pending->subject)||
            !RequestFor(actor,project,pending->subject,material,current,e,false,true)||RevalidateSource(previous,current)<=0))
        {
            DenyMove(system,project,pending,e);StopDeniedMove(body,actor,project,e);
        }
    }
    static void Selected(AITaskSytem* system,Tasker* task,float score,taskPriority priority)
    {
        const unsigned e=Epoch();
        originalSelected(system,task,score,priority);
        if(e!=Epoch())return;
        if(system&&system->character)
        {
            Character* actor=system->character;const hand who=actor->handle;
            CancelAt(Of(who),e);Building* project=SelectedProject(system->body,actor);Request r;
            if(project&&SupportedProject(project))
            {
                // A reservation limits how much more to collect. The
                // native delivery can use useful cargo already in a pack.
                CommitCargo(actor,project,e);Tasker* collection=PendingCollection(system);
                Tasker* action=system->body?system->body->getCurrentAction():NULL;
                if(collection&&DeliveryMove(system,Of(project->handle),action,MoveClass(action)))collection=NULL;
                bool advances=!collection;
                if(collection&&NativePending(collection))
                {
                    bool unmanaged=false;
                    const bool planned=Plan(actor,project,collection,r,e,&unmanaged);
                    if(unmanaged||((planned||r.actor.Valid())&&r.available>0&&r.need>0&&!CanTrack(r)))
                    {/* Unsupported source shape or exhausted claim table: keep native ownership. */}
                    else
                    {
                        advances=planned&&Commit(r)>0;
                        if(!advances)DenyMove(system,project,collection,e);
                    }
                }
                if(advances&&e==Epoch()&&projectStarted)projectStarted(project,e);
            }
        }
        if(e==Epoch()&&selectedCallback)selectedCallback(system,task);
    }
    static int Quantity(Tasker* task,Character* actor,GameData* material,StorageBuilding* source)
    {
        if(!transfer||transfer->epoch!=Epoch()||transfer->action!=task||transfer->body->getCurrentAction()!=task||
            transfer->body->getCharacter()!=actor||!ExactClass(task,false)||!Eligible(actor))
            return originalQuantity(task,actor,material,source);
        Building* project=SelectedProject(transfer->body,actor);
        if(!project||project!=transfer->project||!source)return 0;
        const hand target=task->subject;
        Building* building=target.getBuilding();
        if(!building||building->getFunctionStuff()!=source)return 0;
        Request r;if(!RequestFor(actor,project,target,material,r,transfer->epoch,true,true))return 0;
        // Native selected action is now executing. Its material may differ
        // from the conservative pre-trip proposal: atomically replace our
        // own claim; never subtract or displace another engineer's claim.
        int quantity=Grant(r);
        if(quantity>0){transfer->request=r;transfer->material=material;transfer->hasRequest=true;}
        return quantity;
    }
    static void Finish(Transfer& scope)
    {
        if(!scope.hasRequest||scope.request.epoch!=Epoch())return;
        Character* actor=Handle(scope.request.actor).getCharacter();
        const int total=InventoryCount(actor,scope.material);
        Guard lock;if(Sync(scope.request.epoch))coordinator.Finish(scope.request,total);
    }
    static void Collect(Tasker* task,CharBody* body)
    {
        const unsigned e=Epoch();
        Character* actor=body?body->getCharacter():NULL;Building* project=SelectedProject(body,actor);
        if(e!=Epoch()||!project||!ExactClass(task,false)||body->getCurrentAction()!=task||!SupportedProject(project))
        {originalCollect(task,body);return;}
        Request r;bool unmanaged=false;
        if(!Plan(actor,project,task,r,e,&unmanaged))
        {
            if(e==Epoch())
            {
                CancelAt(Of(actor->handle),e);
                if(unmanaged)originalCollect(task,body);else Replan(body);
            }
            return;
        }
        if(!CanTrack(r)){CancelAt(r.actor,e);originalCollect(task,body);return;}
        if(!Commit(r,true)){if(e==Epoch()){CancelAt(r.actor,e);Replan(body);}return;}
        Transfer scope(task,body,project,e);originalCollect(task,body);
        if(scope.hasRequest)Finish(scope);else CancelAt(r.actor,e);
    }
    static void Pickup(Tasker* task,CharBody* body)
    {
        const unsigned e=Epoch();
        Character* actor=body?body->getCharacter():NULL;Building* project=SelectedProject(body,actor);
        if(e!=Epoch()||!project||!ExactClass(task,true)||body->getCurrentAction()!=task||!SupportedProject(project))
        {originalPickup(task,body);return;}
        Request r;
        if(!Plan(actor,project,task,r,e)){if(e==Epoch()){Cancel(actor->handle);Replan(body);}return;}
        if(!CanTrack(r)){CancelAt(r.actor,e);originalPickup(task,body);return;}
        if(!Commit(r,true)){if(e==Epoch()){CancelAt(r.actor,e);Replan(body);}return;}
        // Native picks a whole ground stack. Reserve the whole source
        // exclusively, even when only a small part is useful to this project.
        // Plan and table admission call native boundaries. Resolve again with
        // the captured full identity before reading the pickup receipt.
        Item* item=Handle(r.source).getItem();GameData* data=item?item->getGameData():NULL;
        if(e!=Epoch()||!item||Of(item->handle)!=r.source||item->isInInventory||
            !item->getInventoryWeAreIn().isNull()||!Matches(data,r.material)||item->quantity!=r.available||!Has(r))
        {if(e==Epoch()){Cancel(actor->handle);Replan(body);}return;}
        Transfer scope(task,body,project,e);scope.request=r;scope.material=data;
        scope.hasRequest=true;scope.receipt=r.source;scope.receiptUnits=r.available;
        scope.receiptBefore=InventoryCount(actor,scope.material);scope.hasReceipt=true;
        originalPickup(task,body);Finish(scope);
    }
    static Item* Take(Inventory* inventory,GameData* material)
    {
        const uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
        Transfer* scope=transfer;bool witnessed=false;int before=-1,stock=-1;
        if(scope&&caller==image+0x33CF1C&&Active())
        {
            // The original shared runAction contains a class6 one-item
            // fallback when all quantity callbacks return0. Deny BEFORE the
            // native removal, otherwise that fallback bypasses reservations.
            if(scope->epoch!=Epoch()||!scope->hasRequest||!Matches(material,scope->request.material)||!Has(scope->request))return NULL;
            Building* source=Handle(scope->request.source).getBuilding();
            if(!source||source->getInventory()!=inventory||scope->epoch!=Epoch())return NULL;
            before=InventoryCount(scope->body->getCharacter(),material);stock=inventory->countItems(material);witnessed=true;
        }
        Item* item=originalTake(inventory,material);
        if(witnessed&&item&&scope->request.epoch==Epoch()&&item->quantity==1&&
            inventory->countItems(material)==stock-1&&Matches(item->getGameData(),scope->request.material))
        {
            scope->receipt=Of(item->handle);scope->receiptBefore=before;scope->receiptUnits=1;scope->hasReceipt=true;
        }
        return item;
    }
    static bool Add(Inventory* inventory,Item* item,int quantity)
    {
        Transfer* scope=transfer;GameData* material=NULL;int before=-1,units=0;
        Character* actor=scope&&scope->epoch==Epoch()?scope->body->getCharacter():NULL;
        bool witness=scope&&scope->epoch==Epoch()&&scope->hasRequest&&scope->hasReceipt&&actor&&item&&Active()&&
            scope->body->getCurrentAction()==scope->action&&Of(item->handle)==scope->receipt;
        if(witness)
        {
            material=item->getGameData();units=item->quantity;
            Inventory* main=actor->getInventory();ContainerItem* bag=actor->hasABackpackOn();
            Inventory* backpack=bag?bag->getInventory():NULL;
            witness=(inventory==main||inventory==backpack)&&Matches(material,scope->request.material)&&
                units>0&&units<=scope->receiptUnits&&HasReceipt(scope->request);
            if(witness){before=InventoryCount(actor,material);witness=before==scope->receiptBefore;}
        }
        // Includes KEO's direct worn-backpack tryAddItem route. Hook order at
        // Character::giveItem cannot hide this actual acquisition boundary.
        const bool result=originalAdd(inventory,item,quantity);
        if(witness&&scope->hasReceipt&&before>=0&&scope->request.epoch==Epoch())
        {
            const int after=InventoryCount(actor,material);
            const int moved=after>=before?after-before:0;
            if(result&&moved>0&&moved<=units)
            {
                scope->hasReceipt=false;
                Guard lock;if(Sync(scope->request.epoch))coordinator.Acquired(scope->request,moved,before,after);
            }
            else if(after!=before)
            {
                // A failed/partial or reentrant transfer is not an acquisition
                // receipt. Native state is retained; ambiguous credit is retired.
                OwnedSurplus rows[AIFixedEngineer::Coordinator::Capacity];unsigned count=ListOwned(rows,AIFixedEngineer::Coordinator::Capacity);
                for(unsigned i=0;i<count;++i)if(rows[i].actor==scope->request.actor&&rows[i].material==scope->request.material)ForgetOwned(rows[i]);
                scope->hasReceipt=false;
                CancelAt(scope->request.actor,scope->request.epoch);
            }
        }
        return result;
    }
    static void Deliver(Tasker* task,CharBody* body)
    {
        const unsigned e=Epoch();
        Character* actor=body?body->getCharacter():NULL;Building* project=SelectedProject(body,actor);
        if(e!=Epoch()||!project||!task||task->key()!=ADD_MATERIALS_TO_BUILDING||body->getCurrentAction()!=task||
            Of(project->handle)!=Of(task->subject)){originalDeliver(task,body);return;}
        struct Before{Material material;GameData* data;int count;};Before snapshots[16];unsigned size=0;
        const Identity actorId=Of(actor->handle);Building::ConstructionState* state=project->getBuildState();
        if(state&&state->mats.size()<=16)
            for(unsigned i=0;i<state->mats.size();++i)
            {
                Building::ConstructionState::BuildMaterial* m=state->mats[i];
                if(m&&MaterialOf(m->mat,snapshots[size].material))
                {snapshots[size].data=m->mat;snapshots[size].count=InventoryCount(actor,m->mat);++size;}
            }
        originalDeliver(task,body);
        if(e!=Epoch())return;
        for(unsigned i=0;i<size;++i)
        {
            const int after=InventoryCount(actor,snapshots[i].data);
            Guard lock;if(Sync(e))coordinator.Delivered(actorId,snapshots[i].material,snapshots[i].count,after);
        }
    }
    static void Body(CharBody* body,float time)
    {
        Character* actor=body?body->getCharacter():NULL;
        if(!actor||!actor->isPlayerCharacter()){originalBody(body,time);return;}
        const unsigned e=Epoch();
        if(Active())
        {
            const unsigned now=GetTickCount();
            if(e!=Epoch()){originalBody(body,time);return;}
            Building* project=SelectedProject(body,actor);
            if(!project)CancelAt(Of(actor->handle),e);
            else{StopDeniedMove(body,actor,project,e);ValidateSourceMove(body,actor,project,e);}
            {Guard lock;if(Sync(e)&&unsigned(now-expiredAt)>=1000){coordinator.Expire(now,e);ExpireDenied(now);expiredAt=now;}}
        }
        originalBody(body,time);
    }

    // Native contract table and Install are appended below. Exported ABI and
    // every private body are verified before any detour is installed.
}

namespace EngineerMaterialsRuntime
{
    struct NativeWindow{unsigned rva,size;unsigned __int64 fnv;};
    static const NativeWindow windows[]={
        {0x5A1FD0,32,0xECDAD4C5BC7F91DCui64}, // findBuildingMaterials
        {0x59C570,2466,0x681D05DB02E19B58ui64}, // complete getResourceSource for private cache receiver and scoped calls
        {0x5A20CB,5,0xD65C28F63E360C47ui64}, // construction caller of getResourceSource
        {0x167B308,4,0x4B72477F9C5C2F98ui64}, // native non-null cache hit score 1.0
        {0x640380,77,0xFDEBFFD670E6287Eui64}, // cache constructor
        {0x63F020,272,0x55C95217471243E0ui64}, // cache destructor
        {0x996EB0,32,0xE948DC3FCFB3251Dui64}, // groundSafety
        {0x8CAFD0,32,0xA950E9DFC678ABEAui64}, // machines
        {0x50C060,32,0x8F16911AF68911ADui64}, // selected
        {0x74B1E0,32,0x95AF3B5FEFA86FCFui64}, // actual native inventory acquisition
        {0x5C6290,32,0xFFC8BCE722FF773Dui64}, // body
        {0x749FF0,32,0x32BBB3D8E9ABD332ui64}, // take
        {0x341160,22,0x20B0408E2E6EC3E8ui64}, // complete private quantity body
        {0x33CCB0,812,0x8082ADD99E2EC7B1ui64}, // complete private collect body
        {0x34F2E0,1289,0xD7FB96BBCCC4F67Dui64}, // complete private pickup body
        {0x348810,739,0xBC047E0F34FDF635ui64}, // complete private deliver body
        {0x998190,32,0x3C6D01831BF88D8Fui64}, // destination
        {0x8CBD80,26,0x585500F46FE32AF1ui64}, // destinationPath
        {0x5AF800,172,0xC992A47FE39410A9ui64}, // AITaskSytem::isPathImpossible, native recent failures
        {0x3BA11,5,0x7C0457CAC6AF67CFui64}, // exact Inventory/ShopTrader virtual thunk
        {0x5C7F40,44,0x971672B7A82B80F8ui64}, // carryWeight
        {0xD20E0,9,0xE0372D5A580DDC7Cui64}, // singleWeight
        {0x8838B0,195,0xB320ADF15FE9E6DCui64}, // nativeEncumbrance
        {0x338050,85,0x185261970584FE1Aui64}, // modifiedStrength
        {0x883BF0,32,0x72C637B322C48516ui64}, // getStat
        {0x883CB9,35,0xCBBFE0FA217E6FE5ui64}, // getStatStrengthBranch
        {0x50BBA0,7,0x6A91FF449B4DD71Eui64}, // bodyEndDispatch
        {0x5C5CE0,60,0xCA862DFB46C88DC6ui64}, // bodyEndAction
        {0x354750,102,0x628356FE75A24369ui64}, // moveEndAction
        {0x332970,188,0xCC1DA0BFC99C0AB7ui64}, // doorMoveEndAction
        {0x50BE00,8,0x1BFEDCA503B6727Bui64}, // taskImpossible
        {0xED562E,6,0x85D4B9433D44322Cui64}, // native delete[] for temporary offer lists
    };
    static const unsigned NativeTimeStamp=0x65D604D7;
    static const unsigned NativeImageSize=0x232C000;

    static bool WindowMatches(const NativeWindow& w,const unsigned char* data)
    {
        unsigned __int64 hash=14695981039346656037ui64;
        for(unsigned i=0;i<w.size;++i){hash^=data[i];hash*=1099511628211ui64;}
        if(hash==w.fnv)return true;
        // Only known shared entries accept a five-byte KenshiLib jump.
        // In particular, KEP's fix_stealing_check already hooks GroundSafe.
        // Our wrapper calls that chain first and preserves a refusal.
        if(data[0]!=0xE9)return false;
        unsigned __int64 expected=0;
        if(w.size==32)
        {
            switch(w.rva)
            {
            case 0x5C6290:expected=0x4A4146B1228BC6F3ui64;break; // KEO frame audit
            case 0x998190:expected=0x071AA82295E69462ui64;break; // our priority wrapper
            case 0x996EB0:expected=0x0E666EF51CE4D95Cui64;break; // KEP stealing check
            default:return false;
            }
        }
        else if(w.rva==0x5C7F40&&w.size==44)expected=0xDB02F90F01634456ui64;
        else return false;
        hash=14695981039346656037ui64;
        for(unsigned i=5;i<w.size;++i){hash^=data[i];hash*=1099511628211ui64;}
        return hash==expected;
    }

    static bool NativeAdmission()
    {
        typedef char NativeConBaseOffset[offsetof(GlobalConstants,ENCUMBRANCE_BASE)==0x128?1:-1];
        typedef char NativeConMultiplierOffset[offsetof(GlobalConstants,CARRY_WEIGHT_MULT)==0x12C?1:-1];
        typedef char NativeSourceCacheOffset[offsetof(AI::AIResultsCacher,resourceSourceCache)==0x80?1:-1];
        typedef char NativeSourceCacheSize[sizeof(AI::AIResultsCacher)==0xA0?1:-1];
        HMODULE module=GetModuleHandleW(NULL);if(!module)return false;
        image=reinterpret_cast<uintptr_t>(module);
        const IMAGE_DOS_HEADER* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>4096)return false;
        const IMAGE_NT_HEADERS64* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
            nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
            nt->FileHeader.TimeDateStamp!=NativeTimeStamp||nt->OptionalHeader.SizeOfImage!=NativeImageSize)return false;
        for(unsigned n=0;n<sizeof(windows)/sizeof(windows[0]);++n)
        {
            const NativeWindow& w=windows[n];if(w.rva>NativeImageSize||w.size>NativeImageSize-w.rva)return false;
            const unsigned char* data=reinterpret_cast<const unsigned char*>(image+w.rva);
            if(!WindowMatches(w,data))
            {
                char message[128];
                sprintf_s(message,"AIFixed: engineer materials code at RVA 0x%X differs (first opcode 0x%02X)",
                    w.rva,static_cast<unsigned>(data[0]));
                ErrorLog(message);
                return false;
            }
        }
        // Factory selection and virtual slot identities are as important as
        // function heads: both action247 branches must be stock native classes.
        const uintptr_t* storage=reinterpret_cast<const uintptr_t*>(image+0x16BD7E0);
        const uintptr_t* pickup=reinterpret_cast<const uintptr_t*>(image+0x16BDE38);
        const uintptr_t* delivery=reinterpret_cast<const uintptr_t*>(image+0x16BDEF8);
        if(storage[2]!=image+0x43E3C||storage[5]!=image+0x3E52C||
            pickup[2]!=image+0x4C41A||delivery[2]!=image+0x29AF0||
            *reinterpret_cast<const uintptr_t*>(image+0x1740910+0x20)!=image+0x5305D||
            *reinterpret_cast<const uintptr_t*>(image+0x170BF88+0x18)!=image+0x3BA11||
            *reinterpret_cast<const uintptr_t*>(image+0x17367C8+0x18)!=image+0x3BA11)return false;
        // Native body-phase cancellation owns only these six noncombat Move
        // classes. Pin their deleting-destructor/run/end identities and every
        // intermediate E9, plus the exact body endAction dispatch slot. The
        // complete body/Move end implementations above are strict windows.
        struct NativeVirtual{unsigned table,slot,thunk,target;};
        static const NativeVirtual stopVirtuals[]={
            {0x16BCD88,0,0xB357,0x33A9B0},{0x16BCD88,1,0x2093C,0x332290},
            {0x16BCD88,2,0x2739,0x3545A0},{0x16BCD88,3,0x4DF68,0x354750},{0x16BCD88,4,0x3687C,0x3320F0},
            {0x16BCEA8,0,0x782E,0x33AB10},{0x16BCEA8,1,0x3CC0E,0x332820},
            {0x16BCEA8,2,0x20E73,0x3328D0},{0x16BCEA8,3,0x27D2C,0x332970},{0x16BCEA8,4,0x3687C,0x3320F0},
            {0x16BCF28,0,0x2D2F9,0x33AB60},{0x16BCF28,1,0x3CC0E,0x332820},
            {0x16BCF28,2,0x246AE,0x332BD0},{0x16BCF28,3,0x27D2C,0x332970},{0x16BCF28,4,0x3687C,0x3320F0},
            {0x16BCFA8,0,0x746E,0x33ABB0},{0x16BCFA8,1,0x3CC0E,0x332820},
            {0x16BCFA8,2,0xC58B,0x332E00},{0x16BCFA8,3,0x27D2C,0x332970},{0x16BCFA8,4,0x3687C,0x3320F0},
            {0x16BD028,0,0x22287,0x33AC00},{0x16BD028,1,0x3CC0E,0x332820},
            {0x16BD028,2,0xE3BD,0x333130},{0x16BD028,3,0x27D2C,0x332970},{0x16BD028,4,0x3687C,0x3320F0},
            {0x16BD0A8,0,0x2C0B6,0x33AC50},{0x16BD0A8,1,0x3CC0E,0x332820},
            {0x16BD0A8,2,0x20E73,0x3328D0},{0x16BD0A8,3,0x27D2C,0x332970},{0x16BD0A8,4,0x3687C,0x3320F0},
            {0x16BDE78,2,0x3642B,0x34A3E0},
            {0x16F7938,12,0x1127A,0x5C5CE0},
        };
        for(unsigned i=0;i<sizeof(stopVirtuals)/sizeof(stopVirtuals[0]);++i)
        {
            const NativeVirtual& v=stopVirtuals[i];
            if(*reinterpret_cast<const uintptr_t*>(image+v.table+v.slot*8)!=image+v.thunk)return false;
            const unsigned char* jump=reinterpret_cast<const unsigned char*>(image+v.thunk);int relative=0;
            std::memcpy(&relative,jump+1,sizeof(relative));
            if(jump[0]!=0xE9||static_cast<__int64>(v.thunk)+5+relative!=v.target)return false;
        }
        return KenshiLib::GetRealAddress(&AI::findBuildingMaterials)==intptr_t(image+0x5A1FD0)&&
            KenshiLib::GetRealAddress(&AI::AIResultsCacher::getResourceSource)==intptr_t(image+0x59C570)&&
            KenshiLib::GetRealAddress(&AI::AIResultsCacher::_CONSTRUCTOR)==intptr_t(image+0x640380)&&
            KenshiLib::GetRealAddress(&AI::AIResultsCacher::_DESTRUCTOR)==intptr_t(image+0x63F020)&&
            KenshiLib::GetRealAddress(&AI::itemOnGroundStealingCheckIsSafe)==intptr_t(image+0x996EB0)&&
            KenshiLib::GetRealAddress(&AI::getAllLocalMachines)==intptr_t(image+0x8CAFD0)&&
            KenshiLib::GetRealAddress(&AITaskSytem::_NV_setCurrentGoal)==intptr_t(image+0x50C060)&&
            KenshiLib::GetRealAddress(&Inventory::_NV_tryAddItem)==intptr_t(image+0x74B1E0)&&
            KenshiLib::GetRealAddress(&CharBody::_NV_update)==intptr_t(image+0x5C6290)&&
            KenshiLib::GetRealAddress(&Inventory::takeOneItemOnly)==intptr_t(image+0x749FF0)&&
            KenshiLib::GetRealAddress(&AI::findSomethingToBuild)==intptr_t(image+0x998190)&&
            KenshiLib::GetRealAddress(&Character::getTotalCarryWeight)==intptr_t(image+0x5C7F40)&&
            KenshiLib::GetRealAddress(&CharStats::getStat)==intptr_t(image+0x883BF0)&&
            KenshiLib::GetRealAddress(&CharBody::endAction)==intptr_t(image+0x50BBA0)&&
            KenshiLib::GetRealAddress(&AITaskSytem::taskImpossible)==intptr_t(image+0x50BE00)&&
            KenshiLib::GetRealAddress(&AITaskSytem::isPathImpossible)==intptr_t(image+0x5AF800)&&
            KenshiLib::GetRealAddress(&InventoryItemBase::_NV_getItemWeightSingle)==intptr_t(image+0xD20E0);
    }
    inline bool Install()
    {
        if(InterlockedCompareExchange(&ready,0,0))return true;
        if(!InterlockedCompareExchange(&initialized,0,0)||!admitted||!readEpoch||!NativeAdmission())
        {ErrorLog("AIFixed: engineer coordination disabled: native Steam1.0.65 contract mismatch");return false;}
        bool ok=true;
        // No detour can become active until every required hook is installed.
        // A partially installed set forwards to native while ready remains0.
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x5A1FD0),Find,&originalFind))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x998190),Destination,&originalDestination))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x8CBD80),DestinationPath,&originalDestinationPath))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x59C570),Source,&originalSource))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x996EB0),GroundSafe,&originalGroundSafe))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x8CAFD0),Machines,&originalMachines))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x50C060),Selected,&originalSelected))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x341160),Quantity,&originalQuantity))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x33CCB0),Collect,&originalCollect))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x34F2E0),Pickup,&originalPickup))&&ok;
        // Hook the common virtual-dispatch E9 thunk, NOT its74B1E0 body.
        // Vanilla giveItem and KEO backpack-first use this vtable slot; KEO
        // steel stocking calls the unchanged body directly with strict pins.
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x3BA11),Add,&originalAdd))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x749FF0),Take,&originalTake))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x348810),Deliver,&originalDeliver))&&ok;
        ok=(KenshiLib::SUCCESS==KenshiLib::AddHook(intptr_t(image+0x5C6290),Body,&originalBody))&&ok;
        if(!ok){ErrorLog("AIFixed: engineer coordination disabled: required hooks incomplete");return false;}
        Invalidate();InterlockedExchange(&ready,1);return true;
    }
}
