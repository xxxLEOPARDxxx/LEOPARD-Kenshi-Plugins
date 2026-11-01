#pragma once
#include <cmath>
#include <cfloat>
namespace ExposurePolicy
{
inline bool Finite(float v){return v==v && v<=FLT_MAX && v>=-FLT_MAX;}
inline bool Decode(float key,float logLuminance,float &luminance)
{
    if(!Finite(key)||key<=0||!Finite(logLuminance))return false;
    const double decoded=std::exp(double(logLuminance));
    if(!(decoded>0)||decoded>FLT_MAX)return false;
    const float value=float(decoded);
    if(!Finite(value)||value<=0||!Finite(key/value))return false;
    luminance=value;return true;
}
}
