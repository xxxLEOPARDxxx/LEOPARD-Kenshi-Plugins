#include "AtlasCapturePolicy.h"
#include "AtlasPixels.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
unsigned checks=0;
void Check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
int main(){
 bool seen[16][16]={0};
 for(unsigned i=0;i<256;++i){
  AtlasCapture::Block b;Check(AtlasCapture::Select(i,b),"every final 2048 block must resolve");
  unsigned tile=i/16,local=i%16;
  Check(b.tileX==tile%4&&b.tileZ==tile/4,"exactly sixteen logical primary tiles");
  Check(b.x==(local%4)*2048&&b.z==(local/4)*2048,"row-major block coverage");
  Check(b.regionX==2*b.tileX+local%4/2&&b.regionZ==2*b.tileZ+local/4/2,"one regional catalog per contained block");
  Check(!seen[tile][local],"no duplicate block");seen[tile][local]=true;
  Check(b.span==18432,"nine world metres per final pixel");
  Check(b.centerX==-147456.0f+b.tileX*73728.0f+(b.x+1024)*9.0f,"absolute X center");
  Check(b.centerZ==-147456.0f+b.tileZ*73728.0f+(b.z+1024)*9.0f,"absolute Z center");
  Check(b.centerX-b.span*.5f>=-147456+b.regionX*36864.0f&&b.centerX+b.span*.5f<=-147456+(b.regionX+1)*36864.0f,"camera fits one SID catalog X");
  Check(b.centerZ-b.span*.5f>=-147456+b.regionZ*36864.0f&&b.centerZ+b.span*.5f<=-147456+(b.regionZ+1)*36864.0f,"camera fits one SID catalog Z");
 }
 AtlasCapture::Block sentinel;sentinel.tileX=99;
 Check(!AtlasCapture::Select(256,sentinel)&&sentinel.tileX==99,"reject overflow without changing result");
 Check(!AtlasCapture::Select(~0u,sentinel),"reject wrapped index");
 AtlasCapture::Camera c={18432,18432,120000,16000,0,0,1};
 for(unsigned z=0;z<2048;z+=37)for(unsigned x=0;x<2048;x+=41){
  const float lx=((x+.5f)/2048-.5f)*c.width,lz=((z+.5f)/2048-.5f)*-c.height;
  const float sourceHeight=500+(x%127)*25.0f;
  const float dy=c.y-sourceHeight,depth=std::sqrt(lx*lx+lz*lz+dy*dy)/c.farClip;
  float actual=-1;Check(AtlasCapture::Height(c,2048,x,z,depth,actual),"valid native radial sample");
  Check(std::fabs(actual-sourceHeight)<.02f,"restore world height without camera-radial atlas seams");
 }
 float h=7;
 Check(!AtlasCapture::Height(c,2048,0,0,0,h)&&h==0,"clear depth has zero coverage/height");
 Check(!AtlasCapture::Height(c,2048,0,0,.00001f,h),"impossible radial/lateral combination rejected");
 Check(!AtlasCapture::Height(c,0,0,0,.5f,h),"zero side refused");
 Check(!AtlasCapture::Height(c,2048,2048,0,.5f,h),"pixel outside capture refused");
 std::printf("PASS %u actual atlas capture coverage/height checks\n",checks);
}
