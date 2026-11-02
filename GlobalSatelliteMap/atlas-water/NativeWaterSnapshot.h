#pragma once
#include "AtlasWater.h"
namespace NativeWater {
// Native504b water constructor passes literal1. SDK enum labels differ.
const unsigned WaterRootIndex=1;
inline bool PassiveFrame(const void*unit,const void*&slot){
    slot=NULL;
    __try{
        if(!unit)return false;const BYTE*b=static_cast<const BYTE*>(unit);
        if(*reinterpret_cast<const unsigned*>(b+0x13c)!=0||b[0xb8])return false;
        const unsigned current=*reinterpret_cast<const unsigned*>(b);
        const ULONG_PTR names=*reinterpret_cast<const ULONG_PTR*>(b+0x148),namesEnd=*reinterpret_cast<const ULONG_PTR*>(b+0x150);
        const ULONG_PTR frames=*reinterpret_cast<const ULONG_PTR*>(b+0x168),framesEnd=*reinterpret_cast<const ULONG_PTR*>(b+0x170);
        if(!names||!frames||namesEnd<names||framesEnd<frames||(namesEnd-names)%40||(framesEnd-frames)%16)return false;
        const ULONG_PTR count=(namesEnd-names)/40,frameCount=(framesEnd-frames)/16;
        if(!count||count>64||frameCount!=count||current>=count||frames%8)return false;
        const BYTE*value=reinterpret_cast<const BYTE*>(frames+ULONG_PTR(current)*16);
        if(!*reinterpret_cast<const void*const*>(value)||!*reinterpret_cast<const void*const*>(value+8))return false;
        slot=value;return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){slot=NULL;return false;}
}
inline bool FloatRange(size_t physical,size_t elements,size_t arraySize,unsigned count,size_t available){return count>=1&&count<=4&&arraySize==1&&elements>=count&&elements<=4&&physical<=available&&elements<=available-physical;}
inline bool DefinitionValid(unsigned type,size_t physical,size_t elements,size_t arraySize,unsigned count,size_t available){return type==count&&available<=1048576&&FloatRange(physical,elements,arraySize,count,available);}
inline bool BasisValid(const float*q,const float*s){
    for(unsigned i=0;i<4;++i)if(!_finite(q[i]))return false;
    for(unsigned i=0;i<3;++i)if(!_finite(s[i])||std::fabs(s[i]-1)>1e-5f)return false;
    return std::fabs(std::fabs(q[0])-1)<1e-5f&&std::fabs(q[1])<1e-5f&&std::fabs(q[2])<1e-5f&&std::fabs(q[3])<1e-5f;
}


inline void ApplyOptions(AtlasWater::Uniforms&u,bool sun,bool ambient){if(!sun)u.waterSunColour[0]=u.waterSunColour[1]=u.waterSunColour[2]=0;if(!ambient)u.waterEnvColour[3]=0;}
inline bool ValuesValid(const AtlasWater::Uniforms&u){
    const float*f=reinterpret_cast<const float*>(&u);for(unsigned i=0;i<sizeof(u)/sizeof(float);++i)if(!_finite(f[i]))return false;
    double sun=0,view=0;for(unsigned i=0;i<3;++i){sun+=double(u.waterSunDirection[i])*u.waterSunDirection[i];view+=double(u.parallelView[i])*u.parallelView[i];}
    return u.waterPlane[1]==1&&u.nativeMapBounds[0]<u.nativeMapBounds[2]&&u.nativeMapBounds[1]<u.nativeMapBounds[3]&&sun>1e-8&&view>1e-8&&u.materialParams[0]>0&&u.materialParams[1]>0&&u.materialParams[2]>=0&&u.materialParams[2]<=1;
}
inline bool BuildSampler(unsigned min,unsigned mag,unsigned mip,const unsigned address[3],unsigned anisotropy,const float border[4],float bias,bool comparison,D3D11_SAMPLER_DESC&out){
    if(min>3||mag>3||mip>3||comparison||!_finite(bias)||bias<D3D11_MIP_LOD_BIAS_MIN||bias>D3D11_MIP_LOD_BIAS_MAX)return false;
    std::memset(&out,0,sizeof(out));D3D11_TEXTURE_ADDRESS_MODE modes[3];
    for(unsigned i=0;i<3;++i){if(address[i]>3)return false;modes[i]=address[i]==0?D3D11_TEXTURE_ADDRESS_WRAP:address[i]==1?D3D11_TEXTURE_ADDRESS_MIRROR:address[i]==2?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_BORDER;}
    for(unsigned i=0;i<4;++i){if(!_finite(border[i]))return false;out.BorderColor[i]=border[i];}
    out.AddressU=modes[0];out.AddressV=modes[1];out.AddressW=modes[2];out.MipLODBias=bias;out.MinLOD=0;out.MaxLOD=D3D11_FLOAT32_MAX;out.ComparisonFunc=D3D11_COMPARISON_NEVER;
    out.MaxAnisotropy=anisotropy<1?1:anisotropy>16?16:anisotropy;
    out.Filter=min==3||mag==3||mip==3?D3D11_FILTER_ANISOTROPIC:D3D11_ENCODE_BASIC_FILTER(min==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mag==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mip==2?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,0);
    return true;
}
class Resources {
    AtlasView::Com<ID3D11Device> device_;AtlasView::Com<ID3D11ShaderResourceView> views_[5];AtlasView::Com<ID3D11SamplerState> samplers_[5];D3D11_SAMPLER_DESC descriptors_[5];unsigned mips_[5];
    Resources(const Resources&);Resources&operator=(const Resources&);
public:
    Resources(){std::memset(descriptors_,0,sizeof(descriptors_));std::memset(mips_,0,sizeof(mips_));}
    void Clear(){for(unsigned i=0;i<5;++i){samplers_[i].Reset();views_[i].Reset();mips_[i]=0;}device_.Reset();std::memset(descriptors_,0,sizeof(descriptors_));}
    ID3D11Device*Device()const{return device_.p;}ID3D11ShaderResourceView*View(unsigned i)const{return i<5?views_[i].p:NULL;}ID3D11SamplerState*Sampler(unsigned i)const{return i<5?samplers_[i].p:NULL;}unsigned Mips(unsigned i)const{return i<5?mips_[i]:0;}
    HRESULT Update(ID3D11ShaderResourceView*const views[5],const D3D11_SAMPLER_DESC desc[5]){
        if(!views||!desc||!views[0])return E_INVALIDARG;
        AtlasView::Com<ID3D11Device> d;views[0]->GetDevice(&d.p);if(!d.p)return E_INVALIDARG;
        AtlasView::Com<ID3D11SamplerState> replacements[5];unsigned counts[5];
        const bool same=AtlasView::Identity(d.p,device_.p);
        for(unsigned i=0;i<5;++i){
            if(!AtlasView::LightView(views[i],d.p,i>=3,1))return E_INVALIDARG;
            D3D11_SHADER_RESOURCE_VIEW_DESC s;views[i]->GetDesc(&s);unsigned first=i>=3?s.TextureCube.MostDetailedMip:s.Texture2D.MostDetailedMip;counts[i]=i>=3?s.TextureCube.MipLevels:s.Texture2D.MipLevels;
            AtlasView::Com<ID3D11Resource> r;views[i]->GetResource(&r.p);AtlasView::Com<ID3D11Texture2D> t;
            if(FAILED(r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p))))return E_INVALIDARG;
            D3D11_TEXTURE2D_DESC td;t.p->GetDesc(&td);if(!td.Width||!td.Height||(i>=3&&td.Width!=td.Height))return E_INVALIDARG;if(counts[i]==UINT(-1))counts[i]=td.MipLevels-first;
            if(!same||!samplers_[i].p||std::memcmp(&descriptors_[i],&desc[i],sizeof(desc[i]))){HRESULT hr=d.p->CreateSamplerState(&desc[i],&replacements[i].p);if(FAILED(hr))return hr;}
        }
        if(!same){Clear();device_.p=d.Detach();}
        for(unsigned i=0;i<5;++i){if(views_[i].p!=views[i]){views[i]->AddRef();views_[i].Reset();views_[i].p=views[i];}if(replacements[i].p){samplers_[i].Reset();samplers_[i].p=replacements[i].Detach();}descriptors_[i]=desc[i];mips_[i]=counts[i];}
        return S_OK;
    }
};
}

