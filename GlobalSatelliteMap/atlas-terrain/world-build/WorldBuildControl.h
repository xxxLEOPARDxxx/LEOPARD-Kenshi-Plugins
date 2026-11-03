#pragma once
#include "../world-native-rebind/NativeRebind.h"
#include "../../atlas-cache/world-write-worker/WorldWriteWorker.h"
namespace WorldBuild {
template<class NativeSession,class Factory,class Config>class Controller {
 typedef WorldWrite::Worker<Factory,Config> Writer;
 typedef WorldCohort::Controller<NativeSession> Producer;
 struct Owner {
  NativeSession native;Writer writer;WorldCohort::Job job;WorldStream::CapturedSources tokens;
  SourceReceipts::Scope scope;WorldWindow::Ledger external;BYTE shaders[21920];
  bool writerReady,queued,writing,stopping,producerCancelled;
  DWORD stopAt;unsigned windows;Owner():writerReady(false),queued(false),writing(false),stopping(false),producerCancelled(false),stopAt(0),windows(0){std::memset(&job,0,sizeof(job));std::memset(&scope,0,sizeof(scope));}
 };
 struct Retained {Owner*owner;WorldWindow::U64 cpu,gpu;Retained():owner(NULL),cpu(0),gpu(0){}};
 static Retained&Debt(){static Retained debt;return debt;}
 Owner*owner_;DWORD thread_;HRESULT error_;bool quarantined_;unsigned completed_;
 Controller(const Controller&);Controller&operator=(const Controller&);
 bool Main()const{return GetCurrentThreadId()==thread_;}
 static WorldWindow::U64 Extra(){return sizeof(Controller)+sizeof(Owner)-sizeof(NativeSession)-sizeof(Writer);}
 bool Ledger(bool initial,WorldWindow::Ledger&out)const{
  out=owner_->external;WorldWindow::U64 n=initial?owner_->native.CpuReservation():owner_->native.ExtraCpuPeakBytes();
  if(n>SourceReceipts::CpuCap||Extra()>SourceReceipts::CpuCap-n||out.other>SourceReceipts::CpuCap-n-Extra())return false;out.other+=n+Extra();return true;
 }
 void Retain(){quarantined_=true;error_=E_FAIL;Debt().owner=owner_;Debt().cpu=SourceReceipts::CpuCap;Debt().gpu=owner_?owner_->native.Cohort().ReservedGpuBytes():0;}
 void Stop(HRESULT reason){if(SUCCEEDED(error_))error_=FAILED(reason)?reason:E_ABORT;if(owner_&&!owner_->stopping){owner_->stopping=true;owner_->stopAt=GetTickCount();owner_->writer.Cancel();}}
 bool Finish(){owner_->native.Cohort().Shutdown();if(WorldCohort::Quarantine().marked){Retain();return false;}completed_=owner_->windows;delete owner_;owner_=NULL;return true;}
public:
 Controller():owner_(NULL),thread_(GetCurrentThreadId()),error_(S_OK),quarantined_(false),completed_(0){}
 ~Controller(){if(!owner_||quarantined_)return;if(!Main()){Retain();return;}Stop(E_ABORT);if(!owner_->writer.Join()){Retain();return;}Finish();}
 bool Begin(const SourceReceipts::Scope&scope,WorldWindow::U64 material,const Config&config,const WorldWindow::Ledger&external=WorldWindow::Ledger()){
  if(!Main()||owner_||quarantined_||Debt().owner||WorldCohort::Quarantine().marked||!SourceReceipts::ScopeValid(scope)||scope.thread!=thread_||!material)return false;
  try{
   owner_=new Owner;owner_->scope=scope;owner_->external=external;error_=S_OK;completed_=0;
   HRESULT hr=owner_->native.Init(scope);WorldWindow::Ledger ledger;
   if(FAILED(hr)||!Ledger(true,ledger)||!owner_->writer.Start(scope,material,config,ledger)){error_=FAILED(hr)?hr:E_FAIL;Stop(error_);return false;}
   return true;
  }catch(...){error_=E_OUTOFMEMORY;Stop(error_);return false;}
 }
 bool Ready()const{return Main()&&owner_&&!quarantined_&&!owner_->stopping&&owner_->writerReady&&!owner_->queued&&!owner_->writing&&owner_->native.Cohort().State()==WorldCohort::Idle;}
 bool Queue(const WorldCohort::Job&job){
  if(!Ready()||!SourceReceipts::SameScope(owner_->scope,job.scope)||!WorldCohort::ValidJob(job))return false;
  Owner&o=*owner_;o.job=job;
  std::memcpy(o.shaders,job.shaders.vertex.data,4440);std::memcpy(o.shaders+4440,job.shaders.partial.data,13208);std::memcpy(o.shaders+17648,job.shaders.tail.data,4272);
  o.job.shaders.vertex.data=o.shaders;o.job.shaders.partial.data=o.shaders+4440;o.job.shaders.tail.data=o.shaders+17648;
  HRESULT hr=o.native.RebindCapture(o.job,o.tokens);if(SUCCEEDED(hr))hr=o.native.Cohort().Submit(true,o.job,o.tokens,o.scope);
  if(FAILED(hr)){Stop(hr);return false;}o.queued=true;return true;
 }
 //1 acknowledges one whole cohort;2 is a gracefully closed operation;
 //0 is pending/idle;-1 is terminal refusal. Neither success publishes an atlas.
 int Tick(bool admitted,ID3D11Device*device,const SourceReceipts::Scope&now){
  if(!Main()||!owner_||quarantined_)return -1;Owner&o=*owner_;Producer&p=o.native.Cohort();
  if(!admitted||!device||!SourceReceipts::SameScope(o.scope,now))Stop(E_ABORT);
  if(o.stopping){
   int joined=o.writer.TryJoin();if(joined<0){Retain();return -1;}if(!joined){if(GetTickCount()-o.stopAt>=5000){Retain();return -1;}return 0;}
   if(FAILED(o.writer.Error()))Stop(o.writer.Error());
   if(!o.producerCancelled){p.Cancel();o.producerCancelled=true;}
   HRESULT hr=p.Pump(false,NULL,o.scope,WorldBank::Ledger());if(WorldCohort::Quarantine().marked){Retain();return -1;}if(FAILED(hr)&&SUCCEEDED(error_))error_=hr;
   if(p.State()==WorldCohort::Idle||p.State()==WorldCohort::Stopped||p.State()==WorldCohort::Failed){bool success=SUCCEEDED(error_);Finish();return !quarantined_&&success?2:-1;}return 0;
  }
  WorldWindow::Location written={};int result=o.writer.Poll(written);
  if(result<0){Stop(o.writer.Error());if(o.writer.Quarantined())Retain();return quarantined_?-1:0;}
  if(result==2)o.writerReady=true;
  if(o.writing){
   if(result!=1)return 0;
   // The worker cleared its CPU borrow before publishing1. Only now may Main
   // Pump/revalidate/release the producer, even if cancellation was requested.
   o.writing=false;const unsigned n=p.CurrentView();
   HRESULT fresh=p.Pump(true,device,now,WorldBank::Ledger());
   if(FAILED(fresh)||n>=o.job.count||!p.Result(n,true,now,o.job.frame[n])){Stop(FAILED(fresh)?fresh:E_ABORT);return 0;}
   WorldWindow::Location expected={};if(!WorldWindow::Locate(o.job.frame[n],expected)||std::memcmp(&expected,&written,sizeof(expected))){Stop(E_ABORT);return 0;}
   HRESULT hr=p.ReleaseView(n,true,now,o.job.frame[n]);if(FAILED(hr)){Stop(hr);return 0;}++o.windows;
   if(p.State()==WorldCohort::Idle){o.queued=false;return 1;}
  }
  if(p.State()==WorldCohort::Booting||o.queued){HRESULT hr=p.Pump(true,device,now,WorldBank::Ledger());if(FAILED(hr)){Stop(hr);return 0;}}
  if(o.queued&&p.State()==WorldCohort::Complete){
   const unsigned n=p.CurrentView();WorldCohort::Borrowed borrowed={};WorldWindow::Ledger ledger;
   if(n>=o.job.count||!p.Borrow(n,true,now,o.job.frame[n],borrowed)||!Ledger(false,ledger)){Stop(E_ABORT);return 0;}
   o.writing=true;if(!o.writer.Submit(borrowed,ledger)){Stop(E_FAIL);if(o.writer.Quarantined())Retain();}
  }
  return quarantined_?-1:0;
 }
 bool Complete(){if(!Ready()||!owner_->windows)return false;owner_->stopping=true;owner_->stopAt=GetTickCount();owner_->writer.Cancel();return true;}
 void Cancel(){if(Main())Stop(E_ABORT);}bool Active()const{return owner_!=NULL;}bool Quarantined()const{return quarantined_;}HRESULT Error()const{return error_;}unsigned Windows()const{return owner_?owner_->windows:completed_;}
};
}
