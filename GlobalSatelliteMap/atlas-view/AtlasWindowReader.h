#pragma once
// C++03 worker-only GST2 windows and GSI1 index. No game/Ogre/D3D calls.
#include "AtlasReader.h"
#include "AtlasWindowPolicy.h"
#include <cwchar>
namespace AtlasView {
const unsigned WindowIndexRecords=21840,WindowIndexHeaderBytes=1024,WindowRecordBytes=96;
const unsigned WindowIndexBodyBytes=WindowIndexRecords*WindowRecordBytes;
const unsigned WindowIndexFileBytes=WindowIndexHeaderBytes+WindowIndexBodyBytes;

class WindowReadFile {
    HANDLE file_;BY_HANDLE_FILE_INFORMATION before_;
    WindowReadFile(const WindowReadFile&);WindowReadFile&operator=(const WindowReadFile&);
public:
    WindowReadFile():file_(INVALID_HANDLE_VALUE){std::memset(&before_,0,sizeof(before_));}
    ~WindowReadFile(){if(file_!=INVALID_HANDLE_VALUE)CloseHandle(file_);}
    HANDLE Handle()const{return file_;}
    U64 Length()const{return (U64(before_.nFileSizeHigh)<<32)|before_.nFileSizeLow;}
    bool Open(const std::wstring&path,DWORD accessHint,ReadRequest&r){
        if(file_!=INVALID_HANDLE_VALUE){r.error=ERROR_INVALID_PARAMETER;return false;}
        std::wstring normalized=path;
        if(normalized.size()<4||normalized.size()>=MAX_PATH||normalized[1]!=L':'||(normalized[2]!=L'\\'&&normalized[2]!=L'/')||!((normalized[0]>=L'A'&&normalized[0]<=L'Z')||(normalized[0]>=L'a'&&normalized[0]<=L'z'))){r.error=ERROR_INVALID_NAME;return false;}
        for(size_t i=0;i<normalized.size();++i){if(normalized[i]<32||normalized[i]==127||(normalized[i]==L':'&&i!=1)){r.error=ERROR_INVALID_NAME;return false;}if(normalized[i]==L'/')normalized[i]=L'\\';}
        wchar_t full[MAX_PATH]={0};DWORD n=GetFullPathNameW(normalized.c_str(),MAX_PATH,full,NULL);
        if(!n||n>=MAX_PATH||CompareStringOrdinal(normalized.c_str(),-1,full,-1,TRUE)!=CSTR_EQUAL){r.error=ERROR_INVALID_NAME;return false;}
        wchar_t drive[4]={normalized[0],L':',L'\\',0};UINT type=GetDriveTypeW(drive);
        if(type==DRIVE_REMOTE||type==DRIVE_UNKNOWN||type==DRIVE_NO_ROOT_DIR){r.error=ERROR_ACCESS_DENIED;return false;}
        // Guard EVERY ancestor, not just the selected leaf. The final handle's
        // normalized physical path is checked too, closing ancestor-swap aliases.
        for(size_t i=3;i<=normalized.size();++i)if(i==normalized.size()||normalized[i]==L'\\'){
            DWORD a=GetFileAttributesW(normalized.substr(0,i).c_str());
            if(a==INVALID_FILE_ATTRIBUTES){r.error=GetLastError();return false;}
            if(a&FILE_ATTRIBUTE_REPARSE_POINT){r.error=ERROR_ACCESS_DENIED;return false;}
        }
        file_=CreateFileW(normalized.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,accessHint|FILE_FLAG_OPEN_REPARSE_POINT,NULL);
        if(file_==INVALID_HANDLE_VALUE){r.error=GetLastError();return false;}
        wchar_t physical[MAX_PATH+4]={0};n=GetFinalPathNameByHandleW(file_,physical,MAX_PATH+4,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
        if(!n||n>=MAX_PATH+4||std::wcsncmp(physical,L"\\\\?\\",4)||CompareStringOrdinal(physical+4,-1,normalized.c_str(),-1,TRUE)!=CSTR_EQUAL||!GetFileInformationByHandle(file_,&before_)||(before_.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){r.error=ERROR_ACCESS_DENIED;return false;}
        return true;
    }
    bool Stable(ReadRequest&r)const{
        BY_HANDLE_FILE_INFORMATION after;
        if(!GetFileInformationByHandle(file_,&after)){r.error=GetLastError();return false;}
        if(before_.dwVolumeSerialNumber!=after.dwVolumeSerialNumber||before_.nFileIndexHigh!=after.nFileIndexHigh||before_.nFileIndexLow!=after.nFileIndexLow||before_.nFileSizeHigh!=after.nFileSizeHigh||before_.nFileSizeLow!=after.nFileSizeLow||CompareFileTime(&before_.ftLastWriteTime,&after.ftLastWriteTime)){r.error=ERROR_INVALID_DATA;return false;}return true;
    }
    bool Close(ReadRequest&r){if(file_==INVALID_HANDLE_VALUE)return true;HANDLE f=file_;file_=INVALID_HANDLE_VALUE;if(!CloseHandle(f)){r.error=GetLastError();return false;}return true;}
};

struct WindowIndex {
    std::vector<unsigned char> records;
    WindowIndex():verified_(false){}
    void Clear(){std::vector<unsigned char>().swap(records);verified_=false;}
    bool Hashes(const WindowKey&key,unsigned char out[3][32])const{
        if(!verified_||!out||!key.Valid()||records.size()!=WindowIndexBodyBytes)return false;
        unsigned record=WindowRecord(key);if(record>=WindowIndexRecords)return false;
        std::memcpy(out,&records[record*WindowRecordBytes],WindowRecordBytes);return true;
    }
private:
    bool verified_;
    friend bool LoadWindowIndex(const std::wstring&,const unsigned char[32],const unsigned char[16][32],const unsigned char[32],ReadRequest&,WindowIndex&);
};

inline bool LoadWindowIndex(const std::wstring&path,const unsigned char expectedCapture[32],const unsigned char expectedHeaders[16][32],const unsigned char expectedIndex[32],ReadRequest&control,WindowIndex&out){
    out.Clear();control.error=ERROR_SUCCESS;control.readBytes=0;
    if(!expectedCapture||!expectedHeaders||!expectedIndex||!Nonzero(expectedCapture,32)||!Nonzero(expectedIndex,32)){control.error=ERROR_INVALID_PARAMETER;return false;}
    for(unsigned i=0;i<16;++i)if(!Nonzero(expectedHeaders[i],32)){control.error=ERROR_INVALID_PARAMETER;return false;}
    if(Cancelled(control))return false;
    bool ok=false;
    try{
        WindowReadFile file;
        if(!file.Open(path,FILE_FLAG_SEQUENTIAL_SCAN,control))return false;
        if(file.Length()!=WindowIndexFileBytes){control.error=ERROR_INVALID_DATA;return false;}
        unsigned char h[1024];Hash hash;
        if(!hash.Begin()){control.error=ERROR_CRC;return false;}
        if(!ReadAt(file.Handle(),0,h,1024,control,&hash))return false;
        if(std::memcmp(h,"GSI1",4)||Get32(h+4)!=1||Get32(h+8)!=1024||Get32(h+12)!=256||Get32(h+16)!=4||Get32(h+20)!=8192||Get32(h+24)!=6||Get32(h+28)!=3||Get32(h+32)!=WindowIndexRecords||Nonzero(h+36,28)||std::memcmp(h+64,expectedCapture,32)||std::memcmp(h+96,expectedHeaders,512)||Nonzero(h+608,416)){control.error=ERROR_INVALID_DATA;return false;}
        out.records.resize(WindowIndexBodyBytes);
        if(!ReadAt(file.Handle(),1024,&out.records[0],WindowIndexBodyBytes,control,&hash)){out.Clear();return false;}
        unsigned char actual[32];
        if(!hash.Finish(actual)||std::memcmp(actual,expectedIndex,32)){control.error=ERROR_CRC;out.Clear();return false;}
        for(unsigned at=0;at<WindowIndexBodyBytes;at+=32){if(!(at&65535)&&Cancelled(control)){out.Clear();return false;}if(!Nonzero(&out.records[at],32)){control.error=ERROR_INVALID_DATA;out.Clear();return false;}}
        ok=!Cancelled(control)&&file.Stable(control)&&file.Close(control);
        if(ok)out.verified_=true;
    }catch(...){control.error=ERROR_NOT_ENOUGH_MEMORY;ok=false;}
    if(!ok)out.Clear();return ok;
}

// key.x/z are window GRID indices. The source level stays in key; ReadRequest
// advertises one256-square upload (level5) on success, never an entire source mip.
inline bool ReadWindow(ReadRequest&r,const WindowKey&key,const unsigned char expectedWindowHashes[3][32]){
    r.Clear();r.error=ERROR_SUCCESS;r.readBytes=0;
    if(!key.Valid()||!expectedWindowHashes||!Nonzero(r.expectedHeader,32)){r.error=ERROR_INVALID_PARAMETER;return false;}
    for(unsigned p=0;p<3;++p)if(!Nonzero(expectedWindowHashes[p],32)){r.error=ERROR_INVALID_PARAMETER;return false;}
    r.tileX=key.tile%4;r.tileZ=key.tile/4;r.level=5;
    if(Cancelled(r))return false;
    bool ok=false;
    try{
        WindowReadFile file;
        if(!file.Open(r.path,FILE_FLAG_RANDOM_ACCESS,r))return false;
        unsigned char header[1024],actual[32];
        if(!ReadAt(file.Handle(),0,header,1024,r,NULL))return false;
        if(!ValidHeader(header,file.Length(),r.tileX,r.tileZ)||!Digest(header,1024,actual)||std::memcmp(actual,r.expectedHeader,32)){r.error=ERROR_INVALID_DATA;return false;}
        ok=true;
        for(unsigned p=0;p<3&&ok;++p){
            if(Cancelled(r)){ok=false;break;}
            const unsigned rowBytes=WindowSide*Stride(p);
            r.plane[p].resize(WindowSide*rowBytes);Hash hash;
            if(!hash.Begin()){r.error=ERROR_CRC;ok=false;break;}
            for(unsigned row=0;row<WindowSide&&ok;++row){
                const U64 at=PlaneOffset(key.level,p)+((U64(key.z)*WindowSide+row)*Side(key.level)+U64(key.x)*WindowSide)*Stride(p);
                if(at>file.Length()||rowBytes>file.Length()-at){r.error=ERROR_INVALID_DATA;ok=false;break;}
                ok=ReadAt(file.Handle(),at,&r.plane[p][row*rowBytes],rowBytes,r,&hash);
            }
            if(ok&&(!hash.Finish(actual)||std::memcmp(actual,expectedWindowHashes[p],32))){r.error=ERROR_CRC;ok=false;}
        }
        ok=ok&&!Cancelled(r)&&file.Stable(r)&&file.Close(r);
    }catch(...){r.error=ERROR_NOT_ENOUGH_MEMORY;ok=false;}
    if(!ok)r.Clear();return ok;
}
}
