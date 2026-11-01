#pragma once
#include <Windows.h>
#include <intrin.h>
// Only for the exact-profile, factory-owned private Terrain worker. Never enumerate
// or close process handles. The duplicate pins kernel identity across native teardown.
class NativeWorkerHandle
{
    typedef BOOL (WINAPI *CompareObjects)(HANDLE,HANDLE);
    HANDLE original,duplicate;
    DWORD id;
    bool joined;
    CompareObjects compare;
    NativeWorkerHandle(const NativeWorkerHandle&);
    NativeWorkerHandle &operator=(const NativeWorkerHandle&);
public:
    NativeWorkerHandle():original(NULL),duplicate(NULL),id(0),joined(false),compare(NULL){}
    ~NativeWorkerHandle(){if(duplicate)CloseHandle(duplicate);}
    bool Adopt(HANDLE handle,DWORD expectedId)
    {
        if(duplicate)return original==handle&&id==expectedId;
        HMODULE kernel=GetModuleHandleA("kernelbase.dll");
        CompareObjects equal=kernel?reinterpret_cast<CompareObjects>(GetProcAddress(kernel,"CompareObjectHandles")):NULL;
        if(!equal)return false; // This private diagnostic requires Windows10+ identity comparison.
        if(!handle||!expectedId||GetThreadId(handle)!=expectedId||
           GetProcessIdOfThread(handle)!=GetCurrentProcessId())return false;
        HANDLE copy=NULL;
        if(!DuplicateHandle(GetCurrentProcess(),handle,GetCurrentProcess(),&copy,0,FALSE,DUPLICATE_SAME_ACCESS))return false;
        if(GetThreadId(copy)!=expectedId||GetProcessIdOfThread(copy)!=GetCurrentProcessId()||!equal(handle,copy))
        {CloseHandle(copy);return false;}
        original=handle;duplicate=copy;id=expectedId;joined=false;compare=equal;return true;
    }
    bool StopAndJoin(volatile BYTE *request,DWORD timeout)
    {
        if(!duplicate||!request)return false;
        _InterlockedExchange8(reinterpret_cast<volatile CHAR*>(request),0);
        joined=WaitForSingleObject(duplicate,timeout)==WAIT_OBJECT_0;
        return joined;
    }
    // Call only after the native Terrain/stream owner has been destroyed.
    bool Complete(bool &closedNative)
    {
        closedNative=false;
        if(!duplicate||!joined||WaitForSingleObject(duplicate,0)!=WAIT_OBJECT_0)return false;
        SetLastError(ERROR_SUCCESS);
        if(compare(original,duplicate))
        {
            if(GetThreadId(original)!=id||GetProcessIdOfThread(original)!=GetCurrentProcessId()||!CloseHandle(original))return false;
            closedNative=true;
        }
        else
        {
            const DWORD error=GetLastError();
            // ERROR_NOT_SAME_OBJECT from the current Windows SDK; absent in SDK7.1.
            if(error!=ERROR_INVALID_HANDLE&&error!=1656UL)return false;
        }
        // Native code may eventually close/reuse its handle. Compare kernel object
        // identity directly; a terminated thread ID is not a future-reuse guarantee.
        if(!CloseHandle(duplicate))return false;
        original=NULL;duplicate=NULL;id=0;joined=false;return true;
    }
};
