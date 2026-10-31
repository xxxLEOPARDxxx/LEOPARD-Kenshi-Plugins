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
    const DfKnownShop* ObservedShop(const DfMarketCache& market,const hand& trader)
    {
        for(size_t i=0;i<market.shops.size();++i)
            if(SameShop(market.shops[i].trader,trader)) return &market.shops[i];
        return NULL;
    }
    const DfKnownShop* KnownShop(const DfMarketCache& market,const hand& trader,unsigned long now)
    {const DfKnownShop* shop=ObservedShop(market,trader);return shop!=NULL && now-shop->quotedAt<=120000 ? shop : NULL;}
    bool EligibleQuote(const DfSupplyQuote& q,DfSupplyKind kind,bool light)
    {
        return q.kind==kind && q.quantity>0 && q.price>0 && _finite(q.amount) && q.amount>0 &&
            (kind!=DF_SUPPLY_BACKPACK || DfBagScore(q.bag,light,q.price)>0);
    }
    bool ObservedPriceBlocks(const DfMarketCache& market,const hand& trader,DfSupplyKind kind,bool light,
        const DfBudgetSnapshot& budget,unsigned long now)
    {
        if(kind==DF_SUPPLY_FOOD || kind<DF_SUPPLY_FIRSTAID || kind>DF_SUPPLY_BACKPACK) return false;
        const DfKnownShop* const shop=ObservedShop(market,trader);
        if(shop==NULL) return false;
        const int slot=kind==DF_SUPPLY_BACKPACK && light ? 4 : static_cast<int>(kind);
        const int price=shop->prices.price[slot];
        if(price<=0 || now-shop->prices.quotedAt[slot]>600000) return false;
        // This evidence can reject a futile trip, never assert stock or permit
        // a purchase. Exact native price/quantity is re-read only on arrival.
        return budget.money<0 || budget.committed<0 || price>budget.money ||
            budget.committed>budget.money-price ||
            (budget.forecastKnown && !DfPlanSupplySpend(budget,price,kind).Allowed());
    }
    bool KnownSupplyBlocked(const DfMarketCache& market,const hand& trader,DfSupplyKind kind,bool light,
        const DfBudgetSnapshot& budget,unsigned long now)
    {
        if(ObservedPriceBlocks(market,trader,kind,light,budget,now)) return true;
        const DfKnownShop* const shop=KnownShop(market,trader,now);
        if(shop==NULL) return false; // An already-authorized physical discovery.
        if(shop->offers.size()>128) return true;
        for(size_t k=0;k<shop->offers.size();++k)
        {
            const DfSupplyQuote& q=shop->offers[k];
            if(!EligibleQuote(q,kind,light) || budget.money<0 || budget.committed<0 ||
                q.price>budget.money || budget.committed>budget.money-q.price) continue;
            if(!budget.forecastKnown || DfPlanSupplySpend(budget,q.price,kind).Allowed()) return false;
        }
        return true;
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
        const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now,
        const DfBudgetSnapshot* budget=NULL,bool allowUnknown=true,size_t* discoveryCursor=NULL,bool* discovering=NULL)
    {
        if(discovering!=NULL)*discovering=false;
        if(member==NULL || traders.size()>128 || market.shops.size()>32 || failed.size()>128) return NULL;
        Character* best=NULL;Character* unknown=NULL;double bestCost=0;float unknownDistance=0;size_t unknownAt=0;
        const size_t start=discoveryCursor!=NULL && !traders.empty() ? *discoveryCursor%traders.size() : 0;
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
            {
                if(budget!=NULL && ObservedPriceBlocks(market,traders[i].who,kind,light,*budget,now)) continue;
                const size_t rank=(i+traders.size()-start)%traders.size();
                if(allowUnknown && (unknown==NULL || (discoveryCursor!=NULL ? rank<unknownAt : distance<unknownDistance)))
                {unknown=t;unknownDistance=distance;unknownAt=discoveryCursor!=NULL ? rank : i;}
                continue;
            }
            if(shop->offers.size()>128) continue;
            for(size_t k=0;k<shop->offers.size();++k)
            {
                const DfSupplyQuote& q=shop->offers[k];
                if(q.kind!=kind || q.quantity<=0 || q.price<=0 || !_finite(q.amount) || q.amount<=0 ||
                    (kind==DF_SUPPLY_FOOD && !CanUseFood(member,q))) continue;
                double cost=static_cast<double>(q.price)/q.amount;
                if(kind==DF_SUPPLY_BACKPACK)
                {const double value=DfBagScore(q.bag,light,q.price);if(value<=0)continue;cost=q.price/value;}
                if(budget!=NULL && !DfPlanSupplySpend(*budget,q.price,kind).Allowed())
                {
                    // Missing forecast may be resolved by one real visit. A
                    // known price above actual cash/commitments never justifies
                    // sending a worker; no transaction or stock peek is done.
                    const size_t rank=(i+traders.size()-start)%traders.size();
                    if(allowUnknown && !budget->forecastKnown && budget->money>=q.price && budget->committed>=0 &&
                        budget->committed<=budget->money-q.price &&
                        (unknown==NULL || (discoveryCursor!=NULL ? rank<unknownAt : distance<unknownDistance)))
                    {unknown=t;unknownDistance=distance;unknownAt=discoveryCursor!=NULL ? rank : i;}
                    continue;
                }
                cost+=std::sqrt(static_cast<double>(distance))*.05;
                if(best==NULL || cost<bestCost){best=t;bestCost=cost;}
            }
        }
        // Only actual visits populate the cache. Unknown shops are explored
        // physically when no suitable known stock exists; no remote stock peek.
        if(best!=NULL) return best;
        if(unknown!=NULL)
        {
            if(discovering!=NULL)*discovering=true;
            if(discoveryCursor!=NULL)*discoveryCursor=(start+unknownAt+1)%traders.size();
        }
        return unknown;
    }
}

