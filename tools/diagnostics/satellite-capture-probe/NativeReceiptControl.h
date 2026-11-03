#pragma once
#include "../../../GlobalSatelliteMap/atlas-terrain/source-receipts/NativeMetadata.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/world-bank/_generated/DecodedPins.h"
namespace NativeReceiptControl {
enum{MaxGroups=32,MaxAttempts=160};
struct Attempt{HRESULT status;char group[128],resource[128],path[260];};
struct Report{HRESULT profile,catalog;unsigned groups,attempts,accepted;Attempt item[MaxAttempts];};
// The inherited member is declared in the real base; apply its base PMD to the
// actual manager. No fake derived instance or map snapshot is constructed.
struct CatalogAccess:Ogre::ResourceGroupManager{
 typedef ResourceGroupMap Map;
 typedef Map Ogre::ResourceGroupManager::*Member;
 static Member Groups(){return &CatalogAccess::mResourceGroupMap;}
};
inline HRESULT Groups(SourceReceipts::NativeProfile&profile,char(&names)[MaxGroups][128],unsigned&count){
 count=0;if(!profile.Valid())return E_ACCESSDENIED;Ogre::ResourceGroupManager*m=profile.Manager();if(!m||!SourceReceipts::ObjectReadable(m))return E_ACCESSDENIED;SourceReceipts::TryLock lock;if(!lock.Lock(m->OGRE_AUTO_MUTEX_NAME))return HRESULT_FROM_WIN32(ERROR_BUSY);
 CatalogAccess::Map&groups=m->*CatalogAccess::Groups();if(groups.size()>MaxGroups)return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
 for(CatalogAccess::Map::const_iterator it=groups.begin();it!=groups.end();++it){if(count>=MaxGroups||!SourceReceipts::NativeString(it->first,names[count],128))return E_INVALIDARG;++count;}return S_OK;
}
// Called only on the checked owning frame. No file access/decode/GPU/thread/UI.
// This is metadata for a NEW owned named-file input, not a historical texture.
inline HRESULT Run(const SourceReceipts::Scope&scope,Report&out){
 std::memset(&out,0,sizeof(out));if(!SourceReceipts::ScopeValid(scope)||scope.thread!=GetCurrentThreadId())return E_INVALIDARG;
 SourceReceipts::NativeProfile profile;out.profile=profile.Init();if(FAILED(out.profile))return out.profile;char names[MaxGroups][128];out.catalog=Groups(profile,names,out.groups);if(FAILED(out.catalog))return out.catalog;
 const WorldBank::RgbaPin&pin=WorldBank::RgbaPins[27];
 const char*logical[5]={pin.path,"newland/land/overlaymaps/new_overlay.3.3.png","land/overlaymaps/new_overlay.3.3.png","overlaymaps/new_overlay.3.3.png","new_overlay.3.3.png"};
 for(unsigned g=0;g<out.groups;++g){SourceReceipts::NativeMetadata metadata(SourceReceipts::OwnedRawIndexedFile);HRESULT hr=metadata.Init(scope);if(FAILED(hr))return hr;SourceReceipts::WorkerRequest r={};r.scope=scope;r.role.asset=28;r.role.kind=SourceReceipts::PaintedOverlay;r.role.index=r.role.slot=0;r.pin.epoch=scope.sourceEpoch;r.pin.asset=28;r.pin.bytes=pin.encodedBytes;
  for(unsigned i=0;i<32;++i){unsigned a=pin.encodedSha[i*2],b=pin.encodedSha[i*2+1];a=a<='9'?a-'0':a-'a'+10;b=b<='9'?b-'0':b-'a'+10;r.pin.sha[i]=BYTE(a*16+b);}
  SourceReceipts::NamedEvidence evidence={r.role,SourceReceipts::OwnedNamedFile,NULL};hr=metadata.Bind(evidence);if(FAILED(hr))return hr;SourceReceipts::MainGate<SourceReceipts::NativeMetadata>gate(metadata);
  for(unsigned n=0;n<5;++n){if(out.attempts>=MaxAttempts)return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);Attempt&a=out.item[out.attempts++];std::strcpy(r.group,names[g]);std::strcpy(r.resource,logical[n]);std::strcpy(a.group,r.group);std::strcpy(a.resource,r.resource);SourceReceipts::MainToken token;a.status=gate.Capture(r,scope,token);if(SUCCEEDED(a.status)){SourceReceipts::WorkerRequest captured;if(!token.Request(captured))return E_FAIL;std::strcpy(a.path,captured.path);++out.accepted;}}
 }return S_OK;
}
}
