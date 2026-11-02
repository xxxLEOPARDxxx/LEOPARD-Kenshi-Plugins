#pragma once
#include "ModelAdmission.h"
#include "decode/AtlasModelDecode.h"
namespace AtlasModelRegion {
struct Bounds {float lo[3],hi[3];Bounds(){std::memset(this,0,sizeof(*this));}};
inline bool WorldBounds(const float*matrix,const float*origin,const float*local,Bounds&out){
 if(!matrix||!origin||!local||!NativeModels::RawBasis(matrix)||!AtlasModels::Finite(origin,3)||!AtlasModels::Finite(local,6))return false;
 for(unsigned axis=0;axis<3;++axis)if(local[axis]>local[axis+3])return false;
 Bounds next;
 for(unsigned corner=0;corner<8;++corner)for(unsigned axis=0;axis<3;++axis){
  double value=double(matrix[axis*4+3])+origin[axis];
  for(unsigned k=0;k<3;++k)value+=double(matrix[axis*4+k])*local[k+((corner&(1u<<k))?3:0)];
  if(!_finite(value)||value>FLT_MAX||value< -FLT_MAX)return false;float f=float(value);
  if(!corner||f<next.lo[axis])next.lo[axis]=f;if(!corner||f>next.hi[axis])next.hi[axis]=f;
 }
 out=next;return true;
}
inline bool Intersects(const Bounds&b,const AtlasModelDecode::Footprint&f){
 const double endX=double(f.minX)+f.width,endZ=double(f.minZ)+f.height;
 return _finite(f.minX)&&_finite(f.minZ)&&_finite(f.width)&&_finite(f.height)&&f.width>0&&f.height>0&&
  double(b.lo[0])<=endX&&double(b.hi[0])>=f.minX&&double(b.lo[2])<=endZ&&double(b.hi[2])>=f.minZ;
}
// Match the decoder's exact footprint arithmetic. Choose a real camera above
// the model, with vertical range comparable to horizontal extent to avoid
// catastrophic cancellation when reconstructing radial depth near tile edges.
inline bool View(const Bounds&b,const float*origin,const AtlasModelDecode::Footprint&f,unsigned generation,AtlasModels::View&out){
 if(!origin||!AtlasModels::Finite(origin,3)||f.nearClip!=0||!Intersects(b,f))return false;
 const double span=(std::max)(double(f.width),double(f.height));
 const double modelHeight=double(b.hi[1])-b.lo[1];if(!_finite(modelHeight)||modelHeight<0)return false;
 const double lift=(std::max)(span,1.0),top=double(b.hi[1])-origin[1]+lift;
 const double farDistance=lift+modelHeight+1.0;
 if(!_finite(top)||!_finite(farDistance)||top>FLT_MAX||top< -FLT_MAX||farDistance>FLT_MAX)return false;
 const float rawX=f.minX-origin[0],rawZ=f.minZ-origin[2];
 return AtlasModels::TopDownView(rawX+f.width*.5f,float(top),rawZ+f.height*.5f,f.width,f.height,f.nearClip,float(farDistance),generation,out);
}
}
