#include <cstdio>
#include "AtlasWater.h"
int main(){unsigned n=0,bad=0;
#define CHECK(x) do{++n;if(!(x)){++bad;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 CHECK(sizeof(AtlasWater::Uniforms)==192);
 CHECK(AtlasWater::Fits(AtlasWater::Cap-AtlasWater::PrivateBytes));
 CHECK(!AtlasWater::Fits(AtlasWater::Cap-AtlasWater::PrivateBytes+1));
 AtlasWater::Uniforms v={};v.sourceWindow[0]=2048;v.tileBounds[2]=v.tileBounds[3]=73728;v.waterPlane[1]=1;
 v.nativeMapBounds[0]=v.nativeMapBounds[1]=-147456;v.nativeMapBounds[2]=v.nativeMapBounds[3]=147456;
 v.waterSunDirection[1]=v.parallelView[1]=1;v.materialParams[0]=1;v.materialParams[1]=120;v.materialParams[2]=.9f;
 CHECK(v.Valid(512,512));v.sourceWindow[1]=1536;v.sourceWindow[2]=1536;CHECK(v.Valid(512,512));
 v.sourceWindow[1]=1537;CHECK(!v.Valid(512,512));v.sourceWindow[1]=.5f;CHECK(!v.Valid(512,512));
 v.sourceWindow[1]=0;CHECK(!v.Valid(513,512));v.sourceWindow[0]=8192;CHECK(!v.Valid(512,512));
 v.sourceWindow[0]=2048;v.materialParams[1]=0;CHECK(!v.Valid(512,512));v.materialParams[1]=120;v.nativeMapBounds[2]=-147456;CHECK(!v.Valid(512,512));
 std::printf("Water policy: %u checks, %u failures\n",n,bad);return bad?1:0;
}
