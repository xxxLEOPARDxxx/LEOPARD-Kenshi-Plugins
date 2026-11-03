#include "NativeMetadata.h"
// Compiled/linked only. Never loaded or invoked by a host test.
extern "C" __declspec(dllexport) HRESULT CompileContract(const SourceReceipts::Scope*s,const SourceReceipts::WorkerRequest*r){SourceReceipts::NativeMetadata provider;HRESULT hr=provider.Init(*s);if(FAILED(hr))return hr;SourceReceipts::NamedEvidence e={r->role,SourceReceipts::OwnedNamedFile,NULL};hr=provider.Bind(e);if(FAILED(hr))return hr;SourceReceipts::MainGate<SourceReceipts::NativeMetadata>gate(provider);SourceReceipts::MainToken token;return gate.Capture(*r,*s,token);}
