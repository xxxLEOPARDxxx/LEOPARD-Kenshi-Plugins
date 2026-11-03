#include "GeoRootChain.h"
#include <map>
#include <vector>
#include <cstdio>
using namespace NativeRegionMaterial;
static unsigned checks=0;static void C(bool v,const char*s){++checks;if(!v){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Memory {
 std::map<SourceReceipts::U64,std::vector<BYTE> >bytes;
 template<class T>void Set(const void*address,const T&v){const BYTE*p=reinterpret_cast<const BYTE*>(&v);bytes[reinterpret_cast<SourceReceipts::U64>(address)]=std::vector<BYTE>(p,p+sizeof(v));}
 bool Read(const void*address,void*out,size_t n){std::map<SourceReceipts::U64,std::vector<BYTE> >::const_iterator i=bytes.find(reinterpret_cast<SourceReceipts::U64>(address));if(i==bytes.end()||i->second.size()!=n)return false;std::memcpy(out,&i->second[0],n);return true;}
};
int main(){try{const void*game=reinterpret_cast<void*>(0x100001000ULL),*zone=reinterpret_cast<void*>(0x200002000ULL),*map=reinterpret_cast<void*>(0x300003000ULL),*table=reinterpret_cast<void*>(0x400004000ULL),*slot=reinterpret_cast<void*>(0x500005000ULL);Memory memory;unsigned side=32;
 memory.Set(GeoAddress(game,0x8b0),zone);memory.Set(slot,zone);memory.Set(GeoAddress(zone,0x168220),map);memory.Set(GeoAddress(map,0x50),side);memory.Set(GeoAddress(map,0x54),side);memory.Set(GeoAddress(map,0x58),table);
 C(game!=zone,"fixture preserves actual distinct GameWorld and ZoneManager objects");GeoRootChain chain;C(chain.Capture(memory,game,slot),"different GameWorld and ZoneManager form one valid proven root chain");C(chain.Current(memory,game,slot),"same exact root chain remains current");
 const void*other=reinterpret_cast<void*>(0x600006000ULL);memory.Set(GeoAddress(game,0x8b0),other);C(!chain.Current(memory,game,slot),"member and engine ZoneManager disagreement refused");memory.Set(slot,other);memory.Set(GeoAddress(other,0x168220),map);C(!chain.Current(memory,game,slot),"stable new ZoneManager cannot reuse old snapshot");memory.Set(GeoAddress(game,0x8b0),zone);memory.Set(slot,zone);
 memory.Set(GeoAddress(zone,0x168220),other);memory.Set(GeoAddress(other,0x50),side);memory.Set(GeoAddress(other,0x54),side);memory.Set(GeoAddress(other,0x58),table);C(!chain.Current(memory,game,slot),"replacement BiomeMap cannot reuse snapshot");memory.Set(GeoAddress(zone,0x168220),map);
 memory.Set(GeoAddress(map,0x58),other);C(!chain.Current(memory,game,slot),"replacement table cannot reuse snapshot");memory.Set(GeoAddress(map,0x58),table);C(!chain.Current(memory,other,slot),"different scope GameWorld refused");
 side=31;memory.Set(GeoAddress(map,0x50),side);C(!chain.Current(memory,game,slot),"unexpected map width refused");side=32;memory.Set(GeoAddress(map,0x50),side);memory.bytes.erase(reinterpret_cast<SourceReceipts::U64>(slot));C(!chain.Current(memory,game,slot),"unreadable engine root refused");memory.Set(slot,zone);C(chain.Current(memory,game,slot),"restored exact original chain is valid");std::printf("PASS %u production distinct-owner root-chain checks\n",checks);return 0;
 }catch(...){return 1;}}
