#pragma once

class Gear;

namespace durability
{
    // Missing records represent full condition. The caller supplies capacity
    // in the range of the signed integer stored in the save.
    unsigned ReadCondition(Gear* gear, unsigned full);
    unsigned WearCondition(Gear* gear, unsigned full, unsigned amount, unsigned* before = 0);
    bool RestoreCondition(Gear* gear, unsigned full, unsigned amount);

    void InstallGearState();
    bool GearStateReady();
}
