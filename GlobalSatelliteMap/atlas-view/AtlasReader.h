#pragma once
// Worker-only file/crypto routine. No Ogre, game, UI, D3D, or global state.
// Caller owns exactly ONE outstanding request; cancel+join/poll its worker before
// freeing this object. Root's existing validation-job lifecycle owns the thread.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#include <vector>
#include <string>
#include "AtlasPolicy.h"
namespace AtlasView {
struct ReadRequest {
    std::wstring path;
    unsigned tileX,tileZ,level;
    unsigned char expectedHeader[32]; // REQUIRED pinned ready.json headerSha256.
    volatile LONG cancel;
    DWORD error;
    U64 readBytes;
    void (*afterChunk)(void*);void*observer; // optional worker-only progress/test observer
    std::vector<unsigned char> plane[3];
    ReadRequest():tileX(0),tileZ(0),level(6),cancel(0),error(ERROR_SUCCESS),readBytes(0),afterChunk(NULL),observer(NULL){std::memset(expectedHeader,0,32);}
    void Clear(){for(unsigned p=0;p<3;++p)std::vector<unsigned char>().swap(plane[p]);}
private:ReadRequest(const ReadRequest&);ReadRequest&operator=(const ReadRequest&);
};
class Hash {
    HCRYPTPROV provider_;HCRYPTHASH hash_;
    Hash(const Hash&);Hash&operator=(const Hash&);
public:
    Hash():provider_(0),hash_(0){}
    ~Hash(){if(hash_)CryptDestroyHash(hash_);if(provider_)CryptReleaseContext(provider_,0);}
    bool Begin(){return !provider_&&CryptAcquireContextW(&provider_,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider_,CALG_SHA_256,0,0,&hash_);}
    bool Add(const void*p,DWORD n){return CryptHashData(hash_,static_cast<const BYTE*>(p),n,0)!=FALSE;}
    bool Finish(unsigned char out[32]){DWORD n=32;return CryptGetHashParam(hash_,HP_HASHVAL,out,&n,0)&&n==32;}
};
inline bool Digest(const void*p,DWORD n,unsigned char out[32]){Hash h;return h.Begin()&&h.Add(p,n)&&h.Finish(out);}
inline bool Cancelled(ReadRequest &r){if(InterlockedCompareExchange(&r.cancel,0,0)){r.error=ERROR_CANCELLED;return true;}return false;}
inline bool ReadAt(HANDLE f,U64 offset,unsigned char*out,DWORD n,ReadRequest&r,Hash *hash){
    LARGE_INTEGER pos;pos.QuadPart=static_cast<LONGLONG>(offset);
    if(!SetFilePointerEx(f,pos,NULL,FILE_BEGIN)){r.error=GetLastError();return false;}
    while(n){if(Cancelled(r))return false;DWORD take=n<65536?n:65536,done=0;if(!ReadFile(f,out,take,&done,NULL)){r.error=GetLastError();return false;}if(done!=take){r.error=ERROR_HANDLE_EOF;return false;}r.readBytes+=done;if(hash&&!hash->Add(out,done)){r.error=ERROR_CRC;return false;}n-=done;out+=done;if(r.afterChunk)r.afterChunk(r.observer);}
    return true;
}
inline bool ReadLevel(ReadRequest&r){
    r.Clear();r.error=ERROR_SUCCESS;r.readBytes=0;
    if(r.level<2||r.level>=6||r.tileX>=4||r.tileZ>=4||!Nonzero(r.expectedHeader,32)){r.error=ERROR_INVALID_PARAMETER;return false;}
    if(Cancelled(r))return false;
    // Deny write/delete sharing throughout header+data read, eliminating the
    // parse/hash/upload race. Offline builder replaces complete files atomically.
    HANDLE f=CreateFileW(r.path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(f==INVALID_HANDLE_VALUE){r.error=GetLastError();return false;}
    bool ok=false;
    try{
        LARGE_INTEGER length;unsigned char header[1024],actual[32];
        if(!GetFileSizeEx(f,&length)){r.error=GetLastError();}
        else if(!ReadAt(f,0,header,1024,r,NULL)){}
        else if(!ValidHeader(header,static_cast<U64>(length.QuadPart),r.tileX,r.tileZ)||!Digest(header,1024,actual)||std::memcmp(actual,r.expectedHeader,32)){r.error=ERROR_INVALID_DATA;}
        else{
            ok=true;
            for(unsigned p=0;p<3&&ok;++p){
                if(Cancelled(r)){ok=false;break;}
                r.plane[p].resize(static_cast<size_t>(PlaneBytes(r.level,p)));
                Hash h;
                if(!h.Begin()){r.error=ERROR_CRC;ok=false;break;}
                ok=ReadAt(f,PlaneOffset(r.level,p),&r.plane[p][0],static_cast<DWORD>(r.plane[p].size()),r,&h);
                if(ok&&(!h.Finish(actual)||std::memcmp(actual,header+128+(r.level*3+p)*48+16,32))){r.error=ERROR_CRC;ok=false;}
            }
            if(ok&&Cancelled(r))ok=false;
        }
    }catch(...){r.error=ERROR_NOT_ENOUGH_MEMORY;ok=false;}
    if(!CloseHandle(f)&&ok){r.error=GetLastError();ok=false;}
    if(!ok)r.Clear();
    return ok;
}
// Return code is a Win32 error. Never call this on the render/UI thread.
inline DWORD WINAPI ReadWorker(void *request){ReadRequest&r=*static_cast<ReadRequest*>(request);return ReadLevel(r)?ERROR_SUCCESS:r.error;}
}
