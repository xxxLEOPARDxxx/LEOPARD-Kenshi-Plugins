#pragma once
#include "../fine-windows/FineWindowPlanner.h"
#include "../world-material/WorldMaterialPlan.h"
namespace WorldLayout {
struct Placement {
 unsigned region,cell,side,spacing,gridX,gridZ,columns,rows;
 unsigned tileX,tileZ,pixelX,pixelZ;
 int centreX,centreZ;
};
inline bool Region(unsigned x,unsigned z,unsigned&region,unsigned&cell){
 if(x>=32||z>=32)return false;region=(z/4)*8+x/4;cell=(z%4)*4+x%4;
 const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[region];
 return r.worldX==-147456+int(x/4)*36864&&r.worldZ==-147456+int(z/4)*36864;
}
inline bool BaseCell(unsigned x,unsigned z,Placement(&out)[4]){
 std::memset(out,0,sizeof(out));unsigned region,cell;if(!Region(x,z,region,cell))return false;
 for(unsigned n=0;n<4;++n){Placement&p=out[n];const unsigned wx=x*2+n%2,wz=z*2+n/2;
  p.region=region;p.cell=cell;p.side=512;p.spacing=9;
  p.centreX=-145152+int(wx)*4608;p.centreZ=-145152+int(wz)*4608;
  p.gridX=x*512+(n%2)*256;p.gridZ=z*512+(n/2)*256;p.columns=p.rows=4;
  p.tileX=wx/16;p.tileZ=wz/16;p.pixelX=(wx%16)*512;p.pixelZ=(wz%16)*512;
 }return true;
}
inline bool Fine(const FineWindows::Key&key,Placement&out){
 std::memset(&out,0,sizeof(out));if(!FineWindows::Valid(key)||(key.spacing!=1&&key.spacing!=3))return false;
 const unsigned span=256*key.spacing,ox=key.x*span,oz=key.z*span;
 Placement p={};if(!Region(ox/9216,oz/9216,p.region,p.cell))return false;
 p.side=256;p.spacing=key.spacing;p.centreX=-147456+int(ox+span/2);p.centreZ=-147456+int(oz+span/2);
 const unsigned patchX=ox/1152,patchZ=oz/1152;
 p.gridX=patchX*64;p.gridZ=patchZ*64;
 p.columns=(ox+span+1151)/1152-patchX;p.rows=(oz+span+1151)/1152-patchZ;
 // Fine cache uses its own key namespace. These coordinates are meaningful
 // only within that namespace and must never reach the512@9m base writer.
 const unsigned across=FineWindows::GridSide(key.spacing)/4;
 p.tileX=key.x/across;p.tileZ=key.z/across;p.pixelX=(key.x%across)*256;p.pixelZ=(key.z%across)*256;
 out=p;return true;
}
}
