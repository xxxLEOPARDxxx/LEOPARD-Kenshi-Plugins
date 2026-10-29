#pragma once
namespace PowerMarkers
{
    void Install();
    bool WorldActive();
    void Tick(bool show, bool includeNoPower, int size, float range, float height, bool debug);
}
