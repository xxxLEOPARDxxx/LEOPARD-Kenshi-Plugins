#pragma once
// Worker-only cached-town bytes. No scene, Ogre, native model loader or COM.
#include "atlas-view/AtlasReader.h"
#include "atlas-models/private-lod/BlobPolicy.h"
#include <cwchar>
#include <float.h>
namespace GsmCachedTown {
const SIZE_T MaxBytes=PrivateTownLod::MaxBytes;
const DWORD ChunkBytes=65536;
enum State {Empty,Ready,Unavailable,Cancelled,Refused};
struct Request {
    unsigned townId;float storedXYZ[3];
    Request():townId(0){std::memset(storedXYZ,0,sizeof(storedXYZ));}
};
struct Result {
    State state;std::vector<unsigned char>bytes;unsigned char sha256[32];PrivateTownLod::BlobInfo info;DWORD winError;char error[192];
    Result():state(Empty),winError(0){std::memset(sha256,0,32);error[0]=0;}
    void Clear(){std::vector<unsigned char>().swap(bytes);std::memset(sha256,0,32);info=PrivateTownLod::BlobInfo();state=Empty;winError=0;error[0]=0;}
};
typedef bool (*CancelFn)(void*);
typedef void (*ChunkFn)(void*,unsigned);
inline State Failure(Result&out,const char*why,DWORD error=0){out.Clear();out.state=Refused;out.winError=error;strncpy_s(out.error,sizeof(out.error),why,_TRUNCATE);return out.state;}
inline State Discard(Result&out){out.Clear();out.state=Cancelled;return out.state;}
inline bool Missing(DWORD error){return error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND;}
inline bool Stopped(CancelFn fn,void*context){return fn&&fn(context);}
class File {
    HANDLE value_;File(const File&);File&operator=(const File&);
public:
    explicit File(HANDLE value):value_(value){}
    ~File(){if(value_!=INVALID_HANDLE_VALUE)CloseHandle(value_);}
    HANDLE Get()const{return value_;}
    bool Close(){HANDLE value=value_;value_=INVALID_HANDLE_VALUE;return CloseHandle(value)!=FALSE;}
};
inline bool Root(const std::string&utf8,std::wstring&out){
    if(utf8.empty()||utf8.size()>4*MAX_PATH)return false;
    for(size_t i=0;i<utf8.size();++i)if(static_cast<unsigned char>(utf8[i])<32||utf8[i]==127||(utf8[i]==':'&&i!=1))return false;
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),int(utf8.size()),NULL,0);if(n<=0||n>=MAX_PATH)return false;
    std::wstring w(n,0);if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8.data(),int(utf8.size()),&w[0],n)!=n)return false;
    for(size_t i=0;i<w.size();++i)if(w[i]=='/')w[i]='\\';
    if(w.size()>3&&w[w.size()-1]=='\\')w.erase(w.size()-1);
    if(w.size()<3||w[1]!=':'||w[2]!='\\'||!((w[0]>='A'&&w[0]<='Z')||(w[0]>='a'&&w[0]<='z')))return false;
    wchar_t full[MAX_PATH]={0};DWORD count=GetFullPathNameW(w.c_str(),MAX_PATH,full,NULL);
    if(!count||count>=MAX_PATH||CompareStringOrdinal(w.c_str(),-1,full,-1,TRUE)!=CSTR_EQUAL)return false;
    for(size_t i=3;i<=w.size();++i)if(i==w.size()||w[i]=='\\')if(i>3&&(w[i-1]=='.'||w[i-1]==' '))return false;
    wchar_t drive[4]={w[0],':','\\',0};UINT type=GetDriveTypeW(drive);if(type==DRIVE_REMOTE||type==DRIVE_UNKNOWN||type==DRIVE_NO_ROOT_DIR)return false;
    out=w;return true;
}
inline bool FinalPath(HANDLE file,const std::wstring&expected){
    wchar_t path[MAX_PATH+4]={0};DWORD n=GetFinalPathNameByHandleW(file,path,MAX_PATH+4,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    return n>=4&&n<MAX_PATH+4&&!std::wcsncmp(path,L"\\\\?\\",4)&&CompareStringOrdinal(path+4,-1,expected.c_str(),-1,TRUE)==CSTR_EQUAL;
}
inline bool SameFile(const BY_HANDLE_FILE_INFORMATION&a,const BY_HANDLE_FILE_INFORMATION&b){
    return a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexLow==b.nFileIndexLow&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileSizeLow==b.nFileSizeLow&&a.nFileSizeHigh==b.nFileSizeHigh&&!CompareFileTime(&a.ftLastWriteTime,&b.ftLastWriteTime);
}
inline State Read(const std::string&game,const Request&request,Result&out,CancelFn cancel,void*context,ChunkFn afterChunk=NULL){
    out.Clear();
    try{
        if(Stopped(cancel,context))return Discard(out);
        for(unsigned i=0;i<3;++i)if(!_finite(request.storedXYZ[i]))return Failure(out,"cached town position is not finite");
        std::wstring root;if(!Root(game,root))return Failure(out,"cached town game root is not canonical/local");
        wchar_t name[48]={0};swprintf_s(name,48,L"distant_%u.mesh",request.townId);
        std::wstring path=root+(root[root.size()-1]=='\\'?L"":L"\\")+L"data\\meshes\\distant\\"+name;
        if(path.size()>=MAX_PATH)return Failure(out,"cached town source path exceeds limit");
        for(size_t i=3;i<path.size();++i)if(path[i]=='\\'){
            DWORD attributes=GetFileAttributesW(path.substr(0,i).c_str());
            if(attributes==INVALID_FILE_ATTRIBUTES){DWORD error=GetLastError();if(Missing(error)){out.state=Unavailable;out.winError=error;return out.state;}return Failure(out,"cached town source directory unavailable",error);}
            if((attributes&FILE_ATTRIBUTE_REPARSE_POINT)||!(attributes&FILE_ATTRIBUTE_DIRECTORY))return Failure(out,"cached town source directory is reparse/non-directory");
        }
        if(Stopped(cancel,context))return Discard(out);
        HANDLE handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OPEN_REPARSE_POINT,NULL);
        if(handle==INVALID_HANDLE_VALUE){DWORD error=GetLastError();if(Missing(error)){out.state=Unavailable;out.winError=error;return out.state;}return Failure(out,"cached town source open failed",error);}
        File file(handle);BY_HANDLE_FILE_INFORMATION before,after;LARGE_INTEGER size;
        State state=Refused;
        do{
            if(GetFileType(handle)!=FILE_TYPE_DISK||!FinalPath(handle,path)||!GetFileInformationByHandle(handle,&before)||(before.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){Failure(out,"cached town source identity/path refused",GetLastError());break;}
            if(!GetFileSizeEx(handle,&size)){Failure(out,"cached town source size query failed",GetLastError());break;}
            if(size.QuadPart<=0||static_cast<AtlasView::U64>(size.QuadPart)>MaxBytes){Failure(out,"cached town source size refused",size.QuadPart>0?ERROR_FILE_TOO_LARGE:ERROR_INVALID_DATA);break;}
            if(Stopped(cancel,context)){state=Discard(out);break;}
            out.bytes.resize(static_cast<SIZE_T>(size.QuadPart));AtlasView::Hash hash;
            if(!hash.Begin()){Failure(out,"cached town SHA initialization failed");break;}
            bool ok=true;SIZE_T offset=0;
            while(offset<out.bytes.size()){
                if(Stopped(cancel,context)){state=Discard(out);ok=false;break;}
                DWORD take=DWORD((std::min)(SIZE_T(ChunkBytes),out.bytes.size()-offset)),done=0;
                if(!ReadFile(handle,&out.bytes[offset],take,&done,NULL)||done!=take||!hash.Add(&out.bytes[offset],done)){Failure(out,"cached town source read/SHA failed",GetLastError());ok=false;break;}
                offset+=done;if(afterChunk)afterChunk(context,done);
                if(Stopped(cancel,context)){state=Discard(out);ok=false;break;}
            }
            if(!ok)break;
            if(!hash.Finish(out.sha256)||!GetFileInformationByHandle(handle,&after)||!SameFile(before,after)||!FinalPath(handle,path)){Failure(out,"cached town source changed during read",GetLastError());break;}
            if(!PrivateTownLod::Parser(&out.bytes[0],out.bytes.size()).Read(out.info)){Failure(out,"cached town native mesh format refused",ERROR_BAD_FORMAT);break;}
            if(Stopped(cancel,context)){state=Discard(out);break;}
            state=Ready;
        }while(false);
        if(!file.Close())return Failure(out,"cached town source close failed",GetLastError());
        if(Stopped(cancel,context))return Discard(out);
        if(state==Ready){out.state=Ready;return out.state;}
        return out.state;
    }catch(...){return Failure(out,"cached town file worker exception");}
}
}
