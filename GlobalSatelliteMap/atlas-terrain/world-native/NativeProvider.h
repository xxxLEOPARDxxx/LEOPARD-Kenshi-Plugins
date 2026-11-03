#pragma once
#include "StageAdapter.h"
#include "../source-receipts/NativeMetadata.h"
namespace WorldNative {
class NativeStage{
 SourceReceipts::NativeMetadata metadata_;NativeStage(const NativeStage&);NativeStage&operator=(const NativeStage&);
public:
 NativeStage():metadata_(SourceReceipts::OwnedRawIndexedFile){}
 HRESULT Init(const SourceReceipts::Scope&s){return metadata_.Init(s);}
 HRESULT BindOwned(const SourceReceipts::Role&r){SourceReceipts::NamedEvidence e={r,SourceReceipts::OwnedNamedFile,NULL};return metadata_.Bind(e);}
 HRESULT Capture(const SourceReceipts::WorkerRequest&r,SourceReceipts::MainSnapshot&out){return metadata_.Capture(r,out);}
 HRESULT Refresh(const SourceReceipts::MainSnapshot&before,SourceReceipts::MainSnapshot&out){return metadata_.Refresh(before,out);}
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*s,unsigned n,const SourceReceipts::Scope&scope){return metadata_.BeginBank(s,n,scope);}
 void EndBank(){metadata_.EndBank();}
};
typedef StageAdapter<NativeStage> Provider;
}
