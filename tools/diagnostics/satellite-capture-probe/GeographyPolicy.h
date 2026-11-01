#pragma once
#include <stddef.h>
#include <cstring>
namespace SatelliteGeography
{
static const size_t CpuLimit=128u*1024u*1024u;
static const unsigned __int64 GpuLimit=1024ULL*1024*1024;
static const unsigned __int64 PoolBytes=385874496,OtherTextureBytes=111848096+5592404+256;
struct Mip {const void* data;size_t bytes;};
inline unsigned U32(const unsigned char* p){unsigned v;std::memcpy(&v,p,4);return v;}
inline bool Dds(const void* input,size_t bytes,unsigned format,Mip (&out)[9])
{
    if(!input||bytes<128||bytes>CpuLimit||(format!=71&&format!=77))return false;
    const unsigned char*p=static_cast<const unsigned char*>(input);const unsigned block=format==77?16:8;
    if(U32(p)!=0x20534444||U32(p+4)!=124||U32(p+8)!=659463||U32(p+12)!=2048||U32(p+16)!=2048||
       U32(p+20)!=512u*512u*block||U32(p+24)!=0||U32(p+28)!=12||U32(p+76)!=32||U32(p+80)!=4||
       U32(p+84)!=(format==77?0x35545844u:0x31545844u)||U32(p+108)!=4198408||U32(p+112)!=0)return false;
    Mip result[9];size_t at=128;
    for(unsigned mip=0;mip<12;++mip)
    {
        const unsigned side=2048>>mip,blocks=(side+3)/4;
        const size_t size=size_t(blocks)*blocks*block;
        if(size>bytes-at)return false;
        if(mip<9){result[mip].data=p+at;result[mip].bytes=size;}
        at+=size;
    }
    if(at!=bytes)return false;
    for(unsigned mip=0;mip<9;++mip)out[mip]=result[mip];return true;
}
inline bool Combined(unsigned __int64 mesh,unsigned __int64 output)
{
    const unsigned __int64 fixed=PoolBytes+OtherTextureBytes;
    return mesh<=GpuLimit-fixed&&output<=GpuLimit-fixed-mesh;
}
}
