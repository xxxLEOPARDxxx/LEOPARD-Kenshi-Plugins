// SPDX-License-Identifier: GPL-3.0-or-later
// Bolt streaks adapted from MeitouClient by VlcekM and contributors. See ORIGIN.md.
#pragma once

#include "BoltTrailHistory.h"

namespace bolt_trail {

const unsigned MaskWidth = 16, MaskHeight = 64, MaskMipLevels = 6;
const float DrawRange = 20400.0f;

inline Point Cross(Point a, Point b)
{
    return Point(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);
}

struct Quad { Point headLeft, headRight, tailRight, tailLeft; };

inline bool MakeQuad(const Streak& streak, Point eye, Point cameraRight, Quad& quad)
{
    const Point axis = streak.head-streak.tail;
    const float length = Length(axis);
    if (!(length > 0.001f) || LengthSquared(streak.head-eye) > (DrawRange+length)*(DrawRange+length))
        return false;
    const Point centre = (streak.head+streak.tail)*0.5f;
    const Point side = Cross(axis*(1.0f/length), eye-centre);
    const float sideLength = Length(side);
    const Point halfWidth = (sideLength > 0.00001f ? side*(1.0f/sideLength) : cameraRight)*(Width*0.5f);
    quad.headLeft = streak.head-halfWidth;
    quad.headRight = streak.head+halfWidth;
    quad.tailRight = streak.tail+halfWidth;
    quad.tailLeft = streak.tail-halfWidth;
    return true;
}

inline unsigned char ByteFromUnit(float value)
{
    const float scaled = value*255.0f;
    const unsigned low = unsigned(scaled);
    const float fraction = scaled-float(low);
    return static_cast<unsigned char>(low + (fraction > 0.5f || (fraction == 0.5f && (low & 1))));
}

inline void MakeMask(unsigned char* rgba, unsigned fadePercent, bool outline)
{
    const float fade = float(fadePercent)/100.0f;
    for (unsigned y = 0; y < MaskHeight; ++y) {
        const float v = (float(y)+0.5f)/MaskHeight;
        const float tail = (1.0f-v)/fade, tip = v/0.04f;
        const float along = (tail < 1.0f ? tail : 1.0f) * (tip < 1.0f ? tip : 1.0f);
        for (unsigned x = 0; x < MaskWidth; ++x) {
            const float across = (float(x)+0.5f)*2.0f/MaskWidth-1.0f;
            const unsigned char rgb = outline && std::abs(across) > 0.68f ? 18 : 255;
            const unsigned at = (y*MaskWidth+x)*4;
            rgba[at] = rgba[at+1] = rgba[at+2] = rgb;
            rgba[at+3] = ByteFromUnit(along*(1.0f-across*across));
        }
    }
}

}
