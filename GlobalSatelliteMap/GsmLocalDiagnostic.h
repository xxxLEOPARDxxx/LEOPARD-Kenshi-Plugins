#pragma once
// Map-visible diagnostic bridge. The reader owns copied values only; no scene
// object is accessed after the synchronous Read call returns.
namespace GsmLocalDiagnostic {
template<class Reader>class Bridge {
    Reader reader_;const void*scene_;LONG generation_;bool failed_;
public:
    Bridge():scene_(NULL),generation_(0),failed_(false){}
    void Reset(){reader_.Reset();scene_=NULL;generation_=0;failed_=false;}
    const Reader&Values()const{return reader_;}
    HRESULT Tick(bool enabled,bool verified,LONG generation,const void*scene,
                 const volatile LONG*live,DWORD now){
        if(!enabled){Reset();return S_FALSE;}
        if(scene!=scene_||generation!=generation_){Reset();scene_=scene;generation_=generation;}
        if(failed_)return S_FALSE;
        HRESULT hr=reader_.Read(true,generation,scene,verified,live,now);
        if(FAILED(hr))failed_=true;
        return hr;
    }
};
}
