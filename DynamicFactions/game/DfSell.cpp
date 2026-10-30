#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfSell.h"
#include "game/DfMine.h"
#include "core/DfTransfer.h"
#include <climits>
#include <float.h>

#include <Windows.h>

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/Town.h>

namespace
{
    // Флаг шаблона персонажа - как TemplateFlag в RecruitMarkers.
    bool TemplateFlag(Character* c, const std::string& key)
    {
        GameData* const d = c->data;
        if (d == NULL)
            return false;
        boost::unordered::unordered_map<std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, bool>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->bdata.find(key);
        return it != d->bdata.end() && it->second;
    }

    void Find(TownBase* town, std::vector<DfTrader>& out)
    {
        out.clear();
        if (ou == NULL || town == NULL)
            return;
        static const std::string kTrader("is trader");
        const Ogre::Vector3 centre = town->getPosition();
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL || c->isDead() || c->isUnconcious() || c->isInCombatMode(true, true) || c->isPlayerCharacter())
                continue;
            if (c->getCurrentTownLocation() != town || !TemplateFlag(c, kTrader))
                continue;
            ActivePlatoon* const ap = c->getPlatoon();
            Ownerships* const own = ap != NULL && ap->me != NULL ? ap->me->getOwnerships() : NULL;
            DfTrader t;
            t.who = c->getHandle();
            t.name = c->getName();
            t.money = own != NULL ? own->money : 0;
            t.distance = c->getPosition().distance(centre);
            out.push_back(t);
        }
    }
}

namespace
{
    int g_sellDepth = 0;
    struct SellGuard
    {
        SellGuard() { ++g_sellDepth; }
        ~SellGuard() { --g_sellDepth; }
    };

    bool Able(Character* c)
    {
        return c != NULL && !c->isDead() && !c->isUnconcious() && !c->isInCombatMode(true, true);
    }

    Ownerships* TraderPurse(Character* c)
    {
        ActivePlatoon* const ap = c != NULL ? c->getPlatoon() : NULL;
        return ap != NULL && ap->me != NULL ? ap->me->getOwnerships() : NULL;
    }

    int UnitPrice(Item* item, Character* trader)
    {
        if (item == NULL || trader == NULL || item->isStolen(true) || item->isEquipped ||
            item->getGameData() == NULL || item->getGameData()->type != ITEM) return 0;
        TownBase* const location = trader->getCurrentTownLocation();
        Town* const town = location != NULL ? location->isTown() : NULL;
        ActivePlatoon* const ap = trader->getPlatoon();
        if (town == NULL || ap == NULL || ap->me == NULL) return 0;
        // Read-only native verification: getValueSingle(false), merchantPriceMod,
        // getTraderPriceMultiplier. Their GUI globals are replaced by THIS town
        // and THIS merchant; do not set the player's global trading context.
        return DfOreUnitPrice(item->getAvgPrice(), town->getLocalTradePriceMult(item->getGameData()),
            item->chargesLeft, item->originalFullChargeAmount, ap->me->priceMultWhenITrade,
            town->getFactionTradeCultureMult(item->getGameData()));
    }

    bool ValidDeal(Character* hauler, Character* trader, Faction* f, bool requireNear = true)
    {
        static const std::string isTrader("is trader");
        if (DfTransferFaulted() || !Able(hauler) || !Able(trader) || hauler == trader || f == NULL ||
            hauler->isPlayerCharacter() || trader->isPlayerCharacter() || hauler->getFaction() != f ||
            trader->getFaction() == NULL || trader->getFaction() == f ||
            !TemplateFlag(trader, isTrader) || hauler->isEnemy(trader, false) || trader->isEnemy(hauler, false))
            return false;
        TownBase* const town = trader->getCurrentTownLocation();
        if (town == NULL) return false;
        if (!requireNear) return true;
        if (hauler->getCurrentTownLocation() != town) return false;
        const float distance = hauler->getPosition().distance(trader->getPosition());
        return _finite(distance) && distance <= kDfTradeRange;
    }

    bool SafeSetMoney(Ownerships* own, int money)
    {
        __try { own->setMoney(money); return true; }
        __except (EXCEPTION_EXECUTE_HANDLER) { ErrorLog("DynamicFactions: native money setter fault"); return false; }
    }

