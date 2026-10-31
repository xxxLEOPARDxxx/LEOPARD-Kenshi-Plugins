#include "../GuardEvents.h"
#include <cstdio>
#include <ctime>
using namespace GatePushGuard;
int main() {
    Samples cache;Gate g;g.key.index=1;g.key.serial=2;g.closed=true;g.epoch=1;
    g.normal=Vec(0,0,1);g.halfWidth=5;g.minY=0;g.maxY=5;
    const clock_t start=clock();unsigned corrections=0,misses=0;
    double now=1;
    for(unsigned round=0;round<10000;++round) {
        now+=.05;
        for(unsigned actor=0;actor<128;++actor) {
            Observation o;o.key.index=actor;o.key.serial=1;o.seconds=now;o.position=Vec((float)(actor%5),0,-.2f);
            Sample* s=cache.Find(o.key,now);if(!s){++misses;continue;}
            Observe(*s,g,o);o.position.z=.2f;o.seconds+=.01;
            corrections+=Observe(*s,g,o).correct?1:0;
        }
    }
    std::printf("fixed cache bytes=%u; observations=2560000; corrections=%u; capacity misses=%u; CPU seconds=%.3f\n",
        (unsigned)sizeof(cache),corrections,misses,(double)(clock()-start)/CLOCKS_PER_SEC);
    static RegionIndex regions;static EventQueue events;static ActorEvent output[EventQueue::Capacity];
    regions.Set(0,true,Vec(),20);const clock_t eventStart=clock();unsigned drained=0,extraAccepted=0;
    for(unsigned round=0;round<1000;++round) {
        for(unsigned actor=0;actor<EventQueue::Capacity+1000;++actor) {
            ActorEvent e;e.key.index=actor;e.key.serial=1;e.seconds=round*.01;e.position=Vec((float)(actor%10),0,0);
            if(regions.Contains(e.position)&&events.Push(e)&&actor>=EventQueue::Capacity)++extraAccepted;
        }
        drained+=events.Drain(output);
    }
    std::printf("event queue bytes=%u; accepted=2048000; saturated attempts=1000000; drained=%u; extra accepted=%u; CPU seconds=%.3f\n",
        (unsigned)sizeof(events),drained,extraAccepted,(double)(clock()-eventStart)/CLOCKS_PER_SEC);
    return corrections==1280000&&misses==0&&drained==2048000&&extraAccepted==0?0:1;
}
