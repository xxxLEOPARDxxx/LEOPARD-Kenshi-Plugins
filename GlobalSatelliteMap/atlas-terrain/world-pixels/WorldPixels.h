#pragma once
#include "../world-raster/WorldTerrainRaster.h"
#include "../world-source/WorldHeightSource.h"
#include "../source-receipts/ReceiptPolicy.h"
namespace WorldPixels {
typedef AtlasView::U64 U64;
struct Raw{SourceReceipts::Scope scope;WorldRaster::Frame frame;const void*material;size_t materialBytes;const void*normal;size_t normalBytes;const void*radial;size_t radialBytes;};
struct View{SourceReceipts::Scope scope;WorldRaster::Frame frame;WorldHeight::Identity heightSource;unsigned side;const BYTE*material;const BYTE*normal;const float*height;};
// World-distance tolerance, not a height recovered from radial depth. Unknown
// hardware/terrain combinations outside this measured margin simply refuse.
const double RadialMetres=0.05;
inline bool Cancelled(HANDLE event){return event&&WaitForSingleObject(event,0)!=WAIT_TIMEOUT;}
inline bool Extent(const void*p,size_t bytes){return p&&bytes&&UINT_PTR(p)<=~UINT_PTR(0)-bytes;}
inline bool Overlap(const void*a,size_t an,const void*b,size_t bn){return Extent(a,an)&&Extent(b,bn)&&UINT_PTR(a)<UINT_PTR(b)+bn&&UINT_PTR(b)<UINT_PTR(a)+an;}
inline bool ValidFrame(const SourceReceipts::Scope&s,const WorldRaster::Frame&f){
 if(!SourceReceipts::ScopeValid(s)||!WorldBank::ValidEpoch(f.source)||f.source.value!=s.sourceEpoch||f.generation!=s.job||!f.materialGeneration||!WorldRaster::CarryBytes(f.side)||f.region>=64||f.cell>=16)return false;
 if(!_finite(f.x)||!_finite(f.z)||!_finite(f.spacing)||!_finite(f.cameraY)||!_finite(f.farClip)||!_finite(f.waterHeight)||!_finite(f.wetness)||f.spacing<1||f.spacing>9||f.cameraY<=1||f.cameraY>20000||f.farClip<=0||f.farClip>1000000||f.waterHeight< -10000||f.waterHeight>10000||f.wetness<0||f.wetness>1)return false;
 const double half=double(f.side)*f.spacing*.5;const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[f.region];const int cx=r.worldX+int(f.cell%4)*9216,cz=r.worldZ+int(f.cell/4)*9216;
 return f.x-half>=cx&&f.x+half<=cx+9216&&f.z-half>=cz&&f.z+half<=cz+9216&&f.x-half>=-147456&&f.x+half<=147456&&f.z-half>=-147456&&f.z+half<=147456&&double(f.farClip)*f.farClip>double(f.cameraY)*f.cameraY+2*half*half;
}
inline bool HeightSource(const WorldHeight::Window&w,const WorldRaster::Frame&f){
 if(!w.Ready()||w.Stamp().epoch!=f.source.value||(!w.Stamp().indexHigh&&!w.Stamp().indexLow)||std::memcmp(w.Stamp().sha,WorldHeight::SourceSha,32))return false;
 const double half=double(f.side)*f.spacing*.5,firstX=f.x-half+.5*f.spacing,firstZ=f.z-half+.5*f.spacing,lastX=f.x+half-.5*f.spacing,lastZ=f.z+half-.5*f.spacing;
 const int x0=int(std::floor((firstX+147456)/18)),z0=int(std::floor((firstZ+147456)/18)),x1=int(std::floor((lastX+147456)/18))+1,z1=int(std::floor((lastZ+147456)/18))+1;
 return x0>=0&&z0>=0&&x1<=16384&&z1<=16384&&FixedTerrain::Supports(w.View(),x0,z0,x1,z1);
}
inline double Triangle(const FixedTerrain::Samples&s,double worldX,double worldZ){
 const double x=(worldX+147456)/18,z=(worldZ+147456)/18;const int ix=int(std::floor(x)),iz=int(std::floor(z));const double tx=x-ix,tz=z-iz;
 const double a=FixedTerrain::Height(s,ix,iz),b=FixedTerrain::Height(s,ix+1,iz),c=FixedTerrain::Height(s,ix,iz+1),d=FixedTerrain::Height(s,ix+1,iz+1);
 return tx+tz<=1?a+(b-a)*tx+(c-a)*tz:d+(c-d)*(1-tx)+(b-d)*(1-tz);
}
class Accepted{
 float*data_;unsigned count_;bool ready_;SourceReceipts::Scope scope_;WorldRaster::Frame frame_;WorldHeight::Identity identity_;double maxError_;
 Accepted(const Accepted&);Accepted&operator=(const Accepted&);
 HRESULT Fail(HRESULT hr){Clear();return hr;}
public:
 Accepted():data_(NULL),count_(0),ready_(false),maxError_(0){std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&identity_,0,sizeof(identity_));}
 ~Accepted(){Clear();}
 void Clear(){delete[]data_;data_=NULL;count_=0;ready_=false;maxError_=0;std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&identity_,0,sizeof(identity_));}
 static U64 RequiredBytes(unsigned side){return WorldRaster::CarryBytes(side)?sizeof(Accepted)+U64(side)*side*16:0;}
 bool Ready()const{return ready_;}U64 OwnedBytes()const{return data_?sizeof(Accepted)+U64(count_)*16:0;}double MaximumRadialError()const{return ready_?maxError_:0;}
 HRESULT Build(bool enabled,const Raw&raw,const SourceReceipts::Scope&now,const WorldRaster::Frame&current,const WorldHeight::Window&window,HANDLE cancel,U64 otherCpu){
  if(!enabled){Clear();return S_FALSE;}
  // A new build withdraws the previous output. Refuse input borrowed from it
  // before freeing its slab, rather than reading a dangling alias afterward.
  const bool alias=data_&&(Overlap(raw.material,raw.materialBytes,data_,size_t(count_)*16)||Overlap(raw.normal,raw.normalBytes,data_,size_t(count_)*16)||Overlap(raw.radial,raw.radialBytes,data_,size_t(count_)*16));
  Clear();if(alias)return E_INVALIDARG;
  if(!ValidFrame(now,current)||!SourceReceipts::SameScope(raw.scope,now)||!WorldRaster::SameFrame(raw.frame,current))return E_ABORT;
  const unsigned side=current.side,n=side*side;const U64 charge=RequiredBytes(side);
  if(!Extent(raw.material,raw.materialBytes)||!Extent(raw.normal,raw.normalBytes)||!Extent(raw.radial,raw.radialBytes)||raw.materialBytes!=size_t(n)*8||raw.normalBytes!=size_t(n)*4||raw.radialBytes!=size_t(n)*4||Overlap(raw.material,raw.materialBytes,raw.normal,raw.normalBytes)||Overlap(raw.material,raw.materialBytes,raw.radial,raw.radialBytes)||Overlap(raw.normal,raw.normalBytes,raw.radial,raw.radialBytes))return E_INVALIDARG;
  if(!charge||charge>SourceReceipts::CpuCap||otherCpu>SourceReceipts::CpuCap-charge)return E_OUTOFMEMORY;
  if(!HeightSource(window,current))return E_INVALIDARG;if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  data_=new(std::nothrow)float[size_t(n)*4];if(!data_)return E_OUTOFMEMORY;count_=n;
  const FixedTerrain::Samples samples=window.View();const BYTE*material=static_cast<const BYTE*>(raw.material),*normal=static_cast<const BYTE*>(raw.normal),*radial=static_cast<const BYTE*>(raw.radial);BYTE*destination=reinterpret_cast<BYTE*>(data_);
  double maximum=0;const double half=double(side)*current.spacing*.5;
  for(unsigned row=0;row<side;++row){
   if(Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));const double z=current.z-half+(double(row)+.5)*current.spacing;
   for(unsigned col=0;col<side;++col){const size_t pixel=size_t(row)*side+col;const BYTE*m=material+pixel*8,*normalPixel=normal+pixel*4;
    for(unsigned channel=0;channel<4;++channel){unsigned short value;std::memcpy(&value,m+channel*2,2);if((value&0x7c00)==0x7c00)return Fail(E_INVALIDARG);}
    const int nx=int(normalPixel[0])*2-255,ny=int(normalPixel[1])*2-255,nz=int(normalPixel[2])*2-255;
    if(normalPixel[3]!=0||(!normalPixel[0]&&!normalPixel[1]&&!normalPixel[2])||nx*nx+ny*ny+nz*nz<=16)return Fail(E_INVALIDARG);
    float depth;std::memcpy(&depth,radial+pixel*4,4);if(!_finite(depth)||depth<=0||depth>=1)return Fail(E_INVALIDARG);
    const double x=current.x-half+(double(col)+.5)*current.spacing,h=Triangle(samples,x,z),dy=current.cameraY-h,dx=x-current.x,dz=z-current.z;
    if(!_finite(h)||dy<=1)return Fail(E_INVALIDARG);const double expected=std::sqrt((dx*dx+dy*dy)+dz*dz),error=std::fabs(double(depth)*current.farClip-expected);
    if(expected>=current.farClip||!_finite(error)||error>RadialMetres)return Fail(E_INVALIDARG);if(error>maximum)maximum=error;
    data_[size_t(n)*3+pixel]=float(h);
   }
   std::memcpy(destination+size_t(row)*side*8,material+size_t(row)*side*8,size_t(side)*8);std::memcpy(destination+size_t(n)*8+size_t(row)*side*4,normal+size_t(row)*side*4,size_t(side)*4);
  }
  if(Cancelled(cancel))return Fail(HRESULT_FROM_WIN32(ERROR_CANCELLED));scope_=now;frame_=current;identity_=window.Stamp();maxError_=maximum;ready_=true;return S_OK;
 }
 bool Acquire(bool enabled,const SourceReceipts::Scope&now,const WorldRaster::Frame&current,const WorldHeight::Window&window,View&out)const{
  std::memset(&out,0,sizeof(out));if(!enabled||!ready_||!data_||!SourceReceipts::SameScope(scope_,now)||!WorldRaster::SameFrame(frame_,current)||!ValidFrame(now,current)||!HeightSource(window,current)||!WorldHeight::Same(identity_,window.Stamp()))return false;
  out.scope=scope_;out.frame=frame_;out.heightSource=identity_;out.side=frame_.side;out.material=reinterpret_cast<const BYTE*>(data_);out.normal=out.material+size_t(count_)*8;out.height=data_+size_t(count_)*3;return true;
 }
};
}
