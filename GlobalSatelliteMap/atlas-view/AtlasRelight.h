#pragma once
// C++03 / SDK7.1. Private CS dispatch; no Ogre/native ABI calls. Root owns the
// verified native-light snapshot and the Ogre output texture identity/lifetime.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <float.h>
#include "AtlasReader.h"
namespace AtlasView {
template<class T> class Com {
    Com(const Com&);Com&operator=(const Com&);
public:T*p;Com():p(NULL){}~Com(){Reset();}void Reset(){if(p){p->Release();p=NULL;}}T*Detach(){T*q=p;p=NULL;return q;}
};
struct Uniforms {
    float tileBounds[4],referenceSide[4],sunDirection[4],worldOffset[4],sunColour[4],envColour[4],ambientParams[4],worldSize[4],exposure[4],viewDirection[4],dispatchWindow[4];
    bool Valid(unsigned side)const{
        const float *f=reinterpret_cast<const float*>(this);for(unsigned i=0;i<sizeof(*this)/sizeof(float);++i)if(!_finite(f[i]))return false;
        const float dl=sunDirection[0]*sunDirection[0]+sunDirection[1]*sunDirection[1]+sunDirection[2]*sunDirection[2];
        const float vl=viewDirection[0]*viewDirection[0]+viewDirection[1]*viewDirection[1]+viewDirection[2]*viewDirection[2];
        return referenceSide[3]==float(side)&&tileBounds[2]>0&&tileBounds[3]>0&&dl>1e-8f&&vl>1e-8f&&_finite(dl)&&_finite(vl)&&exposure[0]>0&&exposure[1]>0;
    }
};
typedef char UniformSize[(sizeof(Uniforms)==UniformGpuBytes)?1:-1];
inline bool Identity(IUnknown*a,IUnknown*b){if(!a||!b)return false;Com<IUnknown>x,y;return SUCCEEDED(a->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void**>(&x.p)))&&SUCCEEDED(b->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void**>(&y.p)))&&x.p==y.p;}
inline bool SameDevice(ID3D11DeviceChild*object,ID3D11Device*device){if(!object)return false;Com<ID3D11Device>d;object->GetDevice(&d.p);return Identity(d.p,device);}
inline bool SameResource(ID3D11View*view,ID3D11Resource*resource){if(!view)return false;Com<ID3D11Resource>r;view->GetResource(&r.p);return Identity(r.p,resource);}
// Writable aliases are refused before mutation. Read-only owned-output aliases
// can be scoped below without touching hidden append counters/OM state.
inline bool Bound(ID3D11DeviceContext*c,ID3D11Resource*r,bool writesOnly=false){
    ID3D11ShaderResourceView*views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    for(unsigned stage=0;stage<(writesOnly?0U:6U);++stage){
        std::memset(views,0,sizeof(views));
        switch(stage){case 0:c->VSGetShaderResources(0,128,views);break;case 1:c->HSGetShaderResources(0,128,views);break;case 2:c->DSGetShaderResources(0,128,views);break;case 3:c->GSGetShaderResources(0,128,views);break;case 4:c->PSGetShaderResources(0,128,views);break;default:c->CSGetShaderResources(0,128,views);break;}
        bool found=false;for(unsigned i=0;i<128;++i)if(views[i]){found=SameResource(views[i],r)||found;views[i]->Release();}if(found)return true;
    }
    ID3D11UnorderedAccessView*u[8]={0};c->CSGetUnorderedAccessViews(0,8,u);bool found=false;
    for(unsigned i=0;i<8;++i)if(u[i]){found=SameResource(u[i],r)||found;u[i]->Release();}if(found)return true;
    c->OMGetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,u);
    for(unsigned i=0;i<8;++i)if(u[i]){found=SameResource(u[i],r)||found;u[i]->Release();}if(found)return true;
    ID3D11RenderTargetView*rt[8]={0};Com<ID3D11DepthStencilView>ds;c->OMGetRenderTargets(8,rt,&ds.p);
    for(unsigned i=0;i<8;++i)if(rt[i]){found=SameResource(rt[i],r)||found;rt[i]->Release();}
    return SameResource(ds.p,r)||found;
}
class SavedOutputReads {
    ID3D11DeviceContext*c_;ID3D11ShaderResourceView*views_[6][128];
    SavedOutputReads(const SavedOutputReads&);SavedOutputReads&operator=(const SavedOutputReads&);
    void Set(unsigned stage,unsigned slot,ID3D11ShaderResourceView*view){
        switch(stage){case 0:c_->VSSetShaderResources(slot,1,&view);break;case 1:c_->HSSetShaderResources(slot,1,&view);break;case 2:c_->DSSetShaderResources(slot,1,&view);break;case 3:c_->GSSetShaderResources(slot,1,&view);break;case 4:c_->PSSetShaderResources(slot,1,&view);break;default:c_->CSSetShaderResources(slot,1,&view);break;}
    }
public:
    SavedOutputReads(ID3D11DeviceContext*c,ID3D11Resource*ownedOutput):c_(c){
        std::memset(views_,0,sizeof(views_));
        c_->VSGetShaderResources(0,128,views_[0]);c_->HSGetShaderResources(0,128,views_[1]);c_->DSGetShaderResources(0,128,views_[2]);c_->GSGetShaderResources(0,128,views_[3]);c_->PSGetShaderResources(0,128,views_[4]);c_->CSGetShaderResources(0,128,views_[5]);
        for(unsigned stage=0;stage<6;++stage)for(unsigned slot=0;slot<128;++slot)if(views_[stage][slot]){
            if(SameResource(views_[stage][slot],ownedOutput))Set(stage,slot,NULL);
            else{views_[stage][slot]->Release();views_[stage][slot]=NULL;}
        }
    }
    ~SavedOutputReads(){for(unsigned stage=0;stage<6;++stage)for(unsigned slot=0;slot<128;++slot)if(views_[stage][slot]){Set(stage,slot,views_[stage][slot]);views_[stage][slot]->Release();}}
};
struct Cell {
    Com<ID3D11Texture2D>texture[3];Com<ID3D11ShaderResourceView>view[3];unsigned side;
    Cell():side(0){}
    void Clear(){for(unsigned p=0;p<3;++p){view[p].Reset();texture[p].Reset();}side=0;}
    // Must reserve three planes AND caller output AND scratch in Budget first.
    HRESULT Upload(ID3D11Device*device,const ReadRequest&r){
        if(!device||side||r.error!=ERROR_SUCCESS||r.level<2||r.level>=6)return E_INVALIDARG;
        const DXGI_FORMAT formats[3]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};
        for(unsigned p=0;p<3;++p){
            if(r.plane[p].size()!=PlaneBytes(r.level,p)){Clear();return E_INVALIDARG;}
            UINT support=0;HRESULT hr=device->CheckFormatSupport(formats[p],&support);
            if(FAILED(hr)||(support&(D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_LOAD))!=(D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_LOAD)){Clear();return FAILED(hr)?hr:E_FAIL;}
            D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=Side(r.level);d.MipLevels=d.ArraySize=1;d.Format=formats[p];d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA data={};data.pSysMem=&r.plane[p][0];data.SysMemPitch=d.Width*Stride(p);
            hr=device->CreateTexture2D(&d,&data,&texture[p].p);if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture[p].p,NULL,&view[p].p);if(FAILED(hr)){Clear();return hr;}
        }side=Side(r.level);return S_OK;
    }
};
inline bool LightView(ID3D11ShaderResourceView*v,ID3D11Device*device,bool cube,unsigned minMips){
    if(!SameDevice(v,device))return false;D3D11_SHADER_RESOURCE_VIEW_DESC s;v->GetDesc(&s);
    if(s.ViewDimension!=(cube?D3D11_SRV_DIMENSION_TEXTURECUBE:D3D11_SRV_DIMENSION_TEXTURE2D))return false;
    switch(s.Format){case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:case DXGI_FORMAT_R16G16B16A16_FLOAT:case DXGI_FORMAT_R32G32B32A32_FLOAT:case DXGI_FORMAT_BC1_UNORM:case DXGI_FORMAT_BC1_UNORM_SRGB:case DXGI_FORMAT_BC2_UNORM:case DXGI_FORMAT_BC2_UNORM_SRGB:case DXGI_FORMAT_BC3_UNORM:case DXGI_FORMAT_BC3_UNORM_SRGB:break;default:return false;}
    Com<ID3D11Resource>r;v->GetResource(&r.p);Com<ID3D11Texture2D>t;if(FAILED(r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p))))return false;
    D3D11_TEXTURE2D_DESC d;t.p->GetDesc(&d);
    unsigned first=cube?s.TextureCube.MostDetailedMip:s.Texture2D.MostDetailedMip,count=cube?s.TextureCube.MipLevels:s.Texture2D.MipLevels;
    if(first>=d.MipLevels)return false;if(count==UINT(-1))count=d.MipLevels-first;
    return d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&d.ArraySize==(cube?6U:1U)&&(!cube||(d.MiscFlags&D3D11_RESOURCE_MISC_TEXTURECUBE))&&(d.BindFlags&D3D11_BIND_SHADER_RESOURCE)&&count>=minMips&&count<=d.MipLevels-first;
}
class SavedCompute {
    ID3D11DeviceContext*c_;ID3D11ComputeShader*shader_;ID3D11ClassInstance*classes_[256];UINT count_;
    ID3D11Buffer*buffer_;ID3D11ShaderResourceView*views_[6];ID3D11SamplerState*samplers_[3];ID3D11UnorderedAccessView*uav_;
    SavedCompute(const SavedCompute&);SavedCompute&operator=(const SavedCompute&);
public:
    SavedCompute(ID3D11DeviceContext*c):c_(c),shader_(NULL),count_(256),buffer_(NULL),uav_(NULL){
        std::memset(classes_,0,sizeof(classes_));std::memset(views_,0,sizeof(views_));std::memset(samplers_,0,sizeof(samplers_));
        c_->CSGetShader(&shader_,classes_,&count_);c_->CSGetConstantBuffers(0,1,&buffer_);c_->CSGetShaderResources(0,6,views_);c_->CSGetSamplers(0,3,samplers_);c_->CSGetUnorderedAccessViews(0,1,&uav_);
    }
    ~SavedCompute(){
        // Unbind our writable scratch before restoring native input bindings.
        ID3D11UnorderedAccessView*none=NULL;c_->CSSetUnorderedAccessViews(0,1,&none,NULL);
        c_->CSSetShader(shader_,classes_,count_);c_->CSSetConstantBuffers(0,1,&buffer_);c_->CSSetShaderResources(0,6,views_);c_->CSSetSamplers(0,3,samplers_);c_->CSSetUnorderedAccessViews(0,1,&uav_,NULL);
        if(shader_)shader_->Release();for(unsigned i=0;i<count_;++i)if(classes_[i])classes_[i]->Release();if(buffer_)buffer_->Release();
        for(unsigned i=0;i<6;++i)if(views_[i])views_[i]->Release();for(unsigned i=0;i<3;++i)if(samplers_[i])samplers_[i]->Release();if(uav_)uav_->Release();
    }
};
class Relighter {
    Com<ID3D11Device>device_;Com<ID3D11ComputeShader>shader_;Com<ID3D11Buffer>constants_;
    Com<ID3D11Texture2D>scratch_;Com<ID3D11UnorderedAccessView>uav_;unsigned side_,scratchSide_;
    Relighter(const Relighter&);Relighter&operator=(const Relighter&);
public:
    Relighter():side_(0),scratchSide_(0){}
    void Clear(){uav_.Reset();scratch_.Reset();constants_.Reset();shader_.Reset();device_.Reset();side_=scratchSide_=0;}
    // One reusable scratch. Clear/reinitialize only on cache/mip configuration
    // changes, never per frame. Account previous live allocations before calling.
    HRESULT Init(ID3D11Device*d,const void*code,SIZE_T bytes,unsigned scratchSide,U64 cellBytes){
        if(device_.p||!d||!code||!bytes||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0||scratchSide<256||scratchSide>2048||(scratchSide&(scratchSide-1))||!Fits(cellBytes,scratchSide))return E_INVALIDARG;
        HRESULT hr=d->CreateComputeShader(code,bytes,NULL,&shader_.p);
        D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(Uniforms);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&constants_.p);
        D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=ScratchSide(scratchSide);t.MipLevels=t.ArraySize=1;t.Format=DXGI_FORMAT_R8G8B8A8_UNORM;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
        if(SUCCEEDED(hr))hr=d->CreateTexture2D(&t,NULL,&scratch_.p);if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(scratch_.p,NULL,&uav_.p);
        if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();side_=scratchSide;scratchSide_=t.Width;return S_OK;
    }
    HRESULT Render(const Cell&cell,const Uniforms&values,ID3D11ShaderResourceView*const light[3],ID3D11SamplerState*const sampler[3],ID3D11Texture2D*output){
        if(!device_.p||!cell.side||cell.side>side_||!values.Valid(cell.side)||!light||!sampler||!SameDevice(output,device_.p))return E_INVALIDARG;
        for(unsigned p=0;p<3;++p)if(!SameDevice(cell.view[p].p,device_.p)||!SameDevice(sampler[p],device_.p)||!LightView(light[p],device_.p,p!=0,p==0?1:(p==1?4:8)))return E_INVALIDARG;
        D3D11_TEXTURE2D_DESC d;output->GetDesc(&d);
        if(d.Width!=cell.side||d.Height!=cell.side||d.MipLevels!=1||d.ArraySize!=1||d.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||d.SampleDesc.Count!=1||d.SampleDesc.Quality!=0||d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||d.MiscFlags)return E_INVALIDARG;
        Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);
        if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||Bound(c.p,output,true))return DXGI_ERROR_INVALID_CALL;
        for(unsigned p=0;p<3;++p){Com<ID3D11Resource>r;light[p]->GetResource(&r.p);if(SameResource(light[p],output)||Identity(cell.texture[p].p,output)||Bound(c.p,r.p,true))return DXGI_ERROR_INVALID_CALL;}
        // Caller guarantees this is its verified owned Ogre output, not an
        // arbitrary native target. Restore only matching read bindings exactly.
        SavedOutputReads outputReads(c.p,output);
        // Native predication must not silently suppress this private dispatch or
        // its copy. Restore the exact predicate/value after BOTH commands.
        Com<ID3D11Predicate>predicate;BOOL predicateValue=FALSE;c.p->GetPredication(&predicate.p,&predicateValue);c.p->SetPredication(NULL,FALSE);
        {
            SavedCompute saved(c.p);ID3D11ShaderResourceView*views[6]={cell.view[0].p,cell.view[1].p,cell.view[2].p,light[0],light[1],light[2]};
            c.p->CSSetShader(shader_.p,NULL,0);c.p->CSSetConstantBuffers(0,1,&constants_.p);c.p->CSSetShaderResources(0,6,views);c.p->CSSetSamplers(0,3,sampler);
            for(unsigned z=0;z<cell.side;z+=scratchSide_)for(unsigned x=0;x<cell.side;x+=scratchSide_){
                unsigned w=cell.side-x<scratchSide_?cell.side-x:scratchSide_,h=cell.side-z<scratchSide_?cell.side-z:scratchSide_;
                Uniforms block=values;block.dispatchWindow[0]=float(x);block.dispatchWindow[1]=float(z);block.dispatchWindow[2]=float(w);block.dispatchWindow[3]=float(h);
                c.p->UpdateSubresource(constants_.p,0,NULL,&block,0,0);c.p->CSSetUnorderedAccessViews(0,1,&uav_.p,NULL);c.p->Dispatch((w+7)/8,(h+7)/8,1);
                ID3D11UnorderedAccessView*none=NULL;c.p->CSSetUnorderedAccessViews(0,1,&none,NULL);
                D3D11_BOX box={0,0,0,w,h,1};c.p->CopySubresourceRegion(output,0,x,z,0,scratch_.p,0,&box);
            }
        }
        c.p->SetPredication(predicate.p,predicateValue);
        return device_.p->GetDeviceRemovedReason();
    }
};
}
