#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <Debug.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/Building/ProductionBuilding.h>

#include <set>
#include <string>

namespace finite_ore
{
    namespace
    {
        typedef std::set<std::string> TemplateIds;

        SRWLOCK catalogLock = SRWLOCK_INIT;
        TemplateIds naturalTemplates;
        bool catalogReady = false;

        class SharedCatalogLock
        {
        public:
            SharedCatalogLock() { AcquireSRWLockShared(&catalogLock); }
            ~SharedCatalogLock() { ReleaseSRWLockShared(&catalogLock); }
        };

        class ExclusiveCatalogLock
        {
        public:
            ExclusiveCatalogLock() { AcquireSRWLockExclusive(&catalogLock); }
            ~ExclusiveCatalogLock() { ReleaseSRWLockExclusive(&catalogLock); }
        };

        bool BuildCatalog(GameDataContainer& data, TemplateIds& result)
        {
            const auto& records = data._getAllData();
            if (records.empty())
                return false;

            const std::string buildingType("building type");
            for (auto it = records.begin(); it != records.end(); ++it)
            {
                GameData* record = it->second;
                if (record == NULL || record->type != FOLIAGE_MESH)
                    continue;

                // Resolve the same first reference as the foliage placer,
                // without inserting a missing list or caching a game pointer.
                const auto refs = record->getReferenceListIfExists(buildingType);
                if (refs != NULL && !refs->empty())
                {
                    const GameData* const target = data.getData(refs->front().sid, BUILDING);
                    if (target != NULL)
                        result.insert(target->stringID);
                }
            }

            if (result.empty())
                return true;

            const std::string unlocks("enable buildings");
            const std::string upgrades("upgrades to");
            for (auto it = records.begin(); it != records.end() && !result.empty(); ++it)
            {
                const GameData* record = it->second;
                if (record == NULL || (record->type != RESEARCH && record->type != BUILDING))
                    continue;
                const auto refs = record->getReferenceListIfExists(
                    record->type == RESEARCH ? unlocks : upgrades);
                if (refs == NULL)
                    continue;
                for (auto ref = refs->begin(); ref != refs->end(); ++ref)
                    result.erase(ref->sid);
            }
            return true;
        }

        bool IsNaturalTemplate(GameDataContainer& data, const std::string& id)
        {
            {
                SharedCatalogLock lock;
                if (catalogReady)
                    return naturalTemplates.find(id) != naturalTemplates.end();
            }

            ExclusiveCatalogLock lock;
            if (!catalogReady)
            {
                try
                {
                    TemplateIds next;
                    if (!BuildCatalog(data, next))
                        return false;
                    naturalTemplates.swap(next);
                    catalogReady = true;
                }
                catch (...)
                {
                    // Fail closed for this world and avoid retrying on every tick.
                    catalogReady = true;
                    throw;
                }
            }
            return naturalTemplates.find(id) != naturalTemplates.end();
        }
    }

    bool IsNaturalOre(ProductionBuilding* building)
    {
        if (building == NULL || !building->isFoliage ||
            building->usesStat != STAT_LABOURING ||
            (building->specialFunction != BF_MINE && building->specialFunction != BF_MINE_NATURAL))
            return false;

        const GameData* const data = building->data;
        const auto product = building->productionItem;
        const GameData* const item = product != NULL ? product->item : NULL;
        // Raw Stone is a valid mining product, outside the ore feature.
        if (data == NULL || data->stringID.empty() || item == NULL || item->type != ITEM ||
            item->stringID.empty() || item->stringID == "1515-gamedata.base")
            return false;

        GameWorld* const world = ou;
        if (world == NULL)
            return false;
        try
        {
            return IsNaturalTemplate(world->gamedata, data->stringID);
        }
        catch (...)
        {
            ErrorLog("FiniteOreVeins: unable to index natural ore templates");
            return false;
        }
    }

    void ResetNaturalOreCatalog()
    {
        ExclusiveCatalogLock lock;
        catalogReady = false;
        naturalTemplates.clear();
    }
}
