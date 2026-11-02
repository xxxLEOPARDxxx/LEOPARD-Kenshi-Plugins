#pragma once
#include "atlas-view/AtlasRelight.h"
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreTextureUnitState.h>
#include <OgreGpuProgramManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorNode.h>
#include <kenshi/KingOfRenderThread.h>
#include <kenshi/Renderer.h>
#include "build/GsmNativeProfile.h"
namespace GsmNative {
inline bool Module(HMODULE h,DWORD timestamp,DWORD size,const GsmCodeWindow*w,unsigned count){
    __try{if(!h)return false;const BYTE*b=reinterpret_cast<const BYTE*>(h);const IMAGE_DOS_HEADER*d=reinterpret_cast<const IMAGE_DOS_HEADER*>(b);if(d->e_magic!=IMAGE_DOS_SIGNATURE||d->e_lfanew<64||d->e_lfanew>4096)return false;const IMAGE_NT_HEADERS64*n=reinterpret_cast<const IMAGE_NT_HEADERS64*>(b+d->e_lfanew);if(n->Signature!=IMAGE_NT_SIGNATURE||n->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||n->FileHeader.TimeDateStamp!=timestamp||n->OptionalHeader.SizeOfImage!=size)return false;for(unsigned i=0;i<count;++i)if(w[i].rva>size-24||std::memcmp(b+w[i].rva,w[i].code,24))return false;return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool Engine(){__try{const BYTE*b=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));const IMAGE_DOS_HEADER*d=reinterpret_cast<const IMAGE_DOS_HEADER*>(b);if(d->e_magic!=IMAGE_DOS_SIGNATURE||d->e_lfanew<64||d->e_lfanew>4096)return false;const IMAGE_NT_HEADERS64*n=reinterpret_cast<const IMAGE_NT_HEADERS64*>(b+d->e_lfanew);return n->Signature==IMAGE_NT_SIGNATURE&&n->FileHeader.TimeDateStamp==0x65d604d7&&n->OptionalHeader.SizeOfImage==0x232c000;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline bool Profile(){return Engine()&&Module(GetModuleHandleA(GsmOgreFile),GsmOgreTimestamp,GsmOgreSize,GsmOgreWindows,sizeof(GsmOgreWindows)/sizeof(GsmOgreWindows[0]))&&Module(GetModuleHandleA(GsmD3d11File),GsmD3d11Timestamp,GsmD3d11Size,GsmD3d11Windows,sizeof(GsmD3d11Windows)/sizeof(GsmD3d11Windows[0]))&&Module(GetModuleHandleA(GsmD3d11File),GsmD3d11Timestamp,GsmD3d11Size,GsmSamplerWindows,sizeof(GsmSamplerWindows)/sizeof(GsmSamplerWindows[0]));}
inline FARPROC Export(const char*name,DWORD rva){HMODULE h=GetModuleHandleA(GsmOgreFile);FARPROC p=GetProcAddress(h,name);if(!p||reinterpret_cast<const BYTE*>(p)!=reinterpret_cast<const BYTE*>(h)+rva)throw std::runtime_error("native lighting export profile differs");return p;}
inline bool TextureType(Ogre::Texture*t){__try{return t&&*reinterpret_cast<const BYTE*const*>(t)==reinterpret_cast<const BYTE*>(GetModuleHandleA(GsmD3d11File))+GsmTextureVtable;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline ID3D11ShaderResourceView*Srv(Ogre::Texture*t){if(!Profile()||!TextureType(t))throw std::runtime_error("native texture identity/profile differs");typedef ID3D11ShaderResourceView*(__fastcall*Get)(Ogre::Texture*);ID3D11ShaderResourceView*v=reinterpret_cast<Get>(reinterpret_cast<BYTE*>(GetModuleHandleA(GsmD3d11File))+GsmSrvGetter)(t);if(!v)throw std::runtime_error("native texture SRV missing");return v;}
inline Ogre::Vector4 Shared(const char*set,const char*name,unsigned count){Ogre::GpuSharedParametersPtr p=Ogre::GpuProgramManager::getSingleton().getSharedParameters(set);const Ogre::GpuConstantDefinition&d=p->getConstantDefinition(name);if(d.arraySize!=1||d.constType!=(count==1?Ogre::GCT_FLOAT1:count==3?Ogre::GCT_FLOAT3:Ogre::GCT_FLOAT4))throw std::runtime_error("native lighting shared type differs");const float*v=static_cast<const Ogre::GpuSharedParameters*>(p.get())->getFloatPointer(d.physicalIndex);Ogre::Vector4 r(0,0,0,0);for(unsigned i=0;i<count;++i){if(!_finite(v[i]))throw std::runtime_error("native lighting nonfinite");r[i]=v[i];}return r;}
inline void Put(float*out,const Ogre::Vector4&v){out[0]=v.x;out[1]=v.y;out[2]=v.z;out[3]=v.w;}
inline D3D11_TEXTURE_ADDRESS_MODE Address(Ogre::TextureUnitState::TextureAddressingMode m){switch(m){case Ogre::TextureUnitState::TAM_WRAP:return D3D11_TEXTURE_ADDRESS_WRAP;case Ogre::TextureUnitState::TAM_MIRROR:return D3D11_TEXTURE_ADDRESS_MIRROR;case Ogre::TextureUnitState::TAM_CLAMP:return D3D11_TEXTURE_ADDRESS_CLAMP;case Ogre::TextureUnitState::TAM_BORDER:return D3D11_TEXTURE_ADDRESS_BORDER;default:throw std::runtime_error("native sampler address unsupported");}}
class Light {
    Ogre::TexturePtr textures_[3];AtlasView::Com<ID3D11ShaderResourceView>ownedViews_[3];AtlasView::Com<ID3D11SamplerState>samplers_[3];
public:
    AtlasView::Com<ID3D11Device>device;
    ID3D11ShaderResourceView*views[3];ID3D11SamplerState*samplers[3];
    Light(){std::memset(views,0,sizeof(views));std::memset(samplers,0,sizeof(samplers));}
    void Clear(){for(unsigned i=0;i<3;++i){views[i]=NULL;samplers[i]=NULL;samplers_[i].Reset();ownedViews_[i].Reset();textures_[i].setNull();}device.Reset();}
    void Init(){
        if(!Profile())throw std::runtime_error("loaded native light profile refused");
        Ogre::MaterialPtr material=Ogre::MaterialManager::getSingleton().getByName("Main_Lighting_NoShadow");if(material.isNull()||material->getNumTechniques()!=1||material->getTechnique(0)->getNumPasses()!=1)throw std::runtime_error("native no-shadow light pass missing");Ogre::Pass*p=material->getTechnique(0)->getPass(0);if(p->getNumTextureUnitStates()!=6||p->getFragmentProgramName()!="Deferred_MainLight_NoShadow"||p->getVertexProgramName()!="Deferred_VP")throw std::runtime_error("native light texture/program layout differs");
        for(unsigned i=0;i<3;++i){Ogre::TextureUnitState*unit=p->getTextureUnitState(i+3);textures_[i]=unit->_getTexturePtr();if(textures_[i].isNull())throw std::runtime_error("native light texture unloaded");views[i]=Srv(textures_[i].get());ownedViews_[i].p=views[i];views[i]->AddRef();if(!device.p)views[i]->GetDevice(&device.p);if(!AtlasView::SameDevice(views[i],device.p)||!AtlasView::LightView(views[i],device.p,i!=0,i==0?1:i==1?4:8))throw std::runtime_error("native light texture descriptor differs");if(unit->getTextureCompareEnabled())throw std::runtime_error("native comparison sampler unsupported");
            D3D11_SAMPLER_DESC s={};const Ogre::TextureUnitState::UVWAddressingMode&a=unit->getTextureAddressingMode();s.AddressU=Address(a.u);s.AddressV=Address(a.v);s.AddressW=Address(a.w);s.ComparisonFunc=D3D11_COMPARISON_NEVER;s.MaxLOD=D3D11_FLOAT32_MAX;s.MaxAnisotropy=unit->getTextureAnisotropy();if(s.MaxAnisotropy<1)s.MaxAnisotropy=1;if(s.MaxAnisotropy>16)s.MaxAnisotropy=16;
            Ogre::FilterOptions min=unit->getTextureFiltering(Ogre::FT_MIN),mag=unit->getTextureFiltering(Ogre::FT_MAG),mip=unit->getTextureFiltering(Ogre::FT_MIP);s.Filter=(min==Ogre::FO_ANISOTROPIC||mag==Ogre::FO_ANISOTROPIC||mip==Ogre::FO_ANISOTROPIC)?D3D11_FILTER_ANISOTROPIC:D3D11_ENCODE_BASIC_FILTER(min==Ogre::FO_LINEAR?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mag==Ogre::FO_LINEAR?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,mip==Ogre::FO_LINEAR?D3D11_FILTER_TYPE_LINEAR:D3D11_FILTER_TYPE_POINT,0);const Ogre::ColourValue&b=unit->getTextureBorderColour();s.BorderColor[0]=b.r;s.BorderColor[1]=b.g;s.BorderColor[2]=b.b;s.BorderColor[3]=b.a;
            HRESULT hr=device.p->CreateSamplerState(&s,&samplers_[i].p);if(FAILED(hr))throw std::runtime_error("native-configured sampler creation failed");samplers[i]=samplers_[i].p;
        }
    }
    AtlasView::Uniforms Snapshot(bool sun,bool ambient){
        if(!au||!au->render||!Profile())throw std::runtime_error("native lighting world not ready");
        AtlasView::Uniforms u={};Put(u.referenceSide,Shared("SharedDeferredParams","camera",3));Put(u.sunDirection,Shared("SharedSkyParams","sunDirection",3));Put(u.worldOffset,Shared("SharedWaterParams","worldOffset",3));Put(u.sunColour,Shared("SharedSkyParams","sunColour",4));Put(u.envColour,Shared("AmbientParams","envColour",4));Put(u.ambientParams,Shared("AmbientParams","ambientParams",4));Put(u.worldSize,Shared("AmbientParams","worldSize",4));u.viewDirection[1]=1;
        typedef Ogre::CompositorNode*(__fastcall*Find)(const Ogre::CompositorWorkspace*,Ogre::IdString,bool);typedef Ogre::TexturePtr*(__fastcall*Texture)(const Ogre::CompositorNode*,Ogre::TexturePtr*,Ogre::IdString,size_t);
        Find find=reinterpret_cast<Find>(Export(GsmNativeFindNodeName,GsmNativeFindNodeRva));Texture texture=reinterpret_cast<Texture>(Export(GsmNativeNodeTextureName,GsmNativeNodeTextureRva));Ogre::CompositorWorkspace*w=au->render->workspace;if(!w)throw std::runtime_error("native HDR workspace unavailable");Ogre::CompositorNode*n=find(w,Ogre::IdString("Resolve_HDR"),false);if(!n)throw std::runtime_error("native HDR resolve missing");Ogre::TexturePtr adapted;if(texture(n,&adapted,Ogre::IdString("rt_adaptedLum"),0)!=&adapted||adapted.isNull()||adapted->getWidth()!=1||adapted->getHeight()!=1||adapted->getDepth()!=1||adapted->getTextureType()!=Ogre::TEX_TYPE_2D||adapted->getFormat()!=Ogre::PF_FLOAT32_R)throw std::runtime_error("native exposure descriptor differs");float logarithm=0;Ogre::PixelBox pixel(1,1,1,Ogre::PF_FLOAT32_R,&logarithm);adapted->getBuffer(0,0)->blitToMemory(Ogre::Box(0,0,0,1,1,1),pixel);u.exposure[0]=Shared("HDRSettings","EXPOSURE_KEY",1).x;double lum=std::exp(double(logarithm));if(!_finite(logarithm)||!_finite(lum)||lum<=0||lum>FLT_MAX||u.exposure[0]<=0)throw std::runtime_error("native exposure invalid");u.exposure[1]=float(lum);if(!sun)u.sunColour[3]=0;if(!ambient)u.envColour[3]=0;
        for(unsigned i=0;i<3;++i){ID3D11ShaderResourceView*v=Srv(textures_[i].get());if(v!=views[i])throw std::runtime_error("native light SRV changed; reload required");}
        return u;
    }
};
}
