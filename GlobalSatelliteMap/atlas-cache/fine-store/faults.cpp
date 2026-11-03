#define NOMINMAX
#include <windows.h>
static int fault=0;static unsigned writeCalls=0;static bool Target(HANDLE h){LARGE_INTEGER n={};return GetFileSizeEx(h,&n)&&n.QuadPart==1049600;}
static BOOL WINAPI TestWrite(HANDLE h,LPCVOID p,DWORD n,LPDWORD done,LPOVERLAPPED over){if(Target(h)){++writeCalls;if(fault==1&&writeCalls==2){*done=0;SetLastError(ERROR_WRITE_FAULT);return FALSE;}if(fault==2&&n>7)n=7;}return WriteFile(h,p,n,done,over);}
static BOOL WINAPI TestRead(HANDLE h,LPVOID p,DWORD n,LPDWORD done,LPOVERLAPPED over){if(fault==3&&Target(h)){*done=0;return TRUE;}return ReadFile(h,p,n,done,over);}
static BOOL WINAPI TestFlush(HANDLE h){if(fault==4&&Target(h)){SetLastError(ERROR_WRITE_FAULT);return FALSE;}return FlushFileBuffers(h);}
static BOOL WINAPI TestClose(HANDLE h){BY_HANDLE_FILE_INFORMATION info={};bool directory=GetFileInformationByHandle(h,&info)&&(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY);if(((fault==5||fault==6||fault==7)&&Target(h))||(fault==8&&directory)){SetLastError(ERROR_WRITE_FAULT);return FALSE;}return CloseHandle(h);}
#define WriteFile TestWrite
#define ReadFile TestRead
#define FlushFileBuffers TestFlush
#define CloseHandle TestClose
#define FINE_STORE_FIXTURE_ONLY
#include "tests.cpp"
static DWORD WINAPI Faults(void*p){try{Input&input=*static_cast<Input*>(p);Fixture fixture;FineStore::Identity id=Identity();FineStore::Ledger ledger=fixture.Budget();FineWindows::Key key=Key(0);WorldRaster::Frame frame=fixture.Build(key);
 for(unsigned mode=1;mode<=8;++mode){wchar_t leaf[40];_snwprintf_s(leaf,40,_TRUNCATE,L"\\fault_%u",mode);FineStore::Config config={};C(FineStore::Configure(config,input.parent,(std::wstring(input.parent)+leaf).c_str()),"fault output pure config");FineStore::Session session;C(session.Begin(true,fixture.scope,id,config,ledger)==S_OK,"fault worker session admitted");FineStore::Receipt receipt={};writeCalls=0;fault=(mode==3||mode>=6)?0:mode;HRESULT hr=session.Put(true,key,id,fixture.accepted,fixture.scope,frame,fixture.window,NULL,ledger,receipt);
  if(mode==2){C(hr==S_OK&&writeCalls>100000&&session.Check(receipt),"actual repeated short writes complete exact bounded payload");FineStore::Blob blob;fault=0;C(session.Get(true,key,id,NULL,ledger,blob)==S_OK,"short-write result has correct independent file SHA");}
  else if(mode==3){C(hr==S_OK&&session.Check(receipt),"read EOF fault baseline sealed write");FineStore::Blob blob;fault=3;C(FAILED(session.Get(true,key,id,NULL,ledger,blob))&&!blob.OwnedBytes()&&session.Poisoned(),"actual ReadFile zero-byte EOF cannot publish blob");}else if(mode==6){C(hr==S_OK,"read close failure baseline");FineStore::Blob blob;fault=6;C(FAILED(session.Get(true,key,id,NULL,ledger,blob))&&!blob.OwnedBytes()&&session.NeedsRetention(),"read close failure retains live file handle and withholds output");}else if(mode==7){C(hr==S_OK,"replacement close failure baseline");fault=7;C(FAILED(session.Put(true,key,id,fixture.accepted,fixture.scope,frame,fixture.window,NULL,ledger,receipt))&&!receipt.serial&&session.NeedsRetention(),"eviction close failure retains delete-pending handle and never replaces");}else if(mode==8){C(hr==S_OK,"directory close failure baseline");fault=8;C(FAILED(session.Close())&&session.NeedsRetention(),"failed directory close retains owning handle");}
  else C(FAILED(hr)&&!receipt.serial&&session.Poisoned(),"write/flush/known-close failure never seals receipt");
  if(mode==5)C(session.NeedsRetention(),"failed Put close retains live file handle");fault=0;HRESULT closed=session.Close();C(!session.NeedsRetention(),"retained handles released only after later known successful close");C(mode==2?closed==S_OK:FAILED(closed),"writer teardown cannot erase latched IO error");
 }
 input.good=true;return 0;}catch(...){fault=0;return 1;}}
int wmain(int argc,wchar_t**argv){try{C(argc==2,"fault runner-owned root");mainThread=GetCurrentThreadId();Input input={};C(std::wcslen(argv[1])<221,"fault fixture path bounded");wcscpy_s(input.parent,221,argv[1]);HANDLE thread=CreateThread(NULL,0,Faults,&input,0,NULL);C(thread&&WaitForSingleObject(thread,180000)==WAIT_OBJECT_0,"fault worker known joined");DWORD exit=~0u;C(GetExitCodeThread(thread,&exit)&&!exit&&input.good,"fault worker success");CloseHandle(thread);std::printf("PASS %u fine-store actual syscall fault controls\n",checks);return 0;}catch(...){return 1;}}
