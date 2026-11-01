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
    CHECK(RasterPolicy::Select(4096,out,true)&&out.detail==16&&out.resolutionOnly&&out.targetBytes==805306368ULL&&out.readbackBytes==402653184ULL);
    CHECK(RasterPolicy::Select(8192,out,true)&&out.detail==32&&out.resolutionOnly&&out.targetBytes==3221225472ULL&&out.readbackBytes==1610612736ULL&&out.bmpBytes==268435510ULL);
    CHECK(!RasterPolicy::Select(16384,out,true));
    CHECK(RasterPolicy::Select(2048,out,true)&&out.detail==8&&!out.resolutionOnly);
    CHECK(RasterPolicy::Select(8192,out,true));
    const unsigned __int64 need=out.targetBytes+out.readbackBytes+3ULL*1024*1024*1024;
    CHECK(RasterPolicy::HostFits(out,need,need));
    CHECK(!RasterPolicy::HostFits(out,need-1,need));
    CHECK(!RasterPolicy::HostFits(out,need,need-1));
    CHECK(RasterPolicy::ViewportMatches(out,8192));
    CHECK(!RasterPolicy::ViewportMatches(out,2048));
    CHECK(!RasterPolicy::ViewportMatches(out,0));
    CHECK(RasterPolicy::Select(512,out)&&out.detail==8&&!out.resolutionOnly);
    std::cout<<checks<<" production bounded diagnostic raster checks passed\n";
}
