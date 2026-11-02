#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <float.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <stdexcept>
namespace NativeLocalLights {
const unsigned Limit=4096;
struct ClockInput {float phase,start,end;double time;};
struct ClockValue {float daytime,time,effect1,effect3;};
inline bool Clock(const ClockInput&c,ClockValue&v){
    if(!_finite(c.phase)||!_finite(c.start)||!_finite(c.end)||!_finite(c.time))return false;
    const float e=.0833333358168602f;v.daytime=0;
    if(c.phase<=c.end&&c.phase>=c.start){
        const float up=c.start+e,down=c.end-e;
        if(c.phase<up)v.daytime=(c.phase-c.start)/(up-c.start);
        else if(c.phase>down)v.daytime=(c.phase-c.end)/(down-c.end);
        else v.daytime=1;
    }
    v.time=float(c.time*60.0);
    const float t4=v.time*4,t30=v.time*30,t17=v.time*17;
    if(!_finite(v.daytime)||!_finite(v.time)||!_finite(t4)||!_finite(t30)||!_finite(t17))return false;
    v.effect1=float(double(::sinf(t4))*.5+.5);
    const float product=::sinf(t30)*::sinf(t17);v.effect3=float(double(product)*.05+.95);
    return _finite(v.effect1)&&_finite(v.effect3);
}


struct LightValue {
    float worldPosition[3],range,worldDirection[3],basePower;
    float diffuse[4],specular[4],attenuation[3],custom0[4],spot[3];
    unsigned type;float effectivePower;
};
typedef char LightValueAbi[sizeof(LightValue)==112?1:-1];
inline bool LightValid(const LightValue&v){
    const float*f=reinterpret_cast<const float*>(&v);for(unsigned i=0;i<26;++i)if(!_finite(f[i]))return false;
    if(v.range<=0||v.basePower<=0||(v.type!=0&&v.type!=2))return false;
    if(v.type==2){const double length=double(v.worldDirection[0])*v.worldDirection[0]+double(v.worldDirection[1])*v.worldDirection[1]+double(v.worldDirection[2])*v.worldDirection[2];if(length<=1e-8||v.spot[0]<=v.spot[1]||v.spot[0]>1||v.spot[1]<-1||v.spot[2]<0)return false;}
    return true;
}
inline bool Power(const LightValue&v,float gain,const ClockValue&clock,float&power){
    if(!_finite(v.custom0[2])||v.custom0[2]<-2147483648.0f||v.custom0[2]>=2147483648.0f)return false;
    const int effect=static_cast<int>(v.custom0[2]);const float f=effect==1?clock.effect1:effect==3?clock.effect3:1;
    const float scaled=v.basePower*gain;power=scaled*f;return _finite(scaled)&&_finite(power);
}
struct RegistryState {
    const void*owner;const void*data;const void*clockOwner;unsigned __int64 count;
    const void*sourceScene;float SourceSceneOrigin[3];
    unsigned enabled;float gain;ClockInput clock;
    RegistryState(){std::memset(this,0,sizeof(*this));}
};
inline bool SameState(const RegistryState&a,const RegistryState&b){return a.owner==b.owner&&a.data==b.data&&a.clockOwner==b.clockOwner&&a.sourceScene==b.sourceScene&&a.SourceSceneOrigin[0]==b.SourceSceneOrigin[0]&&a.SourceSceneOrigin[1]==b.SourceSceneOrigin[1]&&a.SourceSceneOrigin[2]==b.SourceSceneOrigin[2]&&a.count==b.count&&a.enabled==b.enabled&&a.gain==b.gain&&a.clock.phase==b.clock.phase&&a.clock.start==b.clock.start&&a.clock.end==b.clock.end&&a.clock.time==b.clock.time;}
inline bool Generation(const volatile LONG*source,LONG expected){return source&&InterlockedCompareExchange(const_cast<volatile LONG*>(source),0,0)==expected;}
struct Information {
    unsigned registered,hidden,zeroPower,daySuppressed,zeroEffect,enabled;
    float rendererGain;ClockValue clock;float SourceSceneOrigin[3];
};
class Snapshot {
    std::vector<LightValue> values_,work_;unsigned count_;LONG generation_,attemptGeneration_;DWORD attemptedAt_;bool valid_,attempted_;Information information_;char error_[256];unsigned __int64 observedCount_;
    Snapshot(const Snapshot&);Snapshot&operator=(const Snapshot&);
    HRESULT Fail(HRESULT hr,const char*s){Clear();size_t i=0;if(s)for(;i<sizeof(error_)-1&&s[i];++i)error_[i]=s[i];error_[i]=0;return hr;}
public:
    Snapshot():count_(0),generation_(0),attemptGeneration_(0),attemptedAt_(0),valid_(false),attempted_(false),observedCount_(0){std::memset(&information_,0,sizeof(information_));error_[0]=0;}
    void Clear(){count_=0;valid_=false;generation_=0;std::memset(&information_,0,sizeof(information_));}
    void Reset(){Clear();attempted_=false;observedCount_=0;error_[0]=0;}
    void Release(){Reset();std::vector<LightValue>().swap(values_);std::vector<LightValue>().swap(work_);}
    bool Valid(LONG generation)const{return valid_&&generation_==generation;}unsigned Count()const{return valid_?count_:0;}
    const LightValue*Light(unsigned i)const{return valid_&&i<count_?&values_[i]:NULL;}
    const Information&Info()const{return information_;}const char*LastError()const{return error_;}unsigned __int64 ObservedCount()const{return observedCount_;}
    template<class Native>HRESULT Read(Native&native,bool enabled,LONG generation,const void*expectedScene,bool sourceVerified,const volatile LONG*liveGeneration,DWORD now){
        if(!enabled){Reset();return S_FALSE;}
        if(!sourceVerified)return Fail(E_ACCESSDENIED,"native local-light source profile is not verified");
        if(!expectedScene||!Generation(liveGeneration,generation))return Fail(E_INVALIDARG,"native local-light world/generation unavailable");
        if(attempted_&&attemptGeneration_==generation&&DWORD(now-attemptedAt_)<1000)return S_FALSE;
        if(!attempted_||attemptGeneration_!=generation)Clear();attempted_=true;attemptGeneration_=generation;attemptedAt_=now;error_[0]=0;
        try{
            if(!native.Profile(generation))return Fail(E_FAIL,native.Error());
            if(native.Scene()!=expectedScene)return Fail(E_PENDING,"native local-light scene differs");
            RegistryState state;if(!native.ReadState(state,expectedScene))return Fail(E_FAIL,native.Error());observedCount_=state.count;
            if(!state.owner||state.enabled>1||!_finite(state.gain)||state.gain<0)return Fail(E_FAIL,"native local-light registry header invalid");
            if(state.count>Limit)return Fail(HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW),"registered native light count exceeds4096");
            if(state.sourceScene!=expectedScene||!_finite(state.SourceSceneOrigin[0])||!_finite(state.SourceSceneOrigin[1])||!_finite(state.SourceSceneOrigin[2]))return Fail(E_FAIL,"native local-light source scene/origin invalid");
            Information info={};info.registered=unsigned(state.count);info.enabled=state.enabled;info.rendererGain=state.gain;
            std::memcpy(info.SourceSceneOrigin,state.SourceSceneOrigin,12);
            unsigned count=0;
            if(state.enabled){
                if(!state.clockOwner||!Clock(state.clock,info.clock))return Fail(E_FAIL,"native local-light modulation clock invalid");
                if(values_.size()!=Limit)values_.resize(Limit);if(work_.size()!=Limit)work_.resize(Limit);
                const void*pointers[Limit];
                if(!native.CopyPointers(state,pointers))return Fail(E_FAIL,native.Error());
                for(unsigned i=0;i<unsigned(state.count);++i){
                    if(!Generation(liveGeneration,generation))return Fail(E_ABORT,"native local-light world changed during snapshot");
                    LightValue value={};bool visible=false;
                    if(!pointers[i]||!native.ReadLight(pointers[i],expectedScene,value,visible))return Fail(E_FAIL,native.Error());
                    if(!visible){++info.hidden;continue;}
                    if(!_finite(value.basePower))return Fail(E_FAIL,"native local-light nonfinite power");
                    if(value.basePower<=0){++info.zeroPower;continue;}
                    if(!LightValid(value))return Fail(E_FAIL,"native local-light copied values invalid");
                    if(value.custom0[0]>0&&info.clock.daytime>.1f){++info.daySuppressed;continue;}
                    if(!Power(value,state.gain,info.clock,value.effectivePower))return Fail(E_FAIL,"native local-light effective power invalid");
                    if(value.effectivePower<=0){++info.zeroEffect;continue;}
                    work_[count++]=value;
                }
            }
            RegistryState after;
            if(!native.ReadState(after,expectedScene)||!SameState(state,after)||native.Scene()!=expectedScene||!Generation(liveGeneration,generation))return Fail(E_ABORT,"native local-light source changed during snapshot");
            values_.swap(work_);count_=count;generation_=generation;information_=info;valid_=true;return S_OK;
        }catch(const std::exception&e){return Fail(E_FAIL,e.what());}catch(...){return Fail(E_FAIL,"native local-light snapshot failed");}
    }
};
}

