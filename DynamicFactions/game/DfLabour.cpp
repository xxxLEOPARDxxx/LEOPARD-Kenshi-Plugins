#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define KLOC_DOMAIN "dynamic_factions"
#include <Localization.h>
#include "game/DfLabour.h"
#include "core/DfHaul.h"
#include "game/DfGame.h"
#include "game/DfSell.h"
#include "ui/DfTradeReceipt.h"
#include <Windows.h>
#include <Debug.h>
#include <climits>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <set>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/Inventory.h>
#include <kenshi/Town.h>
#include <kenshi/Building/UseableStuff.h>

namespace
{
    typedef std::map<hand,DfMemberWork,DfWorkHandLess> MemberMap;
    Ogre::Vector3 Pos(const DfKnownNode& n) { return Ogre::Vector3(n.x,n.y,n.z); }
    bool Able(Character* c)
    {
        return c != NULL && !c->isDead() && !c->isUnconcious() && !c->isInCombatMode(true,true);
    }
    void Stop(Character* c, DfMemberWork& w)
    {
        if (w.ordered)
        {
            const int goal=DfGoalOf(c);
            if(goal==OPERATE_MACHINERY || (w.moving && goal==MOVE_ON_FREE_WILL)) DfClearOrders(c);
        }
        w.ordered = w.moving = false;
    }
    void Move(Character* c, DfMemberWork& w, const Ogre::Vector3& target)
    {
        const bool newTarget=!w.moving || w.destination.squaredDistance(target)>4.0f;
        if (newTarget) w.progressAt=0;
        if (newTarget || DfGoalOf(c) != MOVE_ON_FREE_WILL)
        {
            c->setDestination(target,false);
            w.destination = target;
            w.ordered = w.moving = true;
        }
    }
    int Known(DfFactionState& s, const DfNode& n, const std::vector<Character*>&)
    {
        if (n.item == NULL || n.itemValue <= 0) return -1;
        for (size_t k=0;k<s.nodes.size();++k)
            if (s.nodes[k].itemSid == n.item->stringID && Pos(s.nodes[k]).squaredDistance(n.pos) < 9.0f)
                return static_cast<int>(k);
        // Public local deposits come from the bounded home-area query. They do
        // not require a speculative scouting circuit before actual mining.
        if (s.nodes.size() >= 512) return -1;
        DfKnownNode k; k.x=n.pos.x;k.y=n.pos.y;k.z=n.pos.z;k.itemSid=n.item->stringID;k.stored=0;
        s.nodes.push_back(k);
        return static_cast<int>(s.nodes.size()-1);
    }
    int OwnOperators(const DfNode& n, Faction* faction)
    {
        Building* const b=n.building.getBuilding();
        UseableStuff* const u=b != NULL ? b->getUseableStuff() : NULL;
        if (u==NULL) return 0;
        int own=0;
        typedef std::set<hand,std::less<hand>,Ogre::STLAllocator<hand,Ogre::GeneralAllocPolicy> > Operators;
        for (Operators::const_iterator it=u->currentOperators.begin();it!=u->currentOperators.end();++it)
        {
            Character* c=it->getCharacter();
            if (c!=NULL && c->getFaction()==faction) ++own;
        }
        return own;
    }
    bool SameHand(const hand& a,const hand& b)
    { DfWorkHandLess less; return !less(a,b) && !less(b,a); }
}

