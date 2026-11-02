#pragma once
#include <cmath>
#include <float.h>
// Admit only a coherent paired scalar read of an existing native town proxy.
// Never infer a translation solely from camera/shared-light values.
namespace GsmLocalMapping {
inline bool Near(double a,double b){return _finite(a)&&_finite(b)&&std::fabs(a-b)<=.125;}
template<class Proof,class Mapping>bool Adopt(const Proof&p,long generation,Mapping&out){
    out=Mapping();if(!p.valid||p.generation!=generation)return false;
    for(unsigned i=0;i<3;++i){
        if(!Near(p.proxyNodeDerived[i]+double(p.sceneOrigin[i]),p.proxyNodeLocal[i])||
           !Near(p.sceneOrigin[i],p.sharedWorldOffset[i])||
           !Near(p.cameraDerived[i],p.sharedCamera[i]))return false;
        if(i!=1&&!Near(p.proxyNodeLocal[i],p.townStored[i]))return false;
    }
    for(unsigned i=0;i<3;++i)out.sourceSceneOrigin[i]=out.sceneToCacheTranslation[i]=p.sceneOrigin[i];
    out.generation=generation;out.verified=true;return true;
}
}
