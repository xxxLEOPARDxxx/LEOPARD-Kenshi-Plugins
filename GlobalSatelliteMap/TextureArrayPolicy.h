#ifndef LEOPARD_SATELLITE_TEXTURE_ARRAY_POLICY_H
#define LEOPARD_SATELLITE_TEXTURE_ARRAY_POLICY_H
#include <cstddef>
namespace SatelliteTextureArray
{
struct Shape { unsigned width,height,depth,mips,format;size_t bytes; };
inline bool Accept(const Shape &s)
{
    return s.width>=1&&s.width<=4096&&s.height>=1&&s.height<=4096&&s.depth==1&&s.mips<=12&&
        s.format!=0&&s.bytes>0&&s.bytes<=32u*1024u*1024u;
}
inline bool Match(const Shape &first,const Shape &next)
{
    return Accept(first)&&Accept(next)&&first.width==next.width&&first.height==next.height&&
        first.depth==next.depth&&first.mips==next.mips&&first.format==next.format;
}
inline bool AddBytes(size_t &total,size_t bytes)
{
    const size_t limit=128u*1024u*1024u;
    if(bytes>limit||total>limit-bytes)return false;
    total+=bytes;return true;
}
}
#endif
