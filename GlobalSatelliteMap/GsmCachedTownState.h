#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GsmCachedTownJob.h"
#include <algorithm>
#include <stdexcept>
struct ID3D11Device;
#ifndef GSM_CACHED_TOWN_POLICY_ONLY
#include "GsmModelCache.h"
#include "atlas-models/loaded-census/NativeCensus.h"
#include "atlas-models/private-lod/NativePrivateLod.h"
#endif
namespace GsmCachedTownState {
// Explicit render-thread Clear is the only native destruction boundary.
// A live owner at late CRT/process teardown remains at a stable heap address:
// no Ogre/COM calls after renderer shutdown, at most this one slot until exit.
// Runtime DLL unloading/reloading is unsupported.
template<class T>class ExplicitOwner {
 T*value_;ExplicitOwner(const ExplicitOwner&);ExplicitOwner&operator=(const ExplicitOwner&);
public:ExplicitOwner():value_(NULL){}~ExplicitOwner(){}
 T*Get()const{return value_;}T&Ensure(){if(!value_)value_=new T;return *value_;}
 bool Clear(){if(!value_)return true;if(!value_->Clear())return false;delete value_;value_=NULL;return true;}
};
const unsigned MemoSlots=64,ChecksPerSelect=4,TownLimit=4096;
struct Context {
 const void*scene;const void*king;const void*engine;LONG generation;volatile LONG*live;float origin[3];
 Context():scene(NULL),king(NULL),engine(NULL),generation(0),live(NULL){std::memset(origin,0,12);}
};
inline bool Finite(const float*p,unsigned n){if(!p)return false;for(unsigned i=0;i<n;++i)if(!_finite(p[i]))return false;return true;}
inline bool Current(const Context&c){return c.generation>0&&c.live&&InterlockedCompareExchange(c.live,0,0)==c.generation;}
inline bool Same(const GsmCachedTown::Request&a,const GsmCachedTown::Request&b){return a.townId==b.townId&&!std::memcmp(a.storedXYZ,b.storedXYZ,12);}
inline bool SameContext(const Context&a,const Context&b){return a.scene==b.scene&&a.king==b.king&&a.engine==b.engine&&a.generation==b.generation&&a.live==b.live&&!std::memcmp(a.origin,b.origin,12);}
inline GsmCachedTown::Request Request(const GsmTownValues::TownValue&v){GsmCachedTown::Request r;r.townId=v.id;std::memcpy(r.storedXYZ,v.storedXYZ,12);return r;}
// This exact control is used by the native State and the host native-boundary
// fixtures. No native Town pointer, file/worker ownership or per-frame timer.
template<class Ops>class Control {
 struct Memo {GsmCachedTown::Request request;unsigned tick;bool used,permanent;Memo():tick(0),used(false),permanent(false){}};
 Ops ops_;Context context_;Memo memo_[MemoSlots];GsmCachedTown::Request selected_,source_;typename Ops::Bounds bounds_;
 unsigned revision_,tick_;bool pending_,launched_,valid_,quarantined_,refused_;ID3D11Device*device_;unsigned char sha_[32];char error_[256];
 Control(const Control&);Control&operator=(const Control&);
 void Dirty(){if(++revision_==0)revision_=1;}
 void Error(const char*s){if(!s)s="cached town failure";if(!std::strcmp(error_,s))return;strncpy_s(error_,sizeof(error_),s,_TRUNCATE);Dirty();}
 void ForgetMemo(){for(unsigned i=0;i<MemoSlots;++i)memo_[i]=Memo();}
 bool MemoLive(const Memo&m)const{return m.used&&(m.permanent||unsigned(tick_-m.tick)<5);}
 bool Blocked(const GsmCachedTown::Request&r)const{for(unsigned i=0;i<MemoSlots;++i)if(MemoLive(memo_[i])&&Same(memo_[i].request,r))return true;return false;}
 bool Room()const{for(unsigned i=0;i<MemoSlots;++i)if(!MemoLive(memo_[i]))return true;return false;}
 bool Remember(const GsmCachedTown::Request&r,bool permanent){
  for(unsigned i=0;i<MemoSlots;++i)if(MemoLive(memo_[i])&&Same(memo_[i].request,r)){memo_[i].permanent|=permanent;memo_[i].tick=tick_;return true;}
  for(unsigned i=0;i<MemoSlots;++i)if(!MemoLive(memo_[i])){memo_[i].request=r;memo_[i].used=true;memo_[i].permanent=permanent;memo_[i].tick=tick_;return true;}return false;
 }
 void Unremember(const GsmCachedTown::Request&r){for(unsigned i=0;i<MemoSlots;++i)if(memo_[i].used&&Same(memo_[i].request,r))memo_[i]=Memo();}
 bool Retire(){bool had=valid_||ops_.OwnedGpuBytes()!=0;valid_=false;device_=NULL;bounds_=typename Ops::Bounds();std::memset(sha_,0,32);if(had)Dirty();if(!ops_.Clear()){quarantined_=true;pending_=launched_=false;Error(ops_.LastError());return false;}source_=GsmCachedTown::Request();return true;}
 static bool Value(const GsmTownValues::Result&v,const GsmCachedTown::Request&r){unsigned found=0;for(unsigned i=0;i<v.count;++i)if(v.towns[i].id==r.townId){if(v.towns[i].hasProxy||!Finite(v.towns[i].storedXYZ,3)||v.towns[i].storedXYZ[1]<=1||std::memcmp(v.towns[i].storedXYZ,r.storedXYZ,12))return false;++found;}return found==1;}
 static double Distance(const GsmCachedTown::Request&r,const float*camera,const float*origin){const double x=double(r.storedXYZ[0])-(double(camera[0])+origin[0]),z=double(r.storedXYZ[2])-(double(camera[2])+origin[2]);return x*x+z*z;}
 HRESULT Inspect(const GsmCachedTown::Request&r){HRESULT hr=ops_.Inspect(context_,r);if(FAILED(hr)&&hr!=E_ABORT){refused_=true;Error("cached town passive source admission refused");}return hr;}
 HRESULT SelectCurrent(const GsmTownValues::Result&values,const float*camera){
  ++tick_;unsigned checks=0;double oldDistance=1e100;
  if(valid_){if(!Value(values,source_)){if(!Retire())return E_FAIL;}else{++checks;if(Inspect(source_)!=S_OK){if(!Retire())return E_FAIL;}else oldDistance=Distance(source_,camera,context_.origin);}}
  if(refused_)return E_FAIL;
  if(pending_&&launched_){if(Value(values,selected_)&&Distance(selected_,camera,context_.origin)<=8192.0*8192.0&&Distance(selected_,camera,context_.origin)<oldDistance)return S_OK;Unremember(selected_);pending_=launched_=false;}
  pending_=launched_=false;
  if(!Room()){Error("cached town attempt memo full");return S_FALSE;}
  unsigned best[ChecksPerSelect];double distance[ChecksPerSelect];for(unsigned i=0;i<ChecksPerSelect;++i){best[i]=TownLimit;distance[i]=1e100;}
  for(unsigned i=0;i<values.count;++i){const GsmTownValues::TownValue&v=values.towns[i];if(v.hasProxy||!Finite(v.storedXYZ,3)||v.storedXYZ[1]<=1)continue;GsmCachedTown::Request r=Request(v);if((valid_&&Same(r,source_))||Blocked(r))continue;double d=Distance(r,camera,context_.origin);if(!_finite(d)||d>8192.0*8192.0||d>=oldDistance)continue;
   for(unsigned k=0;k<ChecksPerSelect;++k)if(d<distance[k]){for(unsigned j=ChecksPerSelect-1;j>k;--j){best[j]=best[j-1];distance[j]=distance[j-1];}best[k]=i;distance[k]=d;break;}
  }
  for(unsigned k=0;k<ChecksPerSelect&&checks<ChecksPerSelect;++k){if(best[k]==TownLimit)break;GsmCachedTown::Request r=Request(values.towns[best[k]]);if(!Value(values,r))continue;++checks;HRESULT hr=Inspect(r);if(hr==S_OK){selected_=r;pending_=true;return S_OK;}if(FAILED(hr)&&hr!=E_ABORT)return hr;Remember(r,false);}
  return S_FALSE;
 }
public:
 Control():revision_(1),tick_(0),pending_(false),launched_(false),valid_(false),quarantined_(false),refused_(false),device_(NULL){std::memset(sha_,0,32);error_[0]=0;}
 bool Clear(){pending_=launched_=refused_=false;ForgetMemo();context_=Context();tick_=0;return Retire();}
 HRESULT Select(const GsmTownValues::Result&values,const float*rawCamera,const float*origin,const void*scene,LONG generation,volatile LONG*live,const void*king,const void*engine){
  if(quarantined_)return E_FAIL;Context next;next.scene=scene;next.king=king;next.engine=engine;next.generation=generation;next.live=live;
  if(!Current(next)||values.generation!=generation||values.count>TownLimit||!scene||!king||!engine||!Finite(rawCamera,3)||!Finite(origin,3)){Clear();return E_ABORT;}
  std::memcpy(next.origin,origin,12);if(!SameContext(context_,next)){if(!Clear())return E_FAIL;context_=next;}
  if(refused_)return E_FAIL;
  try{return SelectCurrent(values,rawCamera);}catch(const std::exception&e){refused_=true;Error(e.what());pending_=launched_=false;Retire();return E_FAIL;}catch(...){refused_=true;Error("cached town selection failed");pending_=launched_=false;Retire();return E_FAIL;}
 }
 bool WantsRequest()const{return pending_&&!launched_&&!quarantined_&&!refused_&&Current(context_);}
 GsmCachedTown::Request RequestValue()const{return selected_;}
 void MarkLaunched(){if(!WantsRequest())return;if(!Remember(selected_,true)){pending_=false;Error("cached town attempt memo full");return;}launched_=true;}
 // otherOwnedGpu excludes this one slot, includes every other live/pending
 // allocation. Main must add OwnedGpuBytes to all subsequent render guards.
 HRESULT Adopt(const GsmCachedTown::Request&request,const GsmCachedTown::Result&result,LONG generation,const float*origin,const void*scene,ID3D11Device*d,bool verified,AtlasView::U64 otherOwnedGpu){
  if(quarantined_||refused_)return E_FAIL;if(!pending_||!launched_||!Same(request,selected_))return E_ABORT;pending_=launched_=false;
  if(!Current(context_)||generation!=context_.generation||scene!=context_.scene||!Finite(origin,3)||std::memcmp(origin,context_.origin,12)){Retire();return E_ABORT;}
  if(!verified){Retire();refused_=true;Error("cached town source profile is not verified");return E_ACCESSDENIED;}
  if(!d){Error("cached town device unavailable");return E_INVALIDARG;}
  if(valid_&&device_!=d&&!Retire())return E_FAIL;
  if(result.state!=GsmCachedTown::Ready){if(result.state==GsmCachedTown::Refused){char text[193];std::memcpy(text,result.error,192);text[192]=0;Error(text[0]?text:"cached town file refused");}return result.state==GsmCachedTown::Refused?E_FAIL:S_FALSE;}
  PrivateTownLod::BlobInfo parsed;unsigned nonzero=0;for(unsigned i=0;i<32;++i)nonzero|=result.sha256[i];
  if(result.bytes.empty()||result.bytes.size()>PrivateTownLod::MaxBytes||!nonzero||!PrivateTownLod::Parser(&result.bytes[0],result.bytes.size()).Read(parsed)||std::memcmp(&parsed,&result.info,sizeof(parsed))){Error("cached town worker metadata differs from bytes");return E_INVALIDARG;}
  if(!PrivateTownLod::Budget(otherOwnedGpu,parsed.GpuReservation(),AtlasView::GpuCap)){Error("cached town owned GPU budget refused");return E_OUTOFMEMORY;}
  try{
   HRESULT inspect=Inspect(request);if(inspect!=S_OK){if(FAILED(inspect))Retire();return inspect;}
   float world[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};for(unsigned i=0;i<3;++i)world[i*4+3]=float(double(request.storedXYZ[i])-origin[i]);typename Ops::Bounds fresh;
   if(!ops_.ComputeBounds(world,origin,parsed.localBounds,fresh)){Error("cached town world bounds refused");return E_INVALIDARG;}
   if(!Retire())return E_FAIL;source_=request;
   HRESULT hr=ops_.Read(request,result,unsigned(generation),world,origin,d,verified,otherOwnedGpu);
   if(hr!=S_OK){refused_=true;Error(ops_.LastError());Retire();return FAILED(hr)?hr:E_FAIL;}
   if(!Current(context_)||Inspect(request)!=S_OK||!ops_.Valid(unsigned(generation))||!ops_.DeviceMatches(d)){Error("cached town source changed during import");Retire();return E_ABORT;}
   bounds_=fresh;device_=d;valid_=true;std::memcpy(sha_,result.sha256,32);Unremember(request);error_[0]=0;Dirty();return S_OK;
  }catch(const std::exception&e){refused_=true;Error(e.what());Retire();return E_FAIL;}catch(...){refused_=true;Error("cached town native import failed");Retire();return E_FAIL;}
 }
 bool Valid(unsigned generation)const{return valid_&&!quarantined_&&!refused_&&context_.generation==LONG(generation)&&Current(context_)&&ops_.Valid(generation);}
 const typename Ops::Packet&Geometry()const{return ops_.Geometry();}const typename Ops::Bounds&Bounds()const{return bounds_;}
 unsigned Revision()const{return revision_;}unsigned TownId()const{return source_.townId;}
 AtlasView::U64 OwnedGpuBytes()const{return ops_.OwnedGpuBytes();}SIZE_T CpuPayloadBytes()const{return sizeof(*this)+ops_.CpuPayloadBytes();}
 const unsigned char*Sha256()const{return sha_;}const char*LastError()const{return error_;}
};
#ifndef GSM_CACHED_TOWN_POLICY_ONLY
class NativeOps {
 ExplicitOwner<PrivateTownLod::Snapshot> source_;AtlasModels::Packet empty_;
 static bool ContextCurrent(const Context&c){return Current(c)&&c.engine==GetModuleHandleA(NULL)&&c.king==shou&&au&&au->render&&au->render->scene==c.scene;}
 static HRESULT InspectUnsafe(const Context&c,const GsmCachedTown::Request&r){
  if(!ContextCurrent(c))return E_ABORT;NativeModels::Profile(unsigned(c.generation));if(!NativeLoadedCensus::Profile())return E_ACCESSDENIED;
  Town*town=GsmModels::Resolve(c.king,c.engine,r.townId,c.generation,c.live);if(!town)return S_FALSE;
  const BYTE*p=reinterpret_cast<const BYTE*>(town);if(NativeLoadedCensus::Pointer(p+0x4f8)||std::memcmp(p+0x48,r.storedXYZ,12))return S_FALSE;
  typedef TownBuildingsManager*(__fastcall*Manager)(Town*);TownBuildingsManager*m=reinterpret_cast<Manager>(reinterpret_cast<const BYTE*>(c.engine)+0x569960)(town);
  if(m!=reinterpret_cast<TownBuildingsManager*>(reinterpret_cast<BYTE*>(town)+0x250)||m->town!=town)return E_INVALIDARG;size_t count=m->buildingEntities.size();if(count>NativeLoadedCensus::EntryLimit)return E_INVALIDARG;if(!count)return S_FALSE;
  typedef void*(__fastcall*Origin)(const Ogre::SceneManager*,float*);Origin get=reinterpret_cast<Origin>(NativeModels::Pin("?getRelativeOrigin@SceneManager"));float origin[3];if(get(static_cast<const Ogre::SceneManager*>(c.scene),origin)!=origin||!Finite(origin,3)||std::memcmp(origin,c.origin,12))return E_ABORT;
  // D8310 attaches the town under root0 at TownXYZ with identity rotation.
  // The raw=absolute-origin shortcut requires this actual root basis too.
  const void*root=NativeLoadedCensus::Pointer(static_cast<const BYTE*>(c.scene)+0x5f0);if(!NativeModels::Typed(root,GetModuleHandleA(GsmOgreFile),ModelSceneNodeVtable))return E_INVALIDARG;
  const Ogre::Matrix4&matrix=NativeModels::Get<const Ogre::Matrix4&>(root,"?_getFullTransform@Node");float rootWorld[16];if(!NativeModels::Copy(&matrix[0][0],rootWorld,64))return E_FAIL;
  for(unsigned i=0;i<16;++i){float expected=i%5==0?1.f:0.f;if(i==3||i==7||i==11)expected=-origin[(i-3)/4];if(rootWorld[i]!=expected)return E_INVALIDARG;}
  return ContextCurrent(c)&&NativeLoadedCensus::Number(p+0x64)==r.townId&&!NativeLoadedCensus::Pointer(p+0x4f8)&&!std::memcmp(p+0x48,r.storedXYZ,12)&&m->town==town&&m->buildingEntities.size()==count?S_OK:E_ABORT;
 }
 static HRESULT InspectProtected(const Context&c,const GsmCachedTown::Request&r){__try{return InspectUnsafe(c,r);}__except(PrivateTownLod::Fault(GetExceptionCode())?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return E_FAIL;}}
public:
 typedef AtlasModels::Packet Packet;typedef AtlasModelRegion::Bounds Bounds;
 HRESULT Inspect(const Context&c,const GsmCachedTown::Request&r){return InspectProtected(c,r);}
 bool Clear(){return source_.Clear();}
 HRESULT Read(const GsmCachedTown::Request&r,const GsmCachedTown::Result&value,unsigned generation,const float*world,const float*origin,ID3D11Device*d,bool verified,AtlasView::U64 other){return source_.Ensure().Read(true,&value.bytes[0],value.bytes.size(),r.townId,generation,world,origin,d,verified,other);}
 bool ComputeBounds(const float*w,const float*origin,const float*local,Bounds&out){return AtlasModelRegion::WorldBounds(w,origin,local,out);}
 bool Valid(unsigned generation)const{return source_.Get()&&source_.Get()->Valid(generation);}
 bool DeviceMatches(ID3D11Device*d)const{UINT bytes=0;ID3D11Buffer*b=Geometry().RetainedBuffer(0,16,bytes);return b&&AtlasView::SameDevice(b,d);}
 const Packet&Geometry()const{return source_.Get()?source_.Get()->Geometry():empty_;}AtlasView::U64 OwnedGpuBytes()const{return source_.Get()?source_.Get()->OwnedGpuBytes():0;}SIZE_T CpuPayloadBytes()const{return source_.Get()?sizeof(PrivateTownLod::Snapshot)+source_.Get()->CpuPayloadBytes():0;}const char*LastError()const{return source_.Get()?source_.Get()->LastError():"";}
};
typedef Control<NativeOps> State;
#endif
}
