#pragma once
#include "GradeProfiles.h"
#include <algorithm>
#include <memory>
namespace IMRuntime {
struct Lock { CRITICAL_SECTION* p;Lock(CRITICAL_SECTION& c):p(&c){EnterCriticalSection(p);}~Lock(){LeaveCriticalSection(p);} };
struct Invoke {
    static int Cpp(IM_Query q,const IM_ItemView* v,IM_Appearance* a,void* c) {
        try { return q(v,a,c); }catch(...) { return -1; }
    }
    static int Seh(IM_Query q,const IM_ItemView* v,IM_Appearance* a,void* c) {
        __try { return Cpp(q,v,a,c); }__except(EXCEPTION_EXECUTE_HANDLER) { return -1; }
    }
    int operator()(IM_Query q,const IM_ItemView& v,IM_Appearance& a,void* c) {
        const int result=Seh(q,&v,&a,c);
        if(result!=0&&(result!=1||!IMExt::Valid(a))) {
            ErrorLog("ItemMarkers: grade provider failed/returned invalid appearance - provider disabled");return -1;
        }return result;
    }
};
struct Palette {
    uint32_t operator()(const char* name,uint32_t rgb) {
        char fallback[8];sprintf_s(fallback,"#%06x",rgb);
        const std::string colour=GameTheme::Hex(name,fallback);
        uint32_t result=rgb;IMExt::Rgb(colour,result);return result;
    }
};
class State {
    CRITICAL_SECTION lock;
    IMExt::Registry providers;
    IMExt::Rules rules;
    struct Pin { IM_Handle handle;HMODULE module; } pins[IMExt::MaxProviders];
    uint64_t revision;
    bool pendingReload;
    std::string directory;
public:
    State():revision(1),pendingReload(false) { InitializeCriticalSection(&lock);std::memset(pins,0,sizeof(pins)); }
    // Game plugins live to process exit. No process-detach FreeLibrary or GUI work.
    ~State(){DeleteCriticalSection(&lock);}
    int Add(const IM_Provider* p,IM_Handle* handle) {
        if(!p||!handle)return IM_INVALID;*handle=0;Lock guard(lock);
        if(providers.querying)return IM_BUSY;
        if(p->size!=sizeof(*p)||!p->query)return IM_INVALID;
        HMODULE module=0;
        if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCSTR>(p->query),&module))return IM_INVALID;
        const int result=providers.Add(*p,*handle);
        if(result!=IM_OK) { FreeLibrary(module);return result; }
        for(unsigned i=0;i<IMExt::MaxProviders;++i)if(!pins[i].handle) { pins[i].handle=*handle;pins[i].module=module;break; }
        ++revision;return IM_OK;
    }
    int Remove(IM_Handle handle) {
        HMODULE module=0;
        { Lock guard(lock);const int result=providers.Remove(handle);if(result!=IM_OK)return result;
          for(unsigned i=0;i<IMExt::MaxProviders;++i)if(pins[i].handle==handle) { module=pins[i].module;pins[i].handle=0;pins[i].module=0;break; }
          ++revision;
        }
        if(module)FreeLibrary(module);return IM_OK;
    }
    int Notify(IM_Handle handle) { Lock guard(lock);if(!providers.Contains(handle))return IM_NOT_FOUND;++revision;return IM_OK; }
    int RequestReload() { Lock guard(lock);pendingReload=true;return IM_OK; }
    uint64_t Revision() { Lock guard(lock);return revision; }
    bool Query(const IM_ItemView& view,IM_Appearance& out,uint64_t& observed) {
        Lock guard(lock);Invoke invoke;unsigned disabled=0;
        for(unsigned i=0;i<providers.count;++i)disabled+=providers.slots[i].disabled?1:0;
        const bool found=providers.Query(view,out,invoke)||rules.Query(view,out);
        unsigned after=0;for(unsigned i=0;i<providers.count;++i)after+=providers.slots[i].disabled?1:0;
        if(after!=disabled)++revision; // revisit earlier visible icons after provider quarantine
        observed=revision;return found;
    }
    void SetDirectory(const std::string& path) { Lock guard(lock);directory=path; }
    void Reload() {
        std::string dir;
        { Lock guard(lock);dir=directory;pendingReload=false; }
        if(dir.empty())return;
        std::auto_ptr<IMExt::Rules> candidate(new IMExt::Rules);
        std::vector<std::string> names;WIN32_FIND_DATAA data;
        const DWORD attrs=GetFileAttributesA(dir.c_str());
        if(attrs!=INVALID_FILE_ATTRIBUTES&&((attrs&FILE_ATTRIBUTE_REPARSE_POINT)||!(attrs&FILE_ATTRIBUTE_DIRECTORY))) {
            ErrorLog("ItemMarkers: grades path must be an ordinary directory - keeping previous rules");return;
        }
        HANDLE find=FindFirstFileA((dir+"\\*.ini").c_str(),&data);
        bool valid=true;
        if(find!=INVALID_HANDLE_VALUE) {
            do {
                if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
                if(names.size()==64||(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||data.nFileSizeHigh||data.nFileSizeLow>65536) { valid=false;break; }
                names.push_back(data.cFileName);
            }while(FindNextFileA(find,&data));
            if(valid&&GetLastError()!=ERROR_NO_MORE_FILES)valid=false;
            FindClose(find);
        }else if(GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)valid=false;
        std::sort(names.begin(),names.end());Palette palette;std::string error;
        for(size_t i=0;valid&&i<names.size();++i) {
            const std::string path=dir+"\\"+names[i];
            HANDLE file=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,0);
            if(file==INVALID_HANDLE_VALUE) { valid=false;break; }
            BY_HANDLE_FILE_INFORMATION info;DWORD read=0;
            valid=GetFileInformationByHandle(file,&info)!=FALSE&&!(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)&&!info.nFileSizeHigh&&info.nFileSizeLow<=65536;
            std::string text;if(valid) {
                text.resize(info.nFileSizeLow);
                if(!text.empty())valid=ReadFile(file,&text[0],static_cast<DWORD>(text.size()),&read,0)!=FALSE&&read==text.size();
            }
            CloseHandle(file);
            if(valid)valid=IMExt::LoadProfile(text,*candidate,palette,error);
            if(!valid) { std::string message="ItemMarkers: grade profile '"+names[i]+"' refused: "+error;ErrorLog(message.c_str()); }
        }
        if(!valid) { ErrorLog("ItemMarkers: grade rules reload refused - keeping previous rules");return; }
        { Lock guard(lock);rules=*candidate;++revision; }
    }
    void ReloadSafely() {
        try { Reload(); } catch(...) {
            { Lock guard(lock);pendingReload=false; }
            ErrorLog("ItemMarkers: grade rules reload failed - keeping previous rules");
        }
    }
    void ProcessReload() { bool pending;{Lock guard(lock);pending=pendingReload;}if(pending)ReloadSafely(); }
};
static State state;
inline State& Get() { return state; }
}
