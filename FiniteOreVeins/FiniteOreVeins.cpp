#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define KLOC_DOMAIN "finite_ore_veins"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <PluginSettings.h>
#include <StartupHookBatch.h>

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <float.h>
#include <list>
#include <map>
#include <set>

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Building/ProductionBuilding.h>
#include <kenshi/GameSaveState.h>
#include <kenshi/Appearance.h>
#include <kenshi/CharBody.h>
#include <kenshi/Character.h>
#include <kenshi/FoliageSystem.h>
#include <kenshi/Tasker.h>
#include <kenshi/ZoneManager.h>
#include <kenshi/ZoneMapContent.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Inventory.h>
#include <kenshi/Kenshi.h>
#include <kenshi/gui/DataPanelLine.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/util/TimeOfDay.h>

#include "FiniteOreVeins.h"
#include "ReserveRules.h"
#include "VeinSound.h"

// Globals.h declares FoliageSystem as a namespace in this SDK snapshot.
__declspec(dllimport) GameWorld* ou;

namespace finite_ore {
void ResetNaturalOreCatalog();

namespace {
const char Section[] = "FiniteOreVeins";
const std::string ShareKey("finite_ore_veins_share_v1");
const std::string UpdateKey("finite_ore_veins_minute_v1");
const std::string MinedKey("finite_ore_veins_last_mined_v1");
const std::string ClosedKey("finite_ore_veins_closed_v1");
const std::string RowKey("FiniteOreVeins.OreRemaining");

unsigned VeinCapacity(ProductionBuilding* building)
{
    const Ogre::Vector3 position = building->getPosition();
    return Capacity(building->numOperatorsMax, position.x, position.z);
}

struct Deposit {
    explicit Deposit(double now) : reserve(now), capacity(0), outputPending(false), removalQueued(false) {}
    Reserve reserve;
    unsigned capacity;
    bool outputPending;
    bool removalQueued;
};

typedef std::map<const Building*, Deposit> Deposits;
Deposits deposits;

struct ExhaustedVein {
    hand target;
    std::set<hand> workers;
};
std::list<ExhaustedVein> pendingRemovals;
volatile LONG removalPending = 0;
SRWLOCK lock = SRWLOCK_INIT;
volatile LONG detailedLog = 0;
volatile LONG enabled = 1;
volatile LONG ready = 0;
volatile LONG storeError = 0;

class StateLock {
public:
    StateLock() { AcquireSRWLockExclusive(&lock); }
    ~StateLock() { ReleaseSRWLockExclusive(&lock); }
private:
    StateLock(const StateLock&);
    StateLock& operator=(const StateLock&);
};

double GameHours()
{
    if (!ou)
        return -1;
    const double hour = ou->getTimeStamp_inGameHours().getTotalHours();
    return _finite(hour) && hour >= 0 && hour < INT_MAX / 60.0 ? hour : -1;
}

bool IniToggle(const std::string& path, const char* key, bool fallback)
{
    std::string value = PluginSettings::Text(Section, key, fallback ? "1" : "0", path);
    const size_t comment = value.find(';');
    if (comment != std::string::npos)
        value.resize(comment);
    while (!value.empty() && (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
        value.resize(value.size() - 1);
    return std::atoi(value.c_str()) != 0 || _stricmp(value.c_str(), "true") == 0;
}

void LoadSettings()
{
    const std::string path = PluginSettings::Path();
    if (path.empty())
        return;
    const bool debug = IniToggle(path, "Debug", false);
    const bool active = IniToggle(path, "Enabled", true);
    KLocLog::SetEnabled(debug);
    InterlockedExchange(&detailedLog, debug);
    InterlockedExchange(&enabled, active);
    if (debug)
        DebugLog(active ? "FiniteOreVeins: enabled, permanent depletion" :
                          "FiniteOreVeins: disabled");
}

void ReportStorageError()
{
    if (!InterlockedExchange(&storeError, 1))
        ErrorLog("FiniteOreVeins: unable to store a vein reserve");
}

void (*nativeReset)(GameWorld*) = NULL;
void ResetWorld(GameWorld* world)
{
    {
        StateLock guard;
        pendingRemovals.clear();
        InterlockedExchange(&removalPending, 0);
    }
    nativeReset(world);
    {
        StateLock guard;
        deposits.clear();
    }
    ResetNaturalOreCatalog();
}

struct BuildingAccess : Building {
    using Building::_DESTRUCTOR;
};

void (*nativeDestroy)(Building*) = NULL;
void DestroyBuilding(Building* building)
{
    if (ready) {
        StateLock guard;
        deposits.erase(building);
    }
    nativeDestroy(building);
}

// The native member returning GameSaveState by value receives hidden result
// storage after its this argument in the Windows x64 MSVC ABI.
typedef GameSaveState* (*SerializeFn)(ProductionBuilding*, GameSaveState*,
                                       GameDataContainer*, GameData*, PosRotPair*);
SerializeFn nativeSerialize = NULL;
GameSaveState* SaveDeposit(ProductionBuilding* building, GameSaveState* result,
                           GameDataContainer* container, GameData* refs, PosRotPair* offset)
{
    result = nativeSerialize(building, result, container, refs, offset);
    if (!ready || !building->isFoliage || !result)
        return result;
    GameData* data = result->getState(GAMESTATE_BUILDING);
    if (!data)
        return result;
    Reserve stored;
    {
        StateLock guard;
        const Deposits::const_iterator entry = deposits.find(building);
        if (entry != deposits.end())
            stored = entry->second.reserve;
    }
    try {
        if (stored.share < 1.0 || stored.closed) {
            data->add(ShareKey, static_cast<int>(stored.share * ShareScale + 0.5), std::string(), false);
            data->add(UpdateKey, static_cast<int>(stored.updated * 60), std::string(), false);
            data->add(MinedKey, stored.lastMined < 0 ? -1 : static_cast<int>(stored.lastMined * 60),
                      std::string(), false);
            data->add(ClosedKey, stored.closed ? 1 : 0, std::string(), false);
        } else {
            data->idata.erase(ShareKey);
            data->idata.erase(UpdateKey);
            data->idata.erase(MinedKey);
            data->idata.erase(ClosedKey);
        }
    } catch (...) {
        ReportStorageError();
    }
    return result;
}

void (*nativeLoad)(ProductionBuilding*, GameSaveState*) = NULL;
void LoadDeposit(ProductionBuilding* building, GameSaveState* state)
{
    nativeLoad(building, state);
    if (!ready)
        return;
    const GameData* data = state ? state->getState(GAMESTATE_BUILDING) : NULL;
    if (!data)
        return;
    const auto share = data->idata.find(ShareKey);
    if (share == data->idata.end())
        return;
    const auto update = data->idata.find(UpdateKey);
    const auto mined = data->idata.find(MinedKey);
    const auto closed = data->idata.find(ClosedKey);
    if (share->second < 0 || share->second > ShareScale ||
        update == data->idata.end() || update->second < 0 ||
        mined == data->idata.end() || mined->second < -1 || mined->second > update->second ||
        closed == data->idata.end() || (closed->second != 0 && closed->second != 1)) {
        ErrorLog("FiniteOreVeins: invalid vein reserve in save");
        return;
    }
    Deposit entry(update->second / 60.0);
    entry.reserve.share = share->second / static_cast<double>(ShareScale);
    entry.reserve.lastMined = mined->second < 0 ? -1 : mined->second / 60.0;
    entry.reserve.closed = closed->second != 0;
    try {
        StateLock guard;
        deposits.erase(building);
        deposits.insert(std::make_pair(building, entry));
    } catch (...) {
        ReportStorageError();
    }
}

bool (*nativeNeedsSaving)(const Building*, const std::string&) = NULL;
bool NeedsSaving(const Building* building, const std::string& mod)
{
    if (nativeNeedsSaving(building, mod))
        return true;
    if (!ready || !building->isFoliage || building->isNestItem || !mod.empty())
        return false;
    {
        StateLock guard;
        if (deposits.find(building) == deposits.end())
            return false;
    }
    InstanceID* const id = const_cast<Building*>(building)->getInstanceID();
    // Generated veins receive a serialisable instance ID on the first save.
    return id && id->needsSaving(mod);
}

struct PanelAddress : ProductionBuilding {
    using ProductionBuilding::_NV_getGUIFertility;
};

struct InspectablePanel : DatapanelGUI {
    static bool CanAddRows(const DatapanelGUI& panel)
    {
        return (panel.*&InspectablePanel::scrollView) != NULL;
    }
};

void (*nativePanel)(ProductionBuilding*, DatapanelGUI*, int) = NULL;
void ShowOre(ProductionBuilding* building, DatapanelGUI* panel, int category)
{
    nativePanel(building, panel, category);
    const int buildingResourceCategory = 6;
    // The HUD displays this page with the resource grade and worker count.
    // Hover tooltips can request the same data without a scroll view.
    // DataPanelLine::createMe dereferences it when creating a custom row.
    if (!ready || category != buildingResourceCategory || !panel || !InspectablePanel::CanAddRows(*panel))
        return;
    if (!IsManagedOre(building)) {
        panel->removeLine(RowKey, category);
        return;
    }
    Reserve shown;
    unsigned capacity;
    {
        StateLock guard;
        const Deposits::const_iterator entry = deposits.find(building);
        capacity = entry != deposits.end() && entry->second.capacity ? entry->second.capacity : VeinCapacity(building);
        if (entry != deposits.end())
            shown = entry->second.reserve;
    }
    char value[32];
    const unsigned left = Units(shown, capacity);
    sprintf_s(value, "%u / %u", left, capacity);
    std::string content(value);
    if (!left) {
        content += " - ";
        content += ::Tr("Depleted");
    }
    const bool newRow = !panel->lineExists(RowKey, category);
    DataPanelLine* const row = panel->setLine(RowKey, ::Tr("Ore remaining"), content, category, false, true);
    if (newRow && row)
        row->setToolTipMainBar(::Tr("Ore still in the vein / total capacity. Each mined unit reduces the reserve by one."), false);
}

struct DestructionAddress : GameWorld {
    using GameWorld::processKillList;
};

bool (*nativeFoliage)(FoliageSystem*, FoliageSystem::EntData*, const Ogre::Vector3&,
                      const Ogre::Degree&, const float&, Forests::TreeLoader3D*,
                      Building*, const Ogre::Matrix4&) = NULL;
const Building* volatile excludedVisual = NULL;
volatile LONG skippedVisual = 0;

bool PlaceFoliage(FoliageSystem* system, FoliageSystem::EntData* data,
                  const Ogre::Vector3& position, const Ogre::Degree& angle, const float& scale,
                  Forests::TreeLoader3D* loader, Building* building, const Ogre::Matrix4& transform)
{
    if (building && building == excludedVisual) {
        InterlockedExchange(&skippedVisual, 1);
        return true;
    }
    return nativeFoliage(system, data, position, angle, scale, loader, building, transform);
}

bool ClearStaleMiningTool(Character& worker, GameData* tool)
{
    Inventory* const inventory = worker.getInventory();
    AppearanceBase* const appearance = worker.getAppearance();
    if (!tool || !appearance || (inventory && inventory->hasItem(tool, 1)))
        return false;

    static const std::string hands("hands");
    const AttachedEntity* const attachment = appearance->getAttachedEntity(hands);
    if (!attachment)
        return false;
    // Both admitted 1.0.65 executables store the model's GameData here.
    // AppearanceBase::detachItem(Item*) checks this same member.
    const GameData* attachedData = NULL;
    const size_t gameDataOffset = 0x78;
    std::memcpy(&attachedData, reinterpret_cast<const char*>(attachment) + gameDataOffset, sizeof(attachedData));
    return attachedData == tool && appearance->detachItem(hands);
}

void EndMining(GameWorld& world, ProductionBuilding& vein, std::set<hand>& workers)
{
    workers.insert(vein.currentOperators.begin(), vein.currentOperators.end());
    static const std::string toolKey("special tool");
    GameData* const tool = vein.functionalityData ?
        vein.functionalityData->getFromListAsData(toolKey, 0, &world.gamedata, ITEM) : NULL;
    unsigned finished = 0;
    unsigned cleared = 0;
    for (std::set<hand>::const_iterator it = workers.begin(); it != workers.end(); ++it) {
        Character* const worker = it->getCharacter();
        CharBody* const body = worker ? worker->getBody() : NULL;
        Tasker* const action = body ? body->getCurrentAction() : NULL;
        if (action && (action->subject.getBuilding() == &vein || action->currentSubTarget.getBuilding() == &vein)) {
            body->endAction();
            ++finished;
        }
        if (vein.currentOperators.find(*it) != vein.currentOperators.end())
            vein.stopOperating(*it);
        if (body && !body->getCurrentAction() && ClearStaleMiningTool(*worker, tool))
            ++cleared;
    }
    if (detailedLog) {
        char message[128];
        sprintf_s(message, "FiniteOreVeins: vein cleanup: workers=%u, actions ended=%u, props detached=%u",
                  static_cast<unsigned>(workers.size()), finished, cleared);
        DebugLog(message);
    }
}

bool DropOutput(ProductionBuilding& vein)
{
    const StorageBuilding::ConsumptionItem* const output = vein.getProductionItem();
    InventorySection* const section = output ? output->inventorySection : NULL;
    if (!section || !output->item)
        return false;
    Ogre::Vector3 position = vein.getPosition();
    position.y += 5.0f;
    const hand outside;
    while (!section->getItems().empty()) {
        Item* const item = section->getItems().back().item;
        if (!item || item->getGameData() != output->item || !section->removeItem(item))
            return false;
        item->setInventoryWeAreIn(outside);
        item->notifyOutdoors();
        item->activate(true, position, Ogre::Quaternion::IDENTITY, false, YesNoMaybe::MAYBE, false);
        position.x += 7.0f;
    }
    return true;
}

// Called with the state lock held. Store handles only across game updates.
void QueueRemoval(ProductionBuilding& vein, Deposit& deposit)
{
    const hand target = vein.getHandle();
    if (deposit.removalQueued || target.isNull())
        return;
    try {
        ExhaustedVein job;
        job.target = target;
        job.workers.insert(vein.currentOperators.begin(), vein.currentOperators.end());
        pendingRemovals.push_back(ExhaustedVein());
        pendingRemovals.back().target = target;
        pendingRemovals.back().workers.swap(job.workers);
        deposit.removalQueued = true;
        InterlockedExchange(&removalPending, 1);
    } catch (...) {
        ReportStorageError();
    }
}

void (*nativeKillList)(GameWorld*, bool) = NULL;
void ProcessOreRemoval(GameWorld* world, bool forceImmediate)
{
    // Rebuilding the visuals is the expensive step. Process one vein per update.
    if (!forceImmediate && removalPending) {
        ExhaustedVein job;
        {
            StateLock guard;
            if (!pendingRemovals.empty()) {
                job.target = pendingRemovals.front().target;
                job.workers.swap(pendingRemovals.front().workers);
                pendingRemovals.pop_front();
            }
            InterlockedExchange(&removalPending, !pendingRemovals.empty());
        }
        Building* const building = job.target.getBuilding();
        if (building && !enabled) {
            StateLock guard;
            const Deposits::iterator deposit = deposits.find(building);
            if (deposit != deposits.end())
                deposit->second.removalQueued = false;
        } else if (building && building->isFoliage && !building->isNestItem) {
            ProductionBuilding* const vein = static_cast<ProductionBuilding*>(building);
            try {
                EndMining(*world, *vein, job.workers);
                if (DropOutput(*vein)) {
                    InterlockedExchange(&skippedVisual, 0);
                    ZoneMap* const zone = world->zoneMgr ? world->zoneMgr->getZoneMap(vein->getPosition()) : NULL;
                    ZoneMapContent* const content = zone ? zone->mapContent : NULL;
                    if (content && content->getFoliageSystem()) {
                        excludedVisual = building;
                        content->_reloadAllFoliage();
                        excludedVisual = NULL;
                    }
                    if (!skippedVisual)
                        ErrorLog("FiniteOreVeins: vein not found during sector visual rebuild");
                    if (job.target.getBuilding() == building) {
                        PlayVeinBreak(*building);
                        world->dynamicDestroyBuilding(job.target);
                    }
                } else {
                    ErrorLog("FiniteOreVeins: could not release vein output before deletion");
                }
            } catch (...) {
                excludedVisual = NULL;
                ErrorLog("FiniteOreVeins: vein cleanup failed");
            }
        }
    }
    nativeKillList(world, forceImmediate);
}

} // namespace

bool IsManagedOre(ProductionBuilding* building)
{
    return ready && enabled && building && building->isFoliage && !building->isNestItem && IsNaturalOre(building);
}

bool BeginOreOutput(ProductionBuilding* building)
{
    const double now = GameHours();
    if (now < 0)
        return false;
    try {
        StateLock guard;
        Deposits::iterator entry = deposits.find(building);
        if (entry == deposits.end())
            entry = deposits.insert(std::make_pair(building, Deposit(now))).first;
        Deposit& deposit = entry->second;
        if (deposit.outputPending)
            return false;
        if (!deposit.capacity) {
            deposit.capacity = VeinCapacity(building);
            // Older saves may still be waiting for the reopening threshold.
            deposit.reserve.closed = !Units(deposit.reserve, deposit.capacity);
        }
        if (deposit.reserve.closed) {
            QueueRemoval(*building, deposit);
            return false;
        }
        deposit.outputPending = true;
        return true;
    } catch (...) {
        ReportStorageError();
        return false;
    }
}

void EndOreOutput(ProductionBuilding* building, bool produced)
{
    double now = produced ? GameHours() : -1;
    bool changed = false;
    {
        StateLock guard;
        const Deposits::iterator entry = deposits.find(building);
        if (entry == deposits.end() || !entry->second.outputPending)
            return;
        Deposit& deposit = entry->second;
        if (now < deposit.reserve.updated)
            now = deposit.reserve.updated;
        if (produced)
            changed = Spend(deposit.reserve, deposit.capacity, now);
        deposit.outputPending = false;
        if (changed && deposit.reserve.closed)
            QueueRemoval(*building, deposit);
    }
    if (changed) {
        InstanceID* const id = building->getInstanceID();
        if (id)
            id->notifyChange();
    }
}

} // namespace finite_ore

extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    using namespace finite_ore;
    static const std::string ini = PluginSettings::Path();
    api->beginMod(api, Section, "Finite Ore Veins (HARD)", ini.c_str(), &LoadSettings);
    api->section(api, ::Tr("Ore deposits"));
    api->toggle(api, Section, "Enabled", ::Tr("Enable plugin"),
                ::Tr("Each natural vein holds 1500-3500 units of ore per worker slot."), 1, 0);
    api->toggle(api, Section, "Debug", ::Tr("Detailed log"),
                ::Tr("Writes diagnostic messages for the plugin. Errors are always logged."), 0, 0);
}

__declspec(dllexport) void startPlugin()
{
    using namespace finite_ore;
    const KenshiLib::BinaryVersion version = KenshiLib::GetKenshiVersion();
    if (!(version == KenshiLib::BinaryVersion(KenshiLib::BinaryVersion::STEAM, "1.0.65")) &&
        !(version == KenshiLib::BinaryVersion(KenshiLib::BinaryVersion::GOG, "1.0.65"))) {
        ErrorLog("FiniteOreVeins: unsupported game version");
        return;
    }
    LoadSettings();
    if (!InstallOutputHook())
        return;
    InitializeVeinSound();
    StartupHooks::Hook hooks[] = {
        StartupHooks::Hook(KenshiLib::GetRealAddress(&BuildingAccess::_DESTRUCTOR),
                           DestroyBuilding, &nativeDestroy, "building destruction"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&GameWorld::resetGame),
                           ResetWorld, &nativeReset, "world reset"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&ProductionBuilding::_NV_serialise),
                           SaveDeposit, &nativeSerialize, "vein persistence"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&ProductionBuilding::_NV_loadFromSerialise),
                           LoadDeposit, &nativeLoad, "vein restore"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&Building::_NV_needsSaving),
                           NeedsSaving, &nativeNeedsSaving, "vein save eligibility"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&PanelAddress::_NV_getGUIFertility),
                           ShowOre, &nativePanel, "vein information"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&DestructionAddress::processKillList),
                           ProcessOreRemoval, &nativeKillList, "vein deletion"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&FoliageSystem::finalMakeEnt),
                           PlaceFoliage, &nativeFoliage, "vein visual removal")
    };
    StartupHooks::Install(hooks, [](void* target, void* detour, void** original) {
        return KenshiLib::AddHook(target, detour, original) == KenshiLib::SUCCESS;
    });
    bool all = true;
    for (unsigned i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i) {
        if (!hooks[i].installed) {
            ErrorLog(std::string("FiniteOreVeins: cannot install ") + hooks[i].name);
            all = false;
        }
    }
    if (all)
        InterlockedExchange(&ready, 1);
}
