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
#include <float.h>
#include <map>

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Building/ProductionBuilding.h>
#include <kenshi/GameSaveState.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Kenshi.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/util/TimeOfDay.h>

#include "FiniteOreVeins.h"
#include "ReserveRules.h"

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
    explicit Deposit(double now) : reserve(now), capacity(0), outputPending(false) {}
    Reserve reserve;
    unsigned capacity;
    bool outputPending;
};

typedef std::map<const Building*, Deposit> Deposits;
Deposits deposits;
SRWLOCK lock = SRWLOCK_INIT;
Rules rules;
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

double IniNumber(const std::string& path, const char* key, const char* fallback, double low, double high)
{
    const double value = std::atof(PluginSettings::Text(Section, key, fallback, path).c_str());
    if (_isnan(value))
        return std::atof(fallback);
    return value < low ? low : value > high ? high : value;
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
    Rules next;
    next.refillDays = IniNumber(path, "RefillDays", "10", 0, 365);
    next.restHours = IniNumber(path, "RestHours", "12", 0, 240);
    next.reopenPercent = static_cast<unsigned>(IniNumber(path, "ReopenPercent", "25", 0, 100) + 0.5);
    const bool debug = IniToggle(path, "Debug", false);
    const bool active = IniToggle(path, "Enabled", true);
    KLocLog::SetEnabled(debug);
    {
        StateLock guard;
        rules = next;
    }
    InterlockedExchange(&enabled, active);
    if (debug) {
        char message[192];
        sprintf_s(message, "FiniteOreVeins: enabled=%u, 1500-3500 units per worker slot, refill=%.1f days, delay=%.1f hours, reopen=%u%%",
                  active ? 1u : 0u, next.refillDays,
                  next.restHours, next.reopenPercent);
        DebugLog(message);
    }
}

void ReportStorageError()
{
    if (!InterlockedExchange(&storeError, 1))
        ErrorLog("FiniteOreVeins: unable to store a vein reserve");
}

void (*nativeReset)(GameWorld*) = NULL;
void ResetWorld(GameWorld* world)
{
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
    const double now = GameHours();
    Reserve stored;
    {
        StateLock guard;
        const Deposits::const_iterator entry = deposits.find(building);
        if (entry != deposits.end()) {
            stored = entry->second.reserve;
            if (now >= 0) {
                const unsigned capacity = entry->second.capacity ? entry->second.capacity : VeinCapacity(building);
                Refresh(stored, rules, capacity, now);
            }
        }
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
    const double now = GameHours();
    if (now < 0)
        return;
    Reserve shown(now);
    unsigned capacity;
    bool permanentlyDry;
    {
        StateLock guard;
        const Deposits::const_iterator entry = deposits.find(building);
        capacity = entry != deposits.end() && entry->second.capacity ? entry->second.capacity : VeinCapacity(building);
        if (entry != deposits.end()) {
            shown = entry->second.reserve;
            Refresh(shown, rules, capacity, now);
        }
        permanentlyDry = rules.refillDays <= 0 && shown.closed;
    }
    char value[32];
    sprintf_s(value, "%u / %u", Units(shown, capacity), capacity);
    std::string content(value);
    if (shown.closed) {
        content += " - ";
        content += ::Tr(permanentlyDry ? "Depleted" : "Recovering");
    }
    panel->setLine(RowKey, ::Tr("Ore remaining"), content, category, false, true);
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
        if (!deposit.capacity)
            deposit.capacity = VeinCapacity(building);
        Refresh(deposit.reserve, rules, deposit.capacity, now);
        if (deposit.reserve.closed || !Units(deposit.reserve, deposit.capacity))
            return false;
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
    api->section(api, ::Tr("Recovery"));
    api->number(api, Section, "RefillDays", ::Tr("Days to refill"),
                ::Tr("Game days to refill after the delay. 0 disables recovery."), 10, 0, 365, 1, 0);
    api->number(api, Section, "RestHours", ::Tr("Hours before recovery"),
                ::Tr("Delay after the last unit was mined, in game hours."), 12, 0, 240, 1, 0);
    api->integer(api, Section, "ReopenPercent", ::Tr("Reopen at (%)"),
                 ::Tr("An exhausted vein stays closed until this share is restored. 0 reopens at one unit."), 25, 0, 100, 0);
    api->section(api, ::Tr("Diagnostics"));
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
                           ShowOre, &nativePanel, "vein information")
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
