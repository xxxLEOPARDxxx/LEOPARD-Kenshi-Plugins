#pragma once
#include "../NativeLocalLightSnapshot.h"
#include "../../atlas-water/AtlasWater.h"
namespace AtlasLocalLights {
typedef unsigned __int64 U64;
static const U64&Cap=AtlasView::GpuCap;
const unsigned Batch=32,MaxBlock=256;
struct GpuLight {float positionRange[4],directionType[4],diffusePower[4],spot[4];};
struct Uniforms {float tileBounds[4],sourceWindow[4],dispatchWindow[4],sceneToCache[4],parallelView[4];GpuLight lights[Batch];};
typedef char UniformAbi[sizeof(Uniforms)==2128?1:-1];
const U64 PrivateBytes=U64(MaxBlock)*MaxBlock*8+sizeof(Uniforms);
inline bool Fits(U64 caller){return caller<=Cap-PrivateBytes;}
struct ProvenMapping {
    float sourceSceneOrigin[3],sceneToCacheTranslation[3];LONG generation;bool verified;
    ProvenMapping():generation(0),verified(false){std::memset(sourceSceneOrigin,0,sizeof(sourceSceneOrigin));std::memset(sceneToCacheTranslation,0,sizeof(sceneToCacheTranslation));}
    bool Matches(const NativeLocalLights::Snapshot&s,LONG g)const{if(!verified||generation!=g||!s.Valid(g))return false;for(unsigned i=0;i<3;++i)if(!_finite(sceneToCacheTranslation[i])||!_finite(sourceSceneOrigin[i])||sourceSceneOrigin[i]!=s.Info().SourceSceneOrigin[i])return false;return true;}
};
struct Region {
    float minX,minZ,spanX,spanZ;unsigned sourceSide,originX,originZ;
    Region():minX(0),minZ(0),spanX(0),spanZ(0),sourceSide(0),originX(0),originZ(0){}
    bool Valid(unsigned w,unsigned h)const{return _finite(minX)&&_finite(minZ)&&_finite(spanX)&&_finite(spanZ)&&spanX>0&&spanZ>0&&_finite(minX+spanX)&&_finite(minZ+spanZ)&&(sourceSide==256||sourceSide==512||sourceSide==1024||sourceSide==2048)&&w&&h&&w<=MaxBlock&&h<=MaxBlock&&originX<=sourceSide&&originZ<=sourceSide&&w<=sourceSide-originX&&h<=sourceSide-originZ;}
};
inline bool Pack(const NativeLocalLights::LightValue&v,const ProvenMapping&m,GpuLight&out){
    if(!NativeLocalLights::LightValid(v)||!_finite(v.effectivePower)||v.effectivePower<0)return false;
    std::memset(&out,0,sizeof(out));for(unsigned i=0;i<3;++i){out.positionRange[i]=v.worldPosition[i]+m.sceneToCacheTranslation[i];if(!_finite(out.positionRange[i]))return false;out.directionType[i]=v.worldDirection[i];out.diffusePower[i]=v.diffuse[i];out.spot[i]=v.spot[i];}
    out.positionRange[3]=v.range;out.directionType[3]=float(v.type);out.diffusePower[3]=v.effectivePower;return true;
}
inline bool Intersects(const GpuLight&l,const Region&r,unsigned w,unsigned h){
    const double x0=double(r.minX)+double(r.originX)/r.sourceSide*r.spanX,z0=double(r.minZ)+double(r.originZ)/r.sourceSide*r.spanZ;
    const double x1=x0+double(w)/r.sourceSide*r.spanX,z1=z0+double(h)/r.sourceSide*r.spanZ;
    const double x=l.positionRange[0],z=l.positionRange[2],dx=x<x0?x0-x:x>x1?x-x1:0,dz=z<z0?z0-z:z>z1?z-z1:0;
    return dx*dx+dz*dz<=double(l.positionRange[3])*l.positionRange[3];
}
class Layer {
    AtlasView::Com<ID3D11Device> device_;AtlasView::Com<ID3D11ComputeShader> shader_;AtlasView::Com<ID3D11Buffer> cb_;AtlasView::Com<ID3D11Texture2D> scratch_;AtlasView::Com<ID3D11UnorderedAccessView> uav_;unsigned batches_,lights_;
    Layer(const Layer&);Layer&operator=(const Layer&);
    void Dispatch(ID3D11DeviceContext*c,ID3D11ShaderResourceView*const source[4],ID3D11Texture2D*hdr,Uniforms&u,unsigned w,unsigned h,unsigned count){
        u.dispatchWindow[0]=float(w);u.dispatchWindow[1]=float(h);u.dispatchWindow[2]=float(count);u.dispatchWindow[3]=0;
        c->UpdateSubresource(cb_.p,0,NULL,&u,0,0);c->CSSetShaderResources(0,4,source);c->CSSetUnorderedAccessViews(0,1,&uav_.p,NULL);c->Dispatch((w+7)/8,(h+7)/8,1);
        ID3D11UnorderedAccessView*noUav=NULL;c->CSSetUnorderedAccessViews(0,1,&noUav,NULL);ID3D11ShaderResourceView*noView=NULL;c->CSSetShaderResources(0,1,&noView);
        D3D11_BOX box={0,0,0,w,h,1};c->CopySubresourceRegion(hdr,0,0,0,0,scratch_.p,0,&box);++batches_;lights_+=count;
    }
public:
    Layer():batches_(0),lights_(0){}
    void Clear(){uav_.Reset();scratch_.Reset();cb_.Reset();shader_.Reset();device_.Reset();batches_=lights_=0;}
    unsigned LastBatches()const{return batches_;}unsigned LastLights()const{return lights_;}
    HRESULT Init(ID3D11Device*d,const void*code,SIZE_T bytes,U64 callerOwned){
        if(device_.p||!d||!code||!bytes||!Fits(callerOwned)||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return E_INVALIDARG;
        UINT support=0;HRESULT hr=d->CheckFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT,&support);const UINT need=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;
        if(FAILED(hr)||(support&need)!=need)return FAILED(hr)?hr:E_FAIL;
        hr=d->CreateComputeShader(code,bytes,NULL,&shader_.p);D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(Uniforms);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&cb_.p);
        D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=MaxBlock;t.MipLevels=t.ArraySize=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
        if(SUCCEEDED(hr))hr=d->CreateTexture2D(&t,NULL,&scratch_.p);if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(scratch_.p,NULL,&uav_.p);if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
    }
    // Source0 MUST be the same caller-owned HDR texture;1/2/3 are decoded GST
    // material16F / normalCoverageRGBA8 / absolute heightR32F. No game calls.
    HRESULT Render(bool enabled,const NativeLocalLights::Snapshot&snapshot,LONG generation,const ProvenMapping&mapping,const Region&region,ID3D11ShaderResourceView*const source[4],ID3D11Texture2D*hdr,U64 callerOwned){
        if(!enabled)return S_FALSE;
        if(!mapping.Matches(snapshot,generation)||snapshot.Count()>NativeLocalLights::Limit||!Fits(callerOwned)||!device_.p||!source||!AtlasView::SameDevice(hdr,device_.p))return E_INVALIDARG;
        batches_=lights_=0;D3D11_TEXTURE2D_DESC d;hdr->GetDesc(&d);
        if(!region.Valid(d.Width,d.Height)||d.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||d.MipLevels!=1||d.ArraySize!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||d.MiscFlags)return E_INVALIDARG;
        if(!AtlasWater::View2D(source[0],device_.p,d.Format,d.Width,d.Height)||!AtlasView::SameResource(source[0],hdr)||!AtlasWater::View2D(source[1],device_.p,d.Format,region.sourceSide,region.sourceSide)||!AtlasWater::View2D(source[2],device_.p,DXGI_FORMAT_R8G8B8A8_UNORM,region.sourceSide,region.sourceSide)||!AtlasWater::View2D(source[3],device_.p,DXGI_FORMAT_R32_FLOAT,region.sourceSide,region.sourceSide))return E_INVALIDARG;
        unsigned intersecting=0;for(unsigned i=0;i<snapshot.Count();++i){const NativeLocalLights::LightValue*v=snapshot.Light(i);GpuLight light;if(!v||!Pack(*v,mapping,light))return E_INVALIDARG;if(light.diffusePower[3]>0&&Intersects(light,region,d.Width,d.Height))++intersecting;}
        if(!intersecting)return S_FALSE;
        AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||AtlasView::Bound(c.p,hdr,true))return DXGI_ERROR_INVALID_CALL;
        for(unsigned i=0;i<4;++i){AtlasView::Com<ID3D11Resource>r;source[i]->GetResource(&r.p);if((i&&AtlasView::Identity(r.p,hdr))||AtlasView::Identity(r.p,scratch_.p)||AtlasView::Bound(c.p,r.p,true))return DXGI_ERROR_INVALID_CALL;}
        AtlasView::SavedOutputReads reads(c.p,hdr);AtlasWater::SavedPredicate predicate(c.p);AtlasView::SavedCompute saved(c.p);
        c.p->CSSetShader(shader_.p,NULL,0);c.p->CSSetConstantBuffers(0,1,&cb_.p);
        Uniforms u={};u.tileBounds[0]=region.minX;u.tileBounds[1]=region.minZ;u.tileBounds[2]=region.spanX;u.tileBounds[3]=region.spanZ;u.sourceWindow[0]=float(region.sourceSide);u.sourceWindow[1]=float(region.originX);u.sourceWindow[2]=float(region.originZ);for(unsigned i=0;i<3;++i)u.sceneToCache[i]=mapping.sceneToCacheTranslation[i];u.parallelView[1]=1;
        unsigned count=0;for(unsigned i=0;i<snapshot.Count();++i){GpuLight light;Pack(*snapshot.Light(i),mapping,light);if(light.diffusePower[3]<=0||!Intersects(light,region,d.Width,d.Height))continue;u.lights[count++]=light;if(count==Batch){Dispatch(c.p,source,hdr,u,d.Width,d.Height,count);count=0;}}
        if(count)Dispatch(c.p,source,hdr,u,d.Width,d.Height,count);return device_.p->GetDeviceRemovedReason();
    }
};
}
