#pragma once
#include "../accepted-window/AcceptedWindow.h"

// CPU-only sealed child. Build belongs to a worker; callers join consumers
// before Build/Clear/destruction. No source/window borrow survives Build.
namespace AcceptedCrop {
typedef WorldPixels::U64 U64;
enum { Side=256, PayloadBytes=Side*Side*16 };
struct Ledger { U64 retainedRaw,other; Ledger():retainedRaw(0),other(0){} };
class Tile {
 BYTE*data_;SourceReceipts::Scope scope_;WorldRaster::Frame parent_,child_;WorldHeight::Identity height_;HRESULT error_;
 Tile(const Tile&);Tile&operator=(const Tile&);
 HRESULT Fail(HRESULT hr){Clear();error_=hr;return hr;}
 static bool Budget(const WorldPixels::Accepted&p,const WorldHeight::Window&w,const Ledger&l){
  U64 left=SourceReceipts::CpuCap;const U64 charges[]={RequiredBytes(),p.OwnedBytes(),w.OwnedBytes(),sizeof(w),l.retainedRaw,l.other};
  for(unsigned i=0;i<sizeof(charges)/sizeof(charges[0]);++i){if(charges[i]>left)return false;left-=charges[i];}return true;
 }
public:
 Tile():data_(NULL),error_(S_OK){std::memset(&scope_,0,sizeof(scope_));std::memset(&parent_,0,sizeof(parent_));std::memset(&child_,0,sizeof(child_));std::memset(&height_,0,sizeof(height_));}
 ~Tile(){Clear();}
 void Clear(){delete[]data_;data_=NULL;error_=S_OK;std::memset(&scope_,0,sizeof(scope_));std::memset(&parent_,0,sizeof(parent_));std::memset(&child_,0,sizeof(child_));std::memset(&height_,0,sizeof(height_));}
 static U64 RequiredBytes(){return sizeof(Tile)+PayloadBytes;}
 U64 OwnedBytes()const{return data_?RequiredBytes():0;}
 bool Ready()const{return data_!=NULL;}
 HRESULT Error()const{return error_;}
 const WorldRaster::Frame&ParentFrame()const{return parent_;}
 const WorldRaster::Frame&ChildFrame()const{return child_;}
 HRESULT Build(bool enabled,const WorldPixels::Accepted&p,const SourceReceipts::Scope&now,const WorldRaster::Frame&parent,const WorldHeight::Window&w,unsigned pixelX,unsigned pixelZ,HANDLE cancel,const Ledger&ledger){
  Clear();if(!enabled)return Fail(S_FALSE);if(GetCurrentThreadId()==now.thread)return Fail(E_ACCESSDENIED);
  // The accepted parent is acquired exactly once. Only decoded, already
  // authenticated planes reach this class; radial depth has no crop API.
  WorldPixels::View v={};if(!p.Acquire(true,now,parent,w,v))return Fail(E_ABORT);
  if((v.side!=256&&v.side!=512&&v.side!=1024)||(pixelX%Side)||(pixelZ%Side)||pixelX>v.side-Side||pixelZ>v.side-Side)return Fail(E_INVALIDARG);
  WorldRaster::Frame child=parent;child.side=Side;child.x+=float(int(pixelX)+Side/2-int(v.side)/2)*parent.spacing;child.z+=float(int(pixelZ)+Side/2-int(v.side)/2)*parent.spacing;
  if(!WorldPixels::ValidFrame(now,child))return Fail(E_INVALIDARG);
  if(!Budget(p,w,ledger))return Fail(E_OUTOFMEMORY);if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
  data_=new(std::nothrow)BYTE[PayloadBytes];if(!data_)return Fail(E_OUTOFMEMORY);
  for(unsigned row=0;row<Side;++row){
   if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));const size_t src=size_t(row+pixelZ)*v.side+pixelX,dst=size_t(row)*Side;
   std::memcpy(data_+dst*8,v.material+src*8,Side*8);std::memcpy(data_+Side*Side*12+dst*4,v.height+src,Side*4);
   BYTE*normal=data_+Side*Side*8+dst*4;const BYTE*original=v.normal+src*4;
   for(unsigned x=0;x<Side;++x){if(original[x*4+3]!=0)return Fail(E_INVALIDARG);std::memcpy(normal+x*4,original+x*4,3);normal[x*4+3]=255;}
  }
  if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));scope_=now;parent_=parent;child_=child;height_=v.heightSource;return error_=S_OK;
 }
 bool Acquire(bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&child,HANDLE cancel,AcceptedWindow::View&out)const{
  std::memset(&out,0,sizeof(out));if(!enabled||!data_||!SourceReceipts::SameScope(scope_,now)||!WorldRaster::SameFrame(child_,child)||WorldPixels::Cancelled(cancel))return false;
  out.scope=scope_;out.frame=child_;out.heightSource=height_;out.side=Side;out.material=data_;out.normal=data_+Side*Side*8;out.height=reinterpret_cast<const float*>(data_+Side*Side*12);return true;
 }
};
}
