#include "../BoltTrailHistory.h"
#include "../BoltTrailRibbon.h"
#include <cassert>
#include <cstdio>

using namespace bolt_trail;

int main()
{
    History history;
    Appearance fullFade;
    fullFade.fadePercent = 100;
    Segment output[MaxSegments];
    const Point origin(0, 0, 0), tip(20, 5, 0), next(40, 3, 2);
    assert(history.Build(100, output, fullFade) == 0);
    history.Sample(1, origin, tip, 100);
    assert(history.Build(100, output, fullFade) == 1);
    assert(output[0].tail.x == 0 && output[0].head.y == 5);
    assert(output[0].tailAlpha == 0 && output[0].headAlpha > 0);
    const float alpha = output[0].headAlpha;
    history.Sample(1, tip, next, 110);
    assert(history.Build(110, output, fullFade) == 2);
    assert(output[0].tail.y == 5 && output[0].head.z == 2);
    assert(output[1].headAlpha == output[0].tailAlpha);
    assert(history.Build(150, output, fullFade) == 2 && output[0].headAlpha < alpha);
    assert(history.Build(210, output, fullFade) == 0);

    history.Sample(1, origin, tip, 230);
    history.Sample(1, next, Point(60, 0, 0), 234); // reused or discontinuous projectile
    assert(history.Build(234, output, fullFade) == 1 && output[0].tail.x == next.x);
    history.Clear();
    history.Sample(1, origin, tip, 250);
    for (unsigned i = 0; i < 8; ++i)
        history.Sample(1, Point(tip.x + 0.25f, tip.y, tip.z), tip, 254 + i);
    assert(history.Build(262, output, fullFade) == 1 && output[0].headAlpha > 0);
    history.Clear();
    history.Sample(2, origin, Point(700, 0, 0), 300);
    assert(history.Build(300, output, fullFade) == 1);
    assert(output[0].head.x == 700 && output[0].tail.x > 0 && output[0].tailAlpha == 0);
    history.Sample(3, origin, origin, 301);
    assert(history.Build(301, output, fullFade) == 1);
    history.Clear();
    history.Sample(2, origin, tip, 0xfffffff0u);
    assert(history.Build(4, output, fullFade) == 1);
    assert(history.Build(100, output, fullFade) == 0);
    history.Clear();
    for (unsigned i = 0; i < 40; ++i)
        history.Sample(2, Point(float(i)), Point(float(i+1)), 400 + i);
    assert(history.Build(439, output, fullFade) == SamplesPerTrail - 1);
    assert(output[0].head.x == 40 && output[SamplesPerTrail - 2].tail.x == 33);
    history.Clear();
    for (unsigned i = 1; i < 1000; ++i)
        history.Sample(i, origin, tip, 500);
    assert(history.Build(500, output, fullFade) == MaxTrails);
    history.Clear();
    assert(history.Build(500, output, fullFade) == 0);
    const Appearance appearance;
    history.Sample(1, origin, Point(500, 0, 0), 600);
    assert(history.Build(600, output, appearance) == 2);
    assert(output[0].tail.x == 410 && output[0].head.x == 500);
    assert(output[0].tailAlpha == 0.85f && output[0].headAlpha == 0.85f);
    assert(output[1].tail.x == 320 && output[1].tailAlpha == 0);
    Appearance longTrail = appearance;
    longTrail.lengthPercent = 200;
    longTrail.fadePercent = 100;
    longTrail.brightnessPercent = 60;
    assert(history.Build(600, output, longTrail) == 1);
    assert(output[0].tail.x == 140 && output[0].headAlpha == 0.60f);
    const View view = { origin, Point(1,0,0), Point(0,1,0), Point(0,0,-1),
                         100.0f, 100.0f, 1.0f, true };
    const Segment atDepth = { Point(0,0,-10), Point(30,0,-30), 0.1f, 0.8f };
    Ribbon ribbon;
    assert(MakeRibbon(view, atDepth, ribbon));
    assert(std::abs(ribbon.tailPerPixel.y - 0.10f) < 0.001f);
    assert(std::abs(ribbon.headPerPixel.y - 0.30f) < 0.001f);
    const Segment crossesNear = { Point(5,0,0), Point(20,0,-2), 0.0f, 1.0f };
    assert(MakeRibbon(view, crossesNear, ribbon));
    assert(ribbon.segment.tail.z == -1.0f && ribbon.segment.tailAlpha == 0.50f);
    const Segment behind = { Point(0,0,1), Point(30,0,2), 0.1f, 0.8f };
    assert(!MakeRibbon(view, behind, ribbon));
    history.Clear();
    for (unsigned id = 1; id < 1000; ++id)
        for (unsigned step = 0; step < 7; ++step)
            history.Sample(id, Point(float(step*10)), Point(float((step+1)*10)), 800);
    assert(history.Build(800, output, appearance) == MaxSegments);
    puts("Bolt history bounds, fade splitting and pixel-width projection passed");
}
