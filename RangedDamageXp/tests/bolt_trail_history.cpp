#include "../BoltTrailRibbon.h"
#include <cassert>
#include <cstdio>

using namespace bolt_trail;

static bool Near(float a, float b)
{
    return std::abs(a-b) < 0.001f;
}

int main()
{
    const Appearance defaults;
    assert(defaults.colour == 0xffffff && defaults.brightnessPercent == 100 &&
           defaults.lengthPercent == 100 && defaults.fadePercent == 100 && !defaults.outline);

    History history;
    Streak output[MaxTrails];
    const Point start, forward(1,0,0);
    assert(!history.Active() && history.Build(1, 0, true, 100, output) == 0);
    history.Observe(1, start, Point(100), forward, 3000, 0, 0.03f, true, false, 1, 0, 100);
    assert(history.Active() && history.Build(1, 0, true, 100, output) == 1);
    assert(Near(output[0].head.x, 100) && Near(output[0].tail.x, 0));
    history.Observe(1, Point(100), Point(800), forward, 3000, 0.03f, 0.26f, true, false, 2, 0.23, 100);
    assert(history.Build(2, 0.23, true, 100, output) == 1 && Near(output[0].tail.x, 300));
    assert(history.Build(3, 0.23, false, 100, output) == 1 && Near(output[0].tail.x, 300));
    history.Observe(1, Point(800), Point(840), forward, 3000, 0.26f, 0.27f, false, true, 4, 0.24, 100);
    assert(history.Build(4, 0.24, true, 100, output) == 1);
    assert(Near(output[0].head.x, 840) && Near(output[0].tail.x, 300));
    assert(history.Build(5, 0.29, true, 100, output) == 1);
    assert(Near(output[0].head.x, 840) && Near(output[0].tail.x, 480));
    assert(history.Build(6, 0.29, false, 100, output) == 1 && Near(output[0].tail.x, 480));
    assert(history.Build(7, 0.40, true, 100, output) == 0 && !history.Active());

    history.Observe(1, start, Point(300), forward, 1000, 0, .30f, true, false, 8, 1, 100);
    assert(history.Build(8, 1, true, 100, output) == 1 && Near(output[0].tail.x, 100));
    assert(history.Build(8, 1, true, 50, output) == 1 && Near(output[0].tail.x, 200));
    assert(history.Build(9, 1.01, true, 100, output) == 1 && Near(output[0].head.x, 300));
    assert(history.Build(10, 1.09, true, 100, output) == 1 && output[0].tail.x > 100);
    assert(history.Build(11, 1.18, true, 100, output) == 0);

    history.Clear();
    for (unsigned i = 1; i < 1000; ++i)
        history.Observe(i, start, Point(20), forward, 500, 0, 0.04f, true, false, 12, 2, 100);
    assert(history.Build(12, 2, true, 100, output) == MaxTrails);
    Quad quad;
    assert(MakeQuad(output[0], Point(10,0,100), Point(1,0,0), quad));
    assert(Near(Length(quad.headRight-quad.headLeft), 0.7f));
    assert(Near(Length(quad.tailRight-quad.tailLeft), 0.7f));
    assert(Near(Length(quad.headRight-quad.tailRight), 20.0f));
    assert(!MakeQuad(output[0], Point(10,0,30000), Point(1,0,0), quad));

    unsigned char mask[MaskWidth*MaskHeight*4];
    MakeMask(mask, 100, false);
    uint32_t checksum = 2166136261u;
    for (unsigned i = 0; i < sizeof(mask); ++i)
        checksum = (checksum ^ mask[i]) * 16777619u;
    assert(checksum == 0x79deed55u);
    assert(mask[3] < mask[4*8+3]); // Cross-section fades at the edges.
    assert(mask[(4*MaskWidth+8)*4+3] > mask[(63*MaskWidth+8)*4+3]);
    MakeMask(mask, 50, true);
    assert(mask[0] < mask[4*8] && mask[4*8] == 255);

    puts("Meitou reference: flight lengths, hit and miss stop, paused clock, fixed width and 1024 exact mask pixels");
}
