#pragma once
#include <algorithm>
#include "../ModelPolicy.h"
namespace LoadedInstances {
typedef AtlasView::U64 U64;typedef AtlasModels::View View;
const unsigned MaxRecords=256,MaxBatches=64,MaxMaterials=16,EntryLimit=4096;
const U64 PrivateBytes=1048576+160+64+48,BufferCap=32ULL*1024*1024,TextureCap=64ULL*1024*1024;
const unsigned MaxTotalIndices=2097152,MaxTotalVertices=2097152;
enum Kind {Dual43,Alpha48};enum Layout {Coloured64,Plain60,Plain56};
inline bool AllowsFamily(unsigned family,unsigned mask){return family<5&&mask&&!(mask&~3u)&&(mask&(family<2?1u:2u));}
inline bool RequiresNativeLighting(Kind k){return k==Alpha48;}
// Texture2D metal maps are black at every mip. BORDER is the only address
// mode that can introduce a nonzero sample; W is unused for these 2D views.
inline bool ZeroMetalSampler(const D3D11_SAMPLER_DESC&s){return AtlasModels::Finite(s.BorderColor,4)&&((s.AddressU!=D3D11_TEXTURE_ADDRESS_BORDER&&s.AddressV!=D3D11_TEXTURE_ADDRESS_BORDER)||s.BorderColor[0]==0);}
inline bool SupportedPhysical(unsigned registryFloor,int physicalFloor,bool instanced,bool shell,bool emissive){return registryFloor==0&&physicalFloor==0&&instanced&&!shell&&!emissive;}
struct Shape {
 Layout layout;unsigned vertexStart,vertexCount,vertexBytes,vertexOffset,indexStart,indexCount,indexBytes;DXGI_FORMAT indexFormat;
 Shape():layout(Coloured64),vertexStart(0),vertexCount(0),vertexBytes(0),vertexOffset(0),indexStart(0),indexCount(0),indexBytes(0),indexFormat(DXGI_FORMAT_UNKNOWN){}
 unsigned Stride()const{return layout==Coloured64?64:layout==Plain60?60:layout==Plain56?56:0;}
};
inline bool ValidShape(const Shape&s,Kind k){
 if(k!=Dual43&&k!=Alpha48)return false;if((k==Dual43)!=(s.layout==Coloured64))return false;
 const unsigned stride=s.Stride(),indexBytes=s.indexFormat==DXGI_FORMAT_R16_UINT?2:s.indexFormat==DXGI_FORMAT_R32_UINT?4:0;
 return stride&&indexBytes&&s.vertexCount&&s.vertexCount<=65536&&s.indexCount&&s.indexCount<=196608&&s.indexCount%3==0&&s.vertexOffset%4==0&&U64(s.vertexOffset)+(U64(s.vertexStart)+s.vertexCount)*stride<=s.vertexBytes&&(U64(s.indexStart)+s.indexCount)*indexBytes<=s.indexBytes;
}
struct Surface {
 float tiling[2],glossMult,dustColour[4],dustAmount[3],waterHeightRel,wetness,threshold;
 Surface(){std::memset(this,0,sizeof(*this));}
};
struct VertexConstants {float tiling[4],cameraDir[4],matrix[32];};
struct PixelConstants {float dustColour[4],dustThreshold[4],cameraFar[4],waterWetGloss[4];};
typedef char VertexAbi[sizeof(VertexConstants)==160?1:-1];typedef char PixelAbi[sizeof(PixelConstants)==64?1:-1];typedef char SurfaceAbi[sizeof(Surface)==52?1:-1];
inline bool Basis(const float*m){if(!m||!AtlasModels::Finite(m,16)||m[12]!=0||m[13]!=0||m[14]!=0||m[15]!=1)return false;const double a=m[0],b=m[1],c=m[2],d=m[4],e=m[5],f=m[6],g=m[8],h=m[9],i=m[10];const double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);return _finite(det)&&det>1e-10;}
inline bool Pack(const float*world,const View&v,const Surface&s,Kind kind,VertexConstants&vc,PixelConstants&pc){
 if(!Basis(world)||!AtlasModels::Finite(v.viewProjection,16)||!AtlasModels::Finite(v.cameraScene,3)||!_finite(v.farClip)||v.farClip<=0||!AtlasModels::Finite(reinterpret_cast<const float*>(&s),13)||(kind!=Dual43&&kind!=Alpha48)||(kind==Alpha48&&s.threshold>=1))return false;
 std::memset(&vc,0,sizeof(vc));std::memset(&pc,0,sizeof(pc));vc.tiling[0]=s.tiling[0];vc.tiling[1]=s.tiling[1];
 // InstanceBatchHW supplies identity as Renderable world transform. Native
 // ACT_CAMERA_DIRECTION_OBJECT_SPACE is therefore raw-scene private down.
 vc.cameraDir[1]=-1;
 for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){vc.matrix[c*4+r]=v.viewProjection[r*4+c];vc.matrix[16+c*4+r]=r==c?1.f:0.f;}
 std::memcpy(pc.dustColour,s.dustColour,16);std::memcpy(pc.dustThreshold,s.dustAmount,12);pc.dustThreshold[3]=kind==Alpha48?s.threshold:0;
 std::memcpy(pc.cameraFar,v.cameraScene,12);pc.cameraFar[3]=v.farClip;pc.waterWetGloss[0]=s.waterHeightRel;pc.waterWetGloss[1]=s.wetness;pc.waterWetGloss[2]=s.glossMult;
 return AtlasModels::Finite(reinterpret_cast<const float*>(&vc),40)&&AtlasModels::Finite(reinterpret_cast<const float*>(&pc),16);
}
inline bool Fits(U64 caller){return caller<=AtlasView::GpuCap-PrivateBytes;}
inline unsigned TextureCount(Kind k){return k==Dual43?7:k==Alpha48?4:0;}
inline U64 TextureBytes(const D3D11_TEXTURE2D_DESC&d){U64 total=0;unsigned w=d.Width,h=d.Height;for(unsigned n=0;n<d.MipLevels;++n){if(d.Format==DXGI_FORMAT_BC1_UNORM)total+=U64((w+3)/4)*((h+3)/4)*8;else if(d.Format==DXGI_FORMAT_BC3_UNORM)total+=U64((w+3)/4)*((h+3)/4)*16;else if(d.Format==DXGI_FORMAT_R8G8B8A8_UNORM)total+=U64(w)*h*4;else return 0;w=(std::max)(w/2,1u);h=(std::max)(h/2,1u);}return total;}
}
