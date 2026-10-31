#pragma once
#include "GuardCache.h"
namespace GatePushGuard {
    struct ActorEvent { Key key;Vec position;double seconds;ActorEvent():seconds(0){} };
    struct EventQueue {
        enum { Capacity=2048,Slots=4096 };
        struct Cell {ActorEvent event;bool used;Cell():used(false){} };
        Cell cells[Slots];unsigned usedCount;
        EventQueue():usedCount(0){}
        bool Push(const ActorEvent& event) {
            if(!Finite(event.position)||!Finite(event.seconds))return false;
            // At saturation even repeated actors are dropped for this batch.
            // Reject in O(1), rather than scanning a full table for every extra
            // actor in a large raid. Table load never exceeds one half.
            if(usedCount>=Capacity)return false;
            const unsigned start=Hash(event.key)%Slots;
            for(unsigned n=0;n<Slots;++n) {
                Cell& c=cells[(start+n)%Slots];
                if(!c.used) {c.event=event;c.used=true;++usedCount;return true;}
                if(c.event.key==event.key) {if(event.seconds>=c.event.seconds)c.event=event;return true;}
            }
            return false;
        }
        unsigned Drain(ActorEvent (&output)[Capacity]) {
            unsigned count=0;
            for(unsigned i=0;i<Slots;++i)if(cells[i].used) {output[count++]=cells[i].event;cells[i].used=false;}
            usedCount=0;
            return count;
        }
        void Clear(){for(unsigned i=0;i<Slots;++i)cells[i].used=false;usedCount=0;}
    };
    struct RegionIndex {
        enum { MaxRegions=64,Cells=512 };
        struct Region {Vec center;float radius;bool active;Region():radius(0),active(false){} };
        struct Cell {int x,z;bool used;Cell():x(0),z(0),used(false){} };
        Region regions[MaxRegions];Cell cells[Cells];
        static unsigned Bucket(int x,int z){return ((unsigned)x*73856093u ^ (unsigned)z*19349663u)%Cells;}
        static bool Supported(const Vec& p){return Finite(p)&&std::fabs(p.x)<1e7f&&std::fabs(p.z)<1e7f;}
        static int Coordinate(float x){return (int)std::floor(x/128.0f);}
        void Insert(int x,int z) {
            const unsigned start=Bucket(x,z);
            for(unsigned n=0;n<Cells;++n) {Cell& c=cells[(start+n)%Cells];if(c.used&&c.x==x&&c.z==z)return;if(!c.used){c.x=x;c.z=z;c.used=true;return;}}
        }
        void Rebuild() {
            for(unsigned i=0;i<Cells;++i)cells[i].used=false;
            for(unsigned i=0;i<MaxRegions;++i)if(regions[i].active) {
                const Region& r=regions[i];
                for(int x=Coordinate(r.center.x-r.radius);x<=Coordinate(r.center.x+r.radius);++x)
                    for(int z=Coordinate(r.center.z-r.radius);z<=Coordinate(r.center.z+r.radius);++z)Insert(x,z);
            }
        }
        void Set(unsigned slot,bool active,const Vec& center,float radius){
            if(slot>=MaxRegions)return;Region& r=regions[slot];
            active=active&&Supported(center)&&Finite(radius)&&radius>0&&radius<64;
            if(r.active==active&&(!active||(Distance2(r.center,center)<.0001f&&r.radius==radius)))return;
            r.active=active;r.center=center;r.radius=radius;Rebuild();
        }
        bool Contains(const Vec& position) const{
            if(!Supported(position))return false;
            const int x=Coordinate(position.x),z=Coordinate(position.z);const unsigned start=Bucket(x,z);
            for(unsigned n=0;n<Cells;++n){const Cell& c=cells[(start+n)%Cells];if(!c.used)return false;if(c.x==x&&c.z==z)return true;}return false;
        }
        void Clear(){for(unsigned i=0;i<MaxRegions;++i)regions[i].active=false;for(unsigned i=0;i<Cells;++i)cells[i].used=false;}
    };
}
