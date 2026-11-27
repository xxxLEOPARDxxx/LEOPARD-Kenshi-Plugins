#include "../ConditionRules.h"
#include "../BreakageNotice.h"
#include <cassert>
#include <cmath>
#include <cstdio>

using namespace durability;

int main()
{
    const int meleeLevels[] = {5, 10, 15, 20, 25, 30, 35, 40, 50, 55, 60, 70, 75, 80};
    const unsigned meleeCaps[] = {600, 600, 1300, 2000, 2000, 3000, 5000, 7000,
                                  10000, 10000, 10000, 15000, 15000, 15000};
    const int craftLevels[] = {5, 20, 40, 60, 80, 95};
    const unsigned craftCaps[] = {600, 1300, 3000, 7000, 15000, 30000};
    for (unsigned i = 0; i < sizeof(meleeLevels) / sizeof(*meleeLevels); ++i)
        assert(MaxCondition(Melee, meleeLevels[i], false) == meleeCaps[i]);
    for (unsigned i = 0; i < sizeof(craftLevels) / sizeof(*craftLevels); ++i) {
        assert(MaxCondition(ArmourGear, craftLevels[i], false) == craftCaps[i]);
        assert(MaxCondition(Ranged, craftLevels[i], false) == craftCaps[i]);
    }
    assert(MaxCondition(ArmourGear, 10, false) == 600);
    assert(MaxCondition(ArmourGear, 11, false) == 1300);
    assert(MaxCondition(ArmourGear, 21, false) == 3000);
    assert(MaxCondition(ArmourGear, 41, false) == 7000);
    assert(MaxCondition(ArmourGear, 61, false) == 15000);
    assert(MaxCondition(ArmourGear, 81, false) == 30000);
    assert(MaxCondition(Melee, 5, true) == 30000);
    assert(!ProtectedMeleeGrade(Melee, 99, false, 100));
    assert(ProtectedMeleeGrade(Melee, 100, false, 100));
    assert(ProtectedMeleeGrade(Melee, 120, false, 100));
    assert(!ProtectedMeleeGrade(ArmourGear, 100, false, 100));
    assert(!ProtectedMeleeGrade(Ranged, 100, false, 100));
    const bool customMeitou = ProtectedMeleeGrade(Melee, 100, false, 100);
    assert(MaxCondition(Melee, 100, false) == 15000);
    assert(!DestroyAtZero(Melee, customMeitou, 0, true));
    assert(!BreakNotice(Melee, customMeitou, 8, 0, true, true));
    assert(ProtectedMeleeGrade(Melee, 90, false, 90));
    assert(!ProtectedMeleeGrade(Melee, 89, false, 90));
    assert(!ProtectedMeleeGrade(Melee, 100, false, 101));
    assert(!ProtectedMeleeGrade(Melee, 100, false, 0));
    assert(ProtectedMeleeGrade(Melee, 80, true, 1000));
    assert(ProtectedMeleeGrade(Melee, 80, true, 0));
    assert(!ProtectedMeleeGrade(ArmourGear, 100, true, 90));
    assert(ConditionTier(Melee, 100, true) == 8);
    assert(ConditionTier(Melee, 100, false) == 7);
    assert(ConditionTier(Unsupported, 100, false) == -1);
    const long customCaps[] = {1, 2, 3, 4, 5, 6, 7, 20000, 40000};
    assert(MaxCondition(Melee, 100, false, customCaps) == 20000);
    assert(MaxCondition(Melee, 100, true, customCaps) == 40000);
    assert(MaxCondition(ArmourGear, 100, false, customCaps) == 40000);
    assert(MaxCondition(Unsupported, 100, false, customCaps) == 0);
    assert(MaxCondition(Unsupported, 99, false) == 0);
    assert(ConditionFactor(700, 600) == 1.0f);
    assert(ConditionFactor(300, 600) == 1.0f);
    assert(std::fabs(ConditionFactor(150, 600) - 0.6f) < 0.0001f);
    assert(ConditionFactor(0, 600) == 0.2f);
    assert(ConditionFactor(0, 0) == 1.0f);
    const unsigned customCurve = PackConditionCurve(80, 20, 40);
    assert(ConditionFactor(80, 100, customCurve) == 1.0f);
    assert(std::fabs(ConditionFactor(50, 100, customCurve) - 0.7f) < 0.0001f);
    assert(ConditionFactor(20, 100, customCurve) == 0.4f);
    assert(ConditionFactor(1, 100, customCurve) == 0.4f);
    const unsigned equalCurve = PackConditionCurve(25, 75, 30);
    assert(ConditionFactor(25, 100, equalCurve) == 0.3f);
    assert(ConditionFactor(26, 100, equalCurve) == 1.0f);
    assert(ConditionFactor(1, 100, PackConditionCurve(0, 0, 20)) == 1.0f);
    assert(ConditionFactor(100, 100, PackConditionCurve(100, 100, 0)) == 1.0f);
    assert(std::fabs(ConditionFactor(500000, 1000000, PackConditionCurve(100, 0, 0)) - 0.5f) < 0.0001f);
    assert(VisibleConditionRatio(0, 0) < 0.0f);
    assert(VisibleConditionRatio(600, 600) < 0.0f);
    assert(VisibleConditionRatio(601, 600) < 0.0f);
    assert(VisibleConditionRatio(300, 600) == 0.5f);
    assert(VisibleConditionRatio(0, 600) == 0.0f);
    assert(CanBreak(Melee, false));
    assert(CanBreak(Ranged, false));
    assert(CanBreak(ArmourGear, false));
    assert(!CanBreak(Melee, true));
    assert(!CanBreak(Unsupported, false));
    assert(DestroyAtZero(Melee, false, 0, true));
    assert(DestroyAtZero(Ranged, false, 0, true));
    assert(DestroyAtZero(ArmourGear, false, 0, true));
    assert(!DestroyAtZero(Melee, true, 0, true));
    assert(!DestroyAtZero(Melee, false, 1, true));
    assert(!DestroyAtZero(Melee, false, 0, false));
    assert(!DestroyAtZero(ArmourGear, false, 0, false));
    assert(!DestroyAtZero(Unsupported, false, 0, true));
    assert(CanQueueBrokenEquipment(false, true, true, false, false));
    assert(!CanQueueBrokenEquipment(false, false, true, false, false));
    assert(!CanQueueBrokenEquipment(true, false, true, false, true));
    assert(!CanQueueBrokenEquipment(false, true, false, true, true));
    assert(CanQueueBrokenEquipment(false, true, false, true, false));
    assert(!CanQueueBrokenEquipment(true, true, false, true, false));
    assert(CanQueueBrokenEquipment(true, true, true, false, true));
    assert(!CanQueueBrokenEquipment(true, true, true, false, false));
    assert(BreakNotice(Melee, false, 1, 0, true, true));
    assert(BreakNotice(ArmourGear, false, 4, 0, true, true));
    assert(!BreakNotice(Ranged, false, 1, 1, true, true));
    assert(!BreakNotice(Ranged, false, 0, 0, true, true));
    assert(!BreakNotice(Melee, true, 1, 0, true, true));
    assert(!BreakNotice(Melee, false, 1, 0, false, true));
    assert(!BreakNotice(Melee, false, 1, 0, true, false));
    assert(RepairBudget(600, 0.0f) == 500);
    assert(RepairBudget(1300, 0.0f) == 500);
    assert(RepairBudget(2000, 0.0f) == 500);
    assert(RepairBudget(30000, 0.0f) == 7500);
    assert(RepairBudget(30000, 100.0f) == 11250);
    assert(RepairBudget(30000, -50.0f) == 7500);
    assert(RepairRate(500, 0.0f) == 62.5);
    assert(RepairRate(750, 100.0f) == 187.5);
    assert(RepairBudget(500, 100.0f, 40, 25, 100) == 400);
    assert(RepairBudget(401, 0.0f, 25, 1, 50) == 101);
    assert(RepairBudget(600, 100.0f, 0, 0, 500) == 0);
    assert(RepairBudget(1000000, 100.0f, 100, 1000000, 500) == 6000000);
    assert(RepairRate(600, 100.0f, 12, 50) == 75.0);
    assert(RepairSlice(0.5, 500, 100.0, 5.0) == 0);
    assert(RepairSlice(100.0, 30, 100.0, 5.0) == 30);
    assert(RepairSlice(100.0, 200, 2.0, 5.0) == 10);
    assert(RepairSlice(100.0, 200, 0.0, 5.0) == 0);
    assert(RepairSlice(100.0, 200, 0.01, 5.0) == 1);
    assert(RepairChargeExhausted(-0.001, 5.0));
    assert(RepairChargeExhausted(0.0001, 5.0));
    assert(!RepairChargeExhausted(0.001, 60000.0));
    // Going from 100 to 0 charge in 50 slices restores exactly 500.
    float charge = 100.0f;
    unsigned repaired = 0;
    for (int step = 0; step < 50; ++step) {
        const unsigned points = RepairSlice(10.0, 600 - repaired, charge, 5.0);
        repaired += points;
        charge -= static_cast<float>(points / 5.0);
    }
    assert(repaired == 500 && charge == 0.0f);
    assert(SalvageFactor(600, 600) == 1.0f);
    assert(SalvageFactor(300, 600) == 0.6f);
    assert(SalvageFactor(0, 600) == 0.2f);
    assert(SalvageFactor(0, 600, 0) == 0.0f);
    assert(SalvageFactor(300, 600, 100) == 1.0f);
    assert(RepairToolChargeForSave(100.0f, 100.0f) == 1.0f);
    assert(RepairToolChargeForSave(25.0f, 100.0f) == 0.25f);
    assert(RestoreRepairToolCharge(1.0f, 100.0f) == 100.0f);
    assert(RestoreRepairToolCharge(0.25f, 100.0f) == 25.0f);
    assert(RestoreRepairToolCharge(0.25f, 1.0f) == 0.25f);
    assert(RestoreRepairToolCharge(87.0f, 100.0f) == 87.0f);
    const char shpilka[] = "\xD0\xA8\xD0\xBF\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xB0";
    const char shpilki[] = "\xD0\xA8\xD0\xBF\xD0\xB8\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xB8";
    assert(BreakageNoticeName(shpilka, "ru_RU") == shpilki);
    assert(BreakageNoticeName(shpilka, "russian") == shpilki);
    assert(BreakageNoticeName(shpilka, "ru") == shpilki);
    assert(BreakageNoticeName(shpilka, "en_GB") == shpilka);
    assert(BreakageNoticeName("Beep", "ru_RU") == "Beep");
    assert(BreakageNoticeName(std::string("A ") + shpilka, "ru_RU") == std::string("A ") + shpilka);
    std::puts("Condition and breakage notice rules passed");
}
