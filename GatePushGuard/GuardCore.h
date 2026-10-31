#pragma once
#include <cmath>
#include <cstring>

// Independently authored; no code or binary from Gate Push Fix is used.
namespace GatePushGuard
{
    struct Vec { float x,y,z; Vec(float a=0,float b=0,float c=0):x(a),y(b),z(c){} };
    struct Key {
        unsigned type,container,containerSerial,index,serial;
        Key():type(0),container(0),containerSerial(0),index(0),serial(0){}
        bool operator==(const Key& k) const { return type==k.type && container==k.container && containerSerial==k.containerSerial && index==k.index && serial==k.serial; }
    };
    struct Gate {
        Key key; Vec center,normal; float halfWidth,minY,maxY; unsigned epoch; bool closed;
        Gate():halfWidth(0),minY(0),maxY(0),epoch(0),closed(false){}
    };
    struct Observation {
        Key key; Vec position; double seconds; unsigned revision;bool eligible;
        Observation():seconds(0),revision(0),eligible(true){}
    };
    struct Sample {
        Key actor,gate; Vec last,outside; double seconds; unsigned epoch,revision; bool valid,armed,entered;
        Sample():seconds(0),epoch(0),revision(0),valid(false),armed(false),entered(false){}
    };
    struct Action { bool correct; Vec target; Action():correct(false){} };
    struct Leaf {
        Vec points[128];unsigned count;bool overflow;
        Leaf():count(0),overflow(false){}
        void Add(const Vec& v) { if(count<128) points[count++]=v;else overflow=true; }
    };
    inline bool Finite(double n) { return n==n && n>-1.0e20 && n<1.0e20; }
    inline bool Finite(const Vec& p) { return Finite(p.x)&&Finite(p.y)&&Finite(p.z); }
    inline float Side(const Gate& g,const Vec& p) { return (p.x-g.center.x)*g.normal.x+(p.z-g.center.z)*g.normal.z; }
    inline float Distance2(const Vec& a,const Vec& b) { float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z; }
    inline float WatchRadius(const Gate& g) {
        const float halfHeight=(g.maxY-g.minY)*.5f+.5f;
        return std::sqrt(g.halfWidth*g.halfWidth+halfHeight*halfHeight+16.0f)+1.0f;
    }
    inline bool Valid(const Gate& g) {
        const float n=g.normal.x*g.normal.x+g.normal.z*g.normal.z;
        return Finite(g.center)&&Finite(g.normal)&&std::fabs(g.normal.y)<.001f &&
            std::fabs(n-1)<.001f && Finite(g.halfWidth)&&g.halfWidth>.15f&&g.halfWidth<50 &&
            Finite(g.minY)&&Finite(g.maxY)&&g.maxY-g.minY>1 && g.maxY-g.minY<30;
    }
    inline bool FitLeaf(Gate& g,const Vec& plane,const Vec& outside,const Vec& inside,const Leaf& leaf) {
        if(!Finite(plane)||!Finite(outside)||!Finite(inside)||leaf.count<2||leaf.overflow) return false;
        float nx=inside.x-outside.x,nz=inside.z-outside.z;
        const float length=std::sqrt(nx*nx+nz*nz);
        if(!Finite(length)||length<.5f||length>100||std::fabs(inside.y-outside.y)>2) return false;
        nx/=length;nz/=length;
        float loT=1e20f,hiT=-1e20f,loN=1e20f,hiN=-1e20f,loY=1e20f,hiY=-1e20f;
        for(unsigned i=0;i<leaf.count;++i) {
            const Vec& p=leaf.points[i];if(!Finite(p))return false;
            const float t=-(p.x-plane.x)*nz+(p.z-plane.z)*nx;
            const float n=(p.x-plane.x)*nx+(p.z-plane.z)*nz;
            if(t<loT)loT=t;if(t>hiT)hiT=t;if(n<loN)loN=n;if(n>hiN)hiN=n;
            if(p.y<loY)loY=p.y;if(p.y>hiY)hiY=p.y;
        }
        // A thick/offset leaf may be a frame, compound mesh, or incompatible
        // marker layout. Do not turn that uncertainty into a movement write.
        if(hiN-loN>4 || loN>2 || hiN< -2 || std::fabs((loN+hiN)*.5f)>3) return false;
        const float middle=(loT+hiT)*.5f;
        g.center=Vec(plane.x-nz*middle,plane.y,plane.z+nx*middle);
        g.normal=Vec(nx,0,nz);g.halfWidth=(hiT-loT)*.5f;g.minY=loY;g.maxY=hiY;
        return Valid(g) && Side(g,outside)<-.1f && Side(g,inside)>.1f;
    }
    inline void Seed(Sample& s,const Gate& g,const Observation& o) {
        s.actor=o.key;s.gate=g.key;s.epoch=g.epoch;s.revision=o.revision;s.last=o.position;s.seconds=o.seconds;
        s.valid=g.closed&&o.eligible&&Valid(g)&&Finite(o.position)&&Finite(o.seconds);
        s.armed=s.valid&&Side(g,o.position)<=-.05f;
        s.entered=false;
        if(s.armed) s.outside=o.position;
    }
    inline Action Observe(Sample& s,const Gate& g,const Observation& o) {
        Action a;
        if(!s.valid || !g.closed || !o.eligible || !Valid(g) || !Finite(o.position) ||
            !Finite(o.seconds) || !(s.actor==o.key) || !(s.gate==g.key) || s.epoch!=g.epoch || s.revision!=o.revision ||
            o.seconds<=s.seconds || o.seconds-s.seconds>.35 ||
            Distance2(s.last,o.position)>16 || std::fabs(s.last.y-o.position.y)>.8f) {
            Seed(s,g,o);return a;
        }
        const float side=Side(g,o.position);
        const float previous=Side(g,s.last);
        if(s.armed && previous<=0 && side>0) {
            // The ACTUAL consecutive segment must cross the aperture. The
            // hysteresis anchor is only a safe return target; joining it to a
            // later point would wrongly straighten a path around a door edge.
            const float t=-previous/(side-previous);
            Vec hit(s.last.x+(o.position.x-s.last.x)*t,
                    s.last.y+(o.position.y-s.last.y)*t,
                    s.last.z+(o.position.z-s.last.z)*t);
            const float lateral=-(hit.x-g.center.x)*g.normal.z+(hit.z-g.center.z)*g.normal.x;
            s.entered=std::fabs(lateral)<g.halfWidth-.1f && hit.y>=g.minY-.5f && hit.y<=g.maxY-.5f;
            if(!s.entered)s.armed=false;
        }
        if(previous>=0 && side<0)s.entered=false;
        if(side>=.05f) {
            // Only a short, continuous observed movement through the actual leaf
            // can be undone. Never infer entry from proximity or an open gate.
            if(s.armed && s.entered && Side(g,s.outside)<=-.05f &&
                Distance2(s.outside,o.position)<=16 && std::fabs(s.outside.y-o.position.y)<=.8f) {
                a.correct=true;a.target=s.outside;
            }
            s.armed=false;s.entered=false;
        }
        if(side<=-.05f) { s.outside=o.position;s.armed=true;s.entered=false; }
        s.last=o.position;s.seconds=o.seconds;
        return a;
    }
}
