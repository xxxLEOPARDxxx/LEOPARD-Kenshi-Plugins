#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#include <cstring>
#include <new>
#include <cctype>
namespace SourceReceipts {
typedef unsigned __int64 U64;
enum{TextLimit=1024,PathLimit=260,MaxAncestors=64,MaxRoles=16};
const U64 BlobCap=16ULL*1024*1024,CpuCap=64ULL*1024*1024;
enum RoleKind{Diffuse=1,Normal=2,PaintedColour=3,PaintedOverlay=4,Blend=5};
enum LookupKind{Exact=1,Insensitive=2};
struct Scope{DWORD thread;U64 world,sourceEpoch,job;};
struct Role{U64 asset;unsigned kind,index,slot;};
struct BasePin{U64 epoch,asset,bytes;BYTE sha[32];};
// Plain copied data only. No Ogre pointer, String, iterator or native reference.
struct WorkerRequest{Scope scope;Role role;BasePin pin;unsigned lookup;char group[TextLimit],resource[TextLimit],archive[TextLimit],archiveType[32],path[PathLimit],cwd[PathLimit],driveBasis[PathLimit];};
inline size_t Length(const char*p,size_t cap){size_t n=0;while(n<cap&&p[n])++n;return n;}
inline bool Ascii(const char*p,size_t cap,bool empty=false){size_t n=Length(p,cap);if(n==cap||(!empty&&!n))return false;for(size_t i=0;i<n;++i)if((unsigned char)p[i]<32||(unsigned char)p[i]>126)return false;return true;}
inline bool Copy(char*out,size_t cap,const char*in){size_t n=Length(in,cap);if(n>=cap)return false;std::memcpy(out,in,n+1);return true;}
inline bool ScopeValid(const Scope&s){return s.thread&&s.world&&s.sourceEpoch&&s.job;}
inline bool SameScope(const Scope&a,const Scope&b){return a.thread==b.thread&&a.world==b.world&&a.sourceEpoch==b.sourceEpoch&&a.job==b.job;}
inline bool SameRole(const Role&a,const Role&b){return a.asset==b.asset&&a.kind==b.kind&&a.index==b.index&&a.slot==b.slot;}
inline bool RoleValid(const Role&r){return r.asset&&r.slot<4&&r.kind>=Diffuse&&r.kind<=Blend&&r.index<((r.kind==Diffuse||r.kind==Normal)?6u:1u);}
inline bool Hash(const BYTE*b,size_t n,BYTE(&out)[32]){HCRYPTPROV p=0;HCRYPTHASH h=0;DWORD size=32;bool ok=CryptAcquireContextW(&p,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)!=FALSE;if(ok)ok=CryptCreateHash(p,CALG_SHA_256,0,0,&h)!=FALSE;if(ok)ok=n<=MAXDWORD&&CryptHashData(h,b,DWORD(n),0)!=FALSE;if(ok)ok=CryptGetHashParam(h,HP_HASHVAL,out,&size,0)!=FALSE&&size==32;if(h)CryptDestroyHash(h);if(p)CryptReleaseContext(p,0);return ok;}
inline bool JoinNative(const char*root,const char*name,char(&out)[TextLimit]){if(!Ascii(root,TextLimit,true)||!Ascii(name,TextLimit))return false;size_t a=Length(root,TextLimit),b=Length(name,TextLimit);bool drive=b>1&&((name[0]>='A'&&name[0]<='Z')||(name[0]>='a'&&name[0]<='z'))&&name[1]==':';if(!a||name[0]=='/'||name[0]=='\\'||drive)return Copy(out,TextLimit,name);if(a+b+2>TextLimit)return false;std::memcpy(out,root,a);out[a]='/';std::memcpy(out+a+1,name,b+1);return true;}
inline bool CanonicalPath(const char*p){if(!Ascii(p,PathLimit)||Length(p,PathLimit)<4||p[1]!=':'||p[2]!='\\'||!((p[0]>='A'&&p[0]<='Z')||(p[0]>='a'&&p[0]<='z')))return false;unsigned count=0;size_t start=3,n=Length(p,PathLimit);for(size_t i=3;i<=n;++i){unsigned char c=p[i];if(i<n&&c!='\\'){if(c=='/'||c==':'||c=='<'||c=='>'||c=='"'||c=='|'||c=='?'||c=='*')return false;continue;}size_t len=i-start;if(!len||++count>MaxAncestors-1||p[i-1]==' '||p[i-1]=='.')return false;char stem[8]={};size_t k=0;while(k<len&&k<7&&p[start+k]!='.'){char ch=p[start+k];stem[k]=(ch>='a'&&ch<='z')?char(ch-32):ch;++k;}if(!std::strcmp(stem,"CON")||!std::strcmp(stem,"PRN")||!std::strcmp(stem,"AUX")||!std::strcmp(stem,"NUL")||((!std::strncmp(stem,"COM",3)||!std::strncmp(stem,"LPT",3))&&stem[3]>='1'&&stem[3]<='9'&&!stem[4]))return false;start=i+1;}return true;}
inline bool NarrowAscii(const WCHAR*in,char*out,size_t cap){size_t n=0;while(n<cap&&in[n]){if(in[n]<32||in[n]>126)return false;out[n]=char(in[n]);++n;}if(n==cap)return false;out[n]=0;return true;}
inline bool ResolvePath(WorkerRequest&r){char joined[TextLimit],after[PathLimit];WCHAR input[TextLimit],wide[PathLimit];if(!JoinNative(r.archive,r.resource,joined))return false;size_t length=Length(joined,TextLimit);for(size_t i=0;i<=length;++i)input[i]=WCHAR(static_cast<unsigned char>(joined[i]));DWORD n=GetCurrentDirectoryW(PathLimit,wide);if(!n||n>=PathLimit||!NarrowAscii(wide,r.cwd,PathLimit))return false;r.driveBasis[0]=0;WCHAR drive[4]={};if(joined[0]&&joined[1]==':'&&joined[2]!='/'&&joined[2]!='\\'){drive[0]=joined[0];drive[1]=':';drive[2]='.';n=GetFullPathNameW(drive,PathLimit,wide,NULL);if(!n||n>=PathLimit||!NarrowAscii(wide,r.driveBasis,PathLimit))return false;}n=GetFullPathNameW(input,PathLimit,wide,NULL);if(!n||n>=PathLimit||!NarrowAscii(wide,r.path,PathLimit)||!CanonicalPath(r.path))return false;n=GetCurrentDirectoryW(PathLimit,wide);if(!n||n>=PathLimit||!NarrowAscii(wide,after,PathLimit)||std::strcmp(after,r.cwd))return false;if(drive[0]){n=GetFullPathNameW(drive,PathLimit,wide,NULL);if(!n||n>=PathLimit||!NarrowAscii(wide,after,PathLimit)||std::strcmp(after,r.driveBasis))return false;}return true;}
inline bool ValidNamed(const WorkerRequest&r){BYTE nonzero=0;for(unsigned i=0;i<32;++i)nonzero|=r.pin.sha[i];return ScopeValid(r.scope)&&RoleValid(r.role)&&r.pin.epoch==r.scope.sourceEpoch&&r.pin.asset==r.role.asset&&r.pin.bytes&&r.pin.bytes<=BlobCap&&nonzero&&Ascii(r.group,TextLimit)&&Ascii(r.resource,TextLimit);}
inline bool Valid(const WorkerRequest&r){return ValidNamed(r)&&(r.lookup==Exact||r.lookup==Insensitive)&&Ascii(r.archive,TextLimit,true)&&Ascii(r.archiveType,sizeof(r.archiveType))&&!std::strcmp(r.archiveType,"FileSystem")&&CanonicalPath(r.path)&&Ascii(r.cwd,PathLimit)&&Ascii(r.driveBasis,PathLimit,true);}
inline bool SameRequest(const WorkerRequest&a,const WorkerRequest&b){return SameScope(a.scope,b.scope)&&SameRole(a.role,b.role)&&a.pin.epoch==b.pin.epoch&&a.pin.asset==b.pin.asset&&a.pin.bytes==b.pin.bytes&&!std::memcmp(a.pin.sha,b.pin.sha,32)&&a.lookup==b.lookup&&!std::strcmp(a.group,b.group)&&!std::strcmp(a.resource,b.resource)&&!std::strcmp(a.archive,b.archive)&&!std::strcmp(a.archiveType,b.archiveType)&&!std::strcmp(a.path,b.path)&&!std::strcmp(a.cwd,b.cwd)&&!std::strcmp(a.driveBasis,b.driveBasis);}
template<class Map,class String>typename Map::mapped_type IndexedWinner(const Map&exact,const Map&insensitive,const String&name,unsigned&kind){typename Map::const_iterator it=exact.find(name);kind=Exact;if(it!=exact.end())return it->second;String lower(name);for(size_t i=0;i<lower.size();++i)lower[i]=char(::tolower(static_cast<unsigned char>(lower[i])));kind=Insensitive;it=insensitive.find(lower);return it==insensitive.end()?typename Map::mapped_type():it->second;}
inline HRESULT LastErrorHr(){DWORD e=GetLastError();return HRESULT_FROM_WIN32(e?e:ERROR_INVALID_DATA);}
}
