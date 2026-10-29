#pragma once
#include <cmath>

// One call to the game's time-based XP function is one raw tick. Fractional
// multipliers carry across ticks; the game's racial/XP_Overhaul scaling is
// still applied separately to each added skill tick.
namespace MiningRule
{
    inline int Ticks(float multiplier, float level, float cap, bool caps, float& carry)
    {
        if (multiplier <= 0 || (caps && level >= cap)) { carry = 0; return 0; }
        carry += multiplier;
        const int n = (int)std::floor(carry);
        carry -= n;
        return n;
    }
}
