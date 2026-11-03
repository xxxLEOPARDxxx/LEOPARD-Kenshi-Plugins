#pragma once
// Private one-window integration. Never writes/advertises a GST/GSI atlas.
// A completed diagnostic file may survive a later context change; only a fresh
// joined Tick acknowledgment is success. Caller owns all active-world authority.
#include "../../../GlobalSatelliteMap/atlas-terrain/world-stream/WorldStream.h"
#include <string>
#include <cstdio>
namespace WorldFrameControl {
typedef SourceReceipts::U64 U64;
struct Header{
 char magic[8];DWORD version,side;U64 world,source,job,material;
 float x,z,spacing,cameraY,farClip,waterHeight,wetness;
 // materialSha hashes parameters[900] only, not distortion[60] or all material
 // authority. Current materialGeneration/provenance is still the caller's gate.
 DWORD formats[3],bytes[3];BYTE heightSha[32],materialSha[32];DWORD pid,token,reserved;
};
typedef char HeaderSize[(sizeof(Header)==176)?1:-1];
struct Export{
 Header header;const BYTE*plane[3];volatile LONG cancel,done;DWORD error;
 std::string directory;Export():cancel(0),done(0),error(0){std::memset(&header,0,sizeof(header));std::memset(plane,0,sizeof(plane));}
};
inline bool Cancelled(const Export&w){return InterlockedCompareExchange(const_cast<volatile LONG*>(&w.cancel),0,0)!=0;}
inline bool Write(HANDLE file,const void*data,DWORD bytes,Export&w){const BYTE*p=static_cast<const BYTE*>(data);for(DWORD at=0;at<bytes;){if(Cancelled(w)){w.error=ERROR_CANCELLED;return false;}DWORD take=bytes-at;if(take>65536)take=65536;DWORD got=0;if(!WriteFile(file,p+at,take,&got,NULL)||got!=take){w.error=GetLastError();if(!w.error)w.error=ERROR_WRITE_FAULT;return false;}at+=take;}return true;}
inline DWORD WINAPI Save(void*value){Export&w=*static_cast<Export*>(value);HANDLE file=INVALID_HANDLE_VALUE;try{
 if(Cancelled(w)){w.error=ERROR_CANCELLED;throw 1;}
 if(!CreateDirectoryA(w.directory.c_str(),NULL)){w.error=GetLastError();throw 1;}
 const std::string partial=w.directory+"/frame.gwf.partial",final=w.directory+"/frame.gwf";
 file=CreateFileA(partial.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE){w.error=GetLastError();throw 1;}
 bool ok=Write(file,&w.header,sizeof(w.header),w);for(unsigned i=0;i<3&&ok;++i)ok=Write(file,w.plane[i],w.header.bytes[i],w);
 if(ok&&!FlushFileBuffers(file)){w.error=GetLastError();ok=false;}CloseHandle(file);file=INVALID_HANDLE_VALUE;
 if(!ok)throw 1;if(Cancelled(w)){w.error=ERROR_CANCELLED;throw 1;}
 if(!MoveFileExA(partial.c_str(),final.c_str(),0)){w.error=GetLastError();throw 1;}
 }catch(...){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);if(!w.error)w.error=ERROR_INVALID_DATA;}
 InterlockedExchange(&w.done,1);return w.error;
}
inline WorldStream::QuarantineRecord&ExportQuarantine(){static WorldStream::QuarantineRecord record;return record;}
// Provider is Main-only. The export worker can reach copied scalar/header/path
// data and borrowed accepted CPU pixels only; no controller/provider/D3D pointer.
template<class Provider>class Controller{
 struct Owner{
  Provider provider;WorldStream::Controller<Provider> stream;WorldStream::Job job;
  WorldStream::CapturedSources tokens;Export output;HANDLE writer;bool submitted,stopping;
  BYTE shaderCopy[21920];
  Owner():stream(provider),writer(NULL),submitted(false),stopping(false){std::memset(&job,0,sizeof(job));}
 };
 Owner*owner_;DWORD thread_;bool quarantined_;HRESULT error_;std::string output_;
 Controller(const Controller&);Controller&operator=(const Controller&);
 bool OnMain()const{return GetCurrentThreadId()==thread_;}
 bool Current(const SourceReceipts::Scope&scope,const WorldRaster::Frame&frame)const{return owner_&&SourceReceipts::ScopeValid(scope)&&scope.thread==thread_&&SourceReceipts::SameScope(owner_->job.scope,scope)&&WorldRaster::SameFrame(owner_->job.frame,frame);}
 void Stop(){if(!owner_)return;owner_->stopping=true;InterlockedExchange(&owner_->output.cancel,1);if(!owner_->writer)owner_->stream.Cancel();}
 void Quarantine(){quarantined_=true;error_=E_FAIL;if(owner_)InterlockedExchange(&owner_->output.cancel,1);WorldStream::QuarantineRecord&q=ExportQuarantine();q.work=owner_;q.cpuBytes=CpuReservation();q.gpuBytes=owner_?owner_->stream.ReservedGpuBytes():0;InterlockedExchange(&q.marked,1);} // Retain complete owner below an unknown worker.
 bool JoinWriter(){if(!owner_->writer)return true;DWORD wait=WaitForSingleObject(owner_->writer,0);if(wait==WAIT_TIMEOUT)return false;if(wait!=WAIT_OBJECT_0){Quarantine();return false;}CloseHandle(owner_->writer);owner_->writer=NULL;return true;}
 void Finish(){owner_->stream.Shutdown();if(WorldStream::Quarantine().marked){Quarantine();return;}delete owner_;owner_=NULL;}
 HRESULT LaunchWriter(DWORD token){Owner&o=*owner_;const SourceReceipts::Scope&s=o.job.scope;const WorldRaster::Frame&f=o.job.frame;Header&h=o.output.header;
  std::memset(&h,0,sizeof(h));std::memcpy(h.magic,"GWSF0001",8);h.version=1;h.side=f.side;h.world=s.world;h.source=s.sourceEpoch;h.job=s.job;h.material=f.materialGeneration;h.x=f.x;h.z=f.z;h.spacing=f.spacing;h.cameraY=f.cameraY;h.farClip=f.farClip;h.waterHeight=f.waterHeight;h.wetness=f.wetness;h.pid=GetCurrentProcessId();h.token=token;
  std::memcpy(h.heightSha,WorldHeight::SourceSha,32);if(!SourceReceipts::Hash(reinterpret_cast<const BYTE*>(&o.job.material.parameters[0]),sizeof(o.job.material.parameters),h.materialSha))return E_FAIL;
  for(unsigned i=0;i<3;++i){o.output.plane[i]=o.stream.Plane(i,true,s,f);if(!o.output.plane[i])return E_ABORT;h.formats[i]=i==0?DXGI_FORMAT_R16G16B16A16_FLOAT:i==1?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R32_FLOAT;h.bytes[i]=f.side*f.side*(i?4:8);}
  o.writer=CreateThread(NULL,0,Save,&o.output,0,NULL);return o.writer?S_OK:HRESULT_FROM_WIN32(GetLastError());
 }
public:
 Controller():owner_(NULL),thread_(GetCurrentThreadId()),quarantined_(false),error_(S_OK){}
 ~Controller(){if(owner_&&!quarantined_){if(!OnMain()){Quarantine();return;}Stop();if(owner_->writer&&!JoinWriter()){Quarantine();return;}if(!quarantined_)Finish();}}
 bool Active()const{return owner_!=NULL;}bool Quarantined()const{return quarantined_;}HRESULT Error()const{return error_;}const std::string&Output()const{return output_;}
 bool Writing()const{return OnMain()&&owner_&&owner_->writer!=NULL;}
 U64 CpuReservation()const{return owner_?owner_->stream.CpuReservation()+sizeof(Owner)+sizeof(Controller)+owner_->output.directory.capacity()+output_.capacity()+2+1024:0;}
 HRESULT Begin(const WorldStream::Job&j,const std::string&directory){
  if(!OnMain()||owner_||quarantined_||ExportQuarantine().marked||WorldStream::Quarantine().marked||!directory.size()||directory.size()>220||!SourceReceipts::ScopeValid(j.scope)||j.scope.thread!=thread_||!WorldRaster::ValidShaders(j.shaders))return E_INVALIDARG;
  error_=S_OK;output_.clear();HRESULT hr=S_OK;
  try{
   owner_=new Owner;Owner&o=*owner_;o.job=j;output_=directory;o.output.directory=directory;
   std::memcpy(o.shaderCopy,j.shaders.vertex.data,4440);std::memcpy(o.shaderCopy+4440,j.shaders.partial.data,13208);std::memcpy(o.shaderCopy+17648,j.shaders.tail.data,4272);o.job.shaders.vertex.data=o.shaderCopy;o.job.shaders.partial.data=o.shaderCopy+4440;o.job.shaders.tail.data=o.shaderCopy+17648;
   if(CpuReservation()>SourceReceipts::CpuCap)hr=E_OUTOFMEMORY;else{hr=o.provider.Init(j.scope);if(SUCCEEDED(hr))hr=o.provider.CaptureSources(j.frame,j.plan,j.scope,o.tokens.token);if(SUCCEEDED(hr))hr=o.stream.Start();}
  }catch(const std::bad_alloc&){hr=E_OUTOFMEMORY;}catch(...){hr=E_FAIL;}
  if(FAILED(hr)){error_=hr;if(owner_)Finish();}return hr;
 }
 // Call only from the checked compositor callback. false withdraws the request.
 // Return1 once after joined completion;0 while active;-1 on terminal refusal.
 int Tick(bool admitted,ID3D11Device*device,DWORD token,const SourceReceipts::Scope&scope,const WorldRaster::Frame&frame){
  if(!OnMain()||!owner_||quarantined_)return -1;Owner&o=*owner_;
  if(!admitted||!device||!Current(scope,frame)){if(SUCCEEDED(error_))error_=E_ABORT;Stop();}
  if(o.writer){if(!JoinWriter())return quarantined_?-1:0;if(InterlockedCompareExchange(&o.output.done,0,0)!=1){error_=E_FAIL;Stop();}else if(o.output.error){error_=HRESULT_FROM_WIN32(o.output.error);Stop();}else if(!o.stopping&&admitted){HRESULT fresh=o.stream.Pump(true,device,scope,WorldBank::Ledger());if(FAILED(fresh)||!o.stream.Result(admitted,scope,frame)||o.output.header.token!=token){error_=FAILED(fresh)?fresh:E_ABORT;Stop();}else{o.stream.ReleaseResult();Finish();return quarantined_?-1:1;}}else Stop();}
  if(o.stopping){HRESULT hr=o.stream.Pump(false,NULL,o.job.scope,WorldBank::Ledger());if(WorldStream::Quarantine().marked){Quarantine();return -1;}if(FAILED(hr)&&SUCCEEDED(error_))error_=hr;if(o.stream.State()==WorldStream::Idle||o.stream.State()==WorldStream::Failed||o.stream.State()==WorldStream::Stopped){Finish();if(SUCCEEDED(error_))error_=E_ABORT;return -1;}return 0;}
  HRESULT hr=o.stream.Pump(true,device,scope,WorldBank::Ledger());if(FAILED(hr)){error_=hr;Stop();return 0;}
  if(o.stream.State()==WorldStream::Idle&&!o.submitted){hr=o.stream.Submit(true,o.job,o.tokens,o.job.scope);if(SUCCEEDED(hr))o.submitted=true;}
  else if(o.stream.State()==WorldStream::Complete){hr=o.stream.Result(admitted,scope,frame)?LaunchWriter(token):E_ABORT;if(FAILED(hr)){error_=hr;Stop();}}
  if(FAILED(hr)){error_=hr;Stop();}return 0;
 }
 void Cancel(){if(OnMain())Stop();}
};
}
