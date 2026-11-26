#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "ExtraDurability.h"

#include <Debug.h>
#include <StartupHookBatch.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/Damages.h>
#include <kenshi/Gear.h>
class GunClassPersonal;
#include <kenshi/GunClass.h>
#include <kenshi/combat/CombatClass.h>

namespace durability {
namespace {


typedef HitMaterialType (*MeleeFn)(Character*, CutDirection, Damages&, Character*,
                                  CombatTechniqueData*, int);
typedef void (*BlockFn)(CombatClass*, CutDirection, const Damages&, RootObject*);
typedef bool (*FleshFn)(Armour*, GameData*);
typedef void (*ShootFn)(GunClass*, Character*, RootObject*, StatsEnumerated, const Ogre::Vector3&);
typedef int (*GunDamageFn)(GunClassPersonal*);

MeleeFn originalMelee = NULL;
BlockFn originalBlock = NULL;
FleshFn originalFlesh = NULL;
ShootFn originalShoot = NULL;
GunDamageFn originalDamageMin = NULL;
GunDamageFn originalDamageMax = NULL;
bool rangedDamageReady = false;

Sword* EquippedSword(Character* character)
{
    Weapon* weapon = character->getCurrentWeapon();
    if (!weapon || !weapon->isInInventory || !weapon->isEquipped)
        return NULL;
    Sword* sword = weapon->isSword();
    return sword && IsSupported(sword) ? sword : NULL;
}

Crossbow* EquippedCrossbow(Character* character, GunClass* gun)
{
    Weapon* weapon = character->getCurrentWeapon();
    if (!weapon || !weapon->isInInventory || !weapon->isEquipped)
        return NULL;
    Crossbow* crossbow = weapon->isCrossbow();
    return crossbow && crossbow->gunClass == gun && IsSupported(crossbow) ? crossbow : NULL;
}

template<class T>
struct ScopedContext
{
    T& slot;
    const T previous;

    ScopedContext(T& current, T next) : slot(current), previous(current)
    {
        slot = next;
    }

