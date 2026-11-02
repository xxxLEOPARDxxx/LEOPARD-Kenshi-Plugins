#ifndef PRIVATE_LOD_NATIVE_HEADER
#define PRIVATE_LOD_NATIVE_HEADER "NativePrivateLod.h"
#endif
#include PRIVATE_LOD_NATIVE_HEADER
HRESULT CompilePrivateLod(PrivateTownLod::Snapshot&s,const void*b,SIZE_T n,unsigned id,unsigned generation,const float*w,const float*o,ID3D11Device*d,bool verified,AtlasView::U64 other){return s.Read(true,b,n,id,generation,w,o,d,verified,other);}
