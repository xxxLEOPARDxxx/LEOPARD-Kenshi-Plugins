#define GSM_LOCAL_LIGHT_POLICY_ONLY
#ifndef LOCAL_LIGHT_HEADER
#define LOCAL_LIGHT_HEADER "NativeLocalLightSnapshot.h"
#endif
#include LOCAL_LIGHT_HEADER
#include <cstdio>
using namespace NativeLocalLights;
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
#include "origin_fixture.h"
namespace Fixture {
struct GsmCodeWindow {DWORD rva;BYTE bytes[24];};struct FunctionPin {DWORD rva;const char*name;BYTE bytes[24];};
#include "NativeLocalLightProfile.h"
#include "backend_fault.inc"
struct Render {const void*scene;};struct World {Render*render;};static World*au=NULL;
static HMODULE currentEngine=NULL,currentOgre=NULL;
static HMODULE GetModuleHandleA(const char*name){return name?currentOgre:currentEngine;}
static FARPROC expectedSin=reinterpret_cast<FARPROC>(ULONG_PTR(0x12345678));
static FARPROC FixtureGetProcAddress(HMODULE,const char*){return expectedSin;}
struct Node {float position[3];bool badOutput;};
struct Light {
    const void*vtable;const void*scene;Node*parent;unsigned type;bool visible,noCustom,badDirection,badColour;float power,range,direction[3],diffuse[4],specular[4],custom[4],attenuation[3],inner,outer,falloff;
};
static unsigned calls=0,cosCalls=0,typeCalls=0;
static const void*expectedNode=NULL;
static const void*expectedOriginScene=NULL;static OriginFixture*originFixture=NULL;static bool badOriginReturn=false,changeSceneDuringOrigin=false,rebaseDuringPosition=false;static unsigned originCalls=0;
static void*__fastcall RelativeOrigin(const void*scene,float*out){++originCalls;if(scene!=expectedOriginScene)return NULL;void*result=originFixture->function(scene,out);if(changeSceneDuringOrigin)au->render->scene=reinterpret_cast<const void*>(0x999);return badOriginReturn?NULL:result;}
static const void*__fastcall Manager(const void*p){++calls;return static_cast<const Light*>(p)->scene;}
static bool __fastcall Visible(const void*p){++calls;return static_cast<const Light*>(p)->visible;}
static float __fastcall PowerScale(const void*p){++calls;return static_cast<const Light*>(p)->power;}
static unsigned __fastcall Type(const void*p){++calls;++typeCalls;return static_cast<const Light*>(p)->type;}
static const void*__fastcall Parent(const void*p){++calls;return static_cast<const Light*>(p)->parent;}
static void*__fastcall Position(const void*p,float*out){++calls;if(p!=expectedNode)return NULL;const Node*n=static_cast<const Node*>(p);std::memcpy(out,n->position,12);if(rebaseDuringPosition)originFixture->Set(2048,12,-2048);return n->badOutput?NULL:out;}
static void*__fastcall Direction(const void*p,float*out){++calls;const Light*l=static_cast<const Light*>(p);std::memcpy(out,l->direction,12);return l->badDirection?NULL:out;}
static const float*__fastcall Custom(const void*p,unsigned short index){++calls;const Light*l=static_cast<const Light*>(p);if(index||l->noCustom)throw std::runtime_error("fixture missing native custom0");return l->custom;}
static const float*__fastcall Diffuse(const void*p){++calls;const Light*l=static_cast<const Light*>(p);return l->badColour?reinterpret_cast<const float*>(1):l->diffuse;}
static const float*__fastcall Specular(const void*p){++calls;return static_cast<const Light*>(p)->specular;}
static float __fastcall Range(const void*p){++calls;return static_cast<const Light*>(p)->range;}
static float __fastcall Constant(const void*p){++calls;return static_cast<const Light*>(p)->attenuation[0];}
static float __fastcall Linear(const void*p){++calls;return static_cast<const Light*>(p)->attenuation[1];}
static float __fastcall Quadric(const void*p){++calls;return static_cast<const Light*>(p)->attenuation[2];}
static const float*__fastcall Inner(const void*p){++calls;return &static_cast<const Light*>(p)->inner;}
static const float*__fastcall Outer(const void*p){++calls;return &static_cast<const Light*>(p)->outer;}
static float __fastcall Falloff(const void*p){++calls;return static_cast<const Light*>(p)->falloff;}
static float __fastcall Cos(const float*p,bool table){++calls;++cosCalls;if(table)throw std::runtime_error("native cosine table flag differs");return ::cosf(*p);}
class Backend {
public:
    HMODULE engine_,ogre_;FARPROC functions_[FunctionCount];bool attempted_,ready_;LONG epoch_;const char*error_;unsigned heavy,resolves;bool goodModules,goodResolve;
    Backend():engine_(NULL),ogre_(NULL),attempted_(false),ready_(false),epoch_(0),error_("fixture backend unavailable"),heavy(0),resolves(0),goodModules(true),goodResolve(true){std::memset(functions_,0,sizeof(functions_));}
    bool ValidateModules(){++heavy;return goodModules;}
    bool Resolve(){++resolves;functions_[FVisible]=reinterpret_cast<FARPROC>(Visible);functions_[FManager]=reinterpret_cast<FARPROC>(Manager);functions_[FPowerScale]=reinterpret_cast<FARPROC>(PowerScale);functions_[FCustom]=reinterpret_cast<FARPROC>(Custom);functions_[FParent]=reinterpret_cast<FARPROC>(Parent);functions_[FPosition]=reinterpret_cast<FARPROC>(Position);functions_[FDirection]=reinterpret_cast<FARPROC>(Direction);functions_[FDiffuse]=reinterpret_cast<FARPROC>(Diffuse);functions_[FSpecular]=reinterpret_cast<FARPROC>(Specular);functions_[FRange]=reinterpret_cast<FARPROC>(Range);functions_[FConstant]=reinterpret_cast<FARPROC>(Constant);functions_[FLinear]=reinterpret_cast<FARPROC>(Linear);functions_[FQuadric]=reinterpret_cast<FARPROC>(Quadric);functions_[FType]=reinterpret_cast<FARPROC>(Type);functions_[FInner]=reinterpret_cast<FARPROC>(Inner);functions_[FOuter]=reinterpret_cast<FARPROC>(Outer);functions_[FFalloff]=reinterpret_cast<FARPROC>(Falloff);functions_[FCos]=reinterpret_cast<FARPROC>(Cos);functions_[FRelativeOrigin]=reinterpret_cast<FARPROC>(RelativeOrigin);return goodResolve;}
    const char*Error()const{return error_;}
#define GetProcAddress FixtureGetProcAddress
#include "backend_sin_import.inc"
#undef GetProcAddress
#include "backend_type.inc"
#include "backend_light_raw.inc"
#include "backend_profile.inc"
#include "backend_scene.inc"
#include "backend_state.inc"
#include "backend_pointers.inc"
#include "backend_light.inc"
};
struct Pages {
    BYTE*base;Pages(SIZE_T bytes):base(static_cast<BYTE*>(VirtualAlloc(NULL,bytes,MEM_RESERVE,PAGE_NOACCESS))){Check(base!=NULL,"fixture virtual reserve");}
    ~Pages(){if(base)VirtualFree(base,0,MEM_RELEASE);}
    void Commit(unsigned at){Check(VirtualAlloc(base+(at&~4095U),4096,MEM_COMMIT,PAGE_READWRITE)!=NULL,"fixture isolated native-layout page");}
};
static void Pointer(BYTE*b,unsigned at,const void*p){std::memcpy(b+at,&p,8);}static void Float(BYTE*b,unsigned at,float v){std::memcpy(b+at,&v,4);}static void Double(BYTE*b,unsigned at,double v){std::memcpy(b+at,&v,8);}static void Count(BYTE*b,unsigned at,unsigned __int64 n){std::memcpy(b+at,&n,8);}
static void Init(Light&l,Node&node,const void*scene){std::memset(&l,0,sizeof(l));l.vtable=reinterpret_cast<BYTE*>(currentOgre)+LightVtable;l.scene=scene;l.parent=&node;l.visible=true;l.power=3;l.range=80;l.diffuse[0]=1.5f;l.diffuse[1]=.25f;l.diffuse[3]=1;l.specular[2]=.75f;l.custom[1]=4;l.attenuation[0]=1;l.attenuation[1]=.01f;l.attenuation[2]=.001f;l.inner=.4f;l.outer=1.2f;l.falloff=2;l.direction[1]=-1;}
static void Run(OriginFixture&fixture){
    Pages engine(0x232c000),ogre(0x9c9000);engine.Commit(RegistryGlobal);engine.Commit(ClockGlobal);engine.Commit(SinIat);ogre.Commit(LightVtable);currentEngine=reinterpret_cast<HMODULE>(engine.base);currentOgre=reinterpret_cast<HMODULE>(ogre.base);
    const void*scene=fixture.Scene();originFixture=&fixture;expectedOriginScene=scene;Render render={scene};World world={&render};au=&world;
    float origin[4]={0,0,0,999};Check(fixture.function(scene,origin)==origin&&origin[0]==1024&&origin[1]==12&&origin[2]==-2048&&origin[3]==999,"actual origin getter selects root0 negates XYZ and returns12byte sret");
    Backend b;Check(b.Profile(1)&&b.heavy==1&&b.resolves==1,"actual profile performs initial heavy validation");for(unsigned i=0;i<10;++i)Check(b.Profile(1)&&b.heavy==1&&b.resolves==1,"actual profile cached once per generation");
    std::memcpy(engine.base+SinIat,&expectedSin,sizeof(expectedSin));Check(b.SinImport(),"native flicker IAT matches shared CRT sinf");FARPROC wrongSin=NULL;std::memcpy(engine.base+SinIat,&wrongSin,sizeof(wrongSin));Check(!b.SinImport(),"modified native sine import refuses profile");std::memcpy(engine.base+SinIat,&expectedSin,sizeof(expectedSin));
    currentOgre=reinterpret_cast<HMODULE>(ogre.base+1);Check(!b.Profile(1)&&b.heavy==1,"module identity change rejects without repeated heavy validation");currentOgre=reinterpret_cast<HMODULE>(ogre.base);Check(b.Profile(2)&&b.heavy==2,"new generation validates profile once");b.goodModules=false;Check(!b.Profile(3)&&!b.Profile(3)&&b.heavy==3,"failed generation profile is not revalidated each light/frame");b.goodModules=true;Check(b.Profile(4)&&b.heavy==4,"new generation can recover profile");
    Check(b.Scene()==scene,"actual scene accessor reads current owned fixture");au=reinterpret_cast<World*>(1);Check(!b.Scene(),"actual scene accessor contains stale-pointer fault");au=&world;
    unsigned __int64 ownerWords[0xc0/8]={0},clockWords[0xc0/8]={0};BYTE*owner=reinterpret_cast<BYTE*>(ownerWords),*clock=reinterpret_cast<BYTE*>(clockWords);Node node={{123.5f,35.25f,-456.5f},false};Light light;Init(light,node,scene);const void*array[2]={&light,&light};
    expectedNode=&node;Pointer(engine.base,RegistryGlobal,owner);Pointer(engine.base,ClockGlobal,clock);Pointer(owner,0xb0,array);Count(owner,0xb8,2);owner[0x5c]=1;Float(owner,0x60,2.25f);Float(clock,0x84,.5f);Float(clock,0xb0,.2f);Float(clock,0xb4,.8f);Double(clock,0xa0,17.125);
    RegistryState state;Check(b.ReadState(state,scene)&&state.owner==owner&&state.data==array&&state.count==2&&state.enabled==1&&state.gain==2.25f&&state.clock.time==17.125&&state.clock.phase==.5f,"actual native registry and clock ABI offsets");Check(state.sourceScene==scene&&state.SourceSceneOrigin[0]==1024&&state.SourceSceneOrigin[1]==12&&state.SourceSceneOrigin[2]==-2048,"actual state captures origin from same expected scene");const void*copied[Limit]={0};Check(b.CopyPointers(state,copied)&&copied[0]==&light&&copied[1]==&light,"actual bounded pointer copy preserves native duplicates");
    Count(owner,0xb8,4097);Pointer(engine.base,ClockGlobal,reinterpret_cast<const void*>(1));RegistryState oversized;unsigned callsBefore=originCalls;Check(b.ReadState(oversized,scene)&&oversized.count==4097&&originCalls==callsBefore,"registry overflow known before clock or origin dereference");Count(owner,0xb8,2);Pointer(engine.base,ClockGlobal,clock);
    badOriginReturn=true;Check(!b.ReadState(oversized,scene),"native origin aggregate output identity required");badOriginReturn=false;
    unsigned nanBits=0x7fc00000;float nan;std::memcpy(&nan,&nanBits,4);fixture.Set(nan,12,-2048);Check(!b.ReadState(oversized,scene),"nonfinite native scene origin refused");fixture.Set(1024,12,-2048);
    callsBefore=originCalls;Check(!b.ReadState(oversized,reinterpret_cast<const void*>(0x999))&&originCalls==callsBefore,"wrong expected scene refuses before origin getter");
    changeSceneDuringOrigin=true;Check(!b.ReadState(oversized,scene),"scene change during origin getter refuses state");changeSceneDuringOrigin=false;render.scene=scene;
    RegistryState bad=state;bad.count=4097;Check(!b.CopyPointers(bad,copied),"actual pointer copier refuses4097");bad=state;bad.data=reinterpret_cast<const void*>(8);Check(!b.CopyPointers(bad,copied),"actual pointer copier contains inaccessible span fault");bad.data=reinterpret_cast<const void*>(ULONG_PTR(-8));Check(!b.CopyPointers(bad,copied),"actual pointer copier refuses address overflow");
    Pointer(engine.base,RegistryGlobal,reinterpret_cast<const void*>(8));Check(!b.ReadState(bad,scene),"actual registry accessor contains owner read fault");Pointer(engine.base,RegistryGlobal,owner);
    LightValue value={};bool visible=false;calls=0;Check(b.ReadLight(&light,scene,value,visible)&&visible&&value.type==0&&value.worldPosition[0]==123.5f&&value.worldPosition[2]==-456.5f&&value.diffuse[0]==1.5f&&value.custom0[1]==4&&value.range==80&&value.attenuation[2]==.001f,"actual native point-light getter dispatch and sret");
    light.type=2;cosCalls=0;value=LightValue();Check(b.ReadLight(&light,scene,value,visible)&&value.worldDirection[1]==-1&&value.spot[0]==::cosf(.2f)&&value.spot[1]==::cosf(.6f)&&value.spot[2]==2&&cosCalls==2,"actual native spot half-angle cosine dispatch");
    node.badOutput=true;Check(!b.ReadLight(&light,scene,value,visible),"actual position aggregate output identity required");node.badOutput=false;light.badDirection=true;Check(!b.ReadLight(&light,scene,value,visible),"actual direction aggregate output identity required");light.badDirection=false;
    light.scene=reinterpret_cast<void*>(0x999);calls=0;Check(!b.ReadLight(&light,scene,value,visible)&&calls==1,"actual native scene mismatch refuses before visibility");light.scene=scene;
    light.vtable=NULL;calls=0;Check(!b.ReadLight(&light,scene,value,visible)&&calls==0,"actual concrete light vtable checked before native getter");light.vtable=ogre.base+LightVtable;
    light.visible=false;typeCalls=0;Check(b.ReadLight(&light,scene,value,visible)&&!visible&&typeCalls==0,"actual invisible light skips dependent getters");light.visible=true;
    light.power=0;typeCalls=0;Check(b.ReadLight(&light,scene,value,visible)&&typeCalls==0,"actual zero power skips dependent getters");light.power=3;
    light.parent=NULL;Check(!b.ReadLight(&light,scene,value,visible),"actual unattached light refuses");light.parent=&node;
    light.inner=light.outer;Check(!b.ReadLight(&light,scene,value,visible),"actual collapsed spot angles refuse");light.inner=.4f;
    light.badColour=true;Check(!b.ReadLight(&light,scene,value,visible),"actual getter reference fault contained");light.badColour=false;
    light.noCustom=true;bool caught=false;try{b.ReadLight(&light,scene,value,visible);}catch(const std::exception&){caught=true;}Check(caught,"C++ native exception is not swallowed by SEH bridge");light.noCustom=false;
    Snapshot snapshot;volatile LONG generation=4;Check(snapshot.Read(b,true,4,scene,true,&generation,0)==S_OK&&snapshot.Count()==2&&snapshot.Light(0)->effectivePower==6.75f,"actual collector and extracted ABI backend integrate on owned fixture");Check(snapshot.Info().SourceSceneOrigin[0]==1024&&snapshot.Light(0)->worldPosition[0]==123.5f,"actual ABI origin published without guessed world conversion");
    rebaseDuringPosition=true;Check(FAILED(snapshot.Read(b,true,4,scene,true,&generation,1000))&&!snapshot.Valid(4)&&snapshot.Count()==0&&snapshot.Info().SourceSceneOrigin[0]==0,"actual native rebase during acquisition retires old origin and payload");rebaseDuringPosition=false;fixture.Set(1024,12,-2048);
    light.noCustom=true;Check(FAILED(snapshot.Read(b,true,4,scene,true,&generation,2000))&&!snapshot.Valid(4)&&std::strstr(snapshot.LastError(),"custom0"),"actual collector retires snapshot on native C++ custom failure");light.noCustom=false;
    au=NULL;currentEngine=currentOgre=NULL;
}
}
int main(int argc,char**argv){if(argc!=2)return 2;try{OriginFixture fixture(argv[1]);Fixture::Run(fixture);std::printf("Native local-light backend PASS %u checks; actual extracted methods, owned native-layout pages/getter seams\n",checks);return 0;}catch(...){return 1;}}
