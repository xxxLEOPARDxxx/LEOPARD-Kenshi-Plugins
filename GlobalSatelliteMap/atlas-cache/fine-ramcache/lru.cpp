#define _CRT_SECURE_NO_WARNINGS
#include <new>
#include <cstdlib>
static bool denyPayload=false;
void*operator new[](size_t n,const std::nothrow_t&)throw(){return denyPayload&&n==1048576?NULL:std::malloc(n);}
#define FINE_WORKER_FIXTURE_ONLY
#include "../fine-worker/tests.cpp"
struct LruInput{const wchar_t*parent;bool good;};
static DWORD WINAPI LruRun(void*arg){try{
 LruInput&input=*static_cast<LruInput*>(arg);Fixture f;FineStore::Identity id=Identity();FineStore::Ledger ledger=f.Budget();FineStore::Session disk;C(disk.Begin(true,f.scope,id,Config(input.parent,L"case9"),ledger)==S_OK,"real disk session belongs to worker");
 FineRam::Cache cache;const FineStore::U64 cap=FineRam::Cache::BaseBytes()+2*FineRam::Cache::EntryBytes();C(cache.Begin(f.scope,id,cap)&&cache.Bytes()==FineRam::Cache::BaseBytes(),"RAM cache allocates bounded metadata only");
 FineStore::Blob original,held;FineStore::View v={};BYTE saved=0;
 for(unsigned i=0;i<3;++i){WorldRaster::Frame frame=f.Build(Key(i));FineStore::Receipt receipt={};C(disk.Put(true,Key(i),id,f.accepted,f.scope,frame,f.window,NULL,ledger,receipt)==S_OK,"real accepted tile sealed on disk");C(disk.Get(true,Key(i),id,NULL,ledger,original)==S_OK,"real file identity and SHA validated source");C(cache.Remember(original,Key(i),id,NULL),"validated ready Blob retained in RAM");
  if(i==1){C(cache.Get(Key(0),id,NULL,ledger,held)==S_OK&&held.Acquire(true,Key(0),id,v),"touching oldest tile publishes independent read clone");saved=v.material[0];}
 }
 C(cache.Entries()==2&&cache.Bytes()==cap,"payload and node/hash metadata obey inclusive RAM cap");C(cache.Get(Key(1),id,NULL,ledger,original)==S_FALSE&&!original.OwnedBytes(),"LRU evicts untouched middle tile, not recent oldest");C(cache.Get(Key(0),id,NULL,ledger,original)==S_OK,"recently touched tile survives eviction");
 FineStore::Identity stale=id;stale.baseSha[31]^=1;C(cache.Get(Key(0),stale,NULL,ledger,original)==S_FALSE&&!original.OwnedBytes(),"full base digest prevents stale ready-byte reuse");stale=id;stale.sourceEpoch^=1ULL<<32;C(cache.Get(Key(0),stale,NULL,ledger,original)==S_FALSE,"high64bit source identity is not truncated");
 FineStore::Ledger edge=ledger;edge.other=SourceReceipts::CpuCap-FineStore::Session::FixedCpuBytes()-FineStore::Session::GetCpuBytes()-edge.retainedRaw;C(cache.Get(Key(0),id,NULL,edge,original)==S_OK,"RAM clone honors inclusive64MiB scratch ledger separately");++edge.other;C(cache.Get(Key(0),id,NULL,edge,original)==E_OUTOFMEMORY&&!original.OwnedBytes(),"one excess scratch byte cannot hide under RAM budget");
 denyPayload=true;C(cache.Get(Key(0),id,NULL,ledger,original)==E_OUTOFMEMORY&&!original.OwnedBytes(),"clone allocation failure exposes no aliased cache pointer");denyPayload=false;C(disk.Get(true,Key(1),id,NULL,ledger,original)==S_OK&&disk.Healthy(),"allocation refusal never poisons valid disk record");denyPayload=true;C(!cache.Remember(original,Key(1),id,NULL)&&cache.Bytes()<=cap,"optional cache retention allocation failure is bounded");denyPayload=false;
 HANDLE cancel=CreateEvent(NULL,TRUE,TRUE,NULL);C(cancel&&cache.Get(Key(0),id,cancel,ledger,original)==HRESULT_FROM_WIN32(ERROR_CANCELLED),"cancel refuses before read allocation");C(!cache.Remember(held,Key(0),id,cancel),"cancel skips optional retention");CloseHandle(cancel);
 cache.Clear();C(cache.Bytes()==0&&cache.Entries()==0&&v.material[0]==saved,"cache eviction and terminal clear never invalidate held clone");C(held.Acquire(true,Key(0),id,v)&&v.material[0]==saved,"read clone owns its bytes after full cache clear");held.Clear();
 const FineStore::U64 limit=32ULL*1024*1024*1024,n=FineRam::Cache::CapacityFor(limit);C(n>32000&&n<32768&&FineRam::Cache::BaseBytes()+n*FineRam::Cache::EntryBytes()<=limit,"near32GiB capacity arithmetic accounts metadata without overflow");C(FineRam::Cache::BaseBytes()+(n+1)*FineRam::Cache::EntryBytes()>limit,"one additional near32GiB entry cannot fit");FineRam::Cache large;C(large.Begin(f.scope,id,limit)&&large.Bytes()<1024*1024&&large.Entries()==0,"32GiB profile is lazy, no eager GiB allocation");large.Clear();FineRam::Cache invalid;C(!invalid.Begin(f.scope,id,limit+1),"invalid over32GiB cap refuses before allocation");
 C(disk.DiscardKnownClosed()==S_OK&&disk.Close()==S_OK,"worker removes only known closed disk fixture records");input.good=true;return 0;
 }catch(...){return 1;}}
int wmain(int argc,wchar_t**argv){try{C(argc==2,"owned LRU root");mainThread=GetCurrentThreadId();LruInput input={argv[1],false};HANDLE worker=CreateThread(NULL,0,LruRun,&input,0,NULL);C(worker&&WaitForSingleObject(worker,10000)==WAIT_OBJECT_0,"LRU owner known joined");DWORD exit=~0u;C(GetExitCodeThread(worker,&exit)&&!exit&&input.good,"actual LRU controls complete");CloseHandle(worker);std::printf("PASS %u actual RAM LRU/OOM/identity/budget controls\n",checks);return 0;}catch(...){return 1;}}
