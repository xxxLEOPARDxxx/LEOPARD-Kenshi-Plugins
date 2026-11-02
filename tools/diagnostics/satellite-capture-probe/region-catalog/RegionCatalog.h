#ifndef SATELLITE_REGION_CATALOG_H
#define SATELLITE_REGION_CATALOG_H
#include <cstring>
#include <cmath>
#include <float.h>
namespace SatelliteRegion {
struct Asset {const char *path,*sha256;unsigned bytes,format;};
struct Biome {const char *sid;unsigned short sources[2][6];};
struct Painted {const char *path,*sha256;unsigned bytes,kind,x,z,side;};
struct Binding {unsigned short source;unsigned char layers[2][6];};
// slots use GLOBAL57 index+1; selector low nibble uses LOCAL15 index+1.
struct Cell {unsigned char slots[5],selector[4];unsigned rootMask,treeBytes;const char* treeSha256;};
struct Region {
    unsigned x,z,primaryX,primaryZ;int worldX,worldZ,paintX,paintZ;
    unsigned biomeCount,diffuseCount,normalCount;unsigned short painted[8],banks[92];
    Binding biomes[15];Cell cells[16];unsigned char selector[256];
};
struct Catalog {const Asset* assets;unsigned assetCount;const Biome* biomes;unsigned biomeCount;
    const Painted* painted;unsigned paintedCount;const Region* regions;unsigned regionCount;};
inline bool Hash(const char* p)
{
    if(!p)return false;
    for(unsigned i=0;i<64;++i)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')))return false;
    return p[64]==0;
}
inline bool Text(const char* p,unsigned limit)
{if(!p||!p[0])return false;for(unsigned i=0;i<limit;++i)if(!p[i])return true;return false;}
inline bool Path(const char* p)
{
    if(!Text(p,512)||std::strncmp(p,"data/",5))return false;
    for(unsigned i=0;p[i];++i)
        if(p[i]=='\\'||p[i]==':'||(p[i]=='/'&&(!p[i+1]||p[i+1]=='/'))||
           (p[i]=='.'&&(p[i+1]=='.'||(i==0||p[i-1]=='/')&&(p[i+1]=='/'||!p[i+1]))))return false;
    return true;
}
inline unsigned DdsBytes(unsigned format)
{
    if(format!=71&&format!=77)return 0;
    unsigned bytes=128;
    for(unsigned mip=0;mip<12;++mip){const unsigned blocks=((2048u>>mip)+3)/4;bytes+=blocks*blocks*(format==77?16:8);}
    return bytes;
}
inline unsigned LocalCode(const Region& r,unsigned globalCode)
{for(unsigned b=0;b<r.biomeCount;++b)if(unsigned(r.biomes[b].source)+1==globalCode)return b+1;return 0;}
// Metadata-only preflight of immutable owned data. File hashes/live pointers
// must still be checked by their respective offline/native adapters.
inline bool ValidateRegion(const Catalog& c,const Region& r,unsigned index)
{
    if(!c.assets||!c.biomes||!c.painted||c.assetCount!=219||c.biomeCount!=57||c.paintedCount!=128||index>=64)return false;
    const unsigned x=index%8,z=index/8;
    if(r.x!=x||r.z!=z||r.primaryX!=x/2||r.primaryZ!=z/2||
       r.worldX!=-147456+int(x)*36864||r.worldZ!=-147456+int(z)*36864||
       r.paintX!=-147456+int(x/2)*73728||r.paintZ!=-147456+int(z/2)*73728||
       !r.biomeCount||r.biomeCount>15||!r.diffuseCount||r.diffuseCount>46||!r.normalCount||r.normalCount>46)return false;
    for(unsigned kind=0;kind<2;++kind)for(unsigned layer=0;layer<4;++layer)
        if(r.painted[kind*4+layer]!=kind*64+(r.primaryZ*2+layer/2)*8+r.primaryX*2+layer%2)return false;
    bool poolUsed[2][46]={0},biomeUsed[15]={0};
    for(unsigned b=0;b<15;++b)
    {
        const Binding& bind=r.biomes[b];
        if(b>=r.biomeCount)
        {
            if(bind.source!=65535)return false;
            for(unsigned k=0;k<2;++k)for(unsigned role=0;role<6;++role)if(bind.layers[k][role])return false;
            continue;
        }
        if(bind.source>=c.biomeCount||(b&&bind.source<=r.biomes[b-1].source))return false;
        for(unsigned kind=0;kind<2;++kind)for(unsigned role=0;role<6;++role)
        {
            const unsigned layer=bind.layers[kind][role],count=kind?r.normalCount:r.diffuseCount;
            if(layer>=count)return false;
            const unsigned source=c.biomes[bind.source].sources[kind][role],target=r.banks[kind*46+layer];
            if(source>=c.assetCount||target>=c.assetCount||c.assets[source].format!=(kind?71u:77u)||
               c.assets[target].format!=c.assets[source].format||!c.assets[source].sha256||!c.assets[target].sha256||
               std::strcmp(c.assets[source].sha256,c.assets[target].sha256))return false;
            poolUsed[kind][layer]=true;
        }
    }
    for(unsigned kind=0;kind<2;++kind)
    {
        const unsigned count=kind?r.normalCount:r.diffuseCount,begin=kind*46;
        for(unsigned layer=0;layer<46;++layer)
        {
            const unsigned source=r.banks[begin+layer];
            if(source>=c.assetCount||c.assets[source].format!=(kind?71u:77u))return false;
            if(layer>=count){if(source!=r.banks[begin])return false;continue;}
            if(!poolUsed[kind][layer]||(layer&&source<=r.banks[begin+layer-1]))return false;
            for(unsigned j=0;j<source;++j)
                if(c.assets[j].format==c.assets[source].format&&!std::strcmp(c.assets[j].sha256,c.assets[source].sha256))return false;
        }
    }
    for(unsigned i=0;i<16;++i)
    {
        const Cell& cell=r.cells[i];
        if(!Hash(cell.treeSha256)||(cell.treeBytes!=1&&cell.treeBytes!=2048)||cell.rootMask>255)return false;
        for(unsigned slot=0;slot<5;++slot)
        {
            if(cell.slots[slot]>57)return false;
            for(unsigned j=0;j<slot;++j)if(cell.slots[slot]&&cell.slots[slot]==cell.slots[j])return false;
        }
        unsigned expected[4]={0},used=0;
        for(int slot=4;slot>=0;--slot)if(cell.slots[slot]&&(cell.rootMask&(1u<<slot)))
        {
            const unsigned code=LocalCode(r,cell.slots[slot]);
            if(!code||used>=4)return false;
            expected[used++]=code|(unsigned(slot)<<4);biomeUsed[code-1]=true;
        }
        if(!used)return false;
        for(unsigned slot=0;slot<4;++slot)if(cell.selector[slot]!=expected[slot])return false;
    }
    for(unsigned b=0;b<r.biomeCount;++b)if(!biomeUsed[b])return false;
    for(unsigned zCell=0;zCell<8;++zCell)for(unsigned xCell=0;xCell<8;++xCell)for(unsigned slot=0;slot<4;++slot)
        if(r.selector[(zCell*8+xCell)*4+slot]!=(xCell<4&&zCell<4?r.cells[zCell*4+xCell].selector[slot]:0))return false;
    return true;
}
inline bool Validate(const Catalog& c)
{
    if(!c.assets||!c.biomes||!c.painted||!c.regions||c.assetCount!=219||c.biomeCount!=57||c.paintedCount!=128||c.regionCount!=64)return false;
    for(unsigned a=0;a<c.assetCount;++a)
    {
        const Asset& asset=c.assets[a];
        if(!Path(asset.path)||!Hash(asset.sha256)||!DdsBytes(asset.format)||asset.bytes!=DdsBytes(asset.format)||
           (a&&std::strcmp(c.assets[a-1].path,asset.path)>=0))return false;
    }
    for(unsigned b=0;b<c.biomeCount;++b)
    {
        if(!Text(c.biomes[b].sid,256)||(b&&std::strcmp(c.biomes[b-1].sid,c.biomes[b].sid)>=0))return false;
        for(unsigned kind=0;kind<2;++kind)for(unsigned role=0;role<6;++role)
        {unsigned source=c.biomes[b].sources[kind][role];if(source>=c.assetCount||c.assets[source].format!=(kind?71u:77u))return false;}
    }
    for(unsigned p=0;p<c.paintedCount;++p)
    {
        const Painted& paint=c.painted[p];
        if(!Path(paint.path)||!Hash(paint.sha256)||!paint.bytes||paint.bytes>128u*1024*1024||
           paint.kind!=p/64||paint.x!=p%8||paint.z!=(p%64)/8||paint.side!=(p<64?1024u:2048u))return false;
    }
    for(unsigned i=0;i<64;++i)if(!ValidateRegion(c,c.regions[i],i))return false;
    return true;
}
// Catalog must have passed Validate once. No file reads/allocation/global scan.
// Half-open boundaries give each world point exactly one region and coarse cell.
inline const Cell* FindCell(const Catalog& c,double worldX,double worldZ,const Region*& owner)
{
    owner=NULL;
    if(!c.regions||c.regionCount!=64||!_finite(worldX)||!_finite(worldZ)||
       worldX< -147456||worldZ< -147456||worldX>=147456||worldZ>=147456)return NULL;
    const unsigned x=unsigned((worldX+147456)/36864),z=unsigned((worldZ+147456)/36864);
    const Region& r=c.regions[z*8+x];
    const unsigned cx=unsigned((worldX-r.worldX)/9216),cz=unsigned((worldZ-r.worldZ)/9216);
    owner=&r;return &r.cells[cz*4+cx];
}
inline void DecodeWeights(const Cell& c,const unsigned rgba[4],int total,int (&out)[4])
{
    out[0]=total;out[1]=out[2]=out[3]=0;
    for(unsigned i=1;i<4;++i)if(c.selector[i]){out[i]=int(rgba[c.selector[i]>>4]);out[0]-=out[i];}
}
}
#endif
