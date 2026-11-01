#pragma once
#include <cstddef>

namespace ProbeOwnership
{
struct RtvDebt
{
    const void *texture,*buffer,*target;
    void *view;
    bool spent;
    RtvDebt():texture(NULL),buffer(NULL),target(NULL),view(NULL),spent(false){}
};
// Only creation references omitted by one exact, verified native destructor.
// Never call this policy on a borrowed engine target or an unknown renderer.
class OwnedRtvDebt
{
    RtvDebt entries[6];
    unsigned count;
    bool active,failed,uncertain;
public:
    OwnedRtvDebt():count(0),active(false),failed(false),uncertain(false){}
    bool Active()const{return active;}
    bool Quarantined()const{return failed;}
    unsigned Count()const{return count;}
    bool Begin()
    {
        if(active||failed)return false;
        count=0;active=true;uncertain=false;return true;
    }
    void UncertainAllocation(){if(active)uncertain=true;}
    void AllocationConfirmed(){if(active&&!failed)uncertain=false;}
    bool Register(unsigned index,const void *texture,const void *buffer,const void *target,void *view)
    {
        if(!active||failed)return false;
        if(!texture||!buffer||!target||!view||index>=6){failed=true;return false;}
        if(index<count)
        {
            const RtvDebt& e=entries[index];
            if(e.texture==texture&&e.buffer==buffer&&e.target==target&&e.view==view)return true;
            failed=true;return false;
        }
        if(index!=count){failed=true;return false;}
        for(unsigned i=0;i<count;++i)
        {
            const RtvDebt& e=entries[i];
            if(e.texture==texture||e.buffer==buffer||e.target==target||e.view==view)
            {failed=true;return false;}
        }
        RtvDebt& e=entries[count++];e.texture=texture;e.buffer=buffer;e.target=target;e.view=view;e.spent=false;
        return true;
    }
    template<class Backend> bool Clean(Backend& backend,bool bindingsGone)
    {
        if(failed)return false;
        if(!active)return true;
        if(uncertain||!bindingsGone){failed=true;return false;}
        try
        {
            // Check ALL owners before destroying any. A texture pin alone does
            // not prove its PixelBuffer/RTT has no independent external owner.
            for(unsigned i=0;i<count;++i)
                if(!backend.Exclusive(i,entries[i])){failed=true;return false;}
            for(unsigned i=0;i<count;++i)
            {
                RtvDebt& e=entries[i];
                if(!backend.DropTexture(i)||backend.BufferCount(i)!=1||!backend.DropBuffer(i))
                {failed=true;return false;}
                e.spent=true; // Never retry Release, even if it throws/faults.
                if(!backend.ReleaseView(i,e.view)){failed=true;return false;}
            }
        }
        catch(...){failed=true;return false;}
        count=0;active=false;return true;
    }
};
}
