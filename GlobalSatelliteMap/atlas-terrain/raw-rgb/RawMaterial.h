#pragma once
#include "../../atlas-view/AtlasRelight.h"
namespace TerrainRaw {
typedef AtlasView::U64 U64;
const U64 Limit=90ULL*1024*1024,PackedBytes=1048576ULL,PrivateBytes=PackedBytes+262144ULL;
inline bool Fits(U64 other){return other<=Limit-PrivateBytes&&other<=AtlasView::GpuCap-PrivateBytes;}
// Resource owner only; the caller serializes render-thread commands and owns
// complete pipeline save/restore, shader/source proof and current input values.
class Targets{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11Texture2D>tex_[4];AtlasView::Com<ID3D11ShaderResourceView>view_[3];AtlasView::Com<ID3D11RenderTargetView>rt_[3];AtlasView::Com<ID3D11DepthStencilView>depth_;
 Targets(const Targets&);Targets&operator=(const Targets&);
public:
 Targets(){}
 void Clear(){depth_.Reset();for(unsigned i=0;i<3;++i){rt_[i].Reset();view_[i].Reset();}for(unsigned i=0;i<4;++i)tex_[i].Reset();device_.Reset();}
 U64 OwnedGpuBytes()const{return device_.p?PrivateBytes:0;}
 ID3D11Texture2D*Texture(unsigned i)const{return device_.p&&i<4?tex_[i].p:NULL;}
 ID3D11ShaderResourceView*View(unsigned i)const{return device_.p&&i<3?view_[i].p:NULL;}
 ID3D11RenderTargetView*Target(unsigned i)const{return device_.p&&i<3?rt_[i].p:NULL;}
 ID3D11DepthStencilView*Depth()const{return device_.p?depth_.p:NULL;}
 HRESULT Init(ID3D11Device*d,U64 other){if(!d||device_.p)return E_INVALIDARG;if(!Fits(other))return E_OUTOFMEMORY;HRESULT hr=S_OK;for(unsigned i=0;i<4&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=256;t.MipLevels=t.ArraySize=1;t.SampleDesc.Count=1;t.Format=i==0?DXGI_FORMAT_R16G16B16A16_FLOAT:i==1?DXGI_FORMAT_R8G8B8A8_UNORM:i==2?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_D32_FLOAT;t.BindFlags=i==3?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;hr=d->CreateTexture2D(&t,NULL,&tex_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateShaderResourceView(tex_[i].p,NULL,&view_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateRenderTargetView(tex_[i].p,NULL,&rt_[i].p);}if(SUCCEEDED(hr))hr=d->CreateDepthStencilView(tex_[3].p,NULL,&depth_.p);if(SUCCEEDED(hr))hr=d->GetDeviceRemovedReason();if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;}
};
}
