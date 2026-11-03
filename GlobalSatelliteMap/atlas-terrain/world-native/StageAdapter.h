#pragma once
#include "SourceFactory.h"
namespace WorldNative {
// Stage supplies Init/BindOwned/Capture/Refresh/BeginBank/EndBank. The production
// NativeStage wrapper uses OwnedRawIndexedFile; tests supply a plain fake.
// This entire owner and all its tokens stay on the Main callback thread.
template<class Stage>class StageAdapter{
 Stage stages_[MaxSids];SourceFactory factory_;SourceReceipts::Scope scope_;DWORD owner_;unsigned active_;bool initialized_,bound_,attempted_;
 StageAdapter(const StageAdapter&);StageAdapter&operator=(const StageAdapter&);
 bool OnMain()const{return owner_==GetCurrentThreadId();}
 bool Route(const SourceReceipts::WorkerRequest&r)const{return OnMain()&&bound_&&r.role.slot<factory_.StageCount()&&factory_.Matches(r);}
 static void ClearTokens(SourceReceipts::MainToken(&tokens)[MaxSids][InputsPerSid]){for(unsigned s=0;s<MaxSids;++s)for(unsigned i=0;i<InputsPerSid;++i)tokens[s][i].Clear();}
public:
 StageAdapter():owner_(GetCurrentThreadId()),active_(MaxSids),initialized_(false),bound_(false),attempted_(false){std::memset(&scope_,0,sizeof(scope_));}
 ~StageAdapter(){EndBank();} // The owner must also be destroyed on Main.
 HRESULT Init(const SourceReceipts::Scope&scope){if(!OnMain())return RPC_E_WRONG_THREAD;if(initialized_||attempted_)return E_UNEXPECTED;if(!SourceReceipts::ScopeValid(scope)||scope.thread!=owner_)return E_INVALIDARG;scope_=scope;initialized_=true;return S_OK;}
 unsigned CapturedStageCount()const{return OnMain()&&bound_?factory_.StageCount():0;}
 HRESULT CaptureSources(const WorldRaster::Frame&frame,const WorldMaterial::Plan&plan,const SourceReceipts::Scope&scope,SourceReceipts::MainToken(&tokens)[MaxSids][InputsPerSid]){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(!initialized_||attempted_)return E_UNEXPECTED;if(!SourceReceipts::SameScope(scope_,scope))return E_INVALIDARG;attempted_=true;ClearTokens(tokens);
  HRESULT hr=factory_.Init(frame,plan,scope);if(FAILED(hr))return hr;
  // Every provider binds its entire stage before any snapshot can be captured.
  for(unsigned s=0;s<factory_.StageCount()&&SUCCEEDED(hr);++s){hr=stages_[s].Init(scope_);for(unsigned i=0;i<InputsPerSid&&SUCCEEDED(hr);++i){SourceReceipts::WorkerRequest r;if(!factory_.Request(s,i,r))hr=E_INVALIDARG;else hr=stages_[s].BindOwned(r.role);}}
  if(FAILED(hr))return hr;bound_=true;SourceReceipts::MainGate<StageAdapter<Stage> >gate(*this);
  for(unsigned s=0;s<factory_.StageCount()&&SUCCEEDED(hr);++s)for(unsigned i=0;i<InputsPerSid&&SUCCEEDED(hr);++i){SourceReceipts::WorkerRequest r;if(!factory_.Request(s,i,r))hr=E_INVALIDARG;else hr=gate.Capture(r,scope_,tokens[s][i]);}
  if(FAILED(hr)){ClearTokens(tokens);bound_=false;}return hr;
 }
 HRESULT Capture(const SourceReceipts::WorkerRequest&r,SourceReceipts::MainSnapshot&out){if(!OnMain())return RPC_E_WRONG_THREAD;if(!Route(r))return E_INVALIDARG;return stages_[r.role.slot].Capture(r,out);}
 HRESULT Refresh(const SourceReceipts::MainSnapshot&before,SourceReceipts::MainSnapshot&out){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(!SourceReceipts::Valid(before.request)||!Route(before.request)||before.semantics!=SourceReceipts::OwnedRawIndexedFile)return E_INVALIDARG;
  HRESULT hr=stages_[before.request.role.slot].Refresh(before,out);if(FAILED(hr))return hr;return SourceReceipts::SameSnapshot(before,out)?S_OK:E_ACCESSDENIED;
 }
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*s,unsigned count,const SourceReceipts::Scope&scope){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(!bound_||!s||count!=InputsPerSid||active_!=MaxSids||!SourceReceipts::SameScope(scope_,scope))return E_INVALIDARG;
  const unsigned slot=s[0].request.role.slot;unsigned mask=0;
  for(unsigned i=0;i<count;++i){const SourceReceipts::WorkerRequest&r=s[i].request;unsigned input=InputIndex(r.role);if(r.role.slot!=slot||!Route(r)||s[i].semantics!=SourceReceipts::OwnedRawIndexedFile||input>=InputsPerSid||(mask&(1u<<input)))return E_INVALIDARG;mask|=1u<<input;}
  if(mask!=32767)return E_INVALIDARG;HRESULT hr=stages_[slot].BeginBank(s,count,scope);if(SUCCEEDED(hr))active_=slot;return hr;
 }
 void EndBank(){if(!OnMain()||active_>=MaxSids)return;stages_[active_].EndBank();active_=MaxSids;}
};
}
