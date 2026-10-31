#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfSell.h"
#include "game/DfMine.h"
#include "core/DfTransfer.h"
#include "core/DfBudget.h"
#include <climits>
#include <algorithm>
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
#include <kenshi/ShopTrader.h>
#include <kenshi/ShopTraderInventory.h>
#include <kenshi/RaceData.h>
#include <kenshi/gui/InventoryGUI.h>
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

    bool SameHandle(const hand& a, const hand& b)
    {
        return a.type==b.type && a.container==b.container && a.containerSerial==b.containerSerial &&
            a.index==b.index && a.serial==b.serial;
    }
    bool PlayerTradingWith(Character* trader)
    {
        // A live shop window owns a copy snapshot and may have a pending mouse
        // transaction. Do not rebuild it or compete with that pending trade.
        Character* const active=InventoryGUI::getNPCTrader();
        if (trader==NULL || active==NULL) return false;
        if (active==trader) return true;
        // Two shopkeepers can share one squad/home and therefore the same real
        // stock. Defer that shared shop as well, not just the visible NPC pointer.
        Ownerships* const current=TraderPurse(active);
        Ownerships* const candidate=TraderPurse(trader);
        return current!=NULL && candidate!=NULL && (current==candidate ||
            (!current->_homeBuilding.isNull() && SameHandle(current->_homeBuilding,candidate->_homeBuilding)));
    }

    ShopTrader* CreateShop(Character* trader) { return new ShopTrader(trader); }
    ShopTrader* SafeCreateShop(Character* trader)
    {
        __try { return CreateShop(trader); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("creating native merchant stock view failed");
            return NULL;
        }
    }
    bool SafeDestroyShop(ShopTrader* shop)
    {
        __try { delete shop; return true; }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("destroying native merchant stock view failed");
            return false;
        }
    }
    struct ShopView
    {
        ShopTrader* shop;
        explicit ShopView(Character* trader) : shop(SafeCreateShop(trader)) {}
        ~ShopView() { Close(); }
        bool Close()
        {
            ShopTrader* const p = shop;
            shop = NULL; // a faulted destruction is never retried
            return p == NULL || SafeDestroyShop(p);
        }
    private:
        ShopView(const ShopView&);
        ShopView& operator=(const ShopView&);
    };
    typedef std::vector<InventorySection*> StockSections;

    bool ReadStockSections(ShopTrader* shop, StockSections& sections)
    {
        sections.clear();
        // Native ctor always constructs ShopTraderInventory. Its public map
        // holds the REAL persistent sections, while getAllItems() holds copies.
        // Native ctor publishes the first section of each selected home-shop
        // container/backpack. Key=that parent inventory's full hand; value=the
        // exact published section. Other sections in its parent are NOT stock.
        ShopTraderInventory* const stock = shop != NULL
            ? static_cast<ShopTraderInventory*>(shop->getInventory()) : NULL;
        if (stock == NULL || stock->inventories.size() > 128) return false;
        typedef ogre_unordered_map<hand,InventorySection*>::type Backings;
        for (Backings::const_iterator it=stock->inventories.begin();it!=stock->inventories.end();++it)
        {
            InventorySection* const section = it->second;
            if (section == NULL || section->getInventory() == NULL) return false;
            // Fail closed if a malformed view points back into its copy inventory.
            if (section->getInventory() == stock) return false;
            if (!SameHandle(it->first,section->getInventory()->getHandle())) return false;
            bool seen = false;
            for (size_t k=0;k<sections.size();++k) if (sections[k] == section) seen = true;
            if (!seen) sections.push_back(section);
        }
        return true;
    }
    bool SafeReadStockSections(ShopTrader* shop, StockSections& sections)
    {
        __try { return ReadStockSections(shop,sections); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("reading native merchant backing sections failed");
            return false;
        }
    }

    InventorySection* StockWithRoom(const StockSections& sections, Item* item)
    {
        for (size_t k=0;k<sections.size();++k)
        {
            InventorySection* const section = sections[k];
            if (section->enabled && section->isLimitedSlotCompatible(item) &&
                section->hasRoomForItem(item->getGameData(),1)) return section;
        }
        return NULL;
    }
    Item* FirstInStockSection(InventorySection* section, GameData* item)
    {
        const Ogre::vector<InventorySection::SectionItem>::type& all = section->getItems();
        if (all.size() > 4096) return NULL;
        for (size_t k=0;k<all.size();++k)
        {
            Item* const p = all[k].item;
            if (p != NULL && !p->isEquipped && p->quantity > 0 && p->getGameData() == item) return p;
        }
        return NULL;
    }
    int CountStockSection(InventorySection* section, GameData* item)
    {
        if (section==NULL || item==NULL) return -1;
        const Ogre::vector<InventorySection::SectionItem>::type& all=section->getItems();
        if (all.size()>4096) return -1;
        int count=0;
        for (size_t k=0;k<all.size();++k)
        {
            Item* const p=all[k].item;
            if (p==NULL || p->quantity<=0) return -1;
            if (p->getGameData()!=item) continue;
            if (count>INT_MAX-p->quantity) return -1;
            count+=p->quantity;
        }
        return count;
    }
    int SafeCountStockSection(InventorySection* section, GameData* item)
    {
        __try { return CountStockSection(section,item); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        { ErrorLog("DynamicFactions: real merchant section snapshot fault");return -1; }
    }

    class NativeTrade : public DfTradePort
    {
        NativePayment money_;
        Inventory* from_;
        Inventory* actual_;
        Item* source_;
        Inventory* to_;
        InventorySection* stockSection_;
        GameData* item_;
    public:
        NativeTrade(Ownerships* buyer, Ownerships* seller, Inventory* from, Inventory* actual,
                    Item* source, InventorySection* stock, GameData* item) : money_(buyer,seller), from_(from),
            actual_(actual), source_(source), to_(stock->getInventory()), stockSection_(stock), item_(item) {}
        virtual bool Read(int& buyer, int& seller) { return money_.Read(buyer,seller); }
        virtual void WriteBuyer(int target) { money_.WriteBuyer(target); }
        virtual void WriteSeller(int target) { money_.WriteSeller(target); }
        virtual bool ReadGoods(int& seller, int& buyer)
        {
            seller = DfCountInventoryItem(from_,item_);
            // The UI publishes THIS backing section, not every section/child in
            // its parent inventory. A redirected add elsewhere cannot authorize pay.
            buyer = stockSection_->getInventory()==to_ ? SafeCountStockSection(stockSection_,item_) : -1;
            return seller >= 0 && buyer >= 0;
        }
        virtual DfUnitResult Deliver()
        {
            if (DfMoveInventoryUnitToSection(from_,actual_,source_,stockSection_,item_)) return DF_UNIT_MOVED;
            return DfTransferFaulted() ? DF_UNIT_FAULT : DF_UNIT_STOP;
        }
        virtual bool ReturnGoods()
        {
            // Accepted add may have merged/deleted source_: find a REAL merchant
            // stack afresh. Never reuse a consumed pointer or manufacture cargo.
            Item* const actualItem = FirstInStockSection(stockSection_,item_);
            return actualItem != NULL && DfMoveInventoryUnit(to_,to_,actualItem,from_,item_);
        }
    };

    bool SellIntoStock(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
                       const StockSections& stock, int& sold, int& paid, DfSellStatus& status)
    {
        Ownerships* const buyer = TraderPurse(trader);
        Ownerships* const seller = f->factionOwnerships;
        Inventory* const from = hauler->getInventory();
        if (buyer == NULL || seller == NULL || buyer == seller || from == NULL) return false;
        const int limit = maxUnits < kDfTransferMaxUnits ? maxUnits : kDfTransferMaxUnits;
        Inventory* actual = NULL;
        Item* source = NULL;
        while (sold < limit)
        {
            if (!ValidDeal(hauler,trader,f) || buyer != TraderPurse(trader) || seller != f->factionOwnerships)
                return false;
            if (PlayerTradingWith(trader)) { status=DF_SELL_PLAYER_TRADING;return true; }
            if (source == NULL) source=DfFirstInventoryItem(from,item,actual,true);
            if (source == NULL) break;
            const int price=UnitPrice(source,trader);
            if (!DfCanPay(buyer->money,seller->money,price) || paid > INT_MAX-price) break;
            InventorySection* const target=StockWithRoom(stock,source);
            if (target == NULL) { status=DF_SELL_NO_STOCK_ROOM;return true; }
            if (target->getInventory() == from)
            { DfQuarantineTransfer("merchant backing inventory aliases the seller");return false; }
            const bool remains=source->quantity > 1;
            NativeTrade native(buyer,seller,from,actual,source,target,item);
            const DfTradeResult outcome=DfTradeOne(native,price);
            if (outcome == DF_TRADE_STOP || outcome == DF_TRADE_RESTORED) return !DfTransferFaulted();
            if (outcome != DF_TRADE_COMPLETE)
            {
                DfQuarantineTransfer("merchant deal requires reconciliation; no replay");
                return false;
            }
            ++sold;paid+=price;
            if (!remains) source=NULL;
        }
        return !DfTransferFaulted();
    }
    bool SafeSellIntoStock(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
                           const StockSections& stock, int& sold, int& paid, DfSellStatus& status)
    {
        __try { return SellIntoStock(hauler,trader,f,item,maxUnits,stock,sold,paid,status); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("selling into real merchant stock raised a native fault");
            return false;
        }
    }
    bool Sell(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
              int& sold, int& paid, DfSellStatus& status)
    {
        if (item == NULL || maxUnits <= 0 || !ValidDeal(hauler,trader,f)) return false;
        if (PlayerTradingWith(trader)) { status=DF_SELL_PLAYER_TRADING;return true; }
        // Exactly one disposable native discovery view for this ARRIVED batch.
        // No view construction/copying per unit or distant candidate scan.
        ShopView view(trader);
        if (view.shop == NULL) return false;
        StockSections stock;
        bool ok=SafeReadStockSections(view.shop,stock);
        if (!ok && !DfTransferFaulted()) { status=DF_SELL_NO_STOCK_ROOM;ok=true;stock.clear(); }
        if (ok && stock.empty()) { status=DF_SELL_NO_STOCK_ROOM; }
        else if (ok) ok=SafeSellIntoStock(hauler,trader,f,item,maxUnits,stock,sold,paid,status);
        // All risky native operations are in narrow SEH wrappers. Return normally
        // through RAII; explicitly finish cleanup before reporting success.
        if (!view.Close()) ok=false;
        return ok && !DfTransferFaulted();
    }

    bool SafeSell(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits,
                  int& sold, int& paid, DfSellStatus& status)
    {
        __try { return Sell(hauler, trader, f, item, maxUnits, sold, paid, status); }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DfQuarantineTransfer("selling ore raised a native fault; no replay");
            return false;
        }
    }

    class NativeFoodPurchase : public DfTradePort
    {
        NativePayment money_;
        InventorySection* stock_;
        Inventory* from_;
        Inventory* to_;
        Item* source_;
        GameData* data_;
        hand buyerTitle_;
    public:
        NativeFoodPurchase(Character* member,Ownerships* buyer,Ownerships* seller,
                           InventorySection* stock,Item* source)
            : money_(buyer,seller),stock_(stock),from_(stock->getInventory()),to_(member->getInventory()),
              source_(source),data_(source->getGameData()),buyerTitle_(member->getHandle()) {}
        virtual bool Read(int& buyer,int& seller) {return money_.Read(buyer,seller);}
        virtual void WriteBuyer(int target) {money_.WriteBuyer(target);}
        virtual void WriteSeller(int target) {money_.WriteSeller(target);}
        virtual bool ReadGoods(int& seller,int& buyer)
        {
            seller=stock_->getInventory()==from_ ? SafeCountStockSection(stock_,data_) : -1;
            buyer=DfCountInventoryItem(to_,data_);
            return seller>=0 && buyer>=0;
        }
        virtual DfUnitResult Deliver()
        {
            if(DfMovePurchasedUnit(from_,from_,source_,to_,data_,buyerTitle_)) return DF_UNIT_MOVED;
            return DfTransferFaulted() ? DF_UNIT_FAULT : DF_UNIT_STOP;
        }
        virtual bool ReturnGoods()
        {
            // Purchases settle money first. Never pull a fresh unit back out
            // of a native stack containing a partly eaten meal.
            return false;
        }
    };

    struct FoodOffer
    {
        Item* item;GameData* data;InventorySection* section;int price;double value;
    };
    bool CheapNutrition(const FoodOffer& a,const FoodOffer& b) {return a.value<b.value;}

    bool BuyFoodFromStock(Character* member,Character* trader,Faction* f,int committed,
                          const StockSections& stock,std::vector<FoodOffer>& offers,std::vector<GameData*>& ranked,
                          int& paid,std::string& itemName,DfBuyStatus& status)
    {
        Ownerships* const buyer=f->factionOwnerships;
        Ownerships* const seller=TraderPurse(trader);
        RaceData* const race=member->getRace();
        if(buyer==NULL || seller==NULL || buyer==seller || race==NULL || committed<0) return false;
        DfBudgetSnapshot budget;budget.money=buyer->money;budget.committed=committed;
        // Food is essential. Optional gear remains blocked until all real
        // deficits/quotes are known; this is not a second wallet/reservation.
        offers.clear();ranked.clear();
        bool edible=false,affordable=false;
        const bool animal=member->isAnimal()!=NULL;
        unsigned int examined=0;
        for(size_t at=0;at<stock.size();++at)
        {
            InventorySection* const s=stock[at];
            const Ogre::vector<InventorySection::SectionItem>::type& items=s->getItems();
            if(items.size()>4096) return false;
            for(size_t i=0;i<items.size();++i)
            {
                if(++examined>4096) return false;
                Item* const p=items[i].item;
                // isTradeItem is the commodity pricing flag. Real shop food
                // normally has it clear; membership in native stock is the
                // sale-source check, not this flag.
                if(p==NULL || p->quantity<=0 || p->isEquipped || p->deathItem ||
                   (p->itemFunction!=ITEM_FOOD && p->itemFunction!=ITEM_FOOD_RESTRICTED) ||
                   !_finite(p->chargesLeft) || p->chargesLeft<=0 || p->chargesLeft!=p->originalFullChargeAmount ||
                   !_finite(p->quality) || p->isStolen(true) || !race->canEat(p,animal)) continue;
                edible=true;
                const int cost=UnitPrice(p,trader);
                // For clean fresh shop food the native retail branch has no
                // used/stolen discount; use this explicit merchant/town formula.
                if(!DfPlanSpend(budget,cost,true).Allowed() || !DfCanPay(buyer->money,seller->money,cost)) continue;
                affordable=true;
                const double perNutrition=static_cast<double>(cost)/p->chargesLeft;
                FoodOffer offer;offer.item=p;offer.data=p->getGameData();offer.section=s;offer.price=cost;offer.value=perNutrition;
                size_t existing=offers.size();
                for(size_t k=0;k<offers.size();++k)
                    if(offers[k].data==offer.data) {existing=k;break;}
                if(existing<offers.size())
                {if(perNutrition<offers[existing].value)offers[existing]=offer;}
                else if(offers.size()<64) offers.push_back(offer);
                else
                {
                    size_t worst=0;
                    for(size_t k=1;k<offers.size();++k)if(offers[k].value>offers[worst].value)worst=k;
                    if(perNutrition<offers[worst].value)offers[worst]=offer;
                }
            }
        }
        std::stable_sort(offers.begin(),offers.end(),CheapNutrition);
        for(size_t i=0;i<offers.size();++i) ranked.push_back(offers[i].data);
        const int fit=DfFirstInventoryFit(member->getInventory(),ranked);
        if(fit<0)
        {status=!edible ? DF_BUY_NO_FOOD : !affordable ? DF_BUY_NO_MONEY : DF_BUY_NO_ROOM;return true;}
        Item* const best=offers[fit].item;
        InventorySection* const section=offers[fit].section;
        const int price=offers[fit].price;
        if(!ValidDeal(member,trader,f) || PlayerTradingWith(trader))
        {status=DF_BUY_PLAYER_TRADING;return true;}
        budget.money=buyer->money;
        if(buyer!=f->factionOwnerships || seller!=TraderPurse(trader) ||
           !DfPlanSpend(budget,price,true).Allowed()) return false;
        itemName=best->getName();
        NativeFoodPurchase purchase(member,buyer,seller,section,best);
        const DfTradeResult result=DfPurchaseOne(purchase,price);
        if(result==DF_TRADE_COMPLETE) {paid=price;status=DF_BUY_OK;return true;}
        if(result==DF_TRADE_STOP || result==DF_TRADE_RESTORED)
        {status=DF_BUY_NO_ROOM;return true;}
        DfQuarantineTransfer("food purchase requires reconciliation; no replay");
        status=DF_BUY_FAULT;return false;
    }
    bool SafeBuyFoodFromStock(Character* member,Character* trader,Faction* f,int committed,
                              const StockSections& stock,std::vector<FoodOffer>& offers,std::vector<GameData*>& ranked,
                              int& paid,std::string& name,DfBuyStatus& status)
    {
        __try {return BuyFoodFromStock(member,trader,f,committed,stock,offers,ranked,paid,name,status);}
        __except(EXCEPTION_EXECUTE_HANDLER)
        {DfQuarantineTransfer("native food purchase fault; no replay");status=DF_BUY_FAULT;return false;}
    }
    bool BuyFood(Character* member,Character* trader,Faction* f,int committed,
                 int& paid,std::string& name,DfBuyStatus& status)
    {
        if(!ValidDeal(member,trader,f)) return false;
        if(PlayerTradingWith(trader)) {status=DF_BUY_PLAYER_TRADING;return true;}
        ShopView view(trader);
        if(view.shop==NULL) return false;
        StockSections stock;
        // Owned buffers live outside the native SEH boundary, so an AV inside
        // quote/transfer still returns through normal cleanup of these vectors.
        std::vector<FoodOffer> offers;
        std::vector<GameData*> ranked;
        bool ok=SafeReadStockSections(view.shop,stock);
        if(ok) ok=SafeBuyFoodFromStock(member,trader,f,committed,stock,offers,ranked,paid,name,status);
        if(!view.Close()) ok=false;
        return ok && !DfTransferFaulted();
    }
    bool SafeBuyFood(Character* member,Character* trader,Faction* f,int committed,
                     int& paid,std::string& name,DfBuyStatus& status)
    {
        __try {return BuyFood(member,trader,f,committed,paid,name,status);}
        __except(EXCEPTION_EXECUTE_HANDLER)
        {DfQuarantineTransfer("food purchase setup fault; no replay");status=DF_BUY_FAULT;return false;}
    }
}

