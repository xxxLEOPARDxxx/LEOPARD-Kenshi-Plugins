#pragma once
#include <vector>
#include <kenshi/util/hand.h>
#include "core/DfOffer.h"
#include "core/DfBudget.h"

class Character;
class Faction;
struct DfTrader;
struct DfKnownShop
{
    hand trader;
    std::vector<DfSupplyQuote> offers;
    unsigned long quotedAt;
    DfKnownShop():quotedAt(0) {}
};
struct DfMarketCache {std::vector<DfKnownShop> shops;};
void DfRememberShop(DfMarketCache& market,const hand& trader,const std::vector<DfSupplyQuote>& offers,unsigned long now);
Character* DfChooseSupplyTrader(Character* member,DfSupplyKind kind,bool light,
    const std::vector<DfTrader>& traders,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long now);
bool DfForecastSupplies(Faction* faction,const std::vector<Character*>& people,
    const DfMarketCache& market,int committed,unsigned long now,DfBudgetSnapshot& budget);
