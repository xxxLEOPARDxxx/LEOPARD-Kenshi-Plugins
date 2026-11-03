#include "NativeAdapter.h"
extern "C" __declspec(dllexport) bool MaterialCaptureContract(const SourceReceipts::Scope*s,unsigned region,const char*root,NativeRegionMaterial::U64 generation,NativeRegionMaterial::Snapshot*out){return s&&out&&NativeRegionMaterial::Capture(*s,region,root,generation,false,*out);}