#ifndef GSM_LOCAL_LIGHT_POLICY_ONLY
#include <kenshi/Globals.h>
#include "../GsmNativeLight.h"
namespace NativeLocalLights {
struct FunctionPin {DWORD rva;const char*name;BYTE bytes[24];};
#include "NativeLocalLightProfile.h"
inline int NativeFault(DWORD code){return code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_IN_PAGE_ERROR||code==EXCEPTION_DATATYPE_MISALIGNMENT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}
inline bool CodeEqual(FARPROC p,const BYTE*bytes){__try{return p&&!std::memcmp(reinterpret_cast<const void*>(p),bytes,24);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
class NativeBackend {
    HMODULE engine_,ogre_;FARPROC functions_[FunctionCount];bool attempted_,ready_;LONG epoch_;const char*error_;
    bool SinImport(){__try{FARPROC expected=GetProcAddress(GetModuleHandleA("MSVCR100.dll"),"sinf");return expected&&*reinterpret_cast<FARPROC*>(reinterpret_cast<BYTE*>(engine_)+SinIat)==expected;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
    bool ValidateModules(){return GsmNative::Profile()&&GsmNative::Module(engine_,EngineTimestamp,EngineSize,EngineWindows,sizeof(EngineWindows)/sizeof(EngineWindows[0]))&&GsmNative::Module(ogre_,GsmOgreTimestamp,GsmOgreSize,OriginWindows,sizeof(OriginWindows)/sizeof(OriginWindows[0]))&&SinImport();}
    bool Resolve(){
        for(unsigned i=0;i<FunctionCount;++i){FARPROC p=GetProcAddress(ogre_,Functions[i].name);if(!p||reinterpret_cast<const BYTE*>(p)!=reinterpret_cast<const BYTE*>(ogre_)+Functions[i].rva||!CodeEqual(p,Functions[i].bytes))return false;functions_[i]=p;}
        return true;
    }
    bool LightType(const void*p)const{__try{return p&&*static_cast<const void*const*>(p)==reinterpret_cast<const BYTE*>(ogre_)+LightVtable;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
    bool ReadLightRaw(const void*p,const void*scene,LightValue&out,bool&visible){
        typedef const void*(__fastcall*Pointer)(const void*);typedef bool(__fastcall*Visible)(const void*);
        typedef float(__fastcall*Scalar)(const void*);typedef unsigned(__fastcall*Type)(const void*);
        typedef const float*(__fastcall*Reference)(const void*);typedef const float*(__fastcall*Custom)(const void*,unsigned short);
        typedef void*(__fastcall*Vector)(const void*,float*);typedef float(__fastcall*Cosine)(const float*,bool);
        visible=false;
        if(!LightType(p)){error_="registered native light concrete type differs";return false;}
        if(reinterpret_cast<Pointer>(functions_[FManager])(p)!=scene){error_="registered native light scene differs";return false;}
        visible=reinterpret_cast<Visible>(functions_[FVisible])(p);if(!visible)return true;
        out.basePower=reinterpret_cast<Scalar>(functions_[FPowerScale])(p);if(!_finite(out.basePower)){error_="native light power nonfinite";return false;}if(out.basePower<=0)return true;
        out.type=reinterpret_cast<Type>(functions_[FType])(p);if(out.type!=0&&out.type!=2){error_="registered native light type unsupported";return false;}
        const void*parent=reinterpret_cast<Pointer>(functions_[FParent])(p);if(!parent){error_="registered native light parent absent";return false;}
        if(reinterpret_cast<Vector>(functions_[FPosition])(parent,out.worldPosition)!=out.worldPosition){error_="native light position output ABI differs";return false;}
        std::memcpy(out.custom0,reinterpret_cast<Custom>(functions_[FCustom])(p,0),16);
        std::memcpy(out.diffuse,reinterpret_cast<Reference>(functions_[FDiffuse])(p),16);std::memcpy(out.specular,reinterpret_cast<Reference>(functions_[FSpecular])(p),16);
        out.range=reinterpret_cast<Scalar>(functions_[FRange])(p);out.attenuation[0]=reinterpret_cast<Scalar>(functions_[FConstant])(p);out.attenuation[1]=reinterpret_cast<Scalar>(functions_[FLinear])(p);out.attenuation[2]=reinterpret_cast<Scalar>(functions_[FQuadric])(p);
        if(out.type==2){
            if(reinterpret_cast<Vector>(functions_[FDirection])(p,out.worldDirection)!=out.worldDirection){error_="native light direction output ABI differs";return false;}
            const float inner=*reinterpret_cast<Reference>(functions_[FInner])(p),outer=*reinterpret_cast<Reference>(functions_[FOuter])(p);
            out.spot[2]=reinterpret_cast<Scalar>(functions_[FFalloff])(p);
            if(!_finite(inner)||!_finite(outer)||inner<0||inner>=outer){error_="native light spot angles invalid";return false;}
            const float halfInner=inner*.5f,halfOuter=outer*.5f;
            out.spot[0]=reinterpret_cast<Cosine>(functions_[FCos])(&halfInner,false);out.spot[1]=reinterpret_cast<Cosine>(functions_[FCos])(&halfOuter,false);
        }
        return true;
    }
public:
    NativeBackend():engine_(NULL),ogre_(NULL),attempted_(false),ready_(false),epoch_(0),error_("native local-light unavailable"){std::memset(functions_,0,sizeof(functions_));}
    const char*Error()const{return error_;}
    bool Profile(LONG generation){
        HMODULE engine=GetModuleHandleA(NULL),ogre=GetModuleHandleA("OgreMain_x64.dll");
        if(attempted_&&epoch_==generation){if(!ready_||engine!=engine_||ogre!=ogre_){error_="native local-light cached module profile unavailable";return false;}return true;}
        attempted_=true;ready_=false;epoch_=generation;engine_=engine;ogre_=ogre;
        if(!engine_||!ogre_||!ValidateModules()||!Resolve()){error_="native local-light module/export profile differs";return false;}
        ready_=true;return true;
    }
    const void*Scene(){__try{return au&&au->render?au->render->scene:NULL;}__except(NativeFault(GetExceptionCode())){error_="native local-light scene read fault";return NULL;}}
    bool ReadState(RegistryState&state,const void*expectedScene){
        __try{
            if(!expectedScene||Scene()!=expectedScene){error_="native local-light source scene differs";return false;}state.sourceScene=expectedScene;
            const BYTE*base=reinterpret_cast<const BYTE*>(engine_);const BYTE*owner=*reinterpret_cast<const BYTE*const*>(base+RegistryGlobal);
            if(!owner){error_="native local-light registry owner absent";return false;}
            state.owner=owner;state.data=*reinterpret_cast<const void*const*>(owner+0xb0);state.count=*reinterpret_cast<const unsigned __int64*>(owner+0xb8);state.enabled=owner[0x5c];state.gain=*reinterpret_cast<const float*>(owner+0x60);
            const BYTE*clock=*reinterpret_cast<const BYTE*const*>(base+ClockGlobal);state.clockOwner=clock;
            if(state.enabled&&clock&&state.count<=Limit){state.clock.phase=*reinterpret_cast<const float*>(clock+0x84);state.clock.start=*reinterpret_cast<const float*>(clock+0xb0);state.clock.end=*reinterpret_cast<const float*>(clock+0xb4);state.clock.time=*reinterpret_cast<const double*>(clock+0xa0);}
            if(state.count<=Limit){
                typedef void*(__fastcall*Vector)(const void*,float*);
                if(reinterpret_cast<Vector>(functions_[FRelativeOrigin])(expectedScene,state.SourceSceneOrigin)!=state.SourceSceneOrigin||!_finite(state.SourceSceneOrigin[0])||!_finite(state.SourceSceneOrigin[1])||!_finite(state.SourceSceneOrigin[2])||Scene()!=expectedScene){error_="native local-light source origin ABI/value/scene differs";return false;}
            }
            return true;
        }__except(NativeFault(GetExceptionCode())){error_="native local-light registry/clock read fault";return false;}
    }
    bool CopyPointers(const RegistryState&state,const void**out){
        __try{
            if(state.count>Limit||!out||(state.count&&(!state.data||reinterpret_cast<ULONG_PTR>(state.data)%sizeof(void*)||reinterpret_cast<ULONG_PTR>(state.data)>ULONG_PTR(-1)-state.count*sizeof(void*)))){error_="native local-light pointer span invalid";return false;}
            if(state.count)std::memcpy(out,state.data,size_t(state.count)*sizeof(void*));return true;
        }__except(NativeFault(GetExceptionCode())){error_="native local-light pointer span read fault";return false;}
    }
    bool ReadLight(const void*p,const void*scene,LightValue&out,bool&visible){
        __try{return ReadLightRaw(p,scene,out,visible);}__except(NativeFault(GetExceptionCode())){error_="native local-light getter read fault";return false;}
    }
};
class NativeSnapshot:public Snapshot {
    NativeBackend native_;
public:
    HRESULT Read(bool enabled,LONG generation,const void*scene,bool sourceVerified,const volatile LONG*liveGeneration,DWORD now){return Snapshot::Read(native_,enabled,generation,scene,sourceVerified,liveGeneration,now);}
};
}
#endif
