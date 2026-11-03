#pragma once
#include "../world-raster/WorldTerrainRaster.h"
#include "../world-source/WorldHeightSource.h"
#include "../png-source/PinnedPng.h"
#include "../source-receipts/MainGate.h"
namespace WorldStream {
typedef WorldBank::U64 U64;
enum Status{Stopped, Booting, Idle, LoadingHeight, LoadingSource, AcceptingSource, DecodingSource, UploadingSource, ReleasingSource, DrawingStage, AwaitingGpu, Complete, Cancelling, Failed};
struct Job{SourceReceipts::Scope scope;WorldRaster::Frame frame;WorldRaster::Material material;WorldRaster::Grid grid;WorldRaster::Shaders shaders;WorldMaterial::Plan plan;wchar_t heightPath[260];};
struct CapturedSources{SourceReceipts::MainToken token[4][15];}; // Main only.
enum WorkerState{WBoot,WIdle,WBusy,WHeight,WOpened,WDecoded,WReleased,WClosed,WError,WStopped};
enum Command{Height=1,Open,Decode,Release,Close,Stop};
// No Provider, native token, engine object or D3D object is reachable by Worker.
struct Work{
 HANDLE wake,cancel;volatile LONG state,command;HRESULT error;WorldHeight::Source height;WorldHeight::Window window;WorldHeight::Operation operation;SourceReceipts::WorkerFileLease lease;PinnedPng::Decoder decoder;std::vector<BYTE>rgba,pixels;
 wchar_t path[260],retainedPath[260];U64 epoch;unsigned side,pin,heightOpens;WorldHeight::Region region;SourceReceipts::WorkerRequest request;
 Work():wake(NULL),cancel(NULL),state(WBoot),command(0),error(S_OK),epoch(0),side(0),pin(0),heightOpens(0){path[0]=retainedPath[0]=0;std::memset(&request,0,sizeof(request));}
 bool Cancelled()const{return WaitForSingleObject(cancel,0)!=WAIT_TIMEOUT;}
 static void Observe(void*p){Work&w=*static_cast<Work*>(p);if(w.Cancelled())w.operation.Cancel();}
 void DropInput(){lease.Clear();std::vector<BYTE>().swap(rgba);}
 void DropHeight(){window.Clear();height.Close();retainedPath[0]=0;}
 static DWORD WINAPI Run(void*p){Work&w=*static_cast<Work*>(p);w.error=w.decoder.Init();InterlockedExchange(&w.state,FAILED(w.error)?WError:WIdle);
  for(;;){if(WaitForSingleObject(w.wake,INFINITE)!=WAIT_OBJECT_0){w.error=E_FAIL;break;}const LONG command=InterlockedExchange(&w.command,0);if(command==Stop)break;
   HRESULT hr=S_OK;LONG done=WIdle;try{
    if(command==Close){w.DropInput();w.DropHeight();std::vector<BYTE>().swap(w.pixels);done=WClosed;}
    else if(command==Release){w.DropInput();done=WReleased;}
    else if(w.Cancelled())hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);
    else if(command==Height){w.DropInput();w.operation.Reset();w.operation.Observe(Observe,&w);if(w.Cancelled())w.operation.Cancel();
     if(w.height.Ready()&&(w.height.Stamp().epoch!=w.epoch||std::wcscmp(w.path,w.retainedPath)))w.DropHeight();
     if(!w.height.Ready()){if(!w.height.Open(w.path,w.epoch,w.operation))hr=HRESULT_FROM_WIN32(w.operation.Error()?w.operation.Error():ERROR_CANCELLED);else{std::wcscpy(w.retainedPath,w.path);++w.heightOpens;}}
     if(SUCCEEDED(hr)&&!w.height.Read(w.region,w.epoch,w.window,w.operation))hr=HRESULT_FROM_WIN32(w.operation.Error()?w.operation.Error():ERROR_CANCELLED);
     if(SUCCEEDED(hr)){w.pixels.resize(size_t(w.side)*w.side*16);done=WHeight;}
    }else if(command==Open){w.DropInput();hr=w.lease.Open(w.request,w.cancel,WorldHeight::MaxSamples*2+512ULL*512*16+16ULL*1024*1024+sizeof(Work));done=WOpened;}
    else if(command==Decode){if(w.pin>=129||!w.lease.Data())hr=E_INVALIDARG;else{const WorldBank::RgbaPin&pin=WorldBank::RgbaPins[w.pin];w.rgba.resize(pin.bytes);hr=w.decoder.Decode(pin,w.lease.Data(),size_t(w.lease.Request().pin.bytes),&w.rgba[0],w.rgba.size(),w.cancel);}done=WDecoded;}
    else hr=E_INVALIDARG;
   }catch(...){hr=E_FAIL;}w.error=hr;InterlockedExchange(&w.state,FAILED(hr)?WError:done);
  }w.DropInput();w.DropHeight();std::vector<BYTE>().swap(w.pixels);w.decoder.Clear();InterlockedExchange(&w.state,WStopped);return 0;
 }
};
inline bool Add(U64&value,U64 amount){if(value>~U64(0)-amount)return false;value+=amount;return true;}
inline void Hex(const char*p,BYTE(&out)[32]){for(unsigned i=0;i<32;++i){unsigned a=p[i*2],b=p[i*2+1];a=a<='9'?a-'0':a-'a'+10;b=b<='9'?b-'0':b-'a'+10;out[i]=BYTE(a*16+b);}}
struct GpuOwners{WorldBank::Sources bank;WorldRaster::Raster raster;AtlasView::Com<ID3D11Device>device;AtlasView::Com<ID3D11DeviceContext>context;AtlasView::Com<ID3D11Texture2D>staging[3];AtlasView::Com<ID3D11Query>query;};
struct QuarantineRecord{void*work;void*gpu;U64 cpuBytes,gpuBytes;volatile LONG marked;QuarantineRecord():work(NULL),gpu(NULL),cpuBytes(0),gpuBytes(0),marked(0){}};
inline volatile LONG&WorkerSlot(){static volatile LONG slot=0;return slot;}
inline QuarantineRecord&Quarantine(){static QuarantineRecord q;return q;}
template<class Provider>class Controller{
 GpuOwners*gpu_;Provider&provider_;SourceReceipts::MainGate<Provider>gate_;DWORD owner_;Status state_;HRESULT error_;Work*work_;HANDLE thread_;bool quarantined_,submitted_,queryPending_,closeSent_;U64 bankEpoch_;Job job_;CapturedSources captured_;SourceReceipts::AcceptedReceipt accepted_[15];unsigned slot_,input_;BYTE shaderCopy_[21920];WorldBank::Sources&bank_;WorldRaster::Raster&raster_;AtlasView::Com<ID3D11Device>&device_;AtlasView::Com<ID3D11DeviceContext>&context_;AtlasView::Com<ID3D11Texture2D>*staging_;AtlasView::Com<ID3D11Query>&query_;
 Controller(const Controller&);Controller&operator=(const Controller&);
 LONG WorkerStatus()const{return work_?InterlockedCompareExchange(&work_->state,0,0):WStopped;}
 bool Send(Command command){InterlockedExchange(&work_->state,WBusy);InterlockedExchange(&work_->command,command);if(SetEvent(work_->wake))return true;work_->error=E_FAIL;InterlockedExchange(&work_->state,WError);return false;}
 bool OnMain()const{return GetCurrentThreadId()==owner_;}
 bool Matches(const SourceReceipts::Scope&now)const{return SourceReceipts::ScopeValid(now)&&SourceReceipts::SameScope(job_.scope,now)&&now.thread==owner_;}
 unsigned PinIndex()const{return input_==2?128:input_*64+job_.frame.region;}
 void FrameClear(){for(unsigned i=0;i<3;++i)staging_[i].Reset();query_.Reset();raster_.Clear();context_.Reset();queryPending_=false;}
 void GpuClear(){FrameClear();bank_.Clear();device_.Reset();bankEpoch_=0;}
 HRESULT Fence(){if(!context_.p||!query_.p)return S_OK;if(!queryPending_){WorldBank::Predicate pred(context_.p);context_.p->End(query_.p);context_.p->Flush();queryPending_=true;}return S_OK;}
 HRESULT Drained(){if(!queryPending_)return S_OK;HRESULT hr=device_.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;return context_.p->GetData(query_.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);}
 HRESULT Refuse(HRESULT hr){error_=FAILED(hr)?hr:E_FAIL;Cancel();return error_;}
 void QuarantineOwners(){quarantined_=true;state_=Failed;QuarantineRecord&q=Quarantine();q.work=work_;q.gpu=gpu_;q.cpuBytes=CpuReservation();q.gpuBytes=MaximumGpuReservation();InterlockedExchange(&q.marked,1);}
 void ReleaseJoinedWorker(){CloseHandle(thread_);thread_=NULL;CloseHandle(work_->wake);CloseHandle(work_->cancel);delete work_;work_=NULL;InterlockedExchange(&WorkerSlot(),0);}
 HRESULT InitializeGpu(ID3D11Device*d,const WorldBank::Ledger&ledger){
  if(device_.p&&(!bank_.SameDevice(d)||bankEpoch_!=job_.frame.source.value))GpuClear();
  if(!device_.p){device_.p=d;d->AddRef();}d->GetImmediateContext(&context_.p);if(!context_.p)return E_FAIL;
  D3D11_QUERY_DESC q={D3D11_QUERY_EVENT,0};HRESULT hr=d->CreateQuery(&q,&query_.p);
  if(SUCCEEDED(hr)&&!bank_.OwnedGpuBytes()){hr=bank_.Init(d,job_.frame.source,ledger);if(SUCCEEDED(hr))bankEpoch_=job_.frame.source.value;}
  if(SUCCEEDED(hr))hr=raster_.Init(d,job_.frame.side,job_.shaders,work_->window.View(),job_.grid,ledger);
  for(unsigned p=0;p<3&&SUCCEEDED(hr);++p){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=job_.frame.side;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;t.Format=p==0?DXGI_FORMAT_R16G16B16A16_FLOAT:p==1?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R32_FLOAT;hr=d->CreateTexture2D(&t,NULL,&staging_[p].p);}
  return hr;
 }
 bool ValidRequest(unsigned slot,unsigned index,const SourceReceipts::WorkerRequest&r)const{
  if(!SourceReceipts::Valid(r)||!SourceReceipts::SameScope(r.scope,job_.scope)||r.role.slot!=slot)return false;
  const char*path=NULL;const char*sha=NULL;unsigned bytes=0,kind=0,layer=0;
  if(index<3){const WorldBank::RgbaPin&p=WorldBank::RgbaPins[index==2?128:index*64+job_.frame.region];path=p.path;sha=p.encodedSha;bytes=p.encodedBytes;kind=index==0?SourceReceipts::PaintedOverlay:index==1?SourceReceipts::PaintedColour:SourceReceipts::Blend;}
  else{unsigned k=(index-3)/6;layer=(index-3)%6;const WorldMaterial::Bank&b=job_.plan.stage[slot].banks[k];if(!b.count||b.count>6)return false;const SatelliteRegion::Asset&a=SatelliteRegion::Data.assets[b.upload[layer<b.count?layer:0]];path=a.path;sha=a.sha256;bytes=a.bytes;kind=k?SourceReceipts::Normal:SourceReceipts::Diffuse;}
  BYTE pin[32];Hex(sha,pin);return r.role.kind==kind&&r.role.index==layer&&r.pin.bytes==bytes&&!std::strcmp(r.resource,path)&&!std::memcmp(r.pin.sha,pin,32);
 }
 HRESULT RequestInput(){if(!captured_.token[slot_][input_].Request(work_->request)||!ValidRequest(slot_,input_,work_->request))return E_INVALIDARG;if(!Send(Open))return E_FAIL;state_=LoadingSource;return S_OK;}
 HRESULT NextInput(const SourceReceipts::Scope&now,const WorldBank::Ledger&l){
  if(++input_<15)return RequestInput();HRESULT hr=gate_.CommitBank(accepted_,15,now);if(SUCCEEDED(hr))hr=bank_.Complete(job_.frame.source,slot_,l);
  if(SUCCEEDED(hr)&&slot_==0)hr=raster_.BeginFrame(true,job_.frame,job_.plan,job_.material,bank_,l);if(SUCCEEDED(hr))state_=DrawingStage;return hr;
 }
public:
 explicit Controller(Provider&p):gpu_(new GpuOwners),provider_(p),gate_(p),owner_(GetCurrentThreadId()),state_(Stopped),error_(S_OK),work_(NULL),thread_(NULL),quarantined_(false),submitted_(false),queryPending_(false),closeSent_(false),bankEpoch_(0),slot_(0),input_(0),bank_(gpu_->bank),raster_(gpu_->raster),device_(gpu_->device),context_(gpu_->context),staging_(gpu_->staging),query_(gpu_->query){std::memset(&job_,0,sizeof(job_));state_=Idle;}
 ~Controller(){Shutdown();if(!quarantined_)delete gpu_;}
 Status State()const{return state_;}HRESULT Error()const{return error_;}
 U64 CpuReservation()const{return sizeof(Controller)+sizeof(GpuOwners)+sizeof(Work)+WorldHeight::MaxSamples*2+16ULL*1024*1024*2+512ULL*512*16+WorldHeight::AuthScratch;}
 U64 OwnedGpuBytes()const{return bank_.OwnedGpuBytes()+raster_.OwnedGpuBytes()+U64(job_.frame.side)*job_.frame.side*((staging_[0].p?8:0)+(staging_[1].p?4:0)+(staging_[2].p?4:0));}
 static U64 MaximumGpuReservation(){return WorldBank::SourceBytes+WorldRaster::RequiredBytes(512,16)+512ULL*512*16;}
 U64 ReservedGpuBytes()const{return quarantined_||queryPending_||OwnedGpuBytes()||(state_!=Idle&&state_!=Booting&&state_!=Stopped&&state_!=Failed)?MaximumGpuReservation():0;}
 bool CountedLedger(const WorldBank::Ledger&external,WorldBank::Ledger&out)const{out=external;return Add(out.carry,WorldRaster::CarryBytes(512))&&Add(out.raster,WorldRaster::MeshBytes(16))&&Add(out.outputs,WorldRaster::TargetBytes(512))&&Add(out.readback,512ULL*512*16)&&WorldBank::Fits(out);}
 HRESULT Start(){if(!OnMain()||work_||thread_||quarantined_||CpuReservation()>SourceReceipts::CpuCap||InterlockedCompareExchange(&WorkerSlot(),1,0)!=0)return E_UNEXPECTED;Work*w=new(std::nothrow)Work;if(!w){InterlockedExchange(&WorkerSlot(),0);return E_OUTOFMEMORY;}w->wake=CreateEventA(NULL,FALSE,FALSE,NULL);w->cancel=CreateEventA(NULL,TRUE,FALSE,NULL);if(!w->wake||!w->cancel){if(w->wake)CloseHandle(w->wake);if(w->cancel)CloseHandle(w->cancel);delete w;InterlockedExchange(&WorkerSlot(),0);return E_FAIL;}HANDLE thread=CreateThread(NULL,0,Work::Run,w,0,NULL);if(!thread){CloseHandle(w->wake);CloseHandle(w->cancel);delete w;InterlockedExchange(&WorkerSlot(),0);return E_FAIL;}work_=w;thread_=thread;state_=Booting;return S_OK;}
 HRESULT Submit(bool enabled,const Job&j,const CapturedSources&tokens,const SourceReceipts::Scope&now){
  if(!enabled)return S_FALSE;if(!OnMain()||state_!=Idle||!work_||WorkerStatus()!=WIdle||quarantined_||!SourceReceipts::ScopeValid(now)||now.thread!=owner_||!SourceReceipts::SameScope(j.scope,now)||j.frame.generation!=now.job||!j.frame.materialGeneration||j.material.generation!=j.frame.materialGeneration||!WorldBank::SameEpoch(j.frame.source,j.material.source)||j.frame.region!=j.material.region||j.frame.source.value!=now.sourceEpoch||!WorldBank::ValidEpoch(j.frame.source)||!WorldRaster::ValidGrid(j.grid)||j.grid.sourceEpoch!=now.sourceEpoch||!WorldRaster::ValidShaders(j.shaders)||!j.heightPath[0]||j.heightPath[259]||!WorldRaster::CarryBytes(j.frame.side))return E_INVALIDARG;
  WorldMaterial::Planner planner;WorldMaterial::Plan expected;if(!planner.Init(now.sourceEpoch)||!planner.Build(j.frame.region,j.frame.cell,now.sourceEpoch,0,expected)||!WorldBank::EqualPlan(j.plan,expected))return E_INVALIDARG;
  job_=j;for(unsigned s=0;s<expected.sidCount;++s)for(unsigned i=0;i<15;++i){SourceReceipts::WorkerRequest r;if(!tokens.token[s][i].Request(r)||!ValidRequest(s,i,r)){std::memset(&job_,0,sizeof(job_));return E_INVALIDARG;}}
  std::memcpy(shaderCopy_,j.shaders.vertex.data,4440);std::memcpy(shaderCopy_+4440,j.shaders.partial.data,13208);std::memcpy(shaderCopy_+17648,j.shaders.tail.data,4272);job_.shaders.vertex.data=shaderCopy_;job_.shaders.partial.data=shaderCopy_+4440;job_.shaders.tail.data=shaderCopy_+17648;
  captured_=tokens;error_=S_OK;submitted_=true;closeSent_=false;slot_=input_=0;ResetEvent(work_->cancel);std::wcscpy(work_->path,j.heightPath);work_->epoch=now.sourceEpoch;work_->side=j.frame.side;work_->region.x0=j.grid.x;work_->region.z0=j.grid.z;work_->region.x1=j.grid.x+j.grid.columns*64;work_->region.z1=j.grid.z+j.grid.rows*64;if(!Send(Height))return Refuse(E_FAIL);state_=LoadingHeight;return S_OK;
 }
 void Cancel(){if(!OnMain()||!work_||quarantined_)return;if(state_==Booting||state_==Stopped)return;if(state_==Idle&&!bank_.OwnedGpuBytes()&&WorkerStatus()==WIdle&&!work_->height.Ready()&&!work_->pixels.capacity())return;SetEvent(work_->cancel);work_->operation.Cancel();raster_.Cancel();Fence();state_=Cancelling;}
 HRESULT Pump(bool enabled,ID3D11Device*d,const SourceReceipts::Scope&now,const WorldBank::Ledger&external){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(quarantined_)return E_FAIL;if(!work_)return E_UNEXPECTED;LONG w=WorkerStatus();DWORD alive=WaitForSingleObject(thread_,0);if(alive==WAIT_FAILED){Fence();QuarantineOwners();return E_FAIL;}bool exited=alive==WAIT_OBJECT_0;
  if(state_==Booting&&!exited){if(w==WBoot)return S_FALSE;if(w==WError){error_=work_->error;state_=Failed;return error_;}state_=Idle;return S_OK;}
  if(exited&&state_!=Cancelling){error_=FAILED(work_->error)?work_->error:E_FAIL;raster_.Cancel();Fence();state_=Cancelling;}
  if(state_==Idle){if(!enabled||now.world!=job_.scope.world||now.sourceEpoch!=job_.scope.sourceEpoch)Cancel();if(state_==Idle)return S_FALSE;}
  if(state_==Stopped||state_==Failed)return S_FALSE;
  if(state_==Cancelling){HRESULT drained=Drained();if(drained==S_FALSE||(!exited&&w==WBusy))return S_FALSE;if(FAILED(drained)&&device_.p&&SUCCEEDED(device_.p->GetDeviceRemovedReason())){QuarantineOwners();return E_FAIL;}if(exited){GpuClear();ReleaseJoinedWorker();state_=Failed;return error_;}if(!closeSent_){if(!Send(Close)){GpuClear();QuarantineOwners();return E_FAIL;}closeSent_=true;return S_FALSE;}if(w!=WClosed)return S_FALSE;GpuClear();submitted_=false;captured_=CapturedSources();for(unsigned i=0;i<15;++i)accepted_[i].Clear();InterlockedExchange(&work_->state,WIdle);ResetEvent(work_->cancel);closeSent_=false;state_=Idle;return S_OK;}
  if(!enabled||!Matches(now)||!d||(device_.p&&!bank_.SameDevice(d)))return Refuse(E_ABORT);
  WorldBank::Ledger l;if(!CountedLedger(external,l))return Refuse(E_OUTOFMEMORY);if(w==WError)return Refuse(work_->error);
  HRESULT hr=S_OK;
  try{
   if(state_==LoadingHeight&&w==WHeight){hr=InitializeGpu(d,l);if(SUCCEEDED(hr))hr=bank_.BeginStage(job_.plan,job_.frame.source,0,l);if(SUCCEEDED(hr))hr=RequestInput();}
   else if(state_==LoadingSource&&w==WOpened){hr=gate_.Accept(captured_.token[slot_][input_],work_->lease,now,accepted_[input_]);if(SUCCEEDED(hr)){if(input_<3&&bank_.NeedsRgba(input_)){work_->pin=PinIndex();if(!Send(Decode))hr=E_FAIL;else state_=DecodingSource;}else if(input_<3){if(!Send(Release))hr=E_FAIL;else state_=ReleasingSource;}else state_=UploadingSource;}}
   else if(state_==DecodingSource&&w==WDecoded)state_=UploadingSource;
   else if(state_==UploadingSource){if(input_<3)hr=bank_.UploadRgba(job_.frame.source,slot_,input_,&work_->rgba[0],work_->rgba.size(),l);else hr=bank_.UploadDds(job_.frame.source,slot_,(input_-3)/6,(input_-3)%6,work_->lease.Data(),size_t(work_->lease.Request().pin.bytes),l);if(SUCCEEDED(hr)){if(!Send(Release))hr=E_FAIL;else state_=ReleasingSource;}}
   else if(state_==ReleasingSource&&w==WReleased)hr=NextInput(now,l);
   else if(state_==DrawingStage){hr=raster_.RenderStage(slot_,bank_,l);if(SUCCEEDED(hr)){if(++slot_<4){if(slot_<job_.plan.sidCount){input_=0;for(unsigned i=0;i<15;++i)accepted_[i].Clear();hr=bank_.BeginStage(job_.plan,job_.frame.source,slot_,l);if(SUCCEEDED(hr))hr=RequestInput();}}else{hr=raster_.Finish(bank_,l);if(SUCCEEDED(hr)){WorldBank::Predicate pred(context_.p);for(unsigned p=0;p<3;++p)context_.p->CopyResource(staging_[p].p,raster_.Texture(p));Fence();state_=AwaitingGpu;}}}}
   else if(state_==AwaitingGpu){hr=Drained();if(hr==S_FALSE)return S_FALSE;if(SUCCEEDED(hr)){for(unsigned p=0;p<3;++p){D3D11_MAPPED_SUBRESOURCE m={};hr=context_.p->Map(staging_[p].p,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&m);if(FAILED(hr))break;unsigned stride=job_.frame.side*(p?4:8);size_t at=p==0?0:size_t(job_.frame.side)*job_.frame.side*(p==1?8:12);for(unsigned y=0;y<job_.frame.side;++y)std::memcpy(&work_->pixels[at+size_t(y)*stride],static_cast<const BYTE*>(m.pData)+size_t(y)*m.RowPitch,stride);context_.p->Unmap(staging_[p].p,0);}if(SUCCEEDED(hr)){queryPending_=false;state_=Complete;}}}
  }catch(...){hr=E_FAIL;}return FAILED(hr)?Refuse(hr):S_OK;
 }
 const Job*Result(bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame)const{return enabled&&OnMain()&&state_==Complete&&Matches(now)&&WorldRaster::SameFrame(job_.frame,frame)?&job_:NULL;}
 const BYTE*Plane(unsigned p,bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&frame)const{if(!Result(enabled,now,frame)||p>=3)return NULL;size_t n=size_t(job_.frame.side)*job_.frame.side;return &work_->pixels[n*(p==0?0:p==1?8:12)];}
 unsigned HeightAuthentications()const{return OnMain()&&work_&&(state_==Idle||state_==Complete)?work_->heightOpens:0;}
 void ReleaseResult(){if(!OnMain()||state_!=Complete)return;FrameClear();captured_=CapturedSources();submitted_=false;InterlockedExchange(&work_->state,WIdle);state_=Idle;}
 void Shutdown(){if(!OnMain()||!work_||quarantined_)return;SetEvent(work_->cancel);work_->operation.Cancel();InterlockedExchange(&work_->command,Stop);SetEvent(work_->wake);Fence();DWORD start=GetTickCount(),wait=WAIT_TIMEOUT;HRESULT drained=S_FALSE;do{wait=WaitForSingleObject(thread_,0);drained=Drained();if(wait==WAIT_OBJECT_0&&drained!=S_FALSE)break;if(wait==WAIT_FAILED)break;Sleep(1);}while(GetTickCount()-start<5000);if(wait!=WAIT_OBJECT_0||drained==S_FALSE||(FAILED(drained)&&device_.p&&SUCCEEDED(device_.p->GetDeviceRemovedReason()))){error_=E_FAIL;QuarantineOwners();return;}ReleaseJoinedWorker();GpuClear();state_=Stopped;}
};
}
