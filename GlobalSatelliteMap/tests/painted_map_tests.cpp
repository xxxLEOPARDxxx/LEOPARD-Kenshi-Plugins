#include "../PaintedMapPolicy.h"
#include <stdio.h>
#include <float.h>
#include <math.h>

static int tests=0,failures=0;
#define CHECK(x) do{++tests;if(!(x)){++failures;printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
static bool near(double a,double b){return fabs(a-b)<1e-10;}
int main()
{
    using namespace SatellitePaintedMap;
    CHECK(ImageShape(0,1024,1024,1,1,0,12,4194304));
    CHECK(ImageShape(1,2048,2048,1,1,0,13,16777216));
    CHECK(!ImageShape(2,2048,2048,1,1,0,13,16777216));
    CHECK(!ImageShape(0,2048,2048,1,1,0,13,16777216));
    CHECK(!ImageShape(1,2048,2048,1,1,1,13,16777216));
    CHECK(!ImageShape(1,2048,2048,1,1,0,14,16777216));
    CHECK(!ImageShape(1,2048,2048,1,1,0,13,16777215));
    CHECK(!ImageShape(1,2048,2048,1,6,0,13,16777216));
    CHECK(!ImageShape(1,2048,2048,4,1,0,13,16777216));
    CHECK(!ImageShape(1,(size_t(1)<<32)+2048,2048,1,1,0,13,16777216));
    Tile t={0};double u=0,v=0;
    CHECK(FromIndex(0,0,t));
    CHECK(t.x==0&&t.z==0&&t.minX==-147474&&t.minZ==-147474&&t.maxX==-110574&&t.maxZ==-110574);
    CHECK(FromIndex(7,7,t));CHECK(t.minX==110574&&t.maxX==147474);
    CHECK(!FromIndex(-1,0,t));CHECK(!FromIndex(0,8,t));
    CHECK(FromIndex(3,3,t));
    CHECK(Uv(t,t.minX,t.minZ,u,v)&&u==0&&v==0);
    CHECK(Uv(t,t.maxX,t.maxZ,u,v)&&u==1&&v==1);
    CHECK(Uv(t,-36864,-36864,u,v)&&near(u,18.0/36900)&&near(v,18.0/36900));
    CHECK(!Uv(t,t.maxX+1,t.maxZ,u,v));
    CHECK(Point(-147456,-147456,t)&&t.x==0&&t.z==0);
    CHECK(Point(0,0,t)&&t.x==4&&t.z==4);
    CHECK(Point(-0.001,-0.001,t)&&t.x==3&&t.z==3);
    CHECK(Point(-1e-14,-1e-14,t)&&t.x==3&&t.z==3);
    CHECK(Point(147456,147456,t)&&t.x==7&&t.z==7);
    CHECK(!Point(-147456.001,0,t));CHECK(!Point(0,147456.001,t));
    Tile tiles[4];unsigned count=99;
    for(int z=0;z<4;++z)for(int x=0;x<4;++x)
    {
        const double loX=-147456+x*73728.0,loZ=-147456+z*73728.0;
        CHECK(Cover(loX,loZ,loX+73728,loZ+73728,tiles,count)&&count==4);
        CHECK(tiles[0].x==2*x&&tiles[0].z==2*z&&tiles[3].x==2*x+1&&tiles[3].z==2*z+1);
    }
    CHECK(Cover(100,100,100.000001,100.000001,tiles,count)&&count==1&&tiles[0].x==4);
    CHECK(Cover(0,0,1e-14,1e-14,tiles,count)&&count==1&&tiles[0].x==4);
    CHECK(Cover(-36864,-36864,0,0,tiles,count)&&count==1&&tiles[0].x==3&&tiles[0].z==3);
    CHECK(!Cover(-147456,-147456,147456,147456,tiles,count)&&count==0);
    CHECK(!Cover(0,0,0,1,tiles,count)&&count==0);
    CHECK(!Cover(-147457,0,0,1,tiles,count)&&count==0);
    CHECK(Patch(-36864,-36864,0,0,t)&&t.x==3&&t.z==3);
    CHECK(Patch(-1152,-1152,0,0,t)&&t.x==3&&t.z==3);
    CHECK(Patch(0,0,1152,1152,t)&&t.x==4&&t.z==4);
    CHECK(!Patch(-1152,-1152,1152,1152,t));
    CHECK(!Patch(-73728,-73728,0,0,t));
    CHECK(!Patch(-147474,0,-147456,1152,t));
    const double nan=sqrt(-1.0);
    CHECK(!Point(nan,0,t));CHECK(!Cover(0,0,nan,1,tiles,count)&&count==0);
    CHECK(!Patch(0,0,DBL_MAX,1,t));CHECK(!Uv(t,nan,0,u,v));
    printf("%d production painted-map bounds/UV/patch checks, %d failures\n",tests,failures);
    return failures?1:0;
}