#ifndef GSM_NATIVE_WATER_POLICY_ONLY
#include <kenshi/Globals.h>
#include "../GsmNativeLight.h"
#include <OgreSceneManager.h>
namespace NativeWater {
struct ExportPin {DWORD rva;const char*name;BYTE bytes[24];};
// BEGIN GENERATED NATIVE WATER PINS
static const ExportPin ExportPins[]={
{0x1dec10,"?_getDerivedOrientation@Node@Ogre@@QEBA?AVQuaternion@2@XZ",{0x44,0x0f,0xb6,0x49,0x38,0x4c,0x8b,0x41,0x70,0x43,0x8b,0x04,0x88,0x89,0x02,0x43,0x8b,0x44,0x88,0x10,0x89,0x42,0x04,0x43}},
{0x1ded50,"?_getDerivedScale@Node@Ogre@@QEBA?AVVector3@2@XZ",{0x44,0x0f,0xb6,0x49,0x38,0x4c,0x8b,0x41,0x78,0x43,0x8b,0x04,0x88,0x89,0x02,0x43,0x8b,0x44,0x88,0x10,0x89,0x42,0x04,0x43}},
{0x37dbd0,"?_getTexturePtr@TextureUnitState@Ogre@@QEBAAEBV?$SharedPtr@VTexture@Ogre@@@2@XZ",{0x8b,0x11,0xe9,0x09,0x00,0x00,0x00,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83}},
{0x37dbe0,"?_getTexturePtr@TextureUnitState@Ogre@@QEBAAEBV?$SharedPtr@VTexture@Ogre@@@2@_K@Z",{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x83,0xb9,0x3c,0x01,0x00,0x00,0x00,0x48,0x8b,0xda,0x48,0x8b,0xf9,0x0f}},
{0xef800,"?findAutoConstantEntry@GpuProgramParameters@Ogre@@QEAAPEBVAutoConstantEntry@12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",{0x40,0x57,0x48,0x81,0xec,0xc0,0x01,0x00,0x00,0x48,0xc7,0x44,0x24,0x40,0xfe,0xff,0xff,0xff,0x48,0x89,0x9c,0x24,0xe0,0x01}},
{0xeb110,"?getConstantDefinition@GpuProgramParameters@Ogre@@QEBAAEBUGpuConstantDefinition@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",{0x48,0x81,0xec,0xc8,0x01,0x00,0x00,0x48,0xc7,0x44,0x24,0x40,0xfe,0xff,0xff,0xff,0x48,0x8b,0x05,0x39,0x73,0x7d,0x00,0x48}},
{0xe7500,"?getConstantDefinition@GpuSharedParameters@Ogre@@QEBAAEBUGpuConstantDefinition@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",{0x48,0x8b,0xc4,0x57,0x48,0x81,0xec,0xf0,0x01,0x00,0x00,0x48,0xc7,0x44,0x24,0x50,0xfe,0xff,0xff,0xff,0x48,0x89,0x58,0x18}},
{0x2146d0,"?getDepthBiasConstant@Pass@Ogre@@QEBAMXZ",{0xf3,0x0f,0x10,0x81,0xb4,0x00,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xf3,0x0f,0x10,0x81,0xb8,0x00,0x00,0x00}},
{0x2146e0,"?getDepthBiasSlopeScale@Pass@Ogre@@QEBAMXZ",{0xf3,0x0f,0x10,0x81,0xb8,0x00,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xf3,0x0f,0x11,0x89,0xbc,0x00,0x00,0x00}},
{0xf79b0,"?getFloatConstantList@GpuProgramParameters@Ogre@@QEBAAEBV?$vector@MV?$STLAllocator@MV?$CategorisedAllocPolicy@$0A@@Ogre@@@Ogre@@@std@@XZ",{0x48,0x8d,0x41,0x58,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8b,0x51,0x08,0x48,0xb8,0x67,0x66}},
{0x230f80,"?getFloatConstantList@GpuSharedParameters@Ogre@@QEBAAEBV?$vector@MV?$STLAllocator@MV?$CategorisedAllocPolicy@$0A@@Ogre@@@Ogre@@@std@@XZ",{0x48,0x8d,0x41,0x78,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8d,0x81,0x98,0x00,0x00,0x00,0xc3}},
{0xf8480,"?getFloatPointer@GpuProgramParameters@Ogre@@QEAAPEAM_K@Z",{0x48,0x8b,0x41,0x58,0x48,0x8d,0x04,0x90,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8b,0x05,0x89,0x16,0x81,0x00,0x44}},
{0xf7960,"?getFloatPointer@GpuProgramParameters@Ogre@@QEBAPEBM_K@Z",{0x48,0x8b,0x41,0x58,0x48,0x8d,0x04,0x90,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8d,0x81,0x38,0x01,0x00,0x00,0xc3}},
{0xf8490,"?getFloatPointer@GpuSharedParameters@Ogre@@QEAAPEAM_K@Z",{0x48,0x8b,0x05,0x89,0x16,0x81,0x00,0x44,0x8b,0x80,0x90,0x01,0x00,0x00,0x48,0x8b,0x41,0x78,0xc6,0x81,0x14,0x01,0x00,0x00}},
{0x230f40,"?getFloatPointer@GpuSharedParameters@Ogre@@QEBAPEBM_K@Z",{0x48,0x8b,0x41,0x78,0x48,0x8d,0x04,0x90,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8b,0x81,0x98,0x00,0x00,0x00,0x48}},
{0x216850,"?getFragmentProgramParameters@Pass@Ogre@@QEBA?AV?$SharedPtr@VGpuProgramParameters@Ogre@@@2@XZ",{0x48,0x89,0x54,0x24,0x10,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff,0xff,0xff,0x48,0x89,0x5c,0x24,0x60}},
{0x3b120,"?getName@TextureUnitState@Ogre@@QEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",{0x48,0x8d,0x81,0x88,0x01,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8d,0x81,0xb0,0x01,0x00,0x00,0xc3}},
{0x2bc090,"?getRootSceneNode@SceneManager@Ogre@@QEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z",{0x48,0x63,0xc2,0x48,0x8b,0x84,0xc1,0xf0,0x05,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0x48,0x8b,0x81,0xa0,0x05,0x00,0x00,0x48}},
{0x37d200,"?getTextureAddressingMode@TextureUnitState@Ogre@@QEBAAEBUUVWAddressingMode@12@XZ",{0x48,0x8d,0x41,0x1c,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x89,0x51,0x1c,0x89,0x51,0x20,0x89,0x51}},
{0x37e460,"?getTextureAnisotropy@TextureUnitState@Ogre@@QEBAIXZ",{0x80,0xb9,0x34,0x01,0x00,0x00,0x00,0x74,0x0e,0x48,0x8b,0x05,0x58,0xb6,0x58,0x00,0x8b,0x80,0xdc,0x01,0x00,0x00,0xc3,0x8b}},
{0x37d270,"?getTextureBorderColour@TextureUnitState@Ogre@@QEBAAEBVColourValue@2@XZ",{0x48,0x8d,0x41,0x28,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x83,0xec,0x68,0x84,0xd2,0x74,0x1c}},
{0x37e420,"?getTextureCompareEnabled@TextureUnitState@Ogre@@QEBA_NXZ",{0x0f,0xb6,0x81,0x24,0x01,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x89,0x91,0x28,0x01,0x00,0x00,0xc3,0xcc}},
{0x37e390,"?getTextureFiltering@TextureUnitState@Ogre@@QEBA?AW4FilterOptions@2@W4FilterType@2@@Z",{0x85,0xd2,0x74,0x53,0xff,0xca,0x74,0x29,0xff,0xca,0x75,0x67,0x38,0x91,0x35,0x01,0x00,0x00,0x74,0x16,0x48,0x8b,0x0d,0x1d}},
{0x3b0e0,"?getTextureMipmapBias@TextureUnitState@Ogre@@QEBAMXZ",{0xf3,0x0f,0x10,0x81,0x30,0x01,0x00,0x00,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x8b,0x81,0x00,0x02,0x00,0x00,0x89,0x02}},
{0x216520,"?getVertexProgramParameters@Pass@Ogre@@QEBA?AV?$SharedPtr@VGpuProgramParameters@Ogre@@@2@XZ",{0x48,0x8b,0xc4,0x57,0x48,0x81,0xec,0xe0,0x01,0x00,0x00,0x48,0xc7,0x44,0x24,0x60,0xfe,0xff,0xff,0xff,0x48,0x89,0x58,0x18}}
};
static const GsmCodeWindow PassiveWindows[]={
{0x37ca30,{0x8b,0x01,0xc3,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0x48,0x8b,0xc1,0x48,0x8b,0x89,0x50,0x01}},
{0x37dbfd,{0x4c,0x8b,0x81,0x50,0x01,0x00,0x00,0x48,0xb8,0x67,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x4c,0x2b,0x81,0x48,0x01,0x00,0x00}},
{0x37dc2b,{0x80,0xb9,0xb8,0x00,0x00,0x00,0x00,0x75,0x21,0x48,0x8b,0xd3,0xe8,0x64,0x02,0x00,0x00,0x48,0xc1,0xe3,0x04,0x48,0x03,0x9f}},
{0x37dc3c,{0x48,0xc1,0xe3,0x04,0x48,0x03,0x9f,0x68,0x01,0x00,0x00,0x48,0x8b,0xc3,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f}},
{0x37dcb0,{0x8b,0x01,0x48,0xc1,0xe0,0x04,0x48,0x03,0x81,0x68,0x01,0x00,0x00,0x48,0x8b,0xc8,0xe9,0x3b,0x49,0xce,0xff,0xcc,0xcc,0xcc}},
{0x37e5a7,{0x48,0x8b,0xbe,0x70,0x01,0x00,0x00,0x48,0x8b,0x9e,0x68,0x01,0x00,0x00,0x48,0x3b,0xdf,0x74,0x7a,0x4c,0x89,0x64,0x24,0x48}}
};
struct SourcePin {const char*path;const char*sha256;};
static const SourcePin SourcePins[]={
{"data/materials/forward/water.material","2e8cddfb41a8a672377741089e1bc9e7fde2868d676ed0fcea7e220858e2f119"},
{"data/materials/forward/water.hlsl","347e9c59ada723e6033cc8a875dd3e724ee8ba5549b4d8bac38ae3cd4265baa5"},
{"data/materials/common/common.program","85c10a97ad80b8cc3d74db2c23da673a1f61453bb5e37481dc9d364eb1143183"},
{"data/materials/common/lightingFunctions.hlsl","51dd7cec6f898b3a79352067dbfbe586c1d364967154609ed821667d22966f9f"},
{"data/materials/common/shadowFunctions.hlsl","6b96b21cbb18a52a093bf07b1aab588873b7b07c2efcbb808d347f7e775ec49e"}
};
// END GENERATED NATIVE WATER PINS
inline bool CodeMatches(FARPROC p,const BYTE*bytes){__try{return !std::memcmp(reinterpret_cast<const void*>(p),bytes,24);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline FARPROC Pinned(DWORD rva){for(unsigned i=0;i<sizeof(ExportPins)/sizeof(ExportPins[0]);++i)if(ExportPins[i].rva==rva){FARPROC p=GsmNative::Export(ExportPins[i].name,rva);if(!CodeMatches(p,ExportPins[i].bytes))throw std::runtime_error("native water export bytes differ");return p;}throw std::runtime_error("native water export is not pinned");}
inline FARPROC PinnedNamed(const char*name){for(unsigned i=0;i<sizeof(ExportPins)/sizeof(ExportPins[0]);++i)if(!std::strncmp(ExportPins[i].name,name,std::strlen(name)))return Pinned(ExportPins[i].rva);throw std::runtime_error("native water named export missing");}
inline void Profile(){if(!GsmNative::Profile()||!GsmNative::Module(GetModuleHandleA(GsmOgreFile),GsmOgreTimestamp,GsmOgreSize,PassiveWindows,sizeof(PassiveWindows)/sizeof(PassiveWindows[0])))throw std::runtime_error("native water module/passive-layout profile differs");for(unsigned i=0;i<sizeof(ExportPins)/sizeof(ExportPins[0]);++i)Pinned(ExportPins[i].rva);}
inline bool CopyFinite(const float*source,unsigned count,float*out){__try{for(unsigned i=0;i<count;++i){if(!_finite(source[i]))return false;out[i]=source[i];}return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
template<class T>inline void Constants(T*p,const char*name,unsigned count,bool shared,float*out){
    if(!p)throw std::runtime_error("native water parameter owner absent");
    typedef const Ogre::GpuConstantDefinition&(__fastcall*Definition)(const T*,const Ogre::String&);
    typedef const Ogre::FloatConstantList&(__fastcall*List)(const T*);
    typedef const float*(__fastcall*Pointer)(const T*,size_t);
    Ogre::String key(name);const Ogre::GpuConstantDefinition&d=reinterpret_cast<Definition>(Pinned(shared?0xe7500:0xeb110))(p,key);
    const Ogre::FloatConstantList&list=reinterpret_cast<List>(Pinned(shared?0x230f80:0xf79b0))(p);
    if(!DefinitionValid(unsigned(d.constType),d.physicalIndex,d.elementSize,d.arraySize,count,list.size()))throw std::runtime_error("native water float type/range differs");
    if(!shared){typedef const void*(__fastcall*Auto)(T*,const Ogre::String&);if(reinterpret_cast<Auto>(Pinned(0xef800))(p,key))throw std::runtime_error("native water manual parameter became auto-bound");}
    const float*v=reinterpret_cast<Pointer>(Pinned(shared?0x230f40:0xf7960))(p,d.physicalIndex);
    if(v!=&list[d.physicalIndex]||!CopyFinite(v,count,out))throw std::runtime_error("native water float pointer/value invalid");
}
inline void StockBasis(){
    if(!au||!au->render||!au->render->scene)throw std::runtime_error("native water world unavailable");
    typedef Ogre::SceneNode*(__fastcall*Root)(Ogre::SceneManager*,Ogre::SceneMemoryMgrTypes);
    Ogre::SceneNode*root=reinterpret_cast<Root>(Pinned(0x2bc090))(au->render->scene,static_cast<Ogre::SceneMemoryMgrTypes>(WaterRootIndex));if(!root)throw std::runtime_error("native water scene root absent");
    typedef void*(__fastcall*Aggregate)(const Ogre::Node*,float*);float q[4]={0},s[3]={0};
    if(reinterpret_cast<Aggregate>(Pinned(0x1dec10))(root,q)!=q||reinterpret_cast<Aggregate>(Pinned(0x1ded50))(root,s)!=s||!BasisValid(q,s))throw std::runtime_error("native water stock plane basis differs");
}
inline Ogre::GpuProgramParametersSharedPtr Parameters(Ogre::Pass*p,bool vertex){
    typedef Ogre::GpuProgramParametersSharedPtr*(__fastcall*Get)(const Ogre::Pass*,Ogre::GpuProgramParametersSharedPtr*);
    Ogre::GpuProgramParametersSharedPtr result;if(reinterpret_cast<Get>(Pinned(vertex?0x216520:0x216850))(p,&result)!=&result||result.isNull())throw std::runtime_error("native water program parameters unavailable");return result;
}
inline void UnitTexture(Ogre::TextureUnitState*u,Ogre::TexturePtr&out){
    const void*slot=NULL;if(!PassiveFrame(u,slot))throw std::runtime_error("native water existing texture frame unavailable");
    const Ogre::TexturePtr&existing=*static_cast<const Ogre::TexturePtr*>(slot);
    if(!GsmNative::TextureType(existing.get()))throw std::runtime_error("native water existing texture type differs");
    typedef bool(__fastcall*Loaded)(const Ogre::Resource*);
    if(!reinterpret_cast<Loaded>(GsmNative::Export(GsmPrivateResourceLoadedName,GsmPrivateResourceLoadedRva))(existing.get()))throw std::runtime_error("native water existing texture is not loaded");
    out=existing;
}
inline void Sampler(Ogre::TextureUnitState*u,D3D11_SAMPLER_DESC&out){
    typedef const Ogre::TextureUnitState::UVWAddressingMode&(__fastcall*Address)(const Ogre::TextureUnitState*);
    typedef unsigned(__fastcall*Anisotropy)(const Ogre::TextureUnitState*);
    typedef const Ogre::ColourValue&(__fastcall*Border)(const Ogre::TextureUnitState*);
    typedef bool(__fastcall*Compare)(const Ogre::TextureUnitState*);
    typedef Ogre::FilterOptions(__fastcall*Filter)(const Ogre::TextureUnitState*,Ogre::FilterType);
    typedef float(__fastcall*Bias)(const Ogre::TextureUnitState*);
    const Ogre::TextureUnitState::UVWAddressingMode&a=reinterpret_cast<Address>(Pinned(0x37d200))(u);unsigned modes[3]={unsigned(a.u),unsigned(a.v),unsigned(a.w)};
    const Ogre::ColourValue&b=reinterpret_cast<Border>(Pinned(0x37d270))(u);float border[4]={b.r,b.g,b.b,b.a};Filter filter=reinterpret_cast<Filter>(Pinned(0x37e390));
    if(!BuildSampler(filter(u,Ogre::FT_MIN),filter(u,Ogre::FT_MAG),filter(u,Ogre::FT_MIP),modes,reinterpret_cast<Anisotropy>(Pinned(0x37e460))(u),border,reinterpret_cast<Bias>(Pinned(0x3b0e0))(u),reinterpret_cast<Compare>(Pinned(0x37e420))(u),out))throw std::runtime_error("native water sampler unsupported");
}
class Snapshot {
    Ogre::MaterialPtr material_;Ogre::GpuProgramParametersSharedPtr vertex_,fragment_;Ogre::GpuSharedParametersPtr shared_[3];Ogre::TexturePtr textures_[5];Resources resources_;AtlasWater::Uniforms values_;float depthBias_[2];unsigned generation_;bool valid_,profiled_;char error_[256];
    Snapshot(const Snapshot&);Snapshot&operator=(const Snapshot&);
    void Error(const char*s){size_t i=0;if(s)for(;i<sizeof(error_)-1&&s[i];++i)error_[i]=s[i];error_[i]=0;}
    void ReadCurrent(bool sun,bool ambient,unsigned generation){
        if(!profiled_){Profile();profiled_=true;}StockBasis();
        Ogre::MaterialPtr material=Ogre::MaterialManager::getSingleton().getByName("waterDistant");
        typedef bool(__fastcall*Loaded)(const Ogre::Resource*);
        if(material.isNull()||!reinterpret_cast<Loaded>(GsmNative::Export(GsmPrivateResourceLoadedName,GsmPrivateResourceLoadedRva))(material.get())||material->getNumTechniques()!=1||material->getTechnique(0)->getNumPasses()!=1)throw std::runtime_error("native waterDistant material unavailable");
        Ogre::Pass*p=material->getTechnique(0)->getPass(0);
        if(p->getVertexProgramName()!="WaterVP"||p->getFragmentProgramName()!="WaterFPDistant"||p->getNumTextureUnitStates()!=6)throw std::runtime_error("native water pass/program layout differs");
        typedef float(__fastcall*Bias)(const Ogre::Pass*);float depthBias[2]={reinterpret_cast<Bias>(PinnedNamed("?getDepthBiasConstant@Pass@"))(p),reinterpret_cast<Bias>(PinnedNamed("?getDepthBiasSlopeScale@Pass@"))(p)};if(!_finite(depthBias[0])||!_finite(depthBias[1]))throw std::runtime_error("native water depth bias invalid");
        Ogre::GpuProgramParametersSharedPtr vertex=Parameters(p,true),fragment=Parameters(p,false);AtlasWater::Uniforms u={};u.waterPlane[1]=1;u.parallelView[1]=1;
        Ogre::GpuSharedParametersPtr shared[3];const char*sets[3]={"SharedWaterParams","SharedSkyParams","AmbientParams"};for(unsigned i=0;i<3;++i)shared[i]=Ogre::GpuProgramManager::getSingleton().getSharedParameters(sets[i]);
        Constants(shared[0].get(),"waterHeight",1,true,u.waterPlane);Constants(shared[1].get(),"gameTime",1,true,u.waterTime);Constants(shared[1].get(),"sunDirection",3,true,u.waterSunDirection);Constants(shared[1].get(),"sunColour",4,true,u.waterSunColour);Constants(shared[2].get(),"envColour",4,true,u.waterEnvColour);
        Constants(vertex.get(),"mapBounds",4,false,u.nativeMapBounds);Constants(vertex.get(),"scale",4,false,u.normalScale);Constants(fragment.get(),"invStrength",1,false,u.materialParams);float distortion[2]={0};Constants(fragment.get(),"distortion",2,false,distortion);u.materialParams[1]=distortion[0];Constants(fragment.get(),"gloss",1,false,u.materialParams+2);Constants(fragment.get(),"glow",1,false,u.materialParams+3);
        ApplyOptions(u,sun,ambient);if(!ValuesValid(u))throw std::runtime_error("native water current values unsupported");
        const unsigned unitIndex[5]={0,1,2,4,5};const char*names[5]={"colourMap","flowMap","normalMap","irradianceCube","specularityCube"};Ogre::TexturePtr textures[5];ID3D11ShaderResourceView*views[5]={0};D3D11_SAMPLER_DESC samplers[5];
        typedef const Ogre::String&(__fastcall*UnitName)(const Ogre::TextureUnitState*);
        for(unsigned i=0;i<5;++i){Ogre::TextureUnitState*unit=p->getTextureUnitState(unitIndex[i]);if(!unit||reinterpret_cast<UnitName>(PinnedNamed("?getName@TextureUnitState@"))(unit)!=names[i])throw std::runtime_error("native water texture unit name differs");UnitTexture(unit,textures[i]);views[i]=GsmNative::Srv(textures[i].get());Sampler(unit,samplers[i]);}
        if(FAILED(resources_.Update(views,samplers)))throw std::runtime_error("native water SRV/device/sampler validation failed");
        material_=material;vertex_=vertex;fragment_=fragment;for(unsigned i=0;i<3;++i)shared_[i]=shared[i];for(unsigned i=0;i<5;++i)textures_[i]=textures[i];values_=u;depthBias_[0]=depthBias[0];depthBias_[1]=depthBias[1];generation_=generation;valid_=true;
    }
public:
    Snapshot():generation_(0),valid_(false),profiled_(false){std::memset(&values_,0,sizeof(values_));depthBias_[0]=depthBias_[1]=0;error_[0]=0;}
    void Clear(){valid_=profiled_=false;resources_.Clear();for(unsigned i=0;i<5;++i)textures_[i].setNull();for(unsigned i=0;i<3;++i)shared_[i].setNull();fragment_.setNull();vertex_.setNull();material_.setNull();std::memset(&values_,0,sizeof(values_));depthBias_[0]=depthBias_[1]=0;generation_=0;}
    HRESULT Read(bool enabled,bool sun,bool ambient,unsigned generation,bool sourceProfileVerified){
        error_[0]=0;if(!enabled){Clear();return S_FALSE;}
        if(!sourceProfileVerified){Clear();Error("native water source profile is not verified");return E_ACCESSDENIED;}
        if(valid_&&generation_!=generation)Clear();valid_=false;
        try{ReadCurrent(sun,ambient,generation);return S_OK;}catch(const std::exception&e){Clear();Error(e.what());return E_FAIL;}catch(...){Clear();Error("native water snapshot failed");return E_FAIL;}
    }
    bool Valid(unsigned generation)const{return valid_&&generation_==generation;}const char*LastError()const{return error_;}
    const AtlasWater::Uniforms&Values()const{return values_;}const Resources&Inputs()const{return resources_;}float DepthBiasConstant()const{return depthBias_[0];}float DepthBiasSlope()const{return depthBias_[1];}
};
}
#endif
