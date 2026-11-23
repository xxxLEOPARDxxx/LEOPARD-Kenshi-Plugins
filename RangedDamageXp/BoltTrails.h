#pragma once

class GameWorld;

namespace bolt_trail {

bool Install();
void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float));

}
