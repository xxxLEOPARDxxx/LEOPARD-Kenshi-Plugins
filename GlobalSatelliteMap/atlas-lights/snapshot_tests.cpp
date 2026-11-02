#define GSM_LOCAL_LIGHT_POLICY_ONLY
#ifndef LOCAL_LIGHT_HEADER
#define LOCAL_LIGHT_HEADER "NativeLocalLightSnapshot.h"
#endif
#include LOCAL_LIGHT_HEADER
#include <cstdio>
#include <cstdlib>
#include <new>
static bool trackAllocations=false;static unsigned allocations=0;
void*operator new(size_t n){if(trackAllocations)++allocations;void*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void*p){std::free(p);}void*operator new[](size_t n){return ::operator new(n);}void operator delete[](void*p){::operator delete(p);}
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
using namespace NativeLocalLights;
struct NativeClockFixture {
    void*memory;typedef float(__fastcall*Function)(const void*);Function function;
    NativeClockFixture(const char*path):memory(NULL),function(NULL){FILE*f=NULL;Check(!fopen_s(&f,path,"rb")&&f,"private native clock fixture open");BYTE bytes[168];Check(fread(bytes,1,sizeof(bytes),f)==sizeof(bytes)&&fgetc(f)==EOF,"private native clock fixture exact length");fclose(f);memory=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);Check(memory!=NULL,"private fixture allocation");std::memcpy(memory,bytes,sizeof(bytes));DWORD old;Check(VirtualProtect(memory,4096,PAGE_EXECUTE_READ,&old)!=FALSE&&FlushInstructionCache(GetCurrentProcess(),memory,sizeof(bytes)),"private fixture execute-only transition");function=reinterpret_cast<Function>(memory);}
    ~NativeClockFixture(){if(memory)VirtualFree(memory,0,MEM_RELEASE);}
    float Evaluate(float phase,float start,float end){unsigned __int64 words[0xc0/8]={0};BYTE*b=reinterpret_cast<BYTE*>(words);std::memcpy(b+0x84,&phase,4);std::memcpy(b+0xb0,&start,4);std::memcpy(b+0xb4,&end,4);return function(b);}
};
static LightValue Light(unsigned i=0){LightValue v={};v.range=100;v.basePower=4;v.worldPosition[0]=float(i);v.worldPosition[1]=12.5f;v.diffuse[0]=1.25f;v.diffuse[1]=.25f;v.diffuse[3]=1;v.specular[2]=.75f;v.attenuation[0]=1;v.attenuation[1]=.01f;v.attenuation[2]=.001f;v.custom0[1]=3;return v;}
struct FakeNative {
    RegistryState state;std::vector<LightValue>lights;std::vector<unsigned>visible;
    unsigned profiles,heavy,scenes,states,copies,reads;LONG epoch;bool profiled,failProfile,failState,failCopy,failRead,throwRead;unsigned change,changeScene,changeGenerationAt,rebaseAt;volatile LONG*generation;const void*scene;
    FakeNative():lights(Limit),visible(Limit,1),profiles(0),heavy(0),scenes(0),states(0),copies(0),reads(0),epoch(0),profiled(false),failProfile(false),failState(false),failCopy(false),failRead(false),throwRead(false),change(0),changeScene(0),changeGenerationAt(0),rebaseAt(0),generation(NULL),scene(reinterpret_cast<void*>(0x1234)){
        state.owner=reinterpret_cast<void*>(0x1000);state.data=reinterpret_cast<void*>(0x2000);state.clockOwner=reinterpret_cast<void*>(0x3000);state.count=3;state.enabled=1;state.gain=2;state.clock.phase=.5f;state.clock.start=.25f;state.clock.end=.75f;state.clock.time=2;state.sourceScene=scene;state.SourceSceneOrigin[0]=1024;state.SourceSceneOrigin[1]=12;state.SourceSceneOrigin[2]=-2048;for(unsigned i=0;i<Limit;++i)lights[i]=Light(i);
    }
    const char*Error()const{return "fixture native read failed";}
    bool Profile(LONG g){++profiles;if(!profiled||epoch!=g){++heavy;epoch=g;profiled=true;}return !failProfile;}
    const void*Scene(){++scenes;return changeScene&&scenes==changeScene?reinterpret_cast<void*>(0x9999):scene;}
    bool ReadState(RegistryState&out,const void*){++states;if(failState)return false;out=state;if(states%2==0){if(change==1)out.owner=reinterpret_cast<void*>(0x8888);if(change==2)out.data=reinterpret_cast<void*>(0x7777);if(change==3)++out.count;if(change==4)out.clockOwner=NULL;if(change==5)out.gain+=1;if(change==6)out.clock.time+=1;if(change>=7&&change<=9)out.SourceSceneOrigin[change-7]+=1;if(change==10)out.sourceScene=reinterpret_cast<void*>(0x7777);}return true;}
    bool CopyPointers(const RegistryState&s,const void**out){++copies;if(failCopy)return false;for(unsigned i=0;i<unsigned(s.count);++i)out[i]=reinterpret_cast<const void*>(ULONG_PTR(i+1));return true;}
    bool ReadLight(const void*p,const void*,LightValue&out,bool&v){++reads;if(throwRead)throw std::runtime_error("fixture C++ native exception");if(failRead)return false;unsigned i=unsigned(reinterpret_cast<ULONG_PTR>(p)-1);out=lights[i];v=visible[i]!=0;if(changeGenerationAt==reads&&generation)InterlockedIncrement(generation);if(rebaseAt==reads)state.SourceSceneOrigin[0]+=16384;return true;}
};
static void ClockTests(NativeClockFixture&fixture){
    ClockInput c={.5f,.25f,.75f,2.0};ClockValue v={};Check(Clock(c,v)&&v.daytime==1&&v.time==120,"current native clock values copied");
    for(unsigned i=0;i<4096;++i){c.phase=float(i)/4095;c.start=.24681357f;c.end=.81726354f;Check(Clock(c,v)&&!std::memcmp(&v.daytime,&(const float&)(fixture.Evaluate(c.phase,c.start,c.end)),4),"exact extracted native clock float ramp");}
    c.start=.25f;c.end=.75f;const float points[]={.25f,.25f+.0833333358168602f,.75f-.0833333358168602f,.75f,0,1};for(unsigned i=0;i<6;++i){c.phase=points[i];float actual=fixture.Evaluate(c.phase,c.start,c.end);Check(Clock(c,v)&&v.daytime==actual,"native clock exact transition boundaries");}
    c.phase=.5f;for(int i=-30;i<31;++i){c.time=double(i)*.013;Check(Clock(c,v),"native modulation time finite");float t=float(c.time*60),product=::sinf(t*30)*::sinf(t*17);Check(v.effect1==float(double(::sinf(t*4))*.5+.5)&&v.effect3==float(double(product)*.05+.95),"native sinf effect operation order");}
    unsigned nan=0x7fc00000;std::memcpy(&c.phase,&nan,4);Check(!Clock(c,v),"nonfinite native clock refused");c.phase=.5f;c.time=DBL_MAX;Check(!Clock(c,v),"overflowing native time refused");
}
static void CollectorTests(){
    Snapshot s;FakeNative n;volatile LONG generation=1;n.generation=&generation;
    Check(s.Read(n,false,1,NULL,false,NULL,0)==S_FALSE&&n.profiles==0&&n.states==0,"OFF makes zero native queries");
    Check(s.Read(n,true,1,n.scene,false,&generation,0)==E_ACCESSDENIED&&n.profiles==0&&!s.Valid(1),"source guard before native queries");
    Check(SUCCEEDED(s.Read(n,true,1,n.scene,true,&generation,0))&&s.Count()==3&&s.Valid(1),"actual production collector copies visible lights");Check(s.Light(0)->range==100&&s.Light(0)->diffuse[0]==1.25f&&s.Light(0)->custom0[1]==3&&s.Light(0)->effectivePower==8,"copied HDR colour floor and effective power preserved");
    Check(s.Info().SourceSceneOrigin[0]==1024&&s.Info().SourceSceneOrigin[1]==12&&s.Info().SourceSceneOrigin[2]==-2048&&s.Light(0)->worldPosition[0]==0,"coherent native origin published without transforming raw position");
    unsigned queries=n.reads;Check(s.Read(n,true,1,n.scene,true,&generation,999)==S_FALSE&&s.Valid(1)&&n.reads==queries,"1000ms gate retains copied snapshot without native queries");
    allocations=0;trackAllocations=true;HRESULT hr=s.Read(n,true,1,n.scene,true,&generation,1000);trackAllocations=false;Check(hr==S_OK&&allocations==0&&n.heavy==1,"warm collector reuses fixed storage and profile");
    n.state.count=Limit;Check(s.Read(n,true,1,n.scene,true,&generation,2000)==S_OK&&s.Count()==4096&&s.Light(4095)->worldPosition[0]==4095,"all4096 copied lights retained without32 total limit");
    n.state.count=4097;queries=n.copies;Check(s.Read(n,true,1,n.scene,true,&generation,3000)==HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW)&&s.Count()==0&&!s.Valid(1)&&n.copies==queries&&s.ObservedCount()==4097,"4097 overflow refuses before enumeration and clears output");
    Check(s.Read(n,true,1,n.scene,true,&generation,3001)==S_FALSE&&n.copies==queries,"failed acquisition is also rate limited");
    n.state.count=3;Check(s.Read(n,false,1,NULL,false,NULL,3002)==S_FALSE&&s.Count()==0,"disable retires copied values");Check(s.Read(n,true,1,n.scene,true,&generation,3002)==S_OK&&s.Count()==3,"reenable refreshes immediately");
    s.Reset();n.visible[0]=0;n.lights[1].basePower=0;n.lights[2].custom0[0]=1;Check(s.Read(n,true,1,n.scene,true,&generation,0)==S_OK&&s.Count()==0&&s.Info().hidden==1&&s.Info().zeroPower==1&&s.Info().daySuppressed==1,"actual visibility power and daytime suppression");
    n.visible[0]=1;n.lights[1].basePower=4;n.lights[2].custom0[0]=0;n.state.gain=0;Check(s.Read(n,true,1,n.scene,true,&generation,1000)==S_OK&&s.Count()==0&&s.Info().zeroEffect==3,"native zero effective power omitted");n.state.gain=2;
    n.lights[0].custom0[2]=1;n.lights[1].custom0[2]=3;Check(s.Read(n,true,1,n.scene,true,&generation,2000)==S_OK&&s.Light(0)->effectivePower==8*s.Info().clock.effect1&&s.Light(1)->effectivePower==8*s.Info().clock.effect3,"native effect1 and3 applied without invented phase");
    float frozen=s.Info().clock.time;Check(s.Read(n,true,1,n.scene,true,&generation,2000000)==S_OK&&s.Info().clock.time==frozen,"wall time never substitutes paused native modulation time");
    for(unsigned change=1;change<=10;++change){s.Reset();n.states=0;n.change=change;Check(FAILED(s.Read(n,true,1,n.scene,true,&generation,0))&&!s.Valid(1)&&s.Count()==0,"source identity change rejects entire snapshot");}n.change=0;
    s.Reset();n.scenes=0;n.changeScene=2;Check(FAILED(s.Read(n,true,1,n.scene,true,&generation,0))&&!s.Valid(1),"scene change rejects copied snapshot");n.changeScene=0;
    s.Reset();n.reads=0;n.changeGenerationAt=2;Check(FAILED(s.Read(n,true,1,n.scene,true,&generation,0))&&!s.Valid(1),"world change during copied-light traversal rejects output");n.changeGenerationAt=0;
    Check(s.Read(n,true,2,n.scene,true,&generation,1)==S_OK&&s.Valid(2)&&n.heavy==2,"new generation bypasses cadence and refreshes profile");
    s.Reset();n.failRead=true;Check(FAILED(s.Read(n,true,2,n.scene,true,&generation,0))&&!s.Valid(2),"native getter failure leaves no partial output");n.failRead=false;
    s.Reset();n.throwRead=true;Check(FAILED(s.Read(n,true,2,n.scene,true,&generation,0))&&std::strstr(s.LastError(),"fixture C++"),"native C++ failure contained in fixed error buffer");n.throwRead=false;
    s.Reset();n.state.enabled=0;queries=n.reads;Check(s.Read(n,true,2,n.scene,true,&generation,0)==S_OK&&s.Count()==0&&!s.Info().enabled&&n.reads==queries,"native local-light disable avoids object enumeration");n.state.enabled=1;
    s.Reset();Check(s.Read(n,true,2,n.scene,true,&generation,0x80000010)==S_OK,"first high-bit tick is initialized");queries=n.reads;Check(s.Read(n,true,2,n.scene,true,&generation,0x80000020)==S_FALSE&&n.reads==queries,"high-bit unsigned tick cadence");Check(s.Read(n,true,2,n.scene,true,&generation,0x10)==S_OK,"long unsigned idle refreshes");
    s.Reset();Check(s.Read(n,true,2,n.scene,true,&generation,0xfffffff0)==S_OK&&s.Read(n,true,2,n.scene,true,&generation,0x10)==S_FALSE&&s.Read(n,true,2,n.scene,true,&generation,0x400)==S_OK,"DWORD wrap cadence uses unsigned elapsed");s.Release();Check(!s.Valid(2)&&s.Count()==0&&!s.Light(0),"owned copied buffers release without native cleanup");
}
static void OriginCollectorTests(){
    Snapshot s;FakeNative n;volatile LONG generation=7;n.state.count=Limit;
    Check(s.Read(n,true,7,n.scene,true,&generation,0)==S_OK&&s.Count()==4096&&s.Info().SourceSceneOrigin[0]==1024,"4096 traversal captures initial native origin");
    n.reads=0;n.rebaseAt=2048;Check(FAILED(s.Read(n,true,7,n.scene,true,&generation,1000))&&n.reads==4096&&!s.Valid(7)&&s.Count()==0&&s.Info().SourceSceneOrigin[0]==0,"origin rebase during4096 traversal rejects copied payload");n.rebaseAt=0;
    Check(s.Read(n,true,7,n.scene,true,&generation,2000)==S_OK&&s.Info().SourceSceneOrigin[0]==17408&&s.Light(0)->worldPosition[0]==0,"new acquisition publishes new origin with untransformed raw values");
    unsigned reads=n.reads,states=n.states;n.state.SourceSceneOrigin[0]+=321;Check(s.Read(n,true,7,n.scene,true,&generation,2001)==S_FALSE&&n.reads==reads&&n.states==states&&s.Info().SourceSceneOrigin[0]==17408,"cached snapshot keeps its original origin tuple without new queries");
    Check(s.Read(n,true,7,n.scene,true,&generation,3000)==S_OK&&s.Info().SourceSceneOrigin[0]==17729,"due acquisition refreshes origin tuple coherently");
    unsigned nanBits=0x7fc00000;float nan;std::memcpy(&nan,&nanBits,4);for(unsigned axis=0;axis<3;++axis){s.Reset();float old=n.state.SourceSceneOrigin[axis];n.state.SourceSceneOrigin[axis]=nan;unsigned copies=n.copies;Check(FAILED(s.Read(n,true,7,n.scene,true,&generation,0))&&n.copies==copies&&s.Count()==0,"nonfinite copied origin rejects before traversal");n.state.SourceSceneOrigin[axis]=old;}
    s.Reset();n.state.sourceScene=reinterpret_cast<const void*>(0x999);unsigned copies=n.copies;Check(FAILED(s.Read(n,true,7,n.scene,true,&generation,0))&&n.copies==copies,"copied origin scene identity must match expected scene");
}
int main(int argc,char**argv){if(argc!=2)return 2;try{NativeClockFixture fixture(argv[1]);ClockTests(fixture);CollectorTests();OriginCollectorTests();std::printf("Native local-light snapshot PASS %u checks; actual clock instructions + production collector\n",checks);return 0;}catch(...){trackAllocations=false;return 1;}}
