#pragma once
#include <stdint.h>

class GameWorld;

namespace bolt_trail {

const char DefaultColourText[] = "#ffffff";

struct Appearance {
    uint32_t colour;
    unsigned brightnessPercent, lengthPercent, fadePercent;
    bool outline;

    Appearance() : colour(0xffffff), brightnessPercent(100), lengthPercent(100),
                   fadePercent(100), outline(false) {}
};

void ConfigureAppearance(const Appearance& appearance);
bool Install();
void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float));

}
