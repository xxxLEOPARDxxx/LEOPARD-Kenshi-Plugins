#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define KLOC_DOMAIN "extra_durability"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <IniDefaults.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <float.h>
#include <map>
#include <vector>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Animation/AnimationClass.h>
#include <kenshi/Building/FurnaceBuilding.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Gear.h>
#include <kenshi/Globals.h>
#include <kenshi/Inventory.h>
#include <kenshi/Kenshi.h>
#include <kenshi/util/StringPair.h>
#include <mygui/MyGUI_TextIterator.h>

#include "BreakageNotice.h"
#include "ConditionRules.h"
#include "ExtraDurability.h"
#include "GearState.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace durability
{
namespace
{
const char Section[] = "ExtraDurability";
const char MeitouId[] = "52293-rebirth.mod";
enum Flags { Enabled = 1, Salvage = 2, Debug = 4, NpcWear = 8, DestroyBroken = 16, BreakNotices = 32 };
volatile LONG flags = Enabled | Salvage | NpcWear | DestroyBroken | BreakNotices;
volatile LONG barPosition = 1;

enum TuningKey {
    ProtectionGrade, MeleeWear, CrossbowWear, ArmourWear,
    WeaponPenaltyStart, WeaponPenaltyEnd, WeaponMinimumEffect,
    ArmourPenaltyStart, ArmourPenaltyEnd, ArmourMinimumEffect,
    Capacity1, Capacity2, Capacity3, Capacity4, Capacity5, Capacity6, Capacity7, Capacity8, Capacity9,
    RepairPortion, RepairMinimum, RepairEfficiencyBonus, RepairDuration, RepairSpeedBonus, RepairExperience,
    SalvageMinimum, TuningCount
};

struct TuningOption {
    const char* key;
    const char* label;
    const char* help;
    int initial;
    int minimum;
    int maximum;
};

const char CapacityHelp[] = "Maximum durability in this tier. Existing wear is stored in points, so changing this value can change the percentage on used gear.";
const TuningOption tuningOptions[] = {
    {"MeitouProtectionGrade", "Breakage protection from grade", "Melee weapons at or above this grade cannot break. 0 protects only vanilla Meitou. Vanilla Meitou is always protected.", 100, 0, 1000},
    {"MeleeWear", "Melee wear per hit or block", "Durability points lost on a registered hit or block. 0 disables new wear from this action.", 8, 0, 1000},
    {"CrossbowWear", "Crossbow wear per shot", "Durability points lost on each shot, including misses. 0 disables new wear from this action.", 8, 0, 1000},
    {"ArmourWear", "Armour wear per hit", "Durability points lost by the armour layer that covers a hit. 0 disables new wear from this action.", 5, 0, 1000},
    {"WeaponPenaltyStart", "Weapon damage starts dropping at (%)", "At higher durability the weapon deals full damage. This setting covers melee weapons and crossbows.", 50, 0, 100},
    {"WeaponPenaltyEnd", "Weapon damage reaches minimum at (%)", "At or below this durability, weapon damage stays at its minimum until breakage. This value is capped at the start of the penalty.", 0, 0, 100},
    {"WeaponMinimumEffect", "Minimum weapon damage (%)", "Share of the original damage at the bottom of the wear range. Broken weapons stop dealing damage. Protected weapons still use this minimum at 0 durability.", 20, 0, 100},
    {"ArmourPenaltyStart", "Armour protection starts dropping at (%)", "At higher durability, normal armour coverage applies.", 50, 0, 100},
    {"ArmourPenaltyEnd", "Armour protection reaches minimum at (%)", "At or below this durability, protection stays at its minimum until breakage. This value is capped at the start of the penalty.", 0, 0, 100},
    {"ArmourMinimumEffect", "Minimum armour effectiveness (%)", "Chance for a layer to stay effective after passing the normal coverage check, at the bottom of the wear range. At 0 durability a broken layer offers no protection.", 20, 0, 100},
    {"DurabilityTier1", "Tier 1 - Rusted / Prototype", CapacityHelp, DefaultConditionCaps[0], 1, 1000000},
    {"DurabilityTier2", "Tier 2 - Mid-Grade / Shoddy", CapacityHelp, DefaultConditionCaps[1], 1, 1000000},
    {"DurabilityTier3", "Tier 3 - Refitted blades", CapacityHelp, DefaultConditionCaps[2], 1, 1000000},
    {"DurabilityTier4", "Tier 4 - Catun No. 1 / Standard", CapacityHelp, DefaultConditionCaps[3], 1, 1000000},
    {"DurabilityTier5", "Tier 5 - Catun No. 2", CapacityHelp, DefaultConditionCaps[4], 1, 1000000},
    {"DurabilityTier6", "Tier 6 - Catun No. 3 / High", CapacityHelp, DefaultConditionCaps[5], 1, 1000000},
    {"DurabilityTier7", "Tier 7 - Mk I to III", CapacityHelp, DefaultConditionCaps[6], 1, 1000000},
    {"DurabilityTier8", "Tier 8 - Edge / Specialist", CapacityHelp, DefaultConditionCaps[7], 1, 1000000},
    {"DurabilityTier9", "Tier 9 - Meitou / Masterwork", CapacityHelp, DefaultConditionCaps[8], 1, 1000000},
    {"RepairPortion", "Restored by a full tool kit (%)", "Base share of maximum durability restored with a full charge, before the smithing bonus.", 25, 0, 100},
    {"RepairMinimum", "Minimum points per full tool kit", "The charge restores at least this many points before the smithing bonus. With both repair amounts set to 0, tools restore nothing.", 500, 0, 1000000},
    {"RepairEfficiencyBonus", "Repair efficiency bonus at skill 100 (%)", "Extra durability a full charge restores at smithing skill 100. The bonus grows with skill.", 50, 0, 500},
    {"RepairDuration", "Full tool kit use time, game seconds", "Time to use a full charge at smithing skill 0, at normal game speed. Smaller repairs use less time and charge.", 8, 1, 120},
    {"RepairSpeedBonus", "Repair speed bonus at skill 100 (%)", "Extra repair speed at smithing skill 100. 100 doubles the speed.", 100, 0, 500},
    {"RepairExperience", "Smithing experience from repairs (%)", "100 awards the usual experience per point restored. 0 disables repair experience.", 100, 0, 500},
    {"SalvageMinimum", "Smelting yield at 0 durability (%)", "Share of the normal materials returned at 0 durability. The yield rises linearly to the normal amount at full durability.", 20, 0, 100},
};
static_assert(sizeof(tuningOptions) / sizeof(tuningOptions[0]) == TuningCount, "One MCM option per tuning value");
volatile LONG tuning[TuningCount];
volatile LONG weaponCurve = DefaultConditionCurve;
volatile LONG armourCurve = DefaultConditionCurve;

void ReadMcmIniValue(const char* key, const char* fallback, const std::string& ini, char (&value)[8193])
{
    GetPrivateProfileStringA(Section, key, fallback, value, sizeof(value), ini.c_str());
    char* end = std::strchr(value, ';');
    if (!end)
        end = value + std::strlen(value);
    while (end != value && (end[-1] == ' ' || end[-1] == '\t'))
        --end;
    *end = '\0';
}

LONG ReadTuning(const TuningOption& option, const std::string& ini)
{
    if (ini.empty())
        return option.initial;
    char fallback[32];
    sprintf_s(fallback, "%d", option.initial);
    char value[8193];
    ReadMcmIniValue(option.key, fallback, ini, value);
    const float amount = static_cast<float>(std::atof(value));
    if (_isnan(amount))
        return option.initial;
    if (amount < option.minimum)
        return option.minimum;
    if (amount > option.maximum)
        return option.maximum;
    return static_cast<LONG>(std::floor(amount + 0.5f));
}

bool ReadMcmToggle(const char* key, bool fallback, const std::string& ini)
{
    char value[8193];
    ReadMcmIniValue(key, fallback ? "1" : "0", ini, value);
    return std::atoi(value) != 0 || _stricmp(value, "true") == 0;
}

LONG ReadBarPosition(const std::string& ini)
{
    char value[8193];
    ReadMcmIniValue("WeaponConditionBar", "1", ini, value);
    return value[0] >= '0' && value[0] <= '2' && value[1] == '\0' ? value[0] - '0' : 1;
}

std::string IniPath()
{
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(
        reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return std::string();
    const std::string dll(path, length);
    const size_t slash = dll.find_last_of("\\/");
    return slash == std::string::npos
        ? std::string() : IniDefaults::Prepare(dll.substr(0, slash + 1) + "ExtraDurability.ini");
}

void LoadSettings()
{
    const std::string ini = IniPath();
    LONG values[TuningCount];
    for (unsigned i = 0; i < TuningCount; ++i) {
        values[i] = ReadTuning(tuningOptions[i], ini);
        InterlockedExchange(&tuning[i], values[i]);
    }
    InterlockedExchange(&weaponCurve, PackConditionCurve(values[WeaponPenaltyStart], values[WeaponPenaltyEnd], values[WeaponMinimumEffect]));
    InterlockedExchange(&armourCurve, PackConditionCurve(values[ArmourPenaltyStart], values[ArmourPenaltyEnd], values[ArmourMinimumEffect]));
    if (ini.empty())
        return;
    LONG next = 0;
    if (ReadMcmToggle("Enabled", true, ini)) next |= Enabled;
    if (ReadMcmToggle("SalvageCondition", true, ini)) next |= Salvage;
    if (ReadMcmToggle("DestroyBroken", true, ini)) next |= DestroyBroken;
    if (ReadMcmToggle("BreakNotices", true, ini)) next |= BreakNotices;
    if (ReadMcmToggle("NpcWear", true, ini)) next |= NpcWear;
    if (ReadMcmToggle("Debug", false, ini)) next |= Debug;
    KLocLog::SetEnabled((next & Debug) != 0);
    InterlockedExchange(&flags, next);
    InterlockedExchange(&barPosition, ReadBarPosition(ini));
}

EquipmentKind Kind(const Item* item)
{
    if (!item || !item->data)
        return Unsupported;
    switch (item->data->type) {
    case WEAPON: return Melee;
    case CROSSBOW: return Ranged;
    case ARMOUR: return ArmourGear;
    default: return Unsupported;
    }
}

bool UsesStockMeitouGrade(const Gear* item, EquipmentKind kind)
{
    return kind == Melee && item->materialData && item->materialData->stringID == MeitouId;
}

bool IsProtected(const Gear* item, EquipmentKind kind)
{
    return kind == Melee && ProtectedMeleeGrade(kind, item->level_0_100, UsesStockMeitouGrade(item, kind),
                                                 tuning[ProtectionGrade]);
}

unsigned Maximum(const Gear* item, EquipmentKind kind)
{
    return kind == Unsupported ? 0 : MaxCondition(kind, item->level_0_100, UsesStockMeitouGrade(item, kind), &tuning[Capacity1]);
}

bool EnabledNow()
{
    return (flags & Enabled) != 0 && GearStateReady();
}

bool Breaks(EquipmentKind kind, const Gear* item)
{
    return CanBreak(kind, IsProtected(item, kind));
}

bool CanWearCharacter(Character* character, LONG current)
{
    return character && ((current & NpcWear) != 0 || character->isPlayerCharacter());
}

Inventory* Backpack(Character* character)
{
    ContainerItem* bag = character->hasABackpackOn();
    return bag ? bag->getInventory() : NULL;
}

StatsEnumerated Smithing(EquipmentKind kind)
{
    if (kind == ArmourGear) return STAT_SMITHING_ARMOUR;
    if (kind == Ranged) return STAT_SMITHING_BOW;
    return STAT_SMITHING_WEAPON;
}

void Message(const char* message)
{
    if (ou)
        ou->showPlayerAMessage(::Tr(message), false);
}

bool SpendKit(Inventory* inventory, GameData* kit, Item* stack)
{
    const int before = inventory->countItems(kit);
    if (before <= 0)
        return false;
    // Partial stack removal can return false even after spending the unit.
    inventory->removeItemAutoDestroy(stack, 1);
    return inventory->countItems(kit) == before - 1;
}


struct RepairJob
{
    hand character;
    hand gear;
    hand kit;
    Ogre::Vector3 start;
    std::string animation;
    const AnimationClassBase::SingleAnimation* playback;
    double work;

    RepairJob(Character* worker, Gear* target, Item* tool)
        : character(worker->handle), gear(target->handle), kit(tool->handle),
          start(worker->getPosition()), playback(NULL), work(0.0) {}
};

std::vector<RepairJob> jobs;
double repairTick = 0.0;
volatile LONG worldGeneration = 0;
LONG repairGeneration = 0;
bool equipmentLoopReady = false;
void (*mainLoop)(GameWorld*, float) = NULL;
void (*clearWorld)(GameWorld*) = NULL;

struct BreakageEvent
{
    hand holder;
    std::string characterName;
    std::string itemName;
    bool armour;
    bool notify;
    bool playerPending;

    BreakageEvent(const hand& owner, bool isArmour)
        : holder(owner), armour(isArmour), notify(false), playerPending(false) {}
};

typedef std::map<hand, BreakageEvent> BreakageEvents;
SRWLOCK breakageLock = SRWLOCK_INIT;
BreakageEvents pendingBreakage;
enum BreakageQueueState { BreakageClosed = -1, BreakageEmpty = 0, BreakagePending = 1 };
volatile LONG breakageState = BreakageEmpty;

struct BreakageLock
{
    BreakageLock() { AcquireSRWLockExclusive(&breakageLock); }
    ~BreakageLock() { ReleaseSRWLockExclusive(&breakageLock); }
private:
    BreakageLock(const BreakageLock&);
    BreakageLock& operator=(const BreakageLock&);
};

void QueueBreakage(Gear* item, EquipmentKind kind, unsigned before, unsigned remaining,
                   bool loaded = false, bool playerPending = false)
{
    if (remaining || !equipmentLoopReady)
        return;
    const LONG current = flags;
    const bool destroy = (current & DestroyBroken) != 0;
    const bool notify = (current & BreakNotices) != 0 && before;
    if (!CanQueueBrokenEquipment(breakageState == BreakageClosed, (current & Enabled) != 0,
                                 destroy, notify, loaded))
        return;
    const bool meitou = IsProtected(item, kind);
    if (!CanBreak(kind, meitou) || item->handle.isNull())
        return;

    const LONG generation = worldGeneration;
    try {
        // The native inventory may not have attached a restored item yet.
        BreakageEvent event(loaded ? hand() : item->getInventoryWeAreIn(), kind == ArmourGear);
        event.playerPending = playerPending;
        Character* wearer = notify ? event.holder.getCharacter() : NULL;
        event.notify = BreakNotice(kind, meitou, before, remaining, notify,
                                   wearer && wearer->isPlayerCharacter());
        if (event.notify) {
            event.characterName = wearer->getName();
            event.itemName = item->getName();
        } else if (!destroy) {
            return;
        }

        BreakageLock lock;
        if (worldGeneration != generation || (breakageState == BreakageClosed && !loaded))
            return;
        const BreakageEvents::iterator queued =
            pendingBreakage.insert(std::make_pair(item->handle, event)).first;
        if (queued->second.holder.isNull() && !event.holder.isNull())
            queued->second.holder = event.holder;
        queued->second.playerPending |= event.playerPending;
        if (event.notify && !queued->second.notify) {
            queued->second.characterName.swap(event.characterName);
            queued->second.itemName.swap(event.itemName);
            queued->second.notify = true;
        }
        if (breakageState != BreakageClosed)
            InterlockedExchange(&breakageState, BreakagePending);
    } catch (...) {
        ErrorLog("ExtraDurability: could not queue broken equipment");
    }
}

void ShowBreakage(GameWorld* world, const BreakageEvent& event)
{
    if (!event.notify || !(flags & Enabled) || !(flags & BreakNotices))
        return;
    try {
        const char* format = event.armour
            ? TrFmt("#FF6363Armour on #8BDD70%s#FF6363 broke - %s")
            : TrFmt("#FF6363Weapon on #8BDD70%s#FF6363 broke - %s");
        const std::string displayName = BreakageNoticeName(event.characterName, KLoc::LowerAscii(KLoc::LoadedLocaleId()));
        const std::string name = MyGUI::TextIterator::toTagsString(MyGUI::UString(displayName)).asUTF8();
        const std::string item = MyGUI::TextIterator::toTagsString(MyGUI::UString(event.itemName)).asUTF8();
        const int length = _scprintf(format, name.c_str(), item.c_str());
        if (length <= 0)
            return;
        std::vector<char> message(static_cast<size_t>(length) + 1);
        sprintf_s(&message[0], message.size(), format, name.c_str(), item.c_str());
        world->showPlayerAMessage(&message[0], true);
    } catch (...) {
        ErrorLog("ExtraDurability: could not show equipment breakage notice");
    }
}


void DestroyBrokenItem(GameWorld* world, const hand& identity, const BreakageEvent& event)
{
    const LONG current = flags;
    if (!(current & Enabled) || !(current & DestroyBroken))
        return;
    Item* item = identity.getItem();
    Gear* gear = item && item->quantity == 1 ? item->isGear() : NULL;
    const EquipmentKind kind = Kind(gear);
    const unsigned full = Maximum(gear, kind);
    if (!full || !DestroyAtZero(kind, IsProtected(gear, kind), ReadCondition(gear, full), true))
        return;

    bool removed = false;
    if (item->isInInventory) {
        const hand& holder = item->getInventoryWeAreIn();
        Item* bag = holder.getItem();
        if (!(current & NpcWear)) {
            Character* owner = bag ? bag->getInventoryWeAreIn().getCharacter() : holder.getCharacter();
            if (owner ? !owner->isPlayerCharacter() :
                !event.playerPending && !CanWearCharacter(event.holder.getCharacter(), current))
                return;
        }
        RootObject* owner = bag ? bag : holder.getRootObject();
        Inventory* inventory = owner ? owner->getInventory() : NULL;
        // The native removal path detaches equipped items before destruction.
        if (inventory && inventory->hasItem(item))
            removed = inventory->removeItemAutoDestroy(item, 1);
        else
            return;
    } else if (item->onGround() && !item->isEquipped &&
               ((current & NpcWear) || event.playerPending || CanWearCharacter(event.holder.getCharacter(), current))) {
        // A false native result can mean deletion was queued by the engine.
        world->destroy(item, false, "ExtraDurability");
        if (current & Debug)
            DebugLog("ExtraDurability: requested destruction of dropped broken equipment");
        return;
    } else {
        return;
    }
    if (!removed)
        ErrorLog("ExtraDurability: game refused to destroy broken equipment");
    else if (current & Debug)
        DebugLog("ExtraDurability: destroyed broken equipment");
}

LONGLONG BreakageBudgetTicks()
{
    LARGE_INTEGER frequency;
    return QueryPerformanceFrequency(&frequency) ? frequency.QuadPart / 1000 : 0;
}

void DrainBreakage(GameWorld* world)
{
    static bool draining = false;
    if (breakageState != BreakagePending || draining)
        return;
    struct DrainScope {
        bool* active;
        explicit DrainScope(bool& value) : active(&value) { *active = true; }
        ~DrainScope() { *active = false; }
    } scope(draining);

    static const LONGLONG budgetTicks = BreakageBudgetTicks();
    LARGE_INTEGER start = {};
    const bool timed = budgetTicks > 0 && QueryPerformanceCounter(&start);
    const LONG generation = worldGeneration;
    const unsigned maxPerFrame = 16;
    for (unsigned count = 0; count < maxPerFrame; ++count) {
        hand identity = hand();
        BreakageEvent event(identity, false);
        {
            BreakageLock lock;
            if (breakageState != BreakagePending || worldGeneration != generation)
                return;
            const BreakageEvents::iterator it = pendingBreakage.begin();
            identity = it->first;
            event.holder = it->second.holder;
            event.armour = it->second.armour;
            event.notify = it->second.notify;
            event.playerPending = it->second.playerPending;
            event.characterName.swap(it->second.characterName);
            event.itemName.swap(it->second.itemName);
            pendingBreakage.erase(it);
            if (pendingBreakage.empty())
                InterlockedExchange(&breakageState, BreakageEmpty);
        }

        if (worldGeneration != generation)
            return;
        DestroyBrokenItem(world, identity, event);
        if (worldGeneration != generation)
            return;
        ShowBreakage(world, event);
        LARGE_INTEGER now;
        if (breakageState != BreakagePending ||
            (timed && QueryPerformanceCounter(&now) && now.QuadPart - start.QuadPart >= budgetTicks))
            return;
    }
}


bool CanRepair(Character* worker)
{
    return worker && worker->isPlayerCharacter() && !worker->isDead() &&
        !worker->isUnconcious() && !worker->isInCombatMode(true, true);
}

Inventory* FindInventory(Character* worker, Item* item)
{
    Inventory* carried = worker->getInventory();
    if (carried && carried->hasItem(item))
        return carried;
    Inventory* backpack = Backpack(worker);
    return backpack && backpack->hasItem(item) ? backpack : NULL;
}

void SyncRepairWorld()
{
    const LONG generation = worldGeneration;
    if (repairGeneration != generation) {
        jobs.clear();
        repairTick = 0.0;
        repairGeneration = generation;
    }
}

void StartRepairAnimation(RepairJob& job, Character* worker)
{
    if (!job.animation.empty())
        return;
    AnimationClass* animation = worker->getAnimationClass();
    if (!animation || animation->hasAction())
        return;
    AnimationData* data = animation->getAnimation_Medic(worker->inWhat);
    if (!data || data->dataName.empty())
        return;
    job.animation = data->dataName;
    animation->playAction(data, 1.0f, 0.0f, false);
    job.playback = animation->getAnimationPlaying(data);
    if (!job.playback) {
        job.animation.clear();
        return;
    }
    worker->audioEvent("Character_Medic_Start", SOUNDRANGE_ALWAYS);
}

void StopRepairAnimation(const RepairJob& job)
{
    if (job.animation.empty())
        return;
    Character* worker = job.character.getCharacter();
    AnimationClass* animation = worker ? worker->getAnimationClass() : NULL;
    const AnimationClassBase::SingleAnimation* current = animation
        ? animation->getAnimationPlaying_datName(job.animation) : NULL;
    if (current && current == job.playback)
        animation->stopAction(job.animation);
    if (worker && (!current || current == job.playback))
        worker->audioEvent("Character_Medic_Stop", SOUNDRANGE_ALWAYS);
}


bool RunRepair(RepairJob& job, double time)
{
    Character* worker = job.character.getCharacter();
    if (!CanRepair(worker) || (worker->getPosition() - job.start).squaredLength() > 225.0f) {
        Message("Equipment repair interrupted.");
        return false;
    }
    Item* item = job.gear.getItem();
    Gear* gear = item ? item->isGear() : NULL;
    const EquipmentKind kind = Kind(gear);
    const unsigned full = Maximum(gear, kind);
    Item* kit = job.kit.getItem();
    Inventory* source = kit ? FindInventory(worker, kit) : NULL;
    if (!full || !FindInventory(worker, gear) || !source || !IsRepairTool(kit->data) || !(kit->chargesLeft > 0.0f) ||
        !(kit->originalFullChargeAmount > 0.0f) ||
        kit->chargesLeft > kit->originalFullChargeAmount) {
        Message("Equipment repair interrupted.");
        return false;
    }

    const unsigned current = ReadCondition(gear, full);
    if (current >= full)
        return false;

    const StatsEnumerated stat = Smithing(kind);
    CharStats* stats = worker->getStats();
    const float skill = stats ? stats->getStat(stat, true) : 0.0f;
    const unsigned budget = RepairBudget(full, skill, tuning[RepairPortion], tuning[RepairMinimum], tuning[RepairEfficiencyBonus]);
    if (!budget)
        return false;
    const double pointsPerCharge = static_cast<double>(budget) / kit->originalFullChargeAmount;
    StartRepairAnimation(job, worker);
    job.work += RepairRate(budget, skill, tuning[RepairDuration], tuning[RepairSpeedBonus]) * time;
    const unsigned points = RepairSlice(job.work, full - current, kit->chargesLeft, pointsPerCharge);
    if (!points)
        return true;

    const double chargeSpent = points / pointsPerCharge;
    const double remaining = static_cast<double>(kit->chargesLeft) - chargeSpent;
    const bool exhausted = RepairChargeExhausted(remaining, pointsPerCharge);
    if (exhausted) {
        if (!SpendKit(source, kit->data, kit)) {
            ErrorLog("ExtraDurability: unexpected repair kit debit");
            Message("Could not use the tools.");
            return false;
        }
    } else {
        kit->chargesLeft = static_cast<float>(remaining);
    }

    if (!RestoreCondition(gear, full, points))
        return false;
    const LONG experience = tuning[RepairExperience];
    if (stats && experience)
        stats->xpStat_eventBased(stat, (static_cast<float>(points) / 500.0f) * (experience / 100.0f));
    job.work -= points;
    if (points < full - current && !exhausted)
        return true;

    Message(points >= full - current ? "Equipment repaired." : "Repair tools spent.");
    if (flags & Debug)
        DebugLog("ExtraDurability: equipment repair finished");
    return false;
}

void TickEquipment(GameWorld* world, float time)
{
    mainLoop(world, time);
    TickConditionBars(time);
    if (jobs.empty() && breakageState == BreakageEmpty)
        return;
    // A pending load can be cancelled. The world teardown hook retires
    // the jobs only when the original world is actually cleared.
    if (world->isLoadingFromASaveGame())
        return;
    if (breakageState == BreakageClosed) {
        AcquireSRWLockExclusive(&breakageLock);
        if (breakageState == BreakageClosed)
            InterlockedExchange(&breakageState, pendingBreakage.empty() ? BreakageEmpty : BreakagePending);
        ReleaseSRWLockExclusive(&breakageLock);
    }
    DrainBreakage(world);
    if (jobs.empty())
        return;
    SyncRepairWorld();
    if (!EnabledNow()) {
        for (size_t i = 0; i < jobs.size(); ++i)
            StopRepairAnimation(jobs[i]);
        jobs.clear();
        repairTick = 0.0;
        return;
    }
    if (jobs.empty() || world->isPaused() || !(time > 0.0f))
        return;
    const float speed = world->getFrameSpeedMultiplier();
    if (!(speed > 0.0f) || !_finite(speed))
        return;
    repairTick += static_cast<double>(speed) * (time < 0.25f ? time : 0.25f);
    if (repairTick < 0.15)
        return;
    const double elapsed = repairTick;
    repairTick = 0.0;
    for (size_t i = 0; i < jobs.size();) {
        if (RunRepair(jobs[i], elapsed))
            ++i;
        else {
            StopRepairAnimation(jobs[i]);
            jobs.erase(jobs.begin() + i);
        }
    }
}

void ResetEquipment(GameWorld* world)
{
    AcquireSRWLockExclusive(&breakageLock);
    InterlockedIncrement(&worldGeneration);
    pendingBreakage.clear();
    InterlockedExchange(&breakageState, BreakageClosed);
    ReleaseSRWLockExclusive(&breakageLock);
    clearWorld(world);
}

void InstallEquipmentLoop()
{
    const bool lifecycle = KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&GameWorld::resetGame), ResetEquipment, &clearWorld)
        == KenshiLib::SUCCESS;
    equipmentLoopReady = lifecycle && KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), TickEquipment, &mainLoop)
        == KenshiLib::SUCCESS;
    if (!equipmentLoopReady)
        ErrorLog("ExtraDurability: timed repair and broken item removal hooks unavailable");
}

