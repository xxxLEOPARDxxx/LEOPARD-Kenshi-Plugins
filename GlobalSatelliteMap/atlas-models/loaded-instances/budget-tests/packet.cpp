#define _CRT_SECURE_NO_WARNINGS
#include "../InstanceRaster.h"
#include <cstdio>
#include <cfloat>
static unsigned checks=0,objects=0;
static void C(bool ok,const char*why){++checks;if(!ok){std::printf("FAIL %s\n",why);throw 1;}}
// Descriptor-only COM resources. No image/buffer payload allocation: actual
// Packet.Adopt consumes these through real ID3D11 interfaces on a WARP device.
template<class I>class Object:public I{
 LONG refs_;ID3D11Device*device_;
protected:virtual bool Supports(REFIID id)const=0;
public:Object(ID3D11Device*d):refs_(1),device_(d){++objects;d->AddRef();}virtual~Object(){--objects;device_->Release();}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**out){if(!out)return E_POINTER;*out=NULL;if(id!=__uuidof(IUnknown)&&id!=__uuidof(ID3D11DeviceChild)&&!Supports(id))return E_NOINTERFACE;*out=static_cast<I*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef(){return ULONG(InterlockedIncrement(&refs_));}ULONG STDMETHODCALLTYPE Release(){LONG n=InterlockedDecrement(&refs_);if(!n)delete this;return ULONG(n);}
 void STDMETHODCALLTYPE GetDevice(ID3D11Device**d){*d=device_;device_->AddRef();}
 HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,UINT*,void*){return E_NOTIMPL;}HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,UINT,const void*){return E_NOTIMPL;}HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,const IUnknown*){return E_NOTIMPL;}
};
class Buffer:public Object<ID3D11Buffer>{D3D11_BUFFER_DESC desc_;
 bool Supports(REFIID id)const{return id==__uuidof(ID3D11Buffer)||id==__uuidof(ID3D11Resource);}
public:Buffer(ID3D11Device*d,UINT bytes,UINT bind):Object(d){std::memset(&desc_,0,sizeof(desc_));desc_.ByteWidth=bytes;desc_.Usage=D3D11_USAGE_IMMUTABLE;desc_.BindFlags=bind;}
 void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION*d){*d=D3D11_RESOURCE_DIMENSION_BUFFER;}void STDMETHODCALLTYPE SetEvictionPriority(UINT){}UINT STDMETHODCALLTYPE GetEvictionPriority(){return 0;}void STDMETHODCALLTYPE GetDesc(D3D11_BUFFER_DESC*d){*d=desc_;}
};
class Texture:public Object<ID3D11Texture2D>{D3D11_TEXTURE2D_DESC desc_;
 bool Supports(REFIID id)const{return id==__uuidof(ID3D11Texture2D)||id==__uuidof(ID3D11Resource);}
public:Texture(ID3D11Device*d,UINT w,UINT h,UINT m,DXGI_FORMAT f):Object(d){std::memset(&desc_,0,sizeof(desc_));desc_.Width=w;desc_.Height=h;desc_.MipLevels=m;desc_.ArraySize=1;desc_.Format=f;desc_.SampleDesc.Count=1;desc_.Usage=D3D11_USAGE_IMMUTABLE;desc_.BindFlags=D3D11_BIND_SHADER_RESOURCE;}
 void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION*d){*d=D3D11_RESOURCE_DIMENSION_TEXTURE2D;}void STDMETHODCALLTYPE SetEvictionPriority(UINT){}UINT STDMETHODCALLTYPE GetEvictionPriority(){return 0;}void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC*d){*d=desc_;}
};
class View:public Object<ID3D11ShaderResourceView>{Texture*texture_;D3D11_SHADER_RESOURCE_VIEW_DESC desc_;
 bool Supports(REFIID id)const{return id==__uuidof(ID3D11ShaderResourceView)||id==__uuidof(ID3D11View);}
public:View(ID3D11Device*d,Texture*t):Object(d),texture_(t){t->AddRef();D3D11_TEXTURE2D_DESC td;t->GetDesc(&td);std::memset(&desc_,0,sizeof(desc_));desc_.Format=td.Format;desc_.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;desc_.Texture2D.MipLevels=td.MipLevels;}
 ~View(){texture_->Release();}void STDMETHODCALLTYPE GetResource(ID3D11Resource**r){*r=texture_;texture_->AddRef();}void STDMETHODCALLTYPE GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC*d){*d=desc_;}
};
struct Fixture{
 AtlasView::Com<ID3D11Device>device;AtlasView::Com<ID3D11SamplerState>sampler;AtlasView::Com<ID3D11Buffer>vertex,index;AtlasView::Com<ID3D11ShaderResourceView>views[14];LoadedInstances::RawDraw raw[2];
 Fixture(){C(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&device.p,NULL,NULL)),"real WARP device only");D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;sd.MaxLOD=FLT_MAX;C(SUCCEEDED(device.p->CreateSamplerState(&sd,&sampler.p)),"existing real sampler creation");vertex.p=new Buffer(device.p,18000000,D3D11_BIND_VERTEX_BUFFER);index.p=new Buffer(device.p,764994,D3D11_BIND_INDEX_BUFFER);
  const UINT sizes[14][4]={{2048,2048,10,77},{2048,2048,10,71},{16,16,5,71},{2048,2048,10,77},{2048,2048,10,71},{512,512,8,71},{4096,2048,11,77},{4096,2048,11,77},{2048,2048,10,77},{2048,2048,10,71},{4096,4096,11,77},{4096,4096,11,77},{4096,4096,11,77},{4096,4096,11,71}};
  for(unsigned i=0;i<14;++i){Texture*t=new Texture(device.p,sizes[i][0],sizes[i][1],sizes[i][2],DXGI_FORMAT(sizes[i][3]));views[i].p=new View(device.p,t);t->Release();}
  for(unsigned n=0;n<2;++n){LoadedInstances::RawDraw&r=raw[n];r.shape.layout=LoadedInstances::Coloured64;r.shape.vertexCount=3;r.shape.vertexBytes=18000000;r.shape.indexCount=3;r.shape.indexBytes=764994;r.shape.indexFormat=DXGI_FORMAT_R16_UINT;r.rawWorld[0]=r.rawWorld[5]=r.rawWorld[10]=r.rawWorld[15]=1;r.localBounds[3]=r.localBounds[4]=r.localBounds[5]=1;r.vertex=vertex.p;r.index=index.p;r.sampler=sampler.p;r.verifiedBlack=true;r.batch=n;for(unsigned t=0;t<7;++t)r.texture[t]=views[n*7+t].p;}
 }
};

