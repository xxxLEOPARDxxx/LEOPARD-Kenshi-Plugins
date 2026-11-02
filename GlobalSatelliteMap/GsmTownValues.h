#pragma once
#include "GsmTownDiagnostic.h"
#include <float.h>
namespace GsmTownValues {
struct TownValue {unsigned id;float storedXYZ[3];int proxyState;bool hasProxy;};
struct Result {LONG generation;unsigned total,count,nullEntries,otherTypes;TownValue towns[GsmTownDiagnostic::Limit];Result(){std::memset(this,0,sizeof(*this));}};
static const GsmTownDiagnostic::CodePin FieldPins[]={
{0xd63a0,{0x48,0x83,0xc0,0x58,0x45,0x33,0xc0,0x44,0x89,0x44,0x24,0x20,0x41,0xb1,0x20,0x8b,0x50,0x0c,0x48,0x8d,0x4c,0x24,0x68,0xff}},
{0x926425,{0x8b,0x43,0x48,0x89,0x07,0x8b,0x43,0x4c,0x89,0x47,0x04,0x8b,0x43,0x50,0x48,0x8b,0x5c,0x24,0x30,0x89,0x47,0x08,0x48,0x8b}}
};
typedef char TownValueAbi[sizeof(TownValue)==24?1:-1];
typedef char ResultBound[sizeof(Result)<128*1024?1:-1];
struct Identity {const BYTE*town;const void*vtable;const BYTE*proxy;};
inline unsigned Bits(float value){unsigned bits;std::memcpy(&bits,&value,4);return bits;}
inline unsigned StateBits(int value){unsigned bits;std::memcpy(&bits,&value,4);return bits;}
inline bool Generation(volatile LONG*live,LONG expected){return InterlockedCompareExchange(live,0,0)==expected;}
inline bool Fields(const BYTE*engine){for(unsigned i=0;i<sizeof(FieldPins)/sizeof(FieldPins[0]);++i)if(std::memcmp(engine+FieldPins[i].rva,FieldPins[i].bytes,24))return false;return true;}
inline bool CaptureValue(const Identity&identity,TownValue&value){
    value.id=GsmTownDiagnostic::Number(identity.town+0x64);
    for(unsigned i=0;i<3;++i){unsigned bits=GsmTownDiagnostic::Number(identity.town+0x48+i*4);std::memcpy(value.storedXYZ+i,&bits,4);if(!_finite(value.storedXYZ[i]))return false;}
    value.hasProxy=identity.proxy!=NULL;value.proxyState=-1;
    if(identity.proxy){if(GsmTownDiagnostic::Pointer(identity.proxy)!=identity.town)return false;unsigned state=GsmTownDiagnostic::Number(identity.proxy+8);std::memcpy(&value.proxyState,&state,4);}return true;
}
inline bool SameValue(const Identity&identity,const TownValue&value){
    if(GsmTownDiagnostic::Number(identity.town+0x64)!=value.id||GsmTownDiagnostic::Pointer(identity.town+0x4f8)!=identity.proxy)return false;
    for(unsigned i=0;i<3;++i)if(GsmTownDiagnostic::Number(identity.town+0x48+i*4)!=Bits(value.storedXYZ[i]))return false;
    return !identity.proxy||(GsmTownDiagnostic::Pointer(identity.proxy)==identity.town&&GsmTownDiagnostic::Number(identity.proxy+8)==StateBits(value.proxyState));
}
// Caller keeps SharedKing/world alive throughout this synchronous call. No
// names/native functions/getPosition or persistent native pointers are used.
inline HRESULT Read(bool enabled,bool verified,const void*sharedKing,const void*engineModule,LONG generation,volatile LONG*live,Result&out){
    out=Result();if(!enabled)return S_FALSE;if(!verified)return E_ACCESSDENIED;if(!generation||!live)return E_INVALIDARG;
    __try{
        if(!Generation(live,generation))return E_ABORT;const BYTE*engine=static_cast<const BYTE*>(engineModule),*king=static_cast<const BYTE*>(sharedKing);
        if(!GsmTownDiagnostic::Profile(engine)||!Fields(engine))return E_ACCESSDENIED;
        GsmTownDiagnostic::ListHeader first;if(!GsmTownDiagnostic::Header(king,first))return E_INVALIDARG;
        Identity identities[GsmTownDiagnostic::Limit];Result next;next.generation=generation;next.total=first.count;
        for(unsigned i=0;i<first.count;++i){Identity&identity=identities[i];identity.town=static_cast<const BYTE*>(GsmTownDiagnostic::Pointer(reinterpret_cast<const BYTE*>(first.array+i)));identity.vtable=NULL;identity.proxy=NULL;
            if(!identity.town){++next.nullEntries;continue;}identity.vtable=GsmTownDiagnostic::Pointer(identity.town);
            if(identity.vtable!=engine+GsmTownDiagnostic::TownVtable){++next.otherTypes;continue;}
            identity.proxy=static_cast<const BYTE*>(GsmTownDiagnostic::Pointer(identity.town+0x4f8));if(!CaptureValue(identity,next.towns[next.count]))return E_INVALIDARG;++next.count;
        }
        GsmTownDiagnostic::ListHeader after;if(!Generation(live,generation)||!GsmTownDiagnostic::Header(king,after)||!GsmTownDiagnostic::SameHeader(first,after))return E_ABORT;
        unsigned record=0;
        for(unsigned i=0;i<first.count;++i){const Identity&identity=identities[i];if(GsmTownDiagnostic::Pointer(reinterpret_cast<const BYTE*>(first.array+i))!=identity.town)return E_ABORT;if(!identity.town)continue;
            if(GsmTownDiagnostic::Pointer(identity.town)!=identity.vtable)return E_ABORT;if(identity.vtable!=engine+GsmTownDiagnostic::TownVtable)continue;
            if(record>=next.count||!SameValue(identity,next.towns[record++]))return E_ABORT;
        }
        if(record!=next.count||!GsmTownDiagnostic::Header(king,after)||!GsmTownDiagnostic::SameHeader(first,after)||!Generation(live,generation))return E_ABORT;
        out=next;if(!Generation(live,generation)){out=Result();return E_ABORT;}return S_OK;
    }__except(GsmTownDiagnostic::NativeFault(GetExceptionCode())){out=Result();return E_FAIL;}
}
}
