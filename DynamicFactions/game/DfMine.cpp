#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfMine.h"

#include <Windows.h>

#include <cstdio>
#include <map>

#include <Debug.h>

#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
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
        lektor<RootObject*> objs;
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
            n.freeSlot = probe != NULL && static_cast<UseableStuff*>(b)->isFreeSlot(probe->getHandle());
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
        lektor<RootObject*> objs;
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
        lektor<RootObject*> objs;
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

int DfCountInNode(const hand& node, GameData* item)
{
    __try
    {
        Building* const b = node.getBuilding();
        Inventory* const inv = b != NULL ? b->getInventory() : NULL;
        return inv != NULL && item != NULL ? inv->countItems(item) : -1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -2;
    }
}

int DfCountItem(Character* c, GameData* item)
{
    __try
    {
        Inventory* const inv = c != NULL ? c->getInventory() : NULL;
        return inv != NULL && item != NULL ? inv->countItems(item) : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}
