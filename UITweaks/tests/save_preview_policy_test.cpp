#include <cstdio>
#include <cstdlib>
#include "../SavePreviewPolicy.h"
static unsigned checks=0;
#define CHECK(x) do { ++checks; if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;} } while(0)
int main()
{
    using namespace SavePreviewPolicy;
    Size s=Fit(1920,1080,640,360);CHECK(s.width==640&&s.height==360);
    s=Fit(1080,1920,640,360);CHECK(s.width==202&&s.height==360);
    s=Fit(2560,1080,640,360);CHECK(s.width==640&&s.height==270);
    s=Fit(0,1,640,360);CHECK(!s.width&&!s.height);
    CHECK(AllowedReadback(3840,2160));CHECK(!AllowedReadback(7680,4320));
    CHECK(!AllowedReadback(0,2160));CHECK(!AllowedReadback(0xffffffffU,0xffffffffU));
    Pending p;CHECK(!p.active);p.Begin("C:/saves/a",17);CHECK(p.active&&!p.captured&&!p.complete);
    p.Complete(false);CHECK(!p.active); // failed copy never becomes a successful preview
    p.Begin("C:/saves/a",18);p.Complete(true);CHECK(p.active&&p.complete&&!p.Ready());
    p.Captured();CHECK(p.Ready());
    p.Begin("D:/saves/a",19);CHECK(p.path=="D:/saves/a"&&!p.captured&&!p.complete);
    p.Captured();CHECK(!p.Ready());p.Complete(true);CHECK(p.Ready());
    p.Reset();CHECK(!p.active&&p.path.empty()&&!p.Ready());
    CHECK(ValidDimensions(640,360));CHECK(!ValidDimensions(641,360));CHECK(!ValidDimensions(1,0));
    CHECK(ValidEncodedSize(1024));CHECK(!ValidEncodedSize(0));CHECK(!ValidEncodedSize(MaxEncodedBytes+1));
    Rect r=PlaceBelow(100,300,680,325,1920,1080,16.0/9.0);
    CHECK(r.x==100&&r.width==680&&r.y>=325&&r.y+r.height<=1072);
    r=PlaceBelow(1700,950,680,325,1920,1080,16.0/9.0);
    CHECK(r.x+r.width<=1912&&r.y+r.height<=1072&&r.height>0);
    r=PlaceBelow(0,0,680,325,640,480,16.0/9.0);
    CHECK(r.width<=624&&r.y+r.height<=472&&r.height>=0);
    std::printf("PASS save preview policy: %u checks\n",checks);return 0;
}
