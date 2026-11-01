#pragma once
#include <cmath>

// GateFix's delayed native-order approach, without raw lifetime registries or
// forced-close fallbacks. Adapted from Basusu (GPLv3); see UPSTREAM.md.
namespace AIFixedGate
{
    struct Point{float x,y,z;Point(float a=0,float b=0,float c=0):x(a),y(b),z(c){}};
    inline bool Finite(const Point& p){return _finite(p.x)&&_finite(p.y)&&_finite(p.z);}
    inline float DistanceSq(const Point& a,const Point& b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
    inline int Side(const Point& p,const Point& inside,const Point& outside)
    {
        if(!Finite(p)||!Finite(inside)||!Finite(outside))return 0;
        const float nx=outside.x-inside.x,nz=outside.z-inside.z;
        if(nx*nx+nz*nz<.01f)return 0;
        const float dot=(p.x-(inside.x+outside.x)*.5f)*nx+(p.z-(inside.z+outside.z)*.5f)*nz;
        return dot>.1f?1:dot<-.1f?-1:0;
    }
    inline bool Crosses(const Point& from,const Point& dest,const Point& inside,const Point& outside,float corridor)
    {
        const int a=Side(from,inside,outside),b=Side(dest,inside,outside);
        if(!a||!b||a==b||corridor<=0||!_finite(corridor)||std::fabs(from.y-inside.y)>8)return false;
        const float nx=outside.x-inside.x,nz=outside.z-inside.z;
        const float cx=(inside.x+outside.x)*.5f,cz=(inside.z+outside.z)*.5f;
        const float da=(from.x-cx)*nx+(from.z-cz)*nz,db=(dest.x-cx)*nx+(dest.z-cz)*nz;
        const float t=da/(da-db),x=from.x+(dest.x-from.x)*t-cx,z=from.z+(dest.z-from.z)*t-cz;
        return _finite(t)&&x*x+z*z<=corridor*corridor;
    }
    inline bool Ready(unsigned now,unsigned lastInput,unsigned sampledAt,float movedSq)
    {return unsigned(now-lastInput)>=400&&unsigned(now-sampledAt)>=250&&movedSq<=.25f;}
    inline bool Expired(unsigned now,unsigned created){return unsigned(now-created)>30000;}
}
