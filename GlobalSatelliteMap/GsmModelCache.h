#pragma once
#include "GsmTownValues.h"
#include "atlas-models/NativeModel.h"
#include "atlas-models/ModelRegion.h"
#include <set>
namespace GsmModels {
const unsigned Slots=64;
// Only borrowed for the immediately following synchronous Refresh. No native
// Town pointer survives a map-update/render boundary.
inline Town* Resolve(const void*sharedKing,const void*module,unsigned id,LONG generation,volatile LONG*live){
 __try{
  if(!GsmTownValues::Generation(live,generation))return NULL;
  const BYTE*engine=static_cast<const BYTE*>(module);if(!GsmTownDiagnostic::Profile(engine))return NULL;
  GsmTownDiagnostic::ListHeader first,after;if(!GsmTownDiagnostic::Header(static_cast<const BYTE*>(sharedKing),first))return NULL;
  const BYTE*chosen=NULL;unsigned at=0;
  for(unsigned i=0;i<first.count;++i){const BYTE*p=static_cast<const BYTE*>(GsmTownDiagnostic::Pointer(reinterpret_cast<const BYTE*>(first.array+i)));if(p&&GsmTownDiagnostic::Pointer(p)==engine+GsmTownDiagnostic::TownVtable&&GsmTownDiagnostic::Number(p+0x64)==id){if(chosen)return NULL;chosen=p;at=i;}}
  if(!chosen||!GsmTownDiagnostic::Header(static_cast<const BYTE*>(sharedKing),after)||!GsmTownDiagnostic::SameHeader(first,after)||
    GsmTownDiagnostic::Pointer(reinterpret_cast<const BYTE*>(first.array+at))!=chosen||GsmTownDiagnostic::Pointer(chosen)!=engine+GsmTownDiagnostic::TownVtable||GsmTownDiagnostic::Number(chosen+0x64)!=id||!GsmTownValues::Generation(live,generation))return NULL;
  return reinterpret_cast<Town*>(const_cast<BYTE*>(chosen));
 }__except(GsmTownDiagnostic::NativeFault(GetExceptionCode())){return NULL;}
}
struct Item {
 unsigned id;bool assigned,errorLogged;NativeModels::Snapshot source;AtlasModelRegion::Bounds bounds;
 Item():id(0),assigned(false),errorLogged(false){}
 void Clear(){source.Clear();assigned=errorLogged=false;id=0;bounds=AtlasModelRegion::Bounds();}
};
class Cache {
 Item item_[Slots];unsigned revision_,cursor_;AtlasView::U64 borrowedBuffers_,borrowedTextures_;
 Cache(const Cache&);Cache&operator=(const Cache&);
 void Dirty(){if(++revision_==0)revision_=1;}
 bool Ledger(){
  // Runs only after adoption/removal, never for every rasterized window.
  std::set<ID3D11Buffer*>unique;std::set<ID3D11Resource*>textures;AtlasView::U64 bytes=0,textureBytes=0;
  for(unsigned i=0;i<Slots;++i){const AtlasModels::Packet&p=item_[i].source.Geometry();for(unsigned d=0;d<p.Count();++d){
   for(unsigned slot=0;slot<17;++slot){UINT size=0;ID3D11Buffer*b=p.RetainedBuffer(d,slot,size);if(b&&unique.insert(b).second){bytes+=size;if(bytes>AtlasModels::BorrowCap)return false;}}
   AtlasView::Com<ID3D11Resource>resource;p.RetainedDiffuse(d)->GetResource(&resource.p);if(!resource.p)return false;
   if(textures.insert(resource.p).second){AtlasView::Com<ID3D11Texture2D>t;if(FAILED(resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p))))return false;
    D3D11_TEXTURE2D_DESC shape;t.p->GetDesc(&shape);if(!shape.Width||!shape.Height||shape.Width>16384||shape.Height>16384||!shape.MipLevels||shape.MipLevels>15||shape.ArraySize!=1||shape.SampleDesc.Count!=1)return false;
    unsigned block=shape.Format==DXGI_FORMAT_BC1_UNORM||shape.Format==DXGI_FORMAT_BC1_UNORM_SRGB?8:shape.Format==DXGI_FORMAT_BC2_UNORM||shape.Format==DXGI_FORMAT_BC2_UNORM_SRGB||shape.Format==DXGI_FORMAT_BC3_UNORM||shape.Format==DXGI_FORMAT_BC3_UNORM_SRGB?16:0;
    if(!block&&shape.Format!=DXGI_FORMAT_R8G8B8A8_UNORM&&shape.Format!=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB&&shape.Format!=DXGI_FORMAT_B8G8R8A8_UNORM&&shape.Format!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)return false;
    for(unsigned level=0,w=shape.Width,h=shape.Height;level<shape.MipLevels;++level,w=(std::max)(w/2,1u),h=(std::max)(h/2,1u)){textureBytes+=block?AtlasView::U64((w+3)/4)*((h+3)/4)*block:AtlasView::U64(w)*h*4;if(textureBytes>64ULL*1024*1024)return false;}
   }
  }}
  borrowedBuffers_=bytes;borrowedTextures_=textureBytes;return true;
 }
