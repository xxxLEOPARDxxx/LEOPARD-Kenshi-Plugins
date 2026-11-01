#include "NativeTerrainCache.h"
#include <iostream>
#include <cstdlib>
unsigned checks=0;
void Check(bool ok,const char *message)
{
    ++checks;if(!ok){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}
}
int main()
{
    using namespace NativeTerrainCache;
    unsigned char terrain[0x1a0],original[sizeof(terrain)];
    std::memset(terrain,0xa5,sizeof(terrain));
    const void *primary=reinterpret_cast<const void*>(0x11111110),
               *listener=reinterpret_cast<const void*>(0x22222220),
               *camera=reinterpret_cast<const void*>(0x33333330);
    float position[3]={1,2,3};
    std::memcpy(terrain,&primary,8);std::memcpy(terrain+0x170,&listener,8);
    std::memcpy(terrain+0x178,&camera,8);std::memcpy(terrain+0x180,position,12);
    std::memcpy(original,terrain,sizeof(terrain));
    Check(Matches(terrain,primary,listener),"exact terrain vtable tags");
    Check(!Matches(terrain,listener,primary),"wrong terrain type refused");
    State state;
    Check(Read(terrain,state),"terrain cache readable");
    Check(state.camera==camera,"terrain cache must not read FrameListener vptr");
    Check(state.position[0]==1&&state.position[1]==2&&state.position[2]==3,"all three native coordinates read");
    State changed=state;changed.camera=reinterpret_cast<const void*>(0x44444440);
    changed.position[0]=4;changed.position[1]=5;changed.position[2]=6;
    Check(Write(terrain,changed),"cache write accepted");
    Check(std::memcmp(terrain+0x170,&listener,8)==0,"FrameListener vptr never changed");
    Check(std::memcmp(terrain+0x178,&changed.camera,8)==0,"camera pointer at native178");
    Check(std::memcmp(terrain+0x180,changed.position,12)==0,"all coordinates at native180");
    Check(Write(terrain,state)&&std::memcmp(terrain,original,sizeof(terrain))==0,"restore exact bytes including surrounding sentinels");
    Check(!Read(NULL,state)&&!Write(NULL,state),"null cache refused");
    Check(!Matches(NULL,primary,listener),"null terrain type refused");
    std::cout<<checks<<" production native-terrain ABI checks passed\n";
}
