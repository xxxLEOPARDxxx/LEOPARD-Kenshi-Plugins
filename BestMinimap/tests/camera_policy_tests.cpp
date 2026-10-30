#include <cstdio>
#include "../CameraRangePolicy.h"
int main(){int failures=0;
#define C(x) if(!(x)){++failures;std::printf("FAIL %d\n",__LINE__);}
C(BestCamera::Radius(2250.0f,5062500.0f)==2250.0f);C(BestCamera::Radius(0,0)==0);
C(BestCamera::Radius(2250,1)==0);C(BestCamera::Radius(-2250,5062500)==0);
C(BestCamera::Radius(3000,9000000)==3000);C(BestCamera::Radius(50001,2500100000.0f)==0);
unsigned char code[BestCamera::ProfileSize];BestCamera::StockProfile(code);C(BestCamera::Matches(code,sizeof(code)));
code[0xf1]^=1;C(BestCamera::Matches(code,sizeof(code)));code[0x102]^=1;C(!BestCamera::Matches(code,sizeof(code)));
C(!BestCamera::Matches(code,3));C(BestCamera::ArrowSize(0,10)==22);C(BestCamera::ArrowSize(100,10)==64);
C(BestCamera::ArrowSize(2,5)==4);C(BestCamera::Stroke(0)==1);C(BestCamera::Stroke(99)==8);
std::printf("15 camera policy checks, %d failures\n",failures);return failures?1:0;}