typedef Ogre::vector<StringPair>::type TooltipLines;

void AddConditionLine(Gear* item, TooltipLines& lines)
{
    if (!EnabledNow())
        return;
    const EquipmentKind kind = Kind(item);
    const unsigned full = Maximum(item, kind);
    if (!full)
        return;
    const unsigned current = ReadCondition(item, full);
    char value[64];
    sprintf_s(value, "%u / %u (%u%%)", current, full, current * 100 / full);
    lines.push_back(StringPair(::Tr("Durability"), value));
    if (!current && Breaks(kind, item)) {
        lines.push_back(StringPair(::Tr("Condition"), ::Tr("Broken")));
        QueueBreakage(item, kind, 0, current);
    }
}

template<class T> struct TooltipHook
{
    static void (*original)(T*, TooltipLines&);
    static void Run(T* item, TooltipLines& lines)
    {
        original(item, lines);
        AddConditionLine(item, lines);
    }
};

template<class T> void (*TooltipHook<T>::original)(T*, TooltipLines&) = NULL;

template<class T> void HookTooltip(void (T::*method)(TooltipLines&), const char* failure)
{
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(method), TooltipHook<T>::Run,
                           &TooltipHook<T>::original) != KenshiLib::SUCCESS)
        ErrorLog(failure);
}

