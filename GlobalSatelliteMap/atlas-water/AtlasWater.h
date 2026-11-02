#pragma once
// Optional isolated HDR material layer. No game/Ogre calls, worker or polling.
// Reuse the already reviewed COM identity/alias/read-binding primitives.
#include "../atlas-view/AtlasRelight.h"
#include <cmath>
namespace AtlasWater {
typedef unsigned __int64 U64;
const U64 Cap=96ULL*1024*1024;
const U64 PrivateBytes=512ULL*512*8+192;
inline bool Fits(U64 callerOwnedBytes){return callerOwnedBytes<=Cap-PrivateBytes;}
struct Uniforms {
 float tileBounds[4],dispatchWindow[4],sourceWindow[4],waterPlane[4],nativeMapBounds[4],normalScale[4],waterSunDirection[4],waterSunColour[4],waterEnvColour[4],materialParams[4],waterTime[4],parallelView[4];
 bool Valid(unsigned width,unsigned height)const{
  const float* f=reinterpret_cast<const float*>(this);for(unsigned i=0;i<sizeof(*this)/4;++i)if(!_finite(f[i]))return false;
  float s=sourceWindow[0],x=sourceWindow[1],z=sourceWindow[2];
  if((s!=256&&s!=512&&s!=1024&&s!=2048)||!width||!height||width>512||height>512||x<0||z<0||x+width>s||z+height>s||std::floor(x)!=x||std::floor(z)!=z||int(x)%2||int(z)%2)return false;
  float sun=0,view=0;for(unsigned i=0;i<3;++i){sun+=waterSunDirection[i]*waterSunDirection[i];view+=parallelView[i]*parallelView[i];}
  return tileBounds[2]>0&&tileBounds[3]>0&&nativeMapBounds[2]>nativeMapBounds[0]&&nativeMapBounds[3]>nativeMapBounds[1]&&_finite(sun)&&_finite(view)&&sun>1e-8f&&view>1e-8f&&materialParams[0]>0&&materialParams[1]>0&&materialParams[2]>=0&&materialParams[2]<=1&&(waterPlane[1]==0||waterPlane[1]==1);
 }
};
typedef char ConstantAbi[(sizeof(Uniforms)==192)?1:-1];
class SavedCompute {
 ID3D11DeviceContext* c_;ID3D11ComputeShader* shader_;ID3D11ClassInstance* classes_[256];UINT count_;
 ID3D11Buffer* cb_;ID3D11ShaderResourceView* views_[8];ID3D11SamplerState* samplers_[5];ID3D11UnorderedAccessView* uav_;
 SavedCompute(const SavedCompute&);SavedCompute&operator=(const SavedCompute&);
public:
 SavedCompute(ID3D11DeviceContext*c):c_(c),shader_(NULL),count_(256),cb_(NULL),uav_(NULL){
  std::memset(classes_,0,sizeof(classes_));std::memset(views_,0,sizeof(views_));std::memset(samplers_,0,sizeof(samplers_));
  c_->CSGetShader(&shader_,classes_,&count_);c_->CSGetConstantBuffers(0,1,&cb_);c_->CSGetShaderResources(0,8,views_);c_->CSGetSamplers(0,5,samplers_);c_->CSGetUnorderedAccessViews(0,1,&uav_);
 }
 ~SavedCompute(){
  ID3D11UnorderedAccessView* none=NULL;c_->CSSetUnorderedAccessViews(0,1,&none,NULL);
  c_->CSSetShader(shader_,classes_,count_);c_->CSSetConstantBuffers(0,1,&cb_);c_->CSSetShaderResources(0,8,views_);c_->CSSetSamplers(0,5,samplers_);c_->CSSetUnorderedAccessViews(0,1,&uav_,NULL);
  if(shader_)shader_->Release();if(cb_)cb_->Release();if(uav_)uav_->Release();
  for(unsigned i=0;i<count_;++i)if(classes_[i])classes_[i]->Release();for(unsigned i=0;i<8;++i)if(views_[i])views_[i]->Release();for(unsigned i=0;i<5;++i)if(samplers_[i])samplers_[i]->Release();
 }
};
class SavedPredicate {
 ID3D11DeviceContext* c_;AtlasView::Com<ID3D11Predicate> p_;BOOL value_;
 SavedPredicate(const SavedPredicate&);SavedPredicate&operator=(const SavedPredicate&);
public:SavedPredicate(ID3D11DeviceContext*c):c_(c),value_(FALSE){c_->GetPredication(&p_.p,&value_);c_->SetPredication(NULL,FALSE);}~SavedPredicate(){c_->SetPredication(p_.p,value_);}
};
inline bool View2D(ID3D11ShaderResourceView*v,ID3D11Device*d,DXGI_FORMAT format,unsigned width,unsigned height){
 if(!AtlasView::SameDevice(v,d))return false;D3D11_SHADER_RESOURCE_VIEW_DESC s;v->GetDesc(&s);
 if(s.Format!=format||s.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||s.Texture2D.MostDetailedMip||s.Texture2D.MipLevels!=1)return false;
 AtlasView::Com<ID3D11Resource> r;v->GetResource(&r.p);AtlasView::Com<ID3D11Texture2D> t;
 if(FAILED(r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p))))return false;
 D3D11_TEXTURE2D_DESC td;t.p->GetDesc(&td);return td.Width==width&&td.Height==height&&td.MipLevels==1&&td.ArraySize==1&&td.SampleDesc.Count==1&&!td.SampleDesc.Quality&&td.Format==format;
}
class Layer {
 AtlasView::Com<ID3D11Device> device_;AtlasView::Com<ID3D11ComputeShader> shader_;AtlasView::Com<ID3D11Buffer> cb_;
 AtlasView::Com<ID3D11Texture2D> scratch_;AtlasView::Com<ID3D11UnorderedAccessView> uav_;
 Layer(const Layer&);Layer&operator=(const Layer&);
public:
 Layer(){}
 void Clear(){uav_.Reset();scratch_.Reset();cb_.Reset();shader_.Reset();device_.Reset();}
 // callerOwnedBytes includes ALL coexisting cache planes, display outputs,
 // terrain HDR block, renderer scratch/constants and pending/retired resources.
 HRESULT Init(ID3D11Device*d,const void*code,SIZE_T bytes,U64 callerOwnedBytes){
  if(device_.p||!d||!code||!bytes||!Fits(callerOwnedBytes)||d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)return E_INVALIDARG;
  UINT support=0;HRESULT hr=d->CheckFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT,&support);
  const UINT need=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;
  if(FAILED(hr)||(support&need)!=need)return FAILED(hr)?hr:E_FAIL;
  hr=d->CreateComputeShader(code,bytes,NULL,&shader_.p);
  D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(Uniforms);b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&cb_.p);
  D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=512;t.MipLevels=t.ArraySize=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
  if(SUCCEEDED(hr))hr=d->CreateTexture2D(&t,NULL,&scratch_.p);if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(scratch_.p,NULL,&uav_.p);
  if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
 }
 // Source0 is a <=512 caller-owned straight LINEAR HDR block. Sources1/2 are
 // selected full-cell coverage/height;3..7 are native water material SRVs.
 // Destination may alias source0 for in-place HDR composition via private scratch.
 // OFF returns S_FALSE without reading native sources or changing any output/state.
 HRESULT Render(const Uniforms&values,ID3D11ShaderResourceView*const source[8],ID3D11SamplerState*const sampler[5],ID3D11Texture2D*ownedHdr,U64 callerOwnedBytes){
  if(values.waterPlane[1]==0)return S_FALSE;
  if(!Fits(callerOwnedBytes)||!device_.p||!source||!sampler||!AtlasView::SameDevice(ownedHdr,device_.p))return E_INVALIDARG;
  D3D11_TEXTURE2D_DESC d;ownedHdr->GetDesc(&d);
  if(!values.Valid(d.Width,d.Height)||d.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||d.MipLevels!=1||d.ArraySize!=1||d.SampleDesc.Count!=1||d.SampleDesc.Quality||d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||d.MiscFlags)return E_INVALIDARG;
  unsigned side=unsigned(values.sourceWindow[0]);
  if(!View2D(source[0],device_.p,d.Format,d.Width,d.Height)||!View2D(source[1],device_.p,DXGI_FORMAT_R8G8B8A8_UNORM,side,side)||!View2D(source[2],device_.p,DXGI_FORMAT_R32_FLOAT,side,side))return E_INVALIDARG;
  for(unsigned p=3;p<8;++p)if(!AtlasView::LightView(source[p],device_.p,p>=6,p<6?1:p==6?4:8))return E_INVALIDARG;
  for(unsigned s=0;s<5;++s)if(!AtlasView::SameDevice(sampler[s],device_.p))return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext> c;device_.p->GetImmediateContext(&c.p);
  if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||AtlasView::Bound(c.p,ownedHdr,true))return DXGI_ERROR_INVALID_CALL;
  for(unsigned p=0;p<8;++p){AtlasView::Com<ID3D11Resource> r;source[p]->GetResource(&r.p);if((p&&AtlasView::Identity(r.p,ownedHdr))||AtlasView::Bound(c.p,r.p,true))return DXGI_ERROR_INVALID_CALL;}
  AtlasView::SavedOutputReads outputReads(c.p,ownedHdr);SavedPredicate predicate(c.p);SavedCompute saved(c.p);
  Uniforms v=values;v.dispatchWindow[0]=float(d.Width);v.dispatchWindow[1]=float(d.Height);v.dispatchWindow[2]=v.dispatchWindow[3]=0;
  c.p->UpdateSubresource(cb_.p,0,NULL,&v,0,0);c.p->CSSetShader(shader_.p,NULL,0);c.p->CSSetConstantBuffers(0,1,&cb_.p);c.p->CSSetShaderResources(0,8,source);c.p->CSSetSamplers(0,5,sampler);c.p->CSSetUnorderedAccessViews(0,1,&uav_.p,NULL);
  c.p->Dispatch((d.Width+7)/8,(d.Height+7)/8,1);
  ID3D11UnorderedAccessView* noUav=NULL;c.p->CSSetUnorderedAccessViews(0,1,&noUav,NULL);
  ID3D11ShaderResourceView* noView=NULL;c.p->CSSetShaderResources(0,1,&noView); // unbind in-place source before copy
  D3D11_BOX box={0,0,0,d.Width,d.Height,1};c.p->CopySubresourceRegion(ownedHdr,0,0,0,0,scratch_.p,0,&box);
  return device_.p->GetDeviceRemovedReason();
 }
};
}
