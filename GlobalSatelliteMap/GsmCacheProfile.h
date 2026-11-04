#pragma once
// Main configures one immutable process-session snapshot before workers start.
// These are ceilings for lazy owned caches, never eager multi-GiB allocations.
namespace GsmCacheProfile {
typedef unsigned long long U64;
enum { MaxVisible=96,MaxFineCacheSlots=6144,FineCacheSlots=MaxFineCacheSlots,
       DiskSlots=512,RetirementSlots=2*MaxFineCacheSlots+256 };
const U64 GiB=1024ULL*1024*1024,FineWindowBytes=256ULL*256*20;
// Covers actual K2 producer137,571,380B +40MiB viewer, with a safety margin.
const U64 TransientReserveBytes=192ULL*1024*1024;
// Overview, lighting, water and both city paths; separate from fine tile cache.
const U64 ViewerBytes=40ULL*1024*1024;
namespace Internal {
struct Session {
 U64 gpu,ram;bool configured;
 Session():gpu(GiB),ram(2*GiB),configured(false){}
};
inline Session&Values(){static Session value;return value;}
}
inline bool Configure(unsigned gpuGiB,unsigned ramGiB){
 if(gpuGiB<1||gpuGiB>8||ramGiB<1||ramGiB>32)return false;
 Internal::Session&v=Internal::Values();const U64 gpu=U64(gpuGiB)*GiB,ram=U64(ramGiB)*GiB;
 if(v.configured)return v.gpu==gpu&&v.ram==ram;
 v.gpu=gpu;v.ram=ram;v.configured=true;return true;
}
inline const U64&GpuBudgetBytes(){return Internal::Values().gpu;}
inline const U64&RamBudgetBytes(){return Internal::Values().ram;}
// Legacy aliases retain a reference to boot storage; no static512MiB copy.
static const U64&GpuBytes=GpuBudgetBytes();
inline unsigned FineSlots(){
 const U64 bytes=GpuBudgetBytes();if(bytes<=TransientReserveBytes)return 0;
 const U64 slots=(bytes-TransientReserveBytes)/FineWindowBytes;
 return slots>MaxFineCacheSlots?MaxFineCacheSlots:unsigned(slots);
}
inline unsigned RetirementCount(){return 2*FineSlots()+256;}
}
