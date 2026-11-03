#define NOMINMAX
#include <windows.h>
static int cleanupFault=0;
static bool TargetCleanup(HANDLE h){LARGE_INTEGER n={};return GetFileSizeEx(h,&n)&&n.QuadPart==1049600;}
static BOOL WINAPI CleanupClose(HANDLE h){if(cleanupFault==3&&TargetCleanup(h)){SetLastError(ERROR_WRITE_FAULT);return FALSE;}return CloseHandle(h);}
static BOOL WINAPI CleanupDisposition(HANDLE h,FILE_INFO_BY_HANDLE_CLASS kind,LPVOID p,DWORD n){if(cleanupFault==4&&kind==FileDispositionInfo&&TargetCleanup(h)){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}return SetFileInformationByHandle(h,kind,p,n);}
#define CloseHandle CleanupClose
#define SetFileInformationByHandle CleanupDisposition
#define FINE_STORE_FIXTURE_ONLY
#include "tests.cpp"
static bool Exists(const std::wstring&p){return GetFileAttributesW(p.c_str())!=INVALID_FILE_ATTRIBUTES;}
static void ReplaceForeign(const std::wstring&path){
 HANDLE h=CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,0,NULL,OPEN_EXISTING,0,NULL);C(h!=INVALID_HANDLE_VALUE,"old physical cache ID available");FineStore::Detail::FileId oldId={};FineStore::U64 bytes=0;C(FineStore::Detail::FileInfo(h,oldId,bytes)&&CloseHandle(h),"old physical cache ID copied");C(DeleteFileW(path.c_str())!=FALSE,"test replaces only its own fixture file");h=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);C(h!=INVALID_HANDLE_VALUE&&FineStore::Detail::Size(h,FineStore::FileBytes),"foreign replacement has identical size");FineStore::Detail::FileId newId={};C(FineStore::Detail::FileInfo(h,newId,bytes)&&!FineStore::Detail::SameId(oldId,newId)&&CloseHandle(h),"actual replacement has different physical identity");
}
static DWORD WINAPI RunCleanup(void*p){try{
 Input&input=*static_cast<Input*>(p);Fixture fixture;FineStore::Identity id=Identity();FineStore::Ledger ledger=fixture.Budget();
 for(unsigned mode=1;mode<=4;++mode){wchar_t leaf[32];_snwprintf_s(leaf,32,_TRUNCATE,L"\\fault_%u",mode);FineStore::Config config={};C(FineStore::Configure(config,input.parent,(std::wstring(input.parent)+leaf).c_str()),"known new cleanup child config");FineStore::Session session;C(session.Begin(true,fixture.scope,id,config,ledger)==S_OK,"cleanup worker Session starts");
  FineStore::Receipt receipt[2]={};for(unsigned n=0;n<2;++n){FineWindows::Key key=Key(n);WorldRaster::Frame frame=fixture.Build(key);C(session.Put(true,key,id,fixture.accepted,fixture.scope,frame,fixture.window,NULL,ledger,receipt[n])==S_OK,"two actual known-closed records");}
  std::wstring first=File(config.directory,receipt[0].slot),last=File(config.directory,receipt[1].slot);C(Exists(first)&&Exists(last),"known payloads present before teardown");if(mode==2)ReplaceForeign(last);cleanupFault=mode>=3?int(mode):0;HRESULT hr=session.DiscardKnownClosed();
  if(mode==1){C(hr==S_OK&&session.Records()==0&&!session.Check(receipt[0])&&!session.Check(receipt[1]),"healthy discard invalidates old receipts");C(!Exists(first)&&!Exists(last),"healthy discard removes exact owned payloads");C(session.DiscardKnownClosed()==S_OK,"empty known index cleanup is bounded and idempotent");}
  else{C(FAILED(hr)&&session.Poisoned()&&Exists(first)&&Exists(last),"foreign/preflight-close/delete failure never sweeps cache files");C(mode==3?session.NeedsRetention():!session.NeedsRetention(),"only unknown live close requires retained owner");C(FAILED(session.DiscardKnownClosed()),"poisoned forensic Session cannot be retried as healthy cleanup");}
  cleanupFault=0;C((mode==1?session.Close()==S_OK:FAILED(session.Close()))&&!session.NeedsRetention(),"known teardown preserves failure after retained-close recovery");
 }
 input.good=true;return 0;
 }catch(...){cleanupFault=0;return 1;}}
int wmain(int argc,wchar_t**argv){try{C(argc==2,"cleanup runner root");mainThread=GetCurrentThreadId();FineStore::Session empty;C(empty.DiscardKnownClosed()==E_ACCESSDENIED,"unowned Main cannot run file cleanup");Input input={};C(std::wcslen(argv[1])<221,"bounded cleanup fixture path");wcscpy_s(input.parent,221,argv[1]);HANDLE thread=CreateThread(NULL,0,RunCleanup,&input,0,NULL);C(thread&&WaitForSingleObject(thread,180000)==WAIT_OBJECT_0,"cleanup worker known joined");DWORD exit=1;C(GetExitCodeThread(thread,&exit)&&exit==0&&input.good,"cleanup controls complete");CloseHandle(thread);printf("PASS %u fine-store known-owned cleanup controls\n",checks);return 0;}catch(...){return 1;}}
