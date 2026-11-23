#pragma once

#include "BoltTrailHistory.h"

namespace bolt_trail {

struct View {
    Point eye, right, up, forward;
    float pixelScaleX, pixelScaleY, nearDepth;
    bool perspective;
};

struct Ribbon {
    Segment segment;
    Point tailPerPixel, headPerPixel;
};

inline Point Offset(Point p, Point direction, float scale)
{
    return Point(p.x + direction.x*scale, p.y + direction.y*scale, p.z + direction.z*scale);
}

inline float Project(Point position, Point eye, Point axis)
{
    return (position.x-eye.x)*axis.x + (position.y-eye.y)*axis.y + (position.z-eye.z)*axis.z;
}

inline bool MakeRibbon(const View& view, const Segment& input, Ribbon& output)
{
    float tailDepth = Project(input.tail, view.eye, view.forward);
    float headDepth = Project(input.head, view.eye, view.forward);
    if (tailDepth < view.nearDepth && headDepth < view.nearDepth) return false;
    output.segment = input;
    Segment& segment = output.segment;
    if (tailDepth < view.nearDepth) {
        const float t = (view.nearDepth-tailDepth) / (headDepth-tailDepth);
        segment.tail = Toward(input.tail, input.head, t);
        segment.tailAlpha += (input.headAlpha-input.tailAlpha)*t;
        tailDepth = view.nearDepth;
    } else if (headDepth < view.nearDepth) {
        const float t = (view.nearDepth-headDepth) / (tailDepth-headDepth);
        segment.head = Toward(input.head, input.tail, t);
        segment.headAlpha += (input.tailAlpha-input.headAlpha)*t;
        headDepth = view.nearDepth;
    }

    const float tailScale = view.perspective ? tailDepth : 1.0f;
    const float headScale = view.perspective ? headDepth : 1.0f;
    const float dx = (Project(segment.head, view.eye, view.right)/headScale -
                      Project(segment.tail, view.eye, view.right)/tailScale) * view.pixelScaleX;
    const float dy = (Project(segment.head, view.eye, view.up)/headScale -
                      Project(segment.tail, view.eye, view.up)/tailScale) * view.pixelScaleY;
    const float lengthSquared = dx*dx + dy*dy;
    if (!(lengthSquared > 0.0001f)) return false;
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    const Point lateral = Offset(Offset(Point(), view.right, -dy*inverseLength/view.pixelScaleX),
                                view.up, dx*inverseLength/view.pixelScaleY);
    output.tailPerPixel = Offset(Point(), lateral, tailScale);
    output.headPerPixel = Offset(Point(), lateral, headScale);
    return true;
}

}
