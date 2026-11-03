#pragma once
#include "../world-raster/WorldTerrainRaster.h"
#include "../source-receipts/MainGate.h"
namespace WorldNative {
enum{InputsPerSid=15,MaxSids=4};
typedef WorldBank::U64 U64;
inline const char*CanonicalPath(U64 asset){if(asset&&asset<=SatelliteRegion::Data.assetCount)return SatelliteRegion::Data.assets[unsigned(asset-1)].path;if(asset>=0x10000ULL&&asset<0x10000ULL+129)return WorldBank::RgbaPins[unsigned(asset-0x10000ULL)].path;return NULL;}
// Asset identities refer to canonical catalog entries, never to logical names.
// DDS IDs:1..219. PNG IDs:0x10000..0x10080. Padding roles share the first
// canonical source but remain distinct role indices in the stage transaction.
inline const char*Basename(const char*path){if(!path)return NULL;const char*name=path;for(const char*p=path;*p;++p)if(*p=='/'||*p=='\\')name=p+1;return *name?name:NULL;}
inline bool ValidateSourceName(const SourceReceipts::WorkerRequest&r,const char*canonicalPath,unsigned index){
 if(index>=InputsPerSid||!canonicalPath||!SourceReceipts::Ascii(canonicalPath,SourceReceipts::TextLimit)||!SourceReceipts::Ascii(r.group,SourceReceipts::TextLimit)||!SourceReceipts::Ascii(r.resource,SourceReceipts::TextLimit))return false;
 const char*name=Basename(canonicalPath);if(!name)return false;
 if(index==2){if(std::strcmp(canonicalPath,WorldBank::RgbaPins[128].path))return false;name="blendMap.png";}
 return !std::strcmp(r.group,index<3?"Overlaymaps":"Landscape")&&!std::strcmp(r.resource,name);
}
inline bool Sha(const char*text,BYTE(&out)[32]){if(!text)return false;for(unsigned i=0;i<32;++i){unsigned a=text[i*2],b=text[i*2+1];if(!((a>='0'&&a<='9')||(a>='a'&&a<='f'))||!((b>='0'&&b<='9')||(b>='a'&&b<='f')))return false;a=a<='9'?a-'0':a-'a'+10;b=b<='9'?b-'0':b-'a'+10;out[i]=BYTE(a*16+b);}return !text[64];}
inline unsigned InputIndex(const SourceReceipts::Role&r){switch(r.kind){case SourceReceipts::PaintedOverlay:return r.index?InputsPerSid:0;case SourceReceipts::PaintedColour:return r.index?InputsPerSid:1;case SourceReceipts::Blend:return r.index?InputsPerSid:2;case SourceReceipts::Diffuse:return r.index<6?3+r.index:InputsPerSid;case SourceReceipts::Normal:return r.index<6?9+r.index:InputsPerSid;default:return InputsPerSid;}}
class SourceFactory{
 SourceReceipts::Scope scope_;WorldRaster::Frame frame_;WorldMaterial::Plan plan_;bool ready_;
 SourceFactory(const SourceFactory&);SourceFactory&operator=(const SourceFactory&);
public:
 SourceFactory(){Clear();}
 void Clear(){ready_=false;std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&plan_,0,sizeof(plan_));}
 HRESULT Init(const WorldRaster::Frame&frame,const WorldMaterial::Plan&plan,const SourceReceipts::Scope&scope){
  if(ready_)return E_UNEXPECTED;if(!SourceReceipts::ScopeValid(scope)||scope.thread!=GetCurrentThreadId()||!WorldBank::ValidEpoch(frame.source)||frame.source.value!=scope.sourceEpoch||frame.generation!=scope.job||!frame.materialGeneration||!WorldRaster::CarryBytes(frame.side))return E_INVALIDARG;
  WorldMaterial::Planner planner;WorldMaterial::Plan expected;if(!planner.Init(scope.sourceEpoch)||!planner.Build(frame.region,frame.cell,scope.sourceEpoch,0,expected)||!WorldBank::EqualPlan(plan,expected)||!expected.sidCount||expected.sidCount>MaxSids)return E_INVALIDARG;
  scope_=scope;frame_=frame;plan_=expected;ready_=true;return S_OK;
 }
 unsigned StageCount()const{return ready_?plan_.sidCount:0;}
 bool Request(unsigned slot,unsigned index,SourceReceipts::WorkerRequest&out)const{
  if(!ready_||slot>=plan_.sidCount||index>=InputsPerSid)return false;
  SourceReceipts::WorkerRequest r={};r.scope=scope_;r.role.slot=slot;r.pin.epoch=scope_.sourceEpoch;
  const char*path=NULL;const char*hash=NULL;
  if(index<3){const unsigned pin=index==2?128:index*64+frame_.region;const WorldBank::RgbaPin&p=WorldBank::RgbaPins[pin];path=p.path;hash=p.encodedSha;r.pin.bytes=p.encodedBytes;r.role.asset=0x10000ULL+pin;r.role.kind=index==0?SourceReceipts::PaintedOverlay:index==1?SourceReceipts::PaintedColour:SourceReceipts::Blend;}
  else{const unsigned kind=(index-3)/6,layer=(index-3)%6;const WorldMaterial::Bank&b=plan_.stage[slot].banks[kind];if(!b.count||b.count>6)return false;const unsigned asset=b.upload[layer<b.count?layer:0];if(asset>=SatelliteRegion::Data.assetCount)return false;const SatelliteRegion::Asset&a=SatelliteRegion::Data.assets[asset];path=a.path;hash=a.sha256;r.pin.bytes=a.bytes;r.role.asset=U64(asset)+1;r.role.kind=kind?SourceReceipts::Normal:SourceReceipts::Diffuse;r.role.index=layer;}
  r.pin.asset=r.role.asset;const char*name=index==2?"blendMap.png":Basename(path);
  if(!name||!SourceReceipts::Copy(r.resource,sizeof(r.resource),name)||!SourceReceipts::Copy(r.group,sizeof(r.group),index<3?"Overlaymaps":"Landscape")||!Sha(hash,r.pin.sha)||!ValidateSourceName(r,path,index)||!SourceReceipts::ValidNamed(r))return false;
  out=r;return true;
 }
 bool Matches(const SourceReceipts::WorkerRequest&r)const{
  if(!SourceReceipts::ValidNamed(r))return false;SourceReceipts::WorkerRequest expected;return Request(r.role.slot,InputIndex(r.role),expected)&&SourceReceipts::SameScope(r.scope,expected.scope)&&SourceReceipts::SameRole(r.role,expected.role)&&r.pin.epoch==expected.pin.epoch&&r.pin.asset==expected.pin.asset&&r.pin.bytes==expected.pin.bytes&&!std::memcmp(r.pin.sha,expected.pin.sha,32)&&!std::strcmp(r.group,expected.group)&&!std::strcmp(r.resource,expected.resource);
 }
};
}
