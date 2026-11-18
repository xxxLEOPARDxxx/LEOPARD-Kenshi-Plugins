#pragma once
// Included inside AIFixed's private namespace, after WorldActive. Read-only
// adapters run synchronously on the native caller thread; no world scan or
// borrowed Character/Building/Tasker is stored by this host bridge.
int EngineerNativeFaultFilter(unsigned code)
{return code==0xe06d7363?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;}
bool EngineerCurrent(unsigned* generation)
{
    const LONG before=InterlockedCompareExchange(&g_loadGeneration,0,0);
    if(!generation||!InterlockedCompareExchange(&g_engineerReady,0,0)||
       !InterlockedCompareExchange(&g_engineers,0,0)||
       InterlockedCompareExchange(&g_loading,0,0)||!InterlockedCompareExchange(&g_engineerGlobalEnabled,0,0)||
       !g_lifecycleReady||!g_newHooksReady||!WorldActive(GetTickCount()))return false;
    if(before!=InterlockedCompareExchange(&g_loadGeneration,0,0))return false;
    *generation=static_cast<unsigned>(before);return true;
}
bool EngineerAdmitted()
{unsigned generation=0;return EngineerCurrent(&generation);}
bool EngineerDebug()
{return InterlockedCompareExchange(&g_engineerDebug,0,0)!=0;}
LONG EngineerEpoch()
{return InterlockedCompareExchange(&g_loadGeneration,0,0);}
bool EngineerAllowed(Character* c)
{
    __try
    {
        unsigned generation=0;
        if(!EngineerCurrent(&generation)||!c||!c->isPlayerCharacter()||
           c->isDead()||c->isDown()||c->isUnconcious()||c->isBeingCarried()||
           c->isCarryingSomething||c->isChained||c->isUsingTurret||
           c->inSomething!=IN_NOTHING||c->isInCombatMode(true,true))return false;
        OrdersReceiver* orders=c->getOrdersReciever();
        return orders&&orders->isJobsEnabled()&&!orders->hasPlayerOrders()&&
            generation==static_cast<unsigned>(EngineerEpoch());
    }
    __except(EngineerNativeFaultFilter(GetExceptionCode())){return false;}
}
bool EngineerAllowActor(AI* ai)
{
    __try{return ai&&EngineerAllowed(ai->me);}
    __except(EngineerNativeFaultFilter(GetExceptionCode())){return false;}
}
bool EngineerSnapshot(Building* b,AIFixedEngineerPriority::Candidate* out)
{
    __try
    {
        unsigned generation=0;
        if(!out||!EngineerCurrent(&generation)||!ou||!ou->player||
           (ou->player->levelEditor&&ou->player->levelEditor->levelEditModeOn)||
           !b||b->isDisabled()||!b->getFaction()||!b->getFaction()->isPlayer||
           b->handle.type!=BUILDING)return false;
        Building::ConstructionState* state=b->getBuildState();
        if(!state)return false;
        AIFixedEngineerPriority::Candidate result={};
        result.key.type=b->handle.type;result.key.container=b->handle.container;
        result.key.containerSerial=b->handle.containerSerial;result.key.index=b->handle.index;
        result.key.serial=b->handle.serial;result.wall=b->isAWall();
        result.complete=state->isComplete;result.paused=state->isPaused;
        result.dismantled=state->isDismantled;
        switch(b->getBuildingClass())
        {
        case BCTYPE_PRODUCTION:case BCTYPE_CRAFTING:case BCTYPE_FARM:
        case BCTYPE_RESEARCH:case BCTYPE_ITEM_FURNACE:result.productive=!result.wall;break;
        default:break;
        }
        // Batteries/engines and some data-driven utility machines share the
        // generic usable class; use native function enums, never translated names.
        switch(b->getSpecialFunction())
        {
        case BF_MINE:case BF_MINE_NATURAL:case BF_REFINERY:case BF_GENERATOR:
        case BF_CRAFTING:case BF_RESEARCH:case BF_BATTERY:case BF_RAIN_COLLECTOR:
        case BF_ENGINE:result.productive=!result.wall;break;
        default:break;
        }
        if(generation!=static_cast<unsigned>(EngineerEpoch()))return false;
        *out=result;return true;
    }
    __except(EngineerNativeFaultFilter(GetExceptionCode())){return false;}
}
Building* EngineerResolve(const AIFixedEngineerPriority::Key& key)
{
    __try
    {
        unsigned generation=0;
        if(key.type!=BUILDING||!EngineerCurrent(&generation))return NULL;
        const hand handle(key.index,key.serial,BUILDING,key.container,key.containerSerial);
        Building* b=handle.getBuilding();
        if(!b||b->handle.type!=key.type||b->handle.container!=key.container||
           b->handle.containerSerial!=key.containerSerial||b->handle.index!=key.index||
           b->handle.serial!=key.serial||generation!=static_cast<unsigned>(EngineerEpoch()))return NULL;
        return b;
    }
    __except(EngineerNativeFaultFilter(GetExceptionCode())){return NULL;}
}
