#include "WorldLayout.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool value,const char*message){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",message);throw 1;}}
static void Inside(const WorldLayout::Placement&p){
 C(p.region<64&&p.cell<16,"canonical region/cell indices");
 const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[p.region];
 const int x0=r.worldX+int(p.cell%4)*9216,z0=r.worldZ+int(p.cell/4)*9216;
 const int half=int(p.side*p.spacing/2);
 C(p.centreX-half>=x0&&p.centreZ-half>=z0&&p.centreX+half<=x0+9216&&p.centreZ+half<=z0+9216,"footprint remains in exact native material cell");
 const int gx=-147456+int(p.gridX)*18,gz=-147456+int(p.gridZ)*18;
 C(p.columns&&p.rows&&p.columns*p.rows<=16,"bounded native64-cell mesh patches");
 C(gx<=p.centreX-half&&gz<=p.centreZ-half&&gx+int(p.columns)*1152>=p.centreX+half&&gz+int(p.rows)*1152>=p.centreZ+half,"mesh covers every pixel footprint without clipping");
 C(gx>=x0&&gz>=z0&&gx+int(p.columns)*1152<=x0+9216&&gz+int(p.rows)*1152<=z0+9216,"normal halo may extend but geometry does not cross material cell");
}
int main(){try{
 bool seen[64][64]={};
 for(unsigned z=0;z<32;++z)for(unsigned x=0;x<32;++x){WorldLayout::Placement p[4];C(WorldLayout::BaseCell(x,z,p),"all1024 world material cells must map to four base windows");
  for(unsigned n=0;n<4;++n){const WorldLayout::Placement&w=p[n];Inside(w);const unsigned px=x*2+n%2,pz=z*2+n/2;
   C(w.side==512&&w.spacing==9,"base is native512@9m not resampled detail");
   C(w.centreX==-145152+int(px)*4608&&w.centreZ==-145152+int(pz)*4608,"base pixels align to world origin and half-texel centres");
   C(w.tileX==px/16&&w.tileZ==pz/16&&w.pixelX==(px%16)*512&&w.pixelZ==(pz%16)*512,"exact4x4 GST2 tile/window coordinates");
   C(!seen[pz][px],"no duplicate base coverage");seen[pz][px]=true;
  }
 }
 for(unsigned z=0;z<64;++z)for(unsigned x=0;x<64;++x)C(seen[z][x],"complete4096 base-window coverage");
 WorldLayout::Placement invalid[4];std::memset(invalid,0xa5,sizeof(invalid));C(!WorldLayout::BaseCell(32,0,invalid)&&!invalid[0].side&&!invalid[3].spacing,"out-of-world base request clears stale result");
 C(!WorldLayout::BaseCell(0,~0u,invalid),"large base index refused without arithmetic overflow");
 for(unsigned spacing=1;spacing<=3;spacing+=2){const unsigned limit=FineWindows::GridSide(spacing);for(unsigned axis=0;axis<limit;++axis){const unsigned other[5]={0,limit-1,axis,limit/2,limit/3};for(unsigned k=0;k<5;++k){FineWindows::Key key={spacing,axis,other[k],{0x100000007ULL,0x200000009ULL}};WorldLayout::Placement w;
  C(WorldLayout::Fine(key,w),"fine windows accepted throughout both axes and cell boundaries");Inside(w);FineWindows::Rect bounds;C(FineWindows::Bounds(key,bounds),"independent frozen key bounds");
  C(w.side==256&&w.spacing==spacing&&w.centreX==bounds.x+bounds.w/2&&w.centreZ==bounds.z+bounds.h/2,"fine key retains its exact density/phase");
  C(w.columns<=2&&w.rows<=2,"fine footprint uses at most2x2 native patches");
  key.x=other[k];key.z=axis;C(WorldLayout::Fine(key,w),"swapped fine axis accepted");Inside(w);
 }}}
 FineWindows::Key bad={9,0,0,{1,2}};WorldLayout::Placement w;std::memset(&w,0xa5,sizeof(w));C(!WorldLayout::Fine(bad,w)&&!w.side,"9m keys belong to complete base atlas, not fine cache");bad.spacing=1;bad.x=1152;C(!WorldLayout::Fine(bad,w),"out-of-world fine key refused");bad.x=0;bad.stamp.material=0;C(!WorldLayout::Fine(bad,w),"invalid fine provenance label refused");
 std::printf("PASS %u layout controls: all4096 base windows and fine-axis boundary coverage\n",checks);return 0;
}catch(...){return 1;}}