void (*kitTooltip)(InventoryItemBase*, TooltipLines&) = NULL;

void AddKitCharge(InventoryItemBase* item, TooltipLines& lines)
{
    kitTooltip(item, lines);
    if (!EnabledNow() || !IsRepairTool(item->data) ||
        !(item->originalFullChargeAmount > 0.0f))
        return;
    const float ratio = item->chargesLeft / item->originalFullChargeAmount;
    const unsigned percent = !(ratio > 0.0f) ? 0 :
        ratio >= 1.0f ? 100 : static_cast<unsigned>(ratio * 100.0f + 0.5f);
    char value[16];
    sprintf_s(value, "%u%%", percent);
    lines.push_back(StringPair(::Tr("Tools charge"), value));
}


float (*furnaceValue)(FurnaceBuilding*, Item*) = NULL;

float FurnaceValue(FurnaceBuilding* furnace, Item* item)
{
    const float stock = furnaceValue(furnace, item);
    if (!(stock > 0.0f) || !(flags & Salvage) || !EnabledNow() || !item)
        return stock;
    Gear* gear = item->isGear();
    const unsigned full = Maximum(gear, Kind(gear));
    return !full ? stock : stock * SalvageFactor(ReadCondition(gear, full), full, tuning[SalvageMinimum]);
}
}

