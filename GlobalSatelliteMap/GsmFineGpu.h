#pragma once
#include "atlas-view/AtlasRelight.h"
// Main/render-thread retirement. A released UI/Ogre owner is not credited
// until our own event, submitted after its last use, actually completes.
namespace GsmFineGpu {
typedef AtlasView::U64 U64;
const U64 WindowBytes=256ULL*256*20,RelightBytes=256ULL*256*4+AtlasView::UniformGpuBytes;
class Retirements {
 struct Record {IUnknown*owned[4];U64 bytes;unsigned batch;Record():bytes(0),batch(0){std::memset(owned,0,sizeof(owned));}} records_[GsmCacheProfile::RetirementSlots];
 struct Fence {AtlasView::Com<ID3D11Query>query;unsigned batch;Fence():batch(0){}} fences_[8];
 unsigned next_,pending_,used_,free_;U64 bytes_;bool failed_;
 Retirements(const Retirements&);Retirements&operator=(const Retirements&);
public:
 Retirements():next_(1),pending_(1),used_(0),free_(0),bytes_(0),failed_(false){}
 // On unknown GPU completion retain refs for process lifetime. Runtime DLL
 // unloading is unsupported; no D3D calls in the late static destructor.
 ~Retirements(){for(unsigned i=0;i<8;++i)fences_[i].query.p=NULL;}
 bool Add(ID3D11Texture2D*const*planes,ID3D11Texture2D*output,U64 bytes){
  if(!bytes)return true;if(bytes>AtlasView::GpuCap||bytes_>AtlasView::GpuCap-bytes)return false;
  for(unsigned i=free_;i<GsmCacheProfile::RetirementCount();++i)if(!records_[i].bytes){Record&r=records_[i];r.bytes=bytes;r.batch=pending_;for(unsigned p=0;p<3;++p)if(planes&&planes[p]){r.owned[p]=planes[p];r.owned[p]->AddRef();}if(output){r.owned[3]=output;r.owned[3]->AddRef();}bytes_+=bytes;if(i>=used_)used_=i+1;free_=i+1;return true;}return false;
 }
 // Opaque layer owners have already released their public refs. D3D retains
 // submitted command references; keep their full allocation debt until fence.
 bool Charge(U64 bytes){return Add(NULL,NULL,bytes);}
 HRESULT Tick(ID3D11Device*d){
  if(failed_||!d)return E_FAIL;AtlasView::Com<ID3D11DeviceContext>c;d->GetImmediateContext(&c.p);
  for(unsigned q=0;q<8;++q)if(fences_[q].query.p){HRESULT hr=c.p->GetData(fences_[q].query.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);if(hr==S_FALSE)continue;if(hr!=S_OK){failed_=true;return FAILED(hr)?hr:E_FAIL;}
   for(unsigned i=0;i<used_;++i){Record&r=records_[i];if(r.bytes&&r.batch==fences_[q].batch){for(unsigned p=0;p<4;++p)if(r.owned[p])r.owned[p]->Release();bytes_-=r.bytes;r=Record();if(i<free_)free_=i;}}fences_[q].query.Reset();fences_[q].batch=0;
  }
  while(used_&&!records_[used_-1].bytes)--used_;
  bool pending=false;for(unsigned i=0;i<used_;++i)pending|=records_[i].bytes&&records_[i].batch==pending_;if(!pending)return S_OK;
  unsigned q=0;while(q<8&&fences_[q].query.p)++q;if(q==8)return S_FALSE;
  D3D11_QUERY_DESC desc={D3D11_QUERY_EVENT,0};HRESULT hr=d->CreateQuery(&desc,&fences_[q].query.p);if(FAILED(hr)){failed_=true;return hr;}
  fences_[q].batch=pending_;c.p->End(fences_[q].query.p);c.p->Flush();if(++next_==0){failed_=true;return E_FAIL;}pending_=next_;return S_OK;
 }
 U64 Bytes()const{return bytes_;}bool Failed()const{return failed_;}
};
inline bool Fits(U64 live,U64 retired,U64 producer,U64 pending){return live<=AtlasView::GpuCap&&retired<=AtlasView::GpuCap-live&&producer<=AtlasView::GpuCap-live-retired&&pending<=AtlasView::GpuCap-live-retired-producer;}
inline HRESULT Upload(ID3D11Device*d,const BYTE*material,const BYTE*normal,const float*height,AtlasView::Cell&cell){
 if(!d||!material||!normal||!height||cell.side)return E_INVALIDARG;const void*data[3]={material,normal,height};const DXGI_FORMAT formats[3]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};const unsigned stride[3]={8,4,4};
 for(unsigned p=0;p<3;++p){D3D11_TEXTURE2D_DESC desc={};desc.Width=desc.Height=256;desc.MipLevels=desc.ArraySize=1;desc.Format=formats[p];desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA input={data[p],256*stride[p],0};HRESULT hr=d->CreateTexture2D(&desc,&input,&cell.texture[p].p);if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(cell.texture[p].p,NULL,&cell.view[p].p);if(FAILED(hr)){cell.Clear();return hr;}}
 cell.side=256;return S_OK;
}
}