void DfLabourStep(DfFactionState& s,DfLabour& rt,Faction* f,TownBase* hub,Character* envoy,
    bool traderHasMoney,double hour,unsigned long clockMs,bool debug,float radius)
{
    (void)traderHasMoney;
    if (f==NULL || hub==NULL || DfTransferFaulted()) return;
    std::vector<Character*> people;DfFactionMembers(f,people);
    if (people.empty()) return;
    const Ogre::Vector3 home=hub->getPosition();
    // One shared spatial query per faction step, not per worker and saved node.
    std::vector<DfNode> loaded;DfFindNodes(home,radius,NULL,loaded);
    std::vector<int> indices(loaded.size(),-1);
    for(size_t k=0;k<loaded.size();++k)
    {
        Building* b=loaded[k].building.getBuilding();
        if(!DfCanHarvestNode(b,f)) continue;
        indices[k]=Known(s,loaded[k],people);
    }
    const size_t n=s.nodes.size();
    std::vector<DfNodeState> nodes(n);
    std::vector<int> live(n,-1);
    std::vector<GameData*> types;
    for(size_t k=0;k<n;++k)
    {
        int value=0;GameData* item=DfItemBySid(s.nodes[k].itemSid,value);
        if(item!=NULL && std::find(types.begin(),types.end(),item)==types.end()) types.push_back(item);
        nodes[k].value=0;nodes[k].distance=Pos(s.nodes[k]).distance(home);
        nodes[k].stored=0;nodes[k].cap=0;nodes[k].maxMiners=0;
    }
    for(size_t k=0;k<loaded.size();++k)
    {
        const int j=indices[k];if(j<0 || live[j]>=0) continue;
        const DfNode& a=loaded[k];live[j]=static_cast<int>(k);
        int stock=DfCountInNode(a.building,a.item);
        if(stock<0) {live[j]=-1;continue;}
        nodes[j].value=a.itemValue;nodes[j].stored=stock;
        nodes[j].cap=a.capacity>0?a.capacity:INT_MAX; // unknown: native inventory remains authoritative
        nodes[j].maxMiners=(std::min)(a.maxMiners,a.freeSlots+OwnOperators(a,f));
    }
    // Only native items are cargo. Recreate this view on every tick and after load.
    std::vector<DfMember> ms(people.size());
    std::vector<GameData*> cargo(people.size(),NULL);
    std::set<hand,DfWorkHandLess> living;
    for(size_t i=0;i<people.size();++i)
    {
        Character* c=people[i];const hand h=c->getHandle();living.insert(h);
        DfMemberWork& w=rt.members[h];int units=0;
        DfFindCarriedOre(c,types,cargo[i],units);
        if(units<0) {DfQuarantineTransfer("DynamicFactions: inventory snapshot unreadable - work stopped");return;}
        if(units>0)
            for(size_t k=0;k<n;++k) if(s.nodes[k].itemSid==cargo[i]->stringID) {w.node=static_cast<int>(k);break;}
        ms[i].id=static_cast<int>(i);ms[i].able=Able(c);ms[i].hiring=c==envoy;
        ms[i].node=w.node;ms[i].role=w.role;ms[i].carried=units;
        ms[i].canHaul=w.blockedUntil==0 || static_cast<long>(clockMs-w.blockedUntil)>=0;
    }
    for(MemberMap::iterator it=rt.members.begin();it!=rt.members.end();)
        if(living.find(it->first)==living.end()) rt.members.erase(it++);else ++it;
    std::vector<DfTrader> traders;DfFindTraders(hub,traders);
    bool money=false;for(size_t t=0;t<traders.size();++t) if(traders[t].money>0) money=true;
    DfAssignRoles(ms,nodes,true,money);
    rt.carried=0;
    for(size_t i=0;i<people.size();++i)
    {
        if(DfTransferFaulted()) break;
        Character* c=people[i];DfMemberWork& w=rt.members[c->getHandle()];
        const bool changed=w.role!=ms[i].role || w.node!=ms[i].node;
        if(changed)
        {
            // Hiring already issued an envoy order earlier in this same tick.
            if(!ms[i].hiring) Stop(c,w);else w.ordered=w.moving=false;
            w.trader=hand();w.progressAt=clockMs;w.progressPosition=c->getPosition();
        }
        w.role=ms[i].role;w.node=ms[i].node;rt.carried+=ms[i].carried;
        if(!ms[i].able || ms[i].hiring) {if(!ms[i].hiring) Stop(c,w);continue;}
        if(w.blockedUntil!=0 && static_cast<long>(clockMs-w.blockedUntil)<0) continue;
        if(w.moving)
        {
            if(w.progressAt==0 || c->getPosition().squaredDistance(w.progressPosition)>2.25f)
            {w.progressAt=clockMs;w.progressPosition=c->getPosition();}
            else if(clockMs-w.progressAt>=30000)
            {
                Stop(c,w);w.failedTrader=w.trader;w.blockedUntil=clockMs+30000;
                if(ms[i].carried==0) {w.role=DF_ROLE_IDLE;w.node=-1;}
                continue;
            }
        }
        const int j=w.node;
        if(w.role==DF_ROLE_MINE && j>=0 && live[j]>=0)
        {
            const DfNode& mine=loaded[live[j]];
            const bool inRange=c->getPosition().squaredDistance(mine.pos)<=mine.takeRange*mine.takeRange;
            if(!inRange)
            {
                if(w.progressAt==0 || c->getPosition().squaredDistance(w.progressPosition)>2.25f)
                {w.progressAt=clockMs;w.progressPosition=c->getPosition();}
                else if(clockMs-w.progressAt>=30000)
                {Stop(c,w);w.blockedUntil=clockMs+30000;w.role=DF_ROLE_IDLE;w.node=-1;continue;}
            }
            else w.progressAt=clockMs;
            if(changed || DfGoalOf(c)!=OPERATE_MACHINERY)
            {w.ordered=DfOrderMine(c,mine);w.moving=false;}
            continue;
        }
        if(w.role!=DF_ROLE_HAUL || j<0 || j>=static_cast<int>(n))
        {
            continue;
        }
        GameData* item=cargo[i];
        if(item==NULL) {int value=0;item=DfItemBySid(s.nodes[j].itemSid,value);}
        if(item==NULL) continue;
        Character* trader=NULL;float best=1e30f;
        for(size_t t=0;t<traders.size();++t)
        {
            Character* candidate=traders[t].who.getCharacter();
            if(candidate==NULL || candidate->getFaction()==f || traders[t].money<=0 || !Able(candidate)) continue;
            if(SameHand(w.failedTrader,candidate->getHandle()) && w.blockedUntil!=0 && clockMs-w.blockedUntil<90000) continue;
            if(ms[i].carried>0 && !DfTraderCanBuy(c,candidate,f,item)) continue;
            const float distance=candidate->getPosition().squaredDistance(c->getPosition());
            if(distance<best) {best=distance;trader=candidate;}
        }
        if(trader==NULL || !SameHand(w.trader,trader->getHandle()))
        {w.trader=trader!=NULL?trader->getHandle():hand();}
        const bool nodeLoaded=live[j]>=0;
        const Ogre::Vector3 nodePos=nodeLoaded?loaded[live[j]].pos:Pos(s.nodes[j]);
        const float takeRange=nodeLoaded?loaded[live[j]].takeRange:kDfTakeRange;
        DfHaulSnapshot hs={};hs.able=true;hs.carried=ms[i].carried;
        hs.nodeLoaded=nodeLoaded;hs.atNode=c->getPosition().squaredDistance(nodePos)<=takeRange*takeRange;
        hs.nodeHasOre=nodes[j].stored>0;
        hs.hasRoom=DfInventoryHasRoom(c->getInventory(),item);
        if(hs.carried==0 && !hs.hasRoom)
        {Stop(c,w);w.blockedUntil=clockMs+30000;w.role=DF_ROLE_IDLE;w.node=-1;continue;}
        hs.hasTrader=trader!=NULL;hs.atTrader=best<=kDfTradeRange*kDfTradeRange;
        switch(DfChooseHaulAction(hs))
        {
        case DF_HAUL_GO_NODE:Move(c,w,nodePos);break;
        case DF_HAUL_TAKE:
        {
            Stop(c,w);
            const int moved=DfTakeOreFromNode(c,loaded[live[j]].building,item,kDfTransferMaxUnits);
            if(moved<=0) {w.blockedUntil=clockMs+30000;w.role=DF_ROLE_IDLE;w.node=-1;}
            break;
        }
        case DF_HAUL_GO_TRADER:Move(c,w,trader->getPosition());break;
        case DF_HAUL_SELL:
        {
            Stop(c,w);int sold=0,paid=0;DfSellStatus status=DF_SELL_OK;
            const bool ok=DfSellOre(c,trader,f,item,kDfTransferMaxUnits,sold,paid,&status);
            if(sold>0)
            {
                if(rt.sold<=INT_MAX-sold) rt.sold+=sold;
                if(rt.revenue<=INT_MAX-paid) rt.revenue+=paid;
                char event[96];sprintf_s(event,sizeof(event),"sale %d %d",sold,paid);DfAddEvent(s,hour,event);
                if(debug) DebugLog(std::string("DynamicFactions: ")+event);
                DfShowTradeReceipt(c,paid);
                if(debug) DebugLog(std::string("DynamicFactions: sale confirmed to shop of ")+trader->getName()+
                    "; item "+item->stringID);
            }
            if(status==DF_SELL_NO_STOCK_ROOM)
            {w.failedTrader=w.trader;w.blockedUntil=clockMs+30000;}
            else if(status==DF_SELL_PLAYER_TRADING || status==DF_SELL_INVALID)
                w.blockedUntil=clockMs+5000;
            if(!ok && debug) DebugLog("DynamicFactions: sale deferred; an ambiguous transfer stops the session");
            break;
        }
        default:break;
        }
    }
    if(debug && (rt.loggedMs==0 || clockMs-rt.loggedMs>=60000))
    {
        rt.loggedMs=clockMs;int miners=0,haulers=0;DfLabourCounts(rt,miners,haulers);
        char line[192];sprintf_s(line,sizeof(line),"DynamicFactions: real work: %d nodes, %d miners, %d haulers, cargo %d, sold %d for %d",static_cast<int>(n),miners,haulers,rt.carried,rt.sold,rt.revenue);DebugLog(line);
    }
}
static void StopAllLabour(DfLabour& rt, Faction* faction, Character* envoy)
{
    for(MemberMap::iterator it=rt.members.begin();it!=rt.members.end();++it)
    {Character* c=it->first.getCharacter();if(c!=NULL) Stop(c,it->second);}
    rt.members.clear();
    if(faction!=NULL)
    {
        std::vector<Character*> people;DfFactionMembers(faction,people);
        for(size_t i=0;i<people.size();++i) if(people[i]!=envoy && Able(people[i]))
        {
            const int goal=DfGoalOf(people[i]);
            if(goal==OPERATE_MACHINERY || goal==MOVE_ON_FREE_WILL) DfClearOrders(people[i]);
        }
    }
}
void DfLabourStop(DfLabour& rt, Faction* faction, Character* envoy)
{
    __try {StopAllLabour(rt,faction,envoy);}
    __except(EXCEPTION_EXECUTE_HANDLER) {rt.members.clear();ErrorLog("DynamicFactions: stopping work failed");}
}
void DfLabourCounts(const DfLabour& rt,int& miners,int& haulers)
{
    miners=haulers=0;
    for(MemberMap::const_iterator it=rt.members.begin();it!=rt.members.end();++it)
    {if(it->second.role==DF_ROLE_MINE)++miners;else if(it->second.role==DF_ROLE_HAUL)++haulers;}
}
std::string DfLabourStatus(const DfLabour& rt)
{
    int miners=0,haulers=0;DfLabourCounts(rt,miners,haulers);
    char b[256];sprintf_s(b,sizeof(b),Tr("Mining: %d; hauling: %d; cargo: %d; sales this session: %d"),miners,haulers,rt.carried,rt.revenue);
    return b;
}
