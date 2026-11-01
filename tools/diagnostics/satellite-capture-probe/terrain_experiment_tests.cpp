#include "TerrainExperimentPolicy.h"
#include <iostream>
#include <cstdlib>
unsigned checks=0;
void Check(bool value,const char *name){++checks;if(!value){std::cerr<<name<<'\n';std::exit(1);}}
int main()
{
    using namespace TerrainExperimentPolicy;
    Region region;
    Check(PrimaryCell(0,0,12000,region)&&region.x==-110592&&region.z==-110592&&region.span==73728,"northwest primary cell");
    Check(PrimaryCell(3,3,16000,region)&&region.x==110592&&region.z==110592,"southeast primary cell");
    Check(!PrimaryCell(-1,0,12000,region)&&!PrimaryCell(0,4,12000,region),"invalid cell refused");
    Check(!PrimaryCell(1,1,9999,region)&&!PrimaryCell(1,1,30001,region),"altitude bounds");
    double previous=-147456;
    for(int x=0;x<4;++x){PrimaryCell(x,0,12000,region);Check(region.x-region.span*.5==previous,"exact half-open neighbor boundary");previous=region.x+region.span*.5;}
    Check(previous==147456,"exact world coverage");
    Check(region.span/8192==9,"nine world units per final pixel");
    Warmup warm;
    Stats s={1024,10,3,100,3,2};
    Check(warm.Observe(100,100,s)==Waiting,"unbuilt waits");
    s.unbuilt=0;
    Check(warm.Observe(100,200,s)==Waiting,"one stable sample insufficient");
    Check(warm.Observe(100,300,s)==Waiting,"two stable samples insufficient");
    Check(warm.Observe(100,400,s)==Ready,"three stable ready samples");
    s.triangles=101;Check(warm.Observe(100,500,s)==Waiting,"geometry changes reset readiness");
    s.visible=0;Check(warm.Observe(100,600,s)==Waiting,"invisible geometry is not ready");
    s.visible=3;s.unbuilt=1;Check(warm.Observe(100,60100,s)==Expired,"deadline refuses incomplete output");
    Warmup frames;for(unsigned i=0;i<599;++i)frames.Observe(0,i,s);
    Check(frames.Observe(0,599,s)==Expired,"frame deadline");
    Warmup memory;s.memory=1024ULL*1024*1024+1;
    Check(memory.Observe(0,1,s)==Exceeded,"native memory budget");
    Warmup wrap;s.memory=1024;s.unbuilt=1;
    Check(wrap.Observe(0xffffff80UL,0x80UL,s)==Waiting,"deadline clock wrap");
    Check(!CanDestroy(NULL,(void*)1)&&!CanDestroy((void*)1,(void*)2)&&CanDestroy((void*)1,(void*)1),"owner identity required");
    std::cout<<checks<<" production isolated-terrain policy checks passed\n";
}
