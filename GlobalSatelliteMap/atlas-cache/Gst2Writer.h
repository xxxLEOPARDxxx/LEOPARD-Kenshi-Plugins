#ifndef LEOPARD_GST2_WRITER_H
#define LEOPARD_GST2_WRITER_H
// C++03, Win32. Caller owns one tightly packed 2048-square plane (8/4/4B per texel).
// Never publishes COMPLETE; build_mips.py validates/finalizes the whole atlas.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>

namespace Gst2 {
typedef unsigned __int64 U64;
enum { HeaderBytes=1024, Grid=4, BaseSide=8192, Levels=6, Planes=3, BlockSide=2048 };
const U64 AllBaseBlocks=0xffffffffffffULL;
inline void Put32(unsigned char *p,unsigned n){for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(n>>(i*8));}
inline void Put64(unsigned char *p,U64 n){for(unsigned i=0;i<8;++i)p[i]=static_cast<unsigned char>(n>>(i*8));}
inline unsigned PlaneStride(unsigned plane){return plane==0?8:4;}
inline unsigned BlockPitch(unsigned plane){return BlockSide*PlaneStride(plane);}
inline unsigned BlockBytes(unsigned plane){return BlockPitch(plane)*BlockSide;}
inline U64 PlaneBytes(unsigned level,unsigned plane){return (static_cast<U64>(BaseSide)>>level)*(BaseSide>>level)*PlaneStride(plane);}
inline U64 PlaneOffset(unsigned level,unsigned plane){U64 n=HeaderBytes;for(unsigned i=0;i<level;++i)for(unsigned p=0;p<Planes;++p)n+=PlaneBytes(i,p);for(unsigned p=0;p<plane;++p)n+=PlaneBytes(level,p);return n;}
inline U64 FileBytes(){return PlaneOffset(Levels-1,Planes-1)+PlaneBytes(Levels-1,Planes-1);}

struct RandomFile {
    virtual ~RandomFile(){}
    virtual bool Resize(U64 length)=0;
    virtual bool WriteAt(U64 offset,const void *data,DWORD bytes)=0;
    virtual bool Flush()=0;
};
class Win32File : public RandomFile {
    HANDLE file_;
    Win32File(const Win32File&);Win32File&operator=(const Win32File&);
public:
    Win32File():file_(INVALID_HANDLE_VALUE){}
    ~Win32File(){Close();}
    bool OpenNew(const wchar_t *path){
        if(file_!=INVALID_HANDLE_VALUE||!path)return false;
        file_=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        return file_!=INVALID_HANDLE_VALUE;
    }
    bool Close(){if(file_==INVALID_HANDLE_VALUE)return true;HANDLE f=file_;file_=INVALID_HANDLE_VALUE;return CloseHandle(f)!=FALSE;}
    bool Resize(U64 n){LARGE_INTEGER at;at.QuadPart=static_cast<LONGLONG>(n);return file_!=INVALID_HANDLE_VALUE&&n<=0x7fffffffffffffffULL&&SetFilePointerEx(file_,at,NULL,FILE_BEGIN)&&SetEndOfFile(file_);}
    bool WriteAt(U64 offset,const void *data,DWORD bytes){
        if(file_==INVALID_HANDLE_VALUE||!data||offset>0x7fffffffffffffffULL||bytes>0x7fffffffffffffffULL-offset)return false;
        LARGE_INTEGER at;at.QuadPart=static_cast<LONGLONG>(offset);
        if(!SetFilePointerEx(file_,at,NULL,FILE_BEGIN))return false;
        const unsigned char *p=static_cast<const unsigned char*>(data);
        while(bytes){DWORD done=0;if(!WriteFile(file_,p,bytes,&done,NULL)||!done||done>bytes)return false;p+=done;bytes-=done;}
        return true;
    }
    bool Flush(){return file_!=INVALID_HANDLE_VALUE&&FlushFileBuffers(file_)!=FALSE;}
};
class Writer {
    RandomFile *io_;U64 mask_;bool poisoned_;
    Writer(const Writer&);Writer&operator=(const Writer&);
public:
    Writer():io_(NULL),mask_(0),poisoned_(false){}
    bool Begin(RandomFile &io,unsigned tileX,unsigned tileZ,bool rawPacked=false){
        if(io_||poisoned_||tileX>=Grid||tileZ>=Grid)return false;
        io_=&io;
        unsigned char h[HeaderBytes];std::memset(h,0,sizeof(h));std::memcpy(h,"GST2",4);
        Put32(h+4,2);Put32(h+8,HeaderBytes);Put32(h+16,tileX);Put32(h+20,tileZ);
        Put32(h+12,rawPacked?2:0);
        Put32(h+24,Grid);Put32(h+28,BaseSide);Put32(h+32,Levels);Put32(h+36,Planes);Put32(h+40,BlockSide);
        Put32(h+56,10);Put32(h+60,28);Put32(h+64,41); // Actual DXGI plane format IDs.
        for(unsigned l=0;l<Levels;++l)for(unsigned p=0;p<Planes;++p){unsigned at=128+(l*Planes+p)*48;Put64(h+at,PlaneOffset(l,p));Put64(h+at+8,PlaneBytes(l,p));}
        if(!io.Resize(FileBytes())||!io.WriteAt(0,h,HeaderBytes)||!io.Flush()){poisoned_=true;return false;}
        return true;
    }
    // x,z are texel coordinates, row0=min world Z, column0=min world X.
    // Exactly one complete aligned block. Validation failures perform no I/O.
    bool WriteBlock(unsigned plane,unsigned x,unsigned z,const void *data,size_t bytes,unsigned pitch){
        if(!io_||poisoned_||plane>=Planes||x>=BaseSide||z>=BaseSide||x%BlockSide||z%BlockSide||!data||bytes!=BlockBytes(plane)||pitch!=BlockPitch(plane))return false;
        unsigned bit=plane*16+(z/BlockSide)*4+x/BlockSide;U64 flag=1ULL<<bit;
        if(mask_&flag)return false;
        const unsigned char *row=static_cast<const unsigned char*>(data);
        for(unsigned j=0;j<BlockSide;++j){
            U64 at=PlaneOffset(0,plane)+(static_cast<U64>(z+j)*BaseSide+x)*PlaneStride(plane);
            if(at>FileBytes()||pitch>FileBytes()-at||!io_->WriteAt(at,row+j*pitch,pitch)){poisoned_=true;return false;}
        }
        if(!io_->Flush()){poisoned_=true;return false;}
        unsigned char ledger[8];Put64(ledger,mask_|flag);
        if(!io_->WriteAt(48,ledger,8)||!io_->Flush()){poisoned_=true;return false;}
        mask_|=flag;return true;
    }
    U64 Mask()const{return mask_;}
    bool Poisoned()const{return poisoned_;}
    bool BaseComplete()const{return io_&&!poisoned_&&mask_==AllBaseBlocks;}
};
}
#endif
