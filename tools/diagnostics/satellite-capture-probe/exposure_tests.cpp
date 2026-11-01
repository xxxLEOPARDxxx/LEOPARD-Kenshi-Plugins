#include "ExposurePolicy.h"
#include <iostream>
#include <limits>
#include <cmath>
int main()
{
    unsigned checks=0;
    float lum=0;
#define CHECK(x) do { ++checks;if(!(x)){std::cerr<<"FAIL "<<#x<<'\n';return 1;} } while(0)
    CHECK(ExposurePolicy::Decode(.55f,0,lum)&&lum==1);
    CHECK(ExposurePolicy::Decode(.55f,float(std::log(.9)),lum)&&std::fabs(lum-.9f)<1e-6f);
    CHECK(!ExposurePolicy::Decode(0,0,lum));
    CHECK(!ExposurePolicy::Decode(-1,0,lum));
    CHECK(!ExposurePolicy::Decode(std::numeric_limits<float>::infinity(),0,lum));
    CHECK(!ExposurePolicy::Decode(.55f,std::numeric_limits<float>::quiet_NaN(),lum));
    CHECK(!ExposurePolicy::Decode(.55f,1000,lum));
    CHECK(!ExposurePolicy::Decode(.55f,-1000,lum));
    CHECK(!ExposurePolicy::Decode(FLT_MAX,-80,lum));
    std::cout<<checks<<" production exposure validation checks passed\n";
}
