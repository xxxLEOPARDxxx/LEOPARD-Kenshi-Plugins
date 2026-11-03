#define _CRT_SECURE_NO_WARNINGS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>
static DWORD mainThread=GetCurrentThreadId(),fakeTick=0;static HANDLE watchedThread=NULL;static LONG waitMode=0;static HANDLE watchedWake=NULL;static bool clockOverride=false;
static DWORD WINAPI ObservedWait(HANDLE h,DWORD timeout){if(GetCurrentThreadId()!=mainThread&&h==watchedWake&&InterlockedCompareExchange(&waitMode,0,0)==3)return WAIT_FAILED;if(GetCurrentThreadId()==mainThread&&h==watchedThread){if(waitMode==1)return WAIT_FAILED;if(waitMode==2)return WAIT_TIMEOUT;}return ::WaitForSingleObject(h,timeout);}
static DWORD WINAPI ObservedTick(){return clockOverride&&GetCurrentThreadId()==mainThread?fakeTick : ::GetTickCount();}
#define WaitForSingleObject ObservedWait
#define GetTickCount ObservedTick
#include "_generated/PublicCohort.h"
#undef GetTickCount
#undef WaitForSingleObject
static unsigned checks=0;static void C(bool b,const char*m){++checks;if(!b){printf("FAIL %s\n",m);throw 1;}}
struct Provider{
 HRESULT Capture(const SourceReceipts::WorkerRequest&,SourceReceipts::MainSnapshot&){return E_FAIL;}
 HRESULT Refresh(const SourceReceipts::MainSnapshot&,SourceReceipts::MainSnapshot&){return E_FAIL;}
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*,unsigned,const SourceReceipts::Scope&){return E_FAIL;}void EndBank(){}
};
typedef WorldCohort::Controller<Provider> Controller;
static Controller*Boot(Provider&p){Controller*c=new Controller(p);C(c->Start()==S_OK,"actual WIC worker starts");SourceReceipts::Scope scope={GetCurrentThreadId(),0x100000007ULL,0x200000009ULL,0x30000000bULL};WorldBank::Ledger ledger={};DWORD start=::GetTickCount();while(c->State()==WorldCohort::Booting&&::GetTickCount()-start<5000){C(SUCCEEDED(c->Pump(false,NULL,scope,ledger)),"actual plain worker boot");Sleep(1);}C(c->State()==WorldCohort::Idle&&c->WorkerStatus()==WorldStream::WIdle&&!c->ReservedGpuBytes(),"actual idle worker, no GPU");watchedThread=c->thread_;watchedWake=c->work_->wake;return c;}
static HRESULT Poll(Controller&c){HRESULT hr=S_FALSE;DWORD start=::GetTickCount();while(hr==S_FALSE&&::GetTickCount()-start<5000){hr=c.PollShutdown();if(hr==S_FALSE)Sleep(1);}return hr;}
struct ForeignArgs{Controller*c;HRESULT request,poll;};
static DWORD WINAPI Foreign(void*p){ForeignArgs&v=*static_cast<ForeignArgs*>(p);v.request=v.c->RequestShutdown();v.poll=v.c->PollShutdown();return 0;}
static void Normal(){C(WorldCohort::Failed==17&&WorldCohort::Stopping==18,"historic state IDs retained");Provider p;Controller*c=Boot(p);C(c->PollShutdown()==E_UNEXPECTED,"cannot poll a stop never requested");
 WorldCohort::Status saved=c->state_;c->state_=WorldCohort::Complete;C(c->RequestShutdown()==E_UNEXPECTED&&c->state_==WorldCohort::Complete,"Complete/borrowed view cannot stop");c->state_=WorldCohort::LoadingSource;C(c->RequestShutdown()==E_UNEXPECTED&&c->state_==WorldCohort::LoadingSource,"active source cannot stop");c->state_=saved;
 c->queryPending_=true;C(c->RequestShutdown()==E_UNEXPECTED&&!c->Quarantined(),"pending GPU fence refuses before thread stop");c->queryPending_=false;c->gpu_->query.p=reinterpret_cast<ID3D11Query*>(1);C(c->RequestShutdown()==E_UNEXPECTED,"query owner refuses even nonpending");c->gpu_->query.p=NULL;c->job_.frame[0].side=512;c->gpu_->staging[0].p=reinterpret_cast<ID3D11Texture2D*>(1);C(c->ReservedGpuBytes()!=0&&c->RequestShutdown()==E_UNEXPECTED,"fake retained GPU reservation refuses stop");c->gpu_->staging[0].p=NULL;
 ForeignArgs other={c,S_OK,S_OK};HANDLE thread=CreateThread(NULL,0,Foreign,&other,0,NULL);C(thread&&::WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"foreign check joined");CloseHandle(thread);C(other.request==RPC_E_WRONG_THREAD&&other.poll==RPC_E_WRONG_THREAD&&c->State()==WorldCohort::Idle,"foreign shutdown calls do not mutate owner");
 C(c->RequestShutdown()==S_OK&&c->State()==WorldCohort::Stopping,"nonblocking stop accepted at real Idle boundary");C(c->RequestShutdown()==E_UNEXPECTED,"only Idle may request stop once");c->Cancel();C(c->State()==WorldCohort::Stopping&&!c->ReservedGpuBytes(),"Cancel cannot restart stop and proven GPU reservation stays zero");waitMode=2;SourceReceipts::Scope nil={};WorldBank::Ledger empty={};C(c->Pump(false,NULL,nil,empty)==S_FALSE&&c->State()==WorldCohort::Stopping,"Pump only polls pending stop");C(c->PollShutdown()==S_FALSE&&c->work_&&c->thread_&&WorldStream::WorkerSlot()==1,"unknown pending join retains actual owners and slot");waitMode=0;C(Poll(*c)==S_OK&&c->State()==WorldCohort::Stopped&&!c->work_&&!c->thread_&&!c->ReservedGpuBytes()&&WorldStream::WorkerSlot()==0&&!c->Quarantined(),"known WStopped and join retire worker asynchronously");C(c->PollShutdown()==S_OK,"completed poll is idempotent");delete c;
 // Reuse the global worker slot repeatedly after known joins.
 for(unsigned n=0;n<12;++n){c=Boot(p);C(c->RequestShutdown()==S_OK&&Poll(*c)==S_OK&&WorldStream::WorkerSlot()==0,"clean repeated worker-slot reuse");delete c;}
}
static void ErrorBeforeDelete(){Provider p;Controller*c=Boot(p);C(c->RequestShutdown()==S_OK,"request error-latch case");C(::WaitForSingleObject(c->thread_,5000)==WAIT_OBJECT_0,"actual cleanup thread finished");c->work_->error=HRESULT(0x8004abcdUL);C(c->PollShutdown()==HRESULT(0x8004abcdUL)&&c->Error()==HRESULT(0x8004abcdUL)&&!c->work_&&WorldStream::WorkerSlot()==0,"worker terminal HRESULT latched before owner delete");C(c->PollShutdown()==HRESULT(0x8004abcdUL),"terminal error remains observable");delete c;}
static void UnsolicitedEof(){Provider p;Controller*c=Boot(p);InterlockedExchange(&waitMode,3);C(SetEvent(c->work_->wake)!=0,"wake actual worker for unsolicited wait failure");C(::WaitForSingleObject(c->thread_,5000)==WAIT_OBJECT_0&&c->WorkerStatus()==WorldStream::WStopped,"actual worker reports unsolicited EOF");InterlockedExchange(&waitMode,0);SourceReceipts::Scope nil={};WorldBank::Ledger empty={};C(c->Pump(false,NULL,nil,empty)==E_FAIL&&c->Error()==E_FAIL&&!c->work_&&WorldStream::WorkerSlot()==0&&!c->Quarantined(),"existing Pump latches unsolicited EOF before deleting known joined worker");delete c;}
static void Fault(const char*name){Provider p;Controller*c=Boot(p);WorldStream::Work*work=c->work_;WorldCohort::GpuOwners*gpu=c->gpu_;HANDLE thread=c->thread_;
 if(!std::strcmp(name,"wait-failed-request")){waitMode=1;C(FAILED(c->RequestShutdown()),"unknown initial thread wait refuses");}
 else{clockOverride=true;fakeTick=0xfffffff0UL;C(c->RequestShutdown()==S_OK,"request fault case");
  if(!std::strcmp(name,"wait-failed")){waitMode=1;C(FAILED(c->PollShutdown()),"failed zero-time poll refuses");}
  else if(!std::strcmp(name,"timeout-wrap")){waitMode=2;fakeTick=DWORD(fakeTick+4999);C(c->PollShutdown()==S_FALSE&&!c->Quarantined(),"wrap-safe pending at4999ms");fakeTick=c->shutdownAt_;fakeTick+=5000;C(FAILED(c->PollShutdown()),"wrap-safe deadline at5000ms");}
  else if(!std::strcmp(name,"missing-marker")){C(::WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"actual thread joined externally for marker fault");InterlockedExchange(&work->state,WorldStream::WIdle);C(FAILED(c->PollShutdown()),"known exit without cleanup marker quarantines");}
  else C(false,"known fault name");
 }
 C(c->Quarantined()&&c->work_==work&&c->gpu_==gpu&&c->thread_==thread&&WorldStream::WorkerSlot()==1,"unproven closure retains full owners and global slot");const WorldCohort::QuarantineRecord&q=WorldCohort::Quarantine();C(q.marked&&q.work==work&&q.gpu==gpu&&q.thread==thread&&q.cpuBytes==c->CpuReservation(),"actual quarantine records retained ownership and CPU debt");delete c;C(q.work==work&&q.gpu==gpu&&q.thread==thread&&WorldStream::WorkerSlot()==1,"destructor does not free quarantined owners");
 // Each fault runs in its own host process. OS exit reclaims injected unknown
 // owners; this is not a production recovery or proof of a known join.
}
int main(int argc,char**argv){try{if(argc==1){Normal();ErrorBeforeDelete();UnsolicitedEof();}else Fault(argv[1]);printf("PASS %u actual idle-worker asynchronous shutdown controls\n",checks);return 0;}catch(...){return 1;}}
