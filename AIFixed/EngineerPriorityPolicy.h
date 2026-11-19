#pragma once
#include <cstring>

// Pure POD policy. The native adapter serialises Registry access; this header
// never calls the game, allocates, logs, scans a world or persists save data.
namespace AIFixedEngineerPriority
{
    struct Key { unsigned type,container,containerSerial,index,serial; };
    struct Candidate { Key key;bool productive,wall,complete,paused,dismantled; };
    inline bool Equal(const Key& a,const Key& b)
    {return a.type==b.type&&a.container==b.container&&a.containerSerial==b.containerSerial&&a.index==b.index&&a.serial==b.serial;}
    inline bool Construction(const Candidate& c)
    {return c.key.type==0&&c.productive&&!c.wall&&!c.complete&&!c.paused&&!c.dismantled;}
    inline bool OtherConstruction(const Candidate& c)
    {return c.key.type==0&&!c.paused&&!c.dismantled&&!Construction(c);}
    struct Round
    {
        unsigned generation,count;bool bad,unknown,started,waiting,trackWaiting,hasAlternative;
        Key best,alternative;unsigned __int64 ordinal;
    };
    class Registry
    {
    public:
        enum { MaxEntries=512,Slots=1024,MaxProbes=64,MaxObserved=256 };
        Registry(){Reset(0);}
        void Reset(unsigned generation)
        {std::memset(entries_,0,sizeof(entries_));generation_=generation;count_=startedCount_=0;next_=0;}
        bool Record(const Key& key)
        {
            if(key.type!=0)return false;
            const unsigned hash=Hash(key);unsigned free=Slots;
            for(unsigned n=0;n<MaxProbes;++n)
            {
                const unsigned at=(hash+n)&(Slots-1);Entry& e=entries_[at];
                if(e.state==1&&Equal(e.key,key))return true;
                if(e.state==2&&free==Slots)free=at;
                if(!e.state){if(free==Slots)free=at;break;}
            }
            if(free==Slots||count_>=MaxEntries||next_==~static_cast<unsigned __int64>(0))return false;
            Entry& e=entries_[free];e.key=key;e.ordinal=++next_;e.started=0;e.state=1;++count_;return true;
        }
        void Start(const Key& key)
        {
            if(!Record(key))return;
            Entry* e=Find(key);
            if(e&&!e->started&&next_!=~static_cast<unsigned __int64>(0)){e->started=++next_;++startedCount_;}
        }
        bool Started(const Key& key)const
        {const Entry* e=Find(key);return e&&e->started;}
        bool Forget(const Key& key)
        {
            Entry* e=Find(key);if(!e)return false;
            if(e->started)--startedCount_;
            e->state=2;--count_;return true;
        }
        unsigned Collect(Key* keys,unsigned limit,unsigned& cursor)const
        {
            if(!count_||!keys||!limit)return 0;
            unsigned found=0;
            for(unsigned n=0;n<MaxProbes&&found<limit;++n)
            {const Entry& e=entries_[cursor&(Slots-1)];cursor=(cursor+1)&(Slots-1);if(e.state==1)keys[found++]=e.key;}
            return found;
        }
        void Begin(Round& q,unsigned generation)const
        {std::memset(&q,0,sizeof(q));q.generation=generation;q.bad=generation!=generation_;q.trackWaiting=startedCount_!=0;}
        void Poison(Round& q)const {q.bad=true;}
        void Observe(Round& q,const Candidate& c,bool waiting=false)const
        {
            if(q.bad)return;
            if(++q.count>MaxObserved||q.generation!=generation_){q.bad=true;return;}
            if(!Construction(c))
            {
                if(!waiting&&!q.hasAlternative&&OtherConstruction(c)){q.alternative=c.key;q.hasAlternative=true;}
                return;
            }
            const Entry* e=Find(c.key);
            // A previously existing blueprint has no proven placement age.
            // A fallback is safer than treating observation/handle order as age.
            if(!e){if(!waiting)q.unknown=true;return;}
            const bool started=e->started!=0;
            if(waiting&&!started)return;
            const unsigned __int64 ordinal=started?e->started:e->ordinal;
            if(!q.ordinal||(started&&!q.started)||(started==q.started&&ordinal<q.ordinal))
            {q.best=c.key;q.ordinal=ordinal;q.started=started;q.waiting=waiting;}
            else if(started==q.started&&ordinal==q.ordinal&&!waiting)q.waiting=false;
        }
        bool Select(const Round& q,unsigned generation,const Candidate& native,Key& out)const
        {
            if(q.bad||(q.unknown&&!q.started)||generation!=generation_||q.generation!=generation||!q.ordinal||!Construction(native))return false;
            const Entry* first=Find(q.best);const Entry* current=Find(native.key);
            if(!first||(first->started!=0)!=q.started||(q.started?first->started:first->ordinal)!=q.ordinal||first==current)return false;
            // The first already elected machine wins only among candidates in
            // this engineer's native local search. Unknown ages stay unknown.
            if(q.started){if(current&&current->started&&current->started<q.ordinal)return false;}
            else if(!current||current->started||current->ordinal<=q.ordinal)return false;
            out=q.best;return true;
        }
    private:
        struct Entry {Key key;unsigned __int64 ordinal,started;unsigned state;};
        Entry entries_[Slots];unsigned generation_,count_,startedCount_;unsigned __int64 next_;
        static unsigned Hash(const Key& k)
        {
            unsigned h=2166136261u;const unsigned values[5]={k.type,k.container,k.containerSerial,k.index,k.serial};
            for(unsigned i=0;i<5;++i){h^=values[i];h*=16777619u;h^=h>>16;}
            return h;
        }
        const Entry* Find(const Key& key)const
        {
            const unsigned hash=Hash(key);
            for(unsigned n=0;n<MaxProbes;++n)
            {const Entry& e=entries_[(hash+n)&(Slots-1)];if(!e.state)return 0;if(e.state==1&&Equal(e.key,key))return &e;}
            return 0;
        }
        Entry* Find(const Key& key){return const_cast<Entry*>(static_cast<const Registry*>(this)->Find(key));}
    };
}