std::string Tr(const char* english)
{
    return std::string(::Tr(english));
}

bool IsSupported(Gear* item)
{
    return Kind(item) != Unsupported;
}

bool IsDamaged(Gear* item)
{
    if (!EnabledNow())
        return false;
    const unsigned full = Maximum(item, Kind(item));
    return full && ReadCondition(item, full) < full;
}

std::string ConditionLabel(Gear* item)
{
    const unsigned full = Maximum(item, Kind(item));
    if (!full)
        return std::string();
    char value[32];
    sprintf_s(value, "%u / %u", ReadCondition(item, full), full);
    return std::string(value);
}

bool AppliesTo(Character* character)
{
    return EnabledNow() && CanWearCharacter(character, flags);
}

bool HasPendingPlayerDestruction(Gear* item)
{
    const LONG current = flags;
    if (!(current & Enabled) || !(current & DestroyBroken) || breakageState == BreakageEmpty)
        return false;
    hand holder;
    {
        BreakageLock lock;
        const BreakageEvents::const_iterator event = pendingBreakage.find(item->handle);
        if (event == pendingBreakage.end())
            return false;
        if (event->second.playerPending)
            return true;
        holder = event->second.holder;
    }
    Character* wearer = holder.getCharacter();
    return wearer && wearer->isPlayerCharacter();
}

