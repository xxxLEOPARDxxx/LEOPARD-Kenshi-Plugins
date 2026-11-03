#pragma once
#include <float.h>
#include <cmath>
namespace GsmZoom {
// Native wheel input arrives in OIS units (120 per notch). Small trackpad
// events still move one level; extreme batches are bounded before negation.
inline int WheelNotches(int relative){if(!relative)return 0;if(relative>=7680)return 64;if(relative<=-7680)return -64;int n=(relative>0?relative:-relative)/120;if(!n)n=1;return relative>0?n:-n;}
inline bool DiscreteTarget(float minimum,float peak,float maximum,float current,int steps,float&out){
 out=current;const double normal=double(peak)*2;
 if(!_finite(minimum)||!_finite(peak)||!_finite(maximum)||!_finite(current)||minimum<=0||peak<=0||!_finite(normal)||normal>FLT_MAX||minimum>normal||maximum<normal||maximum>normal*8||current<=0)return false;
 float grid[64];unsigned count=0;grid[count++]=minimum;
 // Only the tiny native zoom-out tail depends on the viewport minimum. This
 // bounded doubling avoids an arbitrarily large first jump at unusual sizes.
 double tailValue=minimum;while(tailValue*2<normal/16&&count<32){tailValue*=2;grid[count++]=float(tailValue);}
 static const double factors[]={.0625,.125,.25,.5,.75,1,1.25,1.5,2,2.5,3,4,6,8};
 for(unsigned n=0;n<sizeof(factors)/sizeof(factors[0]);++n){float value=float(normal*factors[n]);if(value>grid[count-1]&&value<maximum)grid[count++]=value;}
 if(maximum>grid[count-1])grid[count++]=maximum;
 const double tolerance=normal*1e-6;const bool forward=steps>0;unsigned moves=steps>=64||steps<=-64?64:unsigned(steps<0?-steps:steps);float value=current;
 for(unsigned n=0;n<moves;++n){float next=forward?maximum:minimum;if(forward){for(unsigned i=0;i<count;++i)if(double(grid[i])>double(value)+tolerance){next=grid[i];break;}}
  else{for(unsigned i=count;i>0;--i)if(double(grid[i-1])<double(value)-tolerance){next=grid[i-1];break;}}value=next;}
 out=value;return true;
}
// The audited native wheel clamps before calculating its cursor anchor. A
// singleton interval gives an exact target without modifying entry current,
// reproducing native anchor mathematics or touching the process code.
struct TargetScope{float&minimum,&maximum;float savedMinimum,savedMaximum;TargetScope(float&low,float&high,float target):minimum(low),maximum(high),savedMinimum(low),savedMaximum(high){minimum=maximum=target;}~TargetScope(){minimum=savedMinimum;maximum=savedMaximum;}private:TargetScope(const TargetScope&);TargetScope&operator=(const TargetScope&);};
inline bool StepValues(float minimum,float current){return _finite(minimum)&&_finite(current)&&minimum>0&&current>=minimum;}
struct DepthScope {unsigned&slot;DepthScope(unsigned&value):slot(value){++slot;}~DepthScope(){--slot;}private:DepthScope(const DepthScope&);DepthScope&operator=(const DepthScope&);};
}
