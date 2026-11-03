#include <Windows.h>
#include <ItemMarkersApi.h>
extern "C" __declspec(dllexport) int __cdecl Provider(const IM_ItemView*,IM_Appearance* a,void* context) {
    if(context==reinterpret_cast<void*>(1))throw 23;
    if(context==reinterpret_cast<void*>(2))RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0);
    a->size=sizeof(*a);a->rank=4;a->rgb=0x123456;return 1;
}
