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
    Placement p1=PlacePreview(100,300,900,425,2560,1440,640,360,true);
    CHECK(p1.panel.width==900&&p1.panel.height==511); // fill dialog width, preserve 16:9
    CHECK(p1.image.width==888&&p1.image.height==499&&p1.panel.width==p1.image.width+12);
    CHECK(p1.dialogX==100&&p1.panel.x==100&&p1.panel.y==729);
    p1=PlacePreview(1700,950,680,325,1920,1080,640,360,true);
    CHECK(p1.panel.x+p1.panel.width<=1912&&p1.panel.y+p1.panel.height<=1072);
    CHECK(p1.panel.y==p1.dialogY+325+4);
    p1=PlacePreview(0,0,680,325,640,480,640,360,true);
    CHECK(p1.panel.width==624&&p1.panel.height==p1.image.height+12);
    CHECK(p1.image.x>=6&&p1.image.x+p1.image.width<=p1.panel.width-6);
    CHECK(p1.panel.y+p1.panel.height<=472&&p1.dialogY>=8);
    p1=PlacePreview(100,300,900,425,2560,1440,0,0,false);
    CHECK(p1.panel.width==900&&p1.panel.height==48); // old save message aligns with dialog
    p1=PlacePreview(100,300,900,425,2560,1440,360,640,true);
    CHECK(p1.panel.width==900&&p1.panel.height<=720); // portrait remains uncropped
    CHECK(p1.image.x==(p1.panel.width-p1.image.width)/2);
    std::printf("PASS save preview policy: %u checks\n",checks);return 0;
}
