#pragma once
#include "WorkerFileLease.h"
namespace SourceReceipts {
// These native identity tokens stay on Main, never in WorkerRequest.
struct MainSnapshot{U64 manager,group,archive,module,profile;WorkerRequest request;};
class MainToken{
 template<class Provider>friend class MainGate;
 MainSnapshot snapshot_;bool valid_;
public:
 MainToken():valid_(false){std::memset(&snapshot_,0,sizeof(snapshot_));}
 void Clear(){valid_=false;std::memset(&snapshot_,0,sizeof(snapshot_));}
 bool Request(WorkerRequest&out)const{if(!valid_)return false;out=snapshot_.request;return true;}
};
// A sealed source acceptance summary, not an assertion of successful GPU upload.
// It survives release of the one pending blob/handle lease.
class AcceptedReceipt{
 template<class Provider>friend class MainGate;
 MainSnapshot snapshot_;FileStamp stamp_;bool valid_;
public:
 AcceptedReceipt():valid_(false){std::memset(&snapshot_,0,sizeof(snapshot_));std::memset(&stamp_,0,sizeof(stamp_));}
 void Clear(){valid_=false;std::memset(&snapshot_,0,sizeof(snapshot_));std::memset(&stamp_,0,sizeof(stamp_));}
 bool ValidFor(const Scope&now)const{return valid_&&SameScope(snapshot_.request.scope,now);}
};
inline bool SameSnapshot(const MainSnapshot&a,const MainSnapshot&b){return a.manager==b.manager&&a.group==b.group&&a.archive==b.archive&&a.module==b.module&&a.profile==b.profile&&SameRequest(a.request,b.request);}
template<class Provider>class MainGate{
 Provider&provider_;
 struct BankScope{Provider&p;BankScope(Provider&x):p(x){}~BankScope(){p.EndBank();}};
 static bool Current(const Scope&expected,const Scope&now){return ScopeValid(expected)&&SameScope(expected,now)&&GetCurrentThreadId()==now.thread;}
 HRESULT Fresh(const MainSnapshot&before,const FileStamp&stamp,const Scope&now){
  if(!Current(before.request.scope,now)||!Valid(before.request)||stamp.size!=before.request.pin.bytes||!stamp.id||!Ascii(stamp.finalPath,TextLimit))return E_INVALIDARG;
  MainSnapshot fresh={};HRESULT hr=provider_.Refresh(before,fresh);
  if(FAILED(hr))return hr;
  return SameSnapshot(before,fresh)?S_OK:E_ACCESSDENIED;
 }
public:
 explicit MainGate(Provider&p):provider_(p){}
 HRESULT Capture(const WorkerRequest&request,const Scope&now,MainToken&out){
  out.Clear();if(!ValidNamed(request)||!Current(request.scope,now))return E_INVALIDARG;
  MainSnapshot snapshot={};HRESULT hr=provider_.Capture(request,snapshot);
  if(FAILED(hr))return hr;
  if(!snapshot.manager||!snapshot.group||!snapshot.archive||!snapshot.module||!snapshot.profile||!Valid(snapshot.request)||!SameScope(snapshot.request.scope,now)||!SameRole(snapshot.request.role,request.role)||snapshot.request.pin.epoch!=request.pin.epoch||snapshot.request.pin.asset!=request.pin.asset||snapshot.request.pin.bytes!=request.pin.bytes||std::memcmp(snapshot.request.pin.sha,request.pin.sha,32)||std::strcmp(snapshot.request.group,request.group)||std::strcmp(snapshot.request.resource,request.resource))return E_INVALIDARG;
  out.snapshot_=snapshot;out.valid_=true;return S_OK;
 }
 HRESULT Accept(const MainToken&t,WorkerFileLease&lease,const Scope&now,AcceptedReceipt&out){
  out.Clear();lease.accepted_=false;
  if(!t.valid_||!lease.Ready()||!SameRequest(t.snapshot_.request,lease.Request()))return E_INVALIDARG;
  HRESULT hr=Fresh(t.snapshot_,lease.Stamp(),now);if(FAILED(hr))return hr;
  out.snapshot_=t.snapshot_;out.stamp_=lease.Stamp();out.valid_=true;lease.accepted_=true;return S_OK;
 }
 HRESULT CommitBank(const AcceptedReceipt*receipts,unsigned count,const Scope&now){
  if(!receipts||!count||count>MaxRoles)return E_INVALIDARG;
  MainSnapshot snapshots[MaxRoles];
  for(unsigned i=0;i<count;++i){
   if(!receipts[i].valid_||!Current(receipts[i].snapshot_.request.scope,now))return E_INVALIDARG;
   for(unsigned j=0;j<i;++j){const Role&a=receipts[i].snapshot_.request.role,&b=receipts[j].snapshot_.request.role;if(a.kind==b.kind&&a.index==b.index&&a.slot==b.slot)return E_INVALIDARG;}
   snapshots[i]=receipts[i].snapshot_;
  }
  HRESULT hr=provider_.BeginBank(snapshots,count,now);if(FAILED(hr))return hr;
  BankScope bank(provider_);
  for(unsigned i=0;i<count;++i){hr=Fresh(receipts[i].snapshot_,receipts[i].stamp_,now);if(FAILED(hr))return hr;}
  return S_OK;
 }
};
}
