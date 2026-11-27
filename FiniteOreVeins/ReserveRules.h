#pragma once
#include <cmath>

namespace finite_ore {

const int ShareScale = 1000000000;

struct Rules {
    double refillDays;
    double restHours;
    unsigned reopenPercent;
    Rules() : refillDays(10), restHours(12), reopenPercent(25) {}
};

struct Reserve {
    double share;
    double updated;
    double lastMined;
    bool closed;
    explicit Reserve(double now = 0) : share(1), updated(now), lastMined(-1), closed(false) {}
};

inline unsigned MixSeed(unsigned value)
{
    value = (value ^ (value >> 16)) * 0x7feb352du;
    value = (value ^ (value >> 15)) * 0x846ca68bu;
    return value ^ (value >> 16);
}

inline unsigned PositionSeed(float coordinate)
{
    // Use the position on the map and ignore minor float round-off on reload.
    if (!(coordinate >= -1000000000.0f && coordinate <= 1000000000.0f))
        return 0;
    return static_cast<unsigned>(static_cast<int>(std::floor(coordinate / 8.0 + 0.5)));
}

inline unsigned Capacity(int workers, float x, float z)
{
    const int maxWorkers = 256; // Bound work even for malformed FCS data.
    if (workers < 1)
        workers = 1;
    if (workers > maxWorkers)
        workers = maxWorkers;
    unsigned seed = MixSeed(PositionSeed(x) ^ MixSeed(PositionSeed(z)));
    unsigned total = 0;
    for (int worker = 0; worker < workers; ++worker) {
        seed += 0x9e3779b9u;
        total += 1500 + MixSeed(seed) % 2001;
    }
    return total;
}

inline unsigned Units(const Reserve& reserve, unsigned capacity)
{
    // A stored share can round down by half of one step.
    const double units = (reserve.share + 0.5 / ShareScale) * capacity + 0.0000001;
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
