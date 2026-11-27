#pragma once

namespace durability
{
enum EquipmentKind { Unsupported, Melee, Ranged, ArmourGear };

inline bool ProtectedMeleeGrade(EquipmentKind kind, int level, bool stockMeitou, int minimumGrade)
{
    return kind == Melee && (stockMeitou || (minimumGrade > 0 && level >= minimumGrade));
}

static const long DefaultConditionCaps[] = {600, 1300, 2000, 3000, 5000, 7000, 10000, 15000, 30000};

inline int ConditionTier(EquipmentKind kind, int level, bool stockMeitou)
{
    if (kind == Melee) {
        if (stockMeitou) return 8;
        if (level >= 70) return 7;
        if (level >= 50) return 6;
        if (level >= 40) return 5;
        if (level >= 35) return 4;
        if (level >= 30) return 3;
        if (level >= 20) return 2;
        return level >= 15 ? 1 : 0;
    }
    if (kind != Ranged && kind != ArmourGear)
        return -1;
    if (level > 80) return 8;
    if (level > 60) return 7;
    if (level > 40) return 5;
    if (level > 20) return 3;
    return level > 10 ? 1 : 0;
}

inline unsigned MaxCondition(EquipmentKind kind, int level, bool stockMeitou,
                             const volatile long* caps = DefaultConditionCaps)
{
    const int tier = ConditionTier(kind, level, stockMeitou);
    return tier < 0 ? 0 : caps[tier];
}

enum { DefaultConditionCurve = 50 | (20 << 16) };

inline unsigned PackConditionCurve(unsigned start, unsigned end, unsigned minimumEffect)
{
    return start | ((end < start ? end : start) << 8) | (minimumEffect << 16);
}

inline float ConditionFactor(unsigned current, unsigned full, unsigned curve = DefaultConditionCurve)
{
    if (!full || current >= full)
        return 1.0f;
    const unsigned scaled = current * 100;
    const unsigned start = full * (curve & 0xff);
    const unsigned end = full * ((curve >> 8) & 0xff);
    if (scaled > end && scaled >= start)
        return 1.0f;
    const float minimum = (curve >> 16) / 100.0f;
    if (scaled <= end)
        return minimum;
    return minimum + (1.0f - minimum) * (static_cast<float>(scaled - end) / (start - end));
}

inline float VisibleConditionRatio(unsigned current, unsigned full)
{
    return current < full ? static_cast<float>(current) / full : -1.0f;
}

inline bool CanBreak(EquipmentKind kind, bool meitou)
{
    return (kind == Melee && !meitou) || kind == Ranged || kind == ArmourGear;
}

inline bool DestroyAtZero(EquipmentKind kind, bool meitou, unsigned condition, bool destroy)
{
    return !condition && destroy && CanBreak(kind, meitou);
}

inline bool CanQueueBrokenEquipment(bool queueClosed, bool enabled, bool destroy, bool notify, bool loaded)
{
    return enabled && (loaded ? destroy : !queueClosed && (destroy || notify));
}

inline bool BreakNotice(EquipmentKind kind, bool meitou, unsigned before, unsigned condition,
                        bool notify, bool player)
{
    return before && !condition && notify && player && CanBreak(kind, meitou);
}


inline float RepairToolChargeForSave(float charge, float capacity)
{
    if (capacity <= 1.0f)
        return charge;
    if (!(charge > 0.0f))
        return 0.0f;
    return charge < capacity ? charge / capacity : 1.0f;
}

inline float RestoreRepairToolCharge(float charge, float capacity)
{
    return capacity > 1.0f && charge >= 0.0f && charge <= 1.0f ? charge * capacity : charge;
}

inline float SmithingSkill(float smithing)
{
    if (!(smithing > 0.0f)) return 0.0f;
    return smithing > 100.0f ? 100.0f : smithing;
}

inline unsigned RepairBudget(unsigned full, float smithing, unsigned restorePercent = 25,
                             unsigned minimum = 500, unsigned skillBonusPercent = 50)
{
    const unsigned portion = (full * restorePercent + 99) / 100;
    const unsigned base = portion < minimum ? minimum : portion;
    return static_cast<unsigned>(base * (1.0 + SmithingSkill(smithing) * skillBonusPercent / 10000.0));
}

inline double RepairRate(unsigned budget, float smithing, unsigned seconds = 8, unsigned skillBonusPercent = 100)
{
    return budget * (1.0 + SmithingSkill(smithing) * skillBonusPercent / 10000.0) / seconds;
}

// Native item charges are floats. Express the rounding allowance in repaired points.
static const double RepairPointsEpsilon = 0.005;

inline bool RepairChargeExhausted(double remainingCharge, double pointsPerCharge)
{
    return remainingCharge * pointsPerCharge <= RepairPointsEpsilon;
}

inline unsigned RepairSlice(double work, unsigned missing, double charge, double pointsPerCharge)
{
    if (!(work >= 1.0) || !(charge > 0.0) || !(pointsPerCharge > 0.0))
        return 0;
    const double available = charge * pointsPerCharge + RepairPointsEpsilon;
    if (available < 1.0) return missing ? 1 : 0;
    if (work > available) work = available;
    if (work > missing) return missing;
    return static_cast<unsigned>(work);
}

inline float SalvageFactor(unsigned current, unsigned full, unsigned minimumPercent = 20)
{
    if (!full || current >= full)
        return 1.0f;
    const float minimum = minimumPercent / 100.0f;
    return minimum + (1.0f - minimum) * (static_cast<float>(current) / full);
}
}
