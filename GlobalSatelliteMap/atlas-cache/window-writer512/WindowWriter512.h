#pragma once
#include "../Gst2Writer.h"
// Worker-only streaming into a NEW incomplete GST2 file. No atlas publication.
namespace Gst2Windows {
typedef Gst2::U64 U64;
enum{Side=512,Across=16,Windows=256};
struct Stamp{U64 source,material,capture;};
struct Frame{Stamp stamp;unsigned tileX,tileZ,x,z;}; // x/z are aligned texels in this tile.
inline bool Valid(const Stamp&s){return s.source&&s.material&&s.capture;}
inline bool Same(const Stamp&a,const Stamp&b){return Valid(a)&&a.source==b.source&&a.material==b.material&&a.capture==b.capture;}
typedef bool(*Cancelled)(void*);
class Writer {
    Gst2::RandomFile*io_;Gst2::Writer header_;Stamp stamp_;unsigned tileX_,tileZ_;U64 done_[3][4],mask_;bool poisoned_;DWORD error_,owner_;volatile LONG cancelRequested_;
    Writer(const Writer&);Writer&operator=(const Writer&);
    bool Stop(DWORD error){poisoned_=true;error_=error;return false;}
    bool CancelledNow(Cancelled callback,void*context){return InterlockedCompareExchange(&cancelRequested_,0,0)!=0||(callback&&callback(context));}
    bool CoarseDone(unsigned plane,unsigned x,unsigned z)const{
        const unsigned left=(x/4)*4,top=(z/4)*4;
        for(unsigned row=top;row<top+4;++row)for(unsigned col=left;col<left+4;++col){const unsigned bit=row*Across+col;if(!(done_[plane][bit/64]&(1ULL<<(bit%64))))return false;}
        return true;
    }
public:
    Writer():io_(NULL),tileX_(0),tileZ_(0),mask_(0),poisoned_(false),error_(0),owner_(0),cancelRequested_(0){std::memset(done_,0,sizeof(done_));std::memset(&stamp_,0,sizeof(stamp_));}
    bool Begin(Gst2::RandomFile&io,unsigned tileX,unsigned tileZ,const Stamp&stamp){
        if(owner_&&owner_!=GetCurrentThreadId())return false;
        if(io_||poisoned_||!Valid(stamp)||tileX>=4||tileZ>=4){error_=ERROR_INVALID_PARAMETER;return false;}
        if(CancelledNow(NULL,NULL))return Stop(ERROR_CANCELLED);
        // Existing generator's linearRGB16F partial header; raw-packed flag is OFF.
        try{
        if(!header_.Begin(io,tileX,tileZ,false))return Stop(ERROR_WRITE_FAULT);
        io_=&io;stamp_=stamp;tileX_=tileX;tileZ_=tileZ;owner_=GetCurrentThreadId();error_=ERROR_SUCCESS;return true;
        }catch(...){return Stop(ERROR_UNHANDLED_EXCEPTION);}
    }
    // The only cross-thread operation; all status/access/destruction requires
    // the owning worker to have finished or joined.
    void Cancel(){InterlockedExchange(&cancelRequested_,1);}
    bool WritePlane(unsigned plane,const Frame&frame,const void*data,size_t bytes,unsigned pitch,Cancelled cancelled=NULL,void*context=NULL){
        if(owner_!=GetCurrentThreadId())return false;
        if(!io_||poisoned_)return false;
        if(CancelledNow(NULL,NULL))return Stop(ERROR_CANCELLED);
        if(!Same(stamp_,frame.stamp))return Stop(ERROR_INVALID_DATA);
        const unsigned stride=Gst2::PlaneStride(plane);
        if(plane>=3||frame.tileX!=tileX_||frame.tileZ!=tileZ_||frame.x>=8192||frame.z>=8192||frame.x%Side||frame.z%Side||!data||pitch!=Side*stride||bytes!=size_t(Side)*Side*stride){error_=ERROR_INVALID_PARAMETER;return false;}
        const unsigned x=frame.x/Side,z=frame.z/Side,bit=z*Across+x;
        const U64 flag=1ULL<<(bit%64);if(done_[plane][bit/64]&flag){error_=ERROR_ALREADY_EXISTS;return false;}
        try{
        if(CancelledNow(cancelled,context))return Stop(ERROR_CANCELLED);
        const unsigned char*row=static_cast<const unsigned char*>(data);
        for(unsigned j=0;j<Side;++j){
            if(CancelledNow(cancelled,context))return Stop(ERROR_CANCELLED);
            const U64 at=Gst2::PlaneOffset(0,plane)+(U64(frame.z+j)*8192+frame.x)*stride;
            if(at>Gst2::FileBytes()||pitch>Gst2::FileBytes()-at||!io_->WriteAt(at,row+size_t(j)*pitch,pitch))return Stop(ERROR_WRITE_FAULT);
        }
        if(CancelledNow(cancelled,context))return Stop(ERROR_CANCELLED);
        if(!io_->Flush())return Stop(ERROR_WRITE_FAULT);
        done_[plane][bit/64]|=flag;
        // Preserve the existing48 coarse-plane flags: publish a bit only when
        // ALL16 independently flushed512 windows of its2048 block exist.
        if(CoarseDone(plane,x,z)){
            const U64 coarse=1ULL<<(plane*16+(z/4)*4+x/4);unsigned char ledger[8];Gst2::Put64(ledger,mask_|coarse);
            if(!io_->WriteAt(48,ledger,8)||!io_->Flush())return Stop(ERROR_WRITE_FAULT);
            mask_|=coarse;
        }
        error_=ERROR_SUCCESS;return true;
        }catch(...){return Stop(ERROR_UNHANDLED_EXCEPTION);}
    }
    U64 Mask()const{return mask_;}
    DWORD Error()const{return error_?error_:(cancelRequested_?ERROR_CANCELLED:ERROR_SUCCESS);}
    bool Poisoned()const{return poisoned_||cancelRequested_!=0;}
    bool BaseComplete()const{return io_&&!Poisoned()&&mask_==Gst2::AllBaseBlocks;}
};
}
