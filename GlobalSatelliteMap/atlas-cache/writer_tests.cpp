#include <cstdio>
#include <cstring>
#include <vector>
#include "Gst2Writer.h"
#pragma warning(disable:4127) // test CHECK macro uses the standard do/while(0) form.

static int failures=0,checks=0;
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
struct MemoryIO : Gst2::RandomFile {
    unsigned char header[1024]; unsigned __int64 size,writes,flushes,lastOffset;
    unsigned failWrite; bool failFlush,failResize;
    MemoryIO():size(0),writes(0),flushes(0),lastOffset(0),failWrite(0),failFlush(false),failResize(false){std::memset(header,0,sizeof(header));}
    bool Resize(unsigned __int64 n){size=n;return !failResize;}
    bool WriteAt(unsigned __int64 off,const void *p,DWORD n){++writes;lastOffset=off;if(failWrite&&writes==failWrite)return false;if(off<1024){if(off+n>1024)return false;std::memcpy(header+size_t(off),p,n);}return off<=size&&n<=size-off;}
    bool Flush(){++flushes;return !failFlush;}
};
static unsigned u32(const unsigned char*p){return unsigned(p[0])|(unsigned(p[1])<<8)|(unsigned(p[2])<<16)|(unsigned(p[3])<<24);}
static unsigned __int64 u64(const unsigned char*p){return u32(p)|(static_cast<unsigned __int64>(u32(p+4))<<32);}
int wmain(int argc,wchar_t **argv){
    MemoryIO io;Gst2::Writer writer;std::vector<unsigned char> block(Gst2::BlockBytes(0),0x7b);
    CHECK(writer.Begin(io,3,2));CHECK(io.size==1431307264ULL);CHECK(std::memcmp(io.header,"GST2",4)==0);
    CHECK(u32(io.header+4)==2&&u32(io.header+8)==1024&&u32(io.header+12)==0);
    CHECK(u32(io.header+16)==3&&u32(io.header+20)==2&&u32(io.header+24)==4);
    CHECK(u32(io.header+28)==8192&&u32(io.header+32)==6&&u32(io.header+36)==3&&u32(io.header+40)==2048);
    CHECK(u32(io.header+56)==10&&u32(io.header+60)==28&&u32(io.header+64)==41);
    CHECK(u64(io.header+48)==0);CHECK(u64(io.header+128)==1024);CHECK(u64(io.header+136)==536870912ULL);
    CHECK(u64(io.header+128+48)==536871936ULL);CHECK(u64(io.header+128+18*48-40)==262144);
    const unsigned __int64 before=io.writes;
    CHECK(!writer.WriteBlock(0,1,0,&block[0],block.size(),16384));CHECK(!writer.WriteBlock(3,0,0,&block[0],block.size(),16384));
    CHECK(!writer.WriteBlock(0,8192,0,&block[0],block.size(),16384));CHECK(!writer.WriteBlock(0,0,0,&block[0],block.size()-1,16384));
    CHECK(!writer.WriteBlock(0,0,0,&block[0],block.size(),0xffffffffU));CHECK(io.writes==before);
    CHECK(writer.WriteBlock(0,0,0,&block[0],block.size(),16384));CHECK(writer.Mask()==1&&u64(io.header+48)==1);
    CHECK(io.lastOffset==48);CHECK(!writer.WriteBlock(0,0,0,&block[0],block.size(),16384));CHECK(!writer.BaseComplete());
    for(unsigned p=0;p<3;++p)for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){if(p==0&&z==0&&x==0)continue;CHECK(writer.WriteBlock(p,x*2048,z*2048,&block[0],Gst2::BlockBytes(p),Gst2::BlockPitch(p)));}
    CHECK(writer.BaseComplete()&&writer.Mask()==0xffffffffffffULL);CHECK(u64(io.header+48)==0xffffffffffffULL);CHECK(u32(io.header+12)==0);
    MemoryIO partial;Gst2::Writer broken;CHECK(broken.Begin(partial,0,0));partial.failWrite=unsigned(partial.writes+3);
    CHECK(!broken.WriteBlock(0,0,0,&block[0],block.size(),16384));CHECK(broken.Poisoned());CHECK(broken.Mask()==0&&u64(partial.header+48)==0);
    CHECK(!broken.WriteBlock(0,0,0,&block[0],block.size(),16384));
    MemoryIO disk;Gst2::Writer unflushed;CHECK(unflushed.Begin(disk,0,0));disk.failFlush=true;
    CHECK(!unflushed.WriteBlock(0,0,0,&block[0],block.size(),16384));CHECK(unflushed.Poisoned()&&u64(disk.header+48)==0);
    MemoryIO bad;Gst2::Writer no;CHECK(!no.Begin(bad,4,0));CHECK(bad.writes==0);bad.failResize=true;CHECK(!no.Begin(bad,0,0));
    MemoryIO raw;Gst2::Writer packed;CHECK(packed.Begin(raw,0,0,true));CHECK(u32(raw.header+12)==2);CHECK(!packed.BaseComplete());
    // Actual Win32 backend smoke uses a fresh tiny file supplied inside _generated.
    CHECK(argc==2);
    if(argc==2){
        Gst2::Win32File file;CHECK(file.OpenNew(argv[1]));CHECK(file.Resize(128));
        const unsigned char sample[8]={0,1,2,3,252,253,254,255};CHECK(file.WriteAt(64,sample,8));CHECK(file.Flush());
        Gst2::Win32File collision;CHECK(!collision.OpenNew(argv[1]));CHECK(file.Close());CHECK(!collision.OpenNew(argv[1]));
        HANDLE read=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        CHECK(read!=INVALID_HANDLE_VALUE);
        if(read!=INVALID_HANDLE_VALUE){LARGE_INTEGER length,at;at.QuadPart=64;unsigned char actual[8]={0};DWORD count=0;
            CHECK(GetFileSizeEx(read,&length)&&length.QuadPart==128);CHECK(SetFilePointerEx(read,at,NULL,FILE_BEGIN)!=FALSE);
            CHECK(ReadFile(read,actual,8,&count,NULL)&&count==8&&std::memcmp(sample,actual,8)==0);CHECK(CloseHandle(read)!=FALSE);}
        CHECK(DeleteFileW(argv[1])!=FALSE);
    }
    std::printf("GST2 writer: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
