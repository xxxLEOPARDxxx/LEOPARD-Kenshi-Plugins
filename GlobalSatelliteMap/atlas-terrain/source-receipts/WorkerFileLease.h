#pragma once
#include "ReceiptPolicy.h"
namespace SourceReceipts {
template<class Provider>class MainGate;
struct FileStamp{DWORD volume,links,attributes;U64 id,size;FILETIME creation,write;char finalPath[TextLimit];};
class WorkerFileLease{
 template<class Provider>friend class MainGate;
 HANDLE handle_[MaxAncestors];unsigned handles_;BYTE*bytes_;U64 owned_;WorkerRequest request_;FileStamp stamp_;bool ready_,accepted_;
 WorkerFileLease(const WorkerFileLease&);WorkerFileLease&operator=(const WorkerFileLease&);
 static bool Cancelled(HANDLE c){return c&&WaitForSingleObject(c,0)!=WAIT_TIMEOUT;}
 static bool EqualInfo(const BY_HANDLE_FILE_INFORMATION&a,const BY_HANDLE_FILE_INFORMATION&b){return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow&&a.nFileSizeHigh==b.nFileSizeHigh&&a.nFileSizeLow==b.nFileSizeLow&&a.dwFileAttributes==b.dwFileAttributes&&!std::memcmp(&a.ftCreationTime,&b.ftCreationTime,sizeof(FILETIME))&&!std::memcmp(&a.ftLastWriteTime,&b.ftLastWriteTime,sizeof(FILETIME));}
 HRESULT Add(const char*path,bool directory){if(handles_>=MaxAncestors)return E_INVALIDARG;HANDLE h=CreateFileA(path,FILE_READ_ATTRIBUTES|(directory?0:GENERIC_READ),FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|(directory?FILE_FLAG_BACKUP_SEMANTICS:FILE_FLAG_SEQUENTIAL_SCAN),NULL);if(h==INVALID_HANDLE_VALUE)return LastErrorHr();handle_[handles_++]=h;BY_HANDLE_FILE_INFORMATION info;if(GetFileType(h)!=FILE_TYPE_DISK||!GetFileInformationByHandle(h,&info))return LastErrorHr();if((info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||((info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)!=directory)return E_ACCESSDENIED;return S_OK;}
public:
 WorkerFileLease():handles_(0),bytes_(NULL),owned_(0),ready_(false),accepted_(false){std::memset(&request_,0,sizeof(request_));std::memset(&stamp_,0,sizeof(stamp_));}
 ~WorkerFileLease(){Clear();}
 void Clear(){accepted_=ready_=false;delete[]bytes_;bytes_=NULL;while(handles_)CloseHandle(handle_[--handles_]);owned_=0;std::memset(&request_,0,sizeof(request_));std::memset(&stamp_,0,sizeof(stamp_));}
 static U64 RequiredBytes(U64 bytes){return bytes&&bytes<=BlobCap?bytes+sizeof(WorkerFileLease):0;}
 U64 OwnedBytes()const{return owned_;}bool Ready()const{return ready_;}
 const BYTE*Data()const{return accepted_?bytes_:NULL;}
 const FileStamp&Stamp()const{return stamp_;}
 const WorkerRequest&Request()const{return request_;}
 HRESULT Open(const WorkerRequest&input,HANDLE cancel,U64 other){
  const WorkerRequest r=input;
  if(handles_||bytes_||ready_)return E_UNEXPECTED;if(!Valid(r))return E_INVALIDARG;U64 charge=RequiredBytes(r.pin.bytes);if(charge>CpuCap||other>CpuCap-charge)return E_OUTOFMEMORY;if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  HRESULT hr=S_OK;char prefix[PathLimit];std::memcpy(prefix,r.path,3);prefix[3]=0;hr=Add(prefix,true);size_t n=Length(r.path,PathLimit);
  for(size_t i=3;i<n&&SUCCEEDED(hr);++i)if(r.path[i]=='\\'){std::memcpy(prefix,r.path,i);prefix[i]=0;if(Cancelled(cancel)){hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);break;}hr=Add(prefix,true);}
  if(SUCCEEDED(hr)&&Cancelled(cancel))hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);if(SUCCEEDED(hr))hr=Add(r.path,false);if(FAILED(hr)){Clear();return hr;}HANDLE file=handle_[handles_-1];BY_HANDLE_FILE_INFORMATION before,after;if(!GetFileInformationByHandle(file,&before)){hr=LastErrorHr();Clear();return hr;}U64 size=(U64(before.nFileSizeHigh)<<32)|before.nFileSizeLow;if(size!=r.pin.bytes||!((U64(before.nFileIndexHigh)<<32)|before.nFileIndexLow)){Clear();return E_INVALIDARG;}
  bytes_=new(std::nothrow) BYTE[size_t(size)];if(!bytes_){Clear();return E_OUTOFMEMORY;}owned_=charge;size_t at=0;while(at<size){if(Cancelled(cancel)){Clear();return HRESULT_FROM_WIN32(ERROR_CANCELLED);}DWORD got=0,want=DWORD((size-at)>65536?65536:size-at);if(!ReadFile(file,bytes_+at,want,&got,NULL)||got!=want){hr=LastErrorHr();Clear();return hr;}at+=got;}
  BYTE hash[32];if(Cancelled(cancel)){Clear();return HRESULT_FROM_WIN32(ERROR_CANCELLED);}if(!Hash(bytes_,size_t(size),hash)||std::memcmp(hash,r.pin.sha,32)){Clear();return E_INVALIDARG;}if(Cancelled(cancel)){Clear();return HRESULT_FROM_WIN32(ERROR_CANCELLED);}if(!GetFileInformationByHandle(file,&after)||!EqualInfo(before,after)){Clear();return E_INVALIDARG;}
  WCHAR finalWide[TextLimit];DWORD length=GetFinalPathNameByHandleW(file,finalWide,TextLimit,FILE_NAME_NORMALIZED|VOLUME_NAME_GUID);if(!length||length>=TextLimit||!NarrowAscii(finalWide,stamp_.finalPath,TextLimit)){hr=LastErrorHr();Clear();return hr;}stamp_.volume=after.dwVolumeSerialNumber;stamp_.id=(U64(after.nFileIndexHigh)<<32)|after.nFileIndexLow;stamp_.size=size;stamp_.links=after.nNumberOfLinks;stamp_.attributes=after.dwFileAttributes;stamp_.creation=after.ftCreationTime;stamp_.write=after.ftLastWriteTime;if(Cancelled(cancel)){Clear();return HRESULT_FROM_WIN32(ERROR_CANCELLED);}request_=r;ready_=true;return S_OK;
 }
};
}
