#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfMine.h"

#include <Windows.h>

#include <cstdio>
#include <map>
#include <set>
#include <climits>
#include <float.h>
#include "core/DfTransfer.h"
#include "core/DfSupply.h"
#include <kenshi/Faction.h>

#include <Debug.h>

#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/RaceData.h>
#include <kenshi/Town.h>
#include <kenshi/ZoneManager.h>
#include <kenshi/util/lektor.h>

namespace
{
    const int kMaxObjects = 4096;

    GameData* FirstRef(GameData* d, const char* list)
    {
        if (d == NULL || ou == NULL)
            return NULL;
        const Ogre::vector<GameDataReference>::type* const refs = d->getReferenceListIfExists(list);
        if (refs == NULL)
            return NULL;
        for (size_t i = 0; i < refs->size(); ++i)
            if (GameData* const p = (*refs)[i].getPtr(&ou->gamedata))
                return p;
        return NULL;
    }

    bool BoolField(GameData* d, const char* key)
    {
        if (d == NULL)
            return false;
        boost::unordered::unordered_map<std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, bool>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->bdata.find(key);
        return it != d->bdata.end() && it->second;
    }

    int IntField(GameData* d, const char* key)
    {
        if (d == NULL)
            return 0;
        boost::unordered::unordered_map<std::string, int, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, int>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->idata.find(key);
        return it != d->idata.end() ? it->second : 0;
    }

    bool IsBuildingType(itemType t)
    {
        return t == BUILDING || t == FOLIAGE_BUILDING;
    }

    void Find(const Ogre::Vector3& centre, float radius, Character* probe, std::vector<DfNode>& out)
    {
        out.clear();
        if (ou == NULL || ou->zoneMgr == NULL)
            return;
        static lektor<RootObject*> objs;
        objs.clear();
        ou->zoneMgr->buildingGrid.getObjects(centre, radius, objs, kMaxObjects);
        for (uint32_t i = 0; i < objs.size(); ++i)
        {
            RootObject* const o = objs.stuff[i];
            if (o == NULL || !IsBuildingType(o->getDataType()))
                continue;
            Building* const b = static_cast<Building*>(o);
            const BuildingFunction bf = b->getSpecialFunction();
            if (bf != BF_MINE && bf != BF_MINE_NATURAL)
                continue;
            // Естественная жила - «is foliage» в данных постройки (железо, медь,
            // камень, модовые олово/уран/золото/серебро); BF_MINE без него -
            // фермы, колодцы, буры игрока и города.
            if (bf == BF_MINE && !BoolField(b->data, "is foliage"))
                continue;
            DfNode n;
            n.building = b->getHandle();
            n.pos = b->getPosition();
            n.function = static_cast<int>(bf);
            n.item = FirstRef(FirstRef(b->data, "functionality"), "produces");
            n.itemName = n.item != NULL ? n.item->name : std::string();
            n.itemValue = IntField(n.item, "value");
            n.distance = n.pos.distance(centre);
            if (!_finite(n.distance) || n.distance > radius) continue;
            UseableStuff* const use = b->getUseableStuff();
            n.takeRange = use != NULL && _finite(use->maxUseRange) && use->maxUseRange > 0.0f
                ? use->maxUseRange : kDfTakeRange;
            n.maxMiners = use != NULL && use->numOperatorsMax > 0 ? use->numOperatorsMax : 0;
            n.freeSlots = use != NULL && use->currentOperators.size() < static_cast<size_t>(n.maxMiners)
                ? n.maxMiners - static_cast<int>(use->currentOperators.size()) : 0;
            n.freeSlot = use != NULL && (probe != NULL ? use->isFreeSlot(probe->getHandle()) : n.freeSlots > 0);
            n.capacity = -1;
            Inventory* const nodeInventory = b->getInventory();
            if (nodeInventory != NULL)
            {
                const lektor<InventorySection*>& sections = nodeInventory->getAllSections();
                // itemsLimit counts STACKS, not units (native 719b80). Derive
                // units only from a real output item's footprint/stack limit.
                const lektor<Item*>& produced = nodeInventory->getAllItems();
                Item* sample = NULL;
                bool onlyOutput = produced.size() <= 4096;
                for (uint32_t k = 0; onlyOutput && k < produced.size(); ++k)
                {
                    Item* const p = produced.stuff[k];
                    if (p == NULL || p->getGameData() != n.item) onlyOutput = false;
                    else sample = p;
                }
                if (onlyOutput && sample != NULL && sections.size() == 1 && sections.stuff[0] != NULL)
                {
                    InventorySection* const sec = sections.stuff[0];
                    if (sec->enabled && sec->isLimitedSlotCompatible(n.item) &&
                        sample->itemWidth > 0 && sample->itemHeight > 0 && sec->width > 0 && sec->height > 0)
                    {
                        __int64 slots = static_cast<__int64>(sec->width / sample->itemWidth) *
                            (sec->height / sample->itemHeight);
                        if (sec->itemsLimit > 0 && slots > sec->itemsLimit) slots = sec->itemsLimit;
                        const int stack = sample->isStackable(sec);
                        if (stack > 0 && slots > 0 && slots <= INT_MAX / stack)
                            n.capacity = static_cast<int>(slots * stack);
                    }
                }
            }
            out.push_back(n);
        }
    }
}

