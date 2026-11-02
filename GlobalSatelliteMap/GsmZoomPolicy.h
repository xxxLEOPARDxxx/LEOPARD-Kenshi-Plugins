#pragma once
#include <float.h>
namespace GsmZoom {
enum Result {Unchanged,Apply,Refused,ForeignChange};
struct Action {float maximum;bool clamp;Action():maximum(0),clamp(false){}};
class Lease {
 float saved_,applied_;bool owned_,foreign_;
public:
 Lease():saved_(0),applied_(0),owned_(false),foreign_(false){}
 void Reset(){saved_=applied_=0;owned_=foreign_=false;}
 void AllowRequest(){foreign_=false;}
 bool Owned()const{return owned_;}
 // peak is native marker threshold[2]; the audited constructor sets the native
 // maximum to exactly2*peak. It survives MapScreen::clear, so never multiply the
 // currently overridden maximum. The caller keys this lease to a live WidgetRef.
 Result Plan(bool enabled,int multiplier,float minimum,float peak,float maximum,float current,int width,int height,Action&out){
  out=Action();
  if(multiplier<1||multiplier>8||!_finite(minimum)||!_finite(peak)||!_finite(maximum)||!_finite(current)||minimum<=0||peak<=0||maximum<minimum||current<=0||width<=0||height<=0)return Refused;
  if(owned_&&maximum!=applied_){owned_=false;foreign_=true;return ForeignChange;}
  const bool extend=enabled&&multiplier>1;
  if(!extend&&!owned_)return Unchanged;
  if(extend&&foreign_)return Unchanged;
  const double wanted=extend?double(peak)*2*multiplier:(saved_>minimum?saved_:minimum);
  if(!_finite(wanted)||wanted<minimum||wanted>FLT_MAX)return Refused;
  const float value=float(wanted);if(maximum==value)return Unchanged;
  // Preserve the viewer's validated integer-coordinate range before native
  // resize. No new textures or source data are allocated by this option.
  if(double(width)/current*wanted>999999||double(height)/current*wanted>999999)return Refused;
  if(extend){if(!owned_)saved_=maximum;applied_=value;owned_=true;}else owned_=false;
  out.maximum=value;out.clamp=current>value;return Apply;
 }
};
}
