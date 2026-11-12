// Included inside AIFixed's private namespace. Basusu GateFix adaptation,
// GPLv3; source pin, attribution and exclusions are in UPSTREAM.md.
// GUI owns every episode; only serial-bearing hands cross callbacks.
struct GateState
{
    hand actor,gate,leaf;Ogre::Vector3 destination,sample;
    DWORD stageStartedAt,inputAt,sampledAt,checkedAt,passedAt;
    unsigned serial,doorSerial;int stage,startSide,targetRegion;bool wasLocked,taskMissing;
};
typedef std::map<hand,GateState,HandIdentityLess> GateMap;
GateMap g_gateStates;hand g_gateCursor;DWORD g_lastGateScan=0;unsigned g_gateQueryBudget=0;
typedef std::map<hand,unsigned,HandIdentityLess> GateDoorChanges;
GateDoorChanges g_gateDoorChanges;
bool GateLockSnapshot(DoorStuff* door,hand* identity,bool* locked,bool* wants)
{
    __try
    {
        if(t_ownOrder||!g_gateHooksReady||!door)return false;
        *identity=door->handle;*locked=door->isLocked();*wants=door->wantsToLock;return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void GateLockChanged(DoorStuff* door,const hand& identity,bool beforeLocked,bool beforeWants,bool directButton)
{
    hand after;bool locked=false,wants=false;
    if(!GateLockSnapshot(door,&after,&locked,&wants)||!SameIdentity(identity,after))return;
    if(!directButton&&locked==beforeLocked&&wants==beforeWants)return;
    IntentGuard guard;GateDoorChanges::iterator it=g_gateDoorChanges.find(identity);
    if(it!=g_gateDoorChanges.end())++it->second;
}
void (*g_origGateLock)(DoorStuff*)=NULL;
void (*g_origGateUnlock)(DoorStuff*)=NULL;
void (*g_origGateLockButton)(DoorStuff*,DataPanelLine*)=NULL;
void (*g_origGateOpenButton)(DoorStuff*,DataPanelLine*)=NULL;
void GateLock_hook(DoorStuff* self)
{
    hand id;bool locked=false,wants=false;const bool capture=GateLockSnapshot(self,&id,&locked,&wants);
    const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
    g_origGateLock(self);
    if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))GateLockChanged(self,id,locked,wants,false);
}
void GateUnlock_hook(DoorStuff* self)
{
    hand id;bool locked=false,wants=false;const bool capture=GateLockSnapshot(self,&id,&locked,&wants);
    const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
    g_origGateUnlock(self);
    if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))GateLockChanged(self,id,locked,wants,false);
}
void GateLockButton_hook(DoorStuff* self,DataPanelLine* line)
{
    hand id;bool locked=false,wants=false;const bool capture=GateLockSnapshot(self,&id,&locked,&wants);
    const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
    g_origGateLockButton(self,line);
    if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))GateLockChanged(self,id,locked,wants,true);
}
void GateOpenButton_hook(DoorStuff* self,DataPanelLine* line)
{
    hand id;bool locked=false,wants=false;const bool capture=GateLockSnapshot(self,&id,&locked,&wants);
    const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
    g_origGateOpenButton(self,line);
    if(capture&&generation==InterlockedCompareExchange(&g_loadGeneration,0,0))GateLockChanged(self,id,locked,wants,true);
}
__declspec(thread) char t_gateObjects[sizeof(lektor<RootObject*>)];
lektor<RootObject*>& GateObjects()
{
    lektor<RootObject*>& list=*reinterpret_cast<lektor<RootObject*>*>(t_gateObjects);
    list.clear();return list;
}
AIFixedGate::Point GatePoint(const Ogre::Vector3& p){return AIFixedGate::Point(p.x,p.y,p.z);}
bool AtMove(Tasker* task,const Ogre::Vector3& destination)
{return task&&task->key()==MOVE_CUS_ORDERED&&(task->getLocation()-destination).squaredLength()<1.0f;}
void QueueGateMove(Character* c,Building* dest,RootObject* subject,const Ogre::Vector3& position)
{
    // Object/follow/use commands retain their complete native semantics.
    if(t_ownOrder||!g_newHooksReady||!g_gateHooksReady||!g_gatePassage||g_gateConflict||dest||subject||
       !c||!c->isPlayerCharacter()||InterlockedCompareExchange(&g_loading,0,0)!=0||
       !AIFixedGate::Finite(GatePoint(position)))return;
    OrdersReceiver* orders=c->getOrdersReciever();
    if(!orders||orders->orders.getSecondTask()||!AtMove(orders->getFirstOrder(),position))return;
    // Hooks queue intent only. The GUI assigns all episode timestamps from
    // its active simulation clock when this intent can actually be processed.
    MoveIntent intent;intent.actor=hand(c);intent.destination=position;intent.sample=c->getPosition();intent.serial=InputSequence(c);
    if(intent.serial==~0u)return;
    IntentGuard guard;
    for(size_t i=0;i<g_moveIntents.size();++i)
        if(SameIdentity(g_moveIntents[i].actor,intent.actor)){g_moveIntents[i]=intent;return;}
    if(g_moveIntents.size()<128)g_moveIntents.push_back(intent);
}
int NativeFaultFilter(unsigned code)
{return code==0xe06d7363?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;}
void SafeQueueGateMove(Character* c,Building* dest,RootObject* subject,const Ogre::Vector3& position)
{
    __try{QueueGateMove(c,dest,subject,position);}
    __except(NativeFaultFilter(GetExceptionCode()))
    {ErrorLog("AIFixed: invalid movement boundary; gate intent dropped");}
}
void (*g_origMove)(Character*,Building*,RootObject*,const Ogre::Vector3&)=NULL;
void Move_hook(Character* c,Building* dest,RootObject* subject,const Ogre::Vector3& position)
{
    const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
    g_origMove(c,dest,subject,position);
    if(generation==InterlockedCompareExchange(&g_loadGeneration,0,0))SafeQueueGateMove(c,dest,subject,position);
}
Character* LiveGateActor(const hand& actor)
{
    const lektor<Character*>& players=ou->player->playerCharacters;
    for(uint32_t i=0;i<players.size();++i)
        if(players[i]&&SameIdentity(hand(players[i]),actor))return players[i];
    return NULL;
}
DoorStuff* LiveGate(const GateState& state)
{
    Building* b=state.gate.getBuilding();
    if(!b||!SameIdentity(hand(b),state.gate)||b->isDisabled()||b->getSpecialFunction()!=BF_GATE||!b->isGate())return NULL;
    DoorStuff* door=b->getDoor();
    if(!door||!SameIdentity(hand(door),state.leaf)||!door->isSetupComplete||door->isBroken()||
       door->getFaction()!=b->getFaction())return NULL;
    return door;
}
bool GateDoorUnchanged(const GateState& state)
{
    IntentGuard guard;
    GateDoorChanges::const_iterator changed=g_gateDoorChanges.find(state.leaf);
    return changed!=g_gateDoorChanges.end()&&changed->second==state.doorSerial;
}
bool OwnOpenOrder(OrdersReceiver* orders,const GateState& state)
{
    Tasker* first=orders->getFirstOrder();
    if(!first||first->key()!=OPEN_DOOR||!SameIdentity(first->subject,state.leaf))return false;
    Tasker* second=orders->orders.getSecondTask();
    return !second||AtMove(second,state.destination);
}
void ResumeGateMove(Character* c,const GateState& state)
{OwnRestOrder(c->getOrdersReciever(),MOVE_CUS_ORDERED,hand(),state.destination);}
void OpenGateFor(Character* c,DoorStuff* door,bool unlock)
{
    const bool old=t_ownOrder;t_ownOrder=true;
    __try
    {
        if(unlock)door->unlockDoor();
        c->addOrder(door,OPEN_DOOR,door,false,false,door->getDoorPosition());
    }
    __finally{t_ownOrder=old;}
}
bool OwnCloseGate(DoorStuff* door)
{
    const bool old=t_ownOrder;t_ownOrder=true;bool closed=false;
    __try{closed=door->closeDoor();}__finally{t_ownOrder=old;}
    return closed;
}
void OwnLockGate(DoorStuff* door)
{
    const bool old=t_ownOrder;t_ownOrder=true;
    __try{door->lockDoor();}__finally{t_ownOrder=old;}
}
void RetireGateLock(const GateState& state)
{
    if(!state.wasLocked||!g_gateRelock||!GateDoorUnchanged(state))return;
    DoorStuff* door=LiveGate(state);
    if(!door||door->getFaction()!=ou->player->getFaction()||door->isLocked())return;
    // Keep the original lock responsibility with any other active passage.
    for(GateMap::iterator it=g_gateStates.begin();it!=g_gateStates.end();++it)
    {
        GateState& other=it->second;
        if(&other==&state||!other.stage||other.doorSerial!=state.doorSerial||!SameIdentity(other.leaf,state.leaf))continue;
        other.wasLocked=true;
        return;
    }
    if(door->getDoorOpenAmount()<=.01f)OwnLockGate(door);
}
void SafeRetireGateLock(const GateState& state)
{
    __try{RetireGateLock(state);}
    __except(NativeFaultFilter(GetExceptionCode()))
    {ErrorLog("AIFixed: gate object invalidated during lock restoration");}
}
bool GateOccupied(DoorStuff* door)
{
    if(!g_gateQueryBudget)return true;
    --g_gateQueryBudget;
    const Ogre::Vector3 inside=door->getDoorPosInside(),outside=door->getDoorPosOutside();
    const Ogre::Vector3 center=(inside+outside)*.5f;
    const float x=outside.x-inside.x,z=outside.z-inside.z;
    const float width=std::sqrt(x*x+z*z);
    if(!_finite(width)||width<=1.0f)return true;
    lektor<RootObject*>& occupants=GateObjects();
    ou->getObjectsWithinSphere(occupants,center,AIFixedGate::WorldUnits(8.0f),CHARACTER,128,NULL);
    if(occupants.size()>=128)return true;
    for(unsigned i=0;i<occupants.size();++i)
    {
        Character* actor=static_cast<Character*>(occupants[i]);
        if(!actor)continue;
        const Ogre::Vector3 p=actor->getPosition()-center;
        const float radius=actor->getRadius();
        if(!AIFixedGate::Finite(GatePoint(p))||!_finite(radius)||radius<0)return true;
        if(std::fabs(p.y)>AIFixedGate::WorldUnits(2.0f))continue;
        if(std::fabs(p.x*x+p.z*z)<=width*(width*.5f+radius) &&
           std::fabs(p.z*x-p.x*z)<=width*(AIFixedGate::WorldUnits(5.5f)+radius))return true;
    }
    return false;
}

