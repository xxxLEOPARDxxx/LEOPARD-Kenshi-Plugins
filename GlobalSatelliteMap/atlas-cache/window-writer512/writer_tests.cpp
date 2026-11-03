#define _CRT_SECURE_NO_WARNINGS
#include "WindowWriter512.h"
#include <cstdio>
#include <vector>
using namespace Gst2Windows;
static unsigned checks=0;
static void C(bool ok,const char*what){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",what);throw 1;}}
struct File: Gst2::RandomFile {
    U64 size,writes,flushes,failWrite,failFlush,ledger,lastOffset;unsigned lastBytes;unsigned char header[1024];
    File():size(0),writes(0),flushes(0),failWrite(0),failFlush(0),ledger(0),lastOffset(0),lastBytes(0){std::memset(header,0,sizeof(header));}
    bool Resize(U64 n){size=n;return true;}
    bool WriteAt(U64 at,const void*data,DWORD bytes){
        ++writes;if(failWrite==writes)return false;C(at<=size&&bytes<=size-at,"every production write within file");
        if(at==0&&bytes==1024)std::memcpy(header,data,1024);
        else if(at==48&&bytes==8){const BYTE*p=static_cast<const BYTE*>(data);ledger=0;for(unsigned i=0;i<8;++i)ledger|=U64(p[i])<<(8*i);}
        else {C(bytes==2048||bytes==4096,"one exact512 row");C(static_cast<const BYTE*>(data)[0]==77&&static_cast<const BYTE*>(data)[bytes-1]==77,"borrowed row bytes retained");}
        lastOffset=at;lastBytes=bytes;return true;
    }
    bool Flush(){return ++flushes!=failFlush;}
};
static volatile LONG cancelled=0;
static bool Cancel(void*){return InterlockedCompareExchange(&cancelled,0,0)!=0;}
struct CountingCancel{unsigned calls,at;};
struct OwnedOutput{wchar_t path[160];bool owned;OwnedOutput():owned(false){path[0]=0;}~OwnedOutput(){if(owned)DeleteFileW(path);}};
struct OwnedReadFile{FILE*f;OwnedReadFile():f(NULL){}~OwnedReadFile(){if(f)fclose(f);}};
static bool CancelAfter(void*p){CountingCancel&v=*static_cast<CountingCancel*>(p);return ++v.calls==v.at;}
static bool ThrowAfter(void*p){CountingCancel&v=*static_cast<CountingCancel*>(p);if(++v.calls==v.at)throw 17;return false;}
struct OtherThread{Writer*writer;Frame frame;const BYTE*data;bool result;};
static DWORD WINAPI WriteOtherThread(void*p){OtherThread&v=*static_cast<OtherThread*>(p);v.result=v.writer->WritePlane(0,v.frame,v.data,512*512*8,4096);return 0;}
static DWORD WINAPI CancelOtherThread(void*p){static_cast<Writer*>(p)->Cancel();return 0;}
int main(){try{
    const Stamp stamp={0x100000002ULL,0x200000003ULL,0x300000004ULL};
    File retryFile;Writer retry;Stamp invalid=stamp;invalid.source=0;
    C(!retry.Begin(retryFile,0,0,invalid)&&retry.Error()==ERROR_INVALID_PARAMETER,"invalid Begin status");
    C(retry.Begin(retryFile,0,0,stamp)&&retry.Error()==ERROR_SUCCESS,"corrected Begin resets status after refused arguments");
    File f;Writer writer;C(writer.Begin(f,3,2,stamp),"fresh linear16F header");
    C(f.size==1431307264ULL&&!std::memcmp(f.header,"GST2",4)&&f.header[12]==0,"compatible incomplete linearRGB layout");
    std::vector<BYTE>plane(512*512*8,77);Frame key={stamp,3,2,0,0};U64 before=f.writes;
    C(!writer.WritePlane(3,key,&plane[0],plane.size(),4096)&&f.writes==before,"bad plane refuses before IO");
    key.tileX=0;C(!writer.WritePlane(0,key,&plane[0],plane.size(),4096)&&f.writes==before,"wrong logical tile refuses before IO");key.tileX=3;
    key.x=1;C(!writer.WritePlane(0,key,&plane[0],plane.size(),4096)&&f.writes==before,"alignment refuses before IO");key.x=0;
    C(writer.WritePlane(0,key,&plane[0],plane.size(),4096)&&!writer.Mask(),"first512 cannot mark2048 complete");
    C(f.lastOffset==1024ULL+511*65536ULL&&f.lastBytes==4096,"independent first512 row addressing");
    before=f.writes;C(!writer.WritePlane(0,key,&plane[0],plane.size(),4096)&&f.writes==before,"duplicate refuses before IO");
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){if(!x&&!z)continue;key.x=x*512;key.z=z*512;C(writer.WritePlane(0,key,&plane[0],plane.size(),4096),"remaining subwindows");C(writer.Mask()==(x==3&&z==3?1ULL:0ULL),"coarse bit only after all16 subwindows");}
    C(f.ledger==1&&!writer.BaseComplete(),"only one plane/coarse block persisted");
    // Complete all three planes in reverse tile order; material already has16.
    for(unsigned p=0;p<3;++p)for(int z=15;z>=0;--z)for(int x=15;x>=0;--x){if(p==0&&x<4&&z<4)continue;key.x=x*512;key.z=z*512;const unsigned stride=p?4:8;C(writer.WritePlane(p,key,&plane[0],512*512*stride,512*stride),"all bounded windows in arbitrary order");}
    C(writer.BaseComplete()&&writer.Mask()==0xffffffffffffULL&&f.ledger==writer.Mask(),"all48 coarse-plane flags, never ready status");C(f.header[12]==0,"writer never publishes COMPLETE");
    writer.Cancel();C(!writer.BaseComplete()&&writer.Poisoned(),"late cancellation prevents final publication even with all data");
    File cancelledFile;Writer stopped;C(stopped.Begin(cancelledFile,0,0,stamp),"cancel setup");key.tileX=key.tileZ=key.x=key.z=0;CountingCancel stop={0,7};before=cancelledFile.writes;
    C(!stopped.WritePlane(0,key,&plane[0],plane.size(),4096,&CancelAfter,&stop)&&stopped.Poisoned()&&stopped.Error()==ERROR_CANCELLED&&!stopped.Mask(),"mid-row cancellation poisons without completion");C(cancelledFile.writes-before==5,"cancel checked before each row");
    File staleFile;Writer stale;C(stale.Begin(staleFile,0,0,stamp),"stale setup");before=staleFile.writes;key.stamp.source^=1ULL<<32;C(!stale.WritePlane(0,key,&plane[0],plane.size(),4096)&&stale.Poisoned()&&staleFile.writes==before,"full64 source change permanently stops before IO");key.stamp=stamp;
    for(unsigned field=0;field<2;++field){File changed;Writer w;C(w.Begin(changed,0,0,stamp),"other epoch setup");if(field)key.stamp.capture^=1ULL<<32;else key.stamp.material^=1ULL<<32;U64 calls=changed.writes;C(!w.WritePlane(0,key,&plane[0],plane.size(),4096)&&w.Poisoned()&&changed.writes==calls,"full64 material/capture changes refuse before IO");key.stamp=stamp;}
    File unwindFile;Writer unwind;C(unwind.Begin(unwindFile,0,0,stamp),"unwind setup");CountingCancel throwing={0,7};bool escaped=false,result=true;try{result=unwind.WritePlane(0,key,&plane[0],plane.size(),4096,&ThrowAfter,&throwing);}catch(...){escaped=true;}
    C(!escaped&&!result&&unwind.Poisoned()&&!unwind.Mask(),"callback exception cannot leave reusable partial writer");
    File threadFile;Writer threadWriter;C(threadWriter.Begin(threadFile,0,0,stamp),"thread setup");before=threadFile.writes;OtherThread other={&threadWriter,key,&plane[0],true};HANDLE thread=CreateThread(NULL,0,WriteOtherThread,&other,0,NULL);C(thread&&WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"owned wrong-thread test joins");CloseHandle(thread);C(!other.result&&!threadWriter.Poisoned()&&threadFile.writes==before,"foreign worker refuses without touching owner or shared seek IO");
    thread=CreateThread(NULL,0,CancelOtherThread,&threadWriter,0,NULL);C(thread&&WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"owned cancelling thread joins");CloseHandle(thread);C(!threadWriter.WritePlane(0,key,&plane[0],plane.size(),4096)&&threadWriter.Error()==ERROR_CANCELLED&&threadFile.writes==before,"atomic cross-thread cancellation stops before IO");
    for(unsigned mode=0;mode<4;++mode){File fault;Writer w;C(w.Begin(fault,0,0,stamp),"fault setup");if(mode==0)fault.failWrite=fault.writes+3;if(mode==1)fault.failFlush=fault.flushes+1;
        if(mode>=2){for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){if(x==3&&z==3)continue;key.x=x*512;key.z=z*512;C(w.WritePlane(0,key,&plane[0],plane.size(),4096),"fault coarse setup");}key.x=key.z=1536;if(mode==2)fault.failWrite=fault.writes+513;else fault.failFlush=fault.flushes+2;}
        C(!w.WritePlane(0,key,&plane[0],plane.size(),4096)&&w.Poisoned()&&!w.BaseComplete()&&!w.Mask(),"row/content-flush/ledger/ledger-flush failure terminal");
        U64 calls=fault.writes;C(!w.WritePlane(0,key,&plane[0],plane.size(),4096)&&calls==fault.writes,"poisoned writer cannot retry");key.x=key.z=0;
    }
    // Actual Win32 full-length file, independent seek/read of its last window.
    // Fresh unique path and CREATE_NEW; never delete a prior run's file to start.
    OwnedOutput owned;Gst2::Win32File disk;std::swprintf(owned.path,160,L"_generated/one-tile-%u-%u.gst.partial",GetCurrentProcessId(),GetTickCount());C(disk.OpenNew(owned.path),"test-owned exclusive file");owned.owned=true;Writer actual;C(actual.Begin(disk,1,1,stamp),"actual full-length header");key.tileX=key.tileZ=1;key.x=7168;key.z=7680;std::vector<float>heights(512*512);for(unsigned z=0;z<512;++z)for(unsigned x=0;x<512;++x)heights[z*512+x]=float(z)+(float(x)+.5f)/1024;C(actual.WritePlane(2,key,&heights[0],512*512*4,2048),"actual non-square-position height window");C(disk.Close(),"flush/close actual file");
    OwnedReadFile read;C(_wfopen_s(&read.f,owned.path,L"rb")==0,"actual independent reader");unsigned char bytes[1024];C(fread(bytes,1,1024,read.f)==1024&&bytes[12]==0,"actual incomplete status");
    C(_fseeki64(read.f,805307392ULL+(8191ULL*8192+7168)*4,SEEK_SET)==0&&fread(bytes,1,1024,read.f)==1024,"independent non-square-position final row seek");float first,last;std::memcpy(&first,bytes,4);std::memcpy(&last,bytes+1020,4);C(first==511.f+.5f/1024&&last==511.f+255.5f/1024,"row order/offset/stride preserve literal distinct pixels");fclose(read.f);read.f=NULL;C(DeleteFileW(owned.path)!=FALSE,"remove only this run's fresh verified test output");owned.owned=false;
    std::printf("PASS %u production512 writer checks\n",checks);return 0;
}catch(...){return 1;}}
