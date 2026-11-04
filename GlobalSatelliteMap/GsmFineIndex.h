#pragma once
#include "GsmCacheProfile.h"
#include "atlas-terrain/fine-windows/FineWindowPlanner.h"
#include <map>
#include <new>
namespace GsmFineIndex {
// No padding/memcmp or32bit truncation in cache identity. Ordering also makes
// 1m/3m keys at the same world location unambiguous.
struct Less {
 bool operator()(const FineWindows::Key&a,const FineWindows::Key&b)const{
  if(a.stamp.source!=b.stamp.source)return a.stamp.source<b.stamp.source;
  if(a.stamp.material!=b.stamp.material)return a.stamp.material<b.stamp.material;
  if(a.spacing!=b.spacing)return a.spacing<b.spacing;
  return a.z!=b.z?a.z<b.z:a.x<b.x;
 }
};
enum {MaxVisibleIndices=2*GsmCacheProfile::MaxVisible};
class Index {
 typedef std::map<FineWindows::Key,unsigned,Less> Entries;
 Entries entries_;bool occupied_[GsmCacheProfile::MaxFineCacheSlots];FineWindows::U64 revision_;
 Index(const Index&);Index&operator=(const Index&);
 static bool Valid(const FineWindows::Key&key){return (key.spacing==1||key.spacing==3)&&FineWindows::Valid(key);}
 void Changed(){if(!++revision_)++revision_;}
public:
 Index():revision_(1){std::memset(occupied_,0,sizeof(occupied_));}
 int Find(const FineWindows::Key&key)const{
  if(!Valid(key))return -1;Entries::const_iterator at=entries_.find(key);return at==entries_.end()?-1:int(at->second);
 }
 bool Insert(const FineWindows::Key&key,unsigned slot){
  if(!Valid(key)||slot>=GsmCacheProfile::MaxFineCacheSlots||occupied_[slot])return false;
  // Map allocation is the only allocation here. Nothing else changes on OOM
  // or duplicate-key insertion; the caller retains its unindexed empty slot.
  try{if(!entries_.insert(Entries::value_type(key,slot)).second)return false;}catch(const std::bad_alloc&){return false;}
  occupied_[slot]=true;Changed();return true;
 }
 bool Erase(const FineWindows::Key&key,unsigned slot){
  if(!Valid(key)||slot>=GsmCacheProfile::MaxFineCacheSlots||!occupied_[slot])return false;
  Entries::iterator at=entries_.find(key);if(at==entries_.end()||at->second!=slot)return false;
  entries_.erase(at);occupied_[slot]=false;Changed();return true;
 }
 void Clear(){if(entries_.empty())return;entries_.clear();std::memset(occupied_,0,sizeof(occupied_));Changed();}
 unsigned Count()const{return unsigned(entries_.size());}
 FineWindows::U64 Revision()const{return revision_;}
 // Includes this fixed bitmap/map object and a conservative256B allocation
 // allowance per node PLUS the map sentinel (VC10/64bit default allocator).
 // No texture/blob storage. Charge this once, not the same nodes elsewhere.
 FineWindows::U64 MetadataBytes()const{return sizeof(*this)+(FineWindows::U64(Count())+1)*256;}
 unsigned Visible(const FineWindows::Request&view,unsigned*out,unsigned capacity)const{
  const FineWindows::Rect&w=view.world;
  if(!out||capacity<MaxVisibleIndices||!view.screenWidth||!view.screenHeight||view.screenWidth>FineWindows::MaxScreen||view.screenHeight>FineWindows::MaxScreen||!FineWindows::Same(view.stamp,view.stamp)||!_finite(w.x)||!_finite(w.z)||!_finite(w.w)||!_finite(w.h)||w.w<=0||w.h<=0||!_finite(w.x+w.w)||!_finite(w.z+w.h)||w.x+w.w<=w.x||w.z+w.h<=w.z)return 0;
  FineWindows::Internal::Clip clip={FineWindows::Internal::High(FineWindows::WorldOrigin,w.x),FineWindows::Internal::High(FineWindows::WorldOrigin,w.z),FineWindows::Internal::Low(-FineWindows::WorldOrigin,w.x+w.w),FineWindows::Internal::Low(-FineWindows::WorldOrigin,w.z+w.h)};
  if(clip.x1<=clip.x0||clip.z1<=clip.z0)return 0;
  unsigned count=0;
  for(unsigned spacing=1;spacing<=3;spacing*=3){
   if(FineWindows::Internal::Worst(view,spacing)>GsmCacheProfile::MaxVisible)continue;
   const unsigned x0=FineWindows::Internal::Index(clip.x0,spacing),x1=FineWindows::Internal::Last(clip.x1,spacing),z0=FineWindows::Internal::Index(clip.z0,spacing),z1=FineWindows::Internal::Last(clip.z1,spacing);
   if((x1-x0+1)*(z1-z0+1)>GsmCacheProfile::MaxVisible)continue;
   for(unsigned z=z0;z<=z1;++z)for(unsigned x=x0;x<=x1;++x){FineWindows::Key key={spacing,x,z,view.stamp};const int slot=Find(key);if(slot>=0)out[count++]=unsigned(slot);}
  }
  return count;
 }
};
}
