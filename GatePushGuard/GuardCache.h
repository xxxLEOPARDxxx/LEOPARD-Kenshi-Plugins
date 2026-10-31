#pragma once
#include "GuardCore.h"
namespace GatePushGuard {
    inline unsigned Hash(const Key& k) {
        unsigned h=2166136261u;const unsigned words[5]={k.type,k.container,k.containerSerial,k.index,k.serial};
        for(unsigned i=0;i<5;++i) {h^=words[i];h*=16777619u;}return h;
    }
    struct Cooldowns {
        enum { Capacity=1024,Slots=2048 };
        struct Cell {Key key;double until,keepUntil;unsigned revision;bool used;Cell():until(0),keepUntil(0),revision(0),used(false){} };
        Cell cells[Slots];
        double sweptAt;unsigned usedCount,revisionCounter;
        Cooldowns():sweptAt(0),usedCount(0),revisionCounter(0){}
        const Cell* Lookup(const Key& key) const {
            const unsigned start=Hash(key)%Slots;
            for(unsigned n=0;n<Slots;++n) {
                const Cell& c=cells[(start+n)%Slots];if(!c.used)return 0;
                if(c.key==key)return &c;
            }return 0;
        }
        bool Contains(const Key& key,double now) const {const Cell* c=Lookup(key);return c&&now<c->until;}
        unsigned Revision(const Key& key) const {const Cell* c=Lookup(key);return c?c->revision:0;}
        Cell* Slot(const Key& key,double now) {
            unsigned free=Slots;const unsigned start=Hash(key)%Slots;
            for(unsigned n=0;n<Slots;++n) {
                const unsigned i=(start+n)%Slots;Cell& c=cells[i];
                if(c.used&&c.key==key)return &c;
                if(!c.used) {if(free==Slots)free=i;break;}
                if(free==Slots&&now>=c.keepUntil)free=i;
            }
            if(free==Slots)return 0;
            Cell& c=cells[free];
            if(!c.used) {if(usedCount>=Capacity)return 0;++usedCount;}
            c.until=0;c.keepUntil=0;c.revision=0;
            c.key=key;c.used=true;return &c;
        }
        bool Record(const Key& key,double now,double suppress=.5) {
            Cell* c=Slot(key,now);if(!c)return false;
            const double until=now+suppress;if(until>c->until||suppress==0)c->until=until;
            if(now+.5>c->keepUntil)c->keepUntil=now+.5;
            if(++revisionCounter==0)++revisionCounter;c->revision=revisionCounter;return true;
        }
        bool RecordCorrection(const Key& key,double now){return Record(key,now,0);}
        bool Stamp(const Key& key,double now,unsigned& revision) {
            Cell* c=const_cast<Cell*>(Lookup(key));revision=c?c->revision:0;
            // Keep the revision alive while ANY actor history can still be
            // valid. Losing it may otherwise hide a true crossing at expiry.
            if(c&&now+.5>c->keepUntil)c->keepUntil=now+.5;
            return c&&now<c->until;
        }
        void Clear() {for(unsigned i=0;i<Slots;++i)cells[i].used=false;sweptAt=0;usedCount=0;}
        void Sweep(double now) {
            if(now>=sweptAt && now-sweptAt<.5)return;
            // Rehash active entries, so expired tombstones cannot turn every
            // ordinary actor lookup into a full-table scan after a long raid.
            Cell active[Capacity];unsigned count=0;
            for(unsigned i=0;i<Slots;++i)if(cells[i].used&&cells[i].keepUntil>now)active[count++]=cells[i];
            Clear();
            for(unsigned i=0;i<count;++i) {Cell* c=Slot(active[i].key,now);if(c)*c=active[i];}
            sweptAt=now;
        }
    };
    // Fixed storage. No actor pointers, heap growth or retained STL iterators.
    struct Samples {
        enum { Capacity=256 };
        struct Cell { Sample sample; bool used;Cell():used(false){} };
        Cell cells[Capacity];
        double sweptAt;
        Samples():sweptAt(0){}
        void Clear() { for(unsigned i=0;i<Capacity;++i)cells[i].used=false;sweptAt=0; }
        Sample* Find(const Key& key,double now) {
            unsigned firstFree=Capacity;
            const unsigned start=Hash(key)%Capacity;
            for(unsigned n=0;n<Capacity;++n) {
                const unsigned i=(start+n)%Capacity;Cell& c=cells[i];
                if(c.used && c.sample.actor==key) return &c.sample;
                if(!c.used) {if(firstFree==Capacity)firstFree=i;break;}
                if(firstFree==Capacity && (now-c.sample.seconds>1 || now<c.sample.seconds))firstFree=i;
            }
            if(firstFree==Capacity)return 0;
            Cell& c=cells[firstFree];c.used=true;c.sample=Sample();c.sample.actor=key;return &c.sample;
        }
        void Sweep(double now) {
            if(now>=sweptAt&&now-sweptAt<1)return;
            Cell active[Capacity];unsigned count=0;
            for(unsigned i=0;i<Capacity;++i)if(cells[i].used&&cells[i].sample.seconds<=now&&now-cells[i].sample.seconds<=1)active[count++]=cells[i];
            Clear();
            for(unsigned i=0;i<count;++i) {Sample* p=Find(active[i].sample.actor,now);if(p)*p=active[i].sample;}
            sweptAt=now;
        }
    };
}
