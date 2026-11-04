#pragma once
#include "atlas-view/AtlasRelight.h"
#include <new>
namespace GsmFineSeamReadback {
typedef AtlasView::U64 U64;
enum{Side=256,PlaneBytes=Side*Side*4,PayloadBytes=2*PlaneBytes};
class Blob {
 BYTE*data_;Blob(const Blob&);Blob&operator=(const Blob&);friend class Request;
public:Blob():data_(NULL){}~Blob(){Clear();}const BYTE*Data(unsigned plane)const{return data_&&plane<2?data_+plane*PlaneBytes:NULL;}U64 OwnedBytes()const{return data_?sizeof(Blob)+PayloadBytes:0;}void Clear(){delete[]data_;data_=NULL;}
};
class Predicate {
 ID3D11DeviceContext*c_;AtlasView::Com<ID3D11Predicate>old_;BOOL value_;
 Predicate(const Predicate&);Predicate&operator=(const Predicate&);
public:explicit Predicate(ID3D11DeviceContext*c):c_(c),value_(FALSE){c_->GetPredication(&old_.p,&value_);c_->SetPredication(NULL,FALSE);}~Predicate(){c_->SetPredication(old_.p,value_);}
};
class Request {
 enum State{Idle,Pending,Readable,Complete,Broken};State state_;DWORD thread_;HRESULT error_;
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11DeviceContext>context_;AtlasView::Com<ID3D11Texture2D>source_[2],stage_[2];AtlasView::Com<ID3D11Query>query_;
 Request(const Request&);Request&operator=(const Request&);
 HRESULT Fail(HRESULT hr){error_=FAILED(hr)?hr:E_FAIL;state_=Broken;return error_;}
 static bool Shape(ID3D11Texture2D*t,ID3D11Device*d){
  if(!AtlasView::SameDevice(t,d))return false;D3D11_TEXTURE2D_DESC s;t->GetDesc(&s);return s.Width==Side&&s.Height==Side&&s.Format==DXGI_FORMAT_R8G8B8A8_UNORM&&s.MipLevels==1&&s.ArraySize==1&&s.SampleDesc.Count==1&&!s.SampleDesc.Quality&&s.Usage==D3D11_USAGE_DEFAULT&&!s.CPUAccessFlags;
 }
 struct Maps {
  ID3D11DeviceContext*c;ID3D11Texture2D*t[2];D3D11_MAPPED_SUBRESOURCE value[2];bool mapped[2];
  explicit Maps(ID3D11DeviceContext*context):c(context){std::memset(t,0,sizeof(t));std::memset(value,0,sizeof(value));std::memset(mapped,0,sizeof(mapped));}
  ~Maps(){for(unsigned i=0;i<2;++i)if(mapped[i])c->Unmap(t[i],0);}
 private:Maps(const Maps&);Maps&operator=(const Maps&);
 };
public:
 Request():state_(Idle),thread_(0),error_(S_OK){}~Request(){Clear();}
 // Clear is explicit cancellation/teardown, not a completion proof. D3D11
 // defers destruction of resources still referenced by queued commands. A
 // caller requiring known-fence retirement retains this entire owner instead
 // on Failed()/unknown query outcome until its normal device teardown.
 void Clear(){query_.Reset();for(unsigned i=0;i<2;++i){source_[i].Reset();stage_[i].Reset();}context_.Reset();device_.Reset();state_=Idle;thread_=0;error_=S_OK;}
 U64 GpuBytes()const{return U64(bool(stage_[0].p)+bool(stage_[1].p))*PlaneBytes;}
 // Existing output allocations pinned across the event, not new allocations.
 // A caller may conservatively charge these again so cache retirement cannot
 // make still-pinned output storage disappear from its aggregate debt.
 U64 PinnedSourceBytes()const{return U64(bool(source_[0].p)+bool(source_[1].p))*PlaneBytes;}
 // Conservatively charge staging host storage here too; completed Blob is a
 // separate caller owner and must additionally charge Blob::OwnedBytes().
 U64 CpuBytes()const{return sizeof(*this)+GpuBytes();}
 bool Active()const{return state_==Pending||state_==Readable||state_==Broken;}bool Failed()const{return state_==Broken;}bool Ready()const{return state_==Readable||state_==Complete;}HRESULT Error()const{return error_;}
 HRESULT Begin(ID3D11Device*d,ID3D11Texture2D*a,ID3D11Texture2D*b,U64 otherGpu,U64 otherCpu){
  if(state_!=Idle||!d||!Shape(a,d)||!Shape(b,d)||AtlasView::Identity(a,b))return E_INVALIDARG;
  if(otherGpu>AtlasView::GpuCap-PayloadBytes||otherCpu>AtlasView::CpuCap-PayloadBytes-sizeof(*this))return E_OUTOFMEMORY;
  AtlasView::Com<ID3D11DeviceContext>c;d->GetImmediateContext(&c.p);if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||AtlasView::Bound(c.p,a,true)||AtlasView::Bound(c.p,b,true))return DXGI_ERROR_INVALID_CALL;
  D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=Side;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  HRESULT hr=d->CreateTexture2D(&td,NULL,&stage_[0].p);if(SUCCEEDED(hr))hr=d->CreateTexture2D(&td,NULL,&stage_[1].p);D3D11_QUERY_DESC qd={D3D11_QUERY_EVENT,0};if(SUCCEEDED(hr))hr=d->CreateQuery(&qd,&query_.p);if(FAILED(hr)){Clear();return hr;}
  device_.p=d;d->AddRef();context_.p=c.Detach();source_[0].p=a;a->AddRef();source_[1].p=b;b->AddRef();thread_=GetCurrentThreadId();state_=Pending;
  {Predicate predicate(context_.p);for(unsigned i=0;i<2;++i)context_.p->CopyResource(stage_[i].p,source_[i].p);context_.p->End(query_.p);}
  hr=device_.p->GetDeviceRemovedReason();return FAILED(hr)?Fail(hr):S_OK;
 }
 HRESULT Poll(Blob&out,U64 otherCpu){
  if(thread_!=GetCurrentThreadId()||state_==Idle||state_==Complete||out.data_)return E_INVALIDARG;if(state_==Broken)return error_;
  if(state_==Pending){BOOL done=FALSE;HRESULT hr=context_.p->GetData(query_.p,&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);if(hr==S_FALSE)return S_FALSE;if(FAILED(hr))return Fail(hr);if(!done)return S_FALSE;state_=Readable;source_[0].Reset();source_[1].Reset();}
  if(otherCpu>AtlasView::CpuCap-CpuBytes()-sizeof(Blob)-PayloadBytes)return E_OUTOFMEMORY;
  Maps maps(context_.p);for(unsigned i=0;i<2;++i){maps.t[i]=stage_[i].p;HRESULT hr=context_.p->Map(stage_[i].p,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&maps.value[i]);if(hr==DXGI_ERROR_WAS_STILL_DRAWING)return S_FALSE;if(FAILED(hr))return Fail(hr);maps.mapped[i]=true;if(!maps.value[i].pData||maps.value[i].RowPitch<Side*4)return Fail(E_FAIL);}
  BYTE*copy=new(std::nothrow)BYTE[PayloadBytes];if(!copy)return E_OUTOFMEMORY;
  for(unsigned i=0;i<2;++i)for(unsigned y=0;y<Side;++y)std::memcpy(copy+i*PlaneBytes+y*Side*4,static_cast<const BYTE*>(maps.value[i].pData)+size_t(y)*maps.value[i].RowPitch,Side*4);
  out.data_=copy;state_=Complete;error_=S_OK;return S_OK;
 }
};
}
