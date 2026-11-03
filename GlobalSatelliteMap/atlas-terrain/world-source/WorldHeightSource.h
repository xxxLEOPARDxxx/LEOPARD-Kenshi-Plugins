#pragma once
#include "../../atlas-view/AtlasReader.h"
#include "../fixed-geometry/FixedTerrainGeometry.h"
namespace WorldHeight {
typedef AtlasView::U64 U64;
const U64 FileBytes=536980180ULL,StripOffset=43686ULL,StripBytes=536936450ULL;
enum{WorldSide=16385,MaxVertexSide=1025,MaxSide=1029,MaxSamples=MaxSide*MaxSide,AuthScratch=65536};
static const unsigned char SourceSha[32]={111,125,75,199,186,175,128,219,248,219,145,230,243,103,219,113,248,127,227,143,5,58,52,16,143,25,37,132,201,99,123,24};
struct Region{int x0,z0,x1,z1;}; // Inclusive source vertices; normal halo is added here.
struct Identity{U64 epoch;DWORD volume,indexHigh,indexLow;FILETIME written;unsigned char sha[32];};
inline bool Same(const Identity&a,const Identity&b){return a.epoch&&a.epoch==b.epoch&&a.volume==b.volume&&a.indexHigh==b.indexHigh&&a.indexLow==b.indexLow&&a.written.dwHighDateTime==b.written.dwHighDateTime&&a.written.dwLowDateTime==b.written.dwLowDateTime&&!std::memcmp(a.sha,b.sha,32);}
inline unsigned U16(const unsigned char*p){return unsigned(p[0])|(unsigned(p[1])<<8);}
inline DWORD U32(const unsigned char*p){return DWORD(p[0])|(DWORD(p[1])<<8)|(DWORD(p[2])<<16)|(DWORD(p[3])<<24);}
inline bool Profile(const unsigned char*p,size_t bytes,U64 length){
 if(!p||bytes<266||length!=FileBytes||std::memcmp(p,"II\x2a\x00",4)||U32(p+4)!=8||U16(p+8)!=21||U32(p+262)!=0)return false;
 const unsigned tag[11]={254,256,257,258,259,262,273,274,277,278,279};
 const unsigned type[11]={4,3,3,3,3,3,4,3,3,3,4};
 const DWORD value[11]={0,16385,16385,16,1,1,43686,1,1,16385,536936450};unsigned found=0;
 for(unsigned i=0;i<21;++i){const unsigned char*entry=p+10+i*12;unsigned key=U16(entry);for(unsigned earlier=0;earlier<i;++earlier)if(U16(p+10+earlier*12)==key)return false;
  for(unsigned n=0;n<11;++n)if(key==tag[n]){if(U16(entry+2)!=type[n]||U32(entry+4)!=1||U32(entry+8)!=value[n])return false;found|=1u<<n;}
 }
 return found==2047&&StripBytes==U64(WorldSide)*WorldSide*2&&StripOffset+StripBytes<=length;
}
class Operation{
 friend class Source;AtlasView::ReadRequest io_;Operation(const Operation&);Operation&operator=(const Operation&);
 void Begin(){io_.error=ERROR_SUCCESS;io_.readBytes=0;}
public:
 Operation(){}
 void Cancel(){InterlockedExchange(&io_.cancel,1);}
 // Only when the owner has joined/finished the previous operation.
 void Reset(){InterlockedExchange(&io_.cancel,0);Begin();io_.afterChunk=NULL;io_.observer=NULL;}
 void Observe(void(*afterChunk)(void*),void*observer){io_.afterChunk=afterChunk;io_.observer=observer;}
 DWORD Error()const{return io_.error;}U64 ReadBytes()const{return io_.readBytes;}
};
class Window{
 friend class Source;std::vector<unsigned short>data_;Identity identity_;Region region_;unsigned x_,z_,width_,height_;bool ready_;
 Window(const Window&);Window&operator=(const Window&);
 bool Cached(const Identity&id,const Region&r)const{return ready_&&Same(identity_,id)&&r.x0==region_.x0&&r.z0==region_.z0&&r.x1==region_.x1&&r.z1==region_.z1;}
public:
 Window():x_(0),z_(0),width_(0),height_(0),ready_(false){std::memset(&identity_,0,sizeof(identity_));std::memset(&region_,0,sizeof(region_));}
 void Clear(){std::vector<unsigned short>().swap(data_);ready_=false;x_=z_=width_=height_=0;std::memset(&identity_,0,sizeof(identity_));std::memset(&region_,0,sizeof(region_));}
 FixedTerrain::Samples View()const{FixedTerrain::Samples s={};if(ready_){s.minX=x_;s.minZ=z_;s.width=s.stride=width_;s.height=height_;s.data=&data_[0];s.sampleCount=data_.size();}return s;}
 U64 OwnedBytes()const{return U64(data_.capacity())*2;}bool Ready()const{return ready_;}const Identity&Stamp()const{return identity_;}
};
class Source{
 HANDLE file_;Identity identity_;DWORD owner_;U64 authBytes_;Source(const Source&);Source&operator=(const Source&);
 static bool Fail(Operation&o,DWORD code){o.io_.error=code?code:ERROR_INVALID_DATA;return false;}
public:
 Source():file_(INVALID_HANDLE_VALUE),owner_(0),authBytes_(0){std::memset(&identity_,0,sizeof(identity_));}
 ~Source(){Close();}
 void Close(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);file_=INVALID_HANDLE_VALUE;owner_=0;authBytes_=0;std::memset(&identity_,0,sizeof(identity_));}
 bool Ready()const{return file_!=INVALID_HANDLE_VALUE&&identity_.epoch!=0;}
 const Identity&Stamp()const{return identity_;}U64 AuthenticatedBytes()const{return authBytes_;}
 bool Open(const wchar_t*path,U64 epoch,Operation&o){
  o.Begin();if(AtlasView::Cancelled(o.io_))return false;if(Ready())return Fail(o,ERROR_ALREADY_INITIALIZED);if(!path||!*path||!epoch)return Fail(o,ERROR_INVALID_PARAMETER);
  // Keep this exact authenticated file object; deny all write/delete sharing.
  file_=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_RANDOM_ACCESS,NULL);if(file_==INVALID_HANDLE_VALUE)return Fail(o,GetLastError());
  bool ok=false;try{
   LARGE_INTEGER length={};BY_HANDLE_FILE_INFORMATION info={};unsigned char header[266],digest[32];
   if(!GetFileSizeEx(file_,&length)||!GetFileInformationByHandle(file_,&info))Fail(o,GetLastError());
   else if(length.QuadPart!=FileBytes||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))Fail(o,ERROR_BAD_FORMAT);
   else if(!AtlasView::ReadAt(file_,0,header,sizeof(header),o.io_,NULL)){}
   else if(!Profile(header,sizeof(header),U64(length.QuadPart)))Fail(o,ERROR_BAD_FORMAT);
   else{
    AtlasView::Hash hash;bool hashed=hash.Begin();unsigned char scratch[AuthScratch];
    if(!hashed)Fail(o,ERROR_CRC);
    for(U64 offset=0;hashed&&offset<FileBytes;){DWORD take=DWORD(FileBytes-offset);if(take>AuthScratch)take=AuthScratch;hashed=AtlasView::ReadAt(file_,offset,scratch,take,o.io_,&hash);offset+=take;}
    if(hashed&&(!hash.Finish(digest)||std::memcmp(digest,SourceSha,32)))Fail(o,ERROR_CRC);
    else if(hashed&&!AtlasView::Cancelled(o.io_)){identity_.epoch=epoch;identity_.volume=info.dwVolumeSerialNumber;identity_.indexHigh=info.nFileIndexHigh;identity_.indexLow=info.nFileIndexLow;identity_.written=info.ftLastWriteTime;std::memcpy(identity_.sha,digest,32);owner_=GetCurrentThreadId();authBytes_=FileBytes;ok=true;}
   }
  }catch(...){Fail(o,ERROR_NOT_ENOUGH_MEMORY);}
  if(!ok)Close();return ok;
 }
 bool Read(const Region&r,U64 expectedEpoch,Window&out,Operation&o)const{
  o.Begin();if(AtlasView::Cancelled(o.io_)){out.Clear();return false;}
  if(!Ready()||expectedEpoch!=identity_.epoch||r.x0<0||r.z0<0||r.x1<r.x0||r.z1<r.z0||r.x1>=WorldSide||r.z1>=WorldSide||r.x1-r.x0>=MaxVertexSide||r.z1-r.z0>=MaxVertexSide){out.Clear();return Fail(o,ERROR_INVALID_PARAMETER);}
  if(owner_!=GetCurrentThreadId()){out.Clear();return Fail(o,ERROR_INVALID_THREAD_ID);}
  if(out.Cached(identity_,r))return true;
  const unsigned x=unsigned(r.x0<2?0:r.x0-2),z=unsigned(r.z0<2?0:r.z0-2),lastX=unsigned(r.x1>16382?16384:r.x1+2),lastZ=unsigned(r.z1>16382?16384:r.z1+2),width=lastX-x+1,height=lastZ-z+1;const size_t count=size_t(width)*height;
  if(width>MaxSide||height>MaxSide||count>MaxSamples){out.Clear();return Fail(o,ERROR_INVALID_PARAMETER);}
  bool ok=false;try{
   // Never let vector growth exceed the explicit one-window capacity bound.
   if(out.data_.capacity()<count)out.Clear();out.ready_=false;out.data_.resize(count);
   ok=true;for(unsigned row=0;row<height&&ok;++row){const U64 offset=StripOffset+2*(U64(z+row)*WorldSide+x);ok=AtlasView::ReadAt(file_,offset,reinterpret_cast<unsigned char*>(&out.data_[size_t(row)*width]),width*2,o.io_,NULL);}
   if(ok&&AtlasView::Cancelled(o.io_))ok=false;
   if(ok){out.identity_=identity_;out.region_=r;out.x_=x;out.z_=z;out.width_=width;out.height_=height;out.ready_=true;}
  }catch(...){Fail(o,ERROR_NOT_ENOUGH_MEMORY);ok=false;}
  if(!ok)out.Clear();return ok;
 }
};
}
