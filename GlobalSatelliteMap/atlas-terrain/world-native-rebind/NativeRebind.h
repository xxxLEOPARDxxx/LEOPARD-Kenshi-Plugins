#pragma once
#include "../world-cohort/WorldCohort.h"
#include "../world-native/StageAdapter.h"
namespace WorldNative{class NativeStage;}
namespace WorldNativeRebind{
typedef WorldBank::U64 U64;
inline volatile LONGLONG&BindingCounter(){static volatile LONGLONG value=0;return value;}
template<class Owned=WorldNative::StageAdapter<WorldNative::NativeStage> >class Session{
 DWORD owner_;Owned*provider_;bool initialized_,ready_,rebinding_,capturing_,bankActive_;U64 binding_;unsigned captureSlot_,captureInput_;
 SourceReceipts::Scope scope_;WorldRaster::Frame frame_;WorldMaterial::Plan plan_;WorldNative::SourceFactory factory_;
 WorldStream::CapturedSources nativeTokens_;SourceReceipts::MainSnapshot raw_[4][15],bank_[15];
 WorldCohort::Controller<Session<Owned> >cohort_;
 Session(const Session&);Session&operator=(const Session&);
 bool OnMain()const{return owner_==GetCurrentThreadId();}
 static void Clear(WorldStream::CapturedSources&tokens){for(unsigned s=0;s<4;++s)for(unsigned i=0;i<15;++i)tokens.token[s][i].Clear();}
 SourceReceipts::MainSnapshot Stamp(const SourceReceipts::MainSnapshot&raw)const{SourceReceipts::MainSnapshot out=raw;out.profile=binding_;return out;}
 bool Lookup(const SourceReceipts::MainSnapshot&before,unsigned&slot,unsigned&input)const{
  if(!OnMain()||!ready_||!provider_||!SourceReceipts::Valid(before.request)||!factory_.Matches(before.request))return false;
  slot=before.request.role.slot;input=WorldNative::InputIndex(before.request.role);if(slot>=factory_.StageCount()||input>=15)return false;
  return SourceReceipts::SameSnapshot(before,Stamp(raw_[slot][input]));
 }
 void Withdraw(){ready_=false;capturing_=false;binding_=0;factory_.Clear();Clear(nativeTokens_);std::memset(raw_,0,sizeof(raw_));std::memset(bank_,0,sizeof(bank_));}
public:
 // Cohort stores references only; no callback is made during its construction.
 Session():owner_(GetCurrentThreadId()),provider_(NULL),initialized_(false),ready_(false),rebinding_(false),capturing_(false),bankActive_(false),binding_(0),captureSlot_(0),captureInput_(0),cohort_(*this){std::memset(&scope_,0,sizeof(scope_));std::memset(&frame_,0,sizeof(frame_));std::memset(&plan_,0,sizeof(plan_));std::memset(raw_,0,sizeof(raw_));std::memset(bank_,0,sizeof(bank_));}
 ~Session(){cohort_.Shutdown();EndBank();delete provider_;}
 HRESULT Init(const SourceReceipts::Scope&scope){if(!OnMain())return RPC_E_WRONG_THREAD;if(initialized_)return E_UNEXPECTED;if(!SourceReceipts::ScopeValid(scope)||scope.thread!=owner_)return E_INVALIDARG;if(CpuReservation()>SourceReceipts::CpuCap)return E_OUTOFMEMORY;HRESULT hr=cohort_.Start();if(SUCCEEDED(hr))initialized_=true;return hr;}
 // Creating Main thread only. Frozen controller independently guards its methods.
 WorldCohort::Controller<Session<Owned> >&Cohort(){return cohort_;}
 U64 ExtraCpuPeakBytes()const{return sizeof(Session)-sizeof(cohort_)+2ULL*sizeof(Owned);}
 U64 CpuReservation()const{return cohort_.CpuReservation()+ExtraCpuPeakBytes();}
 U64 Binding()const{return OnMain()&&ready_?binding_:0;}
 bool ReadyFor(const WorldCohort::Job&j)const{return OnMain()&&ready_&&provider_&&SourceReceipts::SameScope(scope_,j.scope)&&WorldRaster::SameFrame(frame_,j.frame[0])&&WorldBank::EqualPlan(plan_,j.plan);}
 HRESULT RebindCapture(const WorldCohort::Job&j,WorldStream::CapturedSources&tokens){
  if(!OnMain())return RPC_E_WRONG_THREAD;Clear(tokens);
  if(!initialized_||rebinding_||bankActive_||cohort_.State()!=WorldCohort::Idle)return E_UNEXPECTED;
  if(!SourceReceipts::ScopeValid(j.scope)||j.scope.thread!=owner_)return E_INVALIDARG;
  // This is our own provider-bound controller, not an unrelated supplied bool.
  // Pump also observes an unsolicited exit before we claim a known Idle boundary.
  WorldBank::Ledger external={};HRESULT hr=cohort_.Pump(true,NULL,j.scope,external);if(FAILED(hr)||cohort_.State()!=WorldCohort::Idle)return FAILED(hr)?hr:HRESULT_FROM_WIN32(ERROR_BUSY);
  rebinding_=true;Owned*old=provider_;provider_=NULL;Withdraw();Owned*fresh=NULL;
  try{
   if(!WorldCohort::ValidJob(j))hr=E_INVALIDARG;else if(CpuReservation()>SourceReceipts::CpuCap)hr=E_OUTOFMEMORY;
   else{LONGLONG serial=InterlockedIncrement64(&BindingCounter());if(serial<=0)hr=E_ABORT;else{binding_=U64(serial);scope_=j.scope;frame_=j.frame[0];plan_=j.plan;hr=factory_.Init(frame_,plan_,scope_);}}
   if(SUCCEEDED(hr)){fresh=new(std::nothrow)Owned;if(!fresh)hr=E_OUTOFMEMORY;}
   if(SUCCEEDED(hr))hr=fresh->Init(scope_);
   if(SUCCEEDED(hr))hr=fresh->CaptureSources(frame_,plan_,scope_,nativeTokens_.token);
   if(SUCCEEDED(hr)&&fresh->CapturedStageCount()!=factory_.StageCount())hr=E_ACCESSDENIED;
   if(SUCCEEDED(hr)){
    provider_=fresh;capturing_=true;SourceReceipts::MainGate<Session<Owned> >gate(*this);
    for(captureSlot_=0;captureSlot_<factory_.StageCount()&&SUCCEEDED(hr);++captureSlot_)for(captureInput_=0;captureInput_<15&&SUCCEEDED(hr);++captureInput_){SourceReceipts::WorkerRequest r;if(!nativeTokens_.token[captureSlot_][captureInput_].Request(r))hr=E_INVALIDARG;else hr=gate.Capture(r,scope_,tokens.token[captureSlot_][captureInput_]);}
    capturing_=false;
   }
  }catch(...){hr=E_FAIL;}
  delete old;
  if(FAILED(hr)){provider_=NULL;delete fresh;Withdraw();Clear(tokens);}else{ready_=true;Clear(nativeTokens_);}
  rebinding_=false;return hr;
 }
 // The only Capture admission is this session's synchronous owned transaction.
 // Old WorkerRequests cannot recapture a token through a freshly rebound owner.
 HRESULT Capture(const SourceReceipts::WorkerRequest&r,SourceReceipts::MainSnapshot&out){
  if(!OnMain())return RPC_E_WRONG_THREAD;std::memset(&out,0,sizeof(out));if(!capturing_||!rebinding_||!provider_||!factory_.Matches(r)||r.role.slot!=captureSlot_||WorldNative::InputIndex(r.role)!=captureInput_)return E_ACCESSDENIED;
  SourceReceipts::WorkerRequest expected;if(!nativeTokens_.token[captureSlot_][captureInput_].Request(expected)||!SourceReceipts::SameRequest(r,expected))return E_ACCESSDENIED;
  SourceReceipts::MainSnapshot raw={};HRESULT hr=provider_->Capture(r,raw);if(FAILED(hr))return hr;if(!raw.profile||raw.semantics!=SourceReceipts::OwnedRawIndexedFile||!factory_.Matches(raw.request))return E_ACCESSDENIED;
  raw_[captureSlot_][captureInput_]=raw;out=Stamp(raw);return S_OK;
 }
 HRESULT Refresh(const SourceReceipts::MainSnapshot&before,SourceReceipts::MainSnapshot&out){
  if(!OnMain())return RPC_E_WRONG_THREAD;std::memset(&out,0,sizeof(out));unsigned slot=0,input=0;if(!Lookup(before,slot,input))return E_ACCESSDENIED;
  SourceReceipts::MainSnapshot fresh={};HRESULT hr=provider_->Refresh(raw_[slot][input],fresh);if(FAILED(hr))return hr;if(!SourceReceipts::SameSnapshot(raw_[slot][input],fresh))return E_ACCESSDENIED;out=Stamp(fresh);return S_OK;
 }
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*s,unsigned count,const SourceReceipts::Scope&scope){
  if(!OnMain())return RPC_E_WRONG_THREAD;if(!s||count!=15||bankActive_||rebinding_||!ready_||!SourceReceipts::SameScope(scope_,scope))return E_INVALIDARG;unsigned selected=4,mask=0;
  for(unsigned n=0;n<count;++n){unsigned slot=0,input=0;if(!Lookup(s[n],slot,input)||(n&&slot!=selected)||(mask&(1u<<input)))return E_ACCESSDENIED;selected=slot;mask|=1u<<input;bank_[n]=raw_[slot][input];}
  if(mask!=32767)return E_INVALIDARG;bankActive_=true;HRESULT hr=S_OK;try{hr=provider_->BeginBank(bank_,count,scope);}catch(...){hr=E_FAIL;}if(FAILED(hr))EndBank();return hr;
 }
 void EndBank(){if(!OnMain()||!bankActive_)return;if(provider_)provider_->EndBank();bankActive_=false;std::memset(bank_,0,sizeof(bank_));}
};
}