void DfRememberShop(DfMarketCache& market,const hand& trader,const std::vector<DfSupplyQuote>& offers,unsigned long now)
{
    if(offers.size()>128) return;
    ++market.revision;
    DfKnownPrices prices;
    const DfKnownShop* previous=ObservedShop(market,trader);
    if(previous!=NULL)prices=previous->prices;
    // Multiple shopkeepers can publish the same home-shop inventory. A new
    // physical observation supersedes ALL old aliases of that backing pool,
    // including a newly empty pool. Never reserve the same stock twice.
    for(size_t i=0;i<market.shops.size();++i)
    {
        std::vector<DfSupplyQuote>& old=market.shops[i].offers;
        bool allShared=!old.empty();
        for(size_t j=0;j<old.size();)
        {
            bool replaced=false;
            for(size_t k=0;k<offers.size();++k)
                if(offers[k].pool.Known() && old[j].pool==offers[k].pool){replaced=true;break;}
            if(replaced)
            {
                // Keep one zero-quantity identity marker per observed pool.
                // It cannot fund a forecast, but later empty aliases can still
                // inherit actual old price evidence instead of rediscovering it.
                bool duplicate=false;
                for(size_t k=0;k<j;++k)if(old[k].pool==old[j].pool){duplicate=true;break;}
                if(duplicate)old.erase(old.begin()+j);
                else {DfSupplyQuote marker;marker.pool=old[j].pool;old[j]=marker;++j;}
            }
            else {allShared=false;++j;}
        }
        if(allShared)
            for(int k=0;k<5;++k)
            {
                const DfKnownPrices& oldPrices=market.shops[i].prices;
                bool matchingPool=false;
                for(size_t j=0;j<offers.size();++j)
                    if(oldPrices.pool[k].Known() && oldPrices.pool[k]==offers[j].pool){matchingPool=true;break;}
                if(matchingPool && oldPrices.price[k]>0 &&
                    (prices.price[k]<=0 || now-oldPrices.quotedAt[k]<now-prices.quotedAt[k]))
                {prices.price[k]=oldPrices.price[k];prices.quotedAt[k]=oldPrices.quotedAt[k];prices.pool[k]=oldPrices.pool[k];}
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
    int observed[5]={0,0,0,0,0};DfStockPool observedPools[5];
    for(size_t i=0;i<offers.size();++i)
        for(int slot=DF_SUPPLY_FIRSTAID;slot<5;++slot)
        {
            const DfSupplyKind kind=slot==4 ? DF_SUPPLY_BACKPACK : static_cast<DfSupplyKind>(slot);
            if(EligibleQuote(offers[i],kind,slot==4) && (observed[slot]==0 || offers[i].price<observed[slot]))
                {observed[slot]=offers[i].price;observedPools[slot]=offers[i].pool;}
        }
    for(int k=0;k<5;++k)if(observed[k]>0){prices.price[k]=observed[k];prices.quotedAt[k]=now;prices.pool[k]=observedPools[k];}
    market.shops[at].trader=trader;market.shops[at].offers=offers;market.shops[at].prices=prices;market.shops[at].quotedAt=now;
}
Character* DfChooseSupplyTrader(Character* member,DfSupplyKind kind,bool light,
    const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now,
    const DfBudgetSnapshot* budget,bool allowUnknown,size_t* discoveryCursor,bool* discovering)
{
    __try {return ChooseSupplyTrader(member,kind,light,traders,market,failed,now,budget,allowUnknown,discoveryCursor,discovering);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return NULL;}
}
bool DfKnownSupplyBlocked(const DfMarketCache& market,const hand& trader,DfSupplyKind kind,bool light,
    const DfBudgetSnapshot& budget,unsigned long now)
{
    __try {return KnownSupplyBlocked(market,trader,kind,light,budget,now);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return true;}
}
bool DfForecastSupplies(Faction* faction,const std::vector<Character*>& people,const DfMarketCache& market,
    int committed,unsigned long now,DfBudgetSnapshot& budget)
{
    __try {return ForecastSupplies(faction,people,market,committed,now,budget);}
    __except(EXCEPTION_EXECUTE_HANDLER) {budget.forecastKnown=false;return false;}
}
