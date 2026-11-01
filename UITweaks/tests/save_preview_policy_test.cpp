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
    CHECK(ValidDimensions(640,360));CHECK(ValidDimensions(1600,900));CHECK(!ValidDimensions(1601,900));CHECK(!ValidDimensions(1,0));
    CHECK(ValidEncodedSize(1024));CHECK(!ValidEncodedSize(0));CHECK(!ValidEncodedSize(MaxEncodedBytes+1));
    Placement p1=PlacePreview(100,300,900,425,2560,1440,640,360,true);
    CHECK(p1.panel.width==900&&p1.panel.height==511); // fill dialog width, preserve 16:9
    CHECK(p1.image.width==888&&p1.image.height==499&&p1.panel.width==p1.image.width+12);
    CHECK(p1.dialogX==100&&p1.dialogY==300&&p1.panel.x==100&&p1.panel.y==729);
    p1=PlacePreview(1700,950,680,325,1920,1080,640,360,true);
    CHECK(p1.dialogX==1700&&p1.dialogY==950&&!p1.panel.height); // no room: never move save window
    p1=PlacePreview(0,0,680,325,640,480,640,360,true);
    CHECK(p1.panel.width==624&&p1.panel.height==p1.image.height+12);
    CHECK(p1.image.x>=6&&p1.image.x+p1.image.width<=p1.panel.width-6);
    CHECK(p1.panel.y+p1.panel.height<=472&&p1.dialogY==0);
    p1=PlacePreview(100,300,900,425,2560,1440,0,0,false);
    CHECK(!p1.panel.width&&!p1.panel.height); // absent screenshot: no placeholder or frame
    p1=PlacePreview(100,300,900,425,2560,1440,360,640,true);
    CHECK(p1.panel.width==900&&p1.panel.height<=703); // portrait remains uncropped
    CHECK(p1.image.x==(p1.panel.width-p1.image.width)/2);
    p1=PlacePreview(100,700,900,425,1920,1440,1600,900,true);
    CHECK(p1.dialogX==100&&p1.dialogY==700&&p1.panel.y==1129&&p1.panel.height==303);
    CHECK(p1.panel.width==900&&p1.image.width==517&&p1.image.height==291);
    Point anchor=AnchorPreviewDialog(100,300,900,425,2560,1440);
    CHECK(anchor.x==100&&anchor.y==250); // reserve preview once before any selection
    Placement first=PlacePreview(anchor.x,anchor.y,900,425,2560,1440,1600,900,true);
    Placement next=PlacePreview(anchor.x,anchor.y,900,425,2560,1440,640,360,true);
    CHECK(first.dialogY==250&&next.dialogY==250&&first.panel.height==next.panel.height);
    CHECK(first.image.width==888&&first.image.height==499);
    anchor=AnchorPreviewDialog(100,50,900,425,2560,1440);CHECK(anchor.y==50); // already raised native window stays put
    const int views[][2]={{640,480},{1280,720},{1920,1080},{2560,1440},{3840,2160}};
    DialogHeightState height;
    CHECK(height.Next(325,false,120,680,1920,1080,true)==390);
    CHECK(height.Next(390,true,120,680,1920,1080,true)==390); // reopen does not compound 120%
    CHECK(height.Next(390,true,100,680,1920,1080,true)==325); // next opening applies changed setting against native base
    CHECK(height.Next(425,false,120,900,2560,1440,true)==510); // new weak root captures new native size
    CHECK(height.Next(510,true,160,900,2560,1440,true)==680);
    CHECK(height.Next(325,false,160,680,640,480,true)==325); // short viewport preserves preview room, no growth
    CHECK(height.Next(325,false,160,680,640,480,false)==464); // without previews only viewport cap applies
    CHECK(height.Next(400,true,120,680,1920,1080,true)==480); // external/native size change becomes the new base
    for(unsigned v=0;v<5;++v)
    {
        anchor=AnchorPreviewDialog(100,300,900,425,views[v][0],views[v][1]);
        p1=PlacePreview(anchor.x,anchor.y,900,425,views[v][0],views[v][1],1600,900,true);
        CHECK(p1.dialogX==anchor.x&&p1.dialogY==anchor.y);
        if(!p1.panel.height)continue;
        CHECK(p1.panel.x>=8&&p1.panel.x+p1.panel.width<=views[v][0]-8);
        CHECK(p1.panel.y==anchor.y+429&&p1.panel.y+p1.panel.height<=views[v][1]-8);
        CHECK(p1.image.x>=6&&p1.image.x+p1.image.width<=p1.panel.width-6);
        CHECK(abs(p1.image.width*900-p1.image.height*1600)<=1600); // at most one pixel of rounding, no crop
    }
    std::printf("PASS save preview policy: %u checks\n",checks);return 0;
}
