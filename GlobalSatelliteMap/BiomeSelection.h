#ifndef LEOPARD_SATELLITE_BIOME_SELECTION_H
#define LEOPARD_SATELLITE_BIOME_SELECTION_H
#include <cmath>
#include <float.h>
#include <cstddef>

// Pure snapshot selection: no engine calls, allocation, maps or retained pointers
// to game objects. Native Steam selector A08E50..A0928C, evidence 2026-11-01.
namespace SatelliteBiome
{
enum Result { Selected, Outside, CrossesCell, NoBiome, Malformed };
struct Bounds { float minX,minZ,maxX,maxZ; };
struct Cell
{
    unsigned id[5];
    const unsigned char *masks;
    size_t maskCount;
    Cell():masks(NULL),maskCount(0){for(unsigned i=0;i<5;++i)id[i]=0;}
};
struct Selection
{
    unsigned cellX,cellZ,treeIndex,count,id[4],channel[4];
    bool located,truncated;
    Selection():cellX(0),cellZ(0),treeIndex(0),count(0),located(false),truncated(false)
    {for(unsigned i=0;i<4;++i){id[i]=0;channel[i]=0;}}
};
inline unsigned Morton(unsigned x,unsigned z)
{
    unsigned result=0;
    for(unsigned bit=0;bit<5;++bit)
        result|=((x>>bit)&1)<<(bit*2)|((z>>bit)&1)<<(bit*2+1);
    return result;
}
inline Result Locate(const Bounds &box,Selection &out)
{
    out=Selection();
    if(!_finite(box.minX)||!_finite(box.minZ)||!_finite(box.maxX)||!_finite(box.maxZ)||
       box.minX>=box.maxX||box.minZ>=box.maxZ)return Malformed;
    if(box.minX< -147456||box.minZ< -147456||box.maxX>147456||box.maxZ>147456)return Outside;
    // Same float scaling and double max-bound epsilon as native. This prevents
    // exact patch edges from being assigned to the following coarse/fine cell.
    const float minX=(box.minX+147456.0f)/294912.0f*32.0f;
    const float minZ=(box.minZ+147456.0f)/294912.0f*32.0f;
    const float maxX=static_cast<float>(double((box.maxX+147456.0f)/294912.0f*32.0f)-0.0001);
    const float maxZ=static_cast<float>(double((box.maxZ+147456.0f)/294912.0f*32.0f)-0.0001);
    // A valid narrow box can have maxAdjusted<min after native epsilon; do not
    // reject it if both still occupy the same coarse cell. Keep signed floors
    // until comparison so thin lower-world bounds never cast negative to unsigned.
    const int signedX=static_cast<int>(std::floor(minX)),signedZ=static_cast<int>(std::floor(minZ));
    if(signedX<0||signedZ<0||signedX>=32||signedZ>=32)return Outside;
    if(signedX!=static_cast<int>(std::floor(maxX))||signedZ!=static_cast<int>(std::floor(maxZ)))return CrossesCell;
    const unsigned cx=static_cast<unsigned>(signedX),cz=static_cast<unsigned>(signedZ);
    const unsigned lx=static_cast<unsigned>((minX-float(cx))*32.0f),lz=static_cast<unsigned>((minZ-float(cz))*32.0f);
    const unsigned hx=static_cast<unsigned>((maxX-float(cx))*32.0f),hz=static_cast<unsigned>((maxZ-float(cz))*32.0f);
    if(lx>=32||lz>=32||hx>=32||hz>=32)return Malformed;
    unsigned low=1024|Morton(lx,lz),high=1024|Morton(hx,hz);
    while(low!=high){low>>=2;high>>=2;}
    if(low<1||low>=2048)return Malformed;
    out.cellX=cx;out.cellZ=cz;out.treeIndex=low;out.located=true;
    return Selected;
}
inline Result Choose(const Cell &cell,Selection &out)
{
    out.count=0;out.truncated=false;
    for(unsigned i=0;i<4;++i){out.id[i]=0;out.channel[i]=0;}
    if(!out.located||out.treeIndex<1||out.treeIndex>=2048||!cell.masks||
       (cell.maskCount!=1&&cell.maskCount!=2048))return Malformed;
    unsigned mask=cell.masks[0];
    if((mask&0xe0)>0x20)
    {
        if(cell.maskCount!=2048)return Malformed;
        mask=cell.masks[out.treeIndex];
    }
    // Native selection visits five source slots descending; material construction
    // keeps its first four. Preserve original blend channels when slots are null.
    for(int slot=4;slot>=0;--slot)
        if((mask&(1u<<slot))&&cell.id[slot])
        {
            if(out.count==4){out.truncated=true;continue;}
            out.id[out.count]=cell.id[slot];out.channel[out.count]=static_cast<unsigned>(slot);++out.count;
        }
    return out.count?Selected:NoBiome;
}
}
#endif
