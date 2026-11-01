#include "../../SavePreviewCodec.h"
#include <process.h>
#include <cstdio>
#include <fstream>
#include <iterator>
// EXTRACT_WRITER
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
static bool File(const std::string& path,const char* value)
{
    HANDLE f=CreateFileW(SavePreviewCodec::Wide(path).c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(f==INVALID_HANDLE_VALUE)return false;DWORD n=0;bool ok=WriteFile(f,value,(DWORD)strlen(value),&n,NULL)&&n==strlen(value);CloseHandle(f);return ok;
}
static std::vector<unsigned char> Bytes(const std::string& path)
{std::ifstream stream(path.c_str(),std::ios::binary);return std::vector<unsigned char>((std::istreambuf_iterator<char>(stream)),std::istreambuf_iterator<char>());}
static void Setup(Writer& job,const std::string& path)
{
    job.path=path;memcpy(job.header.magic,"UITPREV1",8);job.header.version=1;job.header.width=32;job.header.height=16;
    SavePreviewCodec::Stamp(path,job.header);job.pixels.resize(32*16*3,70);
}
static unsigned __stdcall WaitCancel(void* value)
{Writer* job=static_cast<Writer*>(value);while(!InterlockedCompareExchange(&job->cancelled,0,0))Sleep(1);return 0;}
static unsigned __stdcall WaitEvent(void* value){WaitForSingleObject((HANDLE)value,10000);return 0;}
static unsigned __stdcall ProbeGate(void* value)
{Writer* job=static_cast<Writer*>(value);if(!TryEnterCriticalSection(&job->gate))return 0;LeaveCriticalSection(&job->gate);return 1;}
int main(int argc,char**argv)
{
    CHECK(argc==3);const std::string path=argv[1];CreateDirectoryW(SavePreviewCodec::Wide(path).c_str(),NULL);
    CHECK(File(path+"/quick.save","production-boundary-fixture-one"));
    Writer success;Setup(success,path);
    HANDLE handle=(HANDLE)_beginthreadex(NULL,0,WriteThread,&success,0,NULL);CHECK(handle!=NULL);
    CHECK(WaitForSingleObject(handle,10000)==WAIT_OBJECT_0);CloseHandle(handle);CHECK(success.success&&success.error.empty());
    SavePreviewCodec::Header h={};std::vector<unsigned char> rgba;std::string error;
    CHECK(SavePreviewCodec::Read(path,h,rgba,&error)&&error.empty());CHECK(rgba.size()==32*16*4);
    CHECK(GetFileAttributesW(SavePreviewCodec::Wide(path+"/uitweaks-preview.tmp").c_str())==INVALID_FILE_ATTRIBUTES);
    std::vector<unsigned char> oldJpeg=Bytes(argv[2]);CHECK(!oldJpeg.empty());
    SavePreviewCodec::Header legacy=success.header;legacy.encodedBytes=(DWORD)oldJpeg.size();
    HANDLE legacyFile=CreateFileW(SavePreviewCodec::Wide(path+"/uitweaks-preview.bin").c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);CHECK(legacyFile!=INVALID_HANDLE_VALUE);
    DWORD legacyCount=0;CHECK(WriteFile(legacyFile,&legacy,sizeof(legacy),&legacyCount,NULL)&&legacyCount==sizeof(legacy));
    CHECK(WriteFile(legacyFile,&oldJpeg[0],(DWORD)oldJpeg.size(),&legacyCount,NULL)&&legacyCount==oldJpeg.size());CloseHandle(legacyFile);
    CHECK(SavePreviewCodec::Read(path,h,rgba)&&rgba.size()==32*16*4); // existing version-1 JPEG sidecar remains supported
    const std::vector<unsigned char> beforeFailure=Bytes(path+"/uitweaks-preview.bin");
    Writer invalid;Setup(invalid,path);invalid.header.width=1601;CHECK(WriteThread(&invalid)==0&&!invalid.success&&!invalid.error.empty());
    CHECK(Bytes(path+"/uitweaks-preview.bin")==beforeFailure); // encode failure cannot replace a good preview
    Writer cancelled;Setup(cancelled,path);cancelled.cancelled=1;CHECK(WriteThread(&cancelled)==0&&!cancelled.success);
    CHECK(SavePreviewCodec::Read(path,h,rgba));
    Writer stale;Setup(stale,path);CHECK(File(path+"/quick.save","different-save-revision-with-a-different-size"));
    CHECK(WriteThread(&stale)==0&&!stale.success);error.clear();CHECK(!SavePreviewCodec::Read(path,h,rgba,&error)&&error.empty());
    Writer racing;Setup(racing,path);EnterCriticalSection(&racing.gate);
    handle=(HANDLE)_beginthreadex(NULL,0,WriteThread,&racing,0,NULL);CHECK(handle!=NULL);
    for(unsigned i=0;i<200&&GetFileAttributesW(SavePreviewCodec::Wide(path+"/uitweaks-preview.tmp").c_str())==INVALID_FILE_ATTRIBUTES;++i)Sleep(5);
    InterlockedExchange(&racing.cancelled,1);LeaveCriticalSection(&racing.gate);
    CHECK(WaitForSingleObject(handle,10000)==WAIT_OBJECT_0);CloseHandle(handle);CHECK(!racing.success&&racing.error.empty());
    CHECK(GetFileAttributesW(SavePreviewCodec::Wide(path+"/uitweaks-preview.tmp").c_str())==INVALID_FILE_ATTRIBUTES);
    Writer missing;Setup(missing,path+"/missing-parent");CHECK(WriteThread(&missing)==0&&!missing.success&&!missing.error.empty());
    Writer stopped;stopped.thread=(HANDLE)_beginthreadex(NULL,0,WaitCancel,&stopped,0,NULL);CHECK(stopped.thread!=NULL);
    CHECK(StopWriter(stopped,1000)&&stopped.thread==NULL&&stopped.cancelled);
    Writer slow;HANDLE event=CreateEventW(NULL,TRUE,FALSE,NULL);CHECK(event!=NULL);
    slow.thread=(HANDLE)_beginthreadex(NULL,0,WaitEvent,event,0,NULL);CHECK(slow.thread!=NULL);
    CHECK(!StopWriter(slow,0)&&slow.thread!=NULL&&slow.cancelled); // bounded quarantine keeps job/handle alive
    SetEvent(event);CHECK(StopWriter(slow,1000)&&slow.thread==NULL);CloseHandle(event);
    Writer unwind;try{Writer::Gate guard(unwind);throw 7;}catch(int){}
    handle=(HANDLE)_beginthreadex(NULL,0,ProbeGate,&unwind,0,NULL);CHECK(handle!=NULL);
    CHECK(WaitForSingleObject(handle,1000)==WAIT_OBJECT_0);DWORD exitCode=0;GetExitCodeThread(handle,&exitCode);CloseHandle(handle);CHECK(exitCode==1);
    std::printf("PASS actual save preview writer: %u checks\n",checks);return 0;
}
