#pragma once
#include "../../atlas-terrain/world-pixels/WorldPixels.h"
namespace AcceptedWindow{
typedef WorldPixels::U64 U64;
struct Ledger{U64 retainedRaw,other;Ledger():retainedRaw(0),other(0){}};
struct View{SourceReceipts::Scope scope;WorldRaster::Frame frame;WorldHeight::Identity heightSource;unsigned side;const BYTE*material;const BYTE*normal;const float*height;
 const void*Plane(unsigned i)const{if(i==0)return material;if(i==1)return normal;if(i==2)return height;return NULL;}
 unsigned Pitch(unsigned i)const{return i<3?side*(i==0?8:4):0;}
 size_t Bytes(unsigned i)const{return size_t(side)*Pitch(i);}
};
class Adapter{
 BYTE*normal_;unsigned side_;const WorldPixels::Accepted*owner_;SourceReceipts::Scope scope_;WorldRaster::Frame frame_;WorldHeight::Identity height_;HRESULT error_;
 Adapter(const Adapter&);Adapter&operator=(const Adapter&);
 HRESULT Fail(HRESULT hr){Clear();error_=hr;return hr;}
 bool Budget(const WorldPixels::Accepted&p,const WorldHeight::Window&w,unsigned side,const Ledger&l)const{
  const U64 required=RequiredBytes(side);if(!required)return false;U64 remaining=SourceReceipts::CpuCap;
  const U64 charges[6]={required,p.OwnedBytes(),w.OwnedBytes(),sizeof(w),l.retainedRaw,l.other};
  for(unsigned i=0;i<6;++i){if(charges[i]>remaining)return false;remaining-=charges[i];}return true;
 }
public:
 Adapter():normal_(NULL),side_(0),owner_(NULL),error_(S_OK){std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&height_,0,sizeof(height_));}
 ~Adapter(){Clear();}
 void Clear(){delete[]normal_;normal_=NULL;side_=0;owner_=NULL;error_=S_OK;std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&height_,0,sizeof(height_));}
 bool Ready()const{return normal_!=NULL&&owner_!=NULL;}U64 OwnedBytes()const{return normal_?RequiredBytes(side_):0;}HRESULT Error()const{return error_;}
 static U64 RequiredBytes(unsigned side){return WorldRaster::CarryBytes(side)?sizeof(Adapter)+U64(side)*side*4:0;}
 HRESULT Build(bool enabled,const WorldPixels::Accepted&p,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame,const WorldHeight::Window&w,HANDLE cancel,const Ledger&ledger){
  Clear();if(!enabled)return Fail(S_FALSE);WorldPixels::View v={};
  if(!p.Acquire(true,now,frame,w,v))return Fail(E_ABORT);
  if(!Budget(p,w,v.side,ledger))return Fail(E_OUTOFMEMORY);
  if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
  normal_=new(std::nothrow)BYTE[size_t(v.side)*v.side*4];if(!normal_)return Fail(E_OUTOFMEMORY);side_=v.side;
  for(unsigned row=0;row<side_;++row){
   if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
   for(unsigned x=0;x<side_;++x){const size_t at=(size_t(row)*side_+x)*4;const BYTE*src=v.normal+at;BYTE*dst=normal_+at;
    if(src[3]!=0)return Fail(E_INVALIDARG);std::memcpy(dst,src,3);dst[3]=255;
   }
  }
  if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
  owner_=&p;scope_=now;frame_=frame;height_=v.heightSource;return S_OK;
 }
 // Every view borrows the fresh accepted slab, never cached material/height pointers.
 // Owner/window and this adapter must remain alive and unchanged through consumption.
 bool Acquire(bool enabled,const WorldPixels::Accepted&p,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame,const WorldHeight::Window&w,HANDLE cancel,const Ledger&ledger,View&out){
  std::memset(&out,0,sizeof(out));
  if(!enabled){Fail(S_FALSE);return false;}
  if(!Ready()||owner_!=&p||!SourceReceipts::SameScope(scope_,now)||!WorldRaster::SameFrame(frame_,frame)){Fail(E_ABORT);return false;}
  WorldPixels::View v={};if(!p.Acquire(true,now,frame,w,v)||v.side!=side_||!WorldHeight::Same(height_,v.heightSource)){Fail(E_ABORT);return false;}
  if(!Budget(p,w,v.side,ledger)){Fail(E_OUTOFMEMORY);return false;}
  if(WorldPixels::Cancelled(cancel)){Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));return false;}
  // Accepted has no mutation serial. Refuse changed normal contents even if a
  // same-label rebuild reused its old allocation address.
  for(unsigned row=0;row<side_;++row){
   if(WorldPixels::Cancelled(cancel)){Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));return false;}
   for(unsigned x=0;x<side_;++x){const size_t at=(size_t(row)*side_+x)*4;
    if(v.normal[at+3]!=0||normal_[at+3]!=255||std::memcmp(v.normal+at,normal_+at,3)){Fail(E_ABORT);return false;}
   }
  }
  if(WorldPixels::Cancelled(cancel)){Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));return false;}
  out.scope=v.scope;out.frame=v.frame;out.heightSource=v.heightSource;out.side=side_;out.material=v.material;out.normal=normal_;out.height=v.height;error_=S_OK;return true;
 }
};
}
