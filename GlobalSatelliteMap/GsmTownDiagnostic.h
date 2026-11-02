#pragma once
// Synchronous debug census only. Caller keeps SharedKing/world alive for Read.
#include <Windows.h>
#include <cstring>
namespace GsmTownDiagnostic {
const unsigned Limit=4096;
const SIZE_T TownVtable=0x1734c08,IsTownSlot=0x268,IsTownThunk=0x253a6;
struct Stats {
    unsigned total,primary,otherType,nullTown,missingProxy,ownerMismatch,state[4],otherState,nullEntity,candidates;
    Stats(){std::memset(this,0,sizeof(*this));}
};
struct CodePin {DWORD rva;BYTE bytes[24];};
static const CodePin Pins[]={
{0xb85f0,{0x48,0x8d,0x41,0x50,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x83,0xf9,0x05,0x77,0x34,0x48,0x8d,0x15}},
{0x8679bd,{0x48,0x89,0x7b,0x08,0x48,0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x50,0x48,0x83,0xc4,0x30,0x5f,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc}},
{0x927e08,{0x39,0x79,0x58,0x76,0x63,0x48,0x89,0x74,0x24,0x40,0x8b,0xf7,0x4c,0x89,0x64,0x24,0x48,0x0f,0x1f,0x80,0x00,0x00,0x00,0x00}},
{0x927e20,{0x48,0x8b,0x45,0x60,0x48,0x8b,0x0c,0x06,0x48,0x8b,0x01,0xff,0x50,0x58,0x49,0x3b,0xc5,0x75,0x28,0x48,0x8b,0x45,0x60,0x8b}},
{0x253a6,{0xe9,0x35,0xbd,0x92,0x00,0xe9,0x50,0xcb,0x0a,0x00,0xe9,0x6b,0xea,0x8b,0x00,0xe9,0x46,0xb4,0x79,0x00,0xe9,0x51,0x04,0x24}}
};
inline int NativeFault(DWORD code){return code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_IN_PAGE_ERROR||code==EXCEPTION_DATATYPE_MISALIGNMENT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}
// These loads must occur again at publication, even after whole-program /O2.
// Volatile is a reload contract, not concurrent lifetime synchronization.
inline const void*Pointer(const BYTE*p){return *reinterpret_cast<const void*const volatile*>(p);}
inline unsigned Number(const BYTE*p){return *reinterpret_cast<const volatile unsigned*>(p);}
inline bool Profile(const BYTE*base){
    if(!base||reinterpret_cast<ULONG_PTR>(base)>ULONG_PTR(-1)-(TownVtable+IsTownSlot+sizeof(void*)))return false;
    for(unsigned i=0;i<sizeof(Pins)/sizeof(Pins[0]);++i)if(std::memcmp(base+Pins[i].rva,Pins[i].bytes,24))return false;
    return Pointer(base+TownVtable+IsTownSlot)==base+IsTownThunk;
}
struct ListHeader {const BYTE*list;const void*const*array;unsigned count;};
inline bool Header(const BYTE*king,ListHeader&h){
    if(!king)return false;h.list=static_cast<const BYTE*>(Pointer(king+8));if(!h.list)return false;
    h.count=Number(h.list+0x58);h.array=static_cast<const void*const*>(Pointer(h.list+0x60));
    return h.count<=Limit&&(!h.count||(h.array&&!(reinterpret_cast<ULONG_PTR>(h.array)%sizeof(void*))&&reinterpret_cast<ULONG_PTR>(h.array)<=ULONG_PTR(-1)-SIZE_T(h.count)*sizeof(void*)));
}
inline bool SameHeader(const ListHeader&a,const ListHeader&b){return a.list==b.list&&a.array==b.array&&a.count==b.count;}
inline bool SameCandidate(const BYTE*town,const BYTE*proxy,const void*entity,const BYTE*engine){
    return Pointer(town)==engine+TownVtable&&Pointer(town+0x4f8)==proxy&&Pointer(proxy)==town&&Number(proxy+8)==3&&Pointer(proxy+16)==entity;
}
// S_OK publishes a coherent census, even when no candidate exists. The borrowed
// candidate is for immediate synchronous use only. No native pointer is cached.
inline HRESULT Read(bool enabled,const void*sharedKing,const void*engineBase,bool sourceVerified,Stats&out,const void*&candidate){
    out=Stats();candidate=NULL;if(!enabled)return S_FALSE;if(!sourceVerified)return E_ACCESSDENIED;
    __try{
        const BYTE*engine=static_cast<const BYTE*>(engineBase),*king=static_cast<const BYTE*>(sharedKing);
        if(!Profile(engine))return E_ACCESSDENIED;
        ListHeader first;if(!Header(king,first))return E_INVALIDARG;
        const void*pointers[Limit];if(first.count)std::memcpy(pointers,first.array,SIZE_T(first.count)*sizeof(void*));
        Stats next;next.total=first.count;const BYTE*chosen=NULL,*chosenProxy=NULL;const void*chosenEntity=NULL;
        for(unsigned i=0;i<first.count;++i){
            const BYTE*town=static_cast<const BYTE*>(pointers[i]);if(!town){++next.nullTown;continue;}
            if(Pointer(town)!=engine+TownVtable){++next.otherType;continue;}++next.primary;
            const BYTE*proxy=static_cast<const BYTE*>(Pointer(town+0x4f8));if(!proxy){++next.missingProxy;continue;}
            if(Pointer(proxy)!=town){++next.ownerMismatch;continue;}
            unsigned state=Number(proxy+8);if(state<4)++next.state[state];else ++next.otherState;
            if(state!=3)continue;
            const void*entity=Pointer(proxy+16);if(!entity){++next.nullEntity;continue;}
            ++next.candidates;if(!chosen){chosen=town;chosenProxy=proxy;chosenEntity=entity;}
        }
        ListHeader after;if(!Header(king,after)||!SameHeader(first,after))return E_ABORT;
        if(chosen&&!SameCandidate(chosen,chosenProxy,chosenEntity,engine))return E_ABORT;
        out=next;candidate=chosen;return S_OK;
    }__except(NativeFault(GetExceptionCode())){return E_FAIL;}
}
}
