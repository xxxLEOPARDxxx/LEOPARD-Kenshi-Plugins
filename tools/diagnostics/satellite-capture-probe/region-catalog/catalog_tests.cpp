#include "RegionCatalogData.h"
#include "../../../../GlobalSatelliteMap/BiomeSelection.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <limits>

using namespace SatelliteRegion;
static unsigned checks=0;
static void Check(bool condition,const char* message)
{++checks;if(!condition){std::fprintf(stderr,"FAIL %s (check%u)\n",message,checks);std::exit(1);}}
#define CHECK(c) Check((c),#c)
static unsigned Read16(std::ifstream& f){unsigned char b[2]={0};f.read(reinterpret_cast<char*>(b),2);CHECK(f.good());return b[0]+unsigned(b[1])*256;}
static void Read(std::ifstream& f,void* p,unsigned n){f.read(static_cast<char*>(p),n);CHECK(f.good());}
struct RawCell{unsigned char slots[5];std::vector<unsigned char> tree;char hash[65];};
static unsigned GlobalCode(const char* sid)
{for(unsigned i=0;i<Data.biomeCount;++i)if(!std::strcmp(Data.biomes[i].sid,sid))return i+1;return 0;}

static void ShapeAndMutationTests()
{
    CHECK(Validate(Data));
    Catalog bad=Data;bad.regionCount=63;CHECK(!Validate(bad));
    bad=Data;bad.regions=NULL;CHECK(!Validate(bad));
    bad=Data;bad.biomeCount=15;CHECK(!Validate(bad));
    bad=Data;bad.assetCount=65535;CHECK(!Validate(bad));
    Region r=Data.regions[0];r.worldX+=9;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[9];r.paintX=r.worldX;CHECK(!ValidateRegion(Data,r,9));
    r=Data.regions[0];r.biomeCount=16;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.diffuseCount=47;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.biomes[0].source=57;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.biomes[0].layers[0][0]=46;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.banks[0]=219;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.banks[0]=r.banks[46];CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.banks[45]=r.banks[46];CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.banks[45]=r.banks[1];CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.biomes[r.biomeCount].source=0;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.cells[0].slots[0]=58;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.cells[0].selector[0]^=16;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.cells[0].treeBytes=2047;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.cells[0].treeSha256="bad";CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.painted[0]=64;CHECK(!ValidateRegion(Data,r,0));
    r=Data.regions[0];r.selector[16]=1;CHECK(!ValidateRegion(Data,r,0));

    // Native negative residuals are meaningful; clamping changes material mix.
    Cell c={0};c.selector[0]=0x21;c.selector[1]=0x12;c.selector[2]=0x03;
    const unsigned rgba[4]={255,255,0,0};int weights[4];DecodeWeights(c,rgba,255,weights);
    CHECK(weights[0]==-255&&weights[1]==255&&weights[2]==255&&weights[3]==0);

    const Region* owner=NULL;
    CHECK(FindCell(Data,-147456,-147456,owner)==&Data.regions[0].cells[0]&&owner==&Data.regions[0]);
    CHECK(!FindCell(Data,147456,0,owner)&&!owner);
    CHECK(!FindCell(Data,0,147456,owner)&&!owner);
    CHECK(!FindCell(Data,-147456.01,0,owner)&&!owner);
    CHECK(!FindCell(Data,std::numeric_limits<double>::quiet_NaN(),0,owner));
    CHECK(!FindCell(Data,std::numeric_limits<double>::infinity(),0,owner));
    for(unsigned z=0;z<8;++z)for(unsigned x=0;x<8;++x)
    {
        const Region& a=Data.regions[z*8+x];
        CHECK(a.worldX==-147456+int(x)*36864&&a.worldZ==-147456+int(z)*36864);
        CHECK(a.paintX==-147456+int(x/2)*73728&&a.paintZ==-147456+int(z/2)*73728);
        CHECK(FindCell(Data,a.worldX,a.worldZ,owner)==&a.cells[0]&&owner==&a);
        CHECK(FindCell(Data,a.worldX+36863.99,a.worldZ+36863.99,owner)==&a.cells[15]&&owner==&a);
        if(x<7)CHECK(FindCell(Data,a.worldX+36864,a.worldZ,owner)==&Data.regions[z*8+x+1].cells[0]);
        if(z<7)CHECK(FindCell(Data,a.worldX,a.worldZ+36864,owner)==&Data.regions[(z+1)*8+x].cells[0]);
    }
    // Explicitly keep all five currently unused source-slot identities. These
    // are GLOBAL57 indices and must not become local15 row references or null.
    const unsigned regionIndex[5]={1,28,34,53,62};
    const char* sid[5]={"4415-Newwworld.mod","2743-Newwworld.mod","2990-Newwworld.mod","3116-Newwworld.mod","51632-Newwworld.mod"};
    for(unsigned k=0;k<5;++k)
    {
        const Region& a=Data.regions[regionIndex[k]];unsigned code=GlobalCode(sid[k]);CHECK(code>0);
        bool found=false,uploaded=false;
        for(unsigned n=0;n<16;++n)for(unsigned slot=0;slot<5;++slot)if(a.cells[n].slots[slot]==code)found=true;
        for(unsigned b=0;b<a.biomeCount;++b)if(unsigned(a.biomes[b].source)+1==code)uploaded=true;
        CHECK(found&&!uploaded);
    }
}

