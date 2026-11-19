#pragma once
#include "EngineerSurplusPolicy.h"
#include "EngineerMaterialsRuntime.h"
// Included after EngineerMaterialsRuntime.h inside AIFixed's private namespace.
// Steam 1.0.65 only. Native AI callbacks own queries and actions; metadata locks
// never enclose native calls. Invalidated owned FillMachine tasks are tombstoned
// until their native scalar deleting destructor, never sent to whole-stack code.
namespace EngineerSurplusRuntime
{
    typedef AIFixedEngineer::Identity Identity;
    typedef EngineerMaterialsRuntime::OwnedSurplus Owned;
    enum { Capacity=128, OwnedCapacity=128, ItemLimit=1024 };
    bool (*admit)()=NULL,(*debug)()=NULL,(*allowed)(Character*)=NULL;
    LONG (*epoch)()=NULL;
    volatile LONG ready=0;
    unsigned char* image=NULL;
    SRWLOCK metadata=SRWLOCK_INIT;
    unsigned serial=0;
    __declspec(thread) unsigned depth=0;
    __declspec(thread) AITaskSytem* normalJobScope=NULL;
    __declspec(thread) unsigned planningToken=0;
    __declspec(thread) LONG planningEpoch=-1;
    ULONG_PTR ownedFillTable[6]={0,0,0,0,0,0};
    ULONG_PTR OwnedFillVtable(){return reinterpret_cast<ULONG_PTR>(&ownedFillTable[1]);}
    struct DepthScope{DepthScope(){++depth;}~DepthScope(){--depth;}private:DepthScope(const DepthScope&);DepthScope& operator=(const DepthScope&);};
    struct Episode
    {
        bool used,retired,committed,transaction,deleted,pending;
        unsigned token;LONG generation;int jobSlot;DWORD retryAt;
        Identity actor,jobSubject,storage,piece,sourceInventory;
        ULONG_PTR fill,jobTask; Owned credit;
        Episode():used(false),retired(false),committed(false),transaction(false),deleted(false),pending(false),token(0),generation(-1),jobSlot(-1),retryAt(0),fill(0),jobTask(0){}
    };
    Episode episodes[Capacity];
    struct Guard{Guard(){AcquireSRWLockExclusive(&metadata);}~Guard(){ReleaseSRWLockExclusive(&metadata);}};
    LONG Epoch(){return epoch?epoch():-1;}
    Identity Of(const hand& h){return EngineerMaterialsRuntime::Of(h);}
    bool Live(LONG e){return ready&&admit&&admit()&&e==Epoch();}
    bool ReadActor(const Identity& actor,Episode& out)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].actor==actor&&!episodes[i].retired){out=episodes[i];return true;}return false;
    }
    bool ReadFill(Tasker* t,Episode& out)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].fill==reinterpret_cast<ULONG_PTR>(t)){out=episodes[i];return true;}return false;
    }
    bool ReadToken(unsigned token,Episode& out)
    {Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token){out=episodes[i];return true;}return false;}
    void Retire(const Identity& actor)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].actor==actor)
        {episodes[i].retired=true;if(!episodes[i].fill&&!episodes[i].transaction&&!episodes[i].pending)episodes[i]=Episode();}
    }
    void Cancel(const hand& actor){Retire(Of(actor));}
    void Invalidate()
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used)
        {
            episodes[i].retired=true;
            // Only serial identities survive; old-world native ownership stays
            // with its factory. Never resolve an old receipt in a new world.
            if(episodes[i].generation!=Epoch())episodes[i].pending=false;
            if(!episodes[i].fill&&!episodes[i].transaction&&!episodes[i].pending)episodes[i]=Episode();
        }
    }
    bool Eligible(Character* c,LONG e)
    {
        if(!Live(e)||!c||!allowed||!allowed(c)||!c->isPlayerCharacter()||!c->isPhysicalMode||
           c->isUnconcious()||c->isCarryingSomething||c->isBeingCarried()||c->isInCombatMode(true,true))return false;
        OrdersReceiver* o=c->getOrdersReciever();
        // Native hasPlayerOrders deliberately excludes its two idle bookkeeping
        // tasks. hasOrders would permanently suppress otherwise idle engineers.
        return o&&o->isJobsEnabled()&&!o->hasPlayerOrders()&&Live(e);
    }
    int BuilderSlot(AITaskSytem* tasks,Tasker* t)
    {
        if(!tasks||!t||t->key()!=JOB_BUILDER||!t->taskData||t->taskData->permaJob!=PERMAJOB_NORMAL)return -1;
        const int n=tasks->getPermajobCount();if(n<0||n>128)return -1;
        for(int i=0;i<n;++i)if(tasks->getPermajobData(i)==t)return i;
        return -1;
    }
    bool JobStillThere(AITaskSytem* tasks,const Episode& e)
    {
        const int n=tasks->getPermajobCount();if(e.jobSlot<0||e.jobSlot>=n||n>128)return false;
        const Tasker* t=tasks->getPermajobData(e.jobSlot);
        return t&&reinterpret_cast<ULONG_PTR>(t)==e.jobTask&&t->key()==JOB_BUILDER&&Of(t->subject)==e.jobSubject;
    }
    Inventory* Backpack(Character* c)
    {ContainerItem* bag=c->hasABackpackOn();return bag?bag->getInventory():NULL;}
    int Total(Character* c,GameData* data){return EngineerMaterialsRuntime::Count(c,data);}
    bool Returnable(Character* c,const Owned& credit,GameData* data,LONG generation,int& units)
    {
        units=0;int proven=0;
        if(!Live(generation)||!credit.project.Valid()||!EngineerMaterialsRuntime::CheckOwned(credit,Total(c,data),proven)||proven<=0)return false;
        Building* project=EngineerMaterialsRuntime::Handle(credit.project).getBuilding();
        if(!Live(generation)||!project||Of(project->handle)!=credit.project)return false;
        Building::ConstructionState* state=project->getBuildState();
        if(!Live(generation)||!state)return false;
        const int needed=state->isComplete||state->isDismantled?0:state->needsMat(data);
        if(!Live(generation))return false;
        units=AIFixedEngineerSurplus::Surplus(proven,needed);return units>0;
    }
    GameData* DataIn(Inventory* inv,const AIFixedEngineer::Material& material)
    {
        if(!inv)return NULL;const lektor<Item*>& items=inv->getAllItems();if(items.size()>ItemLimit)return NULL;
        for(unsigned i=0;i<items.size();++i)
        {Item* p=items[i];if(p&&!p->isEquipped&&p->quantity>0&&EngineerMaterialsRuntime::Matches(p->getGameData(),material))return p->getGameData();}
        return NULL;
    }
    bool ValidStorage(Character* c,StorageBuilding* b,GameData* data,LONG e)
    {
        return b&&Live(e)&&b->getSpecialFunction()==BF_RESOURCE_STORAGE&&!b->isDisabled()&&
            b->getBuildState()&&b->getBuildState()->isComplete&&b->getFaction()==c->getFaction()&&
            b->specialItemTypesOnly==ITEM&&b->canHaveSomeOfThese(data)&&
            c->getAI()->isDestinationAccessible(b,false)&&Live(e);
    }
    __declspec(thread) char storageList[sizeof(lektor<Building*>)];
    struct SearchSlot
    {
        bool used;Identity actor;DWORD next;unsigned turn,cursor;
        SearchSlot():used(false),next(0),turn(0),cursor(0){}
    };
    SearchSlot searchSlots[Capacity];LONG searchGeneration=-1;DWORD searchWindow=0;unsigned searchCount=0;
    // @SURPLUS_SEARCH_BUDGET_BEGIN@
    bool SearchTicket(const Identity& actor,LONG generation,DWORD now,unsigned& material,unsigned& storage)
    {
        Guard g;
        if(searchGeneration!=generation)
        {for(unsigned i=0;i<Capacity;++i)searchSlots[i]=SearchSlot();searchGeneration=generation;searchWindow=now;searchCount=0;}
        if(now-searchWindow>=250){searchWindow=now;searchCount=0;}
        SearchSlot* slot=NULL;
        for(unsigned i=0;i<Capacity;++i)if(searchSlots[i].used&&searchSlots[i].actor==actor){slot=&searchSlots[i];break;}
        if(slot&&static_cast<LONG>(now-slot->next)<0)return false;
        // Do not stamp the actor when the shared quota is exhausted: actors
        // earlier in native iteration then cool down while later ones progress.
        if(searchCount>=4)return false;
        if(!slot)for(unsigned i=0;i<Capacity;++i)if(!searchSlots[i].used){slot=&searchSlots[i];slot->used=true;slot->actor=actor;break;}
        if(!slot)for(unsigned i=0;i<Capacity;++i)if(static_cast<LONG>(now-searchSlots[i].next)>=0)
        {slot=&searchSlots[i];*slot=SearchSlot();slot->used=true;slot->actor=actor;break;}
        if(!slot)return false;
        ++searchCount;slot->next=now+1500;material=slot->turn++;storage=slot->cursor;slot->cursor+=16;return true;
    }
    // @SURPLUS_SEARCH_BUDGET_END@
    StorageBuilding* FindStorage(Character* c,GameData* data,LONG e,unsigned cursor)
    {
        lektor<Building*>& list=*reinterpret_cast<lektor<Building*>*>(storageList);list.clear();
        c->getAI()->getAllLocalMachines(list,1u<<BF_RESOURCE_STORAGE);
        if(!Live(e)||!list.size()||list.size()>256)return NULL;
        StorageBuilding* best=NULL;float distance=1e30f;const Ogre::Vector3 pos=c->getPosition();
        const unsigned count=list.size()<16?list.size():16;
        for(unsigned i=0;i<count;++i)
        {
            if(!Live(e))return NULL;
            Building* candidate=list[(cursor+i)%list.size()];StorageBuilding* b=candidate?candidate->getFunctionStuff():NULL;
            const bool valid=ValidStorage(c,b,data,e);if(!Live(e))return NULL;if(!valid)continue;
            const float d=(b->getPosition()-pos).squaredLength();
            if(d==d&&d>=0&&d<distance){best=b;distance=d;}
            if(!Live(e))return NULL;
        }
        return best;
    }
    bool FungibleInventory(Inventory* inventory,Item* candidate,GameData* data);
    bool CargoReady(Character* c,GameData* data,LONG generation)
    {
        if(!Live(generation)||!c||!data)return false;
        Inventory* body=c->getInventory();Inventory* pack=Backpack(c);Item* candidate=NULL;
        Inventory* inventories[2]={body,pack};
        for(unsigned j=0;j<2;++j)
        {
            if(!Live(generation))return false;
            Inventory* inv=inventories[j];if(!inv||(j&&inv==body))continue;
            const lektor<Item*>& all=inv->getAllItems();if(!Live(generation)||all.size()>128)return false;
            for(unsigned i=0;i<all.size();++i)if(all[i]&&all[i]->getGameData()==data&&!all[i]->isEquipped&&all[i]->quantity>0)
            {if(!candidate)candidate=all[i];break;}
        }
        return candidate&&Live(generation)&&FungibleInventory(body,candidate,data)&&Live(generation)&&
            (pack==body||FungibleInventory(pack,candidate,data))&&Live(generation);
    }
    bool Prepare(AITaskSytem* tasks,Tasker* builder,int slot,Episode& out)
    {
        Character* c=tasks->character;const LONG e=Epoch();if(!Eligible(c,e))return false;
        const Identity actor=Of(c->handle);
        if(ReadActor(actor,out))
        {
            if(out.generation==e&&out.jobSlot==slot&&out.jobTask==reinterpret_cast<ULONG_PTR>(builder)&&Of(builder->subject)==out.jobSubject)return true;
            Retire(actor);
        }
        unsigned materialTurn=0,storageCursor=0;
        Owned rows[OwnedCapacity],mine[8];unsigned mineCount=0;
        const unsigned n=EngineerMaterialsRuntime::ListOwned(rows,OwnedCapacity);
        for(unsigned i=0;i<n&&i<OwnedCapacity;++i)if(rows[i].actor==actor&&rows[i].epoch==static_cast<unsigned>(e))
        {if(mineCount==8)return false;mine[mineCount++]=rows[i];}
        if(!mineCount||!SearchTicket(actor,e,GetTickCount(),materialTurn,storageCursor))return false;
        const Owned& credit=mine[materialTurn%mineCount];
        GameData* data=DataIn(c->getInventory(),credit.material);if(!data)data=DataIn(Backpack(c),credit.material);
        int units=0;if(!data||!Returnable(c,credit,data,e,units)||units<=0||!CargoReady(c,data,e))return false;
        StorageBuilding* b=FindStorage(c,data,e,storageCursor);
        if(!b||!Eligible(c,e)||BuilderSlot(tasks,builder)!=slot)return false;
        Episode made;made.used=true;made.generation=e;made.jobSlot=slot;made.jobTask=reinterpret_cast<ULONG_PTR>(builder);made.actor=actor;made.jobSubject=Of(builder->subject);made.storage=Of(b->handle);made.credit=credit;
        Guard g;
        for(unsigned j=0;j<Capacity;++j)if(!episodes[j].used)
        {if(++serial==0)++serial;made.token=serial;episodes[j]=made;out=made;return true;}
        return false;
    }
    float (*originalScore)(Tasker*,AI*)=NULL;
    float (*originalGoap)(AITaskSytem*,Tasker*,bool,bool)=NULL;
    int (*originalStorage)(Tasker*,StorageBuilding*,Inventory*)=NULL;
    void* (*originalFillDelete)(Tasker*,unsigned)=NULL;
    typedef Tasker* (*Factory)(TaskRepertoire*,TaskType,const hand&,taskPriority,float,Ogre::Vector3,int,int);
    Factory factory=NULL;
    void (*originalChoosePerma)(AITaskSytem*,void*,TaskMatch&,bool,bool)=NULL;
    void ChoosePermaHook(AITaskSytem* self,void* ordered,TaskMatch& previous,bool urgent,bool jobs)
    {
        AITaskSytem* saved=normalJobScope;normalJobScope=!urgent&&jobs?self:NULL;
        __try{originalChoosePerma(self,ordered,previous,urgent,jobs);}
        __finally{normalJobScope=saved;}
    }
    float ScoreHook(Tasker* self,AI* ai)
    {
        const LONG callEpoch=Epoch();const float native=originalScore(self,ai);
        if(!Live(callEpoch)||depth||!normalJobScope||!ai||normalJobScope!=ai->taskSystemAI)return native;
        const LONG e=callEpoch;AITaskSytem* tasks=ai->taskSystemAI;const int slot=BuilderSlot(tasks,self);
        if(slot<0||!Eligible(tasks->character,e))return native;
        DepthScope scope;Episode state;bool owned=false;
        try{if(native<=0)owned=Prepare(tasks,self,slot,state);}catch(...){throw;}
        AIFixedEngineerSurplus::Admission a;a.enabled=ready!=0;a.normalJobCall=true;a.exactBuilder=true;a.owned=owned;a.eligible=Eligible(tasks->character,e);a.epochStable=Live(e);
        return AIFixedEngineerSurplus::Admit(native,a);
    }
    void Planning(unsigned token,bool on)
    {Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token)episodes[i].transaction=on;}
    bool (*originalSetTask)(AITaskSytem*,AITaskSytem::ActionsListMgr*)=NULL;
    bool SetTaskHook(AITaskSytem* self,AITaskSytem::ActionsListMgr* from)
    {
        if(!planningToken)return originalSetTask(self,from);
        Episode e;
        if(!Live(planningEpoch)||!ReadToken(planningToken,e)||e.retired||e.generation!=planningEpoch||
           from!=&self->actionsTryList||!Eligible(self->character,planningEpoch)||!JobStillThere(self,e))return false;
        const size_t n=from->list.size();if(!n||n>64)return false;
        Tasker* fills[64];unsigned count=0;const ULONG_PTR base=reinterpret_cast<ULONG_PTR>(image);
        for(size_t i=0;i<n;++i)
        {
            Tasker* t=from->list[i];if(!t||!t->taskData)return false;
            const ULONG_PTR table=*reinterpret_cast<ULONG_PTR*>(t);
            // A cleanup plan never starts custom/mod action classes. All ordinary
            // jobs retain their untouched native planning path.
            if(table<base||table>=base+0x232c000)return false;
            if(table==base+0x16be0f0)
            {if((t->key()!=FILL_MACHINE&&t->key()!=OPERATE_STORAGE)||Of(t->subject)!=e.storage)return false;fills[count++]=t;}
            else if(t->key()==FILL_MACHINE||t->key()==OPERATE_STORAGE)return false;
        }
        if(!count)return false;
        {
            Guard g;bool found=false;
            for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==e.token&&!episodes[i].retired)
            {episodes[i].fill=reinterpret_cast<ULONG_PTR>(fills[0]);episodes[i].committed=false;found=true;break;}
            if(!found)return false;
        }
        // No callbacks/allocations between tagging and native ownership handoff.
        // The tag covers every fill, even if later validation/goal commit fails.
        for(unsigned i=0;i<count;++i)*reinterpret_cast<ULONG_PTR*>(fills[i])=OwnedFillVtable();
        return originalSetTask(self,from);
    }
    void DeleteSeed(Tasker* seed)
    {if(seed){typedef void*(*Delete)(Tasker*,unsigned);(*reinterpret_cast<Delete*>(*reinterpret_cast<char**>(seed)))(seed,1);}}
    float GoapHook(AITaskSytem* self,Tasker* target,bool asks,bool playerOrder)
    {
        const ULONG_PTR from=reinterpret_cast<ULONG_PTR>(_ReturnAddress());
        const LONG callEpoch=Epoch();const float native=originalGoap(self,target,asks,playerOrder);
        if(native!=0||!Live(callEpoch)||depth||playerOrder||from!=reinterpret_cast<ULONG_PTR>(image)+0x50fd28||!self)return native;
        const LONG e=callEpoch;Character* c=self->character;const int slot=BuilderSlot(self,target);
        if(slot<0||!Eligible(c,e))return native;
        DepthScope scope;Episode state;bool prepared=false;
        try{prepared=Prepare(self,target,slot,state);}catch(...){throw;}
        if(!prepared){return native;}
        Building* resolved=EngineerMaterialsRuntime::Handle(state.storage).getBuilding();
        UseableStuff* usable=resolved?resolved->getUseableStuff():NULL;
        StorageBuilding* storage=usable?usable->getFunctionStuff():NULL;
        GameData* data=DataIn(c->getInventory(),state.credit.material);if(!data)data=DataIn(Backpack(c),state.credit.material);
        int units=0;
        if(!data||!CargoReady(c,data,e)||!ValidStorage(c,storage,data,e)||!Returnable(c,state.credit,data,e,units)||units<=0||!self->actionsTryList.isEmpty())
        {Retire(state.actor);return native;}
        Tasker* seed=NULL;float result=0;Planning(state.token,true);
        try
        {
            const hand destination=EngineerMaterialsRuntime::Handle(state.storage);
            const Ogre::Vector3 position=storage->getPosition();
            if(Live(e))
            {
                TaskRepertoire* repertoire=self->aiImplementation?self->aiImplementation->taskRepertoire:NULL;
                if(repertoire)seed=factory(repertoire,FILL_MACHINE,destination,target->priority,target->weight,position,target->startTime,target->endTime);
            }
            if(seed&&Live(e)&&seed->key()==FILL_MACHINE&&Of(seed->subject)==state.storage&&
                *reinterpret_cast<ULONG_PTR*>(seed)==reinterpret_cast<ULONG_PTR>(image)+0x16be0f0)
            {
                const unsigned saved=planningToken;const LONG savedEpoch=planningEpoch;
                planningToken=state.token;planningEpoch=e;
                try{result=originalGoap(self,seed,asks,false);}catch(...){planningToken=saved;planningEpoch=savedEpoch;throw;}
                planningToken=saved;planningEpoch=savedEpoch;
            }
            Tasker* deleting=seed;seed=NULL;DeleteSeed(deleting);
            Planning(state.token,false);
            if(result>0)return result;
        }
        catch(...){DeleteSeed(seed);Planning(state.token,false);Retire(state.actor);throw;}
        // A generated native plan may already own its first action. Never
        // cancel body/foreign actions here; failed preparation is disabled.
        Planning(state.token,false);Retire(state.actor);return result>0?result:native;
    }
    void Selected(AITaskSytem* tasks,Tasker* selected)
    {
        if(!tasks||!tasks->character)return;const Identity actor=Of(tasks->character->handle);Episode e;
        if(!ReadActor(actor,e))return;
        const bool own=e.fill&&e.generation==Epoch()&&selected&&selected->key()==JOB_BUILDER&&
            BuilderSlot(tasks,selected)==e.jobSlot&&Of(selected->subject)==e.jobSubject&&JobStillThere(tasks,e);
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==e.token)
        {episodes[i].committed=own;episodes[i].retired=!own;if(!own&&!episodes[i].fill&&!episodes[i].pending&&!episodes[i].transaction)episodes[i]=Episode();}
    }
    void* FillDeleteHook(Tasker* self,unsigned flags)
    {
        {Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].fill==reinterpret_cast<ULONG_PTR>(self))
        {episodes[i].fill=0;episodes[i].retired=true;episodes[i].deleted=true;if(!episodes[i].pending&&!episodes[i].transaction)episodes[i]=Episode();}}
        return originalFillDelete(self,flags);
    }
    bool BeginTransaction(unsigned token)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token&&!episodes[i].retired&&episodes[i].committed&&!episodes[i].transaction&&!episodes[i].pending)
        {episodes[i].transaction=true;return true;}return false;
    }
    void EndTransaction(unsigned token)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token)
        {episodes[i].transaction=false;
            if(episodes[i].generation!=Epoch()){episodes[i].pending=false;episodes[i].piece=Identity();episodes[i].retired=true;}
            if(episodes[i].deleted&&!episodes[i].pending)episodes[i]=Episode();}
    }
    void Pending(unsigned token,const Identity& piece,const Identity& source)
    {
        Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token)
        {episodes[i].pending=true;episodes[i].piece=piece;episodes[i].sourceInventory=source;}
    }
    void ClearPending(unsigned token)
    {Guard g;for(unsigned i=0;i<Capacity;++i)if(episodes[i].used&&episodes[i].token==token){episodes[i].pending=false;episodes[i].piece=Identity();}}
    LONG maintenanceGeneration=-1;DWORD maintenanceWindow=0;unsigned maintenanceCursor=0;bool maintenanceStarted=false;
    // @SURPLUS_MAINTENANCE_BEGIN@
    void Maintenance(bool worldSafe)
    {
        if(!worldSafe||!ready||!epoch)return;
        const LONG generation=Epoch();const DWORD now=GetTickCount();Episode batch[4];unsigned count=0;
        {
            Guard g;
            if(maintenanceGeneration!=generation)
            {maintenanceGeneration=generation;maintenanceCursor=0;maintenanceStarted=false;}
            if(maintenanceStarted&&now-maintenanceWindow<250)return;
            maintenanceStarted=true;maintenanceWindow=now;
            // Fixed round-robin metadata work, four native attempts at most.
            for(unsigned inspected=0;inspected<8&&count<4;++inspected)
            {
                Episode& e=episodes[maintenanceCursor];maintenanceCursor=(maintenanceCursor+1)%Capacity;
                if(!e.used||!e.pending||e.transaction)continue;
                if(e.generation!=generation){e.pending=false;e.piece=Identity();e.retired=true;if(!e.fill)e=Episode();continue;}
                if(e.retryAt&&static_cast<LONG>(now-e.retryAt)<0)continue;
                e.retryAt=now+1500;e.transaction=true;batch[count++]=e;
            }
        }
        for(unsigned i=0;i<count;++i)
        {
            const Episode& e=batch[i];
            try
            {
                // This completes an already-owned inventory transaction. It
                // intentionally has no feature/heartbeat/jobs/combat gate.
                if(generation==Epoch()&&e.actor.Valid()&&e.piece.Valid()&&e.sourceInventory.Valid())
                {
                    Character* c=EngineerMaterialsRuntime::Handle(e.actor).getCharacter();
                    if(generation==Epoch()&&c&&Of(c->handle)==e.actor&&c->isPlayerCharacter()&&generation==Epoch())
                    {
                        Inventory* body=c->getInventory();
                        Inventory* pack=generation==Epoch()?Backpack(c):NULL;
                        Inventory* source=NULL;
                        if(generation==Epoch()&&body&&Of(body->getHandle())==e.sourceInventory)source=body;
                        if(!source&&generation==Epoch()&&pack&&Of(pack->getHandle())==e.sourceInventory)source=pack;
                        if(generation==Epoch()&&source)
                        {
                            Item* item=EngineerMaterialsRuntime::Handle(e.piece).getItem();
                            if(generation==Epoch()&&item&&Of(item->handle)==e.piece&&item->quantity==1)
                            {
                                if(item->isInInventory)
                                {if(source->hasItem(item)&&generation==Epoch())ClearPending(e.token);}
                                else if(generation==Epoch()&&source->addItem(item,1,false,false))ClearPending(e.token);
                            }
                        }
                    }
                }
            }
            catch(...)
            {
                // Preserve all receipts, and release reservations before the
                // original native exception continues through its host.
                for(unsigned j=i;j<count;++j)EndTransaction(batch[j].token);
                throw;
            }
            EndTransaction(e.token);
        }
    }
    // @SURPLUS_MAINTENANCE_END@
    bool FungibleInventory(Inventory* inventory,Item* candidate,GameData* data)
    {
        if(!candidate||!(candidate->originalFullChargeAmount>=0&&candidate->originalFullChargeAmount<=FLT_MAX))return false;
        if(!inventory)return true;
        const lektor<Item*>& all=inventory->getAllItems();if(all.size()>128)return false;
        unsigned matches=0;
        for(unsigned i=0;i<all.size();++i)
        {
            Item* p=all[i];if(!p||p->getGameData()!=data)continue;
            if(++matches>32||p->isEquipped||p->isUnique||p->quantity<=0||p->chargesLeft!=p->originalFullChargeAmount||p->originalFullChargeAmount!=candidate->originalFullChargeAmount||
               p->quality!=candidate->quality||p->manufacturerData!=candidate->manufacturerData||p->materialData!=candidate->materialData||p->coloriseData!=candidate->coloriseData||
               (p!=candidate&&(!p->canStackWith(candidate)||!candidate->canStackWith(p))))return false;
        }
        return true;
    }
    // @SURPLUS_NATIVE_TRANSFER_BEGIN@
    struct NativeTransfer
    {
        typedef Item* Piece;
        Episode episode;Character* actor;Inventory* source;InventorySection* destination;Item* item;GameData* data;Identity sourceId;bool reserved;
        NativeTransfer():actor(NULL),source(NULL),destination(NULL),item(NULL),data(NULL),reserved(false){}
        bool CanStart(){reserved=Live(episode.generation)&&item&&item->quantity>0&&!item->isEquipped&&item->chargesLeft==item->originalFullChargeAmount&&source->hasItem(item)&&destination&&BeginTransaction(episode.token);return reserved;}
        Piece Detach(){return source->removeItemDontDestroy_returnsItem(item,1,true);}
        void Keep(Piece piece)
        {
            // Load/reset callbacks can retire the world during native splitting.
            // Never inspect a returned old-world pointer in that case.
            Pending(episode.token,episode.generation==Epoch()?Of(piece->handle):Identity(),sourceId);
        }
        bool SameEpoch(){return episode.generation==Epoch();}
        bool PutDestination(Piece piece){return Live(episode.generation)&&destination->addItem(piece,1);}
        bool PutBack(Piece piece){return source->addItem(piece,1,false,false);}
        void Commit()
        {
            ClearPending(episode.token);
            if(episode.generation==Epoch())EngineerMaterialsRuntime::Returned(episode.credit,1,Total(actor,data));
        }
        void Restored(){ClearPending(episode.token);}
    };
    int MoveOne(NativeTransfer& op)
    {
        AIFixedEngineerSurplus::TransferResult result;
        try{result=AIFixedEngineerSurplus::TransferOne(op);}catch(...){if(op.reserved)EndTransaction(op.episode.token);throw;}
        if(op.reserved)EndTransaction(op.episode.token);
        if(result==AIFixedEngineerSurplus::KeptPending)ErrorLog("AIFixed: engineer surplus transfer retained a detached native item receipt; no drop or destroy");
        return result==AIFixedEngineerSurplus::Delivered?1:0;
    }
    // @SURPLUS_NATIVE_TRANSFER_END@
    int StorageHook(Tasker* self,StorageBuilding* building,Inventory* inventory)
    {
        // The heap task tag is installed before native startAction. It survives
        // missing metadata, partial plans, disable and epoch invalidation.
        if(*reinterpret_cast<ULONG_PTR*>(self)!=OwnedFillVtable())return originalStorage(self,building,inventory);
        Episode e;if(!ReadFill(self,e))return 0;
        // Tombstones are deliberate: invalidated owned tasks must NEVER reach
        // native whole-stack dumping, including after feature OFF/load/input.
        if(e.retired||!e.committed||depth||!Live(e.generation))return 0;
        Character* c=EngineerMaterialsRuntime::Handle(e.actor).getCharacter();
        if(!Eligible(c,e.generation)||!c->getAI()||!c->getAI()->taskSystemAI)return 0;
        AITaskSytem* tasks=c->getAI()->taskSystemAI;
        if(!JobStillThere(tasks,e)||!tasks->body||tasks->body->getCurrentAction()!=self||self->key()!=FILL_MACHINE||
           Of(self->subject)!=e.storage||Of(building->handle)!=e.storage||tasks->currentGoal.key()!=JOB_BUILDER||
           Of(tasks->currentGoal.subject)!=e.jobSubject)return 0;
        Inventory* body=c->getInventory();Inventory* pack=Backpack(c);
        if(!inventory||(inventory!=body&&inventory!=pack))return 0;
        if(e.pending)return 0; // Recovery has its own bounded, feature-independent maintenance.
        GameData* data=DataIn(inventory,e.credit.material);int units=0;
        if(!data||!ValidStorage(c,building,data,e.generation)||!Returnable(c,e.credit,data,e.generation,units)||units<=0)return 0;
        const int n=building->getNumConsumtionItems();if(n<0||n>32)return 0;
        InventorySection* destination=NULL;
        for(int i=0;i<n;++i){StorageBuilding::ConsumptionItem* slot=building->getConsumtionItems(i);if(slot&&slot->item==data){destination=slot->inventorySection;break;}}
        if(!destination||!destination->hasRoomForItem(data,1))return 0;
        Item* item=NULL;const lektor<Item*>& all=inventory->getAllItems();if(all.size()>ItemLimit)return 0;
        for(unsigned i=0;i<all.size();++i)if(all[i]&&!all[i]->isEquipped&&all[i]->quantity>0&&all[i]->getGameData()==data){item=all[i];break;}
        if(!item||!FungibleInventory(body,item,data)||(pack!=body&&!FungibleInventory(pack,item,data))||!Live(e.generation))return 0;
        NativeTransfer op;op.episode=e;op.actor=c;op.source=inventory;op.destination=destination;op.item=item;op.data=data;op.sourceId=Of(inventory->getHandle());
        DepthScope scope;return MoveOne(op);
    }
    bool Bytes(unsigned rva,const char* bytes,unsigned n){return !std::memcmp(image+rva,bytes,n);}
    bool SharedEntry(unsigned rva,const char* bytes,unsigned n)
    {return Bytes(rva,bytes,n)||(image[rva]==0xe9&&n>5&&!std::memcmp(image+rva+5,bytes+5,n-5));}
    bool MatchesCode(const unsigned char* code,unsigned count,unsigned long long expected)
    {
        unsigned long long hash=14695981039346656037ULL;
        for(unsigned i=0;i<count;++i)hash=(hash^code[i])*1099511628211ULL;
        return hash==expected;
    }
    bool Contract(unsigned rva,unsigned n,unsigned long long expected)
    {return MatchesCode(image+rva,n,expected);}
    bool PlannerEntry()
    {
        if(Contract(0x50f380,0x6af,0x37b09f9ef0175dbbULL))return true;
        const unsigned char* entry=image+0x50f380;
        if(entry[0]!=0xe9||!Contract(0x50f385,0x6aa,0xf27e769eb7fb5a5dULL))return false;
        // LSE's audited runGOAP wrapper forwards ordinary builder and fill
        // tasks unchanged. Resolve its existing MinHook relay, never bypass it.
        // NativeAdmission is called under SEH, including these startup reads.
        const unsigned char* lse=reinterpret_cast<const unsigned char*>(GetModuleHandleW(L"Loot-Scoot-Execute.dll"));
        if(!lse)return false;
        LONG jump=0;
        std::memcpy(&jump,entry+1,sizeof(jump));
        const unsigned char* relay=reinterpret_cast<const unsigned char*>(
            static_cast<ULONG_PTR>(reinterpret_cast<LONG_PTR>(entry)+5+jump));
        const unsigned char relayHead[]={0xff,0x25,0,0,0,0};
        if(std::memcmp(relay,relayHead,sizeof(relayHead))!=0)return false;
        ULONG_PTR target=0;
        std::memcpy(&target,relay+sizeof(relayHead),sizeof(target));
        return target==reinterpret_cast<ULONG_PTR>(lse+0x15c10)&&
            MatchesCode(lse+0x15c10,0xfa,0x953671ec41749d05ULL);
    }
    bool NativeAdmission()
    {
        // Startup admission authenticates code while the host live-world gate
        // is deliberately still closed until all modules install.
        if(!admit||!epoch||!allowed)return false;
        image=reinterpret_cast<unsigned char*>(GetModuleHandleA(NULL));if(!image)return false;
        const IMAGE_DOS_HEADER* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
        const IMAGE_NT_HEADERS64* pe=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
           pe->FileHeader.TimeDateStamp!=0x65d604d7||pe->OptionalHeader.SizeOfImage!=0x232c000)return false;
        if(!SharedEntry(0x60f2f0,"\x48\x89\x5c\x24\x08\x48\x89\x74\x24\x10\x57\x48\x83\xec\x60\x48\x8b\x41\x70\x48\x8b\xd9\x4c\x8d\x41\x58\x0f\x29\x74\x24\x50\x48",32))return false;
        if(!Bytes(0x340eb0,"\x48\x8b\xc4\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xec\x50\x48\xc7\x44\x24\x30\xfe\xff\xff\xff\x48\x89\x58\x08\x48\x89\x68",32))return false;
        if(!Bytes(0x33ee80,"\x48\x89\x5c\x24\x08\x57\x48\x83\xec\x20\x48\x8d\x05\xd7\xdd\x37\x01\x48\x8b\xf9\x48\x83\xc1\x08\x48\x89\x41\xf8\x8b\xda\xff\x15",32))return false;
        if(!SharedEntry(0x32eb90,"\x40\x55\x56\x57\x41\x54\x41\x55\x48\x8d\x6c\x24\xe9\x48\x81\xec\x90\x00\x00\x00\x48\xc7\x45\xd7\xfe\xff\xff\xff\x48\x89\x9c\x24\xd0\x00\x00\x00\x48\x8b\x05\xfd\x77\xde\x01\x48\x33\xc4\x48\x89\x45\x07\x41\x8b\xf1\x49\x8b\xf8\x8b\xda\x89\x55\xcf\x4c\x8b\x65\x6f\x48\x8b\x01\xff\x50\x08\x84\xc0\x75\x07\x33\xc0\xe9\x11\x16\x00\x00\x8d\x43\xff\x45\x33\xed\x3d\x21\x01\x00\x00\x0f\x87\x73\x15\x00\x00\x48\x98\x48\x8d\x15\x04\x14\xcd\xff\x0f\xb6\x84\x02\xc8\x03\x33\x00\x8b\x8c\x82\x18\x02\x33\x00\x48\x03\xca\xff\xe1",128))return false;
        if(!Bytes(0x330194,"\x48\x8d\x55\xcf\x48\x8d\x0d\x51\x7f\x9b\x01\xe8\x5f\x5e\xcf\xff\x48\x8b\x48\x08\x48\x89\x4b\x70\x89\x73\x08\x8b\x47\x08\x89\x43\x18\x8b\x47\x0c\x89\x43\x1c\x8b\x4f\x10\x89\x4b\x20\x8b\x4f\x14\x89\x4b\x24\x8b\x4f\x18\x89\x4b\x28\x48\x8d\x4b\x58\x49\x8b\xd4\xff\x15\x06\x60\xf1\x01\xf3\x0f\x10\x6d\x67\xf3\x0f\x11\x6b\x30\x8b\x4d\x77\x89\x4b\x64\x8b\x4d\x7f\x89\x4b\x68\x48\x8b\xc3",95))return false;
        if(!Bytes(0x50fd16,"\x45\x0f\xb6\xcc\x41\xb0\x01\x48\x8b\xd6\x48\x8b\xcd\xe8\xe5\xb4\xaf\xff\x0f\x2f\xc6\x0f\x87\x96\x00\x00\x00\x48\xc7\x46\x40\x0b\x00\x00\x00\x4c\x89\x6e\x4c\x44\x89\x6e\x48\x44\x38\x6b\x29\x74\x06\x48\x8b\x5b\x10\xeb\x4d\x48\x8b\x03\x44\x38\x68\x29\x75\x1c\x48\x8b\xd8\x48\x8b\x40\x10\x44\x38\x68",74))return false;
        if(!Bytes(0x50e23b,"\x48\x8b\x96\xb8\x01\x00\x00\x48\x8b\xcf\xf3\x0f\x5c\xf7\xe8\xec\x39\xaf\xff\x44\x0f\x2f\xc0\x0f\x83\xda\xfa\xff\xff\x49\x8b\x5e\x08\x48\x8b",35))return false;
        if(!Bytes(0x50c850,"\x40\x53\x41\x55\x48\x83\xec\x58\x48\x89\x6c\x24\x70\x48\x89\x74\x24\x78\x48\x8b\xda\x48\x89\xbc\x24\x80\x00\x00\x00\x48\x8b\xf1",32))return false;
        if(!Bytes(0x749e10,"\x48\x89\x5c\x24\x18\x55\x56\x41\x54\x48\x83\xec\x20\x80\xba\xd8\x00\x00\x00\x00\x41\x0f\xb6\xf1\x45\x8b\xe0\x48\x8b\xda\x48\x8b",32))return false;
        if(!Bytes(0x74bd70,"\x48\x8b\xc4\x44\x89\x40\x18\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8b\xec\x48\x83\xec\x40\x48\xc7\x45\xe0\xfe\xff\xff\xff\x48",32))return false;
        if(!Contract(0x50db20,0x7b0,0x3486840cfe11bde9ULL)&&!(image[0x50db20]==0xe9&&Contract(0x50db25,0x7ab,0xb1b78db36e94607ULL)))return false;
        if(!Contract(0x50cab0,0x170,0x30cde7edcc348bb3ULL))return false;
        if(!Contract(0x3444b0,0x1f5,0xc460a4b9b01ab369ULL))return false;
        if(!Contract(0x340eb0,0x224,0xc6771c66ba2a0f5dULL))return false;
        if(!Contract(0x749e10,0x1da,0xaf57a2800f22161fULL))return false;
        if(!Contract(0x74bd70,0x4e9,0xa9f01c9c7210604aULL))return false;
        if(!PlannerEntry())return false;
        if(!Contract(0x50fbc0,0x2bc,0xdc6ed95659b1ae50ULL))return false;
        if(!Contract(0x50c850,0x124,0x57bbe974df7b4420ULL))return false;
        if(!Contract(0x32eb90,0x1663,0x325bb951c9771e3fULL)&&!(image[0x32eb90]==0xe9&&Contract(0x32eb95,0x165e,0x9ff28a9340a2516aULL)))return false;
        if(KenshiLib::GetRealAddress(&Tasker::score)!=reinterpret_cast<intptr_t>(image+0x60f2f0)||KenshiLib::GetRealAddress(&AITaskSytem::runGOAP)!=reinterpret_cast<intptr_t>(image+0x50f380))return false;
        if(KenshiLib::GetRealAddress(&AITaskSytem::setCurrentTask)!=reinterpret_cast<intptr_t>(image+0x50cab0)||
           KenshiLib::GetRealAddress(&AITaskSytem::choosePermaJob)!=reinterpret_cast<intptr_t>(image+0x50db20)||
           KenshiLib::GetRealAddress(&Inventory::_NV_removeItemDontDestroy_returnsItem)!=reinterpret_cast<intptr_t>(image+0x749e10)||
           KenshiLib::GetRealAddress(&InventorySection::_NV_addItem)!=reinterpret_cast<intptr_t>(image+0x74bd70))return false;
        const ULONG_PTR base=reinterpret_cast<ULONG_PTR>(image);
        return *reinterpret_cast<ULONG_PTR*>(image+0x16be0f0)==base+0x2856a&&
               *reinterpret_cast<ULONG_PTR*>(image+0x16be100)==base+0x43fe5;
    }
    void Init(bool(*a)(),bool(*d)(),LONG(*e)(),bool(*p)(Character*))
    {admit=a;debug=d;epoch=e;allowed=p;}
    bool Install()
    {
        ready=0;
        bool admitted=false;__try{admitted=NativeAdmission();}__except(EXCEPTION_EXECUTE_HANDLER){admitted=false;}
        if(!admitted){ErrorLog("AIFixed: engineer surplus return refused an unsupported native storage/job contract");return false;}
        factory=reinterpret_cast<Factory>(image+0x32eb90);
        // Include native RTTI at [-1] and all five Tasker virtual entries.
        std::memcpy(ownedFillTable,image+0x16be0e8,sizeof(ownedFillTable));
        if(KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x50cab0,SetTaskHook,&originalSetTask)||
           KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x50db20,ChoosePermaHook,&originalChoosePerma)||
           KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x340eb0,StorageHook,&originalStorage)||
           KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x33ee80,FillDeleteHook,&originalFillDelete)||
           KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x60f2f0,ScoreHook,&originalScore)||
           KenshiLib::SUCCESS!=KenshiLib::AddHook(image+0x50f380,GoapHook,&originalGoap))
        {ErrorLog("AIFixed: engineer surplus hooks incomplete; automatic return remains disabled");return false;}
        InterlockedExchange(&ready,1);return true;
    }
}
