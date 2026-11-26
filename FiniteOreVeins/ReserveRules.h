#pragma once
#include <cmath>

namespace finite_ore {

struct Rules {
    unsigned baseOre;
    double refillDays;
    double restHours;
    unsigned reopenPercent;
    bool scaleByQuality;
    Rules() : baseOre(100), refillDays(10), restHours(12), reopenPercent(25), scaleByQuality(true) {}
};

struct Reserve {
    double share;
    double updated;
    double lastMined;
    bool closed;
    explicit Reserve(double now = 0) : share(1), updated(now), lastMined(-1), closed(false) {}
};

inline unsigned Capacity(const Rules& rules, float quality)
{
    const double scale = !rules.scaleByQuality ? 1.0 : !(quality >= 0.25f) ? 0.25 : quality > 3.0f ? 3.0 : quality;
    const unsigned amount = static_cast<unsigned>(rules.baseOre * scale + 0.5);
    return amount ? amount : 1;
}

inline unsigned Units(const Reserve& reserve, unsigned capacity)
{
    // A billionth of capacity is the save format's resolution.
    const double units = reserve.share * capacity + 0.0001;
    return units >= capacity ? capacity : units > 0 ? static_cast<unsigned>(units) : 0;
}

inline void Refresh(Reserve& reserve, const Rules& rules, unsigned capacity, double now)
{
    if (now < reserve.updated) {
        const double shift = reserve.updated - now;
        reserve.updated = now;
        if (reserve.lastMined >= 0)
            reserve.lastMined -= shift;
    }
    if (rules.refillDays > 0 && reserve.share < 1) {
        double from = reserve.updated;
        if (reserve.lastMined >= 0 && from < reserve.lastMined + rules.restHours)
            from = reserve.lastMined + rules.restHours;
        if (now > from) {
            reserve.share += (now - from) / (rules.refillDays * 24.0);
            if (reserve.share > 1)
                reserve.share = 1;
        }
    }
    reserve.updated = now;
    const unsigned rounded = (capacity * rules.reopenPercent + 99) / 100;
    const unsigned units = Units(reserve, capacity);
    if (!units)
        reserve.closed = true;
    else if (reserve.closed && units >= (rounded ? rounded : 1))
        reserve.closed = false;
}

inline bool Spend(Reserve& reserve, unsigned capacity, double now)
{
    if (reserve.closed || !Units(reserve, capacity))
        return false;
    reserve.share -= 1.0 / capacity;
    if (reserve.share < 0)
        reserve.share = 0;
    reserve.lastMined = reserve.updated = now;
    reserve.closed = !Units(reserve, capacity);
    return true;
}

}