void QueueRestoredBrokenEquipment(Gear* item, bool playerPending)
{
    QueueBreakage(item, Kind(item), 0, 0, true, playerPending);
}

float Effect(Gear* item)
{
    if (!EnabledNow())
        return 1.0f;
    const EquipmentKind kind = Kind(item);
    const unsigned full = Maximum(item, kind);
    if (!full)
        return 1.0f;
    const unsigned current = ReadCondition(item, full);
    return !current && Breaks(kind, item) ? 0.0f : ConditionFactor(current, full, weaponCurve);
}

float ArmourEffectAndWear(Gear* item)
{
    if (!EnabledNow())
        return 1.0f;
    const EquipmentKind kind = Kind(item);
    if (kind != ArmourGear || (!(flags & NpcWear) &&
        !CanWearCharacter(item->getInventoryWeAreIn().getCharacter(), flags)))
        return 1.0f;
    const unsigned full = Maximum(item, kind);
    const unsigned amount = tuning[ArmourWear];
    unsigned before;
    if (amount) {
        const unsigned remaining = WearCondition(item, full, amount, &before);
        QueueBreakage(item, kind, before, remaining);
    } else {
        before = ReadCondition(item, full);
    }
    return before ? ConditionFactor(before, full, armourCurve) : 0.0f;
}


