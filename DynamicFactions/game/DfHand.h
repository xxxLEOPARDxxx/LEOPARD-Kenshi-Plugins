#pragma once
#include <kenshi/util/hand.h>

// SDK hand comparison is not reliable across all local ABI builds.
struct DfWorkHandLess
{
    bool operator()(const hand& a,const hand& b) const
    {
        if(a.type!=b.type)return a.type<b.type;
        if(a.container!=b.container)return a.container<b.container;
        if(a.containerSerial!=b.containerSerial)return a.containerSerial<b.containerSerial;
        if(a.index!=b.index)return a.index<b.index;
        return a.serial<b.serial;
    }
};