bool DfSellTransferBusy() { return g_sellDepth != 0; }
bool DfBuyFood(Character* member,Character* trader,Faction* f,int committed,
               int& paid,std::string& name,DfBuyStatus* status)
{
    paid=0;name.clear();DfBuyStatus result=DF_BUY_INVALID;
    if(DfTransferBusy()) {if(status!=NULL)*status=result;return false;}
    SellGuard guard;
    const bool ok=SafeBuyFood(member,trader,f,committed,paid,name,result);
    if(!ok && DfTransferFaulted()) result=DF_BUY_FAULT;
    if(status!=NULL) *status=result;
    return ok;
}
bool DfTraderCanBuy(Character* hauler, Character* trader, Faction* f, GameData* item)
{
    __try
    {
        if (item == NULL || !ValidDeal(hauler, trader, f, false) || PlayerTradingWith(trader)) return false;
        Ownerships* const buyer = TraderPurse(trader);
        Ownerships* const seller = f->factionOwnerships;
        Inventory* actual = NULL;
        Item* const source = DfFirstInventoryItem(hauler->getInventory(), item, actual, true);
        return source != NULL && buyer != NULL && seller != NULL && buyer != seller &&
            DfCanPay(buyer->money, seller->money, UnitPrice(source, trader));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool DfSellOre(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits, int& sold,
               int& paid, DfSellStatus* status)
{
    SellGuard guard;
    sold = paid = 0;
    DfSellStatus result=DF_SELL_OK;
    const bool ok=SafeSell(hauler,trader,f,item,maxUnits,sold,paid,result);
    if (!ok) result=DfTransferFaulted()?DF_SELL_FAULT:DF_SELL_INVALID;
    if (status != NULL) *status=result;
    return ok;
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
