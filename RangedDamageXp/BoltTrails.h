#pragma once
#include <stdint.h>

class GameWorld;

namespace bolt_trail {

const char DefaultColourText[] = "#80e0ff";

struct Appearance {
    uint32_t colour;
    unsigned brightnessPercent, lengthPercent, fadePercent;
    bool outline;

    Appearance() : colour(0x80e0ff), brightnessPercent(85), lengthPercent(100),
                   fadePercent(50), outline(true) {}
};

void ConfigureAppearance(const Appearance& appearance);
bool Install();
void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float));

}