void DfFindNodes(const Ogre::Vector3& from, float radius, Character* probe, std::vector<DfNode>& out)
{
    __try
    {
        Find(from, radius, probe, out);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: looking for ore nodes failed once");
    }
}

namespace
{
    void Describe(const Ogre::Vector3& from, float radius, std::map<int, int>& types, std::map<int, int>& funcs)
    {
        if (ou == NULL || ou->zoneMgr == NULL)
            return;
        static lektor<RootObject*> objs;
        objs.clear();
        ou->zoneMgr->buildingGrid.getObjects(from, radius, objs, kMaxObjects);
        for (uint32_t i = 0; i < objs.size(); ++i)
        {
            RootObject* const o = objs.stuff[i];
            if (o == NULL)
                continue;
            const itemType t = o->getDataType();
            ++types[static_cast<int>(t)];
            if (IsBuildingType(t))
                ++funcs[static_cast<int>(static_cast<Building*>(o)->getSpecialFunction())];
        }
    }

    bool SafeDescribe(const Ogre::Vector3& from, float radius, std::map<int, int>& types, std::map<int, int>& funcs)
    {
        __try
        {
            Describe(from, radius, types, funcs);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

std::string DfDescribeArea(const Ogre::Vector3& from, float radius)
{
    std::map<int, int> types, funcs;
    if (!SafeDescribe(from, radius, types, funcs))
        return "failed";
    std::string r = "types:";
    char buf[32];
    for (std::map<int, int>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        sprintf_s(buf, sizeof(buf), " %d=%d", it->first, it->second);
        r += buf;
    }
    r += "; building functions:";
    for (std::map<int, int>::const_iterator it = funcs.begin(); it != funcs.end(); ++it)
    {
        sprintf_s(buf, sizeof(buf), " %d=%d", it->first, it->second);
        r += buf;
    }
    return r;
}

namespace
{
    bool GameMine(Character* c, Building*& out, float& score)
    {
        out = NULL;
        score = 0.0f;
        AI* const ai = c != NULL ? c->getAI() : NULL;
        if (ai == NULL)
            return false;
        hand target;
        score = ai->findMineToWorkAt(hand(), target, true);
        out = target.getBuilding();
        return out != NULL;
    }

    bool SafeGameMine(Character* c, Building*& out, float& score)
    {
        __try
        {
            return GameMine(c, out, score);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            out = NULL;
            return false;
        }
    }

    void ListMines(const Ogre::Vector3& from, float radius, int max, std::string& r)
    {
        if (ou == NULL || ou->zoneMgr == NULL)
            return;
        static lektor<RootObject*> objs;
        objs.clear();
        ou->zoneMgr->buildingGrid.getObjects(from, radius, objs, kMaxObjects);
        int shown = 0;
        char buf[200];
        for (uint32_t i = 0; i < objs.size() && shown < max; ++i)
        {
            RootObject* const o = objs.stuff[i];
            if (o == NULL || !IsBuildingType(o->getDataType()))
                continue;
            Building* const b = static_cast<Building*>(o);
            const int bf = static_cast<int>(b->getSpecialFunction());
            if (bf != BF_MINE && bf != BF_MINE_NATURAL)
                continue;
            ++shown;
            sprintf_s(buf, sizeof(buf), "%s (fn %d, foliage %d, type %d, %.0f m); ",
                      b->data != NULL ? b->data->name.c_str() : "?", bf, BoolField(b->data, "is foliage") ? 1 : 0,
                      static_cast<int>(o->getDataType()), b->getPosition().distance(from));
            r += buf;
        }
    }

    void SafeListMines(const Ogre::Vector3& from, float radius, int max, std::string& r)
    {
        __try
        {
            ListMines(from, radius, max, r);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

std::string DfGameMineFor(Character* c)
{
    Building* b = NULL;
    float score = 0.0f;
    if (!SafeGameMine(c, b, score))
        return std::string();
    char buf[256];
    sprintf_s(buf, sizeof(buf), "%s (fn %d, %.0f m, score %.2f)", b->data != NULL ? b->data->name.c_str() : "?",
              static_cast<int>(b->getSpecialFunction()), b->getPosition().distance(c->getPosition()), score);
    return buf;
}

std::string DfListMinesNear(const Ogre::Vector3& from, float radius, int max)
{
    std::string r;
    SafeListMines(from, radius, max, r);
    return r.empty() ? std::string("none") : r;
}

bool DfOrderMine(Character* c, const DfNode& node)
{
    __try
    {
        OrdersReceiver* const orders = c != NULL ? c->getOrdersReciever() : NULL;
        if (orders == NULL)
            return false;
        orders->addOrder(OPERATE_MACHINERY, node.building, node.pos, true, false);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

int DfGoalOf(Character* c)
{
    __try
    {
        OrdersReceiver* const orders = c != NULL ? c->getOrdersReciever() : NULL;
        return orders != NULL ? static_cast<int>(orders->getCurrentGoal().key()) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

bool DfEconomyAvailable(Character* c)
{
    if(c==NULL || c->isDead() || c->isUnconcious() || c->isInCombatMode(true,true)) return false;
    const int goal=DfGoalOf(c);
    // Medical, rescue, sleep, preservation and combat goals stay with native AI.
    return goal==-1 || goal==NULL_TASK || goal==MOVE_ON_FREE_WILL || goal==OPERATE_MACHINERY;
}

namespace
{
    bool g_transferFault = false;
    int g_transferDepth = 0;
    struct TransferGuard
    {
        TransferGuard() { ++g_transferDepth; }
        ~TransferGuard() { --g_transferDepth; }
    };

    // Bounded borrowed inventory graph: backpack/container inventories are not
    // included in Inventory::countItems (verified native direct _allItems loop).
    struct InventoryView
    {
        Inventory* inv[16];
        int count;
        bool AddChild(Inventory* child)
        {
            if (child == NULL) return true;
            for (int k = 0; k < count; ++k) if (inv[k] == child) return true;
            if (count == 16) return false;
            inv[count++] = child;
            return true;
        }
        explicit InventoryView(Inventory* root) : count(0)
        {
            if (root == NULL) return;
            inv[count++] = root;
            unsigned int examined = 0;
            for (int at = 0; at < count; ++at)
            {
                const lektor<Item*>& all = inv[at]->getAllItems();
                if (all.size() > 4096) { count = 0; return; }
                for (uint32_t i = 0; i < all.size(); ++i)
                {
                    if (++examined > 4096) { count = 0; return; }
                    Item* const p = all.stuff[i];
                    Inventory* const child = p != NULL ? p->getInventory() : NULL;
                    if (!AddChild(child)) { count = 0; return; }
                }
                // Native _allItems excludes equipped gear. The real worn
                // backpack is published by its physical section, not that list.
                const lektor<InventorySection*>& sections = inv[at]->getAllSections();
                if (sections.size() > 128) { count = 0; return; }
                for (uint32_t s = 0; s < sections.size(); ++s)
                {
                    InventorySection* const section = sections.stuff[s];
                    if (section == NULL || section->getInventory() != inv[at]) { count = 0; return; }
                    const Ogre::vector<InventorySection::SectionItem>::type& placed = section->getItems();
                    if (placed.size() > 4096) { count = 0; return; }
                    for (size_t k = 0; k < placed.size(); ++k)
                    {
                        if (++examined > 4096) { count = 0; return; }
                        Item* const p = placed[k].item;
                        if (p != NULL && p->isEquipped && !AddChild(p->getInventory()))
                        { count = 0; return; }
                    }
                }
            }
        }
    };

    int CountInventory(Inventory* root, GameData* item)
    {
        if (root == NULL || item == NULL) return -1;
        InventoryView view(root);
        if (view.count == 0) return -1;
        int units = 0;
        for (int at = 0; at < view.count; ++at)
        {
            const lektor<Item*>& all = view.inv[at]->getAllItems();
            for (uint32_t i = 0; i < all.size(); ++i)
            {
                Item* const p = all.stuff[i];
                if (p == NULL || p->getGameData() != item) continue;
                if (p->quantity <= 0 || units > INT_MAX - p->quantity) return -1;
                units += p->quantity;
            }
        }
        return units;
    }

    int CountOwnedInventory(Inventory* root,GameData* item)
    {
        if(root==NULL || item==NULL) return -1;
        InventoryView view(root);
        if(view.count==0) return -1;
        // Fixed open-addressed set: no owning heap state across native SEH.
        int count=0;Item* seen[8192]={0};unsigned int entries=0;
        for(int at=0;at<view.count;++at)
        {
            const lektor<InventorySection*>& sections=view.inv[at]->getAllSections();
            for(uint32_t k=0;k<sections.size();++k)
            {
                const Ogre::vector<InventorySection::SectionItem>::type& placed=sections.stuff[k]->getItems();
                for(size_t j=0;j<placed.size();++j)
                {
                    Item* const p=placed[j].item;
                    if(p==NULL || p->getGameData()!=item) continue;
                    if(++entries>4096) return -1;
                    size_t slot=(reinterpret_cast<size_t>(p)>>4)&8191;
                    while(seen[slot]!=NULL && seen[slot]!=p) slot=(slot+1)&8191;
                    if(seen[slot]==p || p->quantity<=0 || count>INT_MAX-p->quantity) return -1;
                    seen[slot]=p;
                    count+=p->quantity;
                }
            }
        }
        return count;
    }
    Item* FirstInventory(Inventory* root, GameData* item, Inventory*& actual, bool onlyClean = false)
    {
        actual = NULL;
        if (root == NULL || item == NULL) return NULL;
        InventoryView view(root);
        for (int at = 0; at < view.count; ++at)
        {
            const lektor<Item*>& all = view.inv[at]->getAllItems();
            for (uint32_t i = 0; i < all.size(); ++i)
            {
                Item* const p = all.stuff[i];
                if (p != NULL && !p->isEquipped && p->quantity > 0 && p->getGameData() == item &&
                    (!onlyClean || !p->isStolen(true)))
                {
                    actual = view.inv[at];
                    return p;
                }
            }
        }
        return NULL;
    }

    Inventory* WithRoom(Inventory* root, GameData* item)
    {
        InventoryView view(root);
        for (int i = 0; i < view.count; ++i)
            if (view.inv[i]->hasRoomForItem(item)) return view.inv[i];
        return NULL;
    }
    int FirstFit(Inventory* root,const std::vector<GameData*>& ranked)
    {
        if(ranked.size()>64) return -1;
        InventoryView view(root);
        for(size_t i=0;i<ranked.size();++i)
            for(int at=0;at<view.count;++at)
                if(ranked[i]!=NULL && view.inv[at]->hasRoomForItem(ranked[i])) return static_cast<int>(i);
        return -1;
    }

    bool SafeRemove(Inventory* inv, Item* source, Item*& detached)
    {
        __try
        {
            detached = inv->removeItemDontDestroy_returnsItem(source, 1, source->quantity > 1);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { ErrorLog("DynamicFactions: native ore removal fault"); return false; }
    }
    bool SafeAdd(Inventory* inv, Item* detached, bool& accepted)
    {
        __try
        {
            accepted = inv->addItem(detached, 1, false, false);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { ErrorLog("DynamicFactions: native ore insertion fault"); return false; }
    }
    bool SafeAddSection(InventorySection* section, Item* detached, bool& accepted)
    {
        __try
        {
            accepted = section->addItem(detached, 1);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { ErrorLog("DynamicFactions: native stock section insertion fault"); return false; }
    }

    bool SameTitle(const hand& a, const hand& b)
    {
        return a.type==b.type && a.container==b.container && a.containerSerial==b.containerSerial &&
            a.index==b.index && a.serial==b.serial;
    }
    bool SafeTitle(Item* item, const hand& title)
    {
        __try
        {
            if (item==NULL || item->isInInventory || item->quantity!=1) return false;
            item->setProperOwner(title);
            return SameTitle(item->getProperOwner(),title);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        { ErrorLog("DynamicFactions: purchase ownership transfer fault");return false; }
    }

    class NativeTransfer : public DfTransferPort
    {
        Inventory* from_;
        Inventory* actual_;
        Item* source_;
        Inventory* to_;
        Inventory* destination_;
        InventorySection* section_;
        GameData* item_;
        bool purchase_;
        hand previousTitle_;
        hand purchaseTitle_;
    public:
        NativeTransfer(Inventory* from, Inventory* actual, Item* source, Inventory* to,
                       Inventory* destination, GameData* item, InventorySection* section = NULL,
                       const hand* title = NULL)
            : from_(from), actual_(actual), source_(source), to_(to), destination_(destination),
            section_(section), item_(item), purchase_(title!=NULL),
            previousTitle_(title!=NULL ? source->getProperOwner() : hand()),
            purchaseTitle_(title!=NULL ? *title : hand()) {}
        virtual bool Read(int& source, int& destination)
        {
            const bool bag=item_->type==CONTAINER;
            source = bag ? DfCountOwnedInventoryItem(from_,item_) : DfCountInventoryItem(from_, item_);
            destination = bag ? DfCountOwnedInventoryItem(to_,item_) : DfCountInventoryItem(to_, item_);
            return source >= 0 && destination >= 0;
        }
        virtual bool Remove(void*& item)
        {
            Item* detached = NULL;
            const bool ok = SafeRemove(actual_, source_, detached);
            item = detached;
            return ok;
        }
        virtual bool Detached(void* item)
        {
            Item* const p = static_cast<Item*>(item);
            return p != NULL && p->quantity == 1 && !p->isInInventory;
        }
        virtual bool Add(void* item, bool& accepted)
        {
            if (purchase_ && !SafeTitle(static_cast<Item*>(item),purchaseTitle_)) return false;
            if (section_ != NULL) return SafeAddSection(section_, static_cast<Item*>(item), accepted);
            return SafeAdd(destination_, static_cast<Item*>(item), accepted);
        }
        virtual bool Restore(void* item, bool& accepted)
        {
            if (purchase_ && !SafeTitle(static_cast<Item*>(item),previousTitle_)) return false;
            return SafeAdd(actual_, static_cast<Item*>(item), accepted);
        }
    };

    bool MoveUnit(Inventory* from, Inventory* actual, Item* source, Inventory* to, GameData* item,
                  InventorySection* pinned = NULL, const hand* title = NULL)
    {
        if (g_transferFault || from == NULL || actual == NULL || source == NULL ||
            to == NULL || from == to || item == NULL || source->isEquipped ||
            source->quantity <= 0 || source->getGameData() != item || !actual->hasItem(source)) return false;
        if (pinned != NULL)
        {
            if (pinned->getInventory() != to || !pinned->enabled ||
                !pinned->isLimitedSlotCompatible(source) || !pinned->hasRoomForItem(item, 1)) return false;
            const lektor<InventorySection*>& sections = to->getAllSections();
            if (sections.size() > 128) return false;
            bool found = false;
            for (uint32_t k = 0; k < sections.size(); ++k) if (sections.stuff[k] == pinned) found = true;
            if (!found) return false;
        }
        Inventory* const destination = pinned != NULL ? to : WithRoom(to, item);
        if (destination == NULL) return false;
        NativeTransfer native(from, actual, source, to, destination, item, pinned, title);
        const DfUnitResult result = DfTransferOne(native);
        if (result == DF_UNIT_FAULT) DfQuarantineTransfer("ore delivery requires reconciliation; no replay");
        return result == DF_UNIT_MOVED;
    }

    int TakeOre(Character* c, const hand& node, GameData* item, int maxUnits)
    {
        if (g_transferFault || c == NULL || item == NULL || maxUnits <= 0 || c->isDead() ||
            c->isUnconcious() || c->isInCombatMode(true, true) || c->isPlayerCharacter()) return 0;
        Building* const b = node.getBuilding();
        if (b == NULL || b->data == NULL || c->getFaction() == NULL) return 0;
        const BuildingFunction bf = b->getSpecialFunction();
        if (bf != BF_MINE_NATURAL && (bf != BF_MINE || !BoolField(b->data, "is foliage"))) return 0;
        if (FirstRef(FirstRef(b->data, "functionality"), "produces") != item) return 0;
        UseableStuff* const use = b->getUseableStuff();
        const float takeRange = use != NULL && _finite(use->maxUseRange) && use->maxUseRange > 0.0f
            ? use->maxUseRange : kDfTakeRange;
        const float distance = c->getPosition().distance(b->getPosition());
        if (!_finite(distance) || distance > takeRange) return 0;
        // Natural resources may have no faction; a claimed hostile resource is
        // not free cargo, and player-owned industry is never harvested here.
        if (!DfCanHarvestNode(b, c->getFaction())) return 0;
        Inventory* const from = b->getInventory();
        Inventory* const to = c->getInventory();
        if (from == NULL || to == NULL) return 0;
        int moved = 0;
        Inventory* actual = NULL;
        Item* source = NULL;
        const int limit = maxUnits < kDfTransferMaxUnits ? maxUnits : kDfTransferMaxUnits;
        while (moved < limit && !g_transferFault)
        {
            if (source == NULL) source = FirstInventory(from, item, actual);
            if (source == NULL) break;
            const bool remains = source->quantity > 1;
            if (!MoveUnit(from, actual, source, to, item)) break;
            ++moved;
            if (!remains) source = NULL;
        }
        return moved;
    }

    void SafeTakeOre(Character* c, const hand& node, GameData* item, int maxUnits, int& moved)
    {
        __try { moved = TakeOre(c, node, item, maxUnits); }
        __except (EXCEPTION_EXECUTE_HANDLER) { DfQuarantineTransfer("taking ore raised a native fault"); }
    }
}

bool DfTransferFaulted() { return g_transferFault; }
bool DfSellTransferBusy();
bool DfTransferBusy() { return g_transferDepth != 0 || DfSellTransferBusy(); }
void DfResetTransferFault() { g_transferFault = false; }
void DfQuarantineTransfer(const char* reason)
{
    if (!g_transferFault) ErrorLog(std::string("DynamicFactions: transfers stopped: ") + reason);
    g_transferFault = true;
}

int DfCountInventoryItem(Inventory* root, GameData* item)
{
    __try { return CountInventory(root, item); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}
bool DfInventoryHasRoom(Inventory* root, GameData* item)
{
    __try { return root != NULL && item != NULL && WithRoom(root, item) != NULL; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
int DfFirstInventoryFit(Inventory* root,const std::vector<GameData*>& ranked)
{
    __try {return FirstFit(root,ranked);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return -1;}
}
int DfCountOwnedInventoryItem(Inventory* root,GameData* item)
{
    __try {return CountOwnedInventory(root,item);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return -1;}
}
Item* DfFirstInventoryItem(Inventory* root, GameData* item, Inventory*& actual, bool onlyClean)
{
    __try { return FirstInventory(root, item, actual, onlyClean); }
    __except (EXCEPTION_EXECUTE_HANDLER) { actual = NULL; return NULL; }
}
bool DfMoveInventoryUnit(Inventory* from, Inventory* actual, Item* source, Inventory* to, GameData* item)
{
    __try { return MoveUnit(from, actual, source, to, item); }
    __except (EXCEPTION_EXECUTE_HANDLER) { DfQuarantineTransfer("moving ore raised a native fault"); return false; }
}
bool DfMoveInventoryUnitToSection(Inventory* from, Inventory* actual, Item* source,
                                  InventorySection* to, GameData* item)
{
    __try { return to != NULL && MoveUnit(from, actual, source, to->getInventory(), item, to); }
    __except (EXCEPTION_EXECUTE_HANDLER)
    { DfQuarantineTransfer("moving ore to real merchant stock raised a native fault"); return false; }
}

bool DfMovePurchasedUnit(Inventory* from, Inventory* actual, Item* source, Inventory* to,
                       GameData* item, const hand& newOwner)
{
    __try { return MoveUnit(from,actual,source,to,item,NULL,&newOwner); }
    __except(EXCEPTION_EXECUTE_HANDLER)
    { DfQuarantineTransfer("physical purchase transfer fault; no replay");return false; }
}

namespace
{
    bool ReadSupplyItem(Item* p,RaceData* race,bool animal,DfSupplyStock& stock,
        Item** bags,int& bagCount,double* totals)
    {
        if(p==NULL) return true;
        if(p->itemFunction==ITEM_CONTAINER && p->slotType==ATTACH_BACKPACK)
        {
            bool seen=false;for(int k=0;k<bagCount;++k)if(bags[k]==p)seen=true;
            if(!seen)
            {
                if(bagCount==128 || p->quantity<=0 || stock.ownedBags>INT_MAX-p->quantity) return false;
                bags[bagCount++]=p;stock.ownedBags+=p->quantity;
                if(p->isEquipped) stock.wearingBag=true;
            }
        }
        if(p->isEquipped || p->deathItem) return true;
        int kind=-1;
        if((p->itemFunction==ITEM_FOOD || p->itemFunction==ITEM_FOOD_RESTRICTED) &&
            !stock.robot && race->canEat(p,animal)) kind=0;
        else if(p->itemFunction==ITEM_FIRSTAID) kind=1;
        else if(p->itemFunction==ITEM_ROBOTREPAIR) kind=2;
        if(kind<0) return true;
        if(p->quantity<=0 || !_finite(p->chargesLeft) || !_finite(p->originalFullChargeAmount) ||
            p->chargesLeft<0 || p->originalFullChargeAmount<=0 || p->chargesLeft>p->originalFullChargeAmount) return false;
        totals[kind]+=p->chargesLeft+static_cast<double>(p->quantity-1)*p->originalFullChargeAmount;
        return totals[kind]<=FLT_MAX;
    }
    bool ReadSupplies(Character* actor,DfSupplyStock& stock)
    {
        stock=DfSupplyStock();
        if(actor==NULL || actor->getRace()==NULL || actor->getMedical()==NULL) return false;
        RaceData* const race=actor->getRace();
        stock.robot=race->robot;stock.foodRate=stock.robot ? 0.0f : race->hungerRate;
        stock.hunger=actor->getMedical()->hunger;stock.animal=actor->isAnimal()!=NULL;
        stock.robotLimbs=stock.robot || actor->getMedical()->hasRobotics();
        if(!_finite(stock.foodRate) || stock.foodRate<0 || !_finite(stock.hunger)) return false;
        InventoryView view(actor->getInventory());
        if(view.count==0) return false;
        double totals[3]={0,0,0};Item* bags[128];int bagCount=0;
        for(int at=0;at<view.count;++at)
        {
            const lektor<Item*>& all=view.inv[at]->getAllItems();
            for(uint32_t k=0;k<all.size();++k)
                if(!ReadSupplyItem(all.stuff[k],race,stock.animal,stock,bags,bagCount,totals)) return false;
            // Worn bags may be absent from allItems. Visit only equipped items
            // here; bag identity deduplication handles SDKs publishing both.
            const lektor<InventorySection*>& sections=view.inv[at]->getAllSections();
            for(uint32_t k=0;k<sections.size();++k)
                for(size_t j=0;j<sections.stuff[k]->getItems().size();++j)
                {
                    Item* const p=sections.stuff[k]->getItems()[j].item;
                    if(p!=NULL && p->isEquipped && !ReadSupplyItem(p,race,stock.animal,stock,bags,bagCount,totals)) return false;
                }
        }
        stock.food=static_cast<float>(totals[0]*.01);stock.firstAid=static_cast<float>(totals[1]);
        stock.repair=static_cast<float>(totals[2]);stock.known=true;
        return true;
    }
    bool ReadFood(Character* actor,float& rate,float& hunger,float& nutrition)
    {
        DfSupplyStock stock;
        if(!ReadSupplies(actor,stock)) return false;
        rate=stock.foodRate;hunger=stock.hunger;nutrition=stock.food;return true;
    }
}

bool DfReadSupplyStock(Character* actor,DfSupplyStock& stock)
{
    __try {return ReadSupplies(actor,stock);}
    __except(EXCEPTION_EXECUTE_HANDLER) {stock=DfSupplyStock();return false;}
}

bool DfReadFood(Character* actor,float& rate,float& hunger,float& nutrition)
{
    __try {return ReadFood(actor,rate,hunger,nutrition);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}

namespace
{
    bool ReadBagSpec(Item* item,DfBagSpec& spec,bool mustBeEmpty)
    {
        spec=DfBagSpec();
        if(item==NULL || item->getGameData()==NULL || item->getGameData()->type!=CONTAINER ||
           item->itemFunction!=ITEM_CONTAINER || item->slotType!=ATTACH_BACKPACK || item->quantity!=1) return false;
        Inventory* const inside=item->getInventory();
        if(inside==NULL) return false;
        const lektor<InventorySection*>& sections=inside->getAllSections();
        if(sections.size()==0 || sections.size()>16) return false;
        for(uint32_t k=0;k<sections.size();++k)
        {
            InventorySection* const s=sections.stuff[k];
            if(s==NULL || s->getInventory()!=inside || s->getItems().size()>4096 ||
                (mustBeEmpty && !s->getItems().empty())) return false;
            if(!s->enabled || s->isAnEquippedItemSection) continue;
            if(s->width<=0 || s->height<=0 || s->width>128 || s->height>128 ||
                spec.cells>4096-s->width*s->height || !_finite(s->stackingBonusMult) ||
                s->stackingBonusMult<0 || s->stackingBonusMin<0) return false;
            spec.cells+=s->width*s->height;
            const float mult=s->stackingBonusMult>s->stackingBonusMin ? s->stackingBonusMult : static_cast<float>(s->stackingBonusMin);
            if(mult>spec.stackMult) spec.stackMult=mult;
        }
        ContainerItem* const bag=static_cast<ContainerItem*>(item);
        spec.weight=item->weight;spec.weightMult=bag->weightMult;
        spec.athletics=bag->athleticsMult;spec.combatSpeed=bag->combatSpeedMult;
        spec.combatBonus=bag->combatSkillBonus;spec.stealth=bag->stealthMult;
        return spec.cells>0;
    }
    bool BagCanWear(Character* actor,Item* item)
    {
        if(actor==NULL || actor->isAnimal()!=NULL || actor->getRace()==NULL || actor->getRace()->data==NULL ||
            actor->getInventory()==NULL || item==NULL || item->getGameData()==NULL || item->getGameData()->type!=CONTAINER) return false;
        if(!static_cast<ContainerItem*>(item)->_NV_canEquip(actor->getRace()->data)) return false;
        const lektor<InventorySection*>& slots=actor->getInventory()->getAllSections();
        if(slots.size()>128) return false;
        for(uint32_t k=0;k<slots.size();++k)
        {
            InventorySection* const s=slots.stuff[k];
            if(s!=NULL && s->enabled && s->isAnEquippedItemSection && s->limitedSlot==ATTACH_BACKPACK &&
                s->getItems().empty() && s->isLimitedSlotCompatible(item) && s->_NV_hasRoomForItem(item->getGameData(),1)) return true;
        }
        return false;
    }
    bool EquipOwnedBag(Character* actor,bool light,bool& equipped)
    {
        equipped=false;
        if(actor==NULL || actor->getInventory()==NULL) return false;
        Inventory* const root=actor->getInventory();
        const lektor<InventorySection*>& sections=root->getAllSections();
        if(sections.size()>128) return false;
        for(uint32_t k=0;k<sections.size();++k)
        {
            if(sections.stuff[k]==NULL || sections.stuff[k]->getItems().size()>4096) return false;
            for(size_t j=0;j<sections.stuff[k]->getItems().size();++j)
            {
                Item* const p=sections.stuff[k]->getItems()[j].item;
                if(p!=NULL && p->isEquipped && p->itemFunction==ITEM_CONTAINER && p->slotType==ATTACH_BACKPACK)
                {equipped=true;return true;}
            }
        }
        const lektor<Item*>& all=root->getAllItems();
        if(all.size()>4096) return false;
        Item* best=NULL;double score=-1;
        for(uint32_t k=0;k<all.size();++k)
        {
            Item* const p=all.stuff[k];DfBagSpec spec;
            if(p==NULL || p->isEquipped || p->deathItem || !ReadBagSpec(p,spec,false) || !BagCanWear(actor,p)) continue;
            const double candidate=DfBagScore(spec,light,0);
            if(candidate>score) {score=candidate;best=p;}
        }
        if(best==NULL) return true;
        GameData* const data=best->getGameData();const hand identity=best->getHandle();
        const int before=DfCountOwnedInventoryItem(root,data);
        if(before<=0) return false;
        root->equipItem(best);
        Item* const after=identity.getItem();
        if(after==NULL || !SameTitle(identity,after->getHandle()) || DfCountOwnedInventoryItem(root,data)!=before)
        {DfQuarantineTransfer("owned backpack equip requires reconciliation; no replay");return false;}
        const lektor<InventorySection*>& updated=root->getAllSections();
        if(updated.size()>128){DfQuarantineTransfer("backpack equip section list changed unexpectedly");return false;}
        if(after->isEquipped)
            for(uint32_t k=0;k<updated.size();++k)
            {
                InventorySection* const s=updated.stuff[k];
                if(s==NULL || s->getItems().size()>4096)
                {DfQuarantineTransfer("backpack equip sections require reconciliation");return false;}
                if(s->isAnEquippedItemSection && s->limitedSlot==ATTACH_BACKPACK)
                    for(size_t j=0;j<s->getItems().size();++j)
                        if(s->getItems()[j].item==after) equipped=true;
            }
        return true;
    }
    bool LightBagRole(Character* actor)
    {
        if(actor==NULL || actor->getStats()==NULL) return true;
        const float strength=actor->getStats()->getStat(STAT_STRENGTH,true);
        const float athletics=actor->getStats()->getStat(STAT_ATHLETICS,true);
        return !_finite(strength) || !_finite(athletics) || strength<30 || strength<athletics*.7f;
    }
}
bool DfReadBagSpec(Item* item,DfBagSpec& spec,bool mustBeEmpty)
{
    __try {return ReadBagSpec(item,spec,mustBeEmpty);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool DfBagCanWear(Character* actor,Item* item)
{
    __try {return BagCanWear(actor,item);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
static bool SafeEquipOwnedBag(Character* actor,bool light,bool& equipped)
{
    __try {return EquipOwnedBag(actor,light,equipped);}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {equipped=false;DfQuarantineTransfer("backpack equip fault; no replay");return false;}
}
bool DfEquipOwnedBag(Character* actor,bool light,bool& equipped)
{
    equipped=false;if(DfTransferBusy() || DfTransferFaulted()) return false;
    TransferGuard guard;
    return SafeEquipOwnedBag(actor,light,equipped);
}
bool DfLightBagRole(Character* actor)
{
    __try {return LightBagRole(actor);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return true;}
}

int DfTakeOreFromNode(Character* c, const hand& node, GameData* item, int maxUnits)
{
    TransferGuard guard;
    int moved = 0;
    SafeTakeOre(c, node, item, maxUnits, moved);
    return moved;
}

namespace
{
    bool FindCarriedOre(Character* c, const std::vector<GameData*>& types, GameData*& item, int& units)
    {
        Inventory* const root = c != NULL ? c->getInventory() : NULL;
        InventoryView view(root);
        if (view.count == 0) { units = -1; return false; }
        const std::set<GameData*> wanted(types.begin(), types.end());
        // One borrowed inventory walk for ALL known ore types, then one count
        // of the selected type. No types * full-inventory native allocations.
        for (int at = 0; at < view.count; ++at)
        {
            const lektor<Item*>& all = view.inv[at]->getAllItems();
            for (uint32_t i = 0; i < all.size(); ++i)
            {
                Item* const p = all.stuff[i];
                if (p == NULL || p->isEquipped || p->quantity <= 0) continue;
                GameData* const data = p->getGameData();
                if (data == NULL || wanted.find(data) == wanted.end()) continue;
                const int count = DfCountInventoryItem(root, data);
                if (count <= 0) { units = -1; return false; }
                item = data;
                units = count;
                return true;
            }
        }
        return false;
    }
}

bool DfFindCarriedOre(Character* c, const std::vector<GameData*>& types, GameData*& item, int& units)
{
    item = NULL;
    units = 0;
    __try { return FindCarriedOre(c, types, item, units); }
    __except (EXCEPTION_EXECUTE_HANDLER) { units = -1; return false; }
}

bool DfLoadedNodeAt(const Ogre::Vector3& pos, DfNode& out)
{
    std::vector<DfNode> here;
    DfFindNodes(pos, 10.0f, NULL, here);
    if (here.empty())
        return false;
    out = here[0];
    return true;
}

namespace
{
    GameData* ItemBySid(const std::string& sid, int& value)
    {
        value = 0;
        if (ou == NULL || sid.empty())
            return NULL;
        GameData* const d = ou->gamedata.getData(sid);
        value = IntField(d, "value");
        return d;
    }

    GameData* SafeItemBySid(const std::string& sid, int& value)
    {
        __try
        {
            return ItemBySid(sid, value);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            value = 0;
            return NULL;
        }
    }
}

GameData* DfItemBySid(const std::string& sid, int& value)
{
    return SafeItemBySid(sid, value);
}

void DfClearOrders(Character* c)
{
    __try
    {
        OrdersReceiver* const orders = c != NULL ? c->getOrdersReciever() : NULL;
        if (orders != NULL)
            orders->clearOrders();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

int DfCountInNode(const hand& node, GameData* item)
{
    __try
    {
        Building* const b = node.getBuilding();
        Inventory* const inv = b != NULL ? b->getInventory() : NULL;
        return inv != NULL && item != NULL ? DfCountInventoryItem(inv, item) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -2;
    }
}

bool DfCanHarvestNode(Building* b, Faction* worker)
{
    Faction* const empty = ou != NULL && ou->factionMgr != NULL
        ? ou->factionMgr->getEmptyFaction() : NULL;
    return b != NULL && DfCanHarvestOwner(b->owner, worker, empty);
}

int DfCountItem(Character* c, GameData* item)
{
    __try
    {
        Inventory* const inv = c != NULL ? c->getInventory() : NULL;
        return inv != NULL && item != NULL ? DfCountInventoryItem(inv, item) : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}
