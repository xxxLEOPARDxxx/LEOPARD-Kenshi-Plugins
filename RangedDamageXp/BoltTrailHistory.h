// SPDX-License-Identifier: GPL-3.0-or-later
// Bolt streaks adapted from MeitouClient by VlcekM and contributors. See ORIGIN.md.
#pragma once

#include "BoltTrails.h"

#include <cmath>
#include <cstddef>

namespace bolt_trail {

const float FlightSeconds = 0.2f;
const float FlightLengthLimit = 500.0f;
const float StopSeconds = 0.15f;
const float Width = 0.7f;
const float BaseOpacity = 0.6f;

struct Point {
    float x, y, z;
    Point(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
};

inline Point operator+(Point a, Point b) { return Point(a.x+b.x, a.y+b.y, a.z+b.z); }
inline Point operator-(Point a, Point b) { return Point(a.x-b.x, a.y-b.y, a.z-b.z); }
inline Point operator*(Point p, float scale) { return Point(p.x*scale, p.y*scale, p.z*scale); }
inline float LengthSquared(Point p) { return p.x*p.x + p.y*p.y + p.z*p.z; }
inline float Length(Point p) { return std::sqrt(LengthSquared(p)); }
inline bool InWorld(Point p)
{
    const float bound = 10000000.0f;
    return p.x >= -bound && p.x <= bound && p.y >= -bound && p.y <= bound &&
           p.z >= -bound && p.z <= bound;
}

struct Streak { Point head, tail; };

class History {
    enum { Ways = 4 };

    struct Trail {
        uintptr_t bolt;
        Point muzzle, head, direction;
        float speed, age, stoppedLength;
        unsigned frame;
        bool stopped;
        double stoppedAt;

        float FlyingLength(unsigned percent) const
        {
            const float scale = float(percent) / 100.0f;
            const float cap = speed * FlightSeconds < FlightLengthLimit
                ? speed * FlightSeconds : FlightLengthLimit;
            const float distance = Length(head-muzzle);
            return distance < cap*scale ? distance : cap*scale;
        }

        float Remaining(double now) const
        {
            if (now < stoppedAt) return 0;
            const float minSpeed = stoppedLength / StopSeconds;
            return stoppedLength - float(now-stoppedAt)*(speed > minSpeed ? speed : minSpeed);
        }

        void Stop(Point at, double now, unsigned percent)
        {
            // Keep the tail in place on impact, then move it towards the head.
            const Point axis = at - (head - direction * FlyingLength(percent));
            head = at;
            stoppedLength = Length(axis);
            if (stoppedLength > 0.001f) direction = axis * (1.0f/stoppedLength);
            stoppedAt = now;
            stopped = true;
        }
    };

    Trail trails[MaxTrails];
    unsigned active, limit, buckets;

public:
    History() : limit(0) { SetLimit(DefaultTrailLimit); }

    void SetLimit(unsigned requested)
    {
        const unsigned next = ClampTrailLimit(requested);
        if (limit == next) return;
        limit = next;
        buckets = (next + Ways - 1) / Ways;
        // Changing the bucket count changes bolt lookup. Start tracking again.
        Clear();
    }

    void Clear()
    {
        for (unsigned i = 0; i < limit; ++i) trails[i].bolt = 0;
        active = 0;
    }

    bool Active() const { return active != 0; }

    void Observe(uintptr_t bolt, Point before, Point head, Point direction, float speed,
                 float ageBefore, float ageAfter, bool flying, bool hit,
                 unsigned frame, double now, unsigned lengthPercent)
    {
        const float directionSq = LengthSquared(direction);
        if (!bolt || !InWorld(before) || !InWorld(head) || !(speed > 0 && speed < 1000000) ||
            !(directionSq > 0.5f && directionSq < 1.5f)) return;

        const unsigned hash = unsigned(bolt ^ (bolt >> 4) ^ (bolt >> 9));
        const unsigned bucket = (hash % buckets) * Ways;
        const unsigned end = bucket + Ways < limit ? bucket + Ways : limit;
        Trail* available = NULL;
        Trail* flight = NULL;
        for (unsigned i = bucket; i < end; ++i) {
            Trail& candidate = trails[i];
            if (!candidate.bolt || (candidate.stopped && candidate.Remaining(now) <= 0)) {
                if (!available) available = &candidate;
            } else if (candidate.bolt == bolt && !candidate.stopped) {
                if (ageBefore < candidate.age) {
                    candidate.Stop(candidate.head, now, lengthPercent);
                } else {
                    flight = &candidate;
                    break;
                }
            }
        }
        if (!flight && (!available || !flying)) return;
        direction = direction * (1.0f/std::sqrt(directionSq));
        if (!flight) {
            flight = available;
            if (!flight->bolt) ++active;
            flight->bolt = bolt;
            flight->muzzle = before;
            flight->stopped = false;
            flight->head = before;
            flight->direction = direction;
            flight->speed = speed;
        }
        flight->frame = frame;
        flight->age = ageAfter;
        if (flying) {
            flight->head = head;
            flight->direction = direction;
            flight->speed = speed;
        } else {
            flight->Stop(hit ? head : flight->head, now, lengthPercent);
        }
    }

    unsigned Build(unsigned frame, double now, bool timeAdvanced, unsigned lengthPercent,
                   Streak (&out)[MaxTrails])
    {
        unsigned used = 0;
        for (unsigned i = 0; i < limit; ++i) {
            Trail& trail = trails[i];
            if (!trail.bolt) continue;
            if (timeAdvanced && !trail.stopped && trail.frame != frame)
                trail.Stop(trail.head, now, lengthPercent);
            const float length = trail.stopped ? trail.Remaining(now) : trail.FlyingLength(lengthPercent);
            if (length > 0.001f) {
                Streak& result = out[used++];
                result.head = trail.head;
                result.tail = trail.head - trail.direction * length;
            } else if (trail.stopped) {
                trail.bolt = 0;
                --active;
            }
        }
        return used;
    }
};

}
