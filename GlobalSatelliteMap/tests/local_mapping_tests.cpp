#include <float.h>
#include <cstdio>
#include <cstring>
#include "../GsmLocalMapping.h"
struct Proof { bool valid; long generation; float townStored[3],proxyNodeLocal[3],proxyNodeDerived[3],sceneOrigin[3],cameraDerived[3],sharedCamera[3],sharedWorldOffset[3]; };
struct Mapping { bool verified; long generation; float sourceSceneOrigin[3],sceneToCacheTranslation[3]; Mapping(){std::memset(this,0,sizeof(*this));} };
unsigned checks=0,bad=0;
#define CHECK(x) do{++checks;if(!(x)){++bad;std::printf("FAIL line %u: %s\n",__LINE__,#x);}}while(0)
Proof fixture(){Proof p={};p.valid=true;p.generation=7;float origin[3]={-51296.75f,0,3071.8125f};for(unsigned i=0;i<3;++i){p.sceneOrigin[i]=p.sharedWorldOffset[i]=origin[i];p.proxyNodeLocal[i]=p.townStored[i]=float(1200+i*500);p.proxyNodeDerived[i]=p.proxyNodeLocal[i]-origin[i];p.sharedCamera[i]=p.cameraDerived[i]=float(300+i*900);}return p;}
int main(){Proof p=fixture();Mapping m;CHECK(GsmLocalMapping::Adopt(p,7,m));CHECK(m.verified&&m.generation==7);for(unsigned i=0;i<3;++i)CHECK(m.sourceSceneOrigin[i]==p.sceneOrigin[i]&&m.sceneToCacheTranslation[i]==p.sceneOrigin[i]);
 {Proof live={};live.valid=true;live.generation=7;float town[3]={-62461,328.779f,16346.8f},derived[3]={-11164.3f,328.779f,13275},origin[3]={-51296.7f,0,3071.81f},camera[3]={302.266f,3349.96f,772.347f};for(unsigned i=0;i<3;++i){live.townStored[i]=live.proxyNodeLocal[i]=town[i];live.proxyNodeDerived[i]=derived[i];live.sceneOrigin[i]=live.sharedWorldOffset[i]=origin[i];live.cameraDerived[i]=live.sharedCamera[i]=camera[i];}live.sharedCamera[0]=302.267f;CHECK(GsmLocalMapping::Adopt(live,7,m));} // Actual paused scene, rounded diagnostic precision.
 CHECK(!GsmLocalMapping::Adopt(p,8,m)&&!m.verified);
 p=fixture();p.valid=false;CHECK(!GsmLocalMapping::Adopt(p,7,m));
 p=fixture();p.townStored[1]=0;CHECK(GsmLocalMapping::Adopt(p,7,m)); // Stored Y may still be lazy; XZ is authoritative.
 for(unsigned i=0;i<3;++i){p=fixture();p.proxyNodeDerived[i]+=1;CHECK(!GsmLocalMapping::Adopt(p,7,m)&&!m.verified);p=fixture();p.sharedWorldOffset[i]+=1;CHECK(!GsmLocalMapping::Adopt(p,7,m));p=fixture();p.sharedCamera[i]+=1;CHECK(!GsmLocalMapping::Adopt(p,7,m));}
 for(unsigned i=0;i<3;i+=2){p=fixture();p.townStored[i]+=1;CHECK(!GsmLocalMapping::Adopt(p,7,m));}
 p=fixture();p.proxyNodeDerived[0]+=.125f;CHECK(GsmLocalMapping::Adopt(p,7,m));p.proxyNodeDerived[0]+=.0625f;CHECK(!GsmLocalMapping::Adopt(p,7,m));
 unsigned nan=0x7fc00000;for(unsigned i=0;i<3;++i){p=fixture();std::memcpy(p.sceneOrigin+i,&nan,4);CHECK(!GsmLocalMapping::Adopt(p,7,m));}
 std::printf("%u mapping checks, %u failed\n",checks,bad);return bad?1:0;}