DoorStuff* EligibleGate(Character* c,Building* b)
{
    if(!b||b->isDisabled()||b->getSpecialFunction()!=BF_GATE||!b->isGate()||b->getFaction()!=c->getFaction())return NULL;
    DoorStuff* door=b->getDoor();
    if(!door||!door->isSetupComplete||door->isBroken()||door->getDoorOpenAmount()>.05f||
       (door->isLocked()&&!g_gateUnlock))return NULL;
    return door;
}
bool GateConnects(Building* gate,DoorStuff* door,int source,int destination)
{
    if(source<0||destination<0||source==destination)return false;
    // These are the two codes that DoorStuff registers in the engine's gate graph.
    const unsigned leaf=door->getGateCode();
    return leaf!=0xffffu&&AIFixedGate::ConnectsRegions(source,destination,leaf,gate->amInsideTownWalls());
}
float GateDistanceSq(const Ogre::Vector3& a,const Ogre::Vector3& b)
{
    const float x=a.x-b.x,z=a.z-b.z;
    return x*x+z*z;
}
DoorStuff* FindGate(Character* c,const Ogre::Vector3& here,const Ogre::Vector3& destination,
                    int source,int target,float radius,Building*& parent)
{
    if(!ou->zoneMgr)return NULL;
    --g_gateQueryBudget;
    lektor<RootObject*>& objects=GateObjects();
    // The loaded building grid indexes Building::getSpecialFunction per cell.
    // Query gates directly so dense bases do not flood the result with walls.
    ou->zoneMgr->buildingGrid.getObjects(here,radius,1u<<BF_GATE,objects,128);
    if(objects.size()>=128)return NULL;
    DoorStuff* best=NULL;float bestMiss=0,bestDistance=0;bool bestRoute=false;
    for(uint32_t i=0;i<objects.size();++i)
    {
        Building* b=static_cast<Building*>(objects[i]);
        DoorStuff* d=EligibleGate(c,b);
        if(!d)continue;
        const bool route=GateConnects(b,d,source,target);
        float miss=0;
        if(!route&&!AIFixedGate::Crosses(GatePoint(here),GatePoint(destination),GatePoint(d->getDoorPosInside()),
            GatePoint(d->getDoorPosOutside()),AIFixedGate::WorldUnits(8.0f),&miss))continue;
        const float distance=GateDistanceSq(here,d->getDoorPosition());
        if(!best||(route&&!bestRoute)||(route==bestRoute&&(miss<bestMiss||(miss==bestMiss&&distance<bestDistance))))
        {best=d;parent=b;bestMiss=miss;bestDistance=distance;bestRoute=route;}
    }
    return best;
}
bool GateOne(GateState& state,DWORD now)
{
    Character* c=LiveGateActor(state.actor);
    if(!c||InputSequence(c)!=state.serial||c->isDead()||c->isDown()||c->isUnconcious())return false;
    OrdersReceiver* orders=c->getOrdersReciever();if(!orders)return false;
    // A distant native move and opening approach can each exceed 30 seconds.
    const bool expired=state.stage<2
        ? AIFixedGate::Expired(now,state.sampledAt)||now-state.stageStartedAt>300000
        : state.stage==2&&state.passedAt
            ? now-state.passedAt>DWORD(g_gateCloseDelay*1000+30000)
            : AIFixedGate::Expired(now,state.stageStartedAt);
    if(!g_gatePassage||!g_gateHooksReady||g_gateConflict||expired)
    {
        if(state.stage==1&&OwnOpenOrder(orders,state))ResumeGateMove(c,state);
        return false;
    }
    if(state.stage==0)
    {
        Tasker* first=orders->getFirstOrder();
        if(orders->orders.getSecondTask()||(first&&!AtMove(first,state.destination)))return false;
        const Ogre::Vector3 here=c->getPosition();
        if(!first)
        {
            // The game can retire a blocked move before the scan. The input
            // serial above still rejects Stop and replacement orders.
            const float dx=here.x-state.destination.x,dz=here.z-state.destination.z;
            if(dx*dx+dz*dz<1.0f)return false;
        }
        const float moved=(here-state.sample).squaredLength();
        if(!first&&moved>.25f)return false;
        state.taskMissing=!first;
        if(moved>.25f){state.sample=here;state.sampledAt=now;return true;}
        if(!AIFixedGate::Ready(now,state.inputAt,state.sampledAt,moved)||now-state.checkedAt<1000)return true;
        if(!g_gateQueryBudget)return true;
        state.checkedAt=now;
        const float radius=AIFixedGate::WorldUnits(float(g_gateRadius));
        const float nearRadius=AIFixedGate::WorldUnits(15.0f);
        Building* bestParent=NULL;
        DoorStuff* best=NULL;
        bool townGate=false;
        int source=-1,target=-1;
        if(ou->navmesh&&!ou->navmesh->isGenerating())
        {
            source=ou->navmesh->getGateCode(here);
            target=ou->navmesh->getGateCode(state.destination);
        }
        TownBase* town=source>=0&&target>=0&&source!=target?c->getCurrentTownLocation():NULL;
        if(town)
        {
            Building* candidate=town->getNearestGate(here);
            DoorStuff* door=EligibleGate(c,candidate);
            if(door&&GateDistanceSq(here,door->getDoorPosition())<=radius*radius&&GateConnects(candidate,door,source,target))
            {best=door;bestParent=candidate;townGate=true;--g_gateQueryBudget;}
        }
        if(!best&&radius>nearRadius&&g_gateQueryBudget>1)
        {
            best=FindGate(c,here,state.destination,source,target,nearRadius,bestParent);
            if(best&&source>=0&&target>=0&&source!=target&&!GateConnects(bestParent,best,source,target))
            {
                Building* fartherParent=NULL;
                DoorStuff* farther=FindGate(c,here,state.destination,source,target,radius,fartherParent);
                if(farther&&GateConnects(fartherParent,farther,source,target)){best=farther;bestParent=fartherParent;}
            }
        }
        if(!best)best=FindGate(c,here,state.destination,source,target,radius,bestParent);
        if(!best)return first!=NULL;
        state.gate=hand(bestParent);state.leaf=hand(best);state.wasLocked=best->isLocked();
        {IntentGuard guard;state.doorSerial=g_gateDoorChanges[state.leaf];}
        state.startSide=AIFixedGate::Side(GatePoint(here),GatePoint(best->getDoorPosInside()),GatePoint(best->getDoorPosOutside()));
        state.targetRegion=GateConnects(bestParent,best,source,target)?target:-1;
        state.sample=here;state.sampledAt=state.stageStartedAt=now;
        state.stage=1;
        OpenGateFor(c,best,state.wasLocked);
        if(g_debug)
            DebugLog(townGate?"AIFixed: native gate passage order issued using town gate":
                "AIFixed: native gate passage order issued using local gate query");
        return true;
    }
    DoorStuff* door=LiveGate(state);
    if(!door||door->getFaction()!=c->getFaction())return false;
    if(!GateDoorUnchanged(state))return false;
    if(state.stage==1)
    {
        if(door->isLocked())return false; // another owner changed the lock
        if(door->getDoorOpenAmount()>=.9f)
        {
            Tasker* first=orders->getFirstOrder();
            // A completed OPEN_DOOR can leave the fallback queue empty.
            const bool completedFallback=state.taskMissing&&!first&&!orders->orders.getSecondTask();
            if(!completedFallback&&!OwnOpenOrder(orders,state)&&!AtMove(first,state.destination))return false;
            ResumeGateMove(c,state);state.stageStartedAt=now;state.stage=2;
        }
        else
        {
            const float progressSample=AIFixedGate::WorldUnits(1.0f);
            const Ogre::Vector3 here=c->getPosition();
            const Ogre::Vector3 gate=door->getDoorPosition();
            if((here-state.sample).squaredLength()>=progressSample*progressSample&&
               (here-gate).squaredLength()<(state.sample-gate).squaredLength()&&OwnOpenOrder(orders,state))
            {
                state.sample=here;state.sampledAt=now;
            }
        }
        return true;
    }
    if(state.stage==2)
    {
        const int side=AIFixedGate::Side(GatePoint(c->getPosition()),GatePoint(door->getDoorPosInside()),GatePoint(door->getDoorPosOutside()));
        const bool crossed=state.targetRegion>=0 ? c->amInsideTownWalls()==state.targetRegion :
            side&&side!=state.startSide;
        if(!crossed){state.passedAt=0;return true;}
        if(!state.passedAt)state.passedAt=now;
        if(!g_gateClose)return false;
        if(now-state.passedAt<DWORD(g_gateCloseDelay*1000)||GateOccupied(door))return true;
        // Do not close while another tracked actor is still approaching this gate.
        for(GateMap::const_iterator it=g_gateStates.begin();it!=g_gateStates.end();++it)
            if(!SameIdentity(it->first,state.actor)&&SameIdentity(it->second.gate,state.gate)&&
               (it->second.stage<2||(it->second.stage==2&&!it->second.passedAt)))return true;
        if(!OwnCloseGate(door))return false;
        if(g_debug)DebugLog("AIFixed: native gate close requested");
        state.stageStartedAt=now;state.stage=3;return true;
    }
    if(door->getDoorOpenAmount()>.01f)return true;
    if(state.wasLocked&&g_gateRelock)OwnLockGate(door);
    if(g_debug)DebugLog("AIFixed: native gate closed after passage");
    return false;
}
bool GateOneCpp(GateState& state,DWORD now)
{
    try{return GateOne(state,now);}
    catch(const std::exception&){ErrorLog("AIFixed: gate operation failed; episode retired");return false;}
}
bool SafeGateOne(GateState& state,DWORD now)
{
    __try{return GateOneCpp(state,now);}
    __except(NativeFaultFilter(GetExceptionCode()))
    {ErrorLog("AIFixed: gate object invalidated; episode retired");return false;}
}
void GateScan(DWORD now)
{
    g_gateQueryBudget=2; // at most eight geographically local queries per second
    std::vector<MoveIntent> pending;
    {IntentGuard guard;pending.swap(g_moveIntents);}
    for(size_t i=0;i<pending.size();++i)
    {
        const MoveIntent& move=pending[i];
        GateMap::iterator prior=g_gateStates.find(move.actor);
        if(prior==g_gateStates.end()&&g_gateStates.size()>=128)continue;
        if(prior!=g_gateStates.end())SafeRetireGateLock(prior->second);
        GateState state={};state.actor=move.actor;state.destination=move.destination;
        state.stageStartedAt=state.inputAt=now;state.serial=move.serial;
        state.sample=move.sample;state.sampledAt=now;g_gateStates[move.actor]=state;
    }
    // Eight actors per 250ms, round robin; no per-frame or global world scan.
    size_t count=(std::min)(size_t(8),g_gateStates.size());
    for(size_t i=0;i<count;++i)
    {
        GateMap::iterator it=g_gateStates.upper_bound(g_gateCursor);
        if(it==g_gateStates.end())it=g_gateStates.begin();
        if(it==g_gateStates.end())break;
        g_gateCursor=it->first;
        if(!SafeGateOne(it->second,now))
        {
            SafeRetireGateLock(it->second);
            g_gateStates.erase(it);
        }
    }
    // Watched door keys exist only while an episode references them.
    IntentGuard guard;
    for(GateDoorChanges::iterator it=g_gateDoorChanges.begin();it!=g_gateDoorChanges.end();)
    {
        bool used=false;
        for(GateMap::const_iterator state=g_gateStates.begin();state!=g_gateStates.end();++state)
            if(SameIdentity(state->second.leaf,it->first)){used=true;break;}
        if(!used)g_gateDoorChanges.erase(it++);else ++it;
    }
}
