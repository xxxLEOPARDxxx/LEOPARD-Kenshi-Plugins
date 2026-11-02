#pragma once
#include "../atlas-view/AtlasRelight.h"
namespace AtlasModels {
typedef AtlasView::U64 U64;
const unsigned Side=256,MaxDraws=16,MaxStreams=16,MaxVertices=65536,MaxIndices=196608;
const U64 BorrowCap=32ULL*1024*1024,PrivateBytes=1048576+128+16;
enum Semantic {Position,Normal,Colour,Texcoord};
struct Element {unsigned semantic,slot,offset;DXGI_FORMAT format;};
struct Shape {unsigned streamCount,stride[16],offset[16],bytes[16],vertexStart,vertexCount,indexCount,indexStart,indexBytes;DXGI_FORMAT indexFormat;Element element[4];};
inline bool Fits(U64 caller){return caller<=AtlasView::GpuCap-PrivateBytes;}
inline unsigned FormatBytes(DXGI_FORMAT f){switch(f){case DXGI_FORMAT_R32G32B32A32_FLOAT:return 16;case DXGI_FORMAT_R32G32B32_FLOAT:return 12;case DXGI_FORMAT_R32G32_FLOAT:return 8;case DXGI_FORMAT_R8G8B8A8_UNORM:return 4;default:return 0;}}
inline const char*Name(unsigned sem){static const char*names[]={"POSITION","NORMAL","COLOR","TEXCOORD"};return sem<4?names[sem]:"";}
inline bool ValidShape(const Shape&s){
    if(!s.streamCount||s.streamCount>MaxStreams||!s.vertexCount||s.vertexCount>MaxVertices||!s.indexCount||s.indexCount>MaxIndices||s.indexCount%3)return false;
    const unsigned indexSize=s.indexFormat==DXGI_FORMAT_R16_UINT?2:s.indexFormat==DXGI_FORMAT_R32_UINT?4:0;
    if(!indexSize||(U64(s.indexStart)+s.indexCount)*indexSize>s.indexBytes)return false;
    unsigned mask=0,slots=0;
    for(unsigned i=0;i<4;++i){const Element&e=s.element[i];if(e.semantic>=4||mask&(1u<<e.semantic)||e.slot>=s.streamCount)return false;mask|=1u<<e.semantic;slots|=1u<<e.slot;
        const bool vec=e.format==DXGI_FORMAT_R32G32B32_FLOAT||e.format==DXGI_FORMAT_R32G32B32A32_FLOAT;
        if((e.semantic<2&&!vec)||(e.semantic==2&&e.format!=DXGI_FORMAT_R32G32B32A32_FLOAT&&e.format!=DXGI_FORMAT_R8G8B8A8_UNORM)||(e.semantic==3&&e.format!=DXGI_FORMAT_R32G32_FLOAT))return false;
        if(e.offset%4||U64(e.offset)+FormatBytes(e.format)>s.stride[e.slot])return false;
    }
    if(mask!=15)return false;
    for(unsigned i=0;i<s.streamCount;++i){if(!(slots&(1u<<i))){if(s.stride[i]||s.offset[i]||s.bytes[i])return false;continue;}
        if(!s.stride[i]||s.stride[i]>2048||s.stride[i]%4||s.offset[i]%4||U64(s.offset[i])+(U64(s.vertexStart)+s.vertexCount)*s.stride[i]>s.bytes[i])return false;
    }return true;
}
struct View {float viewProjection[16],cameraScene[3],farClip;unsigned generation;View():farClip(0),generation(0){std::memset(viewProjection,0,sizeof(viewProjection));std::memset(cameraScene,0,sizeof(cameraScene));}};
inline bool Finite(const float*f,unsigned count){for(unsigned i=0;i<count;++i)if(!_finite(f[i]))return false;return true;}
inline bool TopDownView(float x,float cameraY,float z,float width,float height,float nearClip,float farClip,unsigned generation,View&out){
    const float input[7]={x,cameraY,z,width,height,nearClip,farClip};if(!Finite(input,7)||width<=0||height<=0||nearClip<0||farClip<=nearClip||!generation)return false;
    const float range=farClip-nearClip;if(!_finite(range)||range<=0)return false;View v;v.viewProjection[0]=2/width;v.viewProjection[3]=-x*v.viewProjection[0];v.viewProjection[6]=-2/height;v.viewProjection[7]=-z*v.viewProjection[6];v.viewProjection[9]=-1/range;v.viewProjection[11]=(cameraY-nearClip)/range;v.viewProjection[15]=1;
    v.cameraScene[0]=x;v.cameraScene[1]=cameraY;v.cameraScene[2]=z;v.farClip=farClip;v.generation=generation;if(!Finite(v.viewProjection,16))return false;out=v;return true;
}
inline void MatrixConstants(const float*world,const View&v,float*out){
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){float q=0;for(unsigned k=0;k<4;++k)q+=v.viewProjection[r*4+k]*world[k*4+c];out[c*4+r]=q;out[16+c*4+r]=world[r*4+c];}
}
}
