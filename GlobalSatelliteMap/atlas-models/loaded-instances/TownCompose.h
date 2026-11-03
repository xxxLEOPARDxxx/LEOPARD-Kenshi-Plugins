#pragma once
// Isolated fixed256 HDR compositor. Caller owns source admission, native light
// epoch, Main/compositor scheduling and fences covering borrowed source lifetime.
#include "../../atlas-water/AtlasWater.h"
namespace TownCompose {
typedef AtlasView::U64 U64;
struct Uniforms {
 AtlasView::Uniforms light;float cameraFar[4],sceneToCache[4],limits[4];
 bool Valid()const{
  const float*f=reinterpret_cast<const float*>(this);for(unsigned i=0;i<sizeof(*this)/4;++i)if(!_finite(f[i]))return false;
  return light.Valid(256)&&light.viewDirection[0]==0&&light.viewDirection[2]==0&&light.viewDirection[1]>0&&cameraFar[3]>0&&limits[0]>=16*FLT_EPSILON&&limits[0]<=.001f
   &&cameraFar[0]==light.tileBounds[0]-sceneToCache[0]+light.tileBounds[2]*.5f&&cameraFar[2]==light.tileBounds[1]-sceneToCache[2]+light.tileBounds[3]*.5f;
 }
};
typedef char ConstantAbi[sizeof(Uniforms)==224?1:-1];
const U64 PrivateBytes=256ULL*256*24+sizeof(Uniforms);
inline bool Fits(U64 caller){return caller<=AtlasView::GpuCap-PrivateBytes;}
class SavedCompute {
 ID3D11DeviceContext*c_;ID3D11ComputeShader*shader_;ID3D11ClassInstance*classes_[256];UINT count_;
 ID3D11Buffer*cb_;ID3D11ShaderResourceView*views_[10];ID3D11SamplerState*samplers_[3];ID3D11UnorderedAccessView*uav_[4];
 SavedCompute(const SavedCompute&);SavedCompute&operator=(const SavedCompute&);
public:
 explicit SavedCompute(ID3D11DeviceContext*c):c_(c),shader_(NULL),count_(256),cb_(NULL){
  std::memset(classes_,0,sizeof(classes_));std::memset(views_,0,sizeof(views_));std::memset(samplers_,0,sizeof(samplers_));std::memset(uav_,0,sizeof(uav_));
  c_->CSGetShader(&shader_,classes_,&count_);c_->CSGetConstantBuffers(0,1,&cb_);c_->CSGetShaderResources(0,10,views_);c_->CSGetSamplers(0,3,samplers_);c_->CSGetUnorderedAccessViews(0,4,uav_);
 }
 ~SavedCompute(){
  ID3D11UnorderedAccessView*none[4]={};c_->CSSetUnorderedAccessViews(0,4,none,NULL);
  c_->CSSetShader(shader_,classes_,count_);c_->CSSetConstantBuffers(0,1,&cb_);c_->CSSetShaderResources(0,10,views_);c_->CSSetSamplers(0,3,samplers_);c_->CSSetUnorderedAccessViews(0,4,uav_,NULL);
  if(shader_)shader_->Release();if(cb_)cb_->Release();for(unsigned i=0;i<count_;++i)if(classes_[i])classes_[i]->Release();
  for(unsigned i=0;i<10;++i)if(views_[i])views_[i]->Release();for(unsigned i=0;i<3;++i)if(samplers_[i])samplers_[i]->Release();for(unsigned i=0;i<4;++i)if(uav_[i])uav_[i]->Release();
 }
};
class Layer {
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11ComputeShader>shader_;AtlasView::Com<ID3D11Buffer>cb_;
 AtlasView::Com<ID3D11Texture2D>tex_[4];AtlasView::Com<ID3D11ShaderResourceView>view_[4];AtlasView::Com<ID3D11UnorderedAccessView>uav_[4];DWORD thread_;
 Layer(const Layer&);Layer&operator=(const Layer&);
public:
 Layer():thread_(0){}~Layer(){Clear();}
 void Clear(){for(unsigned i=0;i<4;++i){uav_[i].Reset();view_[i].Reset();tex_[i].Reset();}cb_.Reset();shader_.Reset();device_.Reset();thread_=0;}
 HRESULT Init(ID3D11Device*d,const void*code,SIZE_T bytes,U64 caller){
  if(device_.p||!d||!code||!bytes||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return E_INVALIDARG;if(!Fits(caller))return E_OUTOFMEMORY;
  const DXGI_FORMAT format[]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
  for(unsigned i=0;i<3;++i){UINT support=0;HRESULT h=d->CheckFormatSupport(format[i],&support);const UINT need=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_LOAD|D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;if(FAILED(h)||(support&need)!=need)return FAILED(h)?h:E_FAIL;}
  HRESULT hr=d->CreateComputeShader(code,bytes,NULL,&shader_.p);D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(Uniforms);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&cb_.p);
  for(unsigned i=0;i<4&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=256;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=format[i];t.BindFlags=D3D11_BIND_UNORDERED_ACCESS|D3D11_BIND_SHADER_RESOURCE;
   hr=d->CreateTexture2D(&t,NULL,&tex_[i].p);if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(tex_[i].p,NULL,&view_[i].p);if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(tex_[i].p,NULL,&uav_[i].p);
  }
  if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();thread_=GetCurrentThreadId();return S_OK;
 }
 //0..2 rawRGB/gloss, native normal/translucency, radial;3..5 previous accepted
 //RGB/coverage/Y;6 pristine HDR;7..9 current native ambient/irradiance/specular.
 //All inputs borrowed synchronously. No input may alias ANY owned output.
 //Each success recomposes from caller's pristine inputs, never self-accumulates.
 HRESULT Render(bool enabled,const Uniforms&v,ID3D11ShaderResourceView*const*source,ID3D11SamplerState*const*sampler,U64 caller){
  if(!enabled)return S_FALSE;if(!Fits(caller))return E_OUTOFMEMORY;
  if(!device_.p||thread_!=GetCurrentThreadId()||!v.Valid()||!source||!sampler)return E_INVALIDARG;
  const DXGI_FORMAT format[]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
  for(unsigned i=0;i<7;++i)if(!AtlasWater::View2D(source[i],device_.p,format[i],256,256))return E_INVALIDARG;
  for(unsigned i=0;i<3;++i)if(!AtlasView::LightView(source[i+7],device_.p,i!=0,i==0?1:i==1?4:8)||!AtlasView::SameDevice(sampler[i],device_.p))return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)return DXGI_ERROR_INVALID_CALL;
  for(unsigned i=0;i<4;++i)if(AtlasView::Bound(c.p,tex_[i].p,true))return DXGI_ERROR_INVALID_CALL;
  for(unsigned i=0;i<10;++i){AtlasView::Com<ID3D11Resource>r;source[i]->GetResource(&r.p);for(unsigned j=0;j<4;++j)if(AtlasView::Identity(r.p,tex_[j].p))return DXGI_ERROR_INVALID_CALL;if(AtlasView::Bound(c.p,r.p,true))return DXGI_ERROR_INVALID_CALL;}
  AtlasView::SavedOutputReads a(c.p,tex_[0].p),b(c.p,tex_[1].p),h(c.p,tex_[2].p),hdr(c.p,tex_[3].p);AtlasWater::SavedPredicate predicate(c.p);SavedCompute saved(c.p);
  c.p->UpdateSubresource(cb_.p,0,NULL,&v,0,0);c.p->CSSetShader(shader_.p,NULL,0);c.p->CSSetConstantBuffers(0,1,&cb_.p);c.p->CSSetShaderResources(0,10,source);c.p->CSSetSamplers(0,3,sampler);
  ID3D11UnorderedAccessView*out[]={uav_[0].p,uav_[1].p,uav_[2].p,uav_[3].p};c.p->CSSetUnorderedAccessViews(0,4,out,NULL);c.p->Dispatch(32,32,1);return device_.p->GetDeviceRemovedReason();
 }
 //Borrowed owned handles; valid until Clear/destruction. Caller fences reuse.
 ID3D11Texture2D*Texture(unsigned i){return i<4?tex_[i].p:NULL;}ID3D11ShaderResourceView*Output(unsigned i){return i<4?view_[i].p:NULL;}
};
}
