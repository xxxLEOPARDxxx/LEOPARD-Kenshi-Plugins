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
        rt.kind=DF_SUPPLY_FOOD;rt.light=true;
    }
    void Delay(DfProvision& rt,Character* c,unsigned long now)
    {
        if(c!=NULL) rt.cooldown[c->getHandle()]=now+30000;
        ClearRoute(rt,c);
    }
    bool ReadStock(Character* c,Faction* f,DfSupplyStock& stock)
    {
        return c!=NULL && c->getFaction()==f && !c->isDead() && DfReadSupplyStock(c,stock);
    }
    int SupplyPriority(const DfSupplyStock& stock,DfSupplyKind kind,bool restocking)
    {
        if(!stock.known) return 100;
        if(kind==DF_SUPPLY_BACKPACK) return !stock.animal && !stock.wearingBag ? 3 : 100;
        float target=0;const DfFoodNeed need=DfPlanSupply(stock,kind,restocking,target);
        if(need!=DF_FOOD_RESTOCK && need!=DF_FOOD_URGENT) return 100;
        return kind==DF_SUPPLY_FOOD ? (need==DF_FOOD_URGENT ? 0 : 1) : 2;
    }
    DfSupplyKind NextKind(const DfSupplyStock& stock)
    {
        DfSupplyKind best=DF_SUPPLY_FOOD;int priority=100;
        for(int k=DF_SUPPLY_FOOD;k<=DF_SUPPLY_BACKPACK;++k)
        {
            const DfSupplyKind kind=static_cast<DfSupplyKind>(k);
            const int p=SupplyPriority(stock,kind,false);
            if(p<priority){best=kind;priority=p;}
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
        if(debug) DebugLog(std::string("DynamicFactions: supply route yielded after deadline: ")+rt.memberName);
        Delay(rt,actor,now);return;
    }
    DfSupplyStock current;
    if(actor!=NULL && !ReadStock(actor,f,current)){Delay(rt,actor,now);return;}
    int priority=actor!=NULL ? SupplyPriority(current,rt.kind,true) : 100;
    if(actor!=NULL)
    {
        const DfSupplyKind next=NextKind(current);
        const int p=SupplyPriority(current,next,false);
        if(p<priority)
        {
            // A traveller's own food can run low while seeking a bag or kit.
            ClearRoute(rt,actor);rt.member=actor->getHandle();rt.memberName=actor->getName();
            rt.kind=next;rt.light=DfLightBagRole(actor);priority=p;
            rt.progressPosition=actor->getPosition();rt.progressAt=now;rt.startedAt=now;
        }
    }
    if(actor!=NULL && priority==100){ClearRoute(rt,actor);return;}
    Character* best=NULL;DfSupplyStock bestStock;DfSupplyKind bestKind=DF_SUPPLY_FOOD;
    // A bounded scan continues while travelling: food can preempt optional gear.
    const size_t count=people.size()<4 ? people.size() : 4;
    for(size_t k=0;k<count;++k)
    {
        Character* const c=people[rt.scanCursor%people.size()];++rt.scanCursor;
        if(c==NULL || c==actor || c==envoy || !DfEconomyAvailable(c) ||
            rt.cooldown.find(c->getHandle())!=rt.cooldown.end()) continue;
        DfSupplyStock stock;
        if(!ReadStock(c,f,stock))
        {
            if(!rt.warnedUnreadable){ErrorLog("DynamicFactions: supplies unreadable; procurement deferred");rt.warnedUnreadable=true;}
            continue;
        }
        const DfSupplyKind kind=NextKind(stock);const int p=SupplyPriority(stock,kind,false);
        if(p<priority){best=c;bestStock=stock;bestKind=kind;priority=p;}
    }
    if(best!=NULL)
    {
        ClearRoute(rt,actor);actor=best;current=bestStock;rt.kind=bestKind;
        rt.member=actor->getHandle();rt.memberName=actor->getName();rt.light=DfLightBagRole(actor);
        rt.progressPosition=actor->getPosition();rt.progressAt=now;rt.startedAt=now;
        const int goal=DfGoalOf(actor);
        if(goal==OPERATE_MACHINERY || goal==MOVE_ON_FREE_WILL) DfClearOrders(actor);
        if(debug) DebugLog(std::string("DynamicFactions: real supply route started for ")+rt.memberName);
    }
    if(actor==NULL) return;
    if(rt.kind==DF_SUPPLY_BACKPACK && current.ownedBags>0)
    {
        // An owned bag, even if nested or currently unwearable, prevents buying
        // duplicates. Only a root inventory bag can be safely equipped here.
        bool equipped=false;
        if(!DfEquipOwnedBag(actor,rt.light,equipped)) return;
        if(debug) DebugLog(std::string(equipped ? "DynamicFactions: equipped owned backpack: " :
            "DynamicFactions: owned backpack retained; equip deferred: ")+rt.memberName);
        Delay(rt,actor,now);return;
    }
    Character* trader=rt.trader.isNull() ? NULL : rt.trader.getCharacter();
    if(trader==NULL || trader->isDead() || trader->isUnconcious() || trader->isInCombatMode(true,true))
    {
        trader=DfChooseSupplyTrader(actor,rt.kind,rt.light,traders,rt.market,rt.failedShops,now);
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
    DfBudgetSnapshot budget;budget.committed=committed;
    budget.money=f->factionOwnerships!=NULL ? f->factionOwnerships->money : -1;
    const bool forecast=rt.kind==DF_SUPPLY_FOOD ? false : DfForecastSupplies(f,people,rt.market,committed,now,budget);
    int paid=0;rt.lastItemName.clear();DfBuyStatus status=DF_BUY_INVALID;
    std::vector<DfSupplyQuote> observed;
    const bool ok=DfBuySupply(actor,trader,f,rt.kind,rt.light,budget,paid,rt.lastItemName,observed,&status);
    if(ok && status!=DF_BUY_PLAYER_TRADING && status!=DF_BUY_INVALID && status!=DF_BUY_FAULT)
        DfRememberShop(rt.market,rt.trader,observed,now);
    if(paid>0)
    {
        if(rt.spent<=INT_MAX-paid)rt.spent+=paid;
        const char* type=rt.kind==DF_SUPPLY_FOOD ? "food" : rt.kind==DF_SUPPLY_FIRSTAID ? "aid" :
            rt.kind==DF_SUPPLY_REPAIR ? "repair" : "bag";
        char event[64];sprintf_s(event,sizeof(event),"%s %d",type,paid);DfAddEvent(s,hour,event);
        DfShowTradeReceipt(actor,-paid);
        if(debug)
        {
            DebugLog(std::string("DynamicFactions: confirmed supply purchase: ")+rt.lastItemName+"; buyer "+rt.memberName+"; "+event);
            char detail[192];sprintf_s(detail,sizeof(detail),
                "DynamicFactions: supply reserve: known %d; food %d; medical %d; committed %d; buffer %d",
                budget.forecastKnown?1:0,budget.foodDeficitCost,budget.medicalDeficitCost,budget.committed,budget.emergencyBuffer);
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
        case DF_BUY_NO_FOOD:reason="no eligible supplies in native stock";break;
        case DF_BUY_NO_MONEY:reason="budget or merchant wallet limit";break;
        case DF_BUY_NO_ROOM:reason="no inventory fit or transfer refused";break;
        case DF_BUY_PLAYER_TRADING:reason="player trading or deal conditions changed";break;
        case DF_BUY_FAULT:reason="transaction quarantined";break;
        default:break;
        }
        DebugLog(std::string("DynamicFactions: supply shop rejected: ")+trader->getName()+"; "+reason);
    }
    if(status==DF_BUY_NO_MONEY && f->factionOwnerships!=NULL && f->factionOwnerships->money<=committed)
    {Delay(rt,actor,now);return;}
    if(ok && status==DF_BUY_NO_MONEY && rt.kind!=DF_SUPPLY_FOOD && !forecast)
    {
        // This physical visit may have supplied the missing price knowledge.
        // Wait until the next step before buying; never replay the transaction.
        DfBudgetSnapshot refreshed;
        if(DfForecastSupplies(f,people,rt.market,committed,now,refreshed)) return;
    }
    if(status==DF_BUY_NO_FOOD || status==DF_BUY_INVALID || status==DF_BUY_PLAYER_TRADING ||
       status==DF_BUY_NO_MONEY || status==DF_BUY_NO_ROOM)
    {
        if(rt.failedShops.size()<128)rt.failedShops.push_back(rt.trader);
        rt.trader=hand();
    }
}
