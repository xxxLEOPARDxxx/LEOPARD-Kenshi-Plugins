#pragma once
#include <d3d11.h>
#include "../../atlas-view/AtlasRelight.h"
#include "../../../tools/diagnostics/satellite-capture-probe/GeographyPolicy.h"
#include "_generated/Case.h"
namespace CompactMaterial {
typedef AtlasView::U64 U64;
const U64 Limit=90ULL*1024*1024,SourceBytes=83885956ULL;
struct Plan{unsigned layers[4],selector[4];U64 bytes;};
inline bool FixedPlan(Plan&p){p.layers[0]=p.layers[1]=6;p.layers[2]=p.layers[3]=1;p.selector[0]=18;p.selector[1]=4;p.selector[2]=p.selector[3]=0;p.bytes=SourceBytes;return true;}
inline bool Fits(U64 other){return other<=Limit-SourceBytes&&other<=AtlasView::GpuCap-SourceBytes;}
inline bool HashMatches(const void*data,size_t bytes,const Pin&p){unsigned char hash[32];return data&&bytes==p.bytes&&bytes<=MAXDWORD&&AtlasView::Digest(data,DWORD(bytes),hash)&&!std::memcmp(hash,p.sha,32);}
class SavedPredicate{ID3D11DeviceContext*c_;AtlasView::Com<ID3D11Predicate>old_;BOOL value_;SavedPredicate(const SavedPredicate&);SavedPredicate&operator=(const SavedPredicate&);public:explicit SavedPredicate(ID3D11DeviceContext*c):c_(c),value_(FALSE){c_->GetPredication(&old_.p,&value_);c_->SetPredication(NULL,FALSE);}~SavedPredicate(){c_->SetPredication(old_.p,value_);}};
// Own fixed D3D sources only. File decoding/hash work is caller scheduled. Each
// upload accepts one bounded blob and retains none; no native/Ogre/file calls.
class Inputs{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11Texture2D>tex_[7];AtlasView::Com<ID3D11ShaderResourceView>view_[7];unsigned mask_;bool ready_,broken_;
 Inputs(const Inputs&);Inputs&operator=(const Inputs&);
 HRESULT Texture(unsigned i,unsigned side,unsigned layers,unsigned mips,DXGI_FORMAT format,bool generate,bool array){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=side;t.ArraySize=layers;t.MipLevels=mips;t.Format=format;t.SampleDesc.Count=1;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|(generate?D3D11_BIND_RENDER_TARGET:0);t.MiscFlags=generate?D3D11_RESOURCE_MISC_GENERATE_MIPS:0;HRESULT hr=device_.p->CreateTexture2D(&t,NULL,&tex_[i].p);if(FAILED(hr))return hr;D3D11_SHADER_RESOURCE_VIEW_DESC s={};s.Format=format;s.ViewDimension=array?D3D11_SRV_DIMENSION_TEXTURE2DARRAY:D3D11_SRV_DIMENSION_TEXTURE2D;if(array){s.Texture2DArray.ArraySize=layers;s.Texture2DArray.MipLevels=mips;}else s.Texture2D.MipLevels=mips;return device_.p->CreateShaderResourceView(tex_[i].p,&s,&view_[i].p);}
public:
 Inputs():mask_(0),ready_(false),broken_(false){}
 void Clear(){for(unsigned i=0;i<7;++i){view_[i].Reset();tex_[i].Reset();}device_.Reset();mask_=0;ready_=broken_=false;}
 U64 OwnedGpuBytes()const{return device_.p?SourceBytes:0;}
 bool Ready()const{return ready_&&!broken_;}
 ID3D11ShaderResourceView*View(unsigned slot)const{return Ready()&&slot<8?view_[slot==7?0:slot].p:NULL;}
 HRESULT Init(ID3D11Device*d,U64 other){if(!d||device_.p)return E_INVALIDARG;if(!Fits(other))return E_OUTOFMEMORY;device_.p=d;d->AddRef();HRESULT hr=Texture(0,2048,6,9,DXGI_FORMAT_BC3_UNORM,false,true);if(SUCCEEDED(hr))hr=Texture(1,2048,6,9,DXGI_FORMAT_BC1_UNORM,false,true);if(SUCCEEDED(hr))hr=Texture(2,1024,1,11,DXGI_FORMAT_R8G8B8A8_UNORM,true,true);if(SUCCEEDED(hr))hr=Texture(3,2048,1,12,DXGI_FORMAT_R8G8B8A8_UNORM,true,true);if(SUCCEEDED(hr))hr=Texture(4,1,1,1,DXGI_FORMAT_R32G32_FLOAT,false,false);if(SUCCEEDED(hr))hr=Texture(5,1024,1,11,DXGI_FORMAT_R8G8B8A8_UNORM,true,false);if(SUCCEEDED(hr))hr=Texture(6,4,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,false,false);if(FAILED(hr)){Clear();return hr;}AtlasView::Com<ID3D11DeviceContext>c;d->GetImmediateContext(&c.p);SavedPredicate predicate(c.p);const float zero[2]={};unsigned char selector[64]={};selector[40]=18;selector[41]=4;c.p->UpdateSubresource(tex_[4].p,0,NULL,zero,8,0);c.p->UpdateSubresource(tex_[6].p,0,NULL,selector,16,0);hr=d->GetDeviceRemovedReason();if(FAILED(hr)){Clear();return hr;}return S_OK;}
 HRESULT UploadDds(unsigned index,const void*data,size_t bytes){if(!device_.p||Ready()||broken_||index>=12||(mask_&(1u<<index)))return E_INVALIDARG;SatelliteGeography::Mip mips[9];if(!HashMatches(data,bytes,DdsPins[index])||!SatelliteGeography::Dds(data,bytes,index<6?77:71,mips))return E_INVALIDARG;AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);SavedPredicate predicate(c.p);broken_=true;for(unsigned mip=0;mip<9;++mip){unsigned side=2048>>mip,pitch=((side+3)/4)*(index<6?16:8);c.p->UpdateSubresource(tex_[index/6].p,D3D11CalcSubresource(mip,index%6,9),NULL,mips[mip].data,pitch,0);}HRESULT hr=device_.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;broken_=false;mask_|=1u<<index;return S_OK;}
 HRESULT UploadRgba(unsigned index,const void*data,size_t bytes){if(!device_.p||Ready()||broken_||index>=3||(mask_&(1u<<(12+index)))||!HashMatches(data,bytes,RgbaPins[index]))return E_INVALIDARG;AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);unsigned slot=index==2?5:index+2,side=index==1?2048:1024;SavedPredicate predicate(c.p);broken_=true;c.p->UpdateSubresource(tex_[slot].p,0,NULL,data,side*4,0);c.p->GenerateMips(view_[slot].p);HRESULT hr=device_.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;broken_=false;mask_|=1u<<(12+index);return S_OK;}
 HRESULT Complete(){if(!device_.p||broken_||ready_||mask_!=32767)return E_INVALIDARG;HRESULT hr=device_.p->GetDeviceRemovedReason();if(SUCCEEDED(hr))ready_=true;return hr;}
};
// Only texture-role fields change; current native material/weather values remain
// the adapter's responsibility. Frozen fixture parameters are not current data.
inline void ApplyRoles(float(&values)[900]){for(unsigned b=0;b<2;++b){unsigned row=RoleRows[b];std::memcpy(values+row*60+44,PinnedParameters+row*60+44,8*sizeof(float));std::memcpy(values+row*60+52,PinnedParameters+row*60+52,8*sizeof(float));}}
}