    class NativePayment : public DfPaymentPort
    {
        Ownerships* buyer_;
        Ownerships* seller_;
    public:
        NativePayment(Ownerships* buyer, Ownerships* seller) : buyer_(buyer), seller_(seller) {}
        virtual bool Read(int& buyer, int& seller)
        {
            __try { buyer = buyer_->money; seller = seller_->money; return true; }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ErrorLog("DynamicFactions: native wallet snapshot fault");
                return false;
            }
        }
        virtual void WriteBuyer(int target) { SafeSetMoney(buyer_, target); }
        virtual void WriteSeller(int target) { SafeSetMoney(seller_, target); }
    };

    class NativeTrade : public DfTradePort
    {
        NativePayment money_;
        Inventory* from_;
        Inventory* actual_;
        Item* source_;
        Inventory* to_;
        GameData* item_;
    public:
        NativeTrade(Ownerships* buyer, Ownerships* seller, Inventory* from, Inventory* actual,
                    Item* source, Inventory* to, GameData* item) : money_(buyer,seller), from_(from),
            actual_(actual), source_(source), to_(to), item_(item) {}
        virtual bool Read(int& buyer, int& seller) { return money_.Read(buyer,seller); }
        virtual void WriteBuyer(int target) { money_.WriteBuyer(target); }
        virtual void WriteSeller(int target) { money_.WriteSeller(target); }
        virtual bool ReadGoods(int& seller, int& buyer)
        {
            seller = DfCountInventoryItem(from_,item_);
            buyer = DfCountInventoryItem(to_,item_);
            return seller >= 0 && buyer >= 0;
        }
        virtual DfUnitResult Deliver()
        {
            if (DfMoveInventoryUnit(from_,actual_,source_,to_,item_)) return DF_UNIT_MOVED;
            return DfTransferFaulted() ? DF_UNIT_FAULT : DF_UNIT_STOP;
        }
        virtual bool ReturnGoods()
        {
            // Accepted add may have merged/deleted source_: find a REAL merchant
            // stack afresh. Never reuse a consumed pointer or manufacture cargo.
            Inventory* merchantInventory = NULL;
            Item* const actualItem = DfFirstInventoryItem(to_,item_,merchantInventory);
            return actualItem != NULL && DfMoveInventoryUnit(to_,merchantInventory,actualItem,from_,item_);
        }
    };

    bool Sell(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
              int& sold, int& paid)
    {
        if (item == NULL || maxUnits <= 0 || !ValidDeal(hauler, trader, f)) return false;
        Ownerships* const buyer = TraderPurse(trader);
        Ownerships* const seller = f->factionOwnerships;
        Inventory* const from = hauler->getInventory();
        Inventory* const to = trader->getInventory();
        if (buyer == NULL || seller == NULL || buyer == seller || from == NULL || to == NULL || from == to)
            return false;
        int limit = maxUnits < kDfTransferMaxUnits ? maxUnits : kDfTransferMaxUnits;
        Inventory* actual = NULL;
        Item* source = NULL;
        while (sold < limit)
        {
            if (!ValidDeal(hauler, trader, f) || buyer != TraderPurse(trader) || seller != f->factionOwnerships)
                return false;
            if (source == NULL) source = DfFirstInventoryItem(from, item, actual, true);
            if (source == NULL) break;
            const int price = UnitPrice(source, trader);
            if (!DfCanPay(buyer->money, seller->money, price) || paid > INT_MAX - price) break;
            const bool remains = source->quantity > 1;
            NativeTrade native(buyer,seller,from,actual,source,to,item);
            const DfTradeResult outcome = DfTradeOne(native,price);
            if (outcome == DF_TRADE_STOP || outcome == DF_TRADE_RESTORED)
                return !DfTransferFaulted();
            if (outcome != DF_TRADE_COMPLETE)
            {
                DfQuarantineTransfer("merchant deal requires reconciliation; no replay");
                return false;
            }
            ++sold;
            paid += price;
            if (!remains) source = NULL;
        }
        return !DfTransferFaulted();
    }

    bool SafeSell(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
                  int& sold, int& paid)
    {
        __try { return Sell(hauler, trader, f, item, maxUnits, sold, paid); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("selling ore raised a native fault; no replay");
            return false;
        }
    }
}

bool DfSellTransferBusy() { return g_sellDepth != 0; }
bool DfTraderCanBuy(Character* hauler, Character* trader, Faction* f, GameData* item)
{
    __try
    {
        if (item == NULL || !ValidDeal(hauler, trader, f, false)) return false;
        Ownerships* const buyer = TraderPurse(trader);
        Ownerships* const seller = f->factionOwnerships;
        Inventory* actual = NULL;
        Item* const source = DfFirstInventoryItem(hauler->getInventory(), item, actual, true);
        return source != NULL && buyer != NULL && seller != NULL && buyer != seller &&
            DfCanPay(buyer->money, seller->money, UnitPrice(source, trader)) &&
            DfInventoryHasRoom(trader->getInventory(), item);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool DfSellOre(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits, int& sold,
               int& paid)
{
    SellGuard guard;
    sold = paid = 0;
    return SafeSell(hauler, trader, f, item, maxUnits, sold, paid);
}

void DfFindTraders(TownBase* town, std::vector<DfTrader>& out)
{
    __try
    {
        Find(town, out);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: looking for traders failed once");
    }
}
