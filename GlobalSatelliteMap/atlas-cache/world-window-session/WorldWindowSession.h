#pragma once
#include "../accepted-window/AcceptedWindow.h"
#include "../window-writer512/WindowWriter512.h"
namespace WorldWindow {
typedef AcceptedWindow::U64 U64;typedef AcceptedWindow::Ledger Ledger;
struct Location{unsigned tileX,tileZ,x,z;};
struct Factory{virtual ~Factory(){}virtual U64 FileCpuBound()const=0;virtual Gst2::RandomFile*OpenNew(unsigned tileX,unsigned tileZ)=0;};
inline bool Locate(const WorldRaster::Frame&f,Location&out){
 std::memset(&out,0,sizeof(out));if(f.side!=512||f.spacing!=9.f||!_finite(f.x)||!_finite(f.z))return false;
 const double gx=(double(f.x)-2304+147456)/9,gz=(double(f.z)-2304+147456)/9;
 if(gx<0||gz<0||gx>32768-512||gz>32768-512||std::floor(gx)!=gx||std::floor(gz)!=gz)return false;
 const unsigned x=unsigned(gx),z=unsigned(gz);if(x%512||z%512)return false;
 out.tileX=x/8192;out.tileZ=z/8192;out.x=x%8192;out.z=z%8192;return true;
}
class Session{
 struct Tile{Gst2::RandomFile*file;Gst2Windows::Writer writer;U64 done[4];Tile():file(NULL){std::memset(done,0,sizeof(done));}}tiles_[16];
 AcceptedWindow::Adapter adapter_;Factory*factory_;SourceReceipts::Scope scope_;Gst2Windows::Stamp stamp_;DWORD owner_;U64 fileBytes_;bool active_,poisoned_;unsigned files_,windows_;HRESULT error_;
 Session(const Session&);Session&operator=(const Session&);
 static bool Take(U64&left,U64 n){if(n>left)return false;left-=n;return true;}
 bool InitialBudget(const Ledger&l)const{U64 left=SourceReceipts::CpuCap;return Take(left,Reservation(fileBytes_))&&Take(left,l.retainedRaw)&&Take(left,l.other);}
 bool WriteBudget(const WorldPixels::Accepted&p,const WorldHeight::Window&w,const Ledger&l)const{
  U64 left=SourceReceipts::CpuCap;return Take(left,Reservation(fileBytes_))&&Take(left,512ULL*512*4)&&Take(left,p.OwnedBytes())&&Take(left,w.OwnedBytes())&&Take(left,sizeof(w))&&Take(left,l.retainedRaw)&&Take(left,l.other);
 }
 void Release(){adapter_.Clear();for(unsigned i=0;i<16;++i){delete tiles_[i].file;tiles_[i].file=NULL;}files_=0;active_=false;factory_=NULL;}
 HRESULT Poison(HRESULT hr){poisoned_=true;error_=FAILED(hr)?hr:E_FAIL;for(unsigned i=0;i<16;++i)tiles_[i].writer.Cancel();Release();return error_;}
 HRESULT Refuse(HRESULT hr){error_=hr;return hr;}
 struct StopContext{HANDLE cancel;static bool Check(void*p){return WorldPixels::Cancelled(static_cast<StopContext*>(p)->cancel);}};
public:
 Session():factory_(NULL),owner_(0),fileBytes_(0),active_(false),poisoned_(false),files_(0),windows_(0),error_(S_OK){std::memset(&scope_,0,sizeof(scope_));std::memset(&stamp_,0,sizeof(stamp_));}
 ~Session(){Release();} // Caller must have joined and serialized its owning worker.
 static U64 Reservation(U64 bytes){return bytes>=sizeof(Gst2::RandomFile)&&bytes<=(SourceReceipts::CpuCap-sizeof(Session))/16?sizeof(Session)+16*bytes:0;}
 HRESULT Begin(bool enabled,const SourceReceipts::Scope&scope,U64 atlasMaterial,Factory&factory,const Ledger&ledger){
  if(owner_&&owner_!=GetCurrentThreadId())return E_ACCESSDENIED;if(owner_)return Refuse(E_UNEXPECTED);if(!enabled)return S_FALSE;
  if(!SourceReceipts::ScopeValid(scope)||!atlasMaterial)return Refuse(E_INVALIDARG);
  U64 bound=0;try{bound=factory.FileCpuBound();}catch(...){return Refuse(E_FAIL);}
  if(!Reservation(bound))return Refuse(E_OUTOFMEMORY);fileBytes_=bound;if(!InitialBudget(ledger))return Refuse(E_OUTOFMEMORY);
  scope_=scope;stamp_.source=scope.sourceEpoch;stamp_.material=atlasMaterial;stamp_.capture=scope.job;factory_=&factory;owner_=GetCurrentThreadId();active_=true;error_=S_OK;return S_OK;
 }
 HRESULT Write(bool enabled,const WorldPixels::Accepted&p,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame,const WorldHeight::Window&w,U64 atlasMaterial,HANDLE cancel,const Ledger&ledger,Location&out){
  std::memset(&out,0,sizeof(out));if(owner_!=GetCurrentThreadId())return E_ACCESSDENIED;if(!active_||poisoned_)return E_UNEXPECTED;
  if(!enabled)return Poison(E_ABORT);
  if(!SourceReceipts::SameScope(scope_,now)||atlasMaterial!=stamp_.material||frame.source.value!=stamp_.source||frame.generation!=stamp_.capture)return Poison(E_ABORT);
  if(WorldPixels::Cancelled(cancel))return Poison(HRESULT_FROM_WIN32(ERROR_CANCELLED));
  Location at={};if(!Locate(frame,at))return Refuse(E_INVALIDARG);
  Tile&t=tiles_[at.tileZ*4+at.tileX];const unsigned bit=(at.z/512)*16+at.x/512;const U64 mask=1ULL<<(bit%64);
  if(t.done[bit/64]&mask)return Refuse(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
  if(!WriteBudget(p,w,ledger))return Refuse(E_OUTOFMEMORY);
  Ledger adjusted=ledger;adjusted.other+=Reservation(fileBytes_)-sizeof(AcceptedWindow::Adapter);
  HRESULT hr=adapter_.Build(true,p,now,frame,w,cancel,adjusted);
  if(FAILED(hr)){adapter_.Clear();return hr==E_OUTOFMEMORY?Refuse(hr):Poison(hr);}
  AcceptedWindow::View view={};if(!adapter_.Acquire(true,p,now,frame,w,cancel,adjusted,view)){hr=adapter_.Error();adapter_.Clear();return Poison(hr);}
  try{
   if(!t.file){Gst2::RandomFile*file=factory_->OpenNew(at.tileX,at.tileZ);if(!file)return Poison(E_FAIL);
    // A factory must transfer a fresh uniquely owned object, never a shared file.
    for(unsigned i=0;i<16;++i)if(tiles_[i].file==file)return Poison(E_INVALIDARG);
    t.file=file;++files_;if(WorldPixels::Cancelled(cancel))return Poison(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    if(!t.writer.Begin(*t.file,at.tileX,at.tileZ,stamp_))return Poison(HRESULT_FROM_WIN32(t.writer.Error()?t.writer.Error():ERROR_WRITE_FAULT));
   }
   Gst2Windows::Frame destination={stamp_,at.tileX,at.tileZ,at.x,at.z};StopContext stop={cancel};
   for(unsigned plane=0;plane<3;++plane)if(!t.writer.WritePlane(plane,destination,view.Plane(plane),view.Bytes(plane),view.Pitch(plane),StopContext::Check,&stop))return Poison(HRESULT_FROM_WIN32(t.writer.Error()?t.writer.Error():ERROR_WRITE_FAULT));
   if(WorldPixels::Cancelled(cancel))return Poison(HRESULT_FROM_WIN32(ERROR_CANCELLED));
   t.done[bit/64]|=mask;++windows_;adapter_.Clear();out=at;error_=S_OK;return S_OK;
  }catch(...){return Poison(HRESULT_FROM_WIN32(ERROR_UNHANDLED_EXCEPTION));}
 }
 HRESULT Close(){if(owner_&&owner_!=GetCurrentThreadId())return E_ACCESSDENIED;Release();return poisoned_?error_:S_OK;}
 bool Healthy()const{return active_&&!poisoned_;}bool Poisoned()const{return poisoned_;}unsigned OpenFiles()const{return files_;}unsigned WindowCount()const{return Healthy()?windows_:0;}HRESULT Error()const{return error_;}
};
}
