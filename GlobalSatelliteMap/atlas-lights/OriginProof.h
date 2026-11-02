#pragma once
// Optional one-shot GUI/render-thread scalar diagnostic. Never activates a town.
#include "NativeLocalLightSnapshot.h"
namespace NativeOriginProof {
#include "OriginProofProfile.h"
struct Values {
    float cameraDerived[3],sceneOrigin[3],sharedCamera[3],sharedWorldOffset[3];
    float townStored[3],proxyNodeLocal[3],proxyNodeDerived[3];
    bool townYWouldRefresh,valid;LONG generation;char error[192];
    Values(){std::memset(this,0,sizeof(*this));}
};
struct Header {const void*proxy;const void*entity;float town[3];};
inline bool HeaderRead(const void*town,Header&out){
    __try{
        if(!town)return false;const BYTE*t=static_cast<const BYTE*>(town);const BYTE*p=*reinterpret_cast<const BYTE*const*>(t+0x4f8);
        if(!p||*reinterpret_cast<const void*const*>(p)!=town||*reinterpret_cast<const unsigned*>(p+8)!=3)return false;
        const void*entity=*reinterpret_cast<const void*const*>(p+16);if(!entity)return false;
        out.proxy=p;out.entity=entity;std::memcpy(out.town,t+0x48,12);return _finite(out.town[0])&&_finite(out.town[1])&&_finite(out.town[2]);
    }__except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}
}
inline bool HasExistingProxy(const void*town){Header h;return HeaderRead(town,h);}
inline bool Current(const void*scene,const void*camera){__try{return au&&au->render&&au->render->scene==scene&&au->render->camera==camera;}__except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}}
inline bool Equal(const float*a,const float*b){return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2];}
inline bool Vector(FARPROC f,const void*object,float*out){__try{typedef void*(__fastcall*Get)(const void*,float*);if(reinterpret_cast<Get>(f)(object,out)!=out)return false;return _finite(out[0])&&_finite(out[1])&&_finite(out[2]);}__except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}}
inline bool Camera(FARPROC f,const void*object,float*out){__try{typedef const float*(__fastcall*Get)(const void*);const float*p=reinterpret_cast<Get>(f)(object);if(!p)return false;std::memcpy(out,p,12);return _finite(out[0])&&_finite(out[1])&&_finite(out[2]);}__except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}}
inline const void*Pointer(FARPROC f,const void*object){__try{typedef const void*(__fastcall*Get)(const void*);return reinterpret_cast<Get>(f)(object);}__except(NativeLocalLights::NativeFault(GetExceptionCode())){return NULL;}}
inline void SharedImpl(const char*set,const char*name,float*out){Ogre::Vector4 v=GsmNative::Shared(set,name,3);out[0]=v.x;out[1]=v.y;out[2]=v.z;}
inline bool Shared(const char*set,const char*name,float*out){__try{SharedImpl(set,name,out);return true;}__except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}}
class Reader {
    FARPROC parent_,position_,origin_,manager_,camera_,local_;bool initialized_;LONG epoch_;HMODULE engineModule_,ogreModule_;
    static HRESULT Fail(Values&out,HRESULT hr,const char*message){out=Values();size_t i=0;for(;i<sizeof(out.error)-1&&message&&message[i];++i)out.error[i]=message[i];out.error[i]=0;return hr;}
    static FARPROC Resolve(const NativeLocalLights::FunctionPin&p){FARPROC f=GsmNative::Export(p.name,p.rva);if(!NativeLocalLights::CodeEqual(f,p.bytes))throw std::runtime_error("origin proof native getter profile differs");return f;}
    void Initialize(LONG generation){
        if(initialized_&&epoch_==generation){if(engineModule_!=GetModuleHandleA(NULL)||ogreModule_!=GetModuleHandleA(GsmOgreFile))throw std::runtime_error("origin proof loaded module changed");return;}
        if(!GsmNative::Profile()||!GsmNative::Module(GetModuleHandleA(NULL),NativeLocalLights::EngineTimestamp,NativeLocalLights::EngineSize,NativeLocalLights::EngineWindows,sizeof(NativeLocalLights::EngineWindows)/sizeof(NativeLocalLights::EngineWindows[0]))||!GsmNative::Module(GetModuleHandleA(GsmOgreFile),GsmOgreTimestamp,GsmOgreSize,NativeLocalLights::OriginWindows,sizeof(NativeLocalLights::OriginWindows)/sizeof(NativeLocalLights::OriginWindows[0]))||!GsmNative::Module(GetModuleHandleA(NULL),NativeLocalLights::EngineTimestamp,NativeLocalLights::EngineSize,TownWindows,sizeof(TownWindows)/sizeof(TownWindows[0])))throw std::runtime_error("origin proof module profile differs");
        parent_=Resolve(NativeLocalLights::Functions[NativeLocalLights::FParent]);position_=Resolve(NativeLocalLights::Functions[NativeLocalLights::FPosition]);origin_=Resolve(NativeLocalLights::Functions[NativeLocalLights::FRelativeOrigin]);manager_=Resolve(NativeLocalLights::Functions[NativeLocalLights::FManager]);
        camera_=Resolve(CameraPin);local_=Resolve(LocalPin);engineModule_=GetModuleHandleA(NULL);ogreModule_=GetModuleHandleA(GsmOgreFile);epoch_=generation;initialized_=true;
    }
public:
    Reader():parent_(NULL),position_(NULL),origin_(NULL),manager_(NULL),camera_(NULL),local_(NULL),initialized_(false),epoch_(0),engineModule_(NULL),ogreModule_(NULL){}
    HRESULT Read(bool enabled,LONG generation,const void*scene,const void*camera,const void*town,bool sourceVerified,const volatile LONG*liveGeneration,Values&out){
        out=Values();if(!enabled)return S_FALSE;
        if(!sourceVerified)return Fail(out,E_ACCESSDENIED,"origin proof source profile not verified");
        if(!scene||!camera||!NativeLocalLights::Generation(liveGeneration,generation)||!Current(scene,camera))return Fail(out,E_ABORT,"origin proof current scene/camera/generation differs");
        try{
            Initialize(generation);Header first;if(!HeaderRead(town,first))return Fail(out,S_FALSE,"existing state3 native proxy unavailable");
            if(Pointer(manager_,first.entity)!=scene)return Fail(out,E_ABORT,"origin proof proxy belongs to another scene");const void*node=Pointer(parent_,first.entity);if(!node)return Fail(out,E_ABORT,"origin proof proxy node missing");
            Values v;v.generation=generation;std::memcpy(v.townStored,first.town,12);v.townYWouldRefresh=first.town[1]<=1;
            if(!Vector(origin_,scene,v.sceneOrigin)||!Camera(camera_,camera,v.cameraDerived)||!Vector(local_,node,v.proxyNodeLocal)||!Vector(position_,node,v.proxyNodeDerived))return Fail(out,E_FAIL,"origin proof scalar getter failed");
            if(!Shared("SharedDeferredParams","camera",v.sharedCamera)||!Shared("SharedWaterParams","worldOffset",v.sharedWorldOffset))return Fail(out,E_FAIL,"origin proof shared scalar getter failed");
            Header after;float o[3],c[3],local[3],derived[3],sc[3],sw[3];if(!Shared("SharedDeferredParams","camera",sc)||!Shared("SharedWaterParams","worldOffset",sw))return Fail(out,E_FAIL,"origin proof shared scalar reread failed");
            if(!HeaderRead(town,after)||first.proxy!=after.proxy||first.entity!=after.entity||!Equal(first.town,after.town)||Pointer(manager_,after.entity)!=scene||Pointer(parent_,after.entity)!=node||!Vector(origin_,scene,o)||!Camera(camera_,camera,c)||!Vector(local_,node,local)||!Vector(position_,node,derived)||!Equal(o,v.sceneOrigin)||!Equal(c,v.cameraDerived)||!Equal(local,v.proxyNodeLocal)||!Equal(derived,v.proxyNodeDerived)||!Equal(sc,v.sharedCamera)||!Equal(sw,v.sharedWorldOffset)||!Current(scene,camera)||!NativeLocalLights::Generation(liveGeneration,generation))return Fail(out,E_ABORT,"origin proof values changed during paired read");
            v.valid=true;out=v;return S_OK;
        }catch(const std::exception&e){return Fail(out,E_FAIL,e.what());}catch(...){return Fail(out,E_FAIL,"origin proof scalar read failed");}
    }
};
}
