#include <windows.h>
#include <vector>
#include <cwchar>
#include <cstring>
#include <cstdio>
typedef unsigned long long U64;
namespace WorldHeight {
const U64 MaxSamples=1029*1029;
struct Region {int x0,z0,x1,z1;};
struct Identity {U64 epoch;};
struct Operation {bool cancelled;Operation():cancelled(false){}void Reset(){cancelled=false;}void Cancel(){cancelled=true;}void Observe(void(*)(void*),void*){}DWORD Error(){return ERROR_CANCELLED;}};
struct Window {std::vector<BYTE> bytes;void Clear(){std::vector<BYTE>().swap(bytes);}};
// Only the file source is substituted: tiny real deny-write/delete file handle,
// authentication counter and epoch stand in for the 537MB TIFF authentication.
struct Source {
 HANDLE file;Identity identity;unsigned opens,closes;
 Source():file(INVALID_HANDLE_VALUE),opens(0),closes(0){identity.epoch=0;}~Source(){Close();}
 bool Ready()const{return file!=INVALID_HANDLE_VALUE;}const Identity&Stamp()const{return identity;}
 void Close(){if(Ready()){CloseHandle(file);++closes;}file=INVALID_HANDLE_VALUE;identity.epoch=0;}
 bool Open(const wchar_t*path,U64 epoch,Operation&o){if(o.cancelled)return false;file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,NULL);if(!Ready())return false;++opens;identity.epoch=epoch;return true;}
 bool Read(const Region&,U64 epoch,Window&w,Operation&o){if(o.cancelled||!Ready()||epoch!=identity.epoch)return false;w.bytes.resize(32);return true;}
};
}
namespace WorldBank {struct RgbaPin {unsigned bytes;};RgbaPin RgbaPins[129];}
namespace SourceReceipts {
struct Scope{};struct WorkerRequest{struct{U64 bytes;}pin;};
struct WorkerFileLease {std::vector<BYTE>bytes;WorkerRequest request;void Clear(){std::vector<BYTE>().swap(bytes);}const BYTE*Data(){return bytes.empty()?NULL:&bytes[0];}const WorkerRequest&Request(){return request;}HRESULT Open(const WorkerRequest&,HANDLE,U64){bytes.resize(4);return S_OK;}};
}
namespace PinnedPng {struct Decoder{HRESULT Init(){return S_OK;}void Clear(){}HRESULT Decode(const WorldBank::RgbaPin&,const void*,size_t,void*,size_t,HANDLE){return S_OK;}};}
namespace WorldRaster {struct Frame{unsigned side;};inline bool CarryBytes(unsigned){return true;}}
namespace WorldPixels {
struct Raw{SourceReceipts::Scope scope;WorldRaster::Frame frame;BYTE*a;size_t an;BYTE*b;size_t bn;BYTE*c;size_t cn;};
struct Accepted{bool ready;Accepted():ready(false){}bool Ready()const{return ready;}void Clear(){ready=false;}HRESULT Build(bool,const Raw&,const SourceReceipts::Scope&,const WorldRaster::Frame&,const WorldHeight::Window&,HANDLE,U64){ready=true;return S_OK;}};
}
namespace WorldStream {
// EXTRACTED_WORKER
}
struct CohortControl {
 enum Status{Stopped,Booting,Idle,Complete,Cancelling,Failed,Stopping};
 struct Bank{U64 bytes;Bank():bytes(1){}U64 OwnedGpuBytes(){return bytes;}};
 struct Device{HRESULT GetDeviceRemovedReason(){return S_OK;}};
 struct Gpu{Bank bank;struct{Device*p;}device;Gpu(){device.p=NULL;}}gpu,*gpu_;
 struct CapturedSources{}captured_;
 struct Receipt{void Clear(){}}accepted_[15];
 WorldStream::Work*work_;bool quarantined_,queryPending_,closeSent_,retainHeight_,trimSent_,mainThread;Status state_;HRESULT error_;unsigned cleared,sent,rasters;HRESULT fenceResult;
 CohortControl(WorldStream::Work&w):gpu_(&gpu),work_(&w),quarantined_(false),queryPending_(false),closeSent_(false),retainHeight_(false),trimSent_(false),mainThread(true),state_(Idle),error_(S_OK),cleared(0),sent(0),rasters(0),fenceResult(S_OK){}
 bool OnMain(){return mainThread;}LONG WorkerStatus(){return InterlockedCompareExchange(&work_->state,0,0);}void CancelRasters(){++rasters;}void Fence(){}HRESULT Drained(){return fenceResult;}void GpuClear(){++cleared;gpu.bank.bytes=0;}void QuarantineOwners(){quarantined_=true;}void ReleaseJoinedWorker(){}
 // EXTRACTED_CONTROLLER
 HRESULT PumpCancel(){LONG w=WorkerStatus();const bool exited=false;
 // EXTRACTED_CANCEL_STEP
 return S_FALSE;}
};
static void C(bool value,const char*name){if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::fflush(NULL);ExitProcess(1);}std::printf("PASS %s\n",name);}
static void Send(WorldStream::Work&w,WorldStream::Command command,LONG expected){InterlockedExchange(&w.state,WorldStream::WBusy);InterlockedExchange(&w.command,command);C(SetEvent(w.wake)!=FALSE,"wake worker");DWORD started=GetTickCount();while(InterlockedCompareExchange(&w.state,0,0)==WorldStream::WBusy&&GetTickCount()-started<2000)Sleep(1);C(w.state==expected,"worker command acknowledged");}
static void Fixture(const wchar_t*path){HANDLE h=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);C(h!=INVALID_HANDLE_VALUE,"create tiny fixture");DWORD got=0;BYTE b[8]={};C(WriteFile(h,b,sizeof(b),&got,NULL)&&got==sizeof(b),"write tiny fixture");CloseHandle(h);}
static bool WriteDenied(const wchar_t*path){HANDLE h=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,0,NULL);if(h!=INVALID_HANDLE_VALUE){CloseHandle(h);return false;}return GetLastError()==ERROR_SHARING_VIOLATION;}
int main(){
 using namespace WorldStream;const wchar_t*a=L"height-a.tiny",*b=L"height-b.tiny";Fixture(a);Fixture(b);
 Work w;w.wake=CreateEventW(NULL,FALSE,FALSE,NULL);w.cancel=CreateEventW(NULL,TRUE,FALSE,NULL);HANDLE thread=CreateThread(NULL,0,Work::Run,&w,0,NULL);C(thread!=NULL,"start actual production worker");DWORD started=GetTickCount();while(w.state==WBoot&&GetTickCount()-started<2000)Sleep(1);C(w.state==WIdle,"worker boot joined publication");
 std::wcscpy(w.path,a);w.epoch=1;w.side=2;Send(w,Height,WHeight);C(w.height.opens==1&&WriteDenied(a),"authenticated handle denies writes");
 SetEvent(w.cancel);Send(w,ORDINARY_COMMAND,WClosed);C(w.window.bytes.capacity()==0&&w.pixels.capacity()==0&&w.rgba.capacity()==0&&!w.accepted.ready,"ordinary drain retains no raw payload");ResetEvent(w.cancel);
 Send(w,Height,WHeight);C(w.height.opens==1,"ordinary drain reuses authenticated height handle");C(WriteDenied(a),"retained identity still holds original file lock");
 C(!DeleteFileW(a)&&GetLastError()==ERROR_SHARING_VIOLATION,"retained file identity cannot be deleted or replaced");
 w.epoch=2;Send(w,Height,WHeight);C(w.height.opens==2&&w.height.closes==1,"epoch change fully closes and authenticates replacement");
 std::wcscpy(w.path,b);Send(w,Height,WHeight);C(w.height.opens==3&&w.height.closes==2&&!WriteDenied(a)&&WriteDenied(b),"path change releases old identity and locks replacement");
 SetEvent(w.cancel);Send(w,Close,WClosed);C(!w.height.Ready()&&!w.retainedPath[0]&&!WriteDenied(b),"terminal Close drops identity and lock even while cancelled");
 Send(w,Height,WError);C(!w.height.Ready()&&w.height.opens==3,"cancelled worker cannot reopen or publish height");ResetEvent(w.cancel);Send(w,Height,WHeight);
 // EXTRACTED_CONTROL_TESTS
 InterlockedExchange(&w.command,Stop);SetEvent(w.wake);C(WaitForSingleObject(thread,2000)==WAIT_OBJECT_0,"Stop joins actual worker");C(w.state==WStopped&&!w.height.Ready()&&!WriteDenied(b),"joined Stop closes retained identity and lock");
 CloseHandle(thread);CloseHandle(w.wake);CloseHandle(w.cancel);C(DeleteFileW(a)&&DeleteFileW(b),"tiny fixtures removed after proven worker join");return 0;
}
