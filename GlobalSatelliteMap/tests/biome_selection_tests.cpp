#include "../BiomeSelection.h"
#include <cstdio>
#include <limits>
#include <cstring>
using namespace SatelliteBiome;
static int failures=0,checks=0;
static void Check(bool ok,const char *name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}}
int main()
{
    unsigned char masks[2048]={0};masks[0]=0x8f;masks[1]=0x8f;
    masks[1024]=0x42;masks[1025]=0x44;masks[256]=0x46;
    Cell cell;cell.id[0]=11;cell.id[1]=12;cell.id[2]=13;cell.id[3]=14;cell.id[4]=15;
    cell.masks=masks;cell.maskCount=2048;
    Selection out;
    // Cell16,2 from a real native snapshot, 9216m wide, fine steps288m.
    Bounds b={1.0f,0.0f,2.0f,3.0f};
    b.minX=1;b.minZ=-129023;b.maxX=100;b.maxZ=-128924;
    Check(Locate(b,out)==Selected&&out.cellX==16&&out.cellZ==2,"native coarse placement");
    Check(Choose(cell,out)==Selected&&out.count==1&&out.id[0]==12&&out.channel[0]==1,"fine native bit preserves channel");
    b.maxX=576;b.maxZ=-128448;
    Check(Locate(b,out)==Selected&&out.treeIndex==256,"max edge epsilon chooses common ancestor");
    Check(Choose(cell,out)==Selected&&out.count==2&&out.id[0]==13&&out.id[1]==12,"descending native slots");
    b.maxX=9216.0f;b.maxZ=-119808.0f;
    Check(Locate(b,out)==Selected&&out.treeIndex==1,"whole cell includes exact upper edge");
    Check(Choose(cell,out)==Selected&&out.count==4&&!out.truncated,"four biomes");
    masks[1]=0x9f;
    Check(Choose(cell,out)==Selected&&out.count==4&&out.truncated&&out.id[0]==15&&out.id[3]==12,"native five to four explicit truncation");
    masks[0]=0x23;cell.maskCount=1;
    Check(Choose(cell,out)==Selected&&out.count==2&&out.id[0]==12,"unsplit root uses low five bits");
    cell.id[1]=0;
    Check(Choose(cell,out)==Selected&&out.count==1&&out.id[0]==11,"native null source slot skipped");
    masks[0]=0;
    Check(Choose(cell,out)==NoBiome&&out.count==0,"empty mask");
    masks[0]=0x80;
    Check(Choose(cell,out)==Malformed,"split tree requires full bounded array");
    cell.masks=NULL;
    Check(Choose(cell,out)==Malformed,"missing masks");
    b.minX=-147456;b.minZ=-147456;b.maxX=-138240;b.maxZ=-138240;
    Check(Locate(b,out)==Selected&&out.cellX==0&&out.cellZ==0,"lower world boundary");
    b.minX=138240;b.minZ=138240;b.maxX=147456;b.maxZ=147456;
    Check(Locate(b,out)==Selected&&out.cellX==31&&out.cellZ==31,"upper world boundary");
    b.minX=0;b.minZ=0;b.maxX=9217;b.maxZ=100;
    Check(Locate(b,out)==CrossesCell,"coarse patch crossing falls back");
    b.minX=-147457;b.maxX=-147450;
    Check(Locate(b,out)==Outside,"outside world");
    b.minX=0;b.maxX=0;
    Check(Locate(b,out)==Malformed,"zero width");
    b.maxX=1;b.minZ=0;b.maxZ=-1;
    Check(Locate(b,out)==Malformed,"inverted extent");
    b.maxZ=std::numeric_limits<float>::infinity();
    Check(Locate(b,out)==Malformed,"infinity");
    b.maxZ=std::numeric_limits<float>::quiet_NaN();
    Check(Locate(b,out)==Malformed,"nan");
    // Actual selected bytes from native cell16,2. Full2048-byte tree SHA256:
    // e6d444c3f0385b274c8a89e90f33593f26d70b4325810e3627e418f741e03a0f.
    // Source slots: 5635,64488,3626,2741-Newwworld.mod, null.
    // Unused bytes deliberately zero; tests below touch only observed nodes.
    std::memset(masks,0,sizeof(masks));masks[0]=143;masks[1]=143;
    masks[256]=74;masks[1024]=74;masks[1792]=40;masks[2047]=36;
    cell=Cell();cell.masks=masks;cell.maskCount=2048;
    cell.id[0]=5635;cell.id[1]=64488;cell.id[2]=3626;cell.id[3]=2741;
    b.minX=1;b.minZ=-129023;b.maxX=287;b.maxZ=-128737;
    Check(Locate(b,out)==Selected&&out.treeIndex==1024,"real fixture lower fine leaf");
    Check(Choose(cell,out)==Selected&&out.count==2&&out.id[0]==2741&&out.channel[0]==3&&
          out.id[1]==64488&&out.channel[1]==1,"real mask74 channels3,1");
    b.minX=4609;b.minZ=-124415;b.maxX=4895;b.maxZ=-124129;
    Check(Locate(b,out)==Selected&&out.treeIndex==1792,"real fixture centre fine leaf");
    Check(Choose(cell,out)==Selected&&out.count==1&&out.id[0]==2741&&out.channel[0]==3,"real mask40 channel3");
    b.minX=8929;b.minZ=-120095;b.maxX=9215;b.maxZ=-119809;
    Check(Locate(b,out)==Selected&&out.treeIndex==2047,"real fixture upper fine leaf");
    Check(Choose(cell,out)==Selected&&out.count==1&&out.id[0]==3626&&out.channel[0]==2,"real mask36 channel2");
    // Exhaust all1024 fine leaves, checking against independently formed bit
    // positions rather than a second invocation of the Morton helper.
    bool leaves=true;
    for(unsigned z=0;z<32;++z)for(unsigned x=0;x<32;++x)
    {
        b.minX=float(x*288)+1;b.maxX=float((x+1)*288)-1;
        b.minZ=-129024+float(z*288)+1;b.maxZ=-129024+float((z+1)*288)-1;
        const unsigned expected=1024+(x&1)+((x&2)<<1)+((x&4)<<2)+((x&8)<<3)+((x&16)<<4)+
          ((z&1)<<1)+((z&2)<<2)+((z&4)<<3)+((z&8)<<4)+((z&16)<<5);
        leaves=leaves&&Locate(b,out)==Selected&&out.treeIndex==expected;
    }
    Check(leaves,"all1024 native fine leaf coordinates");
    b.minX=100;b.minZ=100;b.maxX=100.25f;b.maxZ=100.25f;
    Check(Locate(b,out)==Selected&&out.cellX==16&&out.cellZ==16&&out.treeIndex==1024,"native subepsilon positive extent");
    b.minX=-147456;b.minZ=-147456;b.maxX=-147455.9f;b.maxZ=-147455.9f;
    Check(Locate(b,out)==CrossesCell,"thin lower world epsilon stays signed");
    std::printf("%d production biome selection checks, %d failures\n",checks,failures);
    return failures?1:0;
}
