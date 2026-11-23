#pragma once

#include "BoltTrails.h"

#include <stdint.h>
#include <cmath>
#include <cstddef>

namespace bolt_trail {

const unsigned MaxTrails = 64;
const unsigned SamplesPerTrail = 8;
const unsigned MaxSegments = MaxTrails * SamplesPerTrail; // One extra segment for the fade boundary.
const uint32_t TrailLifetimeMs = 100;
const float MaxTrailLength = 180.0f;

struct Point {
    float x, y, z;
    Point(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
};

inline float DistanceSquared(Point a, Point b)
{
    const float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
    return x*x + y*y + z*z;
}

inline Point Toward(Point a, Point b, float t)
{
    return Point(a.x + (b.x-a.x)*t, a.y + (b.y-a.y)*t, a.z + (b.z-a.z)*t);
}

struct Segment {
    Point tail, head;
    float tailAlpha, headAlpha;
};

class History {
    enum { Ways = 4, Buckets = MaxTrails / Ways };

    struct Trail {
        uintptr_t bolt;
        unsigned start, count;
        uint32_t at[SamplesPerTrail];
        Point points[SamplesPerTrail];

        void Push(Point point, uint32_t now)
        {
            const unsigned end = (start + count) % SamplesPerTrail;
            points[end] = point;
            at[end] = now;
            if (count == SamplesPerTrail) start = (start + 1) % SamplesPerTrail;
            else ++count;
        }
        unsigned Last() const { return (start + count - 1) % SamplesPerTrail; }
    };

    Trail trails[MaxTrails];

    static bool InWorld(Point p)
    {
        const float bound = 10000000.0f;
        return p.x >= -bound && p.x <= bound && p.y >= -bound && p.y <= bound &&
               p.z >= -bound && p.z <= bound;
    }

    static unsigned Bucket(uintptr_t bolt)
    {
        const uintptr_t mixed = bolt ^ (bolt >> 4) ^ (bolt >> 9);
        return unsigned(mixed % Buckets) * Ways;
    }

public:
    History() { Clear(); }

    void Clear()
    {
        for (unsigned i = 0; i < MaxTrails; ++i) trails[i].count = 0;
    }

    void Sample(uintptr_t bolt, Point from, Point to, uint32_t now)
    {
        if (!bolt || !InWorld(from) || !InWorld(to)) return;
        const float step = DistanceSquared(from, to);
        if (step < 0.0625f || step > 4096.0f * 4096.0f) return;

        const unsigned bucket = Bucket(bolt);
        Trail* vacant = NULL;
        Trail* live = NULL;
        for (unsigned i = bucket; i < bucket + Ways; ++i) {
            Trail& candidate = trails[i];
            if (!candidate.count || now - candidate.at[candidate.Last()] >= TrailLifetimeMs) {
                if (!vacant) vacant = &candidate;
            } else if (candidate.bolt == bolt) {
                live = &candidate;
                break;
            }
        }
        Trail* trail = live ? live : vacant;
        if (!trail) return;
        // A reused native address or a discontinuity must start its own trail.
        if (!live || DistanceSquared(trail->points[trail->Last()], from) > 0.0001f) {
            trail->start = trail->count = 0;
            trail->bolt = bolt;
            trail->Push(from, now);
        }
        trail->Push(to, now);
    }

    unsigned Build(uint32_t now, Segment (&out)[MaxSegments], const Appearance& appearance) const
    {
        const float maxLength = MaxTrailLength * float(appearance.lengthPercent) / 100.0f;
        const uint32_t maxHistoryAge = appearance.lengthPercent > 100 ? appearance.lengthPercent : TrailLifetimeMs;
        const float brightness = float(appearance.brightnessPercent) / 100.0f;
        unsigned used = 0;
        for (unsigned t = 0; t < MaxTrails; ++t) {
            const Trail& trail = trails[t];
            if (trail.count < 2) continue;
            unsigned head = trail.Last();
            const uint32_t elapsed = now - trail.at[head];
            if (elapsed >= TrailLifetimeMs) continue;
            const float opacity = brightness * (1.0f - float(elapsed) / TrailLifetimeMs);
            float distances[SamplesPerTrail] = {0};
            const unsigned first = used;
            unsigned count = 0;
            for (unsigned i = 1; i < trail.count; ++i) {
                const unsigned tail = (head + SamplesPerTrail - 1) % SamplesPerTrail;
                if (i > 1 && now - trail.at[tail] >= maxHistoryAge) break;
                const Point a = trail.points[head], b = trail.points[tail];
                const float length = std::sqrt(DistanceSquared(a, b));
                const float room = maxLength - distances[count];
                const bool clipped = length >= room;
                Segment& segment = out[used++];
                segment.head = a;
                segment.tail = clipped ? Toward(a, b, room / length) : b;
                ++count;
                distances[count] = distances[count-1] + (clipped ? room : length);
                if (clipped) break;
                head = tail;
            }
            if (!count) continue;
            const float total = distances[count];
            const float fadeLength = total * float(appearance.fadePercent) / 100.0f;
            const float fadeStart = total - fadeLength;
            for (unsigned i = 0; i < count; ++i) {
                Segment& segment = out[first+i];
                const float from = distances[i], to = distances[i+1];
                segment.headAlpha = from > fadeStart ? opacity * (total-from) / fadeLength : opacity;
                segment.tailAlpha = to > fadeStart ? opacity * (total-to) / fadeLength : opacity;
                if (from < fadeStart && fadeStart < to) {
                    const Point startFade = Toward(segment.head, segment.tail, (fadeStart-from) / (to-from));
                    Segment& fading = out[used++];
                    fading.head = startFade;
                    fading.tail = segment.tail;
                    fading.headAlpha = opacity;
                    fading.tailAlpha = segment.tailAlpha;
                    segment.tail = startFade;
                    segment.tailAlpha = opacity;
                }
            }
        }
        return used;
    }
};

}
