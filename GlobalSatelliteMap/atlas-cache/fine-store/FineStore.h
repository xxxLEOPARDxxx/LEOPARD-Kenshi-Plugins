#pragma once
#include "../accepted-window/AcceptedWindow.h"
#include "../world-build-disk/WorldBuildDisk.h"
#include "../../atlas-terrain/fine-windows/FineWindowPlanner.h"

// One Session belongs to ONE file worker. It does not create threads or borrow
// a caller's future work. Caller must join that worker before changing Accepted,
// HeightWindow, Session or Config. No disk record is trusted across processes.
namespace FineStore {
typedef SourceReceipts::U64 U64;
enum { Slots=128, HeaderBytes=1024, PayloadBytes=256*256*16, FileBytes=HeaderBytes+PayloadBytes };
const U64 DiskCap=U64(Slots)*FileBytes;
typedef WorldBuildDisk::Config Config;
inline bool Configure(Config&c,const wchar_t*p,const wchar_t*d){return WorldBuildDisk::Configure(c,p,d);}
struct Identity { U64 sourceEpoch,materialGeneration; BYTE baseSha[32],sourceCatalogSha[32],materialSha[32]; };
inline bool Nonzero(const BYTE*p,unsigned n){for(unsigned i=0;i<n;++i)if(p[i])return true;return false;}
inline bool Valid(const Identity&i){return i.sourceEpoch&&i.materialGeneration&&Nonzero(i.baseSha,32)&&Nonzero(i.sourceCatalogSha,32)&&Nonzero(i.materialSha,32);}
inline bool Same(const Identity&a,const Identity&b){return a.sourceEpoch==b.sourceEpoch&&a.materialGeneration==b.materialGeneration&&!std::memcmp(a.baseSha,b.baseSha,32)&&!std::memcmp(a.sourceCatalogSha,b.sourceCatalogSha,32)&&!std::memcmp(a.materialSha,b.materialSha,32);}
inline bool Anchored(const FineWindows::Key&k,const Identity&i,const SourceReceipts::Scope&s,const WorldRaster::Frame&f){
 if(!Valid(i)||!FineWindows::Valid(k)||(k.spacing!=1&&k.spacing!=3)||k.stamp.source!=i.sourceEpoch||k.stamp.material!=i.materialGeneration||s.sourceEpoch!=i.sourceEpoch||!WorldPixels::ValidFrame(s,f)||f.side!=256||f.spacing!=float(k.spacing))return false;
 FineWindows::Rect r;if(!FineWindows::Bounds(k,r))return false;return f.x==float(r.x+r.w*.5)&&f.z==float(r.z+r.h*.5);
}
struct Ledger { U64 retainedRaw,other; Ledger():retainedRaw(0),other(0){} };
struct Receipt { unsigned slot; U64 session,serial; FineWindows::Key key; Identity identity; BYTE sha[32]; };
struct View { const BYTE*material; const BYTE*normal; const float*height; FineWindows::Key key; Identity identity; WorldRaster::Frame frame; WorldHeight::Identity heightSource; };
class Session;
class Blob {
 friend class Session; BYTE*data_; FineWindows::Key key_; Identity identity_; WorldRaster::Frame frame_; WorldHeight::Identity height_; U64 session_,serial_;
 Blob(const Blob&);Blob&operator=(const Blob&);
public:
 Blob():data_(NULL),session_(0),serial_(0){std::memset(&key_,0,sizeof(key_));std::memset(&identity_,0,sizeof(identity_));std::memset(&frame_,0,sizeof(frame_));std::memset(&height_,0,sizeof(height_));}
 ~Blob(){Clear();}void Clear(){delete[]data_;data_=NULL;session_=serial_=0;}
 U64 OwnedBytes()const{return data_?sizeof(Blob)+PayloadBytes:0;}
 bool Acquire(bool enabled,const FineWindows::Key&key,const Identity&identity,View&out)const{
  std::memset(&out,0,sizeof(out));if(!enabled||!data_||!FineWindows::Equal(key_,key)||!Same(identity_,identity))return false;
  out.material=data_;out.normal=data_+256*256*8;out.height=reinterpret_cast<const float*>(data_+256*256*12);out.key=key_;out.identity=identity_;out.frame=frame_;out.heightSource=height_;return true;
 }
};
namespace Detail {
inline void U32(BYTE*b,unsigned at,DWORD v){std::memcpy(b+at,&v,4);}inline void U64At(BYTE*b,unsigned at,U64 v){std::memcpy(b+at,&v,8);}
inline HANDLE Relative(HANDLE parent,const wchar_t*leaf,ACCESS_MASK access,ULONG disposition){
 typedef NTSTATUS(NTAPI*Create)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
 typedef ULONG(NTAPI*Error)(NTSTATUS);HMODULE m=GetModuleHandleW(L"ntdll.dll");Create create=reinterpret_cast<Create>(GetProcAddress(m,"NtCreateFile"));Error error=reinterpret_cast<Error>(GetProcAddress(m,"RtlNtStatusToDosError"));
 if(!create||!error){SetLastError(ERROR_PROC_NOT_FOUND);return INVALID_HANDLE_VALUE;}UNICODE_STRING name={};name.Buffer=const_cast<wchar_t*>(leaf);name.Length=USHORT(std::wcslen(leaf)*2);name.MaximumLength=name.Length+2;
 OBJECT_ATTRIBUTES object={};object.Length=sizeof(object);object.RootDirectory=parent;object.ObjectName=&name;object.Attributes=OBJ_CASE_INSENSITIVE;IO_STATUS_BLOCK io={};HANDLE h=INVALID_HANDLE_VALUE;
 NTSTATUS status=create(&h,access|SYNCHRONIZE,&object,&io,NULL,FILE_ATTRIBUTE_NORMAL,0,disposition,FILE_NON_DIRECTORY_FILE|FILE_OPEN_REPARSE_POINT|FILE_SYNCHRONOUS_IO_NONALERT,NULL,0);
 if(status<0){SetLastError(error(status));return INVALID_HANDLE_VALUE;}return h;
}
struct FileId { DWORD volume,high,low; };
inline bool SameId(const FileId&a,const FileId&b){return a.volume==b.volume&&a.high==b.high&&a.low==b.low;}
inline U64 NextSession(){static __declspec(align(8)) volatile LONGLONG next=0;return U64(InterlockedIncrement64(&next));}
inline bool FileInfo(HANDLE h,FileId&out,U64&bytes){BY_HANDLE_FILE_INFORMATION i={};LARGE_INTEGER n={};if(!GetFileInformationByHandle(h,&i)||!GetFileSizeEx(h,&n)||n.QuadPart<0||(i.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||i.nNumberOfLinks!=1)return false;out.volume=i.dwVolumeSerialNumber;out.high=i.nFileIndexHigh;out.low=i.nFileIndexLow;bytes=U64(n.QuadPart);return true;}
inline bool Io(HANDLE h,bool write,void*p,DWORD bytes,HANDLE cancel){BYTE*data=static_cast<BYTE*>(p);while(bytes){if(WorldPixels::Cancelled(cancel)){SetLastError(ERROR_CANCELLED);return false;}DWORD take=bytes>65536?65536:bytes,done=0;BOOL ok=write?WriteFile(h,data,take,&done,NULL):ReadFile(h,data,take,&done,NULL);if(!ok||!done||done>take){if(ok)SetLastError(write?ERROR_WRITE_FAULT:ERROR_HANDLE_EOF);return false;}data+=done;bytes-=done;}return !WorldPixels::Cancelled(cancel);}
inline bool Size(HANDLE h,U64 n){LARGE_INTEGER at;at.QuadPart=LONGLONG(n);return SetFilePointerEx(h,at,NULL,FILE_BEGIN)&&SetEndOfFile(h)&&SetFilePointerEx(h,LARGE_INTEGER(),NULL,FILE_BEGIN);}
inline bool Digest(const BYTE*header,const AcceptedWindow::View&v,BYTE(&full)[32],BYTE(&planes)[3][32]){AtlasView::Hash hash;if(!hash.Begin()||!hash.Add(header,HeaderBytes))return false;for(unsigned p=0;p<3;++p)if(!AtlasView::Digest(v.Plane(p),DWORD(v.Bytes(p)),planes[p])||!hash.Add(v.Plane(p),DWORD(v.Bytes(p))))return false;return hash.Finish(full);}
}
class Session {
 struct Record { bool valid; U64 serial,touch; FineWindows::Key key; Identity identity; WorldRaster::Frame frame; WorldHeight::Identity height; Detail::FileId id; BYTE header[HeaderBytes],sha[32],planes[3][32]; Record():valid(false),serial(0),touch(0){std::memset(header,0,sizeof(header));} } records_[Slots];
 Config config_;WorldBuildDisk::Guards guards_;HANDLE directory_,pendingFile_;DWORD owner_;SourceReceipts::Scope scope_;Identity identity_;U64 session_,serial_,clock_;bool active_,poisoned_,directoryCloseUnknown_;HRESULT error_;
 Session(const Session&);Session&operator=(const Session&);
 bool Owner()const{return owner_&&owner_==GetCurrentThreadId();}
 static bool Take(U64&left,U64 n){if(n>left)return false;left-=n;return true;}
 bool Budget(U64 owned,const Ledger&l)const{U64 left=SourceReceipts::CpuCap;return Take(left,sizeof(Session))&&Take(left,owned)&&Take(left,l.retainedRaw)&&Take(left,l.other);}
 HRESULT Fail(HRESULT hr,bool poison=false){error_=FAILED(hr)?hr:E_FAIL;if(poison)poisoned_=true;return error_;}
 HRESULT Last(bool poison=true){DWORD e=GetLastError();return Fail(HRESULT_FROM_WIN32(e?e:ERROR_INVALID_DATA),poison);}
 bool CloseFile(HANDLE file){if(CloseHandle(file))return true;pendingFile_=file;Last();return false;}
 bool Current()const{BY_HANDLE_FILE_INFORMATION i={};return directory_!=INVALID_HANDLE_VALUE&&guards_.Current()&&GetFileInformationByHandle(directory_,&i)&&(i.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)&&!(i.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT);}
 static void Leaf(unsigned slot,wchar_t(&leaf)[32]){_snwprintf_s(leaf,32,_TRUNCATE,L"fine_%03u.gsf",slot);}
 bool Remove(unsigned slot){Record&r=records_[slot];if(!r.valid)return true;wchar_t leaf[32];Leaf(slot,leaf);HANDLE h=Detail::Relative(directory_,leaf,DELETE|FILE_READ_ATTRIBUTES,FILE_OPEN);if(h==INVALID_HANDLE_VALUE)return false;Detail::FileId id={};U64 bytes=0;bool ok=Detail::FileInfo(h,id,bytes)&&bytes==FileBytes&&Detail::SameId(id,r.id);FILE_DISPOSITION_INFO info={TRUE};if(ok)ok=SetFileInformationByHandle(h,FileDispositionInfo,&info,sizeof(info))!=FALSE;if(!CloseFile(h))ok=false;if(ok)r.valid=false;return ok;}
 void Header(Record&r,unsigned slot){BYTE*b=r.header;std::memset(b,0,HeaderBytes);std::memcpy(b,"GSFINE01",8);Detail::U32(b,8,1);Detail::U32(b,12,HeaderBytes);Detail::U32(b,16,FileBytes);Detail::U32(b,20,slot);Detail::U64At(b,24,session_);Detail::U64At(b,32,r.serial);Detail::U32(b,40,r.key.spacing);Detail::U32(b,44,r.key.x);Detail::U32(b,48,r.key.z);Detail::U64At(b,56,r.identity.sourceEpoch);Detail::U64At(b,64,r.identity.materialGeneration);std::memcpy(b+72,r.identity.baseSha,32);std::memcpy(b+104,r.identity.sourceCatalogSha,32);std::memcpy(b+136,r.identity.materialSha,32);
  // Canonical fields, never compiler struct padding or uninitialized bytes.
  Detail::U64At(b,168,r.frame.source.value);Detail::U32(b,176,r.frame.source.profile);Detail::U64At(b,184,r.frame.materialGeneration);Detail::U64At(b,192,r.frame.generation);Detail::U32(b,200,r.frame.region);Detail::U32(b,204,r.frame.cell);Detail::U32(b,208,r.frame.side);const float scalars[7]={r.frame.x,r.frame.z,r.frame.spacing,r.frame.cameraY,r.frame.farClip,r.frame.waterHeight,r.frame.wetness};std::memcpy(b+212,scalars,sizeof(scalars));
  Detail::U64At(b,320,r.height.epoch);Detail::U32(b,328,r.height.volume);Detail::U32(b,332,r.height.indexHigh);Detail::U32(b,336,r.height.indexLow);Detail::U32(b,340,r.height.written.dwHighDateTime);Detail::U32(b,344,r.height.written.dwLowDateTime);std::memcpy(b+348,r.height.sha,32);Detail::U32(b,416,10);Detail::U32(b,420,28);Detail::U32(b,424,41);Detail::U32(b,428,256);}
public:
 Session():directory_(INVALID_HANDLE_VALUE),pendingFile_(INVALID_HANDLE_VALUE),owner_(0),session_(0),serial_(0),clock_(0),active_(false),poisoned_(false),directoryCloseUnknown_(false),error_(S_OK){std::memset(&config_,0,sizeof(config_));std::memset(&scope_,0,sizeof(scope_));std::memset(&identity_,0,sizeof(identity_));}
 ~Session(){if(directory_!=INVALID_HANDLE_VALUE)CloseHandle(directory_);} // worker joined AND NeedsRetention()==false are caller preconditions
 static U64 FixedCpuBytes(){return sizeof(Session);}static U64 PutCpuBytes(){return AcceptedWindow::Adapter::RequiredBytes(256)+HeaderBytes;}static U64 GetCpuBytes(){return sizeof(Blob)+PayloadBytes+HeaderBytes;}
 HRESULT Begin(bool enabled,const SourceReceipts::Scope&scope,const Identity&identity,const Config&config,const Ledger&ledger){
  if(owner_)return E_UNEXPECTED;if(!enabled)return S_FALSE;if(!SourceReceipts::ScopeValid(scope)||!Valid(identity)||scope.sourceEpoch!=identity.sourceEpoch||GetCurrentThreadId()==scope.thread)return E_INVALIDARG;if(!Budget(0,ledger))return E_OUTOFMEMORY;if(!FineStore::Configure(config_,config.allowedParent,config.directory))return E_INVALIDARG;
  owner_=GetCurrentThreadId();scope_=scope;identity_=identity;session_=Detail::NextSession();if(!session_)return Fail(E_UNEXPECTED,true);
  wchar_t prefix[221]={};unsigned n=WorldBuildDisk::Length(config_.allowedParent);std::memcpy(prefix,config_.allowedParent,(n+1)*2);prefix[3]=0;if(!guards_.Open(prefix))return Last();
  for(unsigned end=3;end<=n;++end)if(end==n||config_.allowedParent[end]==L'\\'){if(end==3)continue;std::memcpy(prefix,config_.allowedParent,end*2);prefix[end]=0;if(!guards_.Open(prefix))return Last();}
  if(!guards_.Current()||!guards_.CreateChild(config_.directory+n+(n==3?0:1),config_.directory))return Last();
  directory_=CreateFileW(config_.directory,FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,NULL);if(!Current())return Last();active_=true;return error_=S_OK;
 }
 HRESULT Put(bool enabled,const FineWindows::Key&key,const Identity&identity,const WorldPixels::Accepted&p,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame,const WorldHeight::Window&w,HANDLE cancel,const Ledger&ledger,Receipt&out){
  std::memset(&out,0,sizeof(out));if(!Owner())return E_ACCESSDENIED;if(!active_||poisoned_)return E_UNEXPECTED;if(!enabled||!SourceReceipts::SameScope(scope_,now)||!Same(identity_,identity))return Fail(E_ABORT);if(!Anchored(key,identity,now,frame))return Fail(E_INVALIDARG);if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
  U64 owned=PutCpuBytes()+p.OwnedBytes()+w.OwnedBytes()+sizeof(w);if(!Budget(owned,ledger))return Fail(E_OUTOFMEMORY);
  AcceptedWindow::Adapter adapter;AcceptedWindow::Ledger adjusted;adjusted.retainedRaw=ledger.retainedRaw;adjusted.other=ledger.other+sizeof(Session)+HeaderBytes;
  HRESULT hr=adapter.Build(true,p,now,frame,w,cancel,adjusted);if(FAILED(hr))return Fail(hr);AcceptedWindow::View view={};if(!adapter.Acquire(true,p,now,frame,w,cancel,adjusted,view))return Fail(adapter.Error());
  unsigned slot=Slots;for(unsigned i=0;i<Slots;++i)if(records_[i].valid&&FineWindows::Equal(records_[i].key,key)){slot=i;break;}
  if(slot==Slots)for(unsigned i=0;i<Slots;++i)if(!records_[i].valid){slot=i;break;}
  if(slot==Slots){slot=0;for(unsigned i=1;i<Slots;++i)if(records_[i].touch<records_[slot].touch)slot=i;}
  if(!Current())return Fail(E_ACCESSDENIED,true);if(!Remove(slot))return Last();Record&r=records_[slot];r.valid=false;r.key=key;r.identity=identity;r.frame=frame;r.height=view.heightSource;r.serial=++serial_;if(!r.serial)return Fail(E_UNEXPECTED,true);Header(r,slot);
  if(!Detail::Digest(r.header,view,r.sha,r.planes))return Fail(E_FAIL,true);wchar_t leaf[32];Leaf(slot,leaf);HANDLE file=Detail::Relative(directory_,leaf,GENERIC_READ|GENERIC_WRITE,FILE_CREATE);if(file==INVALID_HANDLE_VALUE)return Last();
  bool ok=Current()&&Detail::Size(file,FileBytes)&&Detail::Io(file,true,r.header,HeaderBytes,cancel);for(unsigned plane=0;plane<3&&ok;++plane)ok=Detail::Io(file,true,const_cast<void*>(view.Plane(plane)),DWORD(view.Bytes(plane)),cancel);U64 bytes=0;if(ok)ok=FlushFileBuffers(file)&&Detail::FileInfo(file,r.id,bytes)&&bytes==FileBytes&&!WorldPixels::Cancelled(cancel)&&Current();DWORD e=ok?ERROR_SUCCESS:GetLastError();if(!CloseFile(file)){ok=false;e=GetLastError();}
  if(!ok)return Fail(HRESULT_FROM_WIN32(e?e:ERROR_WRITE_FAULT),true);r.valid=true;r.touch=++clock_;out.slot=slot;out.session=session_;out.serial=r.serial;out.key=key;out.identity=identity;std::memcpy(out.sha,r.sha,32);return error_=S_OK;
 }
 HRESULT Get(bool enabled,const FineWindows::Key&key,const Identity&identity,HANDLE cancel,const Ledger&ledger,Blob&out){
  out.Clear();if(!Owner())return E_ACCESSDENIED;if(!active_||poisoned_)return E_UNEXPECTED;if(!enabled||!Same(identity_,identity)||!FineWindows::Valid(key)||(key.spacing!=1&&key.spacing!=3)||key.stamp.source!=identity.sourceEpoch||key.stamp.material!=identity.materialGeneration)return Fail(E_ABORT);
  if(!Budget(GetCpuBytes(),ledger))return Fail(E_OUTOFMEMORY);if(WorldPixels::Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));unsigned slot=Slots;for(unsigned i=0;i<Slots;++i)if(records_[i].valid&&FineWindows::Equal(records_[i].key,key)){slot=i;break;}if(slot==Slots)return S_FALSE;if(!Current())return Fail(E_ACCESSDENIED,true);
  Record&r=records_[slot];wchar_t leaf[32];Leaf(slot,leaf);HANDLE file=Detail::Relative(directory_,leaf,GENERIC_READ,FILE_OPEN);if(file==INVALID_HANDLE_VALUE)return Last();Detail::FileId id={};U64 bytes=0;BYTE header[HeaderBytes];bool ok=Detail::FileInfo(file,id,bytes)&&bytes==FileBytes&&Detail::SameId(id,r.id);if(ok)ok=Detail::Io(file,false,header,HeaderBytes,cancel)&&!std::memcmp(header,r.header,HeaderBytes);BYTE*data=NULL;if(ok){data=new(std::nothrow)BYTE[PayloadBytes];ok=data!=NULL;if(!ok)SetLastError(ERROR_NOT_ENOUGH_MEMORY);}if(ok)ok=Detail::Io(file,false,data,PayloadBytes,cancel);BYTE digest[32];if(ok){AtlasView::Hash hash;ok=hash.Begin()&&hash.Add(header,HeaderBytes)&&hash.Add(data,PayloadBytes)&&hash.Finish(digest)&&!std::memcmp(digest,r.sha,32);}
  const DWORD lengths[3]={256*256*8,256*256*4,256*256*4};DWORD at=0;for(unsigned plane=0;plane<3&&ok;++plane){ok=AtlasView::Digest(data+at,lengths[plane],digest)&&!std::memcmp(digest,r.planes[plane],32);at+=lengths[plane];}if(ok)ok=!WorldPixels::Cancelled(cancel)&&Current();DWORD e=ok?ERROR_SUCCESS:GetLastError();if(!CloseFile(file)){ok=false;e=GetLastError();}if(!ok){delete[]data;return Fail(HRESULT_FROM_WIN32(e?e:ERROR_CRC),true);}
  out.data_=data;out.key_=key;out.identity_=identity;out.frame_=r.frame;out.height_=r.height;out.session_=session_;out.serial_=r.serial;r.touch=++clock_;return error_=S_OK;
 }
 HRESULT Close(){if(!Owner())return E_ACCESSDENIED;active_=false;if(pendingFile_!=INVALID_HANDLE_VALUE){if(!CloseHandle(pendingFile_))return Last();pendingFile_=INVALID_HANDLE_VALUE;}if(directory_!=INVALID_HANDLE_VALUE){if(!CloseHandle(directory_)){directoryCloseUnknown_=true;return Last();}directory_=INVALID_HANDLE_VALUE;directoryCloseUnknown_=false;}return poisoned_?error_:S_OK;}
 bool NeedsRetention()const{return pendingFile_!=INVALID_HANDLE_VALUE||directoryCloseUnknown_;}
 bool Healthy()const{return active_&&!poisoned_;}bool Poisoned()const{return poisoned_;}HRESULT Error()const{return error_;}unsigned Records()const{unsigned n=0;for(unsigned i=0;i<Slots;++i)if(records_[i].valid)++n;return n;}
 bool Check(const Receipt&r)const{return Owner()&&active_&&!poisoned_&&r.slot<Slots&&r.session==session_&&records_[r.slot].valid&&records_[r.slot].serial==r.serial&&FineWindows::Equal(records_[r.slot].key,r.key)&&Same(records_[r.slot].identity,r.identity)&&!std::memcmp(records_[r.slot].sha,r.sha,32);}
};
}
