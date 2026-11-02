#include "OriginProof.h"
// Compile-only caller; never linked or invoked by tests.
HRESULT CompileOriginProof(NativeOriginProof::Reader&r,const void*scene,const void*camera,const void*town,const volatile LONG*g,NativeOriginProof::Values&out){return r.Read(true,1,scene,camera,town,true,g,out);}
