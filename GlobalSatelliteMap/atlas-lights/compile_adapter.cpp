#ifndef LOCAL_LIGHT_HEADER
#define LOCAL_LIGHT_HEADER "NativeLocalLightSnapshot.h"
#endif
#include LOCAL_LIGHT_HEADER
typedef char ValuesHaveNoPointers[sizeof(NativeLocalLights::LightValue)==112?1:-1];
// Compile only; this function is never linked or invoked by local tests.
HRESULT CompileLocalLightAdapter(NativeLocalLights::NativeSnapshot&s,const void*scene,const volatile LONG*g){return s.Read(true,1,scene,true,g,1000);}
