#define NOMINMAX
#include <Windows.h>
static unsigned finalCalls=0,wrongFinalAt=0,infoCalls=0;
static bool changedInfo=false,shortRead=false,closeFailure=false,lockedWrite=false;
static HANDLE sourceHandle=INVALID_HANDLE_VALUE;
static DWORD WINAPI FinalName(HANDLE h,LPWSTR out,DWORD size,DWORD flags){++finalCalls;if(finalCalls==wrongFinalAt){const wchar_t wrong[]=L"\\\\?\\C:\\wrong.mesh";wcscpy_s(out,size,wrong);return DWORD(wcslen(wrong));}return GetFinalPathNameByHandleW(h,out,size,flags);}
static BOOL WINAPI FileInfo(HANDLE h,LPBY_HANDLE_FILE_INFORMATION out){BOOL ok=GetFileInformationByHandle(h,out);if(ok&&changedInfo&&++infoCalls==2)++out->ftLastWriteTime.dwLowDateTime;return ok;}
static BOOL WINAPI ReadSource(HANDLE h,LPVOID p,DWORD n,LPDWORD done,LPOVERLAPPED overlapped){sourceHandle=h;BOOL ok=ReadFile(h,p,shortRead?n-1:n,done,overlapped);if(lockedWrite){wchar_t path[MAX_PATH+4];DWORD count=GetFinalPathNameByHandleW(h,path,MAX_PATH+4,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);if(count){HANDLE writer=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);if(writer!=INVALID_HANDLE_VALUE){CloseHandle(writer);throw 1;}if(GetLastError()!=ERROR_SHARING_VIOLATION)throw 1;}}return ok;}
static BOOL WINAPI CloseSource(HANDLE h){BOOL ok=CloseHandle(h);if(h==sourceHandle&&closeFailure){SetLastError(ERROR_INVALID_HANDLE);return FALSE;}return ok;}
#define GetFinalPathNameByHandleW FinalName
#define GetFileInformationByHandle FileInfo
#define ReadFile ReadSource
#define CloseHandle CloseSource
#ifndef CACHED_JOB_HEADER
#define CACHED_JOB_HEADER "../GsmAtlasJob.h"
#endif
#include CACHED_JOB_HEADER
#undef GetFinalPathNameByHandleW
#undef GetFileInformationByHandle
#undef ReadFile
#undef CloseHandle
#include <cstdio>
#include <cstdlib>
#include <string>
namespace GSM { volatile LONG generation=7; }
static unsigned checks=0;
static void C(bool value,const char*why){++checks;if(!value){std::printf("FAIL %s\n",why);throw 1;}}
struct Observe {unsigned calls,bytes;bool cancel,throwAfter;Observe():calls(0),bytes(0),cancel(false),throwAfter(false){}};
static bool Stop(void*p){return static_cast<Observe*>(p)->cancel;}
static void Chunk(void*p,unsigned n){Observe&o=*static_cast<Observe*>(p);++o.calls;o.bytes+=n;if(o.throwAfter)throw 1;}
static void CancelChunk(void*p,unsigned n){Chunk(p,n);static_cast<Observe*>(p)->cancel=true;}
static void Write(const std::wstring&p,const std::vector<unsigned char>&bytes){HANDLE f=CreateFileW(p.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);C(f!=INVALID_HANDLE_VALUE,"fixture open");DWORD done=0;bool ok=WriteFile(f,bytes.empty()?NULL:&bytes[0],DWORD(bytes.size()),&done,NULL)!=FALSE;bool closed=CloseHandle(f)!=FALSE;C(ok&&done==bytes.size()&&closed,"fixture exact write/close");}
static std::wstring Name(const std::string&root,unsigned id){wchar_t n[48];swprintf_s(n,48,L"distant_%u.mesh",id);return GsmAtlas::Wide(root)+L"data\\meshes\\distant\\"+n;}
int main(int argc,char**argv){try{
 C(argc==5,"arguments game fixture townId junction");const std::string game=argv[1],root=argv[2];const unsigned id=unsigned(std::strtoul(argv[3],NULL,10));
 GsmCachedTown::Request request;request.townId=id;request.storedXYZ[0]=1.25f;request.storedXYZ[1]=0;request.storedXYZ[2]=-2.5f;
 GsmCachedTown::Result result;Observe observer;
 C(result.state==GsmCachedTown::Empty&&result.bytes.empty(),"empty owned result");
 C(GsmCachedTown::Read(game,request,result,Stop,&observer,Chunk)==GsmCachedTown::Ready,"actual native cached mesh read and parser");
 C(!result.bytes.empty()&&result.bytes.size()<=GsmCachedTown::MaxBytes&&observer.bytes==result.bytes.size(),"bounded exact file bytes");
 unsigned char actual[32];C(AtlasView::Digest(&result.bytes[0],DWORD(result.bytes.size()),actual)&&!std::memcmp(actual,result.sha256,32),"full coherent SHA256");
 PrivateTownLod::BlobInfo info;C(PrivateTownLod::Parser(&result.bytes[0],result.bytes.size()).Read(info)&&!std::memcmp(info.localBounds,result.info.localBounds,24)&&info.signature==result.info.signature,"actual parser bounds/signature returned");
 std::vector<unsigned char>original=result.bytes;Write(Name(root,id),original);observer=Observe();
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Ready&&result.bytes==original,"owned fixture same bytes");
 C(observer.calls==(original.size()+65535)/65536,"at most64KiB read chunks");
 std::vector<unsigned char>changed=original;size_t vertex=0;for(size_t at=0;at+6<changed.size();++at){unsigned length=0;std::memcpy(&length,&changed[at+2],4);if(changed[at]==0x10&&changed[at+1]==0x52&&length==6+info.vertexBytes&&length<=changed.size()-at){vertex=at+6;break;}}C(vertex!=0,"fixture native vertex-data chunk");changed[vertex]^=1;Write(Name(root,id),changed);observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Ready&&std::memcmp(actual,result.sha256,32),"valid changed cache accepted with fresh SHA not a hardcoded Hub identity");Write(Name(root,id),original);
 observer=Observe();lockedWrite=true;C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Ready,"source denies concurrent writer throughout actual read");lockedWrite=false;
 finalCalls=0;wrongFinalAt=1;observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&observer.bytes==0&&result.bytes.empty(),"final path mismatch refuses before payload IO");
 finalCalls=0;wrongFinalAt=2;observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&observer.bytes==original.size()&&result.bytes.empty(),"final path rechecked after read before publication");wrongFinalAt=0;
 changedInfo=true;infoCalls=0;observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty(),"changed file information refuses payload");changedInfo=false;
 shortRead=true;observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty(),"short successful ReadFile never accepts partial source");shortRead=false;
 closeFailure=true;observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty(),"close failure never publishes Ready");closeFailure=false;
 observer=Observe();C(GsmCachedTown::Read(argv[4],request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&observer.bytes==0,"actual directory junction refused before payload IO");
 observer=Observe();observer.cancel=true;
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Cancelled&&result.bytes.empty()&&!AtlasView::Nonzero(result.sha256,32)&&!result.info.vertices&&observer.bytes==0,"cancel retires old accepted payload before IO");
 observer=Observe();
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,CancelChunk)==GsmCachedTown::Cancelled&&result.bytes.empty()&&observer.calls==1&&observer.bytes<=65536,"midread cancel clears partial payload at chunk boundary");
 observer=Observe();observer.throwAfter=true;
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty(),"callback exception contained with no accepted payload");
 HANDLE write=CreateFileW(Name(root,id).c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);C(write!=INVALID_HANDLE_VALUE,"writer fixture handle");observer=Observe();
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty()&&result.winError==ERROR_SHARING_VIOLATION,"preexisting writer cannot yield incoherent source");C(CloseHandle(write)!=FALSE,"writer fixture close");
 observer=Observe();GsmCachedTown::Request missing=request;missing.townId=4294967295U;
 C(GsmCachedTown::Read(root,missing,result,Stop,&observer,Chunk)==GsmCachedTown::Unavailable&&result.bytes.empty()&&!result.error[0],"missing unsigned filename is unavailable not malformed terrain");
 std::vector<unsigned char>bad=original;bad[0]^=0xff;Write(Name(root,id),bad);observer=Observe();
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&result.bytes.empty()&&!result.info.vertices,"malformed native mesh refused before import");
 bad.assign(512*1024+1,0);Write(Name(root,id),bad);observer=Observe();
 C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&observer.bytes==0,"oversize refused before payload IO");
 bad.clear();Write(Name(root,id),bad);observer=Observe();C(GsmCachedTown::Read(root,request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused&&observer.bytes==0,"empty file refused before IO");
 Write(Name(root,id),original);observer=Observe();C(GsmCachedTown::Read("relative/",request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused,"relative root refused");
 C(GsmCachedTown::Read(root+"../",request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused,"noncanonical root refused");
 C(GsmCachedTown::Read(root+":alias",request,result,Stop,&observer,Chunk)==GsmCachedTown::Refused,"alternate stream root refused");
 GsmCachedTown::Request invalid=request;unsigned nan=0x7fc00000;std::memcpy(&invalid.storedXYZ[0],&nan,4);C(GsmCachedTown::Read(root,invalid,result,Stop,&observer,Chunk)==GsmCachedTown::Refused,"nonfinite copied town position refused");
 GsmAtlas::Job job;C(!job.cachedTown&&job.cachedTownResult.state==GsmCachedTown::Empty,"worker constructor mode and result initialized");
 job.game=root;job.generation=7;job.cachedTown=true;job.cachedTownRequest=request;job.manifest.header[0][0]=77;
 C(GsmAtlas::Run(&job)==0&&job.ok&&job.cachedTownResult.state==GsmCachedTown::Ready&&job.read.readBytes==original.size(),"actual worker distinct cached-town dispatch");
 C(job.manifest.header[0][0]==77&&job.read.plane[0].empty()&&job.windowMinHeight==0&&!job.windowHasFiniteHeight,"cached job never performs atlas window or replaces manifest");
 job.cachedTownRequest=missing;C(GsmAtlas::Run(&job)==0&&job.ok&&job.cachedTownResult.state==GsmCachedTown::Unavailable,"actual worker unavailable is nonfatal control completion");
 bad=original;bad[0]^=0xff;Write(Name(root,id),bad);job.cachedTownRequest=request;C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.cachedTownResult.state==GsmCachedTown::Refused&&job.cachedTownResult.bytes.empty()&&job.error[0],"malformed cached worker control remains explicit Refused");Write(Name(root,id),original);
 job.cachedTownRequest=request;job.validation=true;C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.cachedTownResult.bytes.empty()&&std::strstr(job.error,"ambiguous"),"cached and terrain modes exclusive before read");
 job.validation=false;job.waterValidation=true;C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.cachedTownResult.bytes.empty(),"cached and water modes exclusive");
 job.waterValidation=false;job.modelValidation=true;C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.cachedTownResult.bytes.empty(),"cached and model modes exclusive");
 job.modelValidation=false;job.generation=6;C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.read.cancel&&job.cachedTownResult.state==GsmCachedTown::Cancelled&&job.cachedTownResult.bytes.empty()&&job.read.readBytes==0,"stale generation discarded before read");
 job.generation=7;InterlockedExchange(&job.read.cancel,0);C(GsmAtlas::Run(&job)==0&&job.cachedTownResult.state==GsmCachedTown::Ready,"accepted after owned cancellation reset");
 InterlockedExchange(&job.read.cancel,1);C(GsmAtlas::Run(&job)!=0&&!job.ok&&job.cachedTownResult.bytes.empty()&&!AtlasView::Nonzero(job.cachedTownResult.sha256,32),"repeat run cannot retain accepted bytes after cancel");
 C(DeleteFileW(Name(root,id).c_str())!=FALSE,"all worker source handles closed");
 std::printf("PASS %u actual cached-town file/worker checks\n",checks);return 0;
}catch(...){return 1;}}
