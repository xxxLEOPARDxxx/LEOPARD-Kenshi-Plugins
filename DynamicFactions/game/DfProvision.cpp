#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfProvision.h"
#include "game/DfMine.h"
#include "core/DfSupply.h"
#include "ui/DfTradeReceipt.h"
#include <Windows.h>
#include <Debug.h>
#include <climits>
#include <float.h>
#include <cstdio>
#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/Platoon.h>

namespace
{
    bool Same(const hand& a,const hand& b)
    {DfWorkHandLess less;return !less(a,b) && !less(b,a);}
    bool Future(unsigned long now,unsigned long deadline)
    {return deadline!=0 && static_cast<long>(now-deadline)<0;}
    void ClearRoute(DfProvision& rt,Character* c)
    {
        if(c!=NULL && rt.moving && DfGoalOf(c)==MOVE_ON_FREE_WILL) DfClearOrders(c);
        rt.member=hand();rt.trader=hand();rt.memberName.clear();rt.failedShops.clear();
        rt.moving=false;rt.progressAt=0;rt.startedAt=0;
    }
    void Delay(DfProvision& rt,Character* c,unsigned long now)
    {
        if(c!=NULL) rt.cooldown[c->getHandle()]=now+30000;
        ClearRoute(rt,c);
    }
    bool Food(Character* c,Faction* f,float& rate,float& hunger,float& stock)
    {
        return c!=NULL && c->getFaction()==f && !c->isDead() && DfReadFood(c,rate,hunger,stock);
    }
    bool Failed(const DfProvision& rt,const hand& h)
    {for(size_t i=0;i<rt.failedShops.size();++i)if(Same(h,rt.failedShops[i]))return true;return false;}
    Character* ChooseTrader(Character* c,const std::vector<DfTrader>& traders,const DfProvision& rt)
    {
        Character* best=NULL;float distance=0;
        for(size_t i=0;i<traders.size() && i<128;++i)
        {
            if(Failed(rt,traders[i].who)) continue;
            Character* const t=traders[i].who.getCharacter();
            if(t==NULL || t->isDead() || t->isUnconcious() || t->isInCombatMode(true,true) ||
               c->isEnemy(t,false) || t->isEnemy(c,false)) continue;
            const float d=c->getPosition().squaredDistance(t->getPosition());
            if(_finite(d) && (best==NULL || d<distance)) {best=t;distance=d;}
        }
        return best;
    }
}

bool DfProvisionOwns(const DfProvision& rt,Character* actor)
{return actor!=NULL && !rt.member.isNull() && Same(rt.member,actor->getHandle());}

