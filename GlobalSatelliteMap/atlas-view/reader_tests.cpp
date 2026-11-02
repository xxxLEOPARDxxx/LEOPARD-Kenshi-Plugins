#ifndef ATLAS_READER_HEADER
#define ATLAS_READER_HEADER "AtlasReader.h"
#endif
#include ATLAS_READER_HEADER
#include "../atlas-cache/Gst2Writer.h"
#include <winioctl.h>
#include <cstdio>
using namespace AtlasView;
static unsigned checks=0;
static void Check(bool ok,const char*what){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",what);throw 1;}}
struct HeaderFile:Gst2::RandomFile {
    unsigned char h[1024];U64 length;
    HeaderFile():length(0){std::memset(h,0,1024);}
    bool Resize(U64 n){length=n;return true;}bool Flush(){return true;}
    bool WriteAt(U64 at,const void*p,DWORD n){if(at||n!=1024)return false;std::memcpy(h,p,n);return true;}
};
static void Write(HANDLE f,U64 at,const void*p,DWORD n){LARGE_INTEGER l;l.QuadPart=at;DWORD done;Check(SetFilePointerEx(f,l,NULL,FILE_BEGIN)&&WriteFile(f,p,n,&done,NULL)&&done==n,"write fixture");}
static void CancelAfter(void*p){ReadRequest&r=*static_cast<ReadRequest*>(p);if(r.readBytes>1024)InterlockedExchange(&r.cancel,1);}
struct SharingCheck{ReadRequest*r;bool denied;SharingCheck(ReadRequest*p):r(p),denied(false){}};
static void TryWrite(void*p){SharingCheck&s=*static_cast<SharingCheck*>(p);HANDLE f=CreateFileW(s.r->path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);s.denied=f==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_SHARING_VIOLATION;if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);}
int wmain(int argc,wchar_t**argv){
    if(argc!=2)return 2;
    const wchar_t*path=argv[1];
    try{
        HeaderFile fixture;Gst2::Writer w;Check(w.Begin(fixture,2,3),"production writer header");
        Check(fixture.length==FileBytes(),"reader writer byte layout");
        Gst2::Put32(fixture.h+12,1);Gst2::Put64(fixture.h+48,0xffffffffffffULL);
        for(unsigned i=0;i<18;++i)std::memset(fixture.h+128+i*48+16,0x55,32);
        HANDLE f=CreateFileW(path,GENERIC_WRITE|GENERIC_READ,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);Check(f!=INVALID_HANDLE_VALUE,"create fixture");
        DWORD done;Check(DeviceIoControl(f,FSCTL_SET_SPARSE,NULL,0,NULL,0,&done,NULL)!=FALSE,"sparse fixture");
        LARGE_INTEGER len;len.QuadPart=fixture.length;Check(SetFilePointerEx(f,len,NULL,FILE_BEGIN)&&SetEndOfFile(f),"sparse exact size");
        std::vector<unsigned char> expected[3];
        for(unsigned p=0;p<3;++p){expected[p].resize(static_cast<size_t>(PlaneBytes(5,p)));for(size_t i=0;i<expected[p].size();++i)expected[p][i]=static_cast<unsigned char>(i*13+p*7);Check(Digest(&expected[p][0],static_cast<DWORD>(expected[p].size()),fixture.h+128+(15+p)*48+16),"plane hash");Write(f,PlaneOffset(5,p),&expected[p][0],static_cast<DWORD>(expected[p].size()));}
        Write(f,0,fixture.h,1024);CloseHandle(f);
        ReadRequest r;r.path=path;r.tileX=2;r.tileZ=3;r.level=5;Check(Digest(fixture.h,1024,r.expectedHeader),"pinned manifest hash");
        Check(ReadLevel(r),"valid selected level");Check(r.readBytes==1024+PlaneBytes(5,0)+PlaneBytes(5,1)+PlaneBytes(5,2),"bounded selected bytes only");
        for(unsigned p=0;p<3;++p)Check(r.plane[p]==expected[p],"plane bytes preserved");
        r.expectedHeader[0]^=1;Check(!ReadLevel(r)&&r.error==ERROR_INVALID_DATA&&r.plane[0].empty(),"pinned header mismatch clears payload");r.expectedHeader[0]^=1;
        r.level=0;Check(!ReadLevel(r)&&r.error==ERROR_INVALID_PARAMETER&&r.readBytes==0,"oversize mip refused before IO");r.level=5;
        InterlockedExchange(&r.cancel,1);Check(!ReadLevel(r)&&r.error==ERROR_CANCELLED&&r.readBytes==0,"pre-cancel");InterlockedExchange(&r.cancel,0);
        r.afterChunk=CancelAfter;r.observer=&r;Check(!ReadLevel(r)&&r.error==ERROR_CANCELLED&&r.readBytes==1024+65536&&r.plane[0].empty(),"actual chunk cancel frees partial data");InterlockedExchange(&r.cancel,0);
        SharingCheck sharing(&r);r.afterChunk=TryWrite;r.observer=&sharing;Check(ReadLevel(r)&&sharing.denied,"write sharing refused throughout snapshot");r.afterChunk=NULL;
        HANDLE thread=CreateThread(NULL,0,ReadWorker,&r,0,NULL);Check(thread&&WaitForSingleObject(thread,10000)==WAIT_OBJECT_0,"real worker completes");DWORD code=1;Check(GetExitCodeThread(thread,&code)&&code==0,"worker success status");CloseHandle(thread);
        f=CreateFileW(path,GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);unsigned char corrupt=expected[1][3]^1;Write(f,PlaneOffset(5,1)+3,&corrupt,1);CloseHandle(f);
        Check(!ReadLevel(r)&&r.error==ERROR_CRC&&r.plane[0].empty()&&r.plane[1].empty(),"payload corruption rejected all planes");
        Check(ValidHeader(fixture.h,fixture.length,2,3),"valid header shape");
        const unsigned offsets[]={4,8,12,16,20,24,28,32,36,40,44,48,54,56,60,64,68,127,128,136,992,1023};
        for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i){fixture.h[offsets[i]]^=1;Check(!ValidHeader(fixture.h,fixture.length,2,3),"mutated header refused");fixture.h[offsets[i]]^=1;}
        Check(!ValidHeader(fixture.h,fixture.length-1,2,3)&&!ValidHeader(fixture.h,fixture.length+1,2,3),"exact file size");
        Gst2::Put32(fixture.h+12,2);Check(!ValidHeader(fixture.h,fixture.length,2,3),"raw packed is not ready");Gst2::Put32(fixture.h+12,6);Check(!ValidHeader(fixture.h,fixture.length,2,3),"interrupted decoding is not ready");
        DeleteFileW(path);std::printf("Atlas reader PASS %u checks\n",checks);return 0;
    }catch(...){DeleteFileW(path);return 1;}
}
