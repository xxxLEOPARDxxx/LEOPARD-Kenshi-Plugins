// Included inside AIFixed's private namespace. Basusu GateFix adaptation,
// GPLv3; source pin, attribution and exclusions are in UPSTREAM.md.
// GUI owns every episode; only serial-bearing hands cross callbacks.
struct GateState
{
    hand actor,gate,leaf;Ogre::Vector3 destination,sample;
    DWORD created,inputAt,sampledAt,checkedAt,passedAt;
    unsigned serial,doorSerial;int stage,startSide;bool wasLocked;
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
    if(!b||!SameIdentity(hand(b),state.gate)||b->getSpecialFunction()!=BF_GATE||!b->isGate())return NULL;
    DoorStuff* door=b->getDoor();
    if(!door||!SameIdentity(hand(door),state.leaf)||!door->isSetupComplete||door->isBroken())return NULL;
    return door;
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
bool GateOccupied(DoorStuff* door)
{
    if(!g_gateQueryBudget)return true;
    --g_gateQueryBudget;
    lektor<RootObject*>& occupants=GateObjects();
    ou->getObjectsWithinSphere(occupants,door->getDoorPosition(),8.0f,CHARACTER,128,NULL);
    // Native ignores maxNumber. Any returned actor means occupied.
    return occupants.size()!=0;
}
bool GateOne(GateState& state,DWORD now)
{
    Character* c=LiveGateActor(state.actor);
    if(!c||InputSequence(c)!=state.serial||c->isDead()||c->isDown()||c->isUnconcious())return false;
    OrdersReceiver* orders=c->getOrdersReciever();if(!orders)return false;
    if(!g_gatePassage||!g_gateHooksReady||g_gateConflict||AIFixedGate::Expired(now,state.created))
    {
        if(state.stage==1&&OwnOpenOrder(orders,state))ResumeGateMove(c,state);
        return false;
    }
    if(state.stage==0)
    {
        if(!AtMove(orders->getFirstOrder(),state.destination)||orders->orders.getSecondTask())return false;
        const Ogre::Vector3 here=c->getPosition();
        const float moved=(here-state.sample).squaredLength();
        if(moved>.25f){state.sample=here;state.sampledAt=now;return true;}
        if(!AIFixedGate::Ready(now,state.inputAt,state.sampledAt,moved)||now-state.checkedAt<1000)return true;
        if(!g_gateQueryBudget)return true;
        --g_gateQueryBudget;
        state.checkedAt=now;
        lektor<RootObject*>& objects=GateObjects();
        ou->getObjectsWithinSphere(objects,here,float(g_gateRadius),BUILDING,128,NULL);
        if(objects.size()>=128)return true;
        DoorStuff* best=NULL;Building* bestParent=NULL;float bestDistance=0;
        for(uint32_t i=0;i<objects.size();++i)
        {
            // The native query is explicitly BUILDING; DoorStuff also derives Building.
            Building* b=static_cast<Building*>(objects[i]);
            if(!b||b->getSpecialFunction()!=BF_GATE||!b->isGate()||b->getFaction()!=c->getFaction())continue;
            DoorStuff* d=b->getDoor();
            if(!d||!d->isSetupComplete||d->isBroken()||d->getDoorOpenAmount()>.05f||
               (d->isLocked()&&!g_gateUnlock))continue;
            if(!AIFixedGate::Crosses(GatePoint(here),GatePoint(state.destination),
                GatePoint(d->getDoorPosInside()),GatePoint(d->getDoorPosOutside()),8))continue;
            const float distance=(here-d->getDoorPosition()).squaredLength();
            if(!best||distance<bestDistance){best=d;bestParent=b;bestDistance=distance;}
        }
        if(!best)return true;
        state.gate=hand(bestParent);state.leaf=hand(best);state.wasLocked=best->isLocked();
        {IntentGuard guard;state.doorSerial=g_gateDoorChanges[state.leaf];}
        state.startSide=AIFixedGate::Side(GatePoint(here),GatePoint(best->getDoorPosInside()),GatePoint(best->getDoorPosOutside()));
        state.stage=1;
        OpenGateFor(c,best,state.wasLocked);
        if(g_debug)DebugLog("AIFixed: native gate passage order issued");
        return true;
    }
    DoorStuff* door=LiveGate(state);
    if(!door||door->getFaction()!=c->getFaction())return false;
    {IntentGuard guard;GateDoorChanges::const_iterator changed=g_gateDoorChanges.find(state.leaf);
     if(changed==g_gateDoorChanges.end()||changed->second!=state.doorSerial)return false;}
    if(state.stage==1)
    {
        if(door->isLocked())return false; // another owner changed the lock
        if(door->getDoorOpenAmount()>=.9f)
        {
            if(!OwnOpenOrder(orders,state)&&!AtMove(orders->getFirstOrder(),state.destination))return false;
            ResumeGateMove(c,state);state.stage=2;
        }
        return true;
    }
    if(state.stage==2)
    {
        const int side=AIFixedGate::Side(GatePoint(c->getPosition()),GatePoint(door->getDoorPosInside()),GatePoint(door->getDoorPosOutside()));
        if(!side||side==state.startSide)return true;
        if(!state.passedAt)state.passedAt=now;
        if(!g_gateClose)return false;
        if(now-state.passedAt<DWORD(g_gateCloseDelay*1000)||GateOccupied(door))return true;
        // Do not close while another tracked actor is still approaching this gate.
        for(GateMap::const_iterator it=g_gateStates.begin();it!=g_gateStates.end();++it)
            if(!SameIdentity(it->first,state.actor)&&SameIdentity(it->second.gate,state.gate)&&it->second.stage<2)return true;
        if(!OwnCloseGate(door))return false;
        state.stage=3;return true;
    }
    if(door->getDoorOpenAmount()>.01f)return true;
    if(state.wasLocked&&g_gateRelock)OwnLockGate(door);
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
        if(g_gateStates.find(move.actor)==g_gateStates.end()&&g_gateStates.size()>=128)continue;
        GateState state={};state.actor=move.actor;state.destination=move.destination;
        state.created=state.inputAt=now;state.serial=move.serial;
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
        if(!SafeGateOne(it->second,now))g_gateStates.erase(it);
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
