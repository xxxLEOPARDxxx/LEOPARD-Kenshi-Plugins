#pragma once
#include "../world-window-session/WorldWindowSession.h"
#include <cwchar>
#include <winternl.h>
namespace WorldBuildDisk{
typedef WorldWindow::U64 U64;
enum{MaxDirectoryChars=220,MaxGuards=112};
struct Config{wchar_t allowedParent[MaxDirectoryChars+1],directory[MaxDirectoryChars+1];};
inline unsigned Length(const wchar_t*p){unsigned n=0;if(!p)return MaxDirectoryChars+1;while(n<=MaxDirectoryChars&&p[n])++n;return n;}
inline wchar_t Upper(wchar_t c){return c>=L'a'&&c<=L'z'?c-32:c;}
inline bool Device(const wchar_t*p,unsigned n){unsigned base=0;while(base<n&&p[base]!=L'.')++base;if(base==3){wchar_t a=Upper(p[0]),b=Upper(p[1]),c=Upper(p[2]);if((a==L'C'&&b==L'O'&&c==L'N')||(a==L'P'&&b==L'R'&&c==L'N')||(a==L'A'&&b==L'U'&&c==L'X')||(a==L'N'&&b==L'U'&&c==L'L'))return true;}if(base==4){wchar_t a=Upper(p[0]),b=Upper(p[1]),c=Upper(p[2]),d=p[3];if(((a==L'C'&&b==L'O'&&c==L'M')||(a==L'L'&&b==L'P'&&c==L'T'))&&((d>=L'1'&&d<=L'9')||d==0xb9||d==0xb2||d==0xb3))return true;}return false;}
inline bool Normalize(const wchar_t*p,wchar_t(&out)[MaxDirectoryChars+1]){
 const unsigned n=Length(p);if(n<3||n>MaxDirectoryChars||Upper(p[0])<L'A'||Upper(p[0])>L'Z'||p[1]!=L':'||(p[2]!=L'\\'&&p[2]!=L'/'))return false;
 for(unsigned i=0;i<n;++i){wchar_t c=p[i];if(c<32||c==L'"'||c==L'<'||c==L'>'||c==L'|'||c==L'?'||c==L'*'||(c==L':'&&i!=1))return false;out[i]=c==L'/'?L'\\':c;}out[0]=Upper(out[0]);out[n]=0;
 if(n>3&&out[n-1]==L'\\')return false;
 for(unsigned begin=3;begin<n;){unsigned end=begin;while(end<n&&out[end]!=L'\\')++end;const unsigned count=end-begin;if(!count||out[end-1]==L'.'||out[end-1]==L' '||Device(out+begin,count))return false;begin=end+1;}return true;
}
// Pure copying/lexical validation only; safe before submitting the writer worker.
inline bool Configure(Config&out,const wchar_t*parent,const wchar_t*directory){
 Config temp={};if(!Normalize(parent,temp.allowedParent)||!Normalize(directory,temp.directory)){std::memset(&out,0,sizeof(out));return false;}
 const unsigned p=Length(temp.allowedParent),d=Length(temp.directory);const unsigned prefix=p+(p==3?0:1);
 if(d<=prefix||_wcsnicmp(temp.allowedParent,temp.directory,p)||(p>3&&temp.directory[p]!=L'\\')){std::memset(&out,0,sizeof(out));return false;}
 for(unsigned i=prefix;i<d;++i)if(temp.directory[i]==L'\\'){std::memset(&out,0,sizeof(out));return false;}out=temp;return true;
}
inline HANDLE CreateRelative(HANDLE parent,const wchar_t*leaf,bool directory){
 typedef NTSTATUS(NTAPI*Create)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
 typedef ULONG(NTAPI*Error)(NTSTATUS);HMODULE module=GetModuleHandleW(L"ntdll.dll");Create create=reinterpret_cast<Create>(GetProcAddress(module,"NtCreateFile"));Error error=reinterpret_cast<Error>(GetProcAddress(module,"RtlNtStatusToDosError"));
 if(!create||!error){SetLastError(ERROR_PROC_NOT_FOUND);return INVALID_HANDLE_VALUE;}
 UNICODE_STRING name={};name.Buffer=const_cast<wchar_t*>(leaf);name.Length=USHORT(std::wcslen(leaf)*sizeof(wchar_t));name.MaximumLength=name.Length+sizeof(wchar_t);
 OBJECT_ATTRIBUTES object={};object.Length=sizeof(object);object.RootDirectory=parent;object.ObjectName=&name;object.Attributes=OBJ_CASE_INSENSITIVE;
 HANDLE h=INVALID_HANDLE_VALUE;IO_STATUS_BLOCK io={};const ACCESS_MASK access=directory?(FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES|SYNCHRONIZE):(GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE);
 NTSTATUS status=create(&h,access,&object,&io,NULL,FILE_ATTRIBUTE_NORMAL,directory?FILE_SHARE_READ:0,FILE_CREATE,(directory?FILE_DIRECTORY_FILE:FILE_NON_DIRECTORY_FILE)|FILE_OPEN_REPARSE_POINT|FILE_SYNCHRONOUS_IO_NONALERT,NULL,0);
 if(status<0){SetLastError(error(status));return INVALID_HANDLE_VALUE;}if(io.Information!=FILE_CREATED){CloseHandle(h);SetLastError(ERROR_ALREADY_EXISTS);return INVALID_HANDLE_VALUE;}return h;
}
class Guards{
 HANDLE handles_[MaxGuards];unsigned count_;Guards(const Guards&);Guards&operator=(const Guards&);
 bool Add(HANDLE h,const wchar_t*path){
  BY_HANDLE_FILE_INFORMATION info={};wchar_t finalPath[MAX_PATH]={};const DWORD n=GetFinalPathNameByHandleW(h,finalPath,MAX_PATH,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
  bool valid=GetFileInformationByHandle(h,&info)&&(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0&&!(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)&&n>4&&n<MAX_PATH&&!std::wcsncmp(finalPath,L"\\\\?\\",4)&&!_wcsicmp(finalPath+4,path);
  if(!valid){CloseHandle(h);SetLastError(ERROR_ACCESS_DENIED);return false;}handles_[count_++]=h;return true;
 }
public:
 Guards():count_(0){}~Guards(){while(count_)CloseHandle(handles_[--count_]);}
 bool Open(const wchar_t*path){
  if(count_>=MaxGuards)return false;
  // Deny data-write/delete sharing. Metadata-only reparse mutation may still
  // succeed; HANDLE-relative exclusive creates contain that namespace race.
  HANDLE h=::CreateFileW(path,FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,NULL);if(h==INVALID_HANDLE_VALUE)return false;
  return Add(h,path);
 }
 bool CreateChild(const wchar_t*leaf,const wchar_t*absolute){
  if(!count_||count_>=MaxGuards)return false;
  HANDLE h=CreateRelative(handles_[count_-1],leaf,true);if(h==INVALID_HANDLE_VALUE)return false;
  // The NEW directory and its deny-write/delete handle were acquired atomically.
  return Add(h,absolute);
 }
 HANDLE CreateRelativeFile(const wchar_t*leaf)const{return count_?CreateRelative(handles_[count_-1],leaf,false):INVALID_HANDLE_VALUE;}
 bool Current()const{for(unsigned i=0;i<count_;++i){BY_HANDLE_FILE_INFORMATION info={};if(!GetFileInformationByHandle(handles_[i],&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))return false;}return count_!=0;}
};
class Factory;
class File:public Gst2::RandomFile{
 friend class Factory;HANDLE file_;DWORD owner_;File(const File&);File&operator=(const File&);
 explicit File(HANDLE file):file_(file),owner_(GetCurrentThreadId()){}
public:
 ~File(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);} // No auto-delete or exception.
 bool Resize(U64 n){LARGE_INTEGER at;at.QuadPart=LONGLONG(n);return owner_==GetCurrentThreadId()&&file_!=INVALID_HANDLE_VALUE&&n<=Gst2::FileBytes()&&SetFilePointerEx(file_,at,NULL,FILE_BEGIN)&&SetEndOfFile(file_);}
 bool WriteAt(U64 at,const void*p,DWORD n){
  if(owner_!=GetCurrentThreadId()||file_==INVALID_HANDLE_VALUE||!p||at>Gst2::FileBytes()||U64(n)>Gst2::FileBytes()-at)return false;LARGE_INTEGER offset;offset.QuadPart=LONGLONG(at);if(!SetFilePointerEx(file_,offset,NULL,FILE_BEGIN))return false;
  const BYTE*data=static_cast<const BYTE*>(p);while(n){DWORD done=0;if(!WriteFile(file_,data,n,&done,NULL)||!done||done>n)return false;data+=done;n-=done;}return true;
 }
 bool Flush(){return owner_==GetCurrentThreadId()&&file_!=INVALID_HANDLE_VALUE&&FlushFileBuffers(file_)!=FALSE;}
};
class Factory:public WorldWindow::Factory{
 Config config_;Guards guards_;DWORD owner_;unsigned opened_;Factory(const Factory&);Factory&operator=(const Factory&);
 static void Fail(){DWORD e=GetLastError();throw HRESULT_FROM_WIN32(e?e:ERROR_ACCESS_DENIED);}
public:
 explicit Factory(const Config&c):owner_(GetCurrentThreadId()),opened_(0){
  if(!Configure(config_,c.allowedParent,c.directory)){SetLastError(ERROR_INVALID_NAME);Fail();}
  wchar_t prefix[MaxDirectoryChars+1]={};const unsigned n=Length(config_.allowedParent);std::memcpy(prefix,config_.allowedParent,(n+1)*sizeof(wchar_t));prefix[3]=0;if(!guards_.Open(prefix))Fail();
  for(unsigned end=3;end<=n;++end)if(end==n||config_.allowedParent[end]==L'\\'){if(end==3)continue;std::memcpy(prefix,config_.allowedParent,end*sizeof(wchar_t));prefix[end]=0;if(!guards_.Open(prefix))Fail();}
  if(!guards_.Current()){SetLastError(ERROR_ACCESS_DENIED);Fail();}
  const wchar_t*leaf=config_.directory+n+(n==3?0:1);if(!guards_.CreateChild(leaf,config_.directory))Fail();
  if(!guards_.Current()){SetLastError(ERROR_ACCESS_DENIED);Fail();}
 }
 ~Factory(){} // Files must be destroyed first (WorldWindow/WorldWrite order).
 U64 FileCpuBound()const{return sizeof(File);}
 Gst2::RandomFile*OpenNew(unsigned x,unsigned z){
  if(owner_!=GetCurrentThreadId()||x>=4||z>=4){SetLastError(ERROR_INVALID_PARAMETER);return NULL;}const unsigned mask=1u<<(z*4+x);if(opened_&mask){SetLastError(ERROR_ALREADY_EXISTS);return NULL;}
  if(!guards_.Current()){SetLastError(ERROR_ACCESS_DENIED);return NULL;}wchar_t leaf[32]={};if(_snwprintf_s(leaf,32,_TRUNCATE,L"tile_%u_%u.gst.partial",x,z)<0){SetLastError(ERROR_FILENAME_EXCED_RANGE);return NULL;}
  HANDLE handle=guards_.CreateRelativeFile(leaf);if(handle==INVALID_HANDLE_VALUE)return NULL;if(!guards_.Current()){CloseHandle(handle);SetLastError(ERROR_ACCESS_DENIED);return NULL;}
  File*file=new(std::nothrow)File(handle);if(!file){CloseHandle(handle);SetLastError(ERROR_OUTOFMEMORY);return NULL;}opened_|=mask;return file;
 }
};
}
