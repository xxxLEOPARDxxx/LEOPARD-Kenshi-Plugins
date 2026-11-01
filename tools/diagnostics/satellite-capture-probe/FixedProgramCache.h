#pragma once
#include <string>
// Three fixed private program identities, owned by the native resource manager.
// No shared pointer or dereference of retained raw identities survives a request.
namespace ProbePolicy
{
struct FixedProgramCache
{
    enum Choice {Create,Reuse,Refuse};
    struct Slot {const void *identity;std::string source;Slot():identity(NULL){}} slots[3];
    const void *owner;
    FixedProgramCache():owner(NULL){}
    Choice Inspect(const void *current,unsigned slot,const void *existing,const std::string &source) const
    {
        if(!current||slot>=3||source.empty()||source.size()>32768||(owner&&owner!=current))return Refuse;
        if(slots[slot].identity)
            return existing==slots[slot].identity&&source==slots[slot].source?Reuse:Refuse;
        return existing?Refuse:Create;
    }
    bool Record(const void *current,unsigned slot,const void *identity,const std::string &source)
    {
        if(!identity||Inspect(current,slot,NULL,source)!=Create)return false;
        slots[slot].source=source;slots[slot].identity=identity;owner=current;return true;
    }
    bool Owns(unsigned slot,const void *identity)const
    {return slot<3&&identity&&identity==slots[slot].identity;}
    unsigned Count()const
    {return unsigned(slots[0].identity!=NULL)+unsigned(slots[1].identity!=NULL)+unsigned(slots[2].identity!=NULL);}
};
}
