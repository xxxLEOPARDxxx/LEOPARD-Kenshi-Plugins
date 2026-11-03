#include "NativeProvider.h"
// Compile/link only. This DLL is never loaded or executed by the host runner.
extern "C" __declspec(dllexport) HRESULT CompileWorldNative(const WorldRaster::Frame*frame,const WorldMaterial::Plan*plan,const SourceReceipts::Scope*scope){
 struct Tokens{SourceReceipts::MainToken token[4][15];};WorldNative::Provider*provider=new WorldNative::Provider;Tokens*tokens=new Tokens;SourceReceipts::MainSnapshot*bank=new SourceReceipts::MainSnapshot[15];HRESULT hr=provider->Init(*scope);if(SUCCEEDED(hr))hr=provider->CaptureSources(*frame,*plan,*scope,tokens->token);SourceReceipts::MainSnapshot fresh={};
 for(unsigned i=0;i<15&&SUCCEEDED(hr);++i){SourceReceipts::WorkerRequest r={};if(!tokens->token[0][i].Request(r))hr=E_FAIL;else{hr=provider->Capture(r,bank[i]);if(SUCCEEDED(hr))hr=provider->Refresh(bank[i],fresh);}}
 if(SUCCEEDED(hr))hr=provider->BeginBank(bank,15,*scope);provider->EndBank();delete[]bank;delete tokens;delete provider;return hr;
}
