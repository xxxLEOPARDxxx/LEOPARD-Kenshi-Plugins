#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cmath>
#include <climits>
#include <float.h>
#include "game/DfMarket.h"
#include "game/DfMine.h"
#include "game/DfSell.h"
#include "game/DfHand.h"
#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/Platoon.h>
#include <kenshi/RaceData.h>

namespace
{
    bool SameShop(const hand& a,const hand& b)
    {DfWorkHandLess less;return !less(a,b) && !less(b,a);}
    const DfKnownShop* KnownShop(const DfMarketCache& market,const hand& trader,unsigned long now)
    {
        for(size_t i=0;i<market.shops.size();++i)
            if(SameShop(market.shops[i].trader,trader) && now-market.shops[i].quotedAt<=120000) return &market.shops[i];
        return NULL;
    }
    bool CanUseFood(Character* member,const DfSupplyQuote& quote)
    {
        int value=0;GameData* const data=DfItemBySid(quote.sid,value);
        return data!=NULL && member->getRace()!=NULL && member->getRace()->canEat(data,member->isAnimal()!=NULL);
    }
    bool WithinForecast(const LARGE_INTEGER& frequency,const LARGE_INTEGER& started)
    {
        LARGE_INTEGER current;
        return QueryPerformanceCounter(&current) && current.QuadPart>=started.QuadPart &&
            static_cast<double>(current.QuadPart-started.QuadPart)/frequency.QuadPart<.002;
    }
    int CheapestDeficit(Character* member,DfSupplyKind kind,float deficit,const DfMarketCache& market,unsigned long now,
        int remaining[32][128],const LARGE_INTEGER& frequency,const LARGE_INTEGER& started)
    {
        if(deficit<=0) return 0;
        double left=deficit;__int64 total=0;
        // A shared stock pool is consumed across ALL faction members. One
        // observed cheap kit cannot fund six fictitious cheap replacements.
        for(unsigned int pass=0;pass<4096 && left>0;++pass)
        {
            int shopAt=-1,offerAt=-1;double best=0;
            for(size_t i=0;i<market.shops.size();++i)
            {
                const DfKnownShop& shop=market.shops[i];
                if(now-shop.quotedAt>120000 || shop.offers.size()>128) continue;
                if(!WithinForecast(frequency,started)) return -1;
                for(size_t j=0;j<shop.offers.size();++j)
                {
                    const DfSupplyQuote& q=shop.offers[j];
                    if(remaining[i][j]<=0 || q.kind!=kind || q.price<=0 || !_finite(q.amount) || q.amount<=0 ||
                        (kind==DF_SUPPLY_FOOD && !CanUseFood(member,q))) continue;
                    const double cost=static_cast<double>(q.price)/q.amount;
                    if(shopAt<0 || cost<best){shopAt=static_cast<int>(i);offerAt=static_cast<int>(j);best=cost;}
                }
            }
            if(shopAt<0) return -1;
            const DfSupplyQuote& q=market.shops[shopAt].offers[offerAt];
            const double wanted=std::ceil(left/q.amount);
            const int available=remaining[shopAt][offerAt];
            const int units=wanted>=available ? available : static_cast<int>(wanted);
            if(units<=0) return -1;
            total+=static_cast<__int64>(units)*q.price;if(total>INT_MAX) return -1;
            left-=static_cast<double>(units)*q.amount;remaining[shopAt][offerAt]-=units;
        }
        return left<=0 ? static_cast<int>(total) : -1;
    }
    bool ForecastSupplies(Faction* f,const std::vector<Character*>& people,const DfMarketCache& market,
        int committed,unsigned long now,DfBudgetSnapshot& budget)
    {
        budget=DfBudgetSnapshot();budget.committed=committed;
        if(f==NULL || f->factionOwnerships==NULL || people.empty() || people.size()>512 || market.shops.size()>32 || committed<0) return false;
        budget.money=f->factionOwnerships->money;budget.emergencyBuffer=DfOperatingBuffer(static_cast<int>(people.size()));
        if(budget.money<0 || budget.emergencyBuffer<0) return false;
        LARGE_INTEGER frequency,started;
        if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0 || !QueryPerformanceCounter(&started)) return false;
        int remaining[32][128]={0};
        for(size_t i=0;i<market.shops.size();++i)
        {
            if(market.shops[i].offers.size()>128)return false;
            for(size_t j=0;j<market.shops[i].offers.size();++j)
                remaining[i][j]=market.shops[i].offers[j].quantity;
        }
        __int64 food=0,medical=0;
        for(size_t i=0;i<people.size();++i)
        {
            Character* const c=people[i];
            if(c==NULL || c->getFaction()!=f) return false;
            if(c->isDead()) continue;
            DfSupplyStock stock;
            if(!DfReadSupplyStock(c,stock)) return false;
            for(int k=DF_SUPPLY_FOOD;k<=DF_SUPPLY_REPAIR;++k)
            {
                const DfSupplyKind kind=static_cast<DfSupplyKind>(k);float target=0;
                const DfFoodNeed need=DfPlanSupply(stock,kind,true,target);
                if(need==DF_FOOD_UNKNOWN) return false;
                if(need==DF_FOOD_NOT_NEEDED) continue;
                const float held=kind==DF_SUPPLY_FOOD ? stock.food : kind==DF_SUPPLY_FIRSTAID ? stock.firstAid : stock.repair;
                const int cost=CheapestDeficit(c,kind,target-held,market,now,remaining,frequency,started);
                if(cost<0) return false;
                if(kind==DF_SUPPLY_FOOD) food+=cost;else medical+=cost;
                if(food>INT_MAX || medical>INT_MAX) return false;
            }
            // This forecast gates optional/planned spending. A large/corrupt
            // inventory must not extend an unbounded traversal across a frame.
            if(!WithinForecast(frequency,started)) return false;
        }
        budget.foodDeficitCost=static_cast<int>(food);budget.medicalDeficitCost=static_cast<int>(medical);
        budget.forecastKnown=true;return true;
    }
    Character* ChooseSupplyTrader(Character* member,DfSupplyKind kind,bool light,
        const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now)
    {
        if(member==NULL || traders.size()>128 || market.shops.size()>32 || failed.size()>128) return NULL;
        Character* best=NULL;Character* unknown=NULL;double bestCost=0;float unknownDistance=0;
        for(size_t i=0;i<traders.size();++i)
        {
            bool skip=false;for(size_t k=0;k<failed.size();++k)if(SameShop(traders[i].who,failed[k]))skip=true;
            if(skip) continue;
            Character* const t=traders[i].who.getCharacter();
            if(t==NULL || t->isDead() || t->isUnconcious() || t->isInCombatMode(true,true) ||
                member->isEnemy(t,false) || t->isEnemy(member,false)) continue;
            const float distance=member->getPosition().squaredDistance(t->getPosition());
            if(!_finite(distance) || distance<0) continue;
            const DfKnownShop* const shop=KnownShop(market,traders[i].who,now);
            if(shop==NULL)
            {if(unknown==NULL || distance<unknownDistance){unknown=t;unknownDistance=distance;}continue;}
            if(shop->offers.size()>128) continue;
            for(size_t k=0;k<shop->offers.size();++k)
            {
                const DfSupplyQuote& q=shop->offers[k];
                if(q.kind!=kind || q.quantity<=0 || q.price<=0 || !_finite(q.amount) || q.amount<=0 ||
                    (kind==DF_SUPPLY_FOOD && !CanUseFood(member,q))) continue;
                double cost=static_cast<double>(q.price)/q.amount;
                if(kind==DF_SUPPLY_BACKPACK)
                {const double value=DfBagScore(q.bag,light,q.price);if(value<=0)continue;cost=q.price/value;}
                cost+=std::sqrt(static_cast<double>(distance))*.05;
                if(best==NULL || cost<bestCost){best=t;bestCost=cost;}
            }
        }
        // Only actual visits populate the cache. Unknown shops are explored
        // physically when no suitable known stock exists; no remote stock peek.
        return best!=NULL ? best : unknown;
    }
}

