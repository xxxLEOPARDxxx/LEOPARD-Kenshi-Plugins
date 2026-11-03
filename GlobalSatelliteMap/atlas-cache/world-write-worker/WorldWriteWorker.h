#pragma once
#include "../world-window-session/WorldWindowSession.h"
#include "../../atlas-terrain/world-cohort/WorldCohort.h"
namespace WorldWrite {
template<class Factory,class Config>class Worker {
 enum State{Boot,Idle,Busy,Written,Failed,Stopped};
 enum Command{None,Write,Stop};
 struct Work {
  Config config;SourceReceipts::Scope scope;WorldWindow::U64 material;
  WorldWindow::Ledger initial,writeLedger;WorldCohort::Borrowed borrow;
  HANDLE wake,cancel;volatile LONG state,command,finished;HRESULT error;
  WorldWindow::Location written;Factory*factory;WorldWindow::Session*session;
  Work(const Config&c):config(c),material(0),wake(NULL),cancel(NULL),state(Boot),command(None),finished(0),error(S_OK),factory(NULL),session(NULL){std::memset(&scope,0,sizeof(scope));std::memset(&borrow,0,sizeof(borrow));std::memset(&written,0,sizeof(written));}
  static DWORD WINAPI Run(void*p){Work&w=*static_cast<Work*>(p);
   try{
    w.factory=new Factory(w.config);w.session=new WorldWindow::Session;
    w.error=w.session->Begin(true,w.scope,w.material,*w.factory,w.initial);
    if(FAILED(w.error))throw 1;InterlockedExchange(&w.state,Idle);
    for(;;){
     if(WaitForSingleObject(w.wake,INFINITE)!=WAIT_OBJECT_0){w.error=E_FAIL;break;}
     LONG command=InterlockedExchange(&w.command,None);if(command==Stop)break;
     if(command!=Write){w.error=E_UNEXPECTED;break;}
     w.error=w.session->Write(true,*w.borrow.accepted,w.borrow.scope,w.borrow.frame,*w.borrow.window,w.material,w.cancel,w.writeLedger,w.written);
     // No borrowed owner or plane is touched after this release publication.
     std::memset(&w.borrow,0,sizeof(w.borrow));
     InterlockedExchange(&w.state,SUCCEEDED(w.error)?Written:Failed);
    }
   }catch(const std::bad_alloc&){w.error=E_OUTOFMEMORY;}catch(...){if(SUCCEEDED(w.error))w.error=E_FAIL;}
   std::memset(&w.borrow,0,sizeof(w.borrow));
   if(w.session){HRESULT closed=w.session->Close();if(FAILED(closed)&&SUCCEEDED(w.error))w.error=closed;delete w.session;w.session=NULL;}
   delete w.factory;w.factory=NULL;
   InterlockedExchange(&w.finished,1);InterlockedExchange(&w.state,FAILED(w.error)?Failed:Stopped);return FAILED(w.error)?1:0;
  }
 };
 Work*work_;HANDLE thread_;DWORD owner_;bool ready_,pending_,failed_,stopping_,quarantined_;HRESULT error_;
 Worker(const Worker&);Worker&operator=(const Worker&);
 bool Main()const{return GetCurrentThreadId()==owner_;}
 static bool Add(WorldWindow::U64&a,WorldWindow::U64 b){if(b>SourceReceipts::CpuCap||a>SourceReceipts::CpuCap-b)return false;a+=b;return true;}
 void Retain(){quarantined_=failed_=true;error_=E_FAIL;}
 int Reap(DWORD timeout,bool retainTimeout){
  if(!Main()||quarantined_)return -1;if(!work_)return 1;if(!stopping_)return -1;
  DWORD wait=WaitForSingleObject(thread_,timeout);if(wait==WAIT_TIMEOUT){if(retainTimeout)Retain();return retainTimeout?-1:0;}
  if(wait!=WAIT_OBJECT_0||InterlockedCompareExchange(&work_->finished,0,0)!=1){Retain();return -1;}
  CloseHandle(thread_);thread_=NULL;CloseHandle(work_->wake);CloseHandle(work_->cancel);delete work_;work_=NULL;pending_=false;return 1;
 }
public:
 Worker():work_(NULL),thread_(NULL),owner_(GetCurrentThreadId()),ready_(false),pending_(false),failed_(false),stopping_(false),quarantined_(false),error_(S_OK){}
 ~Worker(){if(!Main()){if(work_)Retain();return;}Cancel();Join();}
 static WorldWindow::U64 FixedCpuBytes(){return sizeof(Worker)+sizeof(Work)+sizeof(Factory);}
 HRESULT Error()const{return error_;}bool Quarantined()const{return quarantined_;}bool Pending()const{return pending_;}
 bool Start(const SourceReceipts::Scope&scope,WorldWindow::U64 material,const Config&config,const WorldWindow::Ledger&external){
  if(!Main()||work_||thread_||stopping_||failed_||quarantined_||scope.thread!=owner_||!SourceReceipts::ScopeValid(scope)||!material)return false;
  WorldWindow::Ledger ledger=external;if(!Add(ledger.other,FixedCpuBytes())||ledger.retainedRaw>SourceReceipts::CpuCap-ledger.other)return false;
  try{work_=new Work(config);}catch(...){error_=E_OUTOFMEMORY;return false;}
  work_->scope=scope;work_->material=material;work_->initial=ledger;work_->wake=CreateEventA(NULL,FALSE,FALSE,NULL);work_->cancel=CreateEventA(NULL,TRUE,FALSE,NULL);
  if(work_->wake&&work_->cancel)thread_=CreateThread(NULL,0,Work::Run,work_,0,NULL);
  if(!thread_){error_=E_FAIL;if(work_->wake)CloseHandle(work_->wake);if(work_->cancel)CloseHandle(work_->cancel);delete work_;work_=NULL;return false;}return true;
 }
 bool Submit(const WorldCohort::Borrowed&borrow,const WorldWindow::Ledger&external){
  if(!Main()||!work_||!ready_||pending_||stopping_||failed_||quarantined_||InterlockedCompareExchange(&work_->state,0,0)!=Idle||!borrow.accepted||!borrow.window||!SourceReceipts::SameScope(work_->scope,borrow.scope))return false;
  WorldWindow::Ledger ledger=external;
  if(!Add(ledger.other,borrow.otherCpuBytes)||!Add(ledger.other,FixedCpuBytes())||!Add(ledger.retainedRaw,borrow.retainedRawBytes))return false;
  work_->borrow=borrow;work_->writeLedger=ledger;std::memset(&work_->written,0,sizeof(work_->written));work_->error=S_OK;
  pending_=true;InterlockedExchange(&work_->state,Busy);InterlockedExchange(&work_->command,Write);
  if(!SetEvent(work_->wake)){Retain();return false;}return true;
 }
 //2 once after initialization;1 once for a consumed completed window;
 //0 while pending/idle;-1 on refusal. Does not wait or release the consumer.
 int Poll(WorldWindow::Location&out){
  std::memset(&out,0,sizeof(out));if(!Main()||!work_||failed_||quarantined_)return -1;
  DWORD wait=WaitForSingleObject(thread_,0);if(wait==WAIT_FAILED){Retain();return -1;}
  if(wait==WAIT_OBJECT_0){error_=FAILED(work_->error)?work_->error:E_FAIL;failed_=true;return -1;}
  LONG state=InterlockedCompareExchange(&work_->state,0,0);
  if(stopping_)return 0;if(state==Failed){error_=FAILED(work_->error)?work_->error:E_FAIL;failed_=true;return -1;}
  if(state==Idle&&!ready_){ready_=true;return 2;}
  if(state==Written&&pending_){out=work_->written;pending_=false;InterlockedExchange(&work_->state,Idle);return 1;}return 0;
 }
 void Cancel(){if(!Main()||!work_||quarantined_)return;stopping_=true;SetEvent(work_->cancel);InterlockedExchange(&work_->command,Stop);if(!SetEvent(work_->wake))Retain();}
 // Teardown only. The caller keeps all borrowed CPU owners alive until true.
 int TryJoin(){return Reap(0,false);}
 bool Join(){return Reap(5000,true)==1;}
};
}
