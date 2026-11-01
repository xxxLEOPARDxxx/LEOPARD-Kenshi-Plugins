#include "../SavePreviewCodec.h"
#include <cstdio>
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    using namespace SavePreviewCodec;
    Header h={};memcpy(h.magic,"UITPREV1",8);h.version=1;h.width=32;h.height=16;h.encodedBytes=500;
    CHECK(Valid(h,sizeof(h)+500));h.width=0;CHECK(!Valid(h,sizeof(h)+500));h.width=32;
    h.encodedBytes=0xffffffffU;CHECK(!Valid(h,sizeof(h)+500));h.encodedBytes=500;
    CHECK(!Valid(h,sizeof(h)+499));Header b=h;CHECK(SameRevision(h,b));b.timeLow++;CHECK(!SameRevision(h,b));
    std::vector<unsigned char> source(32*16*3),encoded,decoded;
    for(unsigned i=0;i<source.size();i+=3){source[i]=10;source[i+1]=75;source[i+2]=200;}
    CHECK(Encode(source,32,16,encoded));CHECK(encoded.size()<SavePreviewPolicy::MaxEncodedBytes);
    CHECK(Decode(&encoded[0],(unsigned)encoded.size(),32,16,decoded));CHECK(decoded.size()==32*16*4);
    CHECK(abs(int(decoded[0])-200)<5&&abs(int(decoded[1])-75)<5&&abs(int(decoded[2])-10)<5&&decoded[3]==255);
    CHECK(!Decode(&encoded[0],(unsigned)encoded.size(),64,16,decoded));
    encoded[0]=0;CHECK(!Decode(&encoded[0],(unsigned)encoded.size(),32,16,decoded));
    CHECK(!Encode(source,641,16,encoded));
    CHECK(Wide(Utf8(L"\x0421\x0435\x0439\x0432\x4FDD\x5B58"))==L"\x0421\x0435\x0439\x0432\x4FDD\x5B58");
    std::printf("PASS save preview WIC codec: %u checks\n",checks);return 0;
}
