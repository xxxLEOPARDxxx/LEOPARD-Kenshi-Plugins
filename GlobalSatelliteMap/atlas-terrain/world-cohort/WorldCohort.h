#pragma once
#include "../world-stream/WorldStream.h"
namespace WorldCohort {
typedef WorldBank::U64 U64;
using WorldStream::Work;
using WorldStream::CapturedSources;
enum Status{Stopped,Booting,Idle,LoadingHeight,InitializingViews,LoadingSource,DecodingSource,UploadingSource,ReleasingSource,BeginningViews,DrawingStage,FinishingViews,QueueReadback,AwaitingGpu,ValidatingPixels,Complete,Cancelling,Failed,Stopping};
struct Job{
 SourceReceipts::Scope scope;WorldRaster::Material material;WorldRaster::Shaders shaders;WorldMaterial::Plan plan;
 wchar_t heightPath[260];unsigned count;WorldRaster::Frame frame[4];WorldRaster::Grid grid[4];
};
struct Borrowed{const WorldPixels::Accepted*accepted;const WorldHeight::Window*window;U64 retainedRawBytes,otherCpuBytes,controllerCpuBytes;SourceReceipts::Scope scope;WorldRaster::Frame frame;};
inline bool CountValid(unsigned count){return count==2||count==4;}
// Conservative side512/16-patch admission, including ONE shared staging trio.
inline U64 GpuReservation(unsigned count){return CountValid(count)?WorldBank::SourceBytes+U64(count)*WorldRaster::RequiredBytes(512,16)+512ULL*512*16:0;}
inline bool ValidJob(const Job&j){
 if(!CountValid(j.count)||!SourceReceipts::ScopeValid(j.scope)||!j.heightPath[0]||j.heightPath[259]||!WorldRaster::ValidShaders(j.shaders))return false;
 const WorldRaster::Frame&first=j.frame[0];if(!WorldPixels::ValidFrame(j.scope,first)||!WorldBank::SameEpoch(first.source,j.material.source)||first.materialGeneration!=j.material.generation||first.region!=j.material.region)return false;
 const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[first.region];const unsigned cellX=unsigned((r.worldX+147456)/18)+(first.cell%4)*512,cellZ=unsigned((r.worldZ+147456)/18)+(first.cell/4)*512;
 for(unsigned n=0;n<j.count;++n){
  const WorldRaster::Frame&f=j.frame[n];const WorldRaster::Grid&g=j.grid[n];
  if(!WorldPixels::ValidFrame(j.scope,f)||!WorldBank::SameEpoch(f.source,first.source)||f.region!=first.region||f.cell!=first.cell||f.side!=first.side||f.spacing!=first.spacing||f.materialGeneration!=first.materialGeneration||f.cameraY!=first.cameraY||f.farClip!=first.farClip||f.waterHeight!=first.waterHeight||f.wetness!=first.wetness||!WorldRaster::ValidGrid(g)||g.sourceEpoch!=j.scope.sourceEpoch||g.x<cellX||g.z<cellZ||g.x+g.columns*64>cellX+512||g.z+g.rows*64>cellZ+512)return false;
  const double half=double(f.side)*f.spacing*.5;
  if(f.x-half<double(g.x)*18-147456||f.z-half<double(g.z)*18-147456||f.x+half>double(g.x+g.columns*64)*18-147456||f.z+half>double(g.z+g.rows*64)*18-147456)return false;
  for(unsigned k=0;k<n;++k)if(f.x==j.frame[k].x&&f.z==j.frame[k].z)return false;
 }
 for(unsigned n=0;n<900;++n)if(!_finite(j.material.parameters[n]))return false;
 for(unsigned n=0;n<60;++n)if(!_finite(j.material.distortion[n]))return false;
 WorldMaterial::Planner planner;WorldMaterial::Plan expected;return planner.Init(j.scope.sourceEpoch)&&planner.Build(first.region,first.cell,j.scope.sourceEpoch,0,expected)&&WorldBank::EqualPlan(j.plan,expected);
}
struct GpuOwners{WorldBank::Sources bank;WorldRaster::Raster raster[4];AtlasView::Com<ID3D11Device>device;AtlasView::Com<ID3D11DeviceContext>context;AtlasView::Com<ID3D11Texture2D>staging[3];AtlasView::Com<ID3D11Query>query;};
struct QuarantineRecord{Work*work;GpuOwners*gpu;HANDLE thread;U64 cpuBytes,gpuBytes;volatile LONG marked;QuarantineRecord():work(NULL),gpu(NULL),thread(NULL),cpuBytes(0),gpuBytes(0),marked(0){}};
inline QuarantineRecord&Quarantine(){static QuarantineRecord q;return q;}
template<class Provider>class Controller{
 GpuOwners*gpu_;Provider&provider_;SourceReceipts::MainGate<Provider>gate_;DWORD owner_;Status state_;HRESULT error_;Work*work_;HANDLE thread_;
 bool quarantined_,queryPending_,closeSent_;U64 bankEpoch_;Job job_;WorldNative::SourceFactory factory_;CapturedSources captured_;SourceReceipts::AcceptedReceipt accepted_[15];
 unsigned slot_,input_,cursor_,output_;DWORD shutdownAt_;BYTE shaderCopy_[21920];
 Controller(const Controller&);Controller&operator=(const Controller&);
 bool OnMain()const{return GetCurrentThreadId()==owner_;}
 LONG WorkerStatus()const{return work_?InterlockedCompareExchange(&work_->state,0,0):WorldStream::WStopped;}
 bool Matches(const SourceReceipts::Scope&now)const{return OnMain()&&SourceReceipts::ScopeValid(now)&&SourceReceipts::SameScope(job_.scope,now);}
 bool Send(WorldStream::Command command){InterlockedExchange(&work_->state,WorldStream::WBusy);InterlockedExchange(&work_->command,command);if(SetEvent(work_->wake))return true;work_->error=E_FAIL;InterlockedExchange(&work_->state,WorldStream::WError);return false;}
 void CancelRasters(){for(unsigned n=0;n<4;++n)gpu_->raster[n].Cancel();}
 void FrameClear(){for(unsigned n=0;n<4;++n)gpu_->raster[n].Clear();for(unsigned n=0;n<3;++n)gpu_->staging[n].Reset();gpu_->query.Reset();gpu_->context.Reset();queryPending_=false;}
 void GpuClear(){FrameClear();gpu_->bank.Clear();gpu_->device.Reset();bankEpoch_=0;}
 HRESULT Fence(){if(gpu_->context.p&&gpu_->query.p&&!queryPending_){WorldBank::Predicate pred(gpu_->context.p);gpu_->context.p->End(gpu_->query.p);gpu_->context.p->Flush();queryPending_=true;}return S_OK;}
 HRESULT Drained(){if(!queryPending_)return S_OK;HRESULT hr=gpu_->device.p->GetDeviceRemovedReason();return FAILED(hr)?hr:gpu_->context.p->GetData(gpu_->query.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);}
 HRESULT Refuse(HRESULT hr){
  error_=FAILED(hr)?hr:E_FAIL;
#ifdef WORLD_COHORT_FAILURE
  // Optional private-caller diagnostics, only on refusal and before Cancel
  // replaces the failing state. No callback, allocation or logging by default.
  try{WORLD_COHORT_FAILURE(job_.frame[0].region,job_.frame[0].cell,state_,slot_,input_,cursor_,output_,error_);}catch(...){}
#endif
  Cancel();return error_;
 }
 void QuarantineOwners(){quarantined_=true;state_=Failed;QuarantineRecord&q=Quarantine();q.work=work_;q.gpu=gpu_;q.thread=thread_;q.cpuBytes=CpuReservation();q.gpuBytes=MaximumGpuReservation();InterlockedExchange(&q.marked,1);}
 void ReleaseJoinedWorker(){CloseHandle(thread_);thread_=NULL;CloseHandle(work_->wake);CloseHandle(work_->cancel);delete work_;work_=NULL;InterlockedExchange(&WorldStream::WorkerSlot(),0);}
 bool ValidRequest(unsigned slot,unsigned index,const SourceReceipts::WorkerRequest&r)const{return SourceReceipts::Valid(r)&&SourceReceipts::SameScope(r.scope,job_.scope)&&r.role.slot==slot&&WorldNative::InputIndex(r.role)==index&&factory_.Matches(r);}
 HRESULT RequestInput(){if(!captured_.token[slot_][input_].Request(work_->request)||!ValidRequest(slot_,input_,work_->request))return E_INVALIDARG;if(!Send(WorldStream::Open))return E_FAIL;state_=LoadingSource;return S_OK;}
 HRESULT NextInput(const SourceReceipts::Scope&now,const WorldBank::Ledger&l){
  if(++input_<15)return RequestInput();HRESULT hr=gate_.CommitBank(accepted_,15,now);if(hr==HRESULT_FROM_WIN32(ERROR_BUSY)){--input_;return S_FALSE;}if(SUCCEEDED(hr))hr=gpu_->bank.Complete(job_.frame[0].source,slot_,l);
  if(SUCCEEDED(hr)){cursor_=0;state_=slot_==0?BeginningViews:DrawingStage;}return hr;
 }
 HRESULT InitializeGpu(ID3D11Device*d,const WorldBank::Ledger&l){
  if(gpu_->device.p&&(!gpu_->bank.SameDevice(d)||bankEpoch_!=job_.scope.sourceEpoch))GpuClear();
  if(!gpu_->device.p){gpu_->device.p=d;d->AddRef();}d->GetImmediateContext(&gpu_->context.p);if(!gpu_->context.p)return E_FAIL;
  D3D11_QUERY_DESC q={D3D11_QUERY_EVENT,0};HRESULT hr=d->CreateQuery(&q,&gpu_->query.p);
  if(SUCCEEDED(hr)&&!gpu_->bank.OwnedGpuBytes()){hr=gpu_->bank.Init(d,job_.frame[0].source,l);if(SUCCEEDED(hr))bankEpoch_=job_.scope.sourceEpoch;}
  for(unsigned p=0;p<3&&SUCCEEDED(hr);++p){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=job_.frame[0].side;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;t.Format=p==0?DXGI_FORMAT_R16G16B16A16_FLOAT:p==1?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R32_FLOAT;hr=d->CreateTexture2D(&t,NULL,&gpu_->staging[p].p);}return hr;
 }
public:
 explicit Controller(Provider&p):gpu_(new GpuOwners),provider_(p),gate_(p),owner_(GetCurrentThreadId()),state_(Stopped),error_(S_OK),work_(NULL),thread_(NULL),quarantined_(false),queryPending_(false),closeSent_(false),bankEpoch_(0),slot_(0),input_(0),cursor_(0),output_(0),shutdownAt_(0){std::memset(&job_,0,sizeof(job_));}
 ~Controller(){Shutdown();if(!quarantined_)delete gpu_;}
 Status State()const{return state_;}HRESULT Error()const{return error_;}bool Quarantined()const{return quarantined_;}unsigned CurrentView()const{return output_;}unsigned CurrentStage()const{return slot_;}unsigned StageCursor()const{return cursor_;}
 unsigned CurrentInput()const{return input_;}LONG ProgressWorkerState()const{return OnMain()?WorkerStatus():WorldStream::WStopped;}
 U64 CpuReservation()const{return sizeof(Controller)+sizeof(GpuOwners)+sizeof(Work)+WorldHeight::MaxSamples*2+32ULL*1024*1024+512ULL*512*16+WorldHeight::AuthScratch+WorldPixels::Accepted::RequiredBytes(512)-sizeof(WorldPixels::Accepted);}
 U64 MaximumGpuReservation()const{return GpuReservation(CountValid(job_.count)?job_.count:4);}
 U64 OwnedGpuBytes()const{U64 n=gpu_->bank.OwnedGpuBytes();for(unsigned k=0;k<4;++k)n+=gpu_->raster[k].OwnedGpuBytes();for(unsigned p=0;p<3;++p)if(gpu_->staging[p].p)n+=U64(job_.frame[0].side)*job_.frame[0].side*(p?4:8);return n;}
 U64 ReservedGpuBytes()const{return quarantined_||queryPending_||OwnedGpuBytes()||(state_!=Idle&&state_!=Booting&&state_!=Stopped&&state_!=Failed&&state_!=Stopping)?MaximumGpuReservation():0;}
 bool KnownStoppedAndJoined()const{return OnMain()&&!quarantined_&&!work_&&!thread_&&!queryPending_&&(state_==Stopped||state_==Failed)&&OwnedGpuBytes()==0;}
 bool CountedLedger(const WorldBank::Ledger&external,WorldBank::Ledger&out)const{out=external;const unsigned n=CountValid(job_.count)?job_.count:4;return WorldStream::Add(out.carry,U64(n)*WorldRaster::CarryBytes(512))&&WorldStream::Add(out.raster,U64(n)*WorldRaster::MeshBytes(16))&&WorldStream::Add(out.outputs,U64(n)*WorldRaster::TargetBytes(512))&&WorldStream::Add(out.readback,512ULL*512*16)&&WorldBank::Fits(out);}
 HRESULT Start(){
  if(!OnMain()||work_||thread_||quarantined_||CpuReservation()>SourceReceipts::CpuCap||InterlockedCompareExchange(&WorldStream::WorkerSlot(),1,0)!=0)return E_UNEXPECTED;
  Work*w=new(std::nothrow)Work;if(!w){InterlockedExchange(&WorldStream::WorkerSlot(),0);return E_OUTOFMEMORY;}w->wake=CreateEventA(NULL,FALSE,FALSE,NULL);w->cancel=CreateEventA(NULL,TRUE,FALSE,NULL);
  if(!w->wake||!w->cancel){if(w->wake)CloseHandle(w->wake);if(w->cancel)CloseHandle(w->cancel);delete w;InterlockedExchange(&WorldStream::WorkerSlot(),0);return E_FAIL;}
  HANDLE thread=CreateThread(NULL,0,Work::Run,w,0,NULL);if(!thread){CloseHandle(w->wake);CloseHandle(w->cancel);delete w;InterlockedExchange(&WorldStream::WorkerSlot(),0);return E_FAIL;}work_=w;thread_=thread;error_=S_OK;shutdownAt_=0;state_=Booting;return S_OK;
 }
 HRESULT Submit(bool enabled,const Job&j,const CapturedSources&tokens,const SourceReceipts::Scope&now){
  if(!enabled)return S_FALSE;if(!OnMain()||state_!=Idle||!work_||WorkerStatus()!=WorldStream::WIdle||quarantined_||now.thread!=owner_||!SourceReceipts::SameScope(j.scope,now)||!ValidJob(j))return E_INVALIDARG;
  // Validate all requests before modifying the retained job or any source bank.
  WorldNative::SourceFactory factory;if(FAILED(factory.Init(j.frame[0],j.plan,j.scope)))return E_INVALIDARG;
  for(unsigned s=0;s<j.plan.sidCount;++s)for(unsigned i=0;i<15;++i){SourceReceipts::WorkerRequest r;if(!tokens.token[s][i].Request(r)||!SourceReceipts::SameScope(r.scope,j.scope)||r.role.slot!=s||WorldNative::InputIndex(r.role)!=i||!factory.Matches(r))return E_INVALIDARG;}
  job_=j;factory_.Clear();if(FAILED(factory_.Init(job_.frame[0],job_.plan,job_.scope)))return E_INVALIDARG;
  std::memcpy(shaderCopy_,j.shaders.vertex.data,4440);std::memcpy(shaderCopy_+4440,j.shaders.partial.data,13208);std::memcpy(shaderCopy_+17648,j.shaders.tail.data,4272);job_.shaders.vertex.data=shaderCopy_;job_.shaders.partial.data=shaderCopy_+4440;job_.shaders.tail.data=shaderCopy_+17648;
  captured_=tokens;error_=S_OK;closeSent_=false;slot_=input_=cursor_=output_=0;ResetEvent(work_->cancel);std::wcscpy(work_->path,j.heightPath);work_->epoch=now.sourceEpoch;work_->side=j.frame[0].side;
  const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[j.frame[0].region];work_->region.x0=(r.worldX+147456)/18+(j.frame[0].cell%4)*512;work_->region.z0=(r.worldZ+147456)/18+(j.frame[0].cell/4)*512;work_->region.x1=work_->region.x0+512;work_->region.z1=work_->region.z0+512;
  if(!Send(WorldStream::Height))return Refuse(E_FAIL);state_=LoadingHeight;return S_OK;
 }
 void Cancel(){if(!OnMain()||!work_||quarantined_||state_==Booting||state_==Stopped||state_==Stopping)return;if(state_==Idle&&!gpu_->bank.OwnedGpuBytes()&&WorkerStatus()==WorldStream::WIdle&&!work_->height.Ready()&&!work_->pixels.capacity())return;SetEvent(work_->cancel);work_->operation.Cancel();CancelRasters();Fence();state_=Cancelling;}
 HRESULT Pump(bool enabled,ID3D11Device*d,const SourceReceipts::Scope&now,const WorldBank::Ledger&external){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(state_==Stopping)return PollShutdown();if(quarantined_)return E_FAIL;if(!work_)return E_UNEXPECTED;LONG w=WorkerStatus();DWORD alive=WaitForSingleObject(thread_,0);if(alive==WAIT_FAILED){Fence();QuarantineOwners();return E_FAIL;}const bool exited=alive==WAIT_OBJECT_0;
  if(state_==Booting&&!exited){if(w==WorldStream::WBoot)return S_FALSE;if(w==WorldStream::WError){error_=work_->error;state_=Failed;return error_;}state_=Idle;return S_OK;}
  // Latch abnormal exit even when explicit Cancel preceded its observation.
  if(exited){error_=FAILED(work_->error)?work_->error:E_FAIL;if(state_!=Cancelling){CancelRasters();Fence();state_=Cancelling;}}
  if(state_==Idle){if(!enabled||now.world!=job_.scope.world||now.sourceEpoch!=job_.scope.sourceEpoch)Cancel();if(state_==Idle)return S_FALSE;}
  if(state_==Stopped||state_==Failed)return S_FALSE;
  if(state_==Cancelling){HRESULT drained=Drained();if(drained==S_FALSE||(!exited&&w==WorldStream::WBusy))return S_FALSE;if(FAILED(drained)&&gpu_->device.p&&SUCCEEDED(gpu_->device.p->GetDeviceRemovedReason())){QuarantineOwners();return E_FAIL;}if(exited){GpuClear();ReleaseJoinedWorker();state_=Failed;return error_;}if(!closeSent_){if(!Send(WorldStream::Close)){QuarantineOwners();return E_FAIL;}closeSent_=true;return S_FALSE;}if(w!=WorldStream::WClosed)return S_FALSE;GpuClear();captured_=CapturedSources();for(unsigned i=0;i<15;++i)accepted_[i].Clear();InterlockedExchange(&work_->state,WorldStream::WIdle);ResetEvent(work_->cancel);closeSent_=false;state_=Idle;return S_OK;}
  if(!enabled||!Matches(now)||!d||(gpu_->device.p&&!gpu_->bank.SameDevice(d)))return Refuse(E_ABORT);
  WorldBank::Ledger l;if(!CountedLedger(external,l))return Refuse(E_OUTOFMEMORY);if(w==WorldStream::WError)return Refuse(work_->error);HRESULT hr=S_OK;bool progress=true;
  try{
   if(state_==LoadingHeight&&w==WorldStream::WHeight){hr=InitializeGpu(d,l);if(SUCCEEDED(hr)){cursor_=0;state_=InitializingViews;}}
   else if(state_==InitializingViews){hr=gpu_->raster[cursor_].Init(d,job_.frame[cursor_].side,job_.shaders,work_->window.View(),job_.grid[cursor_],l);if(SUCCEEDED(hr)&&++cursor_==job_.count){hr=gpu_->bank.BeginStage(job_.plan,job_.frame[0].source,0,l);if(SUCCEEDED(hr))hr=RequestInput();}}
   else if(state_==LoadingSource&&w==WorldStream::WOpened){hr=gate_.Accept(captured_.token[slot_][input_],work_->lease,now,accepted_[input_]);if(hr==HRESULT_FROM_WIN32(ERROR_BUSY))return S_FALSE;if(SUCCEEDED(hr)){if(input_<3&&gpu_->bank.NeedsRgba(input_)){work_->pin=input_==2?128:input_*64+job_.frame[0].region;if(!Send(WorldStream::Decode))hr=E_FAIL;else state_=DecodingSource;}else if(input_<3){if(!Send(WorldStream::Release))hr=E_FAIL;else state_=ReleasingSource;}else state_=UploadingSource;}}
   else if(state_==DecodingSource&&w==WorldStream::WDecoded)state_=UploadingSource;
   else if(state_==UploadingSource){if(input_<3)hr=gpu_->bank.UploadRgba(job_.frame[0].source,slot_,input_,&work_->rgba[0],work_->rgba.size(),l);else hr=gpu_->bank.UploadDds(job_.frame[0].source,slot_,(input_-3)/6,(input_-3)%6,work_->lease.Data(),size_t(work_->lease.Request().pin.bytes),l);if(SUCCEEDED(hr)){if(!Send(WorldStream::Release))hr=E_FAIL;else state_=ReleasingSource;}}
   else if(state_==ReleasingSource&&w==WorldStream::WReleased)hr=NextInput(now,l);
   else if(state_==BeginningViews){hr=gpu_->raster[cursor_].BeginFrame(true,job_.frame[cursor_],job_.plan,job_.material,gpu_->bank,l);if(SUCCEEDED(hr)&&++cursor_==job_.count){cursor_=0;state_=DrawingStage;}}
   else if(state_==DrawingStage){hr=gpu_->raster[cursor_].RenderStage(slot_,gpu_->bank,l);if(SUCCEEDED(hr)&&++cursor_==job_.count){cursor_=0;if(++slot_<4){if(slot_<job_.plan.sidCount){input_=0;for(unsigned i=0;i<15;++i)accepted_[i].Clear();hr=gpu_->bank.BeginStage(job_.plan,job_.frame[0].source,slot_,l);if(SUCCEEDED(hr))hr=RequestInput();}}else state_=FinishingViews;}}
   else if(state_==FinishingViews){hr=gpu_->raster[cursor_].Finish(gpu_->bank,l);if(SUCCEEDED(hr)&&++cursor_==job_.count){output_=0;state_=QueueReadback;}}
   else if(state_==QueueReadback){WorldBank::Predicate pred(gpu_->context.p);for(unsigned p=0;p<3;++p)gpu_->context.p->CopyResource(gpu_->staging[p].p,gpu_->raster[output_].Texture(p));Fence();state_=AwaitingGpu;}
   else if(state_==AwaitingGpu){hr=Drained();if(hr==S_FALSE)return S_FALSE;if(SUCCEEDED(hr)){for(unsigned p=0;p<3;++p){D3D11_MAPPED_SUBRESOURCE m={};hr=gpu_->context.p->Map(gpu_->staging[p].p,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&m);if(FAILED(hr))break;const unsigned side=job_.frame[output_].side,stride=side*(p?4:8);const size_t at=size_t(side)*side*(p==0?0:p==1?8:12);for(unsigned y=0;y<side;++y)std::memcpy(&work_->pixels[at+size_t(y)*stride],static_cast<const BYTE*>(m.pData)+size_t(y)*m.RowPitch,stride);gpu_->context.p->Unmap(gpu_->staging[p].p,0);}if(SUCCEEDED(hr)){queryPending_=false;work_->pixelScope=job_.scope;work_->pixelFrame=job_.frame[output_];work_->pixelOtherCpu=CpuReservation()-WorldPixels::Accepted::RequiredBytes(512);if(!Send(WorldStream::ValidatePixels))hr=E_FAIL;else state_=ValidatingPixels;}}}
   else if(state_==ValidatingPixels&&w==WorldStream::WValidated){WorldPixels::View view={};if(!work_->accepted.Acquire(true,now,job_.frame[output_],work_->window,view))hr=E_ABORT;else state_=Complete;}
   else progress=false;
  }catch(...){hr=E_FAIL;}return FAILED(hr)?Refuse(hr):(progress?S_OK:S_FALSE);
 }
 const WorldRaster::Frame*Result(unsigned view,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame)const{if(!enabled||!OnMain()||state_!=Complete||view!=output_||view>=job_.count||!Matches(now)||!WorldRaster::SameFrame(job_.frame[view],frame)||WorkerStatus()!=WorldStream::WValidated)return NULL;WorldPixels::View accepted={};return work_->accepted.Acquire(true,now,frame,work_->window,accepted)?&job_.frame[view]:NULL;}
 const BYTE*Plane(unsigned view,unsigned p,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame)const{if(p>=3||!Result(view,enabled,now,frame))return NULL;WorldPixels::View accepted={};if(!work_->accepted.Acquire(true,now,frame,work_->window,accepted))return NULL;return p==0?accepted.material:p==1?accepted.normal:reinterpret_cast<const BYTE*>(accepted.height);}
 const BYTE*RawPlane(unsigned view,unsigned p,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame)const{if(p>=3||!Result(view,enabled,now,frame))return NULL;return &work_->pixels[size_t(frame.side)*frame.side*(p==0?0:p==1?8:12)];}
 // CPU-only borrow. Caller must keep this controller alive and MUST NOT Pump,
 // Cancel, ReleaseView, Shutdown or destroy it until every consumer is joined.
 // To cancel a consumer, signal its own cancellation and keep joining it first.
 bool Borrow(unsigned view,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame,Borrowed&out)const{
  std::memset(&out,0,sizeof(out));if(!Result(view,enabled,now,frame))return false;
  const U64 cpu=CpuReservation(),raw=work_->pixels.capacity(),accepted=work_->accepted.OwnedBytes(),window=work_->window.OwnedBytes();
  if(cpu>SourceReceipts::CpuCap||raw>512ULL*512*16||accepted!=WorldPixels::Accepted::RequiredBytes(frame.side)||window>WorldHeight::MaxSamples*2)return false;
  U64 remaining=cpu;const U64 charged[]={accepted,window,sizeof(WorldHeight::Window),raw};for(unsigned n=0;n<4;++n){if(charged[n]>remaining)return false;remaining-=charged[n];}
  out.accepted=&work_->accepted;out.window=&work_->window;out.retainedRawBytes=raw;out.otherCpuBytes=remaining;out.controllerCpuBytes=cpu;out.scope=now;out.frame=frame;return true;
 }
 HRESULT ReleaseView(unsigned view,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(!Result(view,enabled,now,frame))return E_ABORT;
  // Caller must first join every consumer borrowing Plane/RawPlane memory.
  work_->accepted.Clear();InterlockedExchange(&work_->state,WorldStream::WIdle);
  if(++output_<job_.count)state_=QueueReadback;else{FrameClear();captured_=CapturedSources();for(unsigned i=0;i<15;++i)accepted_[i].Clear();state_=Idle;}return S_OK;
 }
 unsigned HeightAuthentications()const{return OnMain()&&work_&&(state_==Idle||state_==Complete)?work_->heightOpens:0;}
 // Asynchronous retirement is deliberately narrower than final teardown:
 // Main has already joined all consumers, drained GPU work and closed the bank.
 // No blocking waits, source reads or GPU operations occur in these two methods.
 HRESULT RequestShutdown(){
  if(!OnMain())return RPC_E_WRONG_THREAD;
  if(quarantined_||state_!=Idle||!work_||!thread_||queryPending_||gpu_->query.p||ReservedGpuBytes()||WorkerStatus()!=WorldStream::WIdle||work_->accepted.Ready())return E_UNEXPECTED;
  const DWORD wait=WaitForSingleObject(thread_,0);
  if(wait!=WAIT_TIMEOUT){error_=wait==WAIT_OBJECT_0&&FAILED(work_->error)?work_->error:E_FAIL;QuarantineOwners();return error_;}
  shutdownAt_=GetTickCount();state_=Stopping;
  InterlockedExchange(&work_->command,WorldStream::Stop);
  // Once stop is published, even a wake failure must go through PollShutdown:
  // the owner and global worker slot remain live until a proven join or quarantine.
  if(!SetEvent(work_->wake))error_=E_FAIL;
  return S_OK;
 }
 HRESULT PollShutdown(){
  if(!OnMain())return RPC_E_WRONG_THREAD;
  if(quarantined_)return FAILED(error_)?error_:E_FAIL;
  if(!work_&&!thread_&&(state_==Stopped||state_==Failed))return error_;
  if(state_!=Stopping||!work_||!thread_)return E_UNEXPECTED;
  const DWORD wait=WaitForSingleObject(thread_,0);
  if(wait==WAIT_OBJECT_0){
   if(WorkerStatus()!=WorldStream::WStopped){error_=E_FAIL;QuarantineOwners();return error_;}
   // Work owns the terminal HRESULT. Latch it BEFORE deleting joined owners.
   if(FAILED(work_->error))error_=work_->error;
   ReleaseJoinedWorker();state_=FAILED(error_)?Failed:Stopped;return error_;
  }
  if(wait!=WAIT_TIMEOUT||DWORD(GetTickCount()-shutdownAt_)>=5000){error_=wait==WAIT_TIMEOUT?HRESULT_FROM_WIN32(ERROR_TIMEOUT):E_FAIL;QuarantineOwners();return error_;}
  return S_FALSE;
 }
 void Shutdown(){
  if(!OnMain()||!work_||quarantined_)return;SetEvent(work_->cancel);work_->operation.Cancel();InterlockedExchange(&work_->command,WorldStream::Stop);SetEvent(work_->wake);Fence();DWORD start=GetTickCount(),wait=WAIT_TIMEOUT;HRESULT drained=S_FALSE;
  do{wait=WaitForSingleObject(thread_,0);drained=Drained();if(wait==WAIT_OBJECT_0&&drained!=S_FALSE)break;if(wait==WAIT_FAILED)break;Sleep(1);}while(GetTickCount()-start<5000);
  if(wait!=WAIT_OBJECT_0||drained==S_FALSE||(FAILED(drained)&&gpu_->device.p&&SUCCEEDED(gpu_->device.p->GetDeviceRemovedReason()))){error_=E_FAIL;QuarantineOwners();return;}ReleaseJoinedWorker();GpuClear();state_=Stopped;
 }
};
}
