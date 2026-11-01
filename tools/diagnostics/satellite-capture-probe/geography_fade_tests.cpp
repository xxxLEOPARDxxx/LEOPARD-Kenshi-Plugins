#include <map>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#ifndef GEOGRAPHY_FADE_HEADER
#define GEOGRAPHY_FADE_HEADER "GeographyPolicy.h"
#endif
#include GEOGRAPHY_FADE_HEADER
#include "GeographyFadeInputs.h"
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
int main()
{
    for(unsigned i=0;i<15;++i){float row[4]={};Check(SatelliteGeography::NativeFadeParameters(FadeFixtures[i].argb,FadeFixtures[i].distance,row),"native15-row fade conversion succeeds");Check(!std::memcmp(row,FadeFixtures[i].expected,sizeof(row)),"exact native ARGB channels/float reciprocal");}
    {float row[4];Check(SatelliteGeography::NativeFadeParameters(0xa0010203u,0,row)&&row[0]==1.0f/255&&row[1]==2*(1.0f/255)&&row[2]==3*(1.0f/255)&&row[3]==0,"ARGB alpha replaced by zero fade coefficient");Check(SatelliteGeography::NativeFadeParameters(0xffabcdefu,-12,row)&&row[3]==-12,"literal negative fade retained");}
    {float row[4]={1,2,3,4};Check(!SatelliteGeography::NativeFadeParameters(0, std::numeric_limits<float>::quiet_NaN(),row)&&row[0]==1&&row[3]==4,"nonfinite source refuses untouched");Check(!SatelliteGeography::NativeFadeParameters(0,1e-44f,row)&&row[0]==1&&row[3]==4,"derived reciprocal overflow refuses untouched");}
    std::map<std::string,int> integers;std::map<std::string,float> floats;float row[4]={1,2,3,4};
    Check(SatelliteGeography::FadeRow(false,integers,floats,row)&&row[0]==0&&row[3]==0,"disabled request needs no fields and zeros prior state");Check(!SatelliteGeography::FadeRow(true,integers,floats,row)&&integers.empty()&&floats.empty(),"missing enabled fields refuse without GameData insertion");
    integers["ground colour"]=int(FadeFixtures[0].argb);Check(!SatelliteGeography::FadeRow(true,integers,floats,row),"missing fade distance refuses");floats["fade distance"]=FadeFixtures[0].distance;
    for(unsigned enabled=0;enabled<3;++enabled){Check(SatelliteGeography::FadeRow(enabled==1,integers,floats,row),"off/on/off production row helper");Check(enabled==1?!std::memcmp(row,FadeFixtures[0].expected,sizeof(row)):(row[0]==0&&row[1]==0&&row[2]==0&&row[3]==0),"off/on/off cannot retain previous row7 values");}
    std::printf("PASS %u production native-fade field/conversion/off-on-off checks\n",checks);
}
