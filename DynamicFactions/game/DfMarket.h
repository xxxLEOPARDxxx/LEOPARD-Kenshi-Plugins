#pragma once
#include <vector>
#include <kenshi/util/hand.h>
#include "core/DfOffer.h"
#include "core/DfBudget.h"

class Character;
class Faction;
struct DfTrader;
struct DfKnownPrices
{
    // Price-only negative evidence lives longer than finite-stock forecasts.
    // Slots 0..3 are supply kinds; slot 4 is role-eligible light backpacks.
    int price[5];unsigned long quotedAt[5];DfStockPool pool[5];
    DfKnownPrices(){for(int i=0;i<5;++i){price[i]=0;quotedAt[i]=0;}}
};
struct DfKnownShop
{
    hand trader;
    std::vector<DfSupplyQuote> offers;
    DfKnownPrices prices;
    unsigned long quotedAt;
    DfKnownShop():quotedAt(0) {}
};
struct DfMarketCache {std::vector<DfKnownShop> shops;unsigned long revision;DfMarketCache():revision(0){}};
void DfRememberShop(DfMarketCache& market,const hand& trader,const std::vector<DfSupplyQuote>& offers,unsigned long now);
Character* DfChooseSupplyTrader(Character* member,DfSupplyKind kind,bool light,
    const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now,
    const DfBudgetSnapshot* budget=NULL,bool allowUnknown=true,size_t* discoveryCursor=NULL,bool* discovering=NULL);
// Cached observations only: no native inventory/stock queries or world traversal.
bool DfKnownSupplyBlocked(const DfMarketCache& market,const hand& trader,DfSupplyKind kind,bool light,
    const DfBudgetSnapshot& budget,unsigned long now);
bool DfForecastSupplies(Faction* faction,const std::vector<Character*>& people,
    const DfMarketCache& market,int committed,unsigned long now,DfBudgetSnapshot& budget);
