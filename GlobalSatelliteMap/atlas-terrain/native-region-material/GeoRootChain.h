#pragma once
#include "../source-receipts/ReceiptPolicy.h"
namespace NativeRegionMaterial {
struct GeoRoots {const void*game;const void*zone;const void*map;const void*table;GeoRoots():game(NULL),zone(NULL),map(NULL),table(NULL){}};
inline const void*GeoAddress(const void*p,size_t offset){return reinterpret_cast<const void*>(reinterpret_cast<SourceReceipts::U64>(p)+offset);}
// The GameWorld and its ZoneManager are different owners. The engine slot
// names the latter; imported ou/scope.world names the former.
template<class Reader>inline bool ReadGeoRoots(Reader&read,const void*game,const void*engineSlot,GeoRoots&out){
 out=GeoRoots();if(!game||!engineSlot)return false;const void*zone=NULL,*slot=NULL,*map=NULL,*table=NULL;unsigned width=0,height=0;
 if(!read.Read(GeoAddress(game,0x8b0),&zone,sizeof(zone))||!read.Read(engineSlot,&slot,sizeof(slot))||!zone||zone!=slot||
    !read.Read(GeoAddress(zone,0x168220),&map,sizeof(map))||!map||!read.Read(GeoAddress(map,0x50),&width,4)||!read.Read(GeoAddress(map,0x54),&height,4)||width!=32||height!=32||
    !read.Read(GeoAddress(map,0x58),&table,sizeof(table))||!table)return false;
 out.game=game;out.zone=zone;out.map=map;out.table=table;return true;
}
class GeoRootChain {
 GeoRoots roots_;bool valid_;
public:GeoRootChain():valid_(false){}
 template<class Reader>bool Capture(Reader&r,const void*game,const void*engineSlot){valid_=ReadGeoRoots(r,game,engineSlot,roots_);return valid_;}
 template<class Reader>bool Current(Reader&r,const void*game,const void*engineSlot)const{GeoRoots fresh;return valid_&&ReadGeoRoots(r,game,engineSlot,fresh)&&fresh.game==roots_.game&&fresh.zone==roots_.zone&&fresh.map==roots_.map&&fresh.table==roots_.table;}
 const GeoRoots&Values()const{return roots_;}
};
}
