#include "../BoltTrailHistory.h"
#include <cassert>
#include <cstdio>

using namespace bolt_trail;

int main()
{
    History history;
    Segment output[MaxSegments];
    const Point origin(0, 0, 0), tip(20, 5, 0), next(40, 3, 2);
    assert(history.Build(100, output) == 0);
    history.Sample(1, origin, tip, 100);
    assert(history.Build(100, output) == 1);
    assert(output[0].tail.x == 0 && output[0].head.y == 5);
    assert(output[0].tailAlpha == 0 && output[0].headAlpha > 0);
    const float alpha = output[0].headAlpha;
    history.Sample(1, tip, next, 110);
    assert(history.Build(110, output) == 2);
    assert(output[0].tail.y == 5 && output[0].head.z == 2);
    assert(output[1].headAlpha == output[0].tailAlpha);
    assert(history.Build(150, output) == 2 && output[0].headAlpha < alpha);
    assert(history.Build(210, output) == 0);

    history.Sample(1, origin, tip, 230);
    history.Sample(1, next, Point(60, 0, 0), 234); // reused or discontinuous projectile
    assert(history.Build(234, output) == 1 && output[0].tail.x == next.x);
    history.Clear();
    history.Sample(1, origin, tip, 250);
    for (unsigned i = 0; i < 8; ++i)
        history.Sample(1, Point(tip.x + 0.25f, tip.y, tip.z), tip, 254 + i);
    assert(history.Build(262, output) == 1 && output[0].headAlpha > 0);
    history.Clear();
    history.Sample(2, origin, Point(700, 0, 0), 300);
    assert(history.Build(300, output) == 1);
    assert(output[0].head.x == 700 && output[0].tail.x > 0 && output[0].tailAlpha == 0);
    history.Sample(3, origin, origin, 301);
    assert(history.Build(301, output) == 1);
    history.Clear();
    history.Sample(2, origin, tip, 0xfffffff0u);
    assert(history.Build(4, output) == 1);
    assert(history.Build(100, output) == 0);
    history.Clear();
    for (unsigned i = 0; i < 40; ++i)
        history.Sample(2, Point(float(i)), Point(float(i+1)), 400 + i);
    assert(history.Build(439, output) == SamplesPerTrail - 1);
    assert(output[0].head.x == 40 && output[SamplesPerTrail - 2].tail.x == 33);
    history.Clear();
    for (unsigned i = 1; i < 1000; ++i)
        history.Sample(i, origin, tip, 500);
    assert(history.Build(500, output) == MaxTrails);
    history.Clear();
    assert(history.Build(500, output) == 0);
    puts("Bolt history follows flight samples, fades, clips length and stays bounded");
}
