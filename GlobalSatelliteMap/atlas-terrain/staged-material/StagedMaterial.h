#pragma once
// Resource/extra-binding owners for the isolated fixed256 ordered shader proof.
// No general renderer, streaming bank or native material winner is implied.
#include "../raster/FixedTerrainRaster.h"
namespace StagedMaterial {
typedef AtlasView::U64 U64;
const U64 CarryBytes=4194304ULL,PrivateBytes=CarryBytes+16;
inline bool Fits(U64 other){return other<=CompactMaterial::Limit-PrivateBytes&&other<=AtlasView::GpuCap-PrivateBytes;}
class Carry{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11Texture2D>texture_[2][2];AtlasView::Com<ID3D11ShaderResourceView>view_[2][2];AtlasView::Com<ID3D11RenderTargetView>target_[2][2];AtlasView::Com<ID3D11Buffer>constants_;
 Carry(const Carry&);Carry&operator=(const Carry&);
public:
 Carry(){}
 void Clear(){for(unsigned set=0;set<2;++set)for(unsigned plane=0;plane<2;++plane){target_[set][plane].Reset();view_[set][plane].Reset();texture_[set][plane].Reset();}constants_.Reset();device_.Reset();}
 U64 OwnedGpuBytes()const{return device_.p?PrivateBytes:0;}
 ID3D11Texture2D*Texture(unsigned set,unsigned plane)const{return device_.p&&set<2&&plane<2?texture_[set][plane].p:NULL;}
 ID3D11ShaderResourceView*View(unsigned set,unsigned plane)const{return device_.p&&set<2&&plane<2?view_[set][plane].p:NULL;}
 ID3D11RenderTargetView*Target(unsigned set,unsigned plane)const{return device_.p&&set<2&&plane<2?target_[set][plane].p:NULL;}
 ID3D11Buffer*Constants()const{return device_.p?constants_.p:NULL;}
 HRESULT Init(ID3D11Device*d,U64 other){
  if(!d||device_.p)return E_INVALIDARG;if(!Fits(other))return E_OUTOFMEMORY;HRESULT hr=S_OK;
  for(unsigned set=0;set<2&&SUCCEEDED(hr);++set)for(unsigned plane=0;plane<2&&SUCCEEDED(hr);++plane){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=256;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;t.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;hr=d->CreateTexture2D(&t,NULL,&texture_[set][plane].p);if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(texture_[set][plane].p,NULL,&view_[set][plane].p);if(SUCCEEDED(hr))hr=d->CreateRenderTargetView(texture_[set][plane].p,NULL,&target_[set][plane].p);}
  D3D11_BUFFER_DESC cb={};cb.ByteWidth=16;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&cb,NULL,&constants_.p);if(SUCCEEDED(hr))hr=d->GetDeviceRemovedReason();if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
 }
};
// Native b0/t0..7 remain unchanged. These are the additional modified PS slots.
class Bindings{
 ID3D11DeviceContext*c_;AtlasView::Com<ID3D11Buffer>constants_;ID3D11ShaderResourceView*views_[2];Bindings(const Bindings&);Bindings&operator=(const Bindings&);
public:explicit Bindings(ID3D11DeviceContext*c):c_(c){c_->PSGetConstantBuffers(1,1,&constants_.p);c_->PSGetShaderResources(8,2,views_);}
 ~Bindings(){c_->PSSetConstantBuffers(1,1,&constants_.p);c_->PSSetShaderResources(8,2,views_);for(unsigned i=0;i<2;++i)if(views_[i])views_[i]->Release();}
};
}