void DfRememberShop(DfMarketCache& market,const hand& trader,const std::vector<DfSupplyQuote>& offers,unsigned long now)
{
    if(offers.size()>128) return;
    // Multiple shopkeepers can publish the same home-shop inventory. A new
    // physical observation supersedes ALL old aliases of that backing pool,
    // including a newly empty pool. Never reserve the same stock twice.
    for(size_t i=0;i<market.shops.size();++i)
    {
        std::vector<DfSupplyQuote>& old=market.shops[i].offers;
        for(size_t j=0;j<old.size();)
        {
            bool replaced=false;
            for(size_t k=0;k<offers.size();++k)
                if(offers[k].pool.Known() && old[j].pool==offers[k].pool){replaced=true;break;}
            if(replaced)old.erase(old.begin()+j);else ++j;
        }
    }
    size_t at=market.shops.size();
    for(size_t i=0;i<market.shops.size();++i)if(SameShop(market.shops[i].trader,trader))at=i;
    if(at==market.shops.size())
    {
        if(at<32) market.shops.push_back(DfKnownShop());
        else
        {at=0;for(size_t i=1;i<market.shops.size();++i)if(now-market.shops[i].quotedAt>now-market.shops[at].quotedAt)at=i;}
    }
    market.shops[at].trader=trader;market.shops[at].offers=offers;market.shops[at].quotedAt=now;
}
Character* DfChooseSupplyTrader(Character* member,DfSupplyKind kind,bool light,
    const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now)
{
    __try {return ChooseSupplyTrader(member,kind,light,traders,market,failed,now);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return NULL;}
}
bool DfForecastSupplies(Faction* faction,const std::vector<Character*>& people,const DfMarketCache& market,
    int committed,unsigned long now,DfBudgetSnapshot& budget)
{
    __try {return ForecastSupplies(faction,people,market,committed,now,budget);}
    __except(EXCEPTION_EXECUTE_HANDLER) {budget.forecastKnown=false;return false;}
}
