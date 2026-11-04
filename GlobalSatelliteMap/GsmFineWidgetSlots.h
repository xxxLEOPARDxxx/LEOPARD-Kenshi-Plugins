#pragma once
#include "GsmCacheProfile.h"
#include <cstring>
namespace GsmFineWidgets {
enum {MaxWidgets=2*GsmCacheProfile::MaxVisible};
// Main-only metadata: retained GPU cache slots do not own native widgets.
// Callers clear/hide a widget before unbinding/reusing its pool index.
class Slots {
 enum {Empty=0xffff};
 unsigned short byCache_[GsmCacheProfile::MaxFineCacheSlots],byWidget_[MaxWidgets];
 unsigned count_;
 Slots(const Slots&);Slots&operator=(const Slots&);
 typedef char CacheIdsFit[GsmCacheProfile::MaxFineCacheSlots<Empty?1:-1];
 typedef char WidgetIdsFit[MaxWidgets<Empty?1:-1];
public:
 Slots(){Clear();}
 void Clear(){std::memset(byCache_,0xff,sizeof(byCache_));std::memset(byWidget_,0xff,sizeof(byWidget_));count_=0;}
 int Find(unsigned slot)const{
  if(slot>=GsmCacheProfile::MaxFineCacheSlots)return -1;
  const unsigned pool=byCache_[slot];return pool<MaxWidgets&&byWidget_[pool]==slot?int(pool):-1;
 }
 int Owner(unsigned pool)const{
  if(pool>=MaxWidgets)return -1;
  const unsigned slot=byWidget_[pool];return slot<GsmCacheProfile::MaxFineCacheSlots&&byCache_[slot]==pool?int(slot):-1;
 }
 int Bind(unsigned slot){
  if(slot>=GsmCacheProfile::MaxFineCacheSlots)return -1;
  const int found=Find(slot);if(found>=0)return found;
  if(count_>=MaxWidgets||byCache_[slot]!=Empty)return -1;
  for(unsigned pool=0;pool<MaxWidgets;++pool)if(byWidget_[pool]==Empty){byCache_[slot]=static_cast<unsigned short>(pool);byWidget_[pool]=static_cast<unsigned short>(slot);++count_;return int(pool);}
  return -1;
 }
 bool Unbind(unsigned slot){
  const int pool=Find(slot);if(pool<0||!count_)return false;
  byCache_[slot]=byWidget_[pool]=Empty;--count_;return true;
 }
 unsigned Count()const{return count_;}
};
}
