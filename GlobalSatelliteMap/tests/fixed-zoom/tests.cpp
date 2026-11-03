#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <limits>
#include "../../GsmZoomStep.h"
unsigned checks=0;void C(bool ok,const char*why){++checks;if(!ok){printf("FAIL %s\n",why);throw 1;}}
int main(){try{
 const float expected[]={2,2.5f,3,4,5,6,8,12,16};float current=1.5f,target=0;
 for(unsigned n=0;n<9;++n){C(GsmZoom::DiscreteTarget(.1209716796875f,1,16,current,1,target)&&target==expected[n],"exact forward ladder");current=target;}
 for(unsigned n=8;n>0;--n){C(GsmZoom::DiscreteTarget(.1209716796875f,1,16,current,-1,target)&&target==expected[n-1],"exact reverse ladder");current=target;}
 C(GsmZoom::DiscreteTarget(.125f,1,16,4.48f,1,target)&&target==5,"arbitrary2.24 snaps forward2.5");
 C(GsmZoom::DiscreteTarget(.125f,1,16,4.48f,-1,target)&&target==4,"arbitrary2.24 snaps backward2");
 C(GsmZoom::DiscreteTarget(.125f,1,10,8,99,target)&&target==10,"non-grid setting5 cap reachable");
 C(GsmZoom::DiscreteTarget(.125f,1,16,16,1,target)&&target==16,"upper endpoint idempotent");
 C(GsmZoom::DiscreteTarget(.125f,1,16,.125f,-1,target)&&target==.125f,"native lower endpoint exact");
 C(GsmZoom::WheelNotches(1)==1&&GsmZoom::WheelNotches(-1)==-1&&GsmZoom::WheelNotches(120)==1&&GsmZoom::WheelNotches(-240)==-2,"wheel batch and high resolution direction");
 C(GsmZoom::WheelNotches(2147483647)==64&&GsmZoom::WheelNotches(-2147483647-1)==-64&&GsmZoom::WheelNotches(0)==0,"wheel extreme bounded without abs overflow");
 C(!GsmZoom::DiscreteTarget(0,1,16,2,1,target)&&!GsmZoom::DiscreteTarget(.125f,0,16,2,1,target)&&!GsmZoom::DiscreteTarget(.125f,1,16,std::numeric_limits<float>::quiet_NaN(),1,target),"invalid native scalars refused");
 float base=.125f,maximum=16;try{GsmZoom::TargetScope scope(base,maximum,5);C(base==5&&maximum==5,"exact temporary clamp interval");throw 1;}catch(...){}C(base==.125f&&maximum==16,"exception restores both exact native bounds");
 printf("PASS %u fixed zoom controls\n",checks);return 0;
 }catch(...){return 1;}}