void DfProvisionStep(DfFactionState& s,DfProvision& rt,Faction* f,
    const std::vector<Character*>& people,const std::vector<DfTrader>& traders,
    Character* envoy,int committed,double hour,unsigned long now,bool debug)
{
    if(f==NULL || DfTransferFaulted() || people.empty() || people.size()>512) return;
    // Cooldowns are bounded by the current faction, including handle serials.
    for(std::map<hand,unsigned long,DfWorkHandLess>::iterator it=rt.cooldown.begin();it!=rt.cooldown.end();)
    {
        Character* const c=it->first.getCharacter();
        if(c==NULL || c->getFaction()!=f || !Future(now,it->second))rt.cooldown.erase(it++);else ++it;
    }
    Character* actor=rt.member.isNull() ? NULL : rt.member.getCharacter();
    if(actor!=NULL && (actor->getFaction()!=f || actor==envoy || !DfEconomyAvailable(actor)))
    {ClearRoute(rt,NULL);actor=NULL;}
    if(actor==NULL && !rt.member.isNull()) ClearRoute(rt,NULL);
    if(actor!=NULL && now-rt.startedAt>=180000)
    {
        // Real movement alone cannot keep an unreachable/chasing route alive
        // forever and starve all other members of the single supply slot.
        if(debug) DebugLog(std::string("DynamicFactions: food route yielded after deadline: ")+rt.memberName);
        Delay(rt,actor,now);return;
    }
    if(actor==NULL)
    {
        Character* best=NULL;float lowest=0;DfFoodNeed bestNeed=DF_FOOD_UNKNOWN;
        const size_t count=people.size()<4 ? people.size() : 4;
        for(size_t k=0;k<count;++k)
        {
            Character* const c=people[rt.scanCursor%people.size()];++rt.scanCursor;
            float rate=0,hunger=0,stock=0,target=0;
            if(!Food(c,f,rate,hunger,stock))
            {
                if(!rt.warnedUnreadable) {ErrorLog("DynamicFactions: food inventory unreadable; supply deferred");rt.warnedUnreadable=true;}
                continue;
            }
            if(c==envoy || !DfEconomyAvailable(c) || rt.cooldown.find(c->getHandle())!=rt.cooldown.end()) continue;
            const DfFoodNeed need=DfPlanFood(rate,hunger,stock,false,target);
            if(need!=DF_FOOD_RESTOCK && need!=DF_FOOD_URGENT) continue;
            if(best==NULL || (need==DF_FOOD_URGENT && bestNeed!=DF_FOOD_URGENT) ||
               (need==bestNeed && hunger<lowest)) {best=c;lowest=hunger;bestNeed=need;}
        }
        if(best==NULL) return;
        actor=best;rt.member=actor->getHandle();rt.memberName=actor->getName();
        rt.progressPosition=actor->getPosition();rt.progressAt=now;rt.startedAt=now;
        const int goal=DfGoalOf(actor);
        if(goal==OPERATE_MACHINERY || goal==MOVE_ON_FREE_WILL) DfClearOrders(actor);
        if(debug) DebugLog(std::string("DynamicFactions: real food supply started for ")+rt.memberName);
    }
    float rate=0,hunger=0,stock=0,target=0;
    if(!Food(actor,f,rate,hunger,stock)) {Delay(rt,actor,now);return;}
    const DfFoodNeed need=DfPlanFood(rate,hunger,stock,true,target);
    if(need==DF_FOOD_STOCKED || need==DF_FOOD_NOT_NEEDED || need==DF_FOOD_UNKNOWN)
    {ClearRoute(rt,actor);return;}
    Character* trader=rt.trader.isNull() ? NULL : rt.trader.getCharacter();
    if(trader==NULL || trader->isDead() || trader->isUnconcious() || trader->isInCombatMode(true,true))
    {
        trader=ChooseTrader(actor,traders,rt);
        if(trader==NULL) {Delay(rt,actor,now);return;}
        rt.trader=trader->getHandle();
    }
    const Ogre::Vector3 destination=trader->getPosition();
    const float d=actor->getPosition().squaredDistance(destination);
    if(!_finite(d)) {Delay(rt,actor,now);return;}
    if(d>kDfTradeRange*kDfTradeRange)
    {
        if(actor->getPosition().squaredDistance(rt.progressPosition)>2.25f)
        {rt.progressPosition=actor->getPosition();rt.progressAt=now;}
        else if(now-rt.progressAt>=30000)
        {
            if(rt.failedShops.size()<128)rt.failedShops.push_back(rt.trader);
            rt.trader=hand();rt.progressAt=now;return;
        }
        if(!rt.moving || rt.destination.squaredDistance(destination)>4 || DfGoalOf(actor)!=MOVE_ON_FREE_WILL)
        {actor->setDestination(destination,false);rt.destination=destination;rt.moving=true;}
        return;
    }
    rt.progressAt=now;rt.progressPosition=actor->getPosition();
    int paid=0;rt.lastItemName.clear();DfBuyStatus status=DF_BUY_INVALID;
    const bool ok=DfBuyFood(actor,trader,f,committed,paid,rt.lastItemName,&status);
    if(paid>0)
    {
        if(rt.spent<=INT_MAX-paid)rt.spent+=paid;
        char event[64];sprintf_s(event,sizeof(event),"food %d",paid);DfAddEvent(s,hour,event);
        DfShowTradeReceipt(actor,-paid);
        if(debug)
        {
            DebugLog(std::string("DynamicFactions: confirmed food purchase: ")+rt.lastItemName+"; buyer "+rt.memberName+"; "+event);
            char detail[192];sprintf_s(detail,sizeof(detail),
                "DynamicFactions: food pre-purchase: hunger %.4f; stock %.4f; target %.4f; racial rate %.4f",hunger,stock,target,rate);
            DebugLog(detail);
        }
    }
    if(!ok && DfTransferFaulted()) return;
    if(debug && paid==0)
    {
        const char* reason="invalid deal";
        switch(status)
        {
        case DF_BUY_OK:reason="no confirmed transfer";break;
        case DF_BUY_NO_FOOD:reason="no eligible food in native stock";break;
        case DF_BUY_NO_MONEY:reason="budget or merchant wallet limit";break;
        case DF_BUY_NO_ROOM:reason="no inventory fit or transfer refused";break;
        case DF_BUY_PLAYER_TRADING:reason="player trading or deal conditions changed";break;
        case DF_BUY_FAULT:reason="transaction quarantined";break;
        default:break;
        }
        DebugLog(std::string("DynamicFactions: food shop rejected: ")+trader->getName()+"; "+reason);
    }
    if(status==DF_BUY_NO_MONEY && f->factionOwnerships!=NULL && f->factionOwnerships->money<=committed)
    {Delay(rt,actor,now);return;}
    if(status==DF_BUY_NO_FOOD || status==DF_BUY_INVALID || status==DF_BUY_PLAYER_TRADING ||
       status==DF_BUY_NO_MONEY || status==DF_BUY_NO_ROOM)
    {
        if(rt.failedShops.size()<128)rt.failedShops.push_back(rt.trader);
        rt.trader=hand();
    }
}
