#pragma once
#include <Windows.h>
#include <cstring>
#include <exception>
#include "atlas-models/loaded-census/CensusRoute.h"
#ifndef GSM_MODEL_DIAGNOSTIC_POLICY_ONLY
#include "atlas-models/loaded-census/NativeCensus.h"
#endif
namespace GsmModelDiagnostic {
struct Report {
    LONG generation;HRESULT modelHr,censusHr;unsigned packets;bool modelValid,modelShadersVerified;
    float rawSceneWorld[16],sourceSceneOrigin[3],localBounds[6];char modelError[256];NativeLoadedCensus::Result loaded;
    Report():generation(0),modelHr(S_FALSE),censusHr(S_FALSE),packets(0),modelValid(false),modelShadersVerified(false){std::memset(rawSceneWorld,0,sizeof(rawSceneWorld));std::memset(sourceSceneOrigin,0,sizeof(sourceSceneOrigin));std::memset(localBounds,0,sizeof(localBounds));std::memset(modelError,0,sizeof(modelError));}
};
inline void Text(char out[256],const char*value){unsigned i=0;if(value)for(;i<255&&value[i];++i)out[i]=value[i];out[i]=0;}
inline bool Generation(volatile LONG*live,LONG expected){return live&&InterlockedCompareExchange(live,0,0)==expected;}
inline void EraseModel(Report&r){r.packets=0;r.modelValid=false;std::memset(r.rawSceneWorld,0,sizeof(r.rawSceneWorld));std::memset(r.sourceSceneOrigin,0,sizeof(r.sourceSceneOrigin));std::memset(r.localBounds,0,sizeof(r.localBounds));}
#ifndef GSM_MODEL_DIAGNOSTIC_POLICY_ONLY
class NativeOps {
    NativeModels::Snapshot snapshot_;Town*town_;Ogre::SceneManager*scene_;LONG generation_;volatile LONG*live_;bool verified_;
public:
    NativeOps(Town*town,Ogre::SceneManager*scene,LONG generation,volatile LONG*live,bool verified):town_(town),scene_(scene),generation_(generation),live_(live),verified_(verified){}
    HRESULT Model(Report&r){
        HRESULT hr=snapshot_.Read(true,town_,scene_,unsigned(generation_),verified_);Text(r.modelError,snapshot_.LastError());
        if(SUCCEEDED(hr)&&snapshot_.Valid(unsigned(generation_))){const AtlasModels::Packet&p=snapshot_.Geometry();r.packets=p.Count();r.modelValid=true;std::memcpy(r.rawSceneWorld,p.RawSceneWorld(),sizeof(r.rawSceneWorld));std::memcpy(r.sourceSceneOrigin,p.SourceSceneOrigin(),sizeof(r.sourceSceneOrigin));std::memcpy(r.localBounds,snapshot_.LocalMeshBounds(),sizeof(r.localBounds));}
        return hr;
    }
    void Clear(){snapshot_.Clear();}
    HRESULT Census(NativeLoadedCensus::Result&r){return NativeLoadedCensus::Read(true,town_,scene_,generation_,live_,verified_,r);}
};
#endif
class Once {
    LONG epoch_;bool attempted_;Report report_;
    HRESULT Admission(bool enabled,bool verified,LONG generation,volatile LONG*live)const{if(!enabled)return S_FALSE;if(!verified)return E_ACCESSDENIED;if(!generation||!live)return E_INVALIDARG;if(!Generation(live,generation))return E_ABORT;return epoch_==generation&&attempted_?S_FALSE:S_OK;}
    HRESULT Changed(LONG generation){report_=Report();report_.generation=generation;report_.modelHr=report_.censusHr=E_ABORT;Text(report_.modelError,"world generation changed during model diagnostic");return E_ABORT;}
public:
    Once():epoch_(0),attempted_(false){}
    void Reset(){epoch_=0;attempted_=false;report_=Report();}
    const Report&Values()const{return report_;}
    // OFF does not erase the one-attempt latch. Reset explicitly on world/session
    // retirement; a new validated generation also starts a fresh attempt.
    template<class Ops>HRESULT CaptureWith(bool enabled,bool verified,LONG generation,volatile LONG*live,Ops&ops,bool*newReport=NULL){
        if(newReport)*newReport=false;HRESULT admit=Admission(enabled,verified,generation,live);if(admit!=S_OK)return admit;
        epoch_=generation;attempted_=true;if(newReport)*newReport=true;Report next;next.generation=generation;
        try{next.modelHr=ops.Model(next);}catch(const std::exception&e){next.modelHr=E_FAIL;Text(next.modelError,e.what());}catch(...){next.modelHr=E_FAIL;Text(next.modelError,"model diagnostic exception");}
        ops.Clear(); // Retire all packet/state/COM ownership before census/logging.
        if(FAILED(next.modelHr))EraseModel(next);if(!Generation(live,generation))return Changed(generation);
        try{next.censusHr=ops.Census(next.loaded);}catch(...){next.censusHr=E_FAIL;}
        if(FAILED(next.censusHr))next.loaded=NativeLoadedCensus::Result();if(!Generation(live,generation))return Changed(generation);
        report_=next;return FAILED(report_.modelHr)?report_.modelHr:report_.censusHr;
    }
#ifndef GSM_MODEL_DIAGNOSTIC_POLICY_ONLY
    HRESULT Capture(bool enabled,Town*town,Ogre::SceneManager*scene,LONG generation,volatile LONG*live,bool verified,bool*newReport=NULL){
        if(newReport)*newReport=false;HRESULT admit=Admission(enabled,verified,generation,live);if(admit!=S_OK)return admit;if(!town||!scene)return E_INVALIDARG;
        NativeOps ops(town,scene,generation,live,verified);return CaptureWith(enabled,verified,generation,live,ops,newReport);
    }
#endif
};
}
