#pragma once
#include "MainGate.h"
#include <ogre/OgreResourceGroupManager.h>
#include <ogre/OgreResource.h>
#include <cstddef>
#include <cctype>
#include "_generated/NativePins.h"
namespace SourceReceipts {
// Legal protected-member exposure. No derived object/downcast is manufactured.
struct GroupAccess:Ogre::ResourceGroupManager{
 typedef ResourceGroup Group;
 typedef Group*(Ogre::ResourceGroupManager::*Getter)(const Ogre::String&);
 static Getter Method(){return &GroupAccess::getResourceGroup;}
 static bool Layout(){return sizeof(Ogre::String)==40&&sizeof(boost::recursive_mutex)==24&&offsetof(GroupAccess,mLoadingListener)==0x98&&offsetof(GroupAccess,mResourceGroupMap)==0xa0&&offsetof(Group,name)==0x30&&offsetof(Group,locationList)==0x60&&offsetof(Group,resourceIndexCaseSensitive)==0x80&&offsetof(Group,resourceIndexCaseInsensitive)==0xa8;}
};
struct ArchiveAccess:Ogre::Archive{static bool Layout(){return offsetof(ArchiveAccess,mName)==8&&offsetof(ArchiveAccess,mType)==0x30;}};
struct ResourceAccess:Ogre::Resource{static bool Layout(){return offsetof(ResourceAccess,mName)==0x60&&offsetof(ResourceAccess,mGroup)==0x88&&offsetof(ResourceAccess,mIsManual)==0xe8;}};
inline bool ReadMemory(const void*p,void*out,size_t n){SIZE_T got=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,n,&got)&&got==n;}
template<class T>inline bool ObjectReadable(const T*p){BYTE bytes[sizeof(T)];return ReadMemory(p,bytes,sizeof(bytes));}
inline bool NativeString(const Ogre::String&s,char*out,size_t cap,bool empty=false){BYTE raw[40];if(!ReadMemory(&s,raw,sizeof(raw)))return false;U64 n=0,c=0;std::memcpy(&n,raw+16,8);std::memcpy(&c,raw+24,8);if(n>=cap||n>c||c>0x1000000)return false;const void*data=&s;if(c>=16)std::memcpy(&data,raw,8);if(!ReadMemory(data,out,size_t(n)+1)||out[n])return false;return Ascii(out,cap,empty);}
class TryLock{
 boost::recursive_mutex*mutex_;TryLock(const TryLock&);TryLock&operator=(const TryLock&);
public:TryLock():mutex_(NULL){}~TryLock(){Clear();}bool Lock(boost::recursive_mutex&m){if(mutex_)return false;if(!m.try_lock())return false;mutex_=&m;return true;}void Clear(){if(mutex_){mutex_->unlock();mutex_=NULL;}}
};
class NativeProfile{
 HMODULE module_;DWORD thread_;HCRYPTPROV crypto_;NativeProfile(const NativeProfile&);NativeProfile&operator=(const NativeProfile&);
 bool MemberImport()const{GroupAccess::Getter method=GroupAccess::Method();BYTE repr[32]={};if(sizeof(method)>sizeof(repr))return false;std::memcpy(repr,&method,sizeof(method));if(sizeof(method)!=8&&sizeof(method)!=16)return false;if(sizeof(method)==16){LONG adjust=0;std::memcpy(&adjust,repr+8,4);if(adjust)return false;}const BYTE*entry=NULL;std::memcpy(&entry,repr,sizeof(entry));FARPROC expected=GetProcAddress(module_,GroupExport);if(entry==reinterpret_cast<const BYTE*>(expected))return true;BYTE code[6];if(!ReadMemory(entry,code,6)||code[0]!=0xff||code[1]!=0x25)return false;LONG rel=0;std::memcpy(&rel,code+2,4);FARPROC actual=NULL;return ReadMemory(entry+6+rel,&actual,sizeof(actual))&&actual==expected;}
 bool AllocatorImport()const{FARPROC expected=GetProcAddress(module_,"?deallocBytes@NedPoolingImpl@Ogre@@SAXPEAX@Z");if(expected!=reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(module_)+0x1a1e60))return false;const BYTE*entry=reinterpret_cast<const BYTE*>(&Ogre::NedPoolingImpl::deallocBytes);if(entry==reinterpret_cast<const BYTE*>(expected))return true;BYTE code[6];if(!ReadMemory(entry,code,6)||code[0]!=0xff||code[1]!=0x25)return false;LONG rel=0;std::memcpy(&rel,code+2,4);FARPROC actual=NULL;return ReadMemory(entry+6+rel,&actual,sizeof(actual))&&actual==expected;}
