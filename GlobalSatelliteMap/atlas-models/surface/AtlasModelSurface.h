#pragma once
#include "../../atlas-water/AtlasWater.h"
namespace AtlasModelSurface {
typedef AtlasView::U64 U64;
const unsigned Side=256;
const U64 PrivateBytes=U64(Side)*Side*16;
inline bool Fits(U64 caller){return caller<=AtlasView::GpuCap-PrivateBytes;}
inline DXGI_FORMAT Format(unsigned plane){const DXGI_FORMAT f[3]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};return plane<3?f[plane]:DXGI_FORMAT_UNKNOWN;}
inline bool SourceShape(const D3D11_TEXTURE2D_DESC&d,unsigned plane){
 const UINT allowed=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET|D3D11_BIND_UNORDERED_ACCESS;
 return plane<3&&d.Width==Side&&d.Height==Side&&d.MipLevels==1&&d.ArraySize==1&&d.Format==Format(plane)&&d.SampleDesc.Count==1&&!d.SampleDesc.Quality&&
  (d.Usage==D3D11_USAGE_DEFAULT||d.Usage==D3D11_USAGE_IMMUTABLE)&&(d.BindFlags&D3D11_BIND_SHADER_RESOURCE)&&!(d.BindFlags&~allowed)&&!d.CPUAccessFlags&&!d.MiscFlags;
}
class Layer {
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11Texture2D>texture_[3];AtlasView::Com<ID3D11ShaderResourceView>view_[3];
 Layer(const Layer&);Layer&operator=(const Layer&);
public:
 Layer(){}
 void Clear(){for(unsigned i=0;i<3;++i){view_[i].Reset();texture_[i].Reset();}device_.Reset();}
 // caller includes every other live/retired atlas allocation; never this surface twice.
 HRESULT Init(bool enabled,ID3D11Device*d,U64 caller){
  if(!enabled)return S_FALSE;if(!d||device_.p||!Fits(caller)||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return E_INVALIDARG;
  for(unsigned i=0;i<3;++i){UINT support=0;HRESULT hr=d->CheckFormatSupport(Format(i),&support);const UINT need=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_LOAD;if(FAILED(hr)||(support&need)!=need)return FAILED(hr)?hr:E_FAIL;}
  HRESULT hr=S_OK;for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=Side;t.MipLevels=t.ArraySize=1;t.Format=Format(i);t.SampleDesc.Count=1;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   hr=d->CreateTexture2D(&t,NULL,&texture_[i].p);if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(texture_[i].p,NULL,&view_[i].p);
  }
  if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
 }
 // Copies pristine cached texels exactly. Caller replays CURRENT models into
 // these DEFAULT textures, shades synchronously, then reuses for another window.
 HRESULT Copy(bool enabled,ID3D11ShaderResourceView*const sources[3],U64 caller){
  if(!enabled)return S_FALSE;if(!device_.p||!sources||!Fits(caller))return E_INVALIDARG;
  AtlasView::Com<ID3D11Texture2D>input[3];
  for(unsigned i=0;i<3;++i){
   if(!AtlasWater::View2D(sources[i],device_.p,Format(i),Side,Side))return E_INVALIDARG;
   AtlasView::Com<ID3D11Resource>r;sources[i]->GetResource(&r.p);if(!r.p||FAILED(r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&input[i].p))))return E_INVALIDARG;
   D3D11_TEXTURE2D_DESC t;input[i].p->GetDesc(&t);if(!SourceShape(t,i))return E_INVALIDARG;
   for(unsigned j=0;j<3;++j)if(AtlasView::Identity(input[i].p,texture_[j].p))return E_INVALIDARG;
   for(unsigned j=0;j<i;++j)if(AtlasView::Identity(input[i].p,input[j].p))return E_INVALIDARG;
  }
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return E_INVALIDARG;
  for(unsigned i=0;i<3;++i)if(AtlasView::Bound(c.p,input[i].p,true)||AtlasView::Bound(c.p,texture_[i].p,true))return DXGI_ERROR_INVALID_CALL;
  const HRESULT before=device_.p->GetDeviceRemovedReason();if(FAILED(before))return before;
  AtlasView::SavedOutputReads r0(c.p,texture_[0].p),r1(c.p,texture_[1].p),r2(c.p,texture_[2].p);AtlasWater::SavedPredicate predicate(c.p);
  for(unsigned i=0;i<3;++i)c.p->CopyResource(texture_[i].p,input[i].p);
  return device_.p->GetDeviceRemovedReason();
 }
 // Borrowed outputs, owned until Clear/destruction; no references transferred.
 ID3D11Texture2D*Texture(unsigned plane)const{return plane<3?texture_[plane].p:NULL;}
 ID3D11ShaderResourceView*Output(unsigned plane)const{return plane<3?view_[plane].p:NULL;}
};
}
