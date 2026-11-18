#pragma once
#include <cstring>
#include <float.h>
#include <cmath>

namespace AIFixedEngineer
{
    inline int StorageCapacity(double capacity,double carried,double unit)
    {
        // This only caps expansion. Unknown/already overloaded retains the
        // native build-material batch2 rather than making engineering fail.
        if(!_finite(capacity)||!_finite(carried)||!_finite(unit)||capacity<=0||carried<0||unit<=0)return 2;
        const double free=capacity-carried;if(free<=0)return 2;
        if(free>=unit*32.0)return 32;
        const double units=std::floor(free/unit);return units>=1.0?static_cast<int>(units):2;
    }
    struct Identity
    {
        unsigned type,container,containerSerial,index,serial;
        Identity():type(11),container(0),containerSerial(0),index(0),serial(0){}
        bool operator==(const Identity& b) const
        {return type==b.type&&container==b.container&&containerSerial==b.containerSerial&&index==b.index&&serial==b.serial;}
        bool operator!=(const Identity& b) const{return !(*this==b);}
        bool Valid() const{return type!=11;}
    };
    struct Material
    {
        unsigned type;char id[160];
        Material():type(0){std::memset(id,0,sizeof(id));}
        bool operator==(const Material& b)const{return type==b.type&&std::memcmp(id,b.id,sizeof(id))==0;}
        bool Valid()const{return id[0]!=0&&id[159]==0;}
    };
    struct Request
    {
        Identity actor,project,source;Material material;
        int need,carried,available,capacity;unsigned now,epoch;bool ground;
        Request():need(0),carried(0),available(0),capacity(32),now(0),epoch(0),ground(false){}
    };
    struct Owned
    {
        Identity actor,project;Material material;
        int owned,baseline,expectedTotal;unsigned token,epoch;
        Owned():owned(0),baseline(0),expectedTotal(0),token(0),epoch(0){}
    };
    class Coordinator
    {
        struct Claim
        {
            Request request;int quantity,sourceUnits;unsigned started,observed,checked;bool used;
            Claim():quantity(0),sourceUnits(0),started(0),observed(0),checked(0),used(false){}
        };
        Claim claims[128];Owned owned[128];unsigned epoch,serial;
        static int Min(int a,int b){return a<b?a:b;}
        static int Positive(int x){return x>0?x:0;}
        static int Sub(int a,int b){return a>b?a-b:0;}
        bool Valid(const Request& r)const
        {return r.epoch==epoch&&r.actor.Valid()&&r.project.Valid()&&r.source.Valid()&&r.material.Valid()&&
            r.need>0&&r.need<=1000000&&r.carried>=0&&r.carried<=1000000&&r.available>0&&r.available<=1000000&&
            r.capacity>=1&&r.capacity<=32;}
        bool Live(const Claim& c,unsigned now)const
        {return c.used&&c.request.epoch==epoch&&unsigned(now-c.started)<LeaseMs;}
        int Find(const Identity& actor,const Material* material=NULL)const
        {for(unsigned i=0;i<Capacity;++i)if(claims[i].used&&claims[i].request.actor==actor&&
            (!material||claims[i].request.material==*material))return int(i);return -1;}
        int FindOwned(const Identity& actor,const Material& material)const
        {for(unsigned i=0;i<Capacity;++i)if(owned[i].token&&owned[i].actor==actor&&owned[i].material==material)return int(i);return -1;}
        unsigned Token(){if(++serial==0)++serial;return serial;}
        void EmptyPlaceholders()
        {
            for(unsigned i=0;i<Capacity;++i)if(owned[i].token&&!owned[i].owned)
            {
                bool reserved=false;
                for(unsigned j=0;j<Capacity;++j)if(claims[j].used&&(claims[j].quantity>0||claims[j].sourceUnits>0)&&claims[j].request.actor==owned[i].actor&&
                    claims[j].request.material==owned[i].material){reserved=true;break;}
                if(!reserved)owned[i]=Owned();
            }
        }
        int Calculate(const Request& r)const
        {
            if(!Valid(r))return 0;
            int need=Sub(r.need,r.carried),stock=r.available;
            for(unsigned i=0;i<Capacity;++i)
            {
                const Claim& c=claims[i];
                if(!Live(c,r.now)||c.request.actor==r.actor||!(c.request.material==r.material))continue;
                if(c.request.project==r.project)need=Sub(need,c.quantity);
                if(c.request.source==r.source)
                {
                    if(r.ground&&c.sourceUnits)return 0;
                    stock=Sub(stock,c.sourceUnits);
                }
            }
            return Min(Batch,Min(r.capacity,Min(need,stock)));
        }
        int Store(const Request& r,bool requireExisting)
        {
            if(!Valid(r))return 0;
            Expire(r.now,r.epoch);int slot=Find(r.actor,&r.material);
            if(requireExisting)
            {
                bool committed=false;
                for(unsigned i=0;i<Capacity;++i)if(claims[i].used&&claims[i].request.actor==r.actor&&claims[i].request.project==r.project)
                {committed=true;break;}
                if(!committed)return 0;
            }
            int quantity=Calculate(r);
            if(!quantity)
            {
                if(slot>=0){claims[slot]=Claim();if(r.carried>0)Cargo(r);}
                EmptyPlaceholders();return 0;
            }
            if(slot<0)for(unsigned i=0;i<Capacity;++i)if(!claims[i].used){slot=int(i);break;}
            if(slot<0&&requireExisting)for(unsigned i=0;i<Capacity;++i)
                if(claims[i].used&&claims[i].request.actor==r.actor&&claims[i].sourceUnits>0&&claims[i].request.carried==0)
                {slot=int(i);break;}
            if(slot<0)return 0;
            int ledger=FindOwned(r.actor,r.material);
            if(ledger<0)for(unsigned i=0;i<Capacity;++i)if(!owned[i].token){ledger=int(i);break;}
            if(ledger<0&&requireExisting)for(unsigned i=0;i<Capacity;++i)
                if(claims[i].used&&claims[i].request.actor==r.actor&&claims[i].sourceUnits>0&&claims[i].request.carried==0)
                {int old=FindOwned(r.actor,claims[i].request.material);if(old>=0&&!owned[old].owned){owned[old]=Owned();ledger=old;break;}}
            if(ledger<0)return 0; // Never acquire managed cargo with no ownership capacity.
            // One native collection action selects one output type. If its
            // actual type differs from the pre-trip proposal, retire only our
            // uncollected part of the old type; keep every real carried type.
            if(requireExisting)for(unsigned i=0;i<Capacity;++i)
                if(int(i)!=slot&&claims[i].used&&claims[i].request.actor==r.actor&&
                    claims[i].request.project==r.project&&claims[i].sourceUnits>0)
                {
                    Claim& old=claims[i];old.sourceUnits=0;old.quantity=Min(old.request.need,old.request.carried);
                    old.request.source=old.request.actor;if(!old.quantity)old=Claim();
                }
            if(!owned[ledger].token)
            {
                Owned& o=owned[ledger];o.actor=r.actor;o.project=r.project;o.material=r.material;
                o.baseline=r.carried;o.expectedTotal=r.carried;o.epoch=epoch;o.token=Token();
            }
            const bool same=Live(claims[slot],r.now)&&claims[slot].request.project==r.project&&
                claims[slot].request.source==r.source&&claims[slot].request.material==r.material;
            const unsigned started=same?claims[slot].started:r.now;
            claims[slot].request=r;claims[slot].quantity=Min(r.need,r.carried+quantity);
            claims[slot].sourceUnits=r.ground?r.available:quantity;
            claims[slot].started=started;claims[slot].observed=r.now;claims[slot].checked=r.now-250;
            claims[slot].used=true;EmptyPlaceholders();return quantity;
        }
    public:
        enum{Capacity=128,Batch=32,LeaseMs=120000};
        Coordinator():epoch(0),serial(0){}
        int Demand(const Identity& actor,const Identity& project,const Material& material,int need,unsigned now,unsigned e)const
        {
            if(e!=epoch||need<0||need>1000000)return 0;
            for(unsigned i=0;i<Capacity;++i)
            {
                const Claim& c=claims[i];
                if(Live(c,now)&&c.request.actor!=actor&&c.request.project==project&&c.request.material==material)
                    need=Sub(need,c.quantity);
            }
            return need;
        }
        bool CanTrack(const Request& r)const
        {
            if(!Valid(r))return false;bool claim=Find(r.actor,&r.material)>=0,ledger=FindOwned(r.actor,r.material)>=0;
            for(unsigned i=0;i<Capacity;++i){if(!claims[i].used)claim=true;if(!owned[i].token)ledger=true;}
            return claim&&ledger;
        }
        bool CanCargo(const Request& r)const
        {if(r.epoch!=epoch||!r.material.Valid())return false;if(Find(r.actor,&r.material)>=0)return true;
            for(unsigned i=0;i<Capacity;++i)if(!claims[i].used)return true;return false;}
        int Preview(const Request& r)const{return Calculate(r);}
        int Cargo(const Request& r)
        {
            if(r.epoch!=epoch||!r.actor.Valid()||!r.project.Valid()||!r.material.Valid()||r.need<=0||r.need>1000000||
                r.carried<=0||r.carried>1000000)return 0;
            Expire(r.now,r.epoch);int slot=Find(r.actor,&r.material);
            if(slot>=0&&claims[slot].sourceUnits>0)return claims[slot].request.project==r.project?claims[slot].quantity:0;
            const int quantity=Min(r.carried,Demand(r.actor,r.project,r.material,r.need,r.now,r.epoch));
            if(!quantity)return 0;
            if(slot<0)for(unsigned i=0;i<Capacity;++i)if(!claims[i].used){slot=int(i);break;}
            if(slot<0)return 0;
            claims[slot].request=r;claims[slot].request.source=r.actor;
            claims[slot].quantity=quantity;claims[slot].sourceUnits=0;claims[slot].started=r.now;
            claims[slot].observed=r.now;claims[slot].checked=r.now;claims[slot].used=true;
            return quantity; // Existing cargo creates no ownership credit or ownership placeholder.
        }
        int Commit(const Request& r){return Store(r,false);}
        int Grant(const Request& r){return Store(r,true);}
        void Reset(unsigned e)
        {epoch=e;Token();for(unsigned i=0;i<Capacity;++i){claims[i]=Claim();owned[i]=Owned();}}
        void Cancel(const Identity& actor)
        {bool found=false;for(unsigned i=0;i<Capacity;++i)if(claims[i].used&&claims[i].request.actor==actor){claims[i]=Claim();found=true;}if(found)EmptyPlaceholders();}
        void Expire(unsigned now,unsigned e)
        {
            if(e!=epoch)return; // Only explicit world invalidation advances generation.
            for(unsigned i=0;i<Capacity;++i)if(claims[i].used&&!Live(claims[i],now))
            {
                Claim& c=claims[i];
                if(c.request.source!=c.request.actor&&unsigned(now-c.observed)<LeaseMs)
                {c.quantity=0;c.sourceUnits=0;} // Live body witness, expired resource/demand lease.
                else c=Claim();
            }
            EmptyPlaceholders();
        }
        bool SourceWatch(const Identity& actor,const Identity& project,const Identity& source,unsigned now,unsigned e,Request& out)
        {
            if(e!=epoch)return false;
            for(unsigned i=0;i<Capacity;++i)
            {
                Claim& c=claims[i];
                if(!c.used||c.request.actor!=actor||c.request.project!=project||c.request.source!=source||source==actor)continue;
                c.observed=now;if(unsigned(now-c.checked)<250)return false;
                c.checked=now;out=c.request;out.now=now;return true;
            }
            return false;
        }
        bool Has(const Request& r)const
        {
            if(r.epoch!=epoch)return false;
            int i=Find(r.actor,&r.material);return i>=0&&Live(claims[i],r.now)&&claims[i].request.project==r.project&&
                claims[i].request.source==r.source&&claims[i].request.material==r.material;
        }
        void Acquired(const Request& r,int moved,int before,int after)
        {
            int c=Find(r.actor,&r.material),o=FindOwned(r.actor,r.material);
            if(c<0||o<0||!Has(r)||moved<=0||before<0||after<before||after-before!=moved)return;
            Owned& ledger=owned[o];
            if(ledger.expectedTotal!=before)
            {ledger.owned=0;ledger.baseline=before;ledger.token=Token();}
            if(moved>1000000-ledger.owned){ledger=Owned();claims[c]=Claim();return;}
            ledger.owned+=moved;ledger.expectedTotal=after;ledger.project=r.project;
            claims[c].sourceUnits=Sub(claims[c].sourceUnits,moved);
            claims[c].quantity=Min(r.need,after+claims[c].sourceUnits);
        }
        void Finish(const Request& r,int total)
        {
            int c=Find(r.actor,&r.material);if(c<0||!Has(r))return;
            claims[c].sourceUnits=0;claims[c].quantity=Min(r.need,Positive(total));
            if(!claims[c].quantity)claims[c]=Claim();EmptyPlaceholders();
        }
        // An observed native delivery may spend only proven owned units. Other
        // inventory mutations retire credit rather than guessing who owns it.
        void Delivered(const Identity& actor,const Material& material,int before,int after)
        {
            int o=FindOwned(actor,material);
            if(o>=0)
            {
                Owned& x=owned[o];
                if(x.expectedTotal!=before||after<0||after>before){x=Owned();}
                else{x.owned=Sub(x.owned,before-after);x.expectedTotal=after;if(!x.owned)x=Owned();}
            }
            int c=Find(actor,&material);if(c>=0)
            {claims[c].quantity=Min(claims[c].quantity,Positive(after));if(!after)claims[c]=Claim();}
            EmptyPlaceholders();
        }
        unsigned ListOwned(Owned* out,unsigned capacity)const
        {unsigned n=0;for(unsigned i=0;i<Capacity&&n<capacity;++i)if(owned[i].token&&owned[i].owned>0)out[n++]=owned[i];return n;}
        unsigned ListOwnedSlice(Owned* out,unsigned capacity,unsigned& cursor)const
        {
            unsigned n=0,examined=0;cursor%=Capacity;
            while(examined<Capacity&&n<capacity)
            {const unsigned i=(cursor+examined)%Capacity;if(owned[i].token&&owned[i].owned>0)out[n++]=owned[i];++examined;}
            cursor=(cursor+examined)%Capacity;return n;
        }
        bool CheckOwned(const Owned& row,int total,int& allowed)
        {
            allowed=0;int i=FindOwned(row.actor,row.material);
            if(i<0||row.epoch!=epoch||owned[i].token!=row.token)return false;
            if(total!=owned[i].expectedTotal){owned[i]=Owned();return false;}
            allowed=Min(owned[i].owned,Sub(total,owned[i].baseline));return allowed>0;
        }
        void Returned(const Owned& row,int moved,int after)
        {
            int i=FindOwned(row.actor,row.material);
            if(i<0||owned[i].token!=row.token||row.epoch!=epoch)return;
            Owned& x=owned[i];
            if(moved<=0||moved>x.owned||after<0||x.expectedTotal-after!=moved||after<x.baseline){x=Owned();return;}
            x.owned-=moved;x.expectedTotal=after;if(!x.owned)x=Owned();
        }
        void ForgetOwned(const Owned& row)
        {int i=FindOwned(row.actor,row.material);if(i>=0&&owned[i].token==row.token&&row.epoch==epoch)owned[i]=Owned();}
    };
}