public:
 NativeProfile():module_(NULL),thread_(0),crypto_(0){}~NativeProfile(){Clear();}
 void Clear(){if(crypto_)CryptReleaseContext(crypto_,0);crypto_=0;if(module_)FreeLibrary(module_);module_=NULL;thread_=0;}
 bool Digest(const BYTE*b,DWORD n,BYTE(&hash)[32])const{HCRYPTHASH h=0;DWORD size=32;bool ok=crypto_&&CryptCreateHash(crypto_,CALG_SHA_256,0,0,&h)!=FALSE;if(ok)ok=CryptHashData(h,b,n,0)!=FALSE;if(ok)ok=CryptGetHashParam(h,HP_HASHVAL,hash,&size,0)!=FALSE&&size==32;if(h)CryptDestroyHash(h);return ok;}
 HRESULT Init(){if(module_)return E_UNEXPECTED;if(!GroupAccess::Layout()||!ArchiveAccess::Layout()||!ResourceAccess::Layout())return E_NOINTERFACE;HMODULE found=GetModuleHandleW(L"OgreMain_x64.dll");if(!found||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCWSTR>(found),&module_))return E_NOINTERFACE;thread_=GetCurrentThreadId();if(!CryptAcquireContextW(&crypto_,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)||!Valid()){Clear();return E_NOINTERFACE;}return S_OK;}
 bool Valid()const{if(!module_||GetCurrentThreadId()!=thread_||GetModuleHandleW(L"OgreMain_x64.dll")!=module_)return false;IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS64 nt;if(!ReadMemory(module_,&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0||dos.e_lfanew>4096||!ReadMemory(reinterpret_cast<const BYTE*>(module_)+dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.FileHeader.TimeDateStamp!=NativeTimestamp||nt.OptionalHeader.SizeOfImage!=NativeImageSize||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;BYTE code[4096],hash[32];for(unsigned i=0;i<sizeof(NativeCodePins)/sizeof(NativeCodePins[0]);++i){const CodePin&p=NativeCodePins[i];if(p.bytes>sizeof(code)||p.rva>NativeImageSize-p.bytes||!ReadMemory(reinterpret_cast<const BYTE*>(module_)+p.rva,code,p.bytes)||!Digest(code,p.bytes,hash)||std::memcmp(hash,p.sha,32))return false;}
  if(GetProcAddress(module_,GroupExport)!=reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(module_)+0x282720)||GetProcAddress(module_,SingletonExport)!=reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(module_)+0x27b6c0)||GetProcAddress(module_,ListenerExport)!=reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(module_)+0x286990)||GetProcAddress(module_,"??_7FileSystemArchive@Ogre@@6B@")!=reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(module_)+0x682dc8)||!MemberImport()||!AllocatorImport())return false;
  const void*entries[14];if(!ReadMemory(reinterpret_cast<const BYTE*>(module_)+0x682dc8,entries,sizeof(entries)))return false;for(unsigned i=0;i<14;++i)if(entries[i]!=reinterpret_cast<const BYTE*>(module_)+NativeFsVtable[i])return false;return true;
 }
 Ogre::ResourceGroupManager*Manager()const{Ogre::ResourceGroupManager*p=NULL;return ReadMemory(reinterpret_cast<const BYTE*>(module_)+0x909b18,&p,sizeof(p))?p:NULL;}
 bool FileSystem(const Ogre::Archive*a)const{const void*v=NULL;return ReadMemory(a,&v,sizeof(v))&&v==reinterpret_cast<const BYTE*>(module_)+0x682dc8;}
 U64 Module()const{return reinterpret_cast<U64>(module_);}DWORD Thread()const{return thread_;}
};
enum NamedEvidenceKind{UnknownSource=0,ObservedNativeResource=1,OwnedNamedFile=2};
// Main-only evidence, never serialized to worker. OwnedNamedFile describes a
// new named-file input; it does not assert equality to historical GPU contents.
struct NamedEvidence{Role role;NamedEvidenceKind kind;const Ogre::Resource*resource;};
class NativeMetadata{
 SourceSemantics semantics_;Scope scope_;NativeProfile profile_;NamedEvidence evidence_[MaxRoles];Ogre::ResourcePtr retained_[MaxRoles];unsigned count_;TryLock bankManager_,bankGroups_[MaxRoles];bool bankActive_;NativeMetadata(const NativeMetadata&);NativeMetadata&operator=(const NativeMetadata&);
 bool Listener(Ogre::ResourceGroupManager*m,U64&value)const{void*p=NULL;if(!ReadMemory(reinterpret_cast<BYTE*>(m)+0x98,&p,sizeof(p)))return false;value=reinterpret_cast<U64>(p);return ListenerPolicyAllows(semantics_,value);}
 bool Evidence(const WorkerRequest&r)const{for(unsigned i=0;i<count_;++i)if(SameRole(r.role,evidence_[i].role)){const NamedEvidence&e=evidence_[i];if(e.kind==OwnedNamedFile)return e.resource==NULL;if(e.kind!=ObservedNativeResource||!e.resource)return false;BYTE manual=1;char name[TextLimit],group[TextLimit];return ReadMemory(reinterpret_cast<const BYTE*>(e.resource)+0xe8,&manual,1)&&!manual&&NativeString(*reinterpret_cast<const Ogre::String*>(reinterpret_cast<const BYTE*>(e.resource)+0x60),name,TextLimit)&&NativeString(*reinterpret_cast<const Ogre::String*>(reinterpret_cast<const BYTE*>(e.resource)+0x88),group,TextLimit)&&!std::strcmp(name,r.resource)&&!std::strcmp(group,r.group);}return false;}
 HRESULT Snapshot(const WorkerRequest&r,MainSnapshot&out){if(!ValidNamed(r)||!SameScope(r.scope,scope_)||GetCurrentThreadId()!=scope_.thread||!profile_.Valid()||!Evidence(r))return E_ACCESSDENIED;Ogre::ResourceGroupManager*m=profile_.Manager();if(!m)return E_ACCESSDENIED;BYTE managerBytes[sizeof(Ogre::ResourceGroupManager)];if(!ReadMemory(m,managerBytes,sizeof(managerBytes)))return E_ACCESSDENIED;TryLock manager;if(!manager.Lock(m->OGRE_AUTO_MUTEX_NAME))return HRESULT_FROM_WIN32(ERROR_BUSY);U64 listener=0;if(!Listener(m,listener))return E_ACCESSDENIED;Ogre::String groupName(r.group),resourceName(r.resource);GroupAccess::Group*g=(m->*GroupAccess::Method())(groupName);if(!g)return E_INVALIDARG;BYTE groupBytes[sizeof(GroupAccess::Group)];if(!ReadMemory(g,groupBytes,sizeof(groupBytes)))return E_ACCESSDENIED;TryLock group;if(!group.Lock(g->OGRE_AUTO_MUTEX_NAME))return HRESULT_FROM_WIN32(ERROR_BUSY);char actualGroup[TextLimit];if(!NativeString(g->name,actualGroup,TextLimit)||std::strcmp(actualGroup,r.group))return E_ACCESSDENIED;Ogre::Archive*archive=NULL;unsigned kind=Exact;
  // The SDK's protected index type is inferred through its actual object.
  archive=IndexedWinner(g->resourceIndexCaseSensitive,g->resourceIndexCaseInsensitive,resourceName,kind);if(!archive||!profile_.FileSystem(archive))return E_ACCESSDENIED;MainSnapshot next={};next.manager=reinterpret_cast<U64>(m);next.group=reinterpret_cast<U64>(g);next.archive=reinterpret_cast<U64>(archive);next.module=profile_.Module();next.profile=0x7d3888d213986475ULL;next.listener=listener;next.semantics=semantics_;next.request=r;next.request.lookup=kind;if(!NativeString(archive->getName(),next.request.archive,TextLimit,true)||!NativeString(archive->getType(),next.request.archiveType,sizeof(next.request.archiveType))||std::strcmp(next.request.archiveType,"FileSystem"))return E_ACCESSDENIED;if(!ResolvePath(next.request))return E_INVALIDARG;
  U64 after=0;if(!Listener(m,after)||after!=listener||!Evidence(r)||!Valid(next.request))return E_ACCESSDENIED;out=next;return S_OK;
 }
 template<class Map>static Ogre::Archive*Find(const Map&map,const Ogre::String&name){typename Map::const_iterator it=map.find(name);return it==map.end()?NULL:it->second;}
public:
 static const bool LiveFrameAdapterTested=false;
 explicit NativeMetadata(SourceSemantics semantics=NativeCompatible):semantics_(semantics),count_(0),bankActive_(false){std::memset(&scope_,0,sizeof(scope_));std::memset(evidence_,0,sizeof(evidence_));}
 ~NativeMetadata(){EndBank();}
 HRESULT Init(const Scope&s){if((semantics_!=NativeCompatible&&semantics_!=OwnedRawIndexedFile)||!ScopeValid(s)||GetCurrentThreadId()!=s.thread)return E_INVALIDARG;HRESULT hr=profile_.Init();if(SUCCEEDED(hr))scope_=s;return hr;}
 HRESULT Bind(const NamedEvidence&e,const Ogre::ResourcePtr&retained=Ogre::ResourcePtr()){if(!SameThread()||!profile_.Valid()||!RoleValid(e.role)||count_>=MaxRoles||!EvidencePolicyAllows(semantics_,e.kind==OwnedNamedFile)||(e.kind!=ObservedNativeResource&&e.kind!=OwnedNamedFile)||(e.kind==OwnedNamedFile&&(e.resource||!retained.isNull()))||(e.kind==ObservedNativeResource&&(!e.resource||retained.getPointer()!=e.resource)))return E_INVALIDARG;for(unsigned i=0;i<count_;++i)if(SameRole(e.role,evidence_[i].role))return E_INVALIDARG;retained_[count_]=retained;evidence_[count_++]=e;return S_OK;}
 bool SameThread()const{return scope_.thread&&GetCurrentThreadId()==scope_.thread;}
 HRESULT Capture(const WorkerRequest&r,MainSnapshot&out){try{return Snapshot(r,out);}catch(...){return E_FAIL;}}
 HRESULT Refresh(const MainSnapshot&before,MainSnapshot&out){return Capture(before.request,out);}
 HRESULT BeginBank(const MainSnapshot*s,unsigned n,const Scope&now){if(bankActive_||!s||!n||n>MaxRoles||!SameScope(scope_,now)||!SameThread())return E_INVALIDARG;for(unsigned i=0;i<n;++i)if(!Valid(s[i].request)||!SameScope(s[i].request.scope,scope_)||s[i].semantics!=semantics_||!Evidence(s[i].request))return E_INVALIDARG;if(!profile_.Valid())return E_ACCESSDENIED;Ogre::ResourceGroupManager*m=profile_.Manager();if(!m||!ObjectReadable(m)||!bankManager_.Lock(m->OGRE_AUTO_MUTEX_NAME))return HRESULT_FROM_WIN32(ERROR_BUSY);bankActive_=true;try{for(unsigned i=0;i<n;++i){GroupAccess::Group*g=(m->*GroupAccess::Method())(Ogre::String(s[i].request.group));if(!g||!ObjectReadable(g)||!bankGroups_[i].Lock(g->OGRE_AUTO_MUTEX_NAME)){EndBank();return HRESULT_FROM_WIN32(ERROR_BUSY);}}U64 listener=0;if(!Listener(m,listener)){EndBank();return E_ACCESSDENIED;}for(unsigned i=0;i<n;++i)if(s[i].listener!=listener){EndBank();return E_ACCESSDENIED;}return S_OK;}catch(...){EndBank();return E_FAIL;}}
 void EndBank(){for(unsigned i=MaxRoles;i>0;--i)bankGroups_[i-1].Clear();bankManager_.Clear();bankActive_=false;}
};
}
