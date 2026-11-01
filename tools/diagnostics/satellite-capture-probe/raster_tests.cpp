#include "RasterPolicy.h"
#include <iostream>
int main()
{
    unsigned checks=0;RasterPolicy::Layout out;
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"FAIL "<<#x<<'\n';return 1;}}while(0)
    CHECK(RasterPolicy::Select(512,out)&&out.pixels==262144&&out.bmpBytes==1048630);
    CHECK(RasterPolicy::Select(1024,out)&&out.targetBytes==50331648);
    CHECK(RasterPolicy::Select(2048,out)&&out.targetBytes==201326592&&out.readbackBytes==100663296&&out.bmpBytes==16777270);
    const RasterPolicy::Layout last=out;
    CHECK(!RasterPolicy::Select(0,out));
    CHECK(!RasterPolicy::Select(-1,out));
    CHECK(!RasterPolicy::Select(513,out));
    CHECK(!RasterPolicy::Select(4096,out));
    CHECK(!RasterPolicy::Select(8192,out));
    CHECK(!RasterPolicy::Select(2147483647,out));
    CHECK(out.side==last.side&&out.pixels==last.pixels&&out.targetBytes==last.targetBytes);
    std::cout<<checks<<" production bounded diagnostic raster checks passed\n";
}
