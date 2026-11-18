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
    struct Round
    {
        unsigned generation,count;bool bad,unknown;Key best;unsigned __int64 ordinal;
    };
    class Registry
    {
    public:
        enum { MaxEntries=512,Slots=1024,MaxProbes=64,MaxObserved=256 };
        Registry(){Reset(0);}
        void Reset(unsigned generation)
        {std::memset(entries_,0,sizeof(entries_));generation_=generation;count_=0;next_=0;}
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
            Entry& e=entries_[free];e.key=key;e.ordinal=++next_;e.state=1;++count_;return true;
        }
        void Forget(const Key& key)
        {Entry* e=Find(key);if(e){e->state=2;--count_;}}
        unsigned Collect(Key* keys,unsigned limit,unsigned& cursor)const
        {
            if(!count_||!keys||!limit)return 0;
            unsigned found=0;
            for(unsigned n=0;n<MaxProbes&&found<limit;++n)
            {const Entry& e=entries_[cursor&(Slots-1)];cursor=(cursor+1)&(Slots-1);if(e.state==1)keys[found++]=e.key;}
            return found;
        }
        void Begin(Round& q,unsigned generation)const
        {std::memset(&q,0,sizeof(q));q.generation=generation;q.bad=generation!=generation_;}
        void Poison(Round& q)const {q.bad=true;}
        void Observe(Round& q,const Candidate& c)const
        {
            if(q.bad)return;
            if(++q.count>MaxObserved||q.generation!=generation_){q.bad=true;return;}
            if(!Construction(c))return;
            const Entry* e=Find(c.key);
            // A previously existing blueprint has no proven placement age.
            // A fallback is safer than treating observation/handle order as age.
            if(!e){q.unknown=true;return;}
            if(!q.ordinal||e->ordinal<q.ordinal){q.best=c.key;q.ordinal=e->ordinal;}
        }
        bool Select(const Round& q,unsigned generation,const Candidate& native,Key& out)const
        {
            if(q.bad||q.unknown||generation!=generation_||q.generation!=generation||!q.ordinal||!Construction(native))return false;
            const Entry* first=Find(q.best);const Entry* current=Find(native.key);
            if(!first||!current||first->ordinal!=q.ordinal||first->ordinal>=current->ordinal)return false;
            out=q.best;return true;
        }
    private:
        struct Entry {Key key;unsigned __int64 ordinal;unsigned state;};
        Entry entries_[Slots];unsigned generation_,count_;unsigned __int64 next_;
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