static void Refuse(Fixture&f,const LoadedInstances::BorrowLimits&l,LoadedInstances::U64 b,LoadedInstances::U64 t,HRESULT expected,const char*part){
 const float origin[3]={};LoadedInstances::Packet p;const char*reason=NULL;LONG before=objects;
 C(p.Adopt(f.device.p,f.raw,2,origin,1,b,t,l,&reason)==expected,"actual packet refuses bounded invalid admission");
 C(!p.Count()&&!p.BorrowedBuffers()&&!p.BorrowedTextures()&&objects==before,"failed packet leaves no owned or partial COM publication");
 C(reason&&std::strstr(reason,part),"specific cap/validation diagnostic survives");
}
int main(){try{using namespace LoadedInstances;const float origin[3]={};{
 Fixture f;const U64 b=18764994,t=126004032;const BorrowLimits loaded=LoadedLimits();
 Refuse(f,BorrowLimits(),784908,349552,E_OUTOFMEMORY,"fresh texture");
 {Packet packet;C(packet.Adopt(f.device.p,f.raw,2,origin,1,784908,349552,loaded)==S_OK,"recorded Hub footprint can be admitted by loaded-town scope");
 C(packet.BorrowedBuffers()==b&&packet.BorrowedTextures()==t,"recorded COM descriptors retain exact unique resource totals");
 Packet refresh;C(refresh.Adopt(f.device.p,f.raw,2,origin,1,packet.BorrowedBuffers()+784908,packet.BorrowedTextures()+349552,loaded)==S_OK,"actual old plus fresh Hub and unrelated model references fit conservative overlap");}
 {Packet exact;BorrowLimits l(b,t,b,t);C(exact.Adopt(f.device.p,f.raw,2,origin,1,0,0,l)==S_OK,"inclusive exact fresh and total bounds succeed");}
 Refuse(f,BorrowLimits(b-1,t,b,t),0,0,E_OUTOFMEMORY,"fresh buffer");
 Refuse(f,BorrowLimits(b,t-1,b,t),0,0,E_OUTOFMEMORY,"fresh texture");
 {Packet exact;C(exact.Adopt(f.device.p,f.raw,2,origin,1,loaded.totalBuffers-b,loaded.totalTextures-t,loaded)==S_OK,"inclusive exact old plus fresh limits succeed");}
 Refuse(f,loaded,loaded.totalBuffers-b+1,0,E_OUTOFMEMORY,"overlapping buffer");
 Refuse(f,loaded,0,loaded.totalTextures-t+1,E_OUTOFMEMORY,"overlapping texture");
 Refuse(f,loaded,~U64(0),0,E_INVALIDARG,"validation");Refuse(f,loaded,0,~U64(0),E_INVALIDARG,"validation");
 BorrowLimits bad[]={BorrowLimits(0),BorrowLimits(BufferCap+1),BorrowLimits(BufferCap,128ULL*1024*1024+1),BorrowLimits(b,t,b-1,t),BorrowLimits(b,t,b,t-1),BorrowLimits(b,t,64ULL*1024*1024+1,t),BorrowLimits(b,t,b,256ULL*1024*1024+1),BorrowLimits(~U64(0))};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i)Refuse(f,bad[i],0,0,E_INVALIDARG,"validation");
 C(!BorrowRefusal(b,loaded.totalBuffers-b,loaded.freshBuffers,loaded.totalBuffers,false),"subtraction boundary remains inclusive");
 C(BorrowRefusal(b,~U64(0),loaded.freshBuffers,loaded.totalBuffers,false)!=NULL,"overflowing aggregate refuses without unsigned addition");
 }C(objects==0,"all descriptor COM refs released");std::printf("PASS %u scoped loaded-town packet controls\n",checks);return 0;}catch(...){return 1;}}
