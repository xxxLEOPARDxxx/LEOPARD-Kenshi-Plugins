#include <vector>
#include <cmath>
#include <cstdio>
#include <cstddef>
template<class T>T Clamp(T a,T b,T c){return a<b?b:a>c?c:a;}
template<class T>T Hi(T a,T b){return a>b?a:b;}
int g_rw=100,g_rh=100,calls=0;
void DrawDisc(float,float,float,unsigned,unsigned,float){++calls;}
#include "camera_stroke_under_test.h"
int main(){int failures=0,checks=0;
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d\n",__LINE__);}}while(0)
std::vector<float>x(1,-23.f),y(1,50.f);DrawCameraRange(x,y,20,false,0,8);C(calls>0);
calls=0;DrawCameraRange(x,y,20,true,0,8);C(calls>0);
calls=0;DrawCameraRange(x,y,20,false,0,1);C(calls==0);
x[0]=123;calls=0;DrawCameraRange(x,y,20,false,0,8);C(calls>0);
x[0]=50;y[0]=-23;calls=0;DrawCameraRange(x,y,20,false,0,8);C(calls>0);
y[0]=123;calls=0;DrawCameraRange(x,y,20,false,0,8);C(calls>0);
y[0]=50;calls=0;DrawCameraRange(x,y,72,false,0,8);C(calls>0);
calls=0;DrawCameraRange(x,y,100,false,0,8);C(calls==0);
x[0]=-1000000;calls=0;DrawCameraRange(x,y,20,true,0,8);C(calls==0);
x.clear();y.clear();calls=0;DrawCameraRange(x,y,20,true,0,8);C(calls==0);
printf("%d production camera stroke checks, %d failures\n",checks,failures);return failures?1:0;}
