#pragma once
#include "../fine-store/FineStore.h"
namespace FineRam {
// One file worker owns this value cache. Published reads are independent Blob
// copies, so neither LRU eviction nor terminal cleanup can reclaim Main's view.
class Cache {
 typedef FineStore::U64 U64;
 struct Node {FineStore::Blob blob;U64 hash;Node*bucketNext,*older,*newer;Node():hash(0),bucketNext(NULL),older(NULL),newer(NULL){}};
 enum {Buckets=16384};
 Node**buckets_;Node*oldest_,*newest_;SourceReceipts::Scope scope_;FineStore::Identity identity_;DWORD owner_;U64 cap_,bytes_,hits_,misses_;unsigned entries_;
 Cache(const Cache&);Cache&operator=(const Cache&);
 bool Owner()const{return owner_&&owner_==GetCurrentThreadId();}
 static U64 Hash(const FineWindows::Key&k){U64 h=1469598103934665603ULL;const U64 values[]={k.spacing,k.x,k.z,k.stamp.source,k.stamp.material};for(unsigned i=0;i<5;++i){U64 v=values[i];for(unsigned n=0;n<8;++n){h^=BYTE(v);h*=1099511628211ULL;v>>=8;}}return h;}
 Node*Find(const FineWindows::Key&k,U64 h)const{for(Node*n=buckets_[unsigned(h)&(Buckets-1)];n;n=n->bucketNext){FineStore::View v={};if(n->hash==h&&n->blob.Acquire(true,k,identity_,v))return n;}return NULL;}
 void UnlinkAge(Node*n){if(n->older)n->older->newer=n->newer;else oldest_=n->newer;if(n->newer)n->newer->older=n->older;else newest_=n->older;}
 void Touch(Node*n){if(n==newest_)return;UnlinkAge(n);n->older=newest_;n->newer=NULL;if(newest_)newest_->newer=n;else oldest_=n;newest_=n;}
 void Remove(Node*n){unsigned slot=unsigned(n->hash)&(Buckets-1);Node**at=buckets_+slot;while(*at&&*at!=n)at=&(*at)->bucketNext;if(*at)*at=n->bucketNext;UnlinkAge(n);bytes_-=EntryBytes();--entries_;delete n;}
 Node*Reserve(const FineWindows::Key&k,const FineStore::Identity&id){
  if(!Owner()||!buckets_||!FineStore::Same(identity_,id)||!FineWindows::Valid(k)||k.stamp.source!=id.sourceEpoch||k.stamp.material!=id.materialGeneration)return NULL;
  const U64 h=Hash(k);Node*old=Find(k,h);if(old)Remove(old);if(EntryBytes()>cap_||BaseBytes()>cap_-EntryBytes())return NULL;
  while(bytes_>cap_-EntryBytes()){if(!oldest_)return NULL;Remove(oldest_);}Node*n=new(std::nothrow)Node;if(n)n->hash=h;return n;
 }
 void Commit(Node*n){unsigned slot=unsigned(n->hash)&(Buckets-1);n->bucketNext=buckets_[slot];buckets_[slot]=n;n->older=newest_;if(newest_)newest_->newer=n;else oldest_=n;newest_=n;bytes_+=EntryBytes();++entries_;}
public:
 Cache():buckets_(NULL),oldest_(NULL),newest_(NULL),owner_(0),cap_(0),bytes_(0),hits_(0),misses_(0),entries_(0){std::memset(&scope_,0,sizeof(scope_));std::memset(&identity_,0,sizeof(identity_));}
 ~Cache(){Clear();}
 static U64 BaseBytes(){return sizeof(Cache)+U64(Buckets)*sizeof(Node*);}static U64 EntryBytes(){return sizeof(Node)+FineStore::PayloadBytes;}
 static U64 CapacityFor(U64 cap){return cap>=BaseBytes()?(cap-BaseBytes())/EntryBytes():0;}
 bool Begin(const SourceReceipts::Scope&s,const FineStore::Identity&id,U64 cap){
  if(owner_||!SourceReceipts::ScopeValid(s)||!FineStore::Valid(id)||s.sourceEpoch!=id.sourceEpoch||s.thread==GetCurrentThreadId()||cap>32ULL*1024*1024*1024||cap<BaseBytes())return false;
  owner_=GetCurrentThreadId();scope_=s;identity_=id;cap_=cap;buckets_=new(std::nothrow)Node*[Buckets];if(!buckets_)return false;std::memset(buckets_,0,Buckets*sizeof(Node*));bytes_=BaseBytes();return true;
 }
 void Clear(){if(owner_&&!Owner())return;while(oldest_)Remove(oldest_);delete[]buckets_;buckets_=NULL;bytes_=0;}
 U64 Bytes()const{return bytes_;}U64 Hits()const{return hits_;}U64 Misses()const{return misses_;}unsigned Entries()const{return entries_;}
 bool Remember(const FineStore::Blob&source,const FineWindows::Key&key,const FineStore::Identity&id,HANDLE cancel){if(WorldPixels::Cancelled(cancel))return false;Node*n=Reserve(key,id);if(!n)return false;if(!source.CloneTo(n->blob,FineStore::PayloadBytes,cancel)){delete n;return false;}FineStore::View view={};if(!n->blob.Acquire(true,key,id,view)){delete n;return false;}Commit(n);return true;}
 bool RememberAccepted(const FineStore::Receipt&r,const WorldPixels::Accepted&p,const SourceReceipts::Scope&s,const WorldRaster::Frame&f,const WorldHeight::Window&w,HANDLE cancel,const AcceptedWindow::Ledger&scratch){
  if(!SourceReceipts::SameScope(scope_,s)||WorldPixels::Cancelled(cancel))return false;Node*n=Reserve(r.key,r.identity);if(!n)return false;if(!n->blob.CaptureAccepted(r,p,s,f,w,cancel,scratch,FineStore::PayloadBytes)){delete n;return false;}Commit(n);return true;
 }
 HRESULT Get(const FineWindows::Key&key,const FineStore::Identity&id,HANDLE cancel,const FineStore::Ledger&scratch,FineStore::Blob&out){
  out.Clear();if(!Owner()||!buckets_||!FineStore::Same(identity_,id))return S_FALSE;if(WorldPixels::Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  Node*n=Find(key,Hash(key));if(!n){++misses_;return S_FALSE;}
  U64 left=SourceReceipts::CpuCap;const U64 charges[]={FineStore::Session::FixedCpuBytes(),FineStore::Session::GetCpuBytes(),scratch.other,scratch.retainedRaw};for(unsigned i=0;i<4;++i){if(charges[i]>left)return E_OUTOFMEMORY;left-=charges[i];}
  if(!n->blob.CloneTo(out,FineStore::PayloadBytes,cancel))return WorldPixels::Cancelled(cancel)?HRESULT_FROM_WIN32(ERROR_CANCELLED):E_OUTOFMEMORY;
  Touch(n);++hits_;return S_OK;
 }
};
}
