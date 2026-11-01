#include "../TextureArrayPolicy.h"
#include <cstdio>
#include <limits>
using namespace SatelliteTextureArray;
int main()
{
    Shape actualDiffuse={2048,2048,1,11,21,5592432},actualNormal={2048,2048,1,11,17,2796216};
    int failures=0,checks=0;
#define CHECK(expr) do{++checks;if(!(expr)){++failures;std::printf("FAIL line %d\n",__LINE__);}}while(0)
    CHECK(Accept(actualDiffuse));CHECK(Accept(actualNormal));CHECK(Match(actualDiffuse,actualDiffuse));
    CHECK(!Match(actualDiffuse,actualNormal));
    Shape invalid=actualDiffuse;invalid.width=8192;CHECK(!Accept(invalid));
    invalid=actualDiffuse;invalid.depth=6;CHECK(!Accept(invalid));
    invalid=actualDiffuse;invalid.mips=13;CHECK(!Accept(invalid));
    invalid=actualDiffuse;invalid.format=0;CHECK(!Accept(invalid));
    invalid=actualDiffuse;invalid.bytes=0;CHECK(!Accept(invalid));
    invalid=actualDiffuse;invalid.height=1024;CHECK(!Match(actualDiffuse,invalid));
    size_t total=0;for(unsigned i=0;i<6;++i){CHECK(AddBytes(total,actualDiffuse.bytes));CHECK(AddBytes(total,actualNormal.bytes));}
    CHECK(total==50331888);CHECK(!AddBytes(total,128u*1024u*1024u));CHECK(total==50331888);
    total=(std::numeric_limits<size_t>::max)();CHECK(!AddBytes(total,1));
    std::printf("%d production texture-array input checks, %d failures\n",checks,failures);return failures?1:0;
}
