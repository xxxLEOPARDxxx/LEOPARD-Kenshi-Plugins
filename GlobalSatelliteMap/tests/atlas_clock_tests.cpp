#ifndef VIEWER_CLOCK_HEADER
#define VIEWER_CLOCK_HEADER "../GsmViewerClock.h"
#endif
#include VIEWER_CLOCK_HEADER
#include <cstdio>
static unsigned checks=0;static void Check(bool ok,const char*s){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
    GsmViewer::TickClock ticks;
    Check(ticks.Due(0,200),"first tick at zero");Check(!ticks.Due(199,200)&&ticks.Due(200,200),"200ms cadence");
    ticks.Reset();Check(ticks.Due(0xf0000000,200),"first tick at high Windows uptime");
    ticks.Reset();Check(ticks.Due(0xffffff80,200),"first tick before wrap");Check(!ticks.Due(0x20,200),"wrap does not run early");Check(ticks.Due(0x48,200),"200ms across DWORD wrap");
    ticks.Reset();Check(ticks.Due(1,200)&&ticks.Due(0xf0000000,200),"tick after long hidden interval");
    GsmViewer::LightClock lights;Check(lights.Due(123,1000),"first light sample");Check(!lights.Due(1122,1000)&&lights.Due(1123,1000),"1000ms light cadence");Check(lights.Due(0xf0000000,1000),"light sample after long hidden interval");
    lights.Reset();Check(lights.Due(0xfffffff0,1000),"light gate resumes immediately after visibility reset");Check(!lights.Due(0x10,1000),"light wrap stays bounded");Check(lights.Due(0x3d8,1000),"1000ms light wrap");
    std::printf("GST2 production clocks PASS %u checks\n",checks);return 0;
}catch(...){return 1;}}
