#pragma once
#include <float.h>
#include <cmath>
#include <algorithm>
namespace GsmMapInfo {
struct Values{double zoom;bool loaded;Values():zoom(0),loaded(false){}};
inline bool Make(double current,double peak,bool loaded,Values&out){
 out=Values();if(!_finite(current)||!_finite(peak)||current<=0||peak<=0)return false;
 Values v;v.zoom=current/(2*peak);v.loaded=loaded;if(!_finite(v.zoom)||v.zoom>1e9)return false;out=v;return true;
}
struct Token{unsigned __int64 zoom;bool loaded;Token():zoom(0),loaded(false){}explicit Token(const Values&v):zoom((unsigned __int64)std::floor(v.zoom*100+.5)),loaded(v.loaded){}bool Same(const Token&b)const{return zoom==b.zoom&&loaded==b.loaded;}};
inline bool Complete(unsigned expected,unsigned shaded,bool centerOnly){return !centerOnly&&expected!=0&&shaded==expected;}
}