void Wear(Gear* item)
{
    if (!EnabledNow())
        return;
    const EquipmentKind kind = Kind(item);
    if (kind != Melee && kind != Ranged)
        return;
    const unsigned amount = tuning[kind == Melee ? MeleeWear : CrossbowWear];
    if (!amount)
        return;
    const unsigned full = Maximum(item, kind);
    if (!full)
        return;
    unsigned before;
    const unsigned remaining = WearCondition(item, full, amount, &before);
    QueueBreakage(item, kind, before, remaining);
}

int ConditionBarPosition()
{
    return EnabledNow() ? static_cast<int>(barPosition) : 0;
}

bool HasConditionBar(const Item* item)
{
    return Kind(item) != Unsupported;
}

float EquipmentConditionRatio(Item* item)
{
    if (!EnabledNow() || !HasConditionBar(item))
        return -1.0f;
    Gear* gear = item->isGear();
    const unsigned full = Maximum(gear, Kind(gear));
    return full ? VisibleConditionRatio(ReadCondition(gear, full), full) : -1.0f;
}

bool HasRepairKit(Character* character)
{
    return EnabledNow() && equipmentLoopReady && (tuning[RepairPortion] || tuning[RepairMinimum]) &&
        ou && CanRepair(character) && FindRepairTool(character);
}

