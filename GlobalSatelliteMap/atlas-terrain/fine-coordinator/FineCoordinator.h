#pragma once
#include "../fine-viewer/FineJob.h"
#include "../world-native-rebind/NativeRebind.h"
#include "../../atlas-cache/fine-worker/FineWorker.h"
namespace FineCoordinator {
typedef FineStore::U64 U64;
struct Residency {U64 viewerGpu,fineGpu,pendingGpu,retiredGpu;unsigned occupied;Residency():viewerGpu(0),fineGpu(0),pendingGpu(0),retiredGpu(0),occupied(0){}};
// Serialized Main progress, not a completion/authentication receipt. Explicit
// fields avoid hashes/padding and observe both source and file-worker readiness.
struct Progress {
 unsigned nativeState,view,stage,cursor,input,ownerFlags,stored;
 LONG nativeWorkerState,consumerWorkerState;HRESULT error;
 Progress():nativeState(0),view(0),stage(0),cursor(0),input(0),ownerFlags(0),stored(0),nativeWorkerState(0),consumerWorkerState(0),error(S_OK){}
};
inline bool SameProgress(const Progress&a,const Progress&b){return a.nativeState==b.nativeState&&a.view==b.view&&a.stage==b.stage&&a.cursor==b.cursor&&a.input==b.input&&a.ownerFlags==b.ownerFlags&&a.stored==b.stored&&a.nativeWorkerState==b.nativeWorkerState&&a.consumerWorkerState==b.consumerWorkerState&&a.error==b.error;}
// Match MakeBudget's viewer/debt attribution without treating retained fine
// images as viewer allocations. All measured owners and the actual K2 bank
// still have to fit the shared GPU cap before starting another producer.
inline bool StartupFits(const Residency&r){
 if(r.viewerGpu>FineJob::ViewerAllowance||r.occupied>FineJob::ActiveSlots()||
    r.fineGpu<U64(r.occupied)*FineJob::WindowBytes||r.fineGpu>U64(FineJob::ActiveSlots())*FineJob::WindowBytes)return false;
 U64 viewer=r.viewerGpu;
 if(r.pendingGpu>FineJob::ViewerAllowance-viewer)return false;viewer+=r.pendingGpu;
 if(r.retiredGpu>FineJob::ViewerAllowance-viewer)return false;viewer+=r.retiredGpu;
 return viewer<=AtlasView::GpuCap&&r.fineGpu<=AtlasView::GpuCap-viewer&&
    WorldCohort::GpuReservation(2)<=AtlasView::GpuCap-viewer-r.fineGpu;
}

template<class NativeSession>class Controller {
 typedef WorldCohort::Controller<NativeSession> Producer;
 struct Owner {
  NativeSession native;FineWrite::Worker worker;FineJob::Plan plan;WorldCohort::Job job;WorldStream::CapturedSources tokens;BYTE shaders[21920];
  SourceReceipts::Scope scope;FineStore::Identity identity;FineStore::Ledger external;FineWindows::Key writingKey,readingKey;
  bool workerReady,queued,writing,reading,readReady,readExposed,draining,soft,producerCancelled,stopping,terminated;unsigned pair,stored;DWORD stopAt;
  Owner():workerReady(false),queued(false),writing(false),reading(false),readReady(false),readExposed(false),draining(false),soft(false),producerCancelled(false),stopping(false),terminated(false),pair(0),stored(0),stopAt(0){std::memset(&scope,0,sizeof(scope));std::memset(&identity,0,sizeof(identity));std::memset(&job,0,sizeof(job));std::memset(&writingKey,0,sizeof(writingKey));std::memset(&readingKey,0,sizeof(readingKey));}
 };
 struct Retained {Owner*owner;U64 cpu,gpu;Retained():owner(NULL),cpu(0),gpu(0){}};
 static Retained&Debt(){static Retained d;return d;}
 Owner*owner_;DWORD thread_;HRESULT error_;bool quarantined_;
 Controller(const Controller&);Controller&operator=(const Controller&);
 bool Main()const{return GetCurrentThreadId()==thread_;}
 static bool Add(U64&a,U64 b,U64 cap){if(b>cap||a>cap-b)return false;a+=b;return true;}
 static U64 Extra(){return sizeof(Controller)+sizeof(Owner)-sizeof(NativeSession)-sizeof(FineWrite::Worker);}
 bool CurrentBase(const FineJob::Demand&now,const FineStore::Identity&id)const{return owner_&&FineJob::Internal::DemandValid(now)&&SourceReceipts::SameScope(owner_->scope,now.scope)&&FineStore::Same(owner_->identity,id)&&now.view.stamp.source==id.sourceEpoch&&now.view.stamp.material==id.materialGeneration;}
 bool CpuLedger(bool wholeNative,const FineStore::Ledger&external,FineStore::Ledger&out)const{out=external;return Add(out.other,Extra(),SourceReceipts::CpuCap)&&Add(out.other,wholeNative?owner_->native.CpuReservation():owner_->native.ExtraCpuPeakBytes(),SourceReceipts::CpuCap);}
 bool GpuLedger(const Residency&r,WorldBank::Ledger&out)const{std::memset(&out,0,sizeof(out));out.retained=r.viewerGpu;if(!Add(out.retained,r.fineGpu,AtlasView::GpuCap))return false;out.pending=r.pendingGpu;out.retired=r.retiredGpu;U64 total=r.viewerGpu;if(!Add(total,r.fineGpu,AtlasView::GpuCap)||!Add(total,r.pendingGpu,AtlasView::GpuCap)||!Add(total,r.retiredGpu,AtlasView::GpuCap))return false;return r.occupied<=FineJob::ActiveSlots()&&r.fineGpu>=U64(r.occupied)*FineJob::WindowBytes&&r.fineGpu<=U64(FineJob::ActiveSlots())*FineJob::WindowBytes;}
 void Retain(){quarantined_=true;if(SUCCEEDED(error_))error_=E_FAIL;Debt().owner=owner_;Debt().cpu=SourceReceipts::CpuCap;Debt().gpu=owner_?owner_->native.Cohort().ReservedGpuBytes():0;}
 void Hard(HRESULT hr){if(SUCCEEDED(error_))error_=FAILED(hr)?hr:E_ABORT;if(owner_&&!owner_->stopping){owner_->stopping=true;owner_->stopAt=GetTickCount();owner_->worker.Cancel();}}
 bool PairMatches(const FineJob::Plan&p,unsigned pair,const FineJob::Demand&now,const WorldCohort::Job&job)const{
  if(!FineJob::Internal::Fresh(p,now)||pair>=p.pairCount||job.count!=2||!WorldCohort::ValidJob(job))return false;
  FineJob::Snapshot snapshot={p.demand.view.stamp,job.material};FineJob::Settings settings={job.frame[0].cameraY,job.frame[0].farClip,job.frame[0].waterHeight,job.frame[0].wetness};WorldCohort::Job expected;
  if(!FineJob::MakeJob(p,pair,now,snapshot,settings,job.shaders,job.heightPath,expected)||!SourceReceipts::SameScope(job.scope,expected.scope)||!WorldBank::EqualPlan(job.plan,expected.plan))return false;
  for(unsigned n=0;n<2;++n){const WorldRaster::Grid&a=job.grid[n],&b=expected.grid[n];if(!WorldRaster::SameFrame(job.frame[n],expected.frame[n])||a.x!=b.x||a.z!=b.z||a.columns!=b.columns||a.rows!=b.rows||a.sourceEpoch!=b.sourceEpoch)return false;}return true;
 }
 bool PollBoot(){if(owner_->workerReady)return true;int r=owner_->worker.PollBoot();if(r<0){Hard(owner_->worker.Error());if(owner_->worker.Quarantined())Retain();return false;}if(r==1)owner_->workerReady=true;return true;}
 bool PollWritten(){Owner&o=*owner_;FineStore::Receipt receipt={};int r=o.worker.PollPut(receipt);if(r<0){Hard(o.worker.Error());if(o.worker.Quarantined())Retain();return false;}if(r==1){
   // FineWorker's completion publication cleared every borrowed pointer.
   if(!FineWindows::Equal(receipt.key,o.writingKey)||!FineStore::Same(receipt.identity,o.identity)||!receipt.serial){Hard(E_ABORT);return false;}o.writing=false;++o.stored;
  }return true;
 }
 int DrainStep(const WorldBank::Ledger&gpu){Owner&o=*owner_;Producer&p=o.native.Cohort();if(o.writing)return 0;
  if(o.reading){if(o.readReady)return 0;int r=o.worker.PollGet();if(r<0){Hard(o.worker.Error());if(o.worker.Quarantined())Retain();return 0;}if(r==1){o.readReady=true;if(!o.worker.ReleaseRead()){Hard(E_FAIL);return 0;}o.reading=o.readReady=false;}else if(r==2)o.reading=false;else return 0;}
  if(!o.producerCancelled){p.Cancel();o.producerCancelled=true;}
  HRESULT hr=p.Pump(false,NULL,o.scope,gpu);if(WorldCohort::Quarantine().marked){Retain();return -1;}if(FAILED(hr)){Hard(hr);return 0;}
  if(p.State()==WorldCohort::Idle&&p.ReservedGpuBytes()==0){o.draining=o.soft=o.queued=o.producerCancelled=false;o.plan=FineJob::Plan();return 2;}return 0;
 }
public:
 Controller():owner_(NULL),thread_(GetCurrentThreadId()),error_(S_OK),quarantined_(false){}~Controller(){Shutdown();}
 bool Begin(const SourceReceipts::Scope&scope,const FineStore::Identity&id,const FineStore::Config&config,const FineStore::Ledger&external){
  if(!Main()||owner_||quarantined_||Debt().owner||WorldCohort::Quarantine().marked||scope.thread!=thread_||!SourceReceipts::ScopeValid(scope)||!FineStore::Valid(id)||scope.sourceEpoch!=id.sourceEpoch)return false;
  try{owner_=new Owner;owner_->scope=scope;owner_->identity=id;owner_->external=external;error_=S_OK;HRESULT hr=owner_->native.Init(scope);FineStore::Ledger ledger;if(FAILED(hr)||!CpuLedger(true,external,ledger)||!owner_->worker.Start(scope,id,config,ledger)){Hard(FAILED(hr)?hr:E_FAIL);return false;}return true;}catch(...){Hard(E_OUTOFMEMORY);return false;}
 }
 bool MakeBudget(const Residency&r,const FineStore::Ledger&external,FineJob::Budget&out)const{
  std::memset(&out,0,sizeof(out));if(!Main()||!owner_||quarantined_)return false;out.viewerGpu=r.viewerGpu;if(!Add(out.viewerGpu,r.pendingGpu,AtlasView::GpuCap)||!Add(out.viewerGpu,r.retiredGpu,AtlasView::GpuCap))return false;out.fineGpu=r.fineGpu;out.occupied=r.occupied;out.producerCpu=owner_->native.CpuReservation();out.otherCpu=external.other;
  U64 scratch=FineStore::Session::PutCpuBytes();if(scratch<FineStore::Session::GetCpuBytes())scratch=FineStore::Session::GetCpuBytes();return Add(out.otherCpu,external.retainedRaw,SourceReceipts::CpuCap)&&Add(out.otherCpu,Extra(),SourceReceipts::CpuCap)&&Add(out.otherCpu,FineWrite::Worker::FixedCpuBytes(),SourceReceipts::CpuCap)&&Add(out.otherCpu,FineStore::Session::FixedCpuBytes(),SourceReceipts::CpuCap)&&Add(out.otherCpu,scratch,SourceReceipts::CpuCap)&&FineJob::Internal::BudgetValid(out);
 }
 bool Ready()const{return Main()&&owner_&&!quarantined_&&!owner_->stopping&&!owner_->draining&&!owner_->queued&&!owner_->writing&&!owner_->reading&&owner_->workerReady&&owner_->native.Cohort().State()==WorldCohort::Idle;}
 bool Queue(const FineJob::Plan&p,unsigned pair,const FineJob::Demand&now,const FineStore::Identity&id,const WorldCohort::Job&job,const Residency&r,const FineStore::Ledger&external){
  if(!Ready()||!CurrentBase(now,id)||!PairMatches(p,pair,now,job))return false;FineJob::Budget budget;if(!MakeBudget(r,external,budget)||!FineJob::CanRender(p,now,budget))return false;
  Owner&o=*owner_;o.plan=p;o.pair=pair;o.job=job;o.external=external;o.soft=false;
  std::memcpy(o.shaders,job.shaders.vertex.data,4440);std::memcpy(o.shaders+4440,job.shaders.partial.data,13208);std::memcpy(o.shaders+17648,job.shaders.tail.data,4272);o.job.shaders.vertex.data=o.shaders;o.job.shaders.partial.data=o.shaders+4440;o.job.shaders.tail.data=o.shaders+17648;
  HRESULT hr=o.native.RebindCapture(o.job,o.tokens);if(SUCCEEDED(hr))hr=o.native.Cohort().Submit(true,o.job,o.tokens,o.scope);if(FAILED(hr)){Hard(hr);return false;}o.queued=true;return true;
 }
 //1 pair acknowledged;2 source GPU fully drained;3 read ready;4 cache miss;
 //0 pending/idle;-1 failure/terminated. None of these publishes a GPU texture.
 int Tick(bool admitted,ID3D11Device*device,const FineJob::Demand&now,const FineStore::Identity&id,const Residency&r,const FineStore::Ledger&external){
  if(!Main()||!owner_||quarantined_)return -1;Owner&o=*owner_;Producer&p=o.native.Cohort();o.external=external;WorldBank::Ledger gpu={};
  if(!admitted||!device||!CurrentBase(now,id)||!GpuLedger(r,gpu))Hard(E_ABORT);
  if(o.stopping){
   // A CPU read is no longer publishable; release only on serialized Main.
   if(o.worker.ReadPending()&&!o.readExposed)o.worker.ReleaseRead();int joined=o.worker.TryJoin();if(joined<0){Retain();return -1;}if(!joined){if(GetTickCount()-o.stopAt>=5000){Retain();return -1;}return 0;}
   o.writing=o.reading=o.readReady=false;if(FAILED(o.worker.Error())&&SUCCEEDED(error_))error_=o.worker.Error();if(!o.producerCancelled){p.Cancel();o.producerCancelled=true;}
   HRESULT hr=p.Pump(false,NULL,o.scope,gpu);if(WorldCohort::Quarantine().marked){Retain();return -1;}if(FAILED(hr)&&SUCCEEDED(error_))error_=hr;
   if((p.State()==WorldCohort::Idle||p.State()==WorldCohort::Stopped||p.State()==WorldCohort::Failed)&&p.ReservedGpuBytes()==0){o.terminated=true;return -1;}return 0;
  }
  if(!PollBoot())return quarantined_?-1:0;
  if(o.plan.state==FineJob::Detail&&!FineJob::Internal::SameDemand(o.plan.demand,now)){o.soft=true;o.draining=true;}
  // Soft viewport drain must acknowledge borrowed pixels even while old fine
  // textures remain charged to a pending retirement fence above32MiB.
  if(o.soft&&o.writing){if(!PollWritten())return quarantined_?-1:0;if(o.writing)return 0;}
  if(o.draining)return DrainStep(gpu);
  FineJob::Budget budget;if(!MakeBudget(r,external,budget)){Residency empty;FineJob::Budget cpuOnly;if(!MakeBudget(empty,external,cpuOnly))Hard(E_OUTOFMEMORY);return 0;}
  if(o.writing){if(!PollWritten())return quarantined_?-1:0;if(o.writing)return 0;if(o.soft)return DrainStep(gpu);
   if(!FineJob::CanRender(o.plan,now,budget)){Hard(E_OUTOFMEMORY);return 0;}unsigned n=p.CurrentView();HRESULT fresh=p.Pump(true,device,o.scope,gpu);if(FAILED(fresh)||n>=2||!p.Result(n,true,o.scope,o.job.frame[n])){Hard(FAILED(fresh)?fresh:E_ABORT);return 0;}HRESULT hr=p.ReleaseView(n,true,o.scope,o.job.frame[n]);if(FAILED(hr)){Hard(hr);return 0;}if(p.State()==WorldCohort::Idle){o.queued=false;return 1;}
  }
  if(o.draining)return DrainStep(gpu);
  if(o.reading){if(o.readReady)return 0;int rr=o.worker.PollGet();if(rr<0){Hard(o.worker.Error());if(o.worker.Quarantined())Retain();return quarantined_?-1:0;}if(rr==1){o.readReady=true;return 3;}if(rr==2){o.reading=false;return 4;}return 0;}
  if(o.queued&&!FineJob::CanRender(o.plan,now,budget)){Hard(E_OUTOFMEMORY);return 0;}
  if(p.State()==WorldCohort::Booting||o.queued){HRESULT hr=p.Pump(o.queued,device,o.scope,gpu);if(FAILED(hr)){Hard(hr);return 0;}}
  if(o.queued&&p.State()==WorldCohort::Complete){unsigned n=p.CurrentView();if(n>=2||!p.Result(n,true,o.scope,o.job.frame[n])){Hard(E_ABORT);return 0;}const FineJob::Pair&pair=o.plan.pairs[o.pair];
   if(!pair.wanted[n]){HRESULT hr=p.ReleaseView(n,true,o.scope,o.job.frame[n]);if(FAILED(hr)){Hard(hr);return 0;}if(p.State()==WorldCohort::Idle){o.queued=false;return 1;}return 0;}
   WorldCohort::Borrowed borrow={};FineStore::Ledger ledger;if(!p.Borrow(n,true,o.scope,o.job.frame[n],borrow)||!CpuLedger(false,external,ledger)){Hard(E_OUTOFMEMORY);return 0;}o.writingKey=pair.key[n];o.writing=true;if(!o.worker.SubmitPut(borrow,o.writingKey,o.identity,ledger)){Hard(E_FAIL);if(o.worker.Quarantined())Retain();}
  }return quarantined_?-1:0;
 }
 bool Drain(){if(!Ready())return false;owner_->draining=true;return true;}
 bool RequestRead(const FineJob::Plan&p,const FineWindows::Key&key,const FineJob::Demand&now,const FineStore::Identity&id,const Residency&r,const FineStore::Ledger&external){
  if(!Ready()||!CurrentBase(now,id)||!FineJob::Wanted(p,now,key)||owner_->native.Cohort().ReservedGpuBytes()!=0)return false;FineJob::Budget b;if(!MakeBudget(r,external,b))return false;FineJob::UploadGate gate={0,true};if(!FineJob::CanRead(p,now,key,b,gate))return false;FineStore::Ledger ledger;if(!CpuLedger(true,external,ledger))return false;
  if(!owner_->worker.SubmitGet(key,id,ledger))return false;owner_->plan=p;owner_->readingKey=key;owner_->reading=true;owner_->readReady=owner_->readExposed=false;return true;
 }
 bool PeekRead(bool enabled,const FineJob::Demand&now,const FineStore::Identity&id,const Residency&r,const FineStore::Ledger&external,FineStore::View&out){
  std::memset(&out,0,sizeof(out));if(!enabled||!Main()||!owner_||quarantined_||owner_->stopping||owner_->draining||!owner_->reading||!owner_->readReady||!CurrentBase(now,id))return false;FineJob::Budget b;if(!MakeBudget(r,external,b))return false;FineJob::UploadGate gate={owner_->native.Cohort().ReservedGpuBytes(),!owner_->writing&&!owner_->worker.BorrowPending()};if(!FineJob::CanUpload(owner_->plan,now,owner_->readingKey,b,gate)||!owner_->worker.PeekRead(true,now.scope,owner_->readingKey,id,out))return false;owner_->readExposed=true;return true;
 }
 bool ReleaseRead(){if(!Main()||!owner_||quarantined_||!owner_->reading||!owner_->readReady||!owner_->worker.ReleaseRead())return false;owner_->reading=owner_->readReady=owner_->readExposed=false;return true;}
 void Cancel(){if(Main())Hard(E_ABORT);}bool Active()const{return owner_!=NULL;}bool Terminated()const{return Main()&&owner_&&owner_->terminated;}bool ReadReady()const{return Main()&&owner_&&owner_->readReady;}bool Quarantined()const{return quarantined_;}HRESULT Error()const{return error_;}unsigned StoredWindows()const{return Main()&&owner_?owner_->stored:0;}
 U64 ProducerGpuBytes()const{return Main()&&owner_?owner_->native.Cohort().ReservedGpuBytes():0;}U64 RetainedCpuBytes()const{return quarantined_?Debt().cpu:0;}U64 RetainedGpuBytes()const{return quarantined_?Debt().gpu:0;}
 Progress ObserveProgress()const{
  Progress p;if(!Main())return p;p.error=error_;p.ownerFlags=quarantined_?8192U:0U;if(!owner_)return p;
  Owner&o=*owner_;const Producer&native=o.native.Cohort();
  p.nativeState=unsigned(native.State());p.view=native.CurrentView();p.stage=native.CurrentStage();p.cursor=native.StageCursor();p.input=native.CurrentInput();p.nativeWorkerState=native.ProgressWorkerState();p.consumerWorkerState=o.worker.ProgressState();p.stored=o.stored;
  p.ownerFlags|=1U|(o.workerReady?2U:0U)|(o.queued?4U:0U)|(o.writing?8U:0U)|(o.reading?16U:0U)|(o.readReady?32U:0U)|(o.readExposed?64U:0U)|(o.draining?128U:0U)|(o.soft?256U:0U)|(o.producerCancelled?512U:0U)|(o.stopping?1024U:0U)|(o.terminated?2048U:0U);
  return p;
 }
 FineWrite::RamStats MemoryStats()const{return Main()&&owner_&&!quarantined_?owner_->worker.MemoryStats():FineWrite::RamStats();}
 U64 RamBytes()const{return MemoryStats().bytes;}
 // Prepared RAM payload is separate from strict producer/scratch CPU debt.
 U64 RetainedRamBytes()const{return Main()&&owner_?(quarantined_?GsmCacheProfile::RamBudgetBytes():owner_->worker.RetainedRamBytes()):0;}
 // After consumer join and GPU bank drain, finish the idle source worker with
 // zero-timeout polling. This is the normal-frame close/re-enable path.
 HRESULT RequestShutdown(){if(!Main()||!owner_||quarantined_||!owner_->terminated||owner_->readExposed||owner_->native.Cohort().ReservedGpuBytes())return E_UNEXPECTED;if(owner_->native.Cohort().State()==WorldCohort::Stopped){delete owner_;owner_=NULL;return S_OK;}HRESULT hr=owner_->native.Cohort().RequestShutdown();if(FAILED(hr)){error_=hr;Retain();}return hr;}
 HRESULT PollShutdown(){if(!Main()||quarantined_)return E_UNEXPECTED;if(!owner_)return S_OK;HRESULT hr=owner_->native.Cohort().PollShutdown();if(hr==S_FALSE)return hr;if(owner_->native.Cohort().Quarantined()){error_=hr;Retain();return FAILED(hr)?hr:E_FAIL;}if(FAILED(hr)){error_=hr;Retain();return hr;}delete owner_;owner_=NULL;return S_OK;}
 // Explicit final teardown only; NEVER call from normal live-viewer Tick.
 void Shutdown(){if(!owner_||quarantined_)return;if(!Main()){Retain();return;}Hard(E_ABORT);if(owner_->readExposed){Retain();return;}if(owner_->worker.ReadPending())owner_->worker.ReleaseRead();if(!owner_->worker.Join()){Retain();return;}owner_->native.Cohort().Shutdown();if(WorldCohort::Quarantine().marked){Retain();return;}delete owner_;owner_=NULL;}
};
}
