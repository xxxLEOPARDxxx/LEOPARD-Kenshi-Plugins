#pragma once
#include <float.h>
#include <cmath>
namespace GsmZoom {
inline bool StepValues(float minimum,float current){return _finite(minimum)&&_finite(current)&&minimum>0&&current>=minimum;}
inline float ButtonStep(float minimum,float current){
 if(!StepValues(minimum,current))return minimum;
 const float relative=current*.2f;return relative>minimum?relative:minimum;
}
inline int WheelStep(int relative,float minimum,float current){
 if(!relative||!StepValues(minimum,current))return relative;
 const double gain=double(current)/(double(minimum)*4);if(gain<=1)return relative;
 const double scaled=double(relative)*gain;
 // Never abs(INT_MIN) or cast an overflowing double to int. Preserve raw
 // input proportions and let native wheel code retain its cursor anchor.
 if(scaled>=32767)return 32767;if(scaled<=-32767)return -32767;
 return int(scaled>=0?std::floor(scaled+.5):std::ceil(scaled-.5));
}
struct StepScope {
 float&slot;float previous;
 StepScope(float&value,float step):slot(value),previous(value){slot=step;}
 ~StepScope(){slot=previous;}
private:StepScope(const StepScope&);StepScope&operator=(const StepScope&);
};
struct DepthScope {unsigned&slot;DepthScope(unsigned&value):slot(value){++slot;}~DepthScope(){--slot;}private:DepthScope(const DepthScope&);DepthScope&operator=(const DepthScope&);};
}