bool TryRepair(Character* repairer, Gear* item)
{
    if (!EnabledNow() || !equipmentLoopReady || !(tuning[RepairPortion] || tuning[RepairMinimum]) ||
        !ou || !repairer || !IsDamaged(item) ||
        !FindInventory(repairer, item))
        return false;
    if (!CanRepair(repairer)) {
        Message("Equipment can only be repaired while conscious and out of combat.");
        return false;
    }
    Item* tool = FindRepairTool(repairer);
    if (!tool || !(tool->chargesLeft > 0.0f)) {
        Message("Tools are required to repair equipment.");
        return false;
    }

    SyncRepairWorld();
    for (size_t i = 0; i < jobs.size(); ++i) {
        if (jobs[i].character == repairer->handle) {
            Message("Equipment repair is already in progress.");
            return false;
        }
    }
    try {
        jobs.push_back(RepairJob(repairer, item, tool));
    } catch (...) {
        ErrorLog("ExtraDurability: could not queue equipment repair");
        return false;
    }
    if (jobs.size() == 1)
        repairTick = 0.0;
    Message("Repair started. Keep the character still.");
    return true;
}

void DescribeTuning(MCM_Api* api, TuningKey first, TuningKey last)
{
    for (int i = first; i <= last; ++i) {
        const TuningOption& option = tuningOptions[i];
        api->integer(api, Section, option.key, ::Tr(option.label), ::Tr(option.help),
                     option.initial, option.minimum, option.maximum, 0);
    }
}

}

extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    using namespace durability;
    static const std::string ini = IniPath();
    api->beginMod(api, Section, "Extra Durability (HARD)", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, ::Tr("Weapons and armour wear in combat. Tools restore durability."));
    api->section(api, ::Tr("Durability"));
    api->toggle(api, Section, "Enabled", ::Tr("Enable plugin"),
                ::Tr("Enables equipment wear, its combat effects, and repairs."), 1, 0);
    api->toggle(api, Section, "NpcWear", ::Tr("NPC equipment wear"),
                ::Tr("Applies the same wear, breakage, and destruction rules to NPC equipment."), 1, 0);
    api->toggle(api, Section, "DestroyBroken", ::Tr("Destroy items at 0% durability"),
                ::Tr("Weapons and armour break at 0% durability. With this option on, they disappear without scrap. With it off, they remain available for repair or smelting. Meitou never breaks."), 1, 0);
    api->toggle(api, Section, "BreakNotices", ::Tr("Equipment breakage notices"),
                ::Tr("Shows who broke which item in your squads. NPC breakage is always silent."), 1, 0);
    static const char* const barValues[] = { "0", "1", "2" };
    const char* const barLabels[] = { ::Tr("Off"), ::Tr("Top"), ::Tr("Bottom") };
    api->choice(api, Section, "WeaponConditionBar", ::Tr("Equipment condition bar"),
                ::Tr("Shows a thin durability bar inside icons for worn weapons and armour."),
                "1", barValues, barLabels, 3, 0);
    api->section(api, ::Tr("Wear and breakage"));
    DescribeTuning(api, ProtectionGrade, ArmourWear);
    api->section(api, ::Tr("Weapon damage"));
    DescribeTuning(api, WeaponPenaltyStart, WeaponMinimumEffect);
    api->section(api, ::Tr("Armour protection"));
    DescribeTuning(api, ArmourPenaltyStart, ArmourMinimumEffect);
    api->section(api, ::Tr("Durability by tier"));
    DescribeTuning(api, Capacity1, Capacity9);
    api->section(api, ::Tr("Repair"));
    DescribeTuning(api, RepairPortion, RepairExperience);
    api->section(api, ::Tr("Smelting"));
    api->toggle(api, Section, "SalvageCondition", ::Tr("Durability affects smelting"),
                ::Tr("The item furnace scales the material yield according to the condition of the gear."), 1, 0);
    DescribeTuning(api, SalvageMinimum, SalvageMinimum);
    api->section(api, ::Tr("Diagnostics"));
    api->toggle(api, Section, "Debug", ::Tr("Detailed log"),
                ::Tr("Logs completed repairs and destroyed equipment. Hook errors are always logged."), 0, 0);
}

__declspec(dllexport) void startPlugin()
{
    using namespace durability;
    LoadSettings();
    const KenshiLib::BinaryVersion supportedSteam(KenshiLib::BinaryVersion::STEAM, "1.0.65");
    const KenshiLib::BinaryVersion supportedGog(KenshiLib::BinaryVersion::GOG, "1.0.65");
    KenshiLib::BinaryVersion version = KenshiLib::GetKenshiVersion();
    if (!(version == supportedSteam) && !(version == supportedGog)) {
        ErrorLog("ExtraDurability: game build unavailable");
        return;
    }
    InstallRepairToolState();
    InstallGearState();
    if (!GearStateReady())
        return;
    InstallEquipmentLoop();
    InstallCombat();
    if (equipmentLoopReady) {
        InstallRepairMenu();
        InstallConditionBar();
    }
    HookTooltip(&Sword::_NV_getTooltipData2, "ExtraDurability: sword tooltip hook failed");
    HookTooltip(&Crossbow::_NV_getTooltipData2, "ExtraDurability: crossbow tooltip hook failed");
    HookTooltip(&Armour::_NV_getTooltipData2, "ExtraDurability: armour tooltip hook failed");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&InventoryItemBase::_NV_getTooltipData2),
                           &AddKitCharge, &kitTooltip) != KenshiLib::SUCCESS)
        ErrorLog("ExtraDurability: repair kit tooltip hook failed");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&FurnaceBuilding::_NV_getInputValue),
                           &FurnaceValue, &furnaceValue) != KenshiLib::SUCCESS)
        ErrorLog("ExtraDurability: item furnace value hook failed");
}
