#pragma once
#include "../AtlasModel.h"
#include "../../atlas-water/AtlasWater.h"
namespace AtlasModelDecode {
typedef AtlasView::U64 U64;
const unsigned Side=256;
struct Uniforms {float rawBounds[4],cameraFar[4],sceneToCache[4],limits[4];};
typedef char ConstantAbi[sizeof(Uniforms)==64?1:-1];
const U64 PrivateBytes=U64(Side)*Side*16+sizeof(Uniforms);
inline bool Fits(U64 caller){return caller<=AtlasView::GpuCap-PrivateBytes;}
struct ProvenMapping {
    float sourceSceneOrigin[3],sceneToCacheTranslation[3];unsigned generation;bool verified;
    ProvenMapping():generation(0),verified(false){std::memset(sourceSceneOrigin,0,12);std::memset(sceneToCacheTranslation,0,12);}
};
struct Footprint {float minX,minZ,width,height,nearClip;Footprint():minX(0),minZ(0),width(0),height(0),nearClip(0){}};
inline bool Values(const AtlasModels::Packet&p,const AtlasModels::View&v,const Footprint&f,const ProvenMapping&m,Uniforms&u){
    if(!m.verified||!p.Count()||!m.generation||m.generation!=p.Generation()||v.generation!=m.generation||!AtlasModels::Finite(m.sourceSceneOrigin,3)||!AtlasModels::Finite(m.sceneToCacheTranslation,3)||std::memcmp(m.sourceSceneOrigin,p.SourceSceneOrigin(),12))return false;
    const float rawX=f.minX-m.sceneToCacheTranslation[0],rawZ=f.minZ-m.sceneToCacheTranslation[2];
    if(!_finite(f.minX)||!_finite(f.minZ)||v.cameraScene[0]!=rawX+f.width*.5f||v.cameraScene[2]!=rawZ+f.height*.5f)return false;
    AtlasModels::View expected;if(!AtlasModels::TopDownView(v.cameraScene[0],v.cameraScene[1],v.cameraScene[2],f.width,f.height,f.nearClip,v.farClip,v.generation,expected)||std::memcmp(expected.viewProjection,v.viewProjection,64))return false;
    std::memset(&u,0,sizeof(u));u.rawBounds[0]=rawX;u.rawBounds[1]=rawZ;u.rawBounds[2]=f.width;u.rawBounds[3]=f.height;
    std::memcpy(u.cameraFar,v.cameraScene,12);u.cameraFar[3]=v.farClip;std::memcpy(u.sceneToCache,m.sceneToCacheTranslation,12);
    u.limits[0]=16.0f*1.1920928955078125e-7f;u.limits[1]=f.nearClip;
    return AtlasModels::Finite(u.rawBounds,4)&&_finite(u.rawBounds[0]+f.width)&&_finite(u.rawBounds[1]+f.height);
}
class SavedUavs {
    ID3D11DeviceContext*c_;ID3D11UnorderedAccessView*old_[3];SavedUavs(const SavedUavs&);SavedUavs&operator=(const SavedUavs&);
public:SavedUavs(ID3D11DeviceContext*c):c_(c){c_->CSGetUnorderedAccessViews(0,3,old_);}~SavedUavs(){ID3D11UnorderedAccessView*none[3]={0};c_->CSSetUnorderedAccessViews(0,3,none,NULL);c_->CSSetUnorderedAccessViews(0,3,old_,NULL);for(unsigned i=0;i<3;++i)if(old_[i])old_[i]->Release();}
};
class Layer {
    AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11ComputeShader>shader_;AtlasView::Com<ID3D11Buffer>cb_;AtlasView::Com<ID3D11Texture2D>scratch_[3];AtlasView::Com<ID3D11UnorderedAccessView>uav_[3];
    Layer(const Layer&);Layer&operator=(const Layer&);
public:Layer(){}void Clear(){for(unsigned i=0;i<3;++i){uav_[i].Reset();scratch_[i].Reset();}cb_.Reset();shader_.Reset();device_.Reset();}
    HRESULT Init(bool enabled,ID3D11Device*d,const void*code,SIZE_T bytes,U64 caller){
        if(!enabled)return S_FALSE;if(!d||device_.p||!code||!bytes||!Fits(caller)||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return E_INVALIDARG;
        const DXGI_FORMAT format[3]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};
        for(unsigned i=0;i<3;++i){UINT support=0;HRESULT hr=d->CheckFormatSupport(format[i],&support);const UINT need=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;if(FAILED(hr)||(support&need)!=need)return FAILED(hr)?hr:E_FAIL;}
        HRESULT hr=d->CreateComputeShader(code,bytes,NULL,&shader_.p);D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(Uniforms);b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&cb_.p);
        for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=Side;t.MipLevels=t.ArraySize=1;t.SampleDesc.Count=1;t.Format=format[i];t.BindFlags=D3D11_BIND_UNORDERED_ACCESS;hr=d->CreateTexture2D(&t,NULL,&scratch_[i].p);if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(scratch_[i].p,NULL,&uav_[i].p);}
        if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
    }
    // sources0..2=native packedalbedo,normal,radial;3..5=decoded existing
    // material16F,normalCoverage8,height32. Outputs must be those same3 planes.
    HRESULT Render(bool enabled,const AtlasModels::Packet&p,const AtlasModels::View&v,const Footprint&f,const ProvenMapping&m,ID3D11ShaderResourceView*const sources[6],ID3D11Texture2D*const outputs[3],U64 caller){
        if(!enabled)return S_FALSE;Uniforms u;if(!device_.p||!sources||!outputs||!Fits(caller)||!Values(p,v,f,m,u))return E_INVALIDARG;
        const DXGI_FORMAT sf[6]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};
        AtlasView::Com<ID3D11Resource>res[6];
        for(unsigned i=0;i<6;++i){if(!AtlasWater::View2D(sources[i],device_.p,sf[i],Side,Side))return E_INVALIDARG;sources[i]->GetResource(&res[i].p);}
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)if(AtlasView::Identity(res[i].p,res[j].p))return E_INVALIDARG;
        for(unsigned i=0;i<3;++i){if(!AtlasView::SameResource(sources[i+3],outputs[i]))return E_INVALIDARG;for(unsigned j=0;j<6;++j){if(j!=i+3&&AtlasView::Identity(res[j].p,outputs[i]))return E_INVALIDARG;for(unsigned k=0;k<3;++k)if(AtlasView::Identity(res[j].p,scratch_[k].p))return E_INVALIDARG;}D3D11_TEXTURE2D_DESC d;outputs[i]->GetDesc(&d);if(d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||d.MiscFlags)return E_INVALIDARG;}
        AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return E_INVALIDARG;
        for(unsigned i=0;i<6;++i)if(AtlasView::Bound(c.p,res[i].p,true))return DXGI_ERROR_INVALID_CALL;
        AtlasView::SavedOutputReads r0(c.p,outputs[0]),r1(c.p,outputs[1]),r2(c.p,outputs[2]);AtlasWater::SavedPredicate predicate(c.p);AtlasView::SavedCompute saved(c.p);SavedUavs uavs(c.p);
        c.p->UpdateSubresource(cb_.p,0,NULL,&u,0,0);c.p->CSSetShader(shader_.p,NULL,0);c.p->CSSetConstantBuffers(0,1,&cb_.p);c.p->CSSetShaderResources(0,6,sources);ID3D11UnorderedAccessView*write[3]={uav_[0].p,uav_[1].p,uav_[2].p};c.p->CSSetUnorderedAccessViews(0,3,write,NULL);c.p->Dispatch(32,32,1);
        ID3D11UnorderedAccessView*none[3]={0};ID3D11ShaderResourceView*noRead[6]={0};c.p->CSSetUnorderedAccessViews(0,3,none,NULL);c.p->CSSetShaderResources(0,6,noRead);for(unsigned i=0;i<3;++i)c.p->CopyResource(outputs[i],scratch_[i].p);
        return device_.p->GetDeviceRemovedReason();
    }
};
}
