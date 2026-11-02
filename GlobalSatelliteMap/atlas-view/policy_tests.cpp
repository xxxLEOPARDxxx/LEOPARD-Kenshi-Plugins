#include <cstdio>
#include <cstring>
#include "AtlasPolicy.h"
using namespace AtlasView;
int main(){
    int failed=0;unsigned checks=0;
    #define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);++failed;}}while(0)
    CHECK(CellBytes(2)==80ULL*1024*1024);
    CHECK(CellBytes(4)*16==80ULL*1024*1024);
    CHECK(Fits(CellBytes(2),2048));
    CHECK(Fits(GpuCap-UniformGpuBytes-512ULL*512*4,2048));
    CHECK(!Fits(GpuCap-UniformGpuBytes-512ULL*512*4+1,2048));
    CHECK(Fits(CellBytes(4)*16,512));
    CHECK(!Fits(CellBytes(1),4096));
    Budget b;
    CHECK(b.Set(0,2));
    CHECK(b.Set(1,5));
    CHECK(!b.Set(2,3));
    b.Remove(0);
    b.Remove(1);
    for(unsigned i=0;i<16;++i)CHECK(b.Set(i,4));
    CHECK(b.Bytes()==80ULL*1024*1024&&b.ScratchSide()==512);
    CHECK(!b.Set(16,5));
    CHECK(!b.Set(0,1));
    std::printf("Atlas policy %s %u checks\n",failed?"FAIL":"PASS",checks);return failed?1:0;
}
