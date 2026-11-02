#define NOMINMAX
#include <float.h>
#include "../ModelRegion.h"
#include <cstdio>
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line%u %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 float matrix[16]={1,0,0,10,0,1,0,20,0,0,1,30,0,0,0,1};float origin[3]={-51296.703125f,0,3071.80664f},local[6]={-2,-3,-4,5,6,7};
 AtlasModelRegion::Bounds b;CHECK(AtlasModelRegion::WorldBounds(matrix,origin,local,b));CHECK(b.lo[0]==8+origin[0]&&b.hi[0]==15+origin[0]);CHECK(b.lo[1]==17&&b.hi[1]==26);CHECK(b.lo[2]==26+origin[2]&&b.hi[2]==37+origin[2]);
 matrix[0]=0;matrix[2]=2;matrix[8]=-3;matrix[10]=0;
 CHECK(AtlasModelRegion::WorldBounds(matrix,origin,local,b));CHECK(b.lo[0]==2+origin[0]&&b.hi[0]==24+origin[0]);CHECK(b.lo[2]==15+origin[2]&&b.hi[2]==36+origin[2]);
 for(unsigned level=0;level<6;++level){AtlasModelDecode::Footprint f;f.minX=b.lo[0]-1;f.minZ=b.lo[2]-1;f.width=f.height=float(256u<<level);f.nearClip=0;
  AtlasModels::View v;CHECK(AtlasModelRegion::View(b,origin,f,9,v));CHECK(v.cameraScene[1]>b.hi[1]-origin[1]);CHECK(v.farClip>v.cameraScene[1]-(b.lo[1]-origin[1]));CHECK(v.cameraScene[0]==f.minX-origin[0]+f.width*.5f);CHECK(v.cameraScene[2]==f.minZ-origin[2]+f.height*.5f);
  f.nearClip=260;CHECK(!AtlasModelRegion::View(b,origin,f,9,v));f.nearClip=-1;CHECK(!AtlasModelRegion::View(b,origin,f,9,v));f.nearClip=0;
  f.minX=b.hi[0]+1;CHECK(!AtlasModelRegion::Intersects(b,f));CHECK(!AtlasModelRegion::View(b,origin,f,9,v));
 }
 float wrong[16];memcpy(wrong,matrix,sizeof(wrong));wrong[15]=0;CHECK(!AtlasModelRegion::WorldBounds(wrong,origin,local,b));wrong[15]=1;wrong[0]=wrong[2]=0;CHECK(!AtlasModelRegion::WorldBounds(wrong,origin,local,b));
 float bad[6];memcpy(bad,local,sizeof(bad));bad[0]=99;CHECK(!AtlasModelRegion::WorldBounds(matrix,origin,bad,b));
 CHECK(!AtlasModelRegion::WorldBounds(NULL,origin,local,b));
 printf("Actual model bounds/view %u checks PASS\n",checks);return 0;
}