    ~ScopedContext() { slot = previous; }

private:
    ScopedContext(const ScopedContext&);
    ScopedContext& operator=(const ScopedContext&);
};

// The native character-hit path calls _blockHit only after the defender
// actually blocks. Scoping the defender avoids reading CombatClass internals.
__declspec(thread) Character* currentDefender;

HitMaterialType HitWithSword(Sword* sword, Character* victim, CutDirection direction, Damages& damage,
                            Character* attacker, CombatTechniqueData* attack, int combo)
{
    const hand usedItem(sword->getHandle());
    const float effect = Effect(sword);
    HitMaterialType result;
    if (effect == 1.0f)
        result = originalMelee(victim, direction, damage, attacker, attack, combo);
    else
    {
        Damages scaled(damage);
        // Native multiply scales the four damage channels, leaving penetration
        // and the bleeding coefficient alone. The next target sees its own copy.
        scaled.multiply(effect);
        result = originalMelee(victim, direction, scaled, attacker, attack, combo);
    }

    if (result != HIT_MISSED && usedItem.getItem() == sword)
        Wear(sword);
    return result;
}

HitMaterialType MeleeHook(Character* victim, CutDirection direction, Damages& damage,
                          Character* attacker, CombatTechniqueData* attack, int combo)
{
    Character* defender = victim && AppliesTo(victim) ? victim : NULL;
    ScopedContext<Character*> defenderScope(currentDefender, defender);
    Sword* sword = attacker && AppliesTo(attacker) ? EquippedSword(attacker) : NULL;
    if (sword)
        return HitWithSword(sword, victim, direction, damage, attacker, attack, combo);
    return originalMelee(victim, direction, damage, attacker, attack, combo);
}

void BlockHook(CombatClass* combat, CutDirection direction, const Damages& damage, RootObject* attacker)
{
    originalBlock(combat, direction, damage, attacker);
    Character* defender = currentDefender;
    if (!defender || defender->getCombatClass() != combat)
        return;
    Sword* sword = EquippedSword(defender);
    if (sword)
        Wear(sword);
}

__declspec(thread) unsigned armourRandom;

bool ArmourLetsHitThrough(float effect)
{
    if (effect >= 1.0f)
        return false;
    if (effect <= 0.0f)
        return true;

    // A private xorshift stream keeps the native coverage rolls in their
    // original order. A full-condition or broken layer does not draw from it.
    unsigned value = armourRandom;
    if (!value)
    {
        value = GetCurrentThreadId() ^ 0x9e3779b9u;
        if (!value) value = 1;
    }
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    armourRandom = value;
    return (value >> 8) * (1.0f / 16777216.0f) >= effect;
}

bool FleshHook(Armour* armour, GameData* bodypart)
{
    if (originalFlesh(armour, bodypart))
        return true;
    if (!armour->isEquipped || !armour->isInInventory)
        return false;
    // Native false means the layer was selected to absorb damage. Condition
    // changes the chance it can stop this hit. No object stats are overwritten.
    return ArmourLetsHitThrough(ArmourEffectAndWear(armour));
}

struct Shot
{
    GunClass* gun;
    float effect;
};

__declspec(thread) const Shot* currentShot;

int DamageForShot(GunClassPersonal* gun, int damage)
{
    const Shot* shot = currentShot;
    if (!shot || shot->gun != gun || damage <= 0 || shot->effect >= 1.0f)
        return damage;
    if (shot->effect <= 0.0f)
        return 0;

    const int scaled = static_cast<int>(damage * static_cast<double>(shot->effect) + 0.5);
    return scaled > 0 ? scaled : 1;
}

int DamageMinHook(GunClassPersonal* gun)
{
    return DamageForShot(gun, originalDamageMin(gun));
}

int DamageMaxHook(GunClassPersonal* gun)
{
    return DamageForShot(gun, originalDamageMax(gun));
}

void ShootHook(GunClass* gun, Character* shooter, RootObject* target, StatsEnumerated stat,
               const Ogre::Vector3& aim)
{
    Crossbow* crossbow = gun && shooter && AppliesTo(shooter) ? EquippedCrossbow(shooter, gun) : NULL;
    const Shot shot = {gun, crossbow && rangedDamageReady ? Effect(crossbow) : 1.0f};
    ScopedContext<const Shot*> shotScope(currentShot, crossbow ? &shot : NULL);
    if (!crossbow)
    {
        originalShoot(gun, shooter, target, stat, aim);
        return;
    }

    const hand usedItem(crossbow->getHandle());
    const int before = gun->numShotsCurrent;
    originalShoot(gun, shooter, target, stat, aim);
    if (usedItem.getItem() == crossbow && crossbow->gunClass == gun && gun->numShotsCurrent < before)
        Wear(crossbow);
}

bool InstallImmediate(void* address, void* callback, void** trampoline)
{
    return KenshiLib::AddHook(address, callback, trampoline) == KenshiLib::SUCCESS;
}

} // namespace

void InstallCombat()
{
    // Resolve through the platform's SDK table. Hardcoded header RVAs are
    // for GOG and differ from the Steam 1.0.65 executable.
    StartupHooks::Hook hooks[] = {
        StartupHooks::Hook(KenshiLib::GetRealAddress(&Character::_NV_hitByMeleeAttack),
                           MeleeHook, &originalMelee, "melee hit"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&CombatClass::_blockHit),
                           BlockHook, &originalBlock, "weapon block"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&Armour::didIHitFlesh),
                           FleshHook, &originalFlesh, "armour coverage"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&GunClass::shoot),
                           ShootHook, &originalShoot, "crossbow shot"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&GunClassPersonal::_NV_getDamageMin),
                           DamageMinHook, &originalDamageMin, "crossbow minimum damage"),
        StartupHooks::Hook(KenshiLib::GetRealAddress(&GunClassPersonal::_NV_getDamageMax),
                           DamageMaxHook, &originalDamageMax, "crossbow maximum damage")
    };
    StartupHooks::Install(hooks, InstallImmediate);
    const size_t count = sizeof(hooks) / sizeof(hooks[0]);
    rangedDamageReady = hooks[count - 2].installed && hooks[count - 1].installed;
    for (size_t i = 0; i < count; ++i)
        if (!hooks[i].installed)
            ErrorLog(std::string("ExtraDurability: failed to hook ") + hooks[i].name);
}

} // namespace durability
