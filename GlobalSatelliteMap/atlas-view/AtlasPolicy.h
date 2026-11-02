#pragma once
#include <cstring>
namespace AtlasView {
typedef unsigned __int64 U64;
const U64 GpuCap=96ULL*1024*1024, CpuCap=64ULL*1024*1024;
const unsigned UniformGpuBytes=176;
inline unsigned Side(unsigned level){return level<6?8192U>>level:0;}
inline unsigned Stride(unsigned plane){return plane==0?8:4;}
inline U64 PlaneBytes(unsigned level,unsigned plane){return U64(Side(level))*Side(level)*Stride(plane);}
inline U64 PlaneOffset(unsigned level,unsigned plane){U64 n=1024;for(unsigned l=0;l<level;++l)for(unsigned p=0;p<3;++p)n+=PlaneBytes(l,p);for(unsigned p=0;p<plane;++p)n+=PlaneBytes(level,p);return n;}
inline U64 FileBytes(){return PlaneOffset(5,2)+PlaneBytes(5,2);}
inline U64 CellBytes(unsigned level){return U64(Side(level))*Side(level)*20;}
inline unsigned ScratchSide(unsigned selectedSide){return selectedSide>512?512:selectedSide;}
inline bool Fits(U64 cellBytes,unsigned selectedSide){unsigned s=ScratchSide(selectedSide);return selectedSide<=2048&&cellBytes<=GpuCap-UniformGpuBytes&&U64(s)*s*4<=GpuCap-UniformGpuBytes-cellBytes;}
// Tracks three cache planes plus ONE caller-owned RGBA8 output per cell and
// one private compute scratch (maximum512 square) plus176-byte constants. Call Remove only
// AFTER releasing that cell. Set cannot replace a live cell: evict first.
class Budget {
    unsigned levels_[16];
public:
    Budget(){for(unsigned i=0;i<16;++i)levels_[i]=6;}
    U64 Bytes()const{U64 n=0;for(unsigned i=0;i<16;++i)n+=CellBytes(levels_[i]);return n;}
    unsigned MaxSide()const{unsigned n=0;for(unsigned i=0;i<16;++i)if(Side(levels_[i])>n)n=Side(levels_[i]);return n;}
    unsigned ScratchSide()const{return AtlasView::ScratchSide(MaxSide());}
    bool Set(unsigned tile,unsigned level){if(tile>=16||level<2||level>=6||levels_[tile]!=6)return false;unsigned s=Side(level);if(s<MaxSide())s=MaxSide();if(!Fits(Bytes()+CellBytes(level),s))return false;levels_[tile]=level;return true;}
    void Remove(unsigned tile){if(tile<16)levels_[tile]=6;}
};
inline unsigned Get32(const unsigned char *p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
inline U64 Get64(const unsigned char *p){return U64(Get32(p))|(U64(Get32(p+4))<<32);}
inline bool Nonzero(const unsigned char *p,unsigned n){for(unsigned i=0;i<n;++i)if(p[i])return true;return false;}
inline bool ValidHeader(const unsigned char *h,U64 length,unsigned x,unsigned z){
    if(!h||x>=4||z>=4||length!=FileBytes()||std::memcmp(h,"GST2",4)||Get32(h+4)!=2||Get32(h+8)!=1024||Get32(h+12)!=1||Get32(h+16)!=x||Get32(h+20)!=z||Get32(h+24)!=4||Get32(h+28)!=8192||Get32(h+32)!=6||Get32(h+36)!=3||Get32(h+40)!=2048||Get32(h+44)!=0||Get64(h+48)!=0xffffffffffffULL||Get32(h+56)!=10||Get32(h+60)!=28||Get32(h+64)!=41)return false;
    if(Nonzero(h+68,60)||Nonzero(h+992,32))return false;
    for(unsigned l=0;l<6;++l)for(unsigned p=0;p<3;++p){const unsigned char*d=h+128+(l*3+p)*48;if(Get64(d)!=PlaneOffset(l,p)||Get64(d+8)!=PlaneBytes(l,p)||!Nonzero(d+16,32))return false;}
    return true;
}
}
