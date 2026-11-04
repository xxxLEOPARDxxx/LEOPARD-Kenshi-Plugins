#pragma once
#include "../fine-store/FineStore.h"
#include "../fine-ramcache/FineRamCache.h"
#include "../../atlas-terrain/world-cohort/WorldCohort.h"
namespace FineWrite {
struct RamStats {FineStore::U64 bytes,hits,misses;unsigned entries;RamStats():bytes(0),hits(0),misses(0),entries(0){}};
// Single persistent worker, no IO/native call on Main. Caller retains borrowed
// producer owners until Put ack OR known finished/joined consumer. On quarantine
// caller MUST retain the whole producer and charge its own CPU/GPU debt too.
class Worker {
 enum State{Boot,Idle,Busy,PutDone,ReadDone,Miss,Failed,Stopped};
 enum Command{None,Put,Get,Stop};
 struct Work {
  FineStore::Config config;SourceReceipts::Scope scope;FineStore::Identity identity;
  FineWindows::Key key;FineStore::Ledger initial,ledger;WorldCohort::Borrowed borrow;
  FineStore::Receipt receipt;FineStore::Blob blob;FineStore::Session*session;
  FineRam::Cache ram;FineStore::U64 ramBudget;
  __declspec(align(8)) volatile LONGLONG ramBytes,ramHits,ramMisses,ramEntries;
  HANDLE wake,cancel;volatile LONG state,command,finished,retained;HRESULT error;
  Work(const FineStore::Config&c):config(c),session(NULL),ramBudget(0),ramBytes(0),ramHits(0),ramMisses(0),ramEntries(0),wake(NULL),cancel(NULL),state(Boot),command(None),finished(0),retained(0),error(S_OK){std::memset(&scope,0,sizeof(scope));std::memset(&identity,0,sizeof(identity));std::memset(&key,0,sizeof(key));std::memset(&borrow,0,sizeof(borrow));std::memset(&receipt,0,sizeof(receipt));}
  void PublishRam(){InterlockedExchange64(&ramBytes,LONGLONG(ram.Bytes()));InterlockedExchange64(&ramHits,LONGLONG(ram.Hits()));InterlockedExchange64(&ramMisses,LONGLONG(ram.Misses()));InterlockedExchange64(&ramEntries,LONGLONG(ram.Entries()));}
  static DWORD WINAPI Run(void*p){Work&w=*static_cast<Work*>(p);
   try{
    w.session=new FineStore::Session;w.error=w.session->Begin(true,w.scope,w.identity,w.config,w.initial);if(FAILED(w.error))throw 1;
    // Optional retention failure must not poison the already valid disk owner.
    w.ram.Begin(w.scope,w.identity,w.ramBudget);w.PublishRam();InterlockedExchange(&w.state,Idle);
    for(;;){
     if(WaitForSingleObject(w.wake,INFINITE)!=WAIT_OBJECT_0){w.error=E_FAIL;InterlockedExchange(&w.retained,1);break;}
     LONG command=InterlockedExchange(&w.command,None);if(command==Stop){
      // Cancel may race the last Get instruction. Main must acknowledge the
      // published read even then; do not reclaim its bytes asynchronously.
      if(w.blob.OwnedBytes()){InterlockedExchange(&w.state,ReadDone);continue;}break;
     }
     if(command==Put){
      w.error=w.session->Put(true,w.key,w.identity,*w.borrow.accepted,w.borrow.scope,w.borrow.frame,*w.borrow.window,w.cancel,w.ledger,w.receipt);
      if(w.error==S_OK&&!w.session->NeedsRetention()){
       AcceptedWindow::Ledger scratch;scratch.retainedRaw=w.ledger.retainedRaw;scratch.other=w.ledger.other+FineStore::Session::FixedCpuBytes();
       w.ram.RememberAccepted(w.receipt,*w.borrow.accepted,w.borrow.scope,w.borrow.frame,*w.borrow.window,w.cancel,scratch);w.PublishRam();
      }
      // This publication is the consumer acknowledgement, including failures.
      std::memset(&w.borrow,0,sizeof(w.borrow));
      if(w.session->NeedsRetention()){InterlockedExchange(&w.retained,1);break;}
      InterlockedExchange(&w.state,SUCCEEDED(w.error)?PutDone:Failed);
     }else if(command==Get){
      w.error=WorldPixels::Cancelled(w.cancel)?HRESULT_FROM_WIN32(ERROR_CANCELLED):w.ram.Get(w.key,w.identity,w.cancel,w.ledger,w.blob);
      if(w.error!=S_OK&&!WorldPixels::Cancelled(w.cancel)){
       w.error=w.session->Get(true,w.key,w.identity,w.cancel,w.ledger,w.blob);
       if(w.error==S_OK&&!w.session->NeedsRetention())w.ram.Remember(w.blob,w.key,w.identity,w.cancel);
      }
      w.PublishRam();
      if(w.session->NeedsRetention()){InterlockedExchange(&w.retained,1);break;}
      InterlockedExchange(&w.state,FAILED(w.error)?Failed:w.error==S_FALSE?Miss:ReadDone);
     }else {w.error=E_UNEXPECTED;break;}
    }
   }catch(const std::bad_alloc&){w.error=E_OUTOFMEMORY;}catch(...){if(SUCCEEDED(w.error))w.error=E_FAIL;}
   std::memset(&w.borrow,0,sizeof(w.borrow));
   // A worker EOF/exception while Main could be reading cannot reclaim Blob.
   if(w.blob.OwnedBytes())InterlockedExchange(&w.retained,1);
   if(!InterlockedCompareExchange(&w.retained,0,0)){w.ram.Clear();w.PublishRam();}
   // A normal/graceful-cancel terminal session cannot reuse its private cache.
   // Remove only known closed indexed payloads here on the worker. Poisoned or
   // uncertain owners retain forensic data; active Main reads never reach here.
   if(!InterlockedCompareExchange(&w.retained,0,0)&&w.session&&w.session->Healthy()&&(SUCCEEDED(w.error)||w.error==E_ABORT||w.error==HRESULT_FROM_WIN32(ERROR_CANCELLED))){HRESULT hr=w.session->DiscardKnownClosed();if(FAILED(hr))w.error=hr;if(w.session->NeedsRetention())InterlockedExchange(&w.retained,1);}
   if(!InterlockedCompareExchange(&w.retained,0,0)&&w.session){HRESULT hr=w.session->Close();if(FAILED(hr)&&SUCCEEDED(w.error))w.error=hr;if(w.session->NeedsRetention())InterlockedExchange(&w.retained,1);else{delete w.session;w.session=NULL;}}
   if(InterlockedCompareExchange(&w.retained,0,0)&&SUCCEEDED(w.error))w.error=E_FAIL;
   InterlockedExchange(&w.finished,1);InterlockedExchange(&w.state,FAILED(w.error)?Failed:Stopped);return FAILED(w.error)?1:0;
  }
 };
 struct Retained {Work*work;HANDLE thread;FineStore::U64 cpu;Retained():work(NULL),thread(NULL),cpu(0){}};
 static Retained&Debt(){static Retained d;return d;}
 static volatile LONG&Slot(){static volatile LONG slot=0;return slot;}
 Work*work_;HANDLE thread_;DWORD owner_;bool ready_,pendingPut_,pendingGet_,announcedRead_,stopping_,failed_,quarantined_;HRESULT error_;
 Worker(const Worker&);Worker&operator=(const Worker&);
 bool Main()const{return owner_==GetCurrentThreadId();}
 static bool Add(FineStore::U64&a,FineStore::U64 b){if(b>SourceReceipts::CpuCap||a>SourceReceipts::CpuCap-b)return false;a+=b;return true;}
 void Retain(){quarantined_=failed_=true;if(SUCCEEDED(error_))error_=E_FAIL;Debt().work=work_;Debt().thread=thread_;Debt().cpu=SourceReceipts::CpuCap;}
 bool Check(){
  if(!Main()||!work_||failed_||quarantined_)return false;DWORD wait=WaitForSingleObject(thread_,0);
  if(wait==WAIT_FAILED){Retain();return false;}
  if(wait==WAIT_OBJECT_0){error_=FAILED(work_->error)?work_->error:E_FAIL;failed_=true;if(InterlockedCompareExchange(&work_->finished,0,0)!=1||InterlockedCompareExchange(&work_->retained,0,0))Retain();return false;}
  if(InterlockedCompareExchange(&work_->state,0,0)==Failed){error_=FAILED(work_->error)?work_->error:E_FAIL;failed_=true;return false;}return true;
 }
 bool IdleReady(){return Check()&&ready_&&!pendingPut_&&!pendingGet_&&!stopping_&&InterlockedCompareExchange(&work_->state,0,0)==Idle;}
 bool Send(Command command){InterlockedExchange(&work_->state,Busy);InterlockedExchange(&work_->command,command);if(SetEvent(work_->wake))return true;Retain();return false;}
 int Reap(DWORD timeout,bool timeoutFatal){
  if(!Main()||quarantined_)return -1;if(!work_)return 1;if(!stopping_)return -1;
  // Main still holds a published CPU read. It must ReleaseRead first; no worker
  // Stop command may asynchronously delete it behind a live consuming pointer.
  if(pendingGet_&&InterlockedCompareExchange(&work_->state,0,0)==ReadDone){announcedRead_=true;if(timeoutFatal){Retain();return -1;}return 0;}
  DWORD wait=WaitForSingleObject(thread_,timeout);if(wait==WAIT_TIMEOUT){if(timeoutFatal)Retain();return timeoutFatal?-1:0;}
  if(wait!=WAIT_OBJECT_0||InterlockedCompareExchange(&work_->finished,0,0)!=1||InterlockedCompareExchange(&work_->retained,0,0)||work_->session){Retain();return -1;}
  if(FAILED(work_->error)&&SUCCEEDED(error_))error_=work_->error;
  // Keep the entire owner if even a kernel handle close has unknown outcome.
  if(!CloseHandle(thread_)){Retain();return -1;}thread_=NULL;
  if(!CloseHandle(work_->wake)){Retain();return -1;}work_->wake=NULL;
  if(!CloseHandle(work_->cancel)){Retain();return -1;}work_->cancel=NULL;
  delete work_;work_=NULL;pendingPut_=pendingGet_=announcedRead_=false;InterlockedExchange(&Slot(),0);return 1;
 }
public:
 Worker():work_(NULL),thread_(NULL),owner_(GetCurrentThreadId()),ready_(false),pendingPut_(false),pendingGet_(false),announcedRead_(false),stopping_(false),failed_(false),quarantined_(false),error_(S_OK){}
 ~Worker(){if(!work_||quarantined_)return;if(!Main()){Retain();return;}Cancel();if(pendingGet_&&announcedRead_){Retain();return;}Join();}
 static FineStore::U64 FixedCpuBytes(){return sizeof(Worker)+sizeof(Work);} // Session charges itself inside FineStore
 FineStore::U64 RetainedCpuBytes()const{return quarantined_?Debt().cpu:0;}bool Quarantined()const{return quarantined_;}HRESULT Error()const{return error_;}
 // Independent retained RAM is never hidden inside the64MiB scratch ceiling.
 RamStats MemoryStats()const{RamStats s;if(!Main()||!work_||quarantined_)return s;s.bytes=FineStore::U64(InterlockedCompareExchange64(&work_->ramBytes,0,0));s.hits=FineStore::U64(InterlockedCompareExchange64(&work_->ramHits,0,0));s.misses=FineStore::U64(InterlockedCompareExchange64(&work_->ramMisses,0,0));s.entries=unsigned(InterlockedCompareExchange64(&work_->ramEntries,0,0));return s;}
 FineStore::U64 RamBytes()const{return MemoryStats().bytes;}FineStore::U64 RetainedRamBytes()const{return quarantined_&&work_?work_->ramBudget:0;}
 bool BorrowPending()const{return pendingPut_;}bool ReadPending()const{return pendingGet_;}
 bool Start(const SourceReceipts::Scope&scope,const FineStore::Identity&identity,const FineStore::Config&config,const FineStore::Ledger&external){
  if(!Main()||work_||thread_||stopping_||failed_||quarantined_||Debt().work||scope.thread!=owner_||!SourceReceipts::ScopeValid(scope)||!FineStore::Valid(identity)||scope.sourceEpoch!=identity.sourceEpoch)return false;
  FineStore::Ledger ledger=external;if(!Add(ledger.other,FixedCpuBytes())||ledger.retainedRaw>SourceReceipts::CpuCap-ledger.other||FineStore::Session::FixedCpuBytes()>SourceReceipts::CpuCap-ledger.other-ledger.retainedRaw)return false;
  if(InterlockedCompareExchange(&Slot(),1,0)!=0)return false;try{work_=new Work(config);}catch(...){InterlockedExchange(&Slot(),0);error_=E_OUTOFMEMORY;return false;}
  work_->scope=scope;work_->identity=identity;work_->initial=ledger;work_->ramBudget=GsmCacheProfile::RamBudgetBytes();work_->wake=CreateEventA(NULL,FALSE,FALSE,NULL);work_->cancel=CreateEventA(NULL,TRUE,FALSE,NULL);if(work_->wake&&work_->cancel)thread_=CreateThread(NULL,0,Work::Run,work_,0,NULL);
  if(!thread_){error_=E_FAIL;if(work_->wake){if(!CloseHandle(work_->wake)){Retain();return false;}work_->wake=NULL;}if(work_->cancel){if(!CloseHandle(work_->cancel)){Retain();return false;}work_->cancel=NULL;}delete work_;work_=NULL;InterlockedExchange(&Slot(),0);return false;}return true;
 }
 int PollBoot(){if(!Check())return -1;if(stopping_)return 0;if(InterlockedCompareExchange(&work_->state,0,0)==Idle&&!ready_){ready_=true;return 1;}return 0;}
 bool SubmitPut(const WorldCohort::Borrowed&borrow,const FineWindows::Key&key,const FineStore::Identity&identity,const FineStore::Ledger&external){
  if(!IdleReady()||!borrow.accepted||!borrow.window||!SourceReceipts::SameScope(work_->scope,borrow.scope)||!FineStore::Same(work_->identity,identity)||!FineStore::Anchored(key,identity,borrow.scope,borrow.frame))return false;
  WorldPixels::View view={};if(!borrow.accepted->Acquire(true,borrow.scope,borrow.frame,*borrow.window,view))return false;
  FineStore::Ledger ledger=external;if(!Add(ledger.other,FixedCpuBytes())||!Add(ledger.other,borrow.otherCpuBytes)||!Add(ledger.retainedRaw,borrow.retainedRawBytes))return false;
  work_->borrow=borrow;work_->key=key;work_->ledger=ledger;std::memset(&work_->receipt,0,sizeof(work_->receipt));work_->error=S_OK;pendingPut_=true;return Send(Put);
 }
 int PollPut(FineStore::Receipt&out){std::memset(&out,0,sizeof(out));if(!Check())return -1;if(stopping_)return 0;if(pendingPut_&&InterlockedCompareExchange(&work_->state,0,0)==PutDone){out=work_->receipt;pendingPut_=false;InterlockedExchange(&work_->state,Idle);return 1;}return 0;}
 bool SubmitGet(const FineWindows::Key&key,const FineStore::Identity&identity,const FineStore::Ledger&external){
  if(!IdleReady()||!FineStore::Same(work_->identity,identity)||!FineWindows::Valid(key)||(key.spacing!=1&&key.spacing!=3)||key.stamp.source!=identity.sourceEpoch||key.stamp.material!=identity.materialGeneration)return false;
  FineStore::Ledger ledger=external;if(!Add(ledger.other,FixedCpuBytes()))return false;work_->key=key;work_->ledger=ledger;work_->error=S_OK;pendingGet_=true;announcedRead_=false;return Send(Get);
 }
 int PollGet(){if(!Check())return -1;if(stopping_)return 0;LONG state=InterlockedCompareExchange(&work_->state,0,0);if(pendingGet_&&state==Miss){pendingGet_=false;InterlockedExchange(&work_->state,Idle);return 2;}if(pendingGet_&&state==ReadDone&&!announcedRead_){announcedRead_=true;return 1;}return 0;}
 bool PeekRead(bool enabled,const SourceReceipts::Scope&now,const FineWindows::Key&key,const FineStore::Identity&identity,FineStore::View&out){
  std::memset(&out,0,sizeof(out));if(!enabled||!Check()||stopping_||!pendingGet_||!announcedRead_||InterlockedCompareExchange(&work_->state,0,0)!=ReadDone||!SourceReceipts::SameScope(work_->scope,now)||!FineStore::Same(work_->identity,identity)||!FineWindows::Equal(work_->key,key))return false;return work_->blob.Acquire(true,key,identity,out);
 }
 bool ReleaseRead(){
  if(!Main()||!work_||quarantined_||!pendingGet_||!announcedRead_||InterlockedCompareExchange(&work_->state,0,0)!=ReadDone)return false;
  // Worker is waiting and cannot touch Blob until a future command. CPU only.
  work_->blob.Clear();pendingGet_=announcedRead_=false;InterlockedExchange(&work_->state,Idle);
  if(stopping_){InterlockedExchange(&work_->command,Stop);if(!SetEvent(work_->wake)){Retain();return false;}}return true;
 }
 void Cancel(){
  if(!Main()||!work_||quarantined_)return;stopping_=true;if(!SetEvent(work_->cancel)){Retain();return;}
  // A completed read may not yet have been polled. Latch it as held too.
  if(pendingGet_&&InterlockedCompareExchange(&work_->state,0,0)==ReadDone){announcedRead_=true;return;}
  InterlockedExchange(&work_->command,Stop);if(!SetEvent(work_->wake))Retain();
 }
 int TryJoin(){return Reap(0,false);}bool Join(){return Reap(5000,true)==1;}
};
}
