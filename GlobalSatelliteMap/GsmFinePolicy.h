#pragma once
#include "atlas-cache/fine-store/FineStore.h"
#include <sstream>
namespace GsmFinePolicy {
// A near-enough density alone is insufficient: the entire visible 3m grid
// must fit the real fine-residency cap, or ordinary base LOD stays enabled.
inline bool CanRefine(const FineWindows::Request&view){
 const FineWindows::Rect&w=view.world;if(!view.screenWidth||!view.screenHeight||view.screenWidth>FineWindows::MaxScreen||view.screenHeight>FineWindows::MaxScreen||!FineWindows::Same(view.stamp,view.stamp)||!_finite(w.x)||!_finite(w.z)||!_finite(w.w)||!_finite(w.h)||w.w<=0||w.h<=0||!_finite(w.x+w.w)||!_finite(w.z+w.h))return false;
 return std::min(w.w/view.screenWidth,w.h/view.screenHeight)<9&&FineWindows::Internal::Worst(view,3)<=96;
}
inline bool Intersects(const FineWindows::Key&key,const FineWindows::Request&view){FineWindows::Rect b;return FineWindows::Same(key.stamp,view.stamp)&&FineWindows::Bounds(key,b)&&b.x<view.world.x+view.world.w&&view.world.x<b.x+b.w&&b.z<view.world.z+view.world.h&&view.world.z<b.z+b.h;}
inline int Farthest(const FineWindows::Key*keys,const bool*valid,unsigned count,const FineWindows::Request&view,const FineWindows::Key*protect=NULL,unsigned protectedCount=0){
 if(!keys||!valid||count>GsmCacheProfile::FineSlots()||protectedCount>2)return -1;int victim=-1;double farScore=-1;for(unsigned n=0;n<count;++n)if(valid[n]){bool pinned=false;for(unsigned p=0;protect&&p<protectedCount;++p)if(FineWindows::Equal(keys[n],protect[p]))pinned=true;if(pinned)continue;FineWindows::Rect b;if(!FineWindows::Bounds(keys[n],b))return int(n);double dx=b.x+b.w*.5-view.world.x-view.world.w*.5,dz=b.z+b.h*.5-view.world.z-view.world.h*.5,d=dx*dx+dz*dz;if(!Intersects(keys[n],view))d+=1e15;if(d>farScore){farScore=d;victim=int(n);}}return victim;
}
// Pure pressure selection: keep visible keys while any older offscreen owner
// can be retired. No caller may credit its bytes before its owned fence completes.
inline int Lru(const FineWindows::Key*keys,const bool*valid,const FineWindows::U64*touch,unsigned count,const FineWindows::Request&view,const FineWindows::Key*protect=NULL,unsigned protectedCount=0,bool outsideOnly=false){
 if(!keys||!valid||!touch||count>GsmCacheProfile::FineSlots()||protectedCount>2)return -1;
 for(unsigned pass=0;pass<(outsideOnly?1u:2u);++pass){int victim=-1;FineWindows::U64 oldest=~FineWindows::U64(0);double farScore=-1;
  for(unsigned n=0;n<count;++n)if(valid[n]){bool pinned=false;for(unsigned p=0;protect&&p<protectedCount;++p)if(FineWindows::Equal(keys[n],protect[p]))pinned=true;if(pinned||(pass==0&&Intersects(keys[n],view)))continue;FineWindows::Rect b;double distance=1e30;if(FineWindows::Bounds(keys[n],b)){double dx=b.x+b.w*.5-view.world.x-view.world.w*.5,dz=b.z+b.h*.5-view.world.z-view.world.h*.5;distance=dx*dx+dz*dz;}if(victim<0||touch[n]<oldest||(touch[n]==oldest&&distance>farScore)){victim=int(n);oldest=touch[n];farScore=distance;}}
  if(victim>=0)return victim;
 }return -1;
}
inline int PlanningVictim(const FineWindows::Key*keys,const bool*valid,unsigned count,const FineWindows::Request&view,const FineWindows::U64*touch=NULL){
 if(!keys||!valid||count>GsmCacheProfile::FineSlots()||!view.screenWidth||!view.screenHeight)return -1;double density=std::min(view.world.w/view.screenWidth,view.world.h/view.screenHeight);if(density>=9||FineWindows::Internal::Worst(view,3)>96)return -1;
 unsigned spacing=density<3&&FineWindows::Internal::Worst(view,1)<=96?1:3;bool obsolete[GsmCacheProfile::FineCacheSlots]={false};for(unsigned n=0;n<count;++n)obsolete[n]=valid[n]&&(!Intersects(keys[n],view)||keys[n].spacing!=spacing);
 return touch?Lru(keys,obsolete,touch,count,view):Farthest(keys,obsolete,count,view);
}
inline std::string NativeRoot(std::string root){while(root.size()>3&&(root[root.size()-1]=='/'||root[root.size()-1]=='\\'))root.erase(root.size()-1);return root;}
// Configure is a pure path copy/validation. The persistent file worker creates
// the one new child and performs all filesystem operations.
inline bool CacheConfig(std::wstring parent,DWORD process,FineStore::U64 epoch,FineStore::Config&out){
 while(parent.size()>3&&(parent[parent.size()-1]==L'\\'||parent[parent.size()-1]==L'/'))parent.erase(parent.size()-1);
 if(!process||!epoch)return false;std::wostringstream child;child<<parent<<L"\\fine-session-"<<process<<L'-'<<epoch;return FineStore::Configure(out,parent.c_str(),child.str().c_str());
}
}
