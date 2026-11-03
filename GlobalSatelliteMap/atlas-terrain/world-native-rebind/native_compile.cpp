#include "NativeSession.h"
// COMPILE/LINK ONLY. Never load/execute this DLL in a host process.
extern "C" __declspec(dllexport) HRESULT CompileNativeRebind(const WorldCohort::Job*j){WorldNativeRebind::NativeSession*s=new WorldNativeRebind::NativeSession;WorldStream::CapturedSources*t=new WorldStream::CapturedSources;HRESULT hr=s->Init(j->scope);if(SUCCEEDED(hr))hr=s->RebindCapture(*j,*t);delete t;delete s;return hr;}
