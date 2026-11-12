#pragma once
#include "GatePolicy.h"

namespace AIFixedUnstuck
{
    using AIFixedGate::Point;
    using AIFixedGate::WorldUnits;

    struct Observation
    {
        Point anchor;
        double since, previous, nextProbe, guardFailure;
        unsigned sampledAt;
        bool active;

        Observation():since(0),previous(0),nextProbe(0),guardFailure(0),sampledAt(0),active(false){}

        bool Sample(const Point& position,double hours,unsigned now)
        {
            if (!_finite(hours) || !AIFixedGate::Finite(position))
            {
                active=false;
                return false;
            }
            const float tolerance=WorldUnits(.5f);
            if (!active || hours<previous || hours-previous>.75 ||
                unsigned(now-sampledAt)>=15000 ||
                AIFixedGate::DistanceSq(anchor,position)>tolerance*tolerance)
            {
                anchor=position;
                since=hours;
                nextProbe=hours+4.0;
                guardFailure=hours-2.0;
                active=true;
            }
            previous=hours;
            sampledAt=now;
            return hours>=nextProbe;
        }

        bool SampleGuard(const Point& position,double hours,unsigned now,bool startsAttempt,bool failed)
        {
            if (!active && !startsAttempt) return false;
            const bool continued=active;
            const double started=since;
            const bool due=Sample(position,hours,now);
            if (!active || ((!continued || since!=started) && !startsAttempt))
            {
                active=false;
                return false;
            }
            if (startsAttempt && failed) guardFailure=hours;
            return due && guardFailure>=since && hours-guardFailure<=1.0;
        }

        void Probed(bool exhausted) { nextProbe=previous+(exhausted?.75:.05); }
    };

    inline bool LandingAllowed(const Point& origin,const Point& target,float surface,float maximumDrop=8.0f,
                               float maximumDistance=6.0f)
    {
        if (!AIFixedGate::Finite(origin) || !AIFixedGate::Finite(target) || !_finite(surface))
            return false;
        const float dx=target.x-origin.x,dz=target.z-origin.z;
        const float drop=origin.y-target.y;
        return dx*dx+dz*dz<=WorldUnits(maximumDistance)*WorldUnits(maximumDistance) &&
            drop>=-WorldUnits(1.5f) && drop<=WorldUnits(maximumDrop) &&
            std::fabs(target.y-surface)<=WorldUnits(.3f) &&
            AIFixedGate::DistanceSq(origin,target)>=WorldUnits(.25f)*WorldUnits(.25f);
    }

    inline bool EntersSolid(float distance,float normalAlongRay,float span)
    {
        const float margin=WorldUnits(.15f);
        return _finite(distance) && _finite(normalAlongRay) &&
            distance>=margin && distance<=span-margin && normalAlongRay<-.1f;
    }

    inline bool OpposedRockFaces(unsigned directions)
    {
        unsigned pairs=(directions & (directions>>4)) & 0x0fu;
        return (pairs & (pairs-1u))!=0;
    }

    inline bool AboveGateDeck(float height,float ground,float deck)
    {
        return _finite(height) && _finite(ground) && _finite(deck) &&
            height-ground>=WorldUnits(5.0f) && height-ground<=WorldUnits(14.0f) &&
            height-deck>=WorldUnits(5.0f);
    }

    inline bool IsSteepGuardSlope(const Point& normal)
    {
        return AIFixedGate::Finite(normal) && normal.y>=.2f && normal.y<=.60f;
    }

    inline bool GuardLeavesSlope(const Point& from,const Point& to,const Point& normal)
    {
        return AIFixedGate::Finite(from) && AIFixedGate::Finite(to) && IsSteepGuardSlope(normal) &&
            (to.x-from.x)*normal.x+(to.z-from.z)*normal.z>=WorldUnits(.2f);
    }

    inline bool LeavesSupportingWall(float from,float to,float outsideSide,bool exteriorConfirmed)
    {
        if (!_finite(from) || !_finite(to) || !_finite(outsideSide)) return false;
        const float clearance=WorldUnits(.1f);
        if ((from<0.0f && to<=from-clearance) || (from>0.0f && to>=from+clearance))
            return true;
        // A character on the crest may descend to the native exterior.
        return exteriorConfirmed && std::fabs(from)<=WorldUnits(2.0f) &&
            ((outsideSide<0.0f && to<=-clearance) || (outsideSide>0.0f && to>=clearance));
    }

    inline bool PastSameWallEnd(float from,float to,float length)
    {
        if (!_finite(from) || !_finite(to) || !_finite(length) || length<=0.0f) return false;
        const float margin=WorldUnits(1.0f);
        return (from<=-margin && to<=-margin) ||
            (from>=length+margin && to>=length+margin);
    }

    inline bool OverlapsStructure(const Point& from,const Point& to,const Point& center,const Point& half)
    {
        if (half.x<0 || half.y<0 || half.z<0) return true;
        const float padding=WorldUnits(.2f);
        const float a[]={from.x,from.y,from.z}, b[]={to.x,to.y,to.z};
        const float c[]={center.x,center.y,center.z}, size[]={half.x,half.y,half.z};
        for (unsigned i=0;i<3;++i)
            if ((a[i]>c[i]+size[i]+padding && b[i]>c[i]+size[i]+padding) ||
                (a[i]<c[i]-size[i]-padding && b[i]<c[i]-size[i]-padding)) return false;
        return true;
    }
}
