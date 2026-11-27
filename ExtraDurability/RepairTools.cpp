#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "ExtraDurability.h"
#include "ConditionRules.h"

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>

namespace durability {
namespace {

const char ToolsId[] = "43404-changes_otto.mod";
const char LegacyKitId[] = "1-Extra Durability (HARD).mod";
const char ChargesKey[] = "charges";
bool migrateTools = false;

GameData* (*saveItem)(Item*, GameDataContainer*, itemType) = NULL;
void (*loadItem)(Item*, GameDataContainer*, GameData*) = NULL;

bool IsStandardTools(const GameData* data)
{
    return data && data->type == ITEM && data->stringID == ToolsId;
}

GameData* SaveTool(Item* item, GameDataContainer* container, itemType type)
{
    GameData* record = saveItem(item, container, type);
    if (migrateTools && record && item->itemFunction == ITEM_TOOL &&
        item->originalFullChargeAmount > 1.0f && IsStandardTools(item->data)) {
        const auto saved = record->fdata.find(ChargesKey);
        if (saved != record->fdata.end())
            saved->second = RepairToolChargeForSave(item->chargesLeft, item->originalFullChargeAmount);
    }
    return record;
}

void LoadTool(Item* item, GameDataContainer* container, GameData* record)
{
    loadItem(item, container, record);
    if (!migrateTools || !record || item->itemFunction != ITEM_TOOL || !IsStandardTools(item->data))
        return;
    // Vanilla saves store a full set of Tools as 1. The same scale in
    // both directions also preserves fractional charges through a save.
    item->chargesLeft = RestoreRepairToolCharge(item->chargesLeft, item->originalFullChargeAmount);
}

Item* FindChargedItem(Inventory* inventory, GameData* data)
{
    return inventory && data ? inventory->getBestItemWithLowestCharges(data) : NULL;
}

} // namespace

bool IsRepairTool(const GameData* data)
{
    return data && data->type == ITEM &&
        (data->stringID == ToolsId || data->stringID == LegacyKitId);
}

Item* FindRepairTool(Character* character)
{
    if (!ou || !character)
        return NULL;
    static const std::string toolsId(ToolsId);
    static const std::string legacyId(LegacyKitId);
    Inventory* carried = character->getInventory();
    ContainerItem* bag = character->hasABackpackOn();
    Inventory* backpack = bag ? bag->getInventory() : NULL;

    GameData* tools = ou->gamedata.getData(toolsId, ITEM);
    if (Item* item = FindChargedItem(carried, tools))
        return item;
    if (Item* item = FindChargedItem(backpack, tools))
        return item;

    GameData* legacyTool = ou->gamedata.getData(legacyId, ITEM);
    Item* item = FindChargedItem(carried, legacyTool);
    return item ? item : FindChargedItem(backpack, legacyTool);
}

void InstallRepairToolState()
{
    const bool saveReady = KenshiLib::AddHook(KenshiLib::GetRealAddress(&Item::_NV__serialise),
                            &SaveTool, &saveItem) == KenshiLib::SUCCESS;
    migrateTools = saveReady && KenshiLib::AddHook(KenshiLib::GetRealAddress(&Item::_NV__loadFromSerialise),
                            &LoadTool, &loadItem) == KenshiLib::SUCCESS;
    if (!migrateTools)
        ErrorLog("ExtraDurability: could not upgrade Tools charge for old saves");
}

} // namespace durability
