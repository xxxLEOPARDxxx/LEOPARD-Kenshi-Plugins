#include <cstdio>
#include <cmath>
#include <limits>
#include <climits>
#include "../GsmZoomStep.h"
static unsigned checks=0;
static bool C(bool condition,const char*name){++checks;if(!condition){std::printf("FAIL %s at %u\n",name,checks);return false;}return true;}
int main(){
 const float minima[]={.00001f,.03125f,.1209716796875f,.125f,.5f,1.f,10.f};
 const int raw[]={INT_MIN,-32768,-120,-1,0,1,120,32768,INT_MAX};
 for(unsigned m=0;m<sizeof(minima)/sizeof(minima[0]);++m)for(unsigned ratio=1;ratio<=1024;++ratio){
  const float minimum=minima[m],current=minimum*ratio,step=GsmZoom::ButtonStep(minimum,current);
  if(!C(step>=minimum&&step<=current,"bounded positive button step"))return 1;
  if(!C(ratio<2||current-step>=step,"temporary Out floor cannot obstruct true minimum"))return 1;
  float slot=minimum;{GsmZoom::StepScope scope(slot,step);if(!C(slot==step,"scope applies input step"))return 1;}if(!C(slot==minimum,"scope restores exact native base"))return 1;
  for(unsigned r=0;r<sizeof(raw)/sizeof(raw[0]);++r){int value=GsmZoom::WheelStep(raw[r],minimum,current);
   if(!C((raw[r]==0&&value==0)||(raw[r]<0&&value<0)||(raw[r]>0&&value>0),"raw wheel sign/zero preserved"))return 1;
   if(!C(ratio<=4?value==raw[r]:(value>=-32767&&value<=32767),"unchanged near zoom; bounded accelerated int"))return 1;
  }
 }
 const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
 if(!C(!GsmZoom::StepValues(0,1)&&!GsmZoom::StepValues(-1,1)&&!GsmZoom::StepValues(1,.5f)&&!GsmZoom::StepValues(nan,1)&&!GsmZoom::StepValues(1,inf),"invalid state rejected"))return 1;
 if(!C(GsmZoom::WheelStep(INT_MIN,nan,1)==INT_MIN&&GsmZoom::WheelStep(INT_MAX,1,inf)==INT_MAX,"invalid input bypass avoids overflow"))return 1;
 for(unsigned m=0;m<sizeof(minima)/sizeof(minima[0]);++m){float current=minima[m]*128;unsigned n=0;while(current>minima[m]&&n<256){float step=GsmZoom::ButtonStep(minima[m],current);current=(current-step>step?current-step:step);++n;}if(!C(current==minima[m]&&n<40,"repeated Out reaches exact native minimum"))return 1;}
 unsigned depth=0;try{GsmZoom::DepthScope scope(depth);throw 1;}catch(...){}if(!C(depth==0,"exception restores reentry depth"))return 1;
 std::printf("PASS zoom step policy %u checks\n",checks);return 0;
}
