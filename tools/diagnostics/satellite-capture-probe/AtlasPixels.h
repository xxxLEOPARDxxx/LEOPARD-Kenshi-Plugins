#pragma once
#include <cmath>
#include <float.h>
namespace AtlasCapture {
struct Camera {float width,height,farClip,y,rightY,upY,backY;};
inline bool Height(const Camera& c,unsigned side,unsigned x,unsigned z,float depth,float& height){
 height=0;
 if(!side||x>=side||z>=side||!_finite(depth)||depth<=0||depth>=1||!_finite(c.farClip)||c.farClip<=0)return false;
 const float radial=depth*c.farClip;
 const float lx=((x+.5f)/side-.5f)*c.width,ly=((z+.5f)/side-.5f)*-c.height;
 const float rs=radial*radial,ls=lx*lx+ly*ly,forward=rs-ls;
 if(!_finite(rs)||!_finite(ls)||forward < -4.76837158203125e-7f*(rs>ls?rs:ls))return false;
 const float value=c.y+c.rightY*lx+c.upY*ly-c.backY*std::sqrt(forward>0?forward:0);
 if(!_finite(value)||value < -10000 ||value>20000)return false;
 height=value;return true;
}
}
