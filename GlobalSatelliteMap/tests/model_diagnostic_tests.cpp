#define GSM_MODEL_DIAGNOSTIC_POLICY_ONLY
#include "../GsmModelDiagnostic.h"
#include <cstdio>
#include <stdexcept>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Ops {
    unsigned models,censuses,clears,held;HRESULT model,census;bool throwModel,throwCensus,changeModel,changeCensus;volatile LONG*live;
    Ops():models(0),censuses(0),clears(0),held(0),model(S_OK),census(S_OK),throwModel(false),throwCensus(false),changeModel(false),changeCensus(false),live(NULL){}
    HRESULT Model(GsmModelDiagnostic::Report&r){++models;++held;if(changeModel)++*live;if(throwModel)throw std::runtime_error("owned model failure");r.packets=2;r.modelValid=true;r.rawSceneWorld[3]=12;r.sourceSceneOrigin[0]=34;r.localBounds[5]=56;strcpy_s(r.modelError,"source message");return model;}
    void Clear(){++clears;held=0;}
    HRESULT Census(NativeLoadedCensus::Result&r){++censuses;C(!held,"packet released before census or log");r.selected=3;if(changeCensus)++*live;if(throwCensus)throw std::runtime_error("owned census failure");return census;}
};
int main(){try{
    GsmModelDiagnostic::Once once;Ops ops;volatile LONG live=7;ops.live=&live;bool fresh=true;
    C(once.CaptureWith(false,false,7,reinterpret_cast<volatile LONG*>(1),ops,&fresh)==S_FALSE&&!fresh&&!ops.models&&!ops.censuses,"OFF no source queries");
    C(once.CaptureWith(true,false,7,&live,ops,&fresh)==E_ACCESSDENIED&&!fresh&&!ops.models,"unverified no source queries");
    C(once.CaptureWith(true,true,7,&live,ops,&fresh)==S_OK&&fresh&&ops.models==1&&ops.censuses==1&&!ops.held,"one validated attempt captures both sources and retires packet");
    const GsmModelDiagnostic::Report&r=once.Values();C(r.generation==7&&r.packets==2&&r.modelValid&&!r.modelShadersVerified&&r.rawSceneWorld[3]==12&&r.sourceSceneOrigin[0]==34&&r.localBounds[5]==56&&r.loaded.selected==3,"copied scalar report preserves metadata without shader proof");
    C(once.CaptureWith(true,true,7,&live,ops,&fresh)==S_FALSE&&!fresh&&ops.models==1,"same generation no second native query");
    once.CaptureWith(false,true,7,&live,ops,&fresh);C(once.CaptureWith(true,true,7,&live,ops,&fresh)==S_FALSE&&ops.models==1,"OFF does not reset epoch latch");
    live=8;C(once.CaptureWith(true,true,8,&live,ops,&fresh)==S_OK&&fresh&&ops.models==2&&once.Values().generation==8,"new generation allows one new report");
    once.Reset();C(once.CaptureWith(true,true,8,&live,ops,&fresh)==S_OK&&fresh&&ops.models==3,"explicit reset allows deliberate retry");
    GsmModelDiagnostic::Once failure;Ops bad;bad.live=&live;bad.model=E_NOINTERFACE;
    C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==E_NOINTERFACE&&fresh&&bad.censuses==1&&!bad.held,"unsupported model HRESULT preserved and census still inspected");
    C(!failure.Values().modelValid&&!failure.Values().packets&&!failure.Values().rawSceneWorld[3]&&failure.Values().loaded.selected==3&&!std::strcmp(failure.Values().modelError,"source message"),"failed model metadata retired but source error retained");
    C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==S_FALSE&&!fresh&&bad.models==1,"failed source attempted only once per generation");
    failure.Reset();bad.throwModel=true;C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==E_FAIL&&bad.censuses==2&&!bad.held&&!std::strcmp(failure.Values().modelError,"owned model failure"),"model C++ exception contained and packet retired");
    failure.Reset();bad.throwModel=false;bad.model=S_OK;bad.census=E_INVALIDARG;
    C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==E_INVALIDARG&&failure.Values().modelValid&&!failure.Values().loaded.selected,"failed census code retained without partial census");
    failure.Reset();bad.throwCensus=true;C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==E_FAIL&&!failure.Values().loaded.selected,"census exception contained without partial output");
    failure.Reset();bad.throwCensus=false;bad.census=S_OK;bad.changeModel=true;unsigned previous=bad.censuses;
    C(failure.CaptureWith(true,true,8,&live,bad,&fresh)==E_ABORT&&fresh&&bad.censuses==previous&&!bad.held,"world change after model skips stale-town census");
    C(!failure.Values().modelValid&&!failure.Values().packets&&!failure.Values().loaded.selected&&failure.Values().modelHr==E_ABORT&&failure.Values().censusHr==E_ABORT,"world change retires all source metadata");
    failure.Reset();bad.changeModel=false;bad.changeCensus=true;
    C(failure.CaptureWith(true,true,live,&live,bad,&fresh)==E_ABORT&&!failure.Values().packets&&!failure.Values().loaded.selected,"world change during census retires entire report");
    failure.Reset();bad.changeCensus=false;previous=bad.models;
    C(failure.CaptureWith(true,true,0,&live,bad,&fresh)==E_INVALIDARG&&!fresh&&bad.models==previous,"zero generation no native attempt");
    C(failure.CaptureWith(true,true,live,NULL,bad,&fresh)==E_INVALIDARG&&bad.models==previous,"missing live-generation owner no native attempt");
    C(failure.CaptureWith(true,true,live-1,&live,bad,&fresh)==E_ABORT&&bad.models==previous,"stale generation no native attempt");
    char longMessage[512],shortMessage[256];std::memset(longMessage,'x',511);longMessage[511]=0;GsmModelDiagnostic::Text(shortMessage,longMessage);
    C(std::strlen(shortMessage)==255&&shortMessage[255]==0,"model source message bounded to255 bytes");
    C(sizeof(GsmModelDiagnostic::Report)<65536,"stored report bounded below64KiB with value-only census");
    std::printf("model diagnostic: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
