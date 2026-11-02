#pragma once
#include <cmath>
#include <cstddef>
// Research-only CPU packet builder. Caller owns validated source and destination.
// Literal pinned native40B packet and morph0 declaration, no PatchGeometry ABI.
namespace FixedTerrain {
enum {Side=65,Vertices=Side*Side,IndexCount=8572};
struct Vec3 {float x,y,z;};
struct Vertex {float value[10];};
typedef char VertexMustBe40[(sizeof(Vertex)==40)?1:-1];
struct Samples {unsigned minX,minZ,width,height,stride;const unsigned short* data;size_t sampleCount;};
inline Vec3 Cross(const Vec3&a,const Vec3&b){Vec3 r={a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};return r;}
inline Vec3 Normalize(Vec3 a){float len=std::sqrt((a.x*a.x+a.y*a.y)+a.z*a.z);if(len>1e-8f){float inv=1.0f/len;a.x*=inv;a.y*=inv;a.z*=inv;}return a;}
inline float Height(const Samples&s,int x,int z){
 if(x<0)x=0;if(z<0)z=0;if(x>16384)x=16384;if(z>16384)z=16384;
 return s.data[(unsigned(z)-s.minZ)*s.stride+unsigned(x)-s.minX]*(9800.0f/65535.0f);
}
inline Vec3 FineNormal(const Samples&s,int x,int z){
 static const int ring[6][2]={{0,-1},{1,-1},{1,0},{0,1},{-1,1},{-1,0}};
 const float h=Height(s,x,z);Vec3 v[6];
 for(unsigned i=0;i<6;++i){v[i].x=float(ring[i][0]*18);v[i].z=float(ring[i][1]*18);v[i].y=Height(s,x+ring[i][0],z+ring[i][1])-h;}
 Vec3 sum={0,0,0};for(unsigned i=0;i<6;++i){Vec3 q=Cross(v[i],v[(i+1)%6]);sum.x+=q.x;sum.y+=q.y;sum.z+=q.z;}
 sum=Normalize(sum);sum.x=-sum.x;sum.y=-sum.y;sum.z=-sum.z;return sum;
}
inline bool Supports(const Samples&s,int x0,int z0,int x1,int z1){
 if(!s.data||!s.width||!s.height||s.width>16385||s.height>16385||s.stride<s.width||s.stride>16385||s.minX>16384||s.minZ>16384||s.width>16385-s.minX||s.height>16385-s.minZ)return false;
 if(size_t(s.height-1)*s.stride+s.width>s.sampleCount)return false;
 if(x0<0)x0=0;if(z0<0)z0=0;if(x1>16384)x1=16384;if(z1>16384)z1=16384;
 return unsigned(x0)>=s.minX&&unsigned(z0)>=s.minZ&&unsigned(x1)-s.minX<s.width&&unsigned(z1)-s.minZ<s.height;
}
inline bool Build(const Samples&s,unsigned x,unsigned z,Vertex*out,size_t count){
 if(!out||count<Vertices||(x&63)||(z&63)||x>16320||z>16320||!Supports(s,int(x)-2,int(z)-2,x+66,z+66))return false;
 // Coarse-normal neighbor support needs67 fine normals, each with one source ring.
 Vec3 fine[67*67];
 for(unsigned j=0;j<67;++j)for(unsigned i=0;i<67;++i)fine[j*67+i]=FineNormal(s,int(x)+int(i)-1,int(z)+int(j)-1);
 for(unsigned j=0;j<Side;++j)for(unsigned i=0;i<Side;++i){
  const unsigned gx=x+i,gz=z+j;Vec3 n=fine[(j+1)*67+i+1],coarse=n;float target=Height(s,gx,gz);int dx=0,dz=0;
  if((gx&1u)&&(gz&1u)){dx=1;dz=1;}else if(gx&1u)dx=1;else if(gz&1u)dz=1;
  if(dx||dz){Vec3 a=fine[(j+1-dz)*67+i+1-dx],b=fine[(j+1+dz)*67+i+1+dx];Vec3 sum={a.x+b.x,a.y+b.y,a.z+b.z};coarse=Normalize(sum);target=(Height(s,int(gx)-dx,int(gz)-dz)+Height(s,int(gx)+dx,int(gz)+dz))*.5f;}
  Vertex&v=out[j*Side+i];v.value[0]=float(-147456+int(gx)*18);v.value[1]=Height(s,gx,gz);v.value[2]=float(-147456+int(gz)*18);
  v.value[3]=n.x;v.value[4]=n.y;v.value[5]=n.z;v.value[6]=coarse.x;v.value[7]=coarse.y;v.value[8]=coarse.z;v.value[9]=target;
 }
 return true;
}
inline Vec3 ShaderNormal(const Vertex&v){Vec3 result={v.value[6],v.value[4],v.value[5]};return Normalize(result);}
inline bool Indices(unsigned short*out,size_t count){
 if(!out||count<IndexCount)return false;unsigned k=0;
 for(unsigned z=0;z<64;++z){if(z){unsigned short last=out[k-1];out[k++]=last;out[k++]=last;out[k++]=unsigned short(z*65);out[k++]=unsigned short(z*65);}for(unsigned x=0;x<65;++x){out[k++]=unsigned short(z*65+x);out[k++]=unsigned short((z+1)*65+x);}}
 return k==IndexCount;
}
} // namespace FixedTerrain
