#pragma once
#include <stdint.h>

class GameWorld;

namespace bolt_trail {

const char DefaultColourText[] = "#ffffff";
const unsigned DefaultTrailLimit = 64;
const unsigned MaxTrails = 256;

inline unsigned ClampTrailLimit(unsigned count)
{
    if (count > MaxTrails) return MaxTrails;
    return count ? count : 1;
}

struct Appearance {
    uint32_t colour;
    unsigned brightnessPercent, lengthPercent, fadePercent, trailLimit;
    bool outline;

    Appearance() : colour(0xffffff), brightnessPercent(100), lengthPercent(100),
                   fadePercent(100), trailLimit(DefaultTrailLimit), outline(false) {}
};

void ConfigureAppearance(const Appearance& appearance);
bool Install();
void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float));

}
