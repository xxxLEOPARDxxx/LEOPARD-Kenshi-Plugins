#include "../SavePreviewCodec.h"
#include <cstdio>
#include <fstream>
#include <iterator>
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char** argv)
{
    using namespace SavePreviewCodec;
    Header h={};memcpy(h.magic,"UITPREV1",8);h.version=1;h.width=32;h.height=16;h.encodedBytes=500;
    CHECK(Valid(h,sizeof(h)+500));h.width=0;CHECK(!Valid(h,sizeof(h)+500));h.width=32;
    h.encodedBytes=0xffffffffU;CHECK(!Valid(h,sizeof(h)+500));h.encodedBytes=500;
    CHECK(!Valid(h,sizeof(h)+499));Header b=h;CHECK(SameRevision(h,b));b.timeLow++;CHECK(!SameRevision(h,b));
    std::vector<unsigned char> source(32*16*3),encoded,decoded;
    for(unsigned i=0;i<source.size();i+=3){source[i]=10;source[i+1]=75;source[i+2]=200;}
    CHECK(Encode(source,32,16,encoded));CHECK(encoded.size()<SavePreviewPolicy::MaxEncodedBytes);
    CHECK(encoded.size()>8&&memcmp(&encoded[0],"\x89PNG\r\n\x1a\n",8)==0);
    CHECK(Decode(&encoded[0],(unsigned)encoded.size(),32,16,decoded));CHECK(decoded.size()==32*16*4);
    CHECK(decoded[0]==200&&decoded[1]==75&&decoded[2]==10&&decoded[3]==255);
    CHECK(!Decode(&encoded[0],(unsigned)encoded.size(),64,16,decoded));
    encoded[0]=0;CHECK(!Decode(&encoded[0],(unsigned)encoded.size(),32,16,decoded));
    CHECK(!Encode(source,1601,16,encoded));
    CHECK(argc==2);std::ifstream old(argv[1],std::ios::binary);std::vector<unsigned char> legacy((std::istreambuf_iterator<char>(old)),std::istreambuf_iterator<char>());
    CHECK(!legacy.empty()&&Decode(&legacy[0],(unsigned)legacy.size(),32,16,decoded));
    std::vector<unsigned char> bgra(4*4*4),filtered;
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){unsigned at=(y*4+x)*4;unsigned char v=((x+y)&1)?255:0;bgra[at]=bgra[at+1]=bgra[at+2]=v;bgra[at+3]=255;}
    CHECK(ResizeBgra(bgra,4,4,2,2,filtered)&&filtered.size()==12);
    for(unsigned i=0;i<filtered.size();++i)CHECK(filtered[i]>=126&&filtered[i]<=129); // point-picking aliases this checkerboard to black
    for(unsigned i=0;i<16;++i){bgra[i*4]=10;bgra[i*4+1]=75;bgra[i*4+2]=200;bgra[i*4+3]=0;}
    CHECK(ResizeBgra(bgra,4,4,2,2,filtered)&&filtered[0]==10&&filtered[1]==75&&filtered[2]==200); // backbuffer alpha does not blacken RGB
    CHECK(!ResizeBgra(bgra,4,4,5,4,filtered));
    CHECK(!ResizeBgra(bgra,3841,2160,1600,900,filtered));
    std::vector<unsigned char> noisy(1600*900*3);unsigned random=1;
    for(size_t i=0;i<noisy.size();++i){random=random*1664525U+1013904223U;noisy[i]=(unsigned char)(random>>24);}
    CHECK(Encode(noisy,1600,900,encoded)&&encoded.size()<=SavePreviewPolicy::MaxEncodedBytes);
    CHECK(Decode(&encoded[0],(unsigned)encoded.size(),1600,900,decoded)&&decoded.size()==1600*900*4);
    for(unsigned i=0;i<1600*900;i+=7919)CHECK(decoded[i*4]==noisy[i*3+2]&&decoded[i*4+1]==noisy[i*3+1]&&decoded[i*4+2]==noisy[i*3]);
    std::vector<unsigned char> full(3840*2160*4,255);LARGE_INTEGER frequency={},start={},end={};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    CHECK(ResizeBgra(full,3840,2160,1600,900,filtered)&&filtered.size()==1600*900*3);QueryPerformanceCounter(&end);
    CHECK(filtered[0]==255&&filtered[filtered.size()-1]==255);
    std::printf("WIC 3840x2160 -> 1600x900 one-shot filter: %.3f ms (isolated CPU fixture, not game FPS)\n",1000.0*(end.QuadPart-start.QuadPart)/frequency.QuadPart);
    CHECK(Wide(Utf8(L"\x0421\x0435\x0439\x0432\x4FDD\x5B58"))==L"\x0421\x0435\x0439\x0432\x4FDD\x5B58");
    std::printf("PASS save preview WIC codec: %u checks\n",checks);return 0;
}
