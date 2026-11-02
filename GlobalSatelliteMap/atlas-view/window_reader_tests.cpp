#ifndef WINDOW_READER_HEADER
#define WINDOW_READER_HEADER "AtlasWindowReader.h"
#endif
#include WINDOW_READER_HEADER
#include "../atlas-cache/Gst2Writer.h"
#include <winioctl.h>
#include <cstdio>
using namespace AtlasView;
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void Require(bool b,const char*s){if(!b){std::fprintf(stderr,"SETUP FAIL %s\n",s);throw 1;}}
struct Header:Gst2::RandomFile {
    unsigned char bytes[1024];U64 length;
    Header():length(0){std::memset(bytes,0,1024);}
    bool Resize(U64 n){length=n;return true;}bool Flush(){return true;}
    bool WriteAt(U64 at,const void*p,DWORD n){if(at||n!=1024)return false;std::memcpy(bytes,p,n);return true;}
};
static void Write(HANDLE f,U64 at,const void*p,DWORD n){LARGE_INTEGER pos;pos.QuadPart=at;DWORD done=0;Require(SetFilePointerEx(f,pos,NULL,FILE_BEGIN)&&WriteFile(f,p,n,&done,NULL)&&done==n,"fixture write");}
static void Resize(HANDLE f,U64 n){LARGE_INTEGER pos;pos.QuadPart=n;Require(SetFilePointerEx(f,pos,NULL,FILE_BEGIN)&&SetEndOfFile(f),"fixture resize");}
static HANDLE OpenWrite(const std::wstring&path,bool create){HANDLE f=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,create?CREATE_ALWAYS:OPEN_EXISTING,0,NULL);Require(f!=INVALID_HANDLE_VALUE,"fixture open");return f;}
struct Sample {
    WindowKey key;std::vector<unsigned char>data[3];unsigned char hash[3][32];
    void Fill(const WindowKey&k){key=k;for(unsigned p=0;p<3;++p){data[p].resize(256*256*Stride(p));for(unsigned z=0;z<256;++z)for(unsigned x=0;x<256;++x)for(unsigned b=0;b<Stride(p);++b)data[p][(z*256+x)*Stride(p)+b]=static_cast<unsigned char>(k.tile*19+k.level*31+k.x*29+k.z*37+x*3+z*7+p*43+b*13);Require(Digest(&data[p][0],static_cast<DWORD>(data[p].size()),hash[p]),"sample SHA");}}
    void Save(HANDLE f)const{for(unsigned p=0;p<3;++p)for(unsigned z=0;z<256;++z){U64 at=PlaneOffset(key.level,p)+((U64(key.z)*256+z)*Side(key.level)+U64(key.x)*256)*Stride(p);Write(f,at,&data[p][z*256*Stride(p)],256*Stride(p));}}
};
static void IndexFile(const std::wstring&path,const std::vector<unsigned char>&raw,unsigned char sha[32]){HANDLE f=OpenWrite(path,true);Write(f,0,&raw[0],static_cast<DWORD>(raw.size()));Require(CloseHandle(f)!=FALSE,"index close");Require(Digest(&raw[0],static_cast<DWORD>(raw.size()),sha),"index SHA");}
struct Observe {ReadRequest*r;std::wstring path;bool denied;Observe(ReadRequest*p):r(p),denied(false){}};
static void CancelPayload(void*p){ReadRequest&r=*static_cast<ReadRequest*>(p);if(r.readBytes>1024)InterlockedExchange(&r.cancel,1);}
static void DeniedWrite(void*p){Observe&o=*static_cast<Observe*>(p);HANDLE f=CreateFileW(o.path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);o.denied=f==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_SHARING_VIOLATION;if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);}
static std::wstring Absolute(const wchar_t*s){wchar_t path[MAX_PATH]={0};DWORD n=GetFullPathNameW(s,MAX_PATH,path,NULL);Require(n&&n<MAX_PATH,"absolute fixture path");return path;}
static bool Junction(const std::wstring&link,const std::wstring&target){
    if(!CreateDirectoryW(link.c_str(),NULL))return false;
    HANDLE h=CreateFileW(link.c_str(),GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if(h==INVALID_HANDLE_VALUE){RemoveDirectoryW(link.c_str());return false;}
    std::wstring sub=L"\\??\\"+target;unsigned subBytes=static_cast<unsigned>(sub.size()*2),printBytes=static_cast<unsigned>(target.size()*2);
    std::vector<unsigned char>b(16+subBytes+2+printBytes+2,0);DWORD tag=0xa0000003;std::memcpy(&b[0],&tag,4);unsigned short size=static_cast<unsigned short>(b.size()-8),subLength=static_cast<unsigned short>(subBytes),printAt=static_cast<unsigned short>(subBytes+2),printLength=static_cast<unsigned short>(printBytes);std::memcpy(&b[4],&size,2);std::memcpy(&b[10],&subLength,2);std::memcpy(&b[12],&printAt,2);std::memcpy(&b[14],&printLength,2);std::memcpy(&b[16],sub.data(),subBytes);std::memcpy(&b[16+subBytes+2],target.data(),printBytes);DWORD done=0;bool ok=DeviceIoControl(h,FSCTL_SET_REPARSE_POINT,&b[0],static_cast<DWORD>(b.size()),NULL,0,&done,NULL)!=FALSE;CloseHandle(h);if(!ok)RemoveDirectoryW(link.c_str());return ok;
}
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;std::wstring root=Absolute(argv[1]),gst=root+L"\\fixture.gst",idx=root+L"\\windows.gsi",alias=root+L"\\junction";bool junction=false;
try{
    Header h;Gst2::Writer writer;Require(writer.Begin(h,3,2),"production GST header");Gst2::Put32(h.bytes+12,1);Gst2::Put64(h.bytes+48,0xffffffffffffULL);for(unsigned i=0;i<18;++i)std::memset(h.bytes+128+i*48+16,0x55,32);
    unsigned char expectedHeader[32];Require(Digest(h.bytes,1024,expectedHeader),"header hash");HANDLE f=OpenWrite(gst,true);DWORD done=0;Require(DeviceIoControl(f,FSCTL_SET_SPARSE,NULL,0,NULL,0,&done,NULL)!=FALSE,"sparse production-sized GST");Resize(f,FileBytes());Write(f,0,h.bytes,1024);
    const WindowKey keys[8]={WindowKey(11,0,0,0),WindowKey(11,0,31,31),WindowKey(11,1,15,15),WindowKey(11,2,7,3),WindowKey(11,3,3,3),WindowKey(11,4,1,1),WindowKey(11,5,0,0),WindowKey(11,0,17,9)};
    Sample samples[8];for(unsigned i=0;i<8;++i){samples[i].Fill(keys[i]);samples[i].Save(f);}Require(CloseHandle(f)!=FALSE,"GST close");
    unsigned char capture[32],headers[16][32],indexSha[32];std::memset(capture,0x33,32);std::memset(headers,0x44,sizeof(headers));std::memcpy(headers[11],expectedHeader,32);
    std::vector<unsigned char>raw(WindowIndexFileBytes,0x55);std::memset(&raw[0],0,1024);std::memcpy(&raw[0],"GSI1",4);Gst2::Put32(&raw[4],1);Gst2::Put32(&raw[8],1024);Gst2::Put32(&raw[12],256);Gst2::Put32(&raw[16],4);Gst2::Put32(&raw[20],8192);Gst2::Put32(&raw[24],6);Gst2::Put32(&raw[28],3);Gst2::Put32(&raw[32],21840);std::memcpy(&raw[64],capture,32);std::memcpy(&raw[96],headers,512);for(unsigned i=0;i<8;++i)std::memcpy(&raw[1024+WindowRecord(keys[i])*96],samples[i].hash,96);IndexFile(idx,raw,indexSha);
    ReadRequest control;WindowIndex index;Check(LoadWindowIndex(idx,capture,headers,indexSha,control,index),"load complete pinned GSI1");Check(control.readBytes==WindowIndexFileBytes&&index.records.size()==WindowIndexBodyBytes,"bounded exact index bytes");unsigned char hashes[3][32];Check(!index.Hashes(WindowKey(),hashes),"invalid key lookup refused");
    ReadRequest r;r.path=gst;std::memcpy(r.expectedHeader,expectedHeader,32);
    for(unsigned i=0;i<8;++i){Check(index.Hashes(keys[i],hashes)&&!std::memcmp(hashes,samples[i].hash,96),"record tile-level-row-column mapping");r.level=0;r.tileX=r.tileZ=99;Check(ReadWindow(r,keys[i],hashes),"read actual source window");Check(r.level==5&&r.tileX==3&&r.tileZ==2,"256 upload shape separate from source key");Check(r.readBytes==1024+1048576,"oneMiB payload independent of full source level");for(unsigned p=0;p<3;++p)Check(r.plane[p]==samples[i].data[p],"actual window rows/plane bytes exact");}
    Check(!ReadWindow(r,WindowKey(11,0,32,0),hashes)&&r.error==ERROR_INVALID_PARAMETER&&r.readBytes==0&&r.plane[0].empty(),"out-of-range window refused before IO");
    index.Hashes(keys[0],hashes);r.expectedHeader[0]^=1;Check(!ReadWindow(r,keys[0],hashes)&&r.error==ERROR_INVALID_DATA&&r.plane[0].empty(),"GST header pin mismatch");r.expectedHeader[0]^=1;
    InterlockedExchange(&r.cancel,1);Check(!ReadWindow(r,keys[0],hashes)&&r.error==ERROR_CANCELLED&&r.readBytes==0,"window pre-cancel");InterlockedExchange(&r.cancel,0);r.afterChunk=CancelPayload;r.observer=&r;Check(!ReadWindow(r,keys[0],hashes)&&r.error==ERROR_CANCELLED&&r.readBytes==1024+2048&&r.plane[0].empty(),"window row cancellation frees every plane");r.afterChunk=NULL;InterlockedExchange(&r.cancel,0);
    Observe write(&r);write.path=gst;r.afterChunk=DeniedWrite;r.observer=&write;Check(ReadWindow(r,keys[0],hashes)&&write.denied,"source writer denied throughout window snapshot");r.afterChunk=NULL;
    std::wstring validPath=r.path;r.path=gst+std::wstring(1,L'\0')+L"suffix";Check(!ReadWindow(r,keys[0],hashes)&&r.error==ERROR_INVALID_NAME,"embedded NUL source path refused");r.path=validPath;r.path+=L":ads";Check(!ReadWindow(r,keys[0],hashes),"source ADS refused");r.path=validPath;
    junction=Junction(alias,root);Require(junction,"real junction fixture creation");r.path=alias+L"\\fixture.gst";Check(!ReadWindow(r,keys[0],hashes)&&r.error==ERROR_ACCESS_DENIED,"reparse ancestor refused for GST");r.path=validPath;Check(!LoadWindowIndex(alias+L"\\windows.gsi",capture,headers,indexSha,control,index)&&control.error==ERROR_ACCESS_DENIED,"reparse ancestor refused for index");
    control.afterChunk=CancelPayload;control.observer=&control;Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index)&&control.error==ERROR_CANCELLED&&control.readBytes==1024+65536&&index.records.empty(),"index64KiB cancellation frees index");control.afterChunk=NULL;InterlockedExchange(&control.cancel,0);
    const unsigned mutations[]={0,4,8,12,16,20,24,28,32,36,63,64,95,96,607,608,1023};
    for(unsigned i=0;i<sizeof(mutations)/sizeof(mutations[0]);++i){raw[mutations[i]]^=1;IndexFile(idx,raw,indexSha);Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index)&&index.records.empty(),"structural/pin mismatch refused even with recomputed index SHA");raw[mutations[i]]^=1;}
    IndexFile(idx,raw,indexSha);indexSha[0]^=1;Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index)&&control.error==ERROR_CRC&&index.records.empty(),"full index SHA checked");indexSha[0]^=1;
    std::vector<unsigned char>zero=raw;std::memset(&zero[1024],0,32);IndexFile(idx,zero,indexSha);Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index)&&control.error==ERROR_INVALID_DATA,"zero digest record refused");IndexFile(idx,raw,indexSha);
    f=OpenWrite(idx,false);Resize(f,WindowIndexFileBytes-1);CloseHandle(f);Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index),"truncated index refused");IndexFile(idx,raw,indexSha);f=OpenWrite(idx,false);Resize(f,WindowIndexFileBytes+1);CloseHandle(f);Check(!LoadWindowIndex(idx,capture,headers,indexSha,control,index),"extra index bytes refused");IndexFile(idx,raw,indexSha);
    Check(LoadWindowIndex(idx,capture,headers,indexSha,control,index)&&index.Hashes(keys[7],hashes),"reload valid index");f=OpenWrite(gst,false);unsigned char corrupt=samples[7].data[2][19]^1;Write(f,PlaneOffset(0,2)+(U64(9)*256*8192+17*256)*4+19,&corrupt,1);CloseHandle(f);Check(!ReadWindow(r,keys[7],hashes)&&r.error==ERROR_CRC&&r.plane[0].empty()&&r.plane[1].empty()&&r.plane[2].empty(),"window checksum mismatch discards all planes");
    RemoveDirectoryW(alias.c_str());junction=false;DeleteFileW(gst.c_str());DeleteFileW(idx.c_str());std::printf("GST2 window/index reader PASS %u checks; production-shaped sparse source,8windows/all6levels,real junction/cancel/sharing\n",checks);return 0;
}catch(...){if(junction)RemoveDirectoryW(alias.c_str());DeleteFileW(gst.c_str());DeleteFileW(idx.c_str());return 1;}}
