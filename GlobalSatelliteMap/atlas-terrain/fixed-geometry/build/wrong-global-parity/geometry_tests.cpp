#include "FixedTerrainGeometry.h"
#include <cstdio>
#include <vector>
#include <cstring>
#include <cmath>
static unsigned checks=0,failures=0;
void Check(bool ok,const char* why){++checks;if(!ok){++failures;std::printf("FAIL %s\n",why);}}
int main(){
 using namespace FixedTerrain;
 FILE*f=NULL;fopen_s(&f,"height-window.u16","rb");if(!f)return 2;
 unsigned dims[4];fread(dims,4,4,f);std::vector<unsigned short> data(dims[2]*dims[3]);fread(&data[0],2,data.size(),f);fclose(f);
 Samples samples={dims[0],dims[1],dims[2],dims[3],dims[2],&data[0],data.size()};
 std::vector<Vertex>a(Vertices),b(Vertices),c(Vertices),d(Vertices),expected(Vertices);
 Check(Build(samples,7104,7360,&a[0],a.size()),"actual source patch accepted");
 Check(Build(samples,7168,7360,&b[0],b.size()),"right neighbor accepted");
 Check(Build(samples,7104,7424,&c[0],c.size()),"bottom neighbor accepted");
 Check(Build(samples,7168,7424,&d[0],d.size()),"diagonal neighbor accepted");
 fopen_s(&f,"expected-patch.f32","rb");if(!f)return 2;Check(fread(&expected[0],sizeof(Vertex),Vertices,f)==Vertices,"independent Python instruction-order reference loaded");fclose(f);
 for(unsigned i=0;i<Vertices;++i)Check(!std::memcmp(&a[i],&expected[i],40),"literal ten-float packet matches independent real-height reference");
 for(unsigned i=0;i<Side;++i){Check(!std::memcmp(&a[i*Side+64],&b[i*Side],40),"all40B shared vertical edge identical");Check(!std::memcmp(&a[64*Side+i],&c[i],40),"all40B shared horizontal edge identical");}
 Check(!std::memcmp(&a.back(),&d[0],40),"four-way shared corner identical");
 fopen_s(&f,"raster-window.u16","rb");if(!f)return 2;unsigned rd[4];fread(rd,4,4,f);std::vector<unsigned short> raster(rd[2]*rd[3]);fread(&raster[0],2,raster.size(),f);fclose(f);
 Samples rs={rd[0],rd[1],rd[2],rd[3],rd[2],&raster[0],raster.size()};std::vector<Vertex> r0(Vertices),r1(Vertices),r2(Vertices),r3(Vertices);
 Check(rd[0]==7358&&rd[1]==7358&&rd[2]==133&&rd[3]==133,"first controlled raster133-square source bounds exact");
 Check(Build(rs,7360,7360,&r0[0],r0.size())&&Build(rs,7424,7360,&r1[0],r1.size())&&Build(rs,7360,7424,&r2[0],r2.size())&&Build(rs,7424,7424,&r3[0],r3.size()),"root256-square3m control four source-pinned patches build");
 for(unsigned i=0;i<Side;++i){Check(!std::memcmp(&r0[i*Side+64],&r1[i*Side],40),"raster fixture vertical edge exact");Check(!std::memcmp(&r0[64*Side+i],&r2[i],40),"raster fixture horizontal edge exact");}
 Check(r3[0].value[0]==-13824&&r3[0].value[2]==-13824&&!std::memcmp(&r0.back(),&r3[0],40),"root control centre is global four-way shared corner");
 Vertex guard;std::memset(&guard,0xA5,40);Vertex before=guard;
 Check(!Build(samples,7105,7360,&guard,1)&&!std::memcmp(&guard,&before,40),"misaligned request refuses without writes");
 Check(!Build(samples,7104,7360,&guard,1)&&!std::memcmp(&guard,&before,40),"short destination refuses before writes");
 Samples narrow=samples;narrow.minX=7104;narrow.width=65;
 Check(!Build(narrow,7104,7360,&a[0],a.size()),"missing global two-sample support refuses");
 Samples tight=samples;tight.width=68;
 Check(!Build(tight,7104,7360,&a[0],a.size()),"rightmost second source halo sample required before payload reads");
 Samples shortSource=samples;shortSource.sampleCount=1;Check(!Build(shortSource,7104,7360,&a[0],a.size()),"short declared source plane refuses before payload reads");
 unsigned short indices[IndexCount];Check(Indices(indices,IndexCount),"native row strip built");
 std::vector<unsigned short> nativeIndices(IndexCount);fopen_s(&f,"expected-indices.u16","rb");if(!f)return 2;size_t readIndices=fread(&nativeIndices[0],2,IndexCount,f);fclose(f);Check(readIndices==IndexCount&&!std::memcmp(indices,&nativeIndices[0],IndexCount*2),"all17144 bytes match independently generated native strip");
 unsigned triangles=0;for(unsigned i=2;i<IndexCount;++i)if(indices[i]!=indices[i-1]&&indices[i]!=indices[i-2]&&indices[i-1]!=indices[i-2])++triangles;
 Check(triangles==8192&&IndexCount==8572,"degenerate joins excluded from real triangle count");
 unsigned short sentinel=0xABCD;Check(!Indices(&sentinel,1)&&sentinel==0xABCD,"short index output refuses before writes");
 // Native GPU declaration consumes raw[4..6], native VS swizzles.zxy.
 const Vertex&v=a[1];Vec3 native=ShaderNormal(v),fine={v.value[3],v.value[4],v.value[5]};
 Check(native.y>0&&native.y<=1,"literal packed shader normal finite and upward");
 bool differs=false;for(unsigned i=0;i<Vertices;++i){Vec3 n=ShaderNormal(a[i]);Vec3 q={a[i].value[3],a[i].value[4],a[i].value[5]};if(std::fabs(n.x-q.x)>1e-5f)differs=true;}
 Check(differs,"literal native raw/GPU normal is not silently replaced by fineXYZ");
 // Native map edge uses zero outside, never edge-clamping. Owned test samples.
 std::vector<unsigned short> coast(67*67,1000);Samples edge={0,0,67,67,67,&coast[0],coast.size()};
 Check(Build(edge,0,0,&a[0],a.size()),"world corner keeps outside-world zero sample semantics");
 Check(a[0].value[3]<-.01f&&a[0].value[5]<-.01f,"corner normal includes actual native zero outside world");
 std::printf("geometry helper %u checks, %u failures\n",checks,failures);return failures?1:0;
}
