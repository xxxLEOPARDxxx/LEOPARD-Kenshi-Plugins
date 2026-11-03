#pragma once
#include "../world-material/WorldMaterialPlan.h"
#include "../../atlas-view/AtlasRelight.h"
#include "../../../tools/diagnostics/satellite-capture-probe/GeographyPolicy.h"
#include "_generated/DecodedPins.h"
namespace WorldBank {
typedef AtlasView::U64 U64;
enum{PinnedBaseSnapshot=1};
struct SourceEpoch{U64 value;unsigned profile;};
struct Ledger{U64 retained,carry,raster,outputs,readback,pending,retired,other;};
struct Bindings{ID3D11ShaderResourceView*view[8];SourceEpoch epoch;U64 revision,resourceToken;unsigned region,cell,slot,row;};
const U64 SourceBytes=83885956ULL,BankLimit=90ULL*1024*1024;
inline bool ValidEpoch(const SourceEpoch&e){return e.value&&e.profile==PinnedBaseSnapshot;}
inline bool SameEpoch(const SourceEpoch&a,const SourceEpoch&b){return a.value==b.value&&a.profile==b.profile&&ValidEpoch(a);}
inline bool Fits(const Ledger&l,U64*total=NULL){const U64 values[]={l.retained,l.carry,l.raster,l.outputs,l.readback,l.pending,l.retired,l.other};U64 sum=0;if(SourceBytes>BankLimit)return false;for(unsigned i=0;i<8;++i){if(values[i]>AtlasView::GpuCap||sum>AtlasView::GpuCap-values[i])return false;sum+=values[i];}if(sum>AtlasView::GpuCap-SourceBytes)return false;if(total)*total=sum+SourceBytes;return true;}
inline bool Hash(const void*data,size_t bytes,unsigned expectedBytes,const unsigned char(&pin)[32]){unsigned char actual[32];return data&&bytes==expectedBytes&&bytes<=MAXDWORD&&AtlasView::Digest(data,DWORD(bytes),actual)&&!std::memcmp(actual,pin,32);}
inline bool AssetHash(const void*data,size_t bytes,const SatelliteRegion::Asset&a){unsigned char pin[32];for(unsigned i=0;i<32;++i){unsigned hi=a.sha256[i*2],lo=a.sha256[i*2+1];hi=hi<='9'?hi-'0':hi-'a'+10;lo=lo<='9'?lo-'0':lo-'a'+10;pin[i]=static_cast<unsigned char>(hi*16+lo);}return Hash(data,bytes,a.bytes,pin);}
class Predicate{ID3D11DeviceContext*c_;AtlasView::Com<ID3D11Predicate>old_;BOOL value_;Predicate(const Predicate&);Predicate&operator=(const Predicate&);public:explicit Predicate(ID3D11DeviceContext*c):c_(c),value_(FALSE){c_->GetPredication(&old_.p,&value_);c_->SetPredication(NULL,FALSE);}~Predicate(){c_->SetPredication(old_.p,value_);}};
inline bool EqualBank(const WorldMaterial::Bank&a,const WorldMaterial::Bank&b){return a.count==b.count&&a.count<=24&&!std::memcmp(a.canonical,b.canonical,a.count*2)&&!std::memcmp(a.upload,b.upload,a.count*2);}
inline bool EqualPlan(const WorldMaterial::Plan&a,const WorldMaterial::Plan&b){
 if(a.epoch!=b.epoch||a.region!=b.region||a.cell!=b.cell||a.sidCount!=b.sidCount||a.sourceCount!=b.sourceCount||a.mode>WorldMaterial::OrderedStageCandidate||a.rootMask!=b.rootMask||a.treeBytes!=b.treeBytes||!SatelliteRegion::Hash(a.treeSha256)||std::memcmp(a.treeSha256,b.treeSha256,65)||a.monoSourceBytes!=b.monoSourceBytes||a.stageSourceBytes!=b.stageSourceBytes||std::memcmp(a.selector,b.selector,4)||std::memcmp(a.nativeSlots,b.nativeSlots,5)||std::memcmp(a.painted,b.painted,4)||std::memcmp(a.sourceFiles,b.sourceFiles,b.sourceCount*2))return false;
 for(unsigned k=0;k<2;++k)if(!EqualBank(a.banks[k],b.banks[k]))return false;
 for(unsigned n=0;n<b.sidCount;++n){const WorldMaterial::Binding&x=a.binding[n],&y=b.binding[n];if(x.source!=y.source||x.shaderRow!=y.shaderRow||std::memcmp(x.layers,y.layers,sizeof(x.layers)))return false;const WorldMaterial::Stage&s=a.stage[n],&t=b.stage[n];if(s.binding!=t.binding||s.channel!=t.channel||s.slot!=t.slot||s.shaderRow!=t.shaderRow||s.selectorCode!=t.selectorCode||s.pixelDiffuseOffset!=t.pixelDiffuseOffset||s.pixelNormalOffset!=t.pixelNormalOffset||s.vertexDistortionOffset!=t.vertexDistortionOffset||s.residual!=t.residual||std::memcmp(s.layers,t.layers,sizeof(s.layers)))return false;for(unsigned k=0;k<2;++k)if(!EqualBank(s.banks[k],t.banks[k]))return false;}return true;
}
class Sources{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11Texture2D>tex_[7];AtlasView::Com<ID3D11ShaderResourceView>view_[7];WorldMaterial::Planner planner_;WorldMaterial::Plan plan_;SourceEpoch epoch_;U64 revision_,allocationSerial_;unsigned slot_,region_,ddsMask_,rgbaMask_;bool selected_,ready_,broken_;
 Sources(const Sources&);Sources&operator=(const Sources&);
 bool Stage(const SourceEpoch&e,unsigned slot)const{return device_.p&&!broken_&&selected_&&SameEpoch(epoch_,e)&&slot==slot_&&slot<plan_.sidCount;}
 HRESULT Texture(unsigned i,unsigned side,unsigned layers,unsigned mips,DXGI_FORMAT format,bool generate,bool array){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=side;t.ArraySize=layers;t.MipLevels=mips;t.SampleDesc.Count=1;t.Format=format;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|(generate?D3D11_BIND_RENDER_TARGET:0);t.MiscFlags=generate?D3D11_RESOURCE_MISC_GENERATE_MIPS:0;HRESULT hr=device_.p->CreateTexture2D(&t,NULL,&tex_[i].p);if(FAILED(hr))return hr;D3D11_SHADER_RESOURCE_VIEW_DESC s={};s.Format=format;s.ViewDimension=array?D3D11_SRV_DIMENSION_TEXTURE2DARRAY:D3D11_SRV_DIMENSION_TEXTURE2D;if(array){s.Texture2DArray.ArraySize=layers;s.Texture2DArray.MipLevels=mips;}else s.Texture2D.MipLevels=mips;return device_.p->CreateShaderResourceView(tex_[i].p,&s,&view_[i].p);}
 bool NoWrites(ID3D11DeviceContext*c)const{for(unsigned i=0;i<7;++i)if(AtlasView::Bound(c,tex_[i].p,true))return false;return true;}
 HRESULT Health(){HRESULT hr=device_.p->GetDeviceRemovedReason();if(FAILED(hr)){broken_=true;ready_=false;}return hr;}
public:
 Sources():revision_(0),allocationSerial_(0),slot_(4),region_(64),ddsMask_(0),rgbaMask_(0),selected_(false),ready_(false),broken_(false){std::memset(&epoch_,0,sizeof(epoch_));std::memset(&plan_,0,sizeof(plan_));}
 void Clear(){for(unsigned i=0;i<7;++i){view_[i].Reset();tex_[i].Reset();}device_.Reset();planner_.Clear();std::memset(&epoch_,0,sizeof(epoch_));std::memset(&plan_,0,sizeof(plan_));slot_=4;region_=64;ddsMask_=rgbaMask_=0;selected_=ready_=broken_=false;}
 U64 OwnedGpuBytes()const{return device_.p?SourceBytes:0;}bool Ready()const{return ready_&&!broken_;}bool Broken()const{return broken_;}
 bool SameDevice(ID3D11Device*d)const{return AtlasView::Identity(device_.p,d);}
 ID3D11Device*Device()const{return device_.p;}U64 ResourceToken()const{return device_.p?allocationSerial_:0;}
 const WorldMaterial::Stage*CurrentStage(const SourceEpoch&e,unsigned slot)const{return Stage(e,slot)&&Ready()?&plan_.stage[slot_]:NULL;}
 bool LeaseCurrent(const Bindings&b)const{if(!Stage(b.epoch,b.slot)||!Ready()||b.resourceToken!=ResourceToken()||b.revision!=revision_||b.region!=region_||b.cell!=plan_.cell||b.row!=plan_.stage[slot_].shaderRow)return false;for(unsigned i=0;i<8;++i)if(b.view[i]!=view_[i==7?0:i].p)return false;return true;}
 HRESULT Init(ID3D11Device*d,const SourceEpoch&e,const Ledger&l){
  if(!d||device_.p||!ValidEpoch(e))return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;if(allocationSerial_==~U64(0))return E_ABORT;if(!planner_.Init(e.value))return E_INVALIDARG;device_.p=d;d->AddRef();epoch_=e;
  HRESULT hr=Texture(0,2048,6,9,DXGI_FORMAT_BC3_UNORM,false,true);if(SUCCEEDED(hr))hr=Texture(1,2048,6,9,DXGI_FORMAT_BC1_UNORM,false,true);if(SUCCEEDED(hr))hr=Texture(2,1024,1,11,DXGI_FORMAT_R8G8B8A8_UNORM,true,true);if(SUCCEEDED(hr))hr=Texture(3,2048,1,12,DXGI_FORMAT_R8G8B8A8_UNORM,true,true);if(SUCCEEDED(hr))hr=Texture(4,1,1,1,DXGI_FORMAT_R32G32_FLOAT,false,false);if(SUCCEEDED(hr))hr=Texture(5,1024,1,11,DXGI_FORMAT_R8G8B8A8_UNORM,true,false);if(SUCCEEDED(hr))hr=Texture(6,4,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,false,false);
  if(SUCCEEDED(hr)){AtlasView::Com<ID3D11DeviceContext>c;d->GetImmediateContext(&c.p);Predicate predicate(c.p);const float zero[2]={};c.p->UpdateSubresource(tex_[4].p,0,NULL,zero,8,0);hr=Health();}if(FAILED(hr)){Clear();return hr;}++allocationSerial_;return S_OK;
 }
 HRESULT BeginStage(const WorldMaterial::Plan&p,const SourceEpoch&e,unsigned slot,const Ledger&l){
  if(!device_.p||broken_||!SameEpoch(e,epoch_)||slot>=p.sidCount)return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;if(revision_==~U64(0))return E_ABORT;WorldMaterial::Plan expected;if(!planner_.Build(p.region,p.cell,e.value,0,expected)||!EqualPlan(p,expected)||slot>=expected.sidCount||(expected.selector[slot]&15)==0)return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!NoWrites(c.p))return E_INVALIDARG;const bool changed=region_!=p.region;ready_=false;++revision_;selected_=true;plan_=expected;slot_=slot;region_=p.region;ddsMask_=0;if(changed)rgbaMask_&=4;
  BYTE selector[64];for(unsigned cell=0;cell<16;++cell)std::memcpy(selector+cell*4,SatelliteRegion::Data.regions[region_].cells[cell].selector,4);Predicate predicate(c.p);c.p->UpdateSubresource(tex_[6].p,0,NULL,selector,16,0);return Health();
 }
 const SatelliteRegion::Asset*ExpectedDds(const SourceEpoch&e,unsigned slot,unsigned kind,unsigned layer)const{if(!Stage(e,slot)||kind>=2||layer>=6)return NULL;const WorldMaterial::Bank&b=plan_.stage[slot_].banks[kind];if(!b.count||b.count>6)return NULL;return &SatelliteRegion::Data.assets[b.upload[layer<b.count?layer:0]];}
 const RgbaPin*ExpectedRgba(const SourceEpoch&e,unsigned slot,unsigned kind)const{if(!Stage(e,slot)||kind>=3)return NULL;return &RgbaPins[kind<2?kind*64+region_:128];}
 bool NeedsRgba(unsigned kind)const{return selected_&&!broken_&&kind<3&&!(rgbaMask_&(1u<<kind));}
 HRESULT UploadDds(const SourceEpoch&e,unsigned slot,unsigned kind,unsigned layer,const void*data,size_t bytes,const Ledger&l){
  const SatelliteRegion::Asset*a=ExpectedDds(e,slot,kind,layer);if(!a||ready_||(ddsMask_&(1u<<(kind*6+layer))))return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;SatelliteGeography::Mip mips[9];if(!AssetHash(data,bytes,*a)||!SatelliteGeography::Dds(data,bytes,kind?71:77,mips))return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!NoWrites(c.p))return E_INVALIDARG;Predicate predicate(c.p);broken_=true;for(unsigned mip=0;mip<9;++mip){unsigned side=2048>>mip,pitch=((side+3)/4)*(kind?8:16);c.p->UpdateSubresource(tex_[kind].p,D3D11CalcSubresource(mip,layer,9),NULL,mips[mip].data,pitch,0);}HRESULT hr=Health();if(FAILED(hr))return hr;broken_=false;ddsMask_|=1u<<(kind*6+layer);return S_OK;
 }
 HRESULT UploadRgba(const SourceEpoch&e,unsigned slot,unsigned kind,const void*data,size_t bytes,const Ledger&l){
  const RgbaPin*p=ExpectedRgba(e,slot,kind);if(!p||ready_||(rgbaMask_&(1u<<kind)))return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;if(!Hash(data,bytes,p->bytes,p->sha))return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!NoWrites(c.p))return E_INVALIDARG;unsigned index=kind==2?5:kind+2,side=kind==1?2048:1024;Predicate predicate(c.p);broken_=true;c.p->UpdateSubresource(tex_[index].p,0,NULL,data,side*4,0);c.p->GenerateMips(view_[index].p);HRESULT hr=Health();if(FAILED(hr))return hr;broken_=false;rgbaMask_|=1u<<kind;return S_OK;
 }
 HRESULT Complete(const SourceEpoch&e,unsigned slot,const Ledger&l){if(!Stage(e,slot)||ready_||ddsMask_!=4095||rgbaMask_!=7)return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!NoWrites(c.p))return E_INVALIDARG;HRESULT hr=Health();if(SUCCEEDED(hr))ready_=true;return hr;}
 HRESULT AcquireBase(const SourceEpoch&e,unsigned slot,const Ledger&l,Bindings&out){if(!Stage(e,slot)||!Ready())return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;HRESULT hr=Health();if(FAILED(hr))return hr;AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!NoWrites(c.p))return E_INVALIDARG;Bindings b={};for(unsigned i=0;i<8;++i)b.view[i]=view_[i==7?0:i].p;b.epoch=epoch_;b.revision=revision_;b.resourceToken=ResourceToken();b.region=region_;b.cell=plan_.cell;b.slot=slot_;b.row=plan_.stage[slot_].shaderRow;out=b;return S_OK;}
 HRESULT PatchRoles(const SourceEpoch&e,unsigned slot,const Ledger&l,void*pixelConstants,size_t bytes)const{if(!Stage(e,slot)||!Ready()||!pixelConstants||bytes!=3856)return E_INVALIDARG;if(!Fits(l))return E_OUTOFMEMORY;const WorldMaterial::Stage&s=plan_.stage[slot_];unsigned char*p=static_cast<unsigned char*>(pixelConstants);float role[8];for(unsigned k=0;k<2;++k){const unsigned offset=k?s.pixelNormalOffset:s.pixelDiffuseOffset;std::memcpy(role,p+offset,32);for(unsigned r=0;r<6;++r)role[r]=float(s.layers[k][r]);std::memcpy(p+offset,role,32);}return S_OK;}
};
}
