#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <new>
#include <cstdlib>
static volatile LONG denyPayload=0,fileReads=0;
void*operator new[](size_t n,const std::nothrow_t&)throw(){return InterlockedCompareExchange(&denyPayload,0,0)&&n==1048576?NULL:std::malloc(n);}
static BOOL WINAPI ReadCount(HANDLE h,LPVOID p,DWORD n,LPDWORD done,LPOVERLAPPED o){InterlockedIncrement(&fileReads);return ::ReadFile(h,p,n,done,o);}
#define ReadFile ReadCount
#define FINE_WORKER_FIXTURE_ONLY
#include "../fine-worker/tests.cpp"
#undef ReadFile
int wmain(int argc,wchar_t**argv){try{C(argc==2,"owned allocation root");mainThread=GetCurrentThreadId();HANDLE maker=CreateThread(NULL,0,MakeFixture,NULL,0,NULL);C(maker&&WaitForSingleObject(maker,10000)==WAIT_OBJECT_0,"accepted source ready");DWORD code=~0u;C(GetExitCodeThread(maker,&code)&&!code,"source known joined");CloseHandle(maker);FineStore::Identity id=Identity();FineStore::Ledger external=fixture->Budget();external.other+=fixture->accepted.OwnedBytes();FineWrite::Worker worker;C(worker.Start(fixture->scope,id,Config(argv[1],L"normal"),external),"worker starts with optional RAM");Boot(worker);FineStore::Ledger empty;
 InterlockedExchange(&denyPayload,1);C(worker.SubmitPut(Borrow(),Key(0),id,empty),"valid Put despite cache allocation pressure");FineStore::Receipt receipt=Written(worker);C(receipt.serial&&worker.MemoryStats().entries==0,"RAM payload OOM does not turn accepted disk Put into failure");InterlockedExchange(&denyPayload,0);fixture->accepted.Clear();InterlockedExchange(&fileReads,0);C(worker.SubmitGet(Key(0),id,external)&&Read(worker)==1,"cache OOM falls back to real validated disk Get");C(fileReads>0&&worker.MemoryStats().entries==1&&worker.RamBytes()>1048576,"successful disk Get populates optional RAM");FineStore::View v={};C(worker.PeekRead(true,fixture->scope,Key(0),id,v)&&worker.ReleaseRead(),"validated disk clone acknowledged");InterlockedExchange(&fileReads,0);C(worker.SubmitGet(Key(0),id,external)&&Read(worker)==1&&!fileReads&&worker.MemoryStats().hits==1,"next demand reads retained ready bytes without IO");C(worker.ReleaseRead(),"RAM clone acknowledged");worker.Cancel();Joined(worker);C(!worker.Quarantined()&&!worker.RamBytes()&&!worker.RetainedRamBytes(),"normal joined worker releases all retained RAM");delete fixture;fixture=NULL;std::printf("PASS %u actual optional-RAM allocation fallback controls\n",checks);return 0;}catch(...){return 1;}}