static void GlobalWeights(const Region& r,const Cell& c,const unsigned rgba[4],int total,int (&out)[58])
{
    std::memset(out,0,sizeof(out));int weight[4];DecodeWeights(c,rgba,total,weight);
    for(unsigned i=0;i<4;++i)if(c.selector[i])out[r.biomes[(c.selector[i]&15)-1].source+1]+=weight[i];
}

int main(int argc,char** argv)
{
    CHECK(argc==2);ShapeAndMutationTests();
    std::ifstream f(argv[1],std::ios::binary);CHECK(f.good());char magic[8];Read(f,magic,8);CHECK(!std::memcmp(magic,"RGCFIX1\0",8));
    CHECK(Read16(f)==57);
    for(unsigned i=0;i<57;++i){unsigned size=Read16(f);CHECK(size<256);std::string sid(size,'\0');Read(f,&sid[0],size);CHECK(sid==Data.biomes[i].sid);}
    RawCell raw[1024];
    for(unsigned i=0;i<1024;++i){Read(f,raw[i].slots,5);unsigned bytes=Read16(f);CHECK(bytes==1||bytes==2048);raw[i].tree.resize(bytes);Read(f,&raw[i].tree[0],bytes);Read(f,raw[i].hash,64);raw[i].hash[64]=0;}
    std::vector<unsigned char> pixels(1024*1024*4);Read(f,&pixels[0],static_cast<unsigned>(pixels.size()));CHECK(f.peek()==std::char_traits<char>::eof());
    unsigned fineCount=0,boundaryCount=0,unused=0;
    for(unsigned cz=0;cz<32;++cz)for(unsigned cx=0;cx<32;++cx)
    {
        const RawCell& source=raw[cz*32+cx];const Region& r=Data.regions[(cz/4)*8+cx/4];const Cell& c=r.cells[(cz%4)*4+cx%4];
        CHECK(!std::memcmp(source.slots,c.slots,5));CHECK(c.treeBytes==source.tree.size());
        CHECK(!std::strcmp(source.hash,c.treeSha256));CHECK(c.rootMask==source.tree[source.tree.size()==1?0:1]);
        for(unsigned slot=0;slot<5;++slot)if(c.slots[slot]){bool used=false;for(unsigned b=0;b<r.biomeCount;++b)used|=unsigned(r.biomes[b].source)+1==c.slots[slot];if(!used)++unused;}
        SatelliteBiome::Cell native;for(unsigned slot=0;slot<5;++slot)native.id[slot]=source.slots[slot];native.masks=&source.tree[0];native.maskCount=source.tree.size();
        for(unsigned z=0;z<32;++z)for(unsigned x=0;x<32;++x)
        {
            unsigned gx=cx*32+x,gz=cz*32+z;const Region* located=NULL;
            const Cell* actual=FindCell(Data,-147456+(gx+.5)*288,-147456+(gz+.5)*288,located);
            CHECK(actual==&c&&located==&r);
            SatelliteBiome::Selection leaf;leaf.located=true;leaf.treeIndex=1024|SatelliteBiome::Morton(x,z);
            CHECK(SatelliteBiome::Choose(native,leaf)==SatelliteBiome::Selected);
            const unsigned char* p=&pixels[(gz*1024+gx)*4];unsigned rgba[4]={p[0],p[1],p[2],p[3]};int got[58],want[58]={0};
            GlobalWeights(r,c,rgba,255,got);want[leaf.id[0]]=255;
            for(unsigned j=1;j<leaf.count;++j){want[leaf.id[j]]=rgba[leaf.channel[j]];want[leaf.id[0]]-=rgba[leaf.channel[j]];}
            CHECK(!std::memcmp(got,want,sizeof(got)));++fineCount;
        }
    }
    for(unsigned z=0;z<1024;++z)for(unsigned x=0;x<1024;++x)for(unsigned axis=0;axis<2;++axis)
    {
        unsigned nx=x+(axis==0),nz=z+(axis==1);
        if(nx>=1024||nz>=1024||(x/32==nx/32&&z/32==nz/32))continue;
        const Region *a=NULL,*b=NULL;const Cell* ca=FindCell(Data,-147456+(x+.5)*288,-147456+(z+.5)*288,a);
        const Cell* cb=FindCell(Data,-147456+(nx+.5)*288,-147456+(nz+.5)*288,b);CHECK(ca&&cb&&a&&b);
        unsigned rgba[4];for(unsigned ch=0;ch<4;++ch)rgba[ch]=pixels[(z*1024+x)*4+ch]+pixels[(nz*1024+nx)*4+ch];
        int wa[58],wb[58];GlobalWeights(*a,*ca,rgba,510,wa);GlobalWeights(*b,*cb,rgba,510,wb);CHECK(!std::memcmp(wa,wb,sizeof(wa)));++boundaryCount;
    }
    CHECK(fineCount==1048576&&boundaryCount==63488&&unused>0);
    std::printf("PASS region catalog: %u checks;1048576 native fine leaves;63488 boundaries;5 unused-slot cases; static structs=%u bytes\n",checks,
        unsigned(sizeof(Assets)+sizeof(Biomes)+sizeof(PaintedMaps)+sizeof(Regions)));
    return 0;
}
