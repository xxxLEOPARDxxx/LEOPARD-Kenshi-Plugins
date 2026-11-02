#define GSM_NATIVE_WATER_POLICY_ONLY
#ifndef NATIVE_WATER_HEADER
#define NATIVE_WATER_HEADER "NativeWaterSnapshot.h"
#endif
#include NATIVE_WATER_HEADER
#include <cstdio>
#include <stdexcept>
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void Values(AtlasWater::Uniforms&u){std::memset(&u,0,sizeof(u));u.waterPlane[0]=137.5f;u.waterPlane[1]=1;u.nativeMapBounds[0]=u.nativeMapBounds[1]=-147456;u.nativeMapBounds[2]=u.nativeMapBounds[3]=147456;u.normalScale[0]=u.normalScale[1]=.004f;u.waterSunDirection[1]=1;u.waterSunColour[0]=2;u.waterSunColour[1]=1;u.waterSunColour[3]=.5f;u.waterEnvColour[0]=.2f;u.waterEnvColour[3]=.8f;u.materialParams[0]=1;u.materialParams[1]=120;u.materialParams[2]=.9f;u.waterTime[0]=12.75f;u.parallelView[1]=1;}
class DispatchFixture {
    bool valid_;unsigned generation_;char error_[256];
    void Error(const char*s){strncpy_s(error_,sizeof(error_),s,_TRUNCATE);}
    void ReadCurrent(bool sun,bool ambient,unsigned generation){++queries;if(fail)throw std::runtime_error("fixture current native read failed");observedSun=sun;observedAmbient=ambient;generation_=generation;valid_=true;}
public:
    unsigned queries,clears;bool fail,observedSun,observedAmbient;
    DispatchFixture():valid_(false),generation_(0),queries(0),clears(0),fail(false),observedSun(false),observedAmbient(false){error_[0]=0;}
    void Clear(){++clears;valid_=false;generation_=0;}
    const char*ErrorText()const{return error_;}bool Valid()const{return valid_;}
#include "native_snapshot_dispatch.inc"
};
namespace BasisFixture {
namespace Ogre {
enum SceneMemoryMgrTypes {SCENE_DYNAMIC=0};
struct SceneNode {float q[4],s[3];};struct SceneManager {SceneNode roots[2];};typedef SceneNode Node;
}
struct Render {Ogre::SceneManager*scene;};struct World {Render*render;};
static World*au=NULL;static unsigned selected=99,rootCalls=0;static bool badReturn=false;
using NativeWater::WaterRootIndex;using NativeWater::BasisValid;
static Ogre::SceneNode*__fastcall Root(Ogre::SceneManager*s,Ogre::SceneMemoryMgrTypes index){++rootCalls;selected=unsigned(index);return selected<2?&s->roots[selected]:NULL;}
static void*__fastcall Quaternion(const Ogre::Node*n,float*out){std::memcpy(out,n->q,16);return badReturn?NULL:out;}
static void*__fastcall Scale(const Ogre::Node*n,float*out){std::memcpy(out,n->s,12);return badReturn?NULL:out;}
static FARPROC Pinned(DWORD rva){return rva==0x2bc090?reinterpret_cast<FARPROC>(Root):rva==0x1dec10?reinterpret_cast<FARPROC>(Quaternion):reinterpret_cast<FARPROC>(Scale);}
#include "native_snapshot_basis.inc"
static bool Invoke(){try{StockBasis();return true;}catch(...){return false;}}
static void Test(){
    Ogre::SceneManager scene={};scene.roots[0].q[1]=1;scene.roots[1].q[0]=1;for(unsigned i=0;i<3;++i)scene.roots[0].s[i]=scene.roots[1].s[i]=1;
    Render render={&scene};World world={&render};au=&world;
    Check(Invoke()&&selected==1&&rootCalls==1,"actual native water root index1 selected instead of SDK dynamic0");
    scene.roots[1].s[2]=2;Check(!Invoke()&&selected==1,"actual root1 basis guard rejects changed scale");scene.roots[1].s[2]=1;
    badReturn=true;Check(!Invoke(),"actual aggregate getter output identity required");badReturn=false;
    au=NULL;unsigned before=rootCalls;Check(!Invoke()&&rootCalls==before,"world absence refuses before native root query");
}
}
namespace TextureFixture {
namespace Ogre {
struct Resource {bool loaded;};struct Texture:Resource {bool type;};struct Control {int refs;};
struct TexturePtr {
    Texture*p;Control*c;TexturePtr():p(NULL),c(NULL){}~TexturePtr(){if(p&&c)--c->refs;}
    TexturePtr(const TexturePtr&v):p(v.p),c(v.c){if(p&&c)++c->refs;}
    TexturePtr&operator=(const TexturePtr&v){if(v.p&&v.c)++v.c->refs;if(p&&c)--c->refs;p=v.p;c=v.c;return *this;}
    Texture*get()const{return p;}
};
struct TextureUnitState {unsigned __int64 raw[0x180/8];};
}
static unsigned loadedQueries=0,activeLoads=0;
static bool __fastcall Loaded(const Ogre::Resource*r){++loadedQueries;return r->loaded;}
namespace GsmNative {
static bool TextureType(Ogre::Texture*t){return t&&t->type;}
static FARPROC Export(const char*,DWORD){return reinterpret_cast<FARPROC>(Loaded);}
}
static const char GsmPrivateResourceLoadedName[]="fixture-isLoaded";static const DWORD GsmPrivateResourceLoadedRva=0x66750;
using NativeWater::PassiveFrame;
static const Ogre::TexturePtr&__fastcall ActiveGetter(const Ogre::TextureUnitState*u){++activeLoads;const void*slot=NULL;if(!PassiveFrame(u,slot))throw std::runtime_error("fixture invalid active frame");const Ogre::TexturePtr&result=*static_cast<const Ogre::TexturePtr*>(slot);result.get()->loaded=true;return result;}
static FARPROC Pinned(DWORD){return reinterpret_cast<FARPROC>(ActiveGetter);}
#include "native_snapshot_texture.inc"
static bool Invoke(Ogre::TextureUnitState*u,Ogre::TexturePtr&out){try{UnitTexture(u,out);return true;}catch(...){return false;}}
static void Number(Ogre::TextureUnitState&u,unsigned offset,unsigned value){std::memcpy(reinterpret_cast<BYTE*>(&u)+offset,&value,4);}
static void Pointer(Ogre::TextureUnitState&u,unsigned offset,const void*value){std::memcpy(reinterpret_cast<BYTE*>(&u)+offset,&value,8);}
static void Test(){
    Ogre::TextureUnitState unit={};BYTE names[80]={0};Ogre::Texture tex[2];Ogre::Control control[2];Ogre::TexturePtr frames[2],out;
    for(unsigned i=0;i<2;++i){tex[i].loaded=true;tex[i].type=true;control[i].refs=1;frames[i].p=&tex[i];frames[i].c=&control[i];}
    Number(unit,0,1);Pointer(unit,0x148,names);Pointer(unit,0x150,names+80);Pointer(unit,0x168,frames);Pointer(unit,0x170,frames+2);
    tex[1].loaded=false;Check(!Invoke(&unit,out)&&!out.get()&&activeLoads==0&&loadedQueries==1,"unloaded existing texture refused without native loading");tex[1].loaded=true;
    Check(Invoke(&unit,out)&&out.get()==&tex[1]&&control[1].refs==2&&activeLoads==0,"actual passive UnitTexture retains selected current frame");out=Ogre::TexturePtr();Check(control[1].refs==1,"passive texture owner releases normally");
    Number(unit,0,0);Check(Invoke(&unit,out)&&out.get()==&tex[0],"actual passive UnitTexture distinguishes native frame0 and1");out=Ogre::TexturePtr();Number(unit,0,1);
    unsigned before=loadedQueries;frames[1].p=NULL;Check(!Invoke(&unit,out)&&loadedQueries==before&&!out.get(),"null frame refuses before resource getter");frames[1].p=&tex[1];
    frames[1].c=NULL;Check(!Invoke(&unit,out)&&loadedQueries==before,"missing frame controlblock refused");frames[1].c=&control[1];
    Number(unit,0,2);Check(!Invoke(&unit,out)&&loadedQueries==before,"out-of-range current frame refuses before getter");Number(unit,0,1);
    Pointer(unit,0x170,frames+1);Check(!Invoke(&unit,out),"native name/frame span mismatch refused");Pointer(unit,0x170,frames+2);
    Pointer(unit,0x170,reinterpret_cast<BYTE*>(frames)+31);Check(!Invoke(&unit,out),"misaligned frame span refused");Pointer(unit,0x170,frames+2);
    Pointer(unit,0x170,reinterpret_cast<void*>(reinterpret_cast<ULONG_PTR>(frames)-16));Check(!Invoke(&unit,out),"reversed frame span refused");Pointer(unit,0x170,frames+2);
    Pointer(unit,0x150,reinterpret_cast<void*>(reinterpret_cast<ULONG_PTR>(names)+40*65));Check(!Invoke(&unit,out),"bounded native frame count enforced");Pointer(unit,0x150,names+80);
    Number(unit,0x13c,2);Check(!Invoke(&unit,out),"non-named native content refused");Number(unit,0x13c,0);
    reinterpret_cast<BYTE*>(&unit)[0xb8]=1;Check(!Invoke(&unit,out),"native load-failed frame refuses");reinterpret_cast<BYTE*>(&unit)[0xb8]=0;
    tex[1].type=false;Check(!Invoke(&unit,out),"wrong concrete texture type refused");tex[1].type=true;
    Check(!Invoke(NULL,out)&&activeLoads==0,"missing unit never invokes active getter");
}
}
static void View(ID3D11Device*d,bool cube,unsigned mips,unsigned first,unsigned visible,AtlasView::Com<ID3D11ShaderResourceView>&out){
    D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=16;t.MipLevels=mips;t.ArraySize=cube?6:1;t.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;t.MiscFlags=cube?D3D11_RESOURCE_MISC_TEXTURECUBE:0;
    AtlasView::Com<ID3D11Texture2D>texture;Check(SUCCEEDED(d->CreateTexture2D(&t,NULL,&texture.p)),"WARP texture fixture");D3D11_SHADER_RESOURCE_VIEW_DESC s={};s.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;s.ViewDimension=cube?D3D11_SRV_DIMENSION_TEXTURECUBE:D3D11_SRV_DIMENSION_TEXTURE2D;
    if(cube){s.TextureCube.MostDetailedMip=first;s.TextureCube.MipLevels=visible;}else{s.Texture2D.MostDetailedMip=first;s.Texture2D.MipLevels=visible;}
    Check(SUCCEEDED(d->CreateShaderResourceView(texture.p,&s,&out.p)),"WARP sRGB visible mip view fixture");
}
static ULONG References(IUnknown*p){ULONG n=p->AddRef();p->Release();return n-1;}
int main(){try{
    BasisFixture::Test();
    TextureFixture::Test();
    AtlasWater::Uniforms u;Values(u);Check(NativeWater::ValuesValid(u),"current finite native values admitted");
    NativeWater::ApplyOptions(u,false,true);Check(u.waterSunColour[0]==0&&u.waterSunColour[1]==0&&u.waterSunColour[2]==0&&u.waterSunColour[3]==.5f,"sun OFF zeros water RGB while retaining native W");NativeWater::ApplyOptions(u,true,false);Check(u.waterEnvColour[0]==.2f&&u.waterEnvColour[3]==0,"ambient OFF zeros native W only");
    Values(u);u.waterPlane[0]=-1e8f;Check(NativeWater::ValuesValid(u),"absolute height is not hardcoded initial100");u.waterTime[0]=-123.25f;Check(NativeWater::ValuesValid(u),"native gameTime preserved without wall clock substitution");
    const unsigned invalidFields[]={0,12,16,20,24,28,32,36,40,44};for(unsigned i=0;i<sizeof(invalidFields)/sizeof(invalidFields[0]);++i){Values(u);unsigned nan=0x7fc00000;std::memcpy(reinterpret_cast<float*>(&u)+invalidFields[i],&nan,4);Check(!NativeWater::ValuesValid(u),"nonfinite value rejected in copied uniform rows");}
    Values(u);u.nativeMapBounds[2]=u.nativeMapBounds[0];Check(!NativeWater::ValuesValid(u),"ordered map bounds required");Values(u);u.materialParams[2]=1.1f;Check(!NativeWater::ValuesValid(u),"native gloss range guarded");Values(u);u.waterSunDirection[1]=0;Check(!NativeWater::ValuesValid(u),"nonzero native sun required");
    for(unsigned n=1;n<=4;++n){Check(NativeWater::DefinitionValid(n,8,n,1,n,12),"typed scalar/vector range admitted");Check(!NativeWater::DefinitionValid(n+8,8,n,1,n,12),"integer or wrong constant type rejected");Check(!NativeWater::DefinitionValid(n,8,n,2,n,16),"constant array rejected");Check(!NativeWater::DefinitionValid(n,16,n,1,n,16),"float buffer end rejected");Check(!NativeWater::DefinitionValid(n,size_t(-1),n,1,n,16),"overflowing physical index rejected");}
    Check(!NativeWater::DefinitionValid(1,0,8,1,1,16)&&!NativeWater::DefinitionValid(1,0,1,1,1,1048577),"unexpected padding/buffer size rejected");
    float q[4]={1,0,0,0},scale[3]={1,1,1};Check(NativeWater::BasisValid(q,scale),"stock basis admitted");q[0]=-1;Check(NativeWater::BasisValid(q,scale),"equivalent negative identity quaternion admitted");q[1]=.01f;Check(!NativeWater::BasisValid(q,scale),"rotated root refused");q[1]=0;scale[2]=2;Check(!NativeWater::BasisValid(q,scale),"scaled root refused");
    D3D11_SAMPLER_DESC desc[5];unsigned address[3]={0,2,3};float border[4]={.1f,.2f,.3f,.4f};
    for(unsigned min=0;min<4;++min)for(unsigned mag=0;mag<4;++mag)for(unsigned mip=0;mip<4;++mip){D3D11_SAMPLER_DESC d;Check(NativeWater::BuildSampler(min,mag,mip,address,32,border,-2.25f,false,d),"all64 native filter triples admitted");Check(d.MipLODBias==-2.25f&&d.MinLOD==0&&d.MaxLOD==D3D11_FLOAT32_MAX&&d.MaxAnisotropy==16,"sampler bias and FO_NONE unlimited MaxLOD preserved");D3D11_FILTER want=min==3||mag==3||mip==3?D3D11_FILTER_ANISOTROPIC:D3D11_ENCODE_BASIC_FILTER(min==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mag==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mip==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,0);Check(d.Filter==want&&d.AddressU==D3D11_TEXTURE_ADDRESS_WRAP&&d.AddressV==D3D11_TEXTURE_ADDRESS_CLAMP&&d.AddressW==D3D11_TEXTURE_ADDRESS_BORDER&&d.BorderColor[3]==.4f,"native filter address border conversion");}
    Check(!NativeWater::BuildSampler(4,1,1,address,1,border,0,false,desc[0]),"unknown filter refused");Check(!NativeWater::BuildSampler(1,1,1,address,1,border,0,true,desc[0]),"comparison sampler refused");Check(!NativeWater::BuildSampler(1,1,1,address,1,border,16,false,desc[0]),"out-of-range bias refused");
    for(unsigned i=0;i<5;++i)Check(NativeWater::BuildSampler(2,2,2,address,0,border,float(i)*.5f,false,desc[i]),"fixture native sampler descriptor");
    AtlasView::Com<ID3D11Device>device,other;AtlasView::Com<ID3D11DeviceContext>context,otherContext;D3D_FEATURE_LEVEL actual;
    Check(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&device.p,&actual,&context.p)),"real WARP device");Check(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&other.p,&actual,&otherContext.p)),"different WARP device fixture");
    AtlasView::Com<ID3D11ShaderResourceView>own[5],different;ID3D11ShaderResourceView*views[5];for(unsigned i=0;i<5;++i){View(device.p,i>=3,i==4?1:4,i==2?1:0,i==2?2:i==4?1:4,own[i]);views[i]=own[i].p;}View(other.p,false,1,0,1,different);
    ULONG before=References(views[4]);NativeWater::Resources resources;context.p->PSSetShaderResources(7,1,&views[0]);
    Check(SUCCEEDED(resources.Update(views,desc)),"actual one-mip water specular cube admitted");Check(resources.Mips(4)==1&&resources.Mips(2)==2&&resources.View(4)==views[4],"native visible mip range and SRV identity preserved");Check(References(views[4])==before+1,"snapshot independently retains native SRV COM reference");
    D3D11_SHADER_RESOURCE_VIEW_DESC sv;resources.View(0)->GetDesc(&sv);Check(sv.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,"native gamma interpretation retained");ID3D11SamplerState*same=resources.Sampler(2);Check(SUCCEEDED(resources.Update(views,desc))&&resources.Sampler(2)==same&&References(views[4])==before+1,"unchanged refresh reuses sampler and bounded SRV ownership");
    D3D11_SAMPLER_DESC got;resources.Sampler(2)->GetDesc(&got);Check(got.MipLODBias==1&&got.MaxLOD==D3D11_FLOAT32_MAX,"actual D3D sampler preserves native bias");desc[2].MipLODBias=-1.5f;Check(SUCCEEDED(resources.Update(views,desc)),"descriptor change refreshes sampler");resources.Sampler(2)->GetDesc(&got);Check(got.MipLODBias==-1.5f,"changed native bias not cached stale");
    views[1]=different.p;Check(FAILED(resources.Update(views,desc))&&resources.View(4)==own[4].p,"different COM device refused before commit");views[1]=own[1].p;views[4]=own[0].p;Check(FAILED(resources.Update(views,desc)),"2D water specular substitution refused");views[4]=own[4].p;
    AtlasView::Com<ID3D11ShaderResourceView>bound;context.p->PSGetShaderResources(7,1,&bound.p);Check(bound.p==own[0].p,"snapshot does not mutate native context bindings");resources.Clear();Check(!resources.Device()&&!resources.View(4)&&!resources.Sampler(2)&&References(views[4])==before,"clear releases all independently retained resources");
    DispatchFixture dispatch;Check(dispatch.Read(false,true,true,7,false)==S_FALSE&&dispatch.queries==0&&!dispatch.Valid(),"production OFF branch performs no native queries");Check(dispatch.Read(true,true,true,7,false)==E_ACCESSDENIED&&dispatch.queries==0&&dispatch.ErrorText()[0],"unverified source profile refuses before native queries");Check(dispatch.Read(true,false,true,7,true)==S_OK&&dispatch.queries==1&&!dispatch.observedSun&&dispatch.observedAmbient&&dispatch.Valid(),"production dispatch passes options and generation");unsigned clears=dispatch.clears;Check(dispatch.Read(true,true,false,8,true)==S_OK&&dispatch.clears==clears+1,"world generation clears snapshot before new read");dispatch.fail=true;Check(FAILED(dispatch.Read(true,true,true,8,true))&&!dispatch.Valid()&&std::strstr(dispatch.ErrorText(),"fixture current"),"read failure retires snapshot and preserves fixed error");
    std::printf("Native water snapshot PASS %u checks; real WARP ownership/mips/samplers + extracted production dispatch\n",checks);return 0;
}catch(...){return 1;}}