public:
 Cache():revision_(1),cursor_(0),borrowedBuffers_(0),borrowedTextures_(0){}
 void Clear(){for(unsigned i=0;i<Slots;++i)item_[i].Clear();borrowedBuffers_=borrowedTextures_=cursor_=0;Dirty();NativeModels::ResetProfile();}
 unsigned Revision()const{return revision_;}AtlasView::U64 BorrowedBuffers()const{return borrowedBuffers_;}AtlasView::U64 BorrowedTextures()const{return borrowedTextures_;}
 void Invalidate(){Dirty();}
 const Item&At(unsigned i)const{return item_[i];}
 unsigned Valid(unsigned generation)const{unsigned n=0;for(unsigned i=0;i<Slots;++i)if(item_[i].source.Valid(generation))++n;return n;}
 // Complete catalog publication: absence retires the old packet and dirties its
 // old windows. Overflow refuses the model layer instead of silently omitting.
 bool Catalog(const GsmTownValues::Result&values){
  unsigned ids[Slots],count=0;
  for(unsigned i=0;i<values.count;++i){const GsmTownValues::TownValue&v=values.towns[i];if(!v.hasProxy||!GsmTownDiagnostic::LoadedState(unsigned(v.proxyState)))continue;
   if(count==Slots)return false;for(unsigned k=0;k<count;++k)if(ids[k]==v.id)return false;ids[count++]=v.id;
  }
  bool removed=false;
  for(unsigned i=0;i<Slots;++i)if(item_[i].assigned){bool found=false;for(unsigned k=0;k<count;++k)if(ids[k]==item_[i].id)found=true;if(!found){removed|=item_[i].source.Geometry().Count()!=0;item_[i].Clear();}}
  for(unsigned k=0;k<count;++k){bool found=false;for(unsigned i=0;i<Slots;++i)if(item_[i].assigned&&item_[i].id==ids[k])found=true;if(found)continue;
   for(unsigned i=0;i<Slots;++i)if(!item_[i].assigned){item_[i].assigned=true;item_[i].id=ids[k];break;}}
  if(removed){Dirty();if(!Ledger())return false;}return true;
 }
 // At most two native source admissions per200ms map tick; no native calls in
 // per-window Render. Existing immutable DistantTown geometry only.
 template<class Log>bool Refresh(const void*king,const void*module,Ogre::SceneManager*scene,LONG generation,volatile LONG*live,Log log){
  unsigned admitted=0;
  for(unsigned scanned=0;scanned<Slots&&admitted<2;++scanned){unsigned index=cursor_;cursor_=(cursor_+1)%Slots;Item&e=item_[index];if(!e.assigned)continue;++admitted;
   bool old=e.source.Valid(unsigned(generation)),changed=false;Town*town=Resolve(king,module,e.id,generation,live);
   HRESULT hr=e.source.Refresh(true,town,scene,unsigned(generation),true,&changed);
   if(!GsmTownValues::Generation(live,generation)){Clear();return false;}
   bool valid=SUCCEEDED(hr)&&e.source.Valid(unsigned(generation));
   if(valid&&!AtlasModelRegion::WorldBounds(e.source.Geometry().RawSceneWorld(),e.source.Geometry().SourceSceneOrigin(),e.source.LocalMeshBounds(),e.bounds)){e.source.Clear();valid=false;hr=E_INVALIDARG;}
   if((old&&!valid)||changed){Dirty();if(!Ledger()){e.source.Clear();Ledger();return false;}}
   if(FAILED(hr)){if(!e.errorLogged){log(e.id,e.source.LastError());e.errorLogged=true;}}else e.errorLogged=false;
  }return true;
 }
};
}
