#pragma once
#include "GsmAtlasManifest.h"
#include "CaptureValidation.h" // Existing atomic world/load generation, no game calls.
#include "build/GsmNativeProfile.h"
#include "../tools/diagnostics/satellite-capture-probe/region-catalog/RegionCatalogData.h"
namespace GsmAtlas {
inline std::wstring Wide(const std::string&s){for(size_t i=0;i<s.size();++i)if(static_cast<unsigned char>(s[i])<32||s[i]==127||(s[i]==':'&&i!=1))throw std::runtime_error("source path control/alias refused");int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),NULL,0);if(n<=0||n>=MAX_PATH)throw std::runtime_error("source path UTF8/length refused");std::wstring w(n,0);if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&w[0],n)!=n)throw std::runtime_error("source path conversion failed");for(size_t i=0;i<w.size();++i)if(w[i]=='/')w[i]='\\';return w;}
inline std::wstring Canonical(const std::string&s){
    std::wstring w=Wide(s);if(w.size()<4||w[1]!=':'||w[2]!='\\'||!((w[0]>='A'&&w[0]<='Z')||(w[0]>='a'&&w[0]<='z')))throw std::runtime_error("source must be a local absolute path");
    wchar_t out[MAX_PATH]={0};DWORD n=GetFullPathNameW(w.c_str(),MAX_PATH,out,NULL);if(!n||n>=MAX_PATH||CompareStringOrdinal(w.c_str(),-1,out,-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("source path is not canonical");
    wchar_t drive[4]={w[0],':','\\',0};UINT type=GetDriveTypeW(drive);if(type==DRIVE_REMOTE||type==DRIVE_UNKNOWN||type==DRIVE_NO_ROOT_DIR)throw std::runtime_error("source drive is not local");
    for(size_t i=3;i<=w.size();++i)if(i==w.size()||w[i]=='\\'){std::wstring part=w.substr(0,i);DWORD a=GetFileAttributesW(part.c_str());if(a==INVALID_FILE_ATTRIBUTES||(a&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("source missing or reparse path refused");}
    return w;
}
struct Job {
    HANDLE thread;LONG generation;bool validation,ok;std::string folder,game,engine,module[2];char error[512];Manifest manifest;AtlasView::ReadRequest read;
    Job():thread(NULL),generation(0),validation(false),ok(false){error[0]=0;}
};
inline void ObserveGeneration(void*input){Job&job=*static_cast<Job*>(input);if(job.generation!=InterlockedCompareExchange(&GSM::generation,0,0))InterlockedExchange(&job.read.cancel,1);}
inline bool Cancelled(Job&job){ObserveGeneration(&job);return AtlasView::Cancelled(job.read);}
inline std::wstring Lower(std::wstring s){if(!s.empty())CharLowerBuffW(&s[0],static_cast<DWORD>(s.size()));return s;}
inline std::string Utf8(const std::wstring&w){int n=WideCharToMultiByte(CP_UTF8,0,w.data(),static_cast<int>(w.size()),NULL,0,NULL,NULL);if(n<=0)throw std::runtime_error("source UTF8 conversion failed");std::string s(n,0);if(WideCharToMultiByte(CP_UTF8,0,w.data(),static_cast<int>(w.size()),&s[0],n,NULL,NULL)!=n)throw std::runtime_error("source UTF8 conversion failed");return s;}
inline void CandidateTree(const std::wstring&root,const std::set<std::wstring>&names,std::set<std::wstring>&found,std::set<std::wstring>&result,Job&job,unsigned&entries,unsigned&directories){
    std::vector<std::pair<std::wstring,unsigned> >queue;queue.push_back(std::make_pair(root,0U));
    for(size_t q=0;q<queue.size();++q){if(Cancelled(job))throw std::runtime_error("source enumeration cancelled");if(++directories>16384||queue[q].second>32)throw std::runtime_error("source directory budget exceeded");std::wstring base=queue[q].first;WIN32_FIND_DATAW row;HANDLE search=FindFirstFileW((base+L"\\*").c_str(),&row);if(search==INVALID_HANDLE_VALUE){DWORD e=GetLastError();if(e==ERROR_FILE_NOT_FOUND||e==ERROR_PATH_NOT_FOUND)continue;throw std::runtime_error("source directory enumeration failed");}
        try{do{if(Cancelled(job))throw std::runtime_error("source enumeration cancelled");if(++entries>250000)throw std::runtime_error("source enumeration entry budget exceeded");if(!std::wcscmp(row.cFileName,L".")||!std::wcscmp(row.cFileName,L".."))continue;std::wstring path=base+L"\\"+row.cFileName;
            if(row.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)throw std::runtime_error("reparse mod candidate tree refused");
            if(row.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(queue.size()>=16384)throw std::runtime_error("source directory queue budget exceeded");queue.push_back(std::make_pair(path,queue[q].second+1));}
            else{std::wstring name=Lower(row.cFileName);if(name.size()>=4&&name.substr(name.size()-4)==L".mod"&&names.find(name)!=names.end()){found.insert(name);result.insert(Lower(Canonical(Utf8(path))));if(result.size()>512)throw std::runtime_error("source candidate count exceeded");}}
        }while(FindNextFileW(search,&row));if(GetLastError()!=ERROR_NO_MORE_FILES)throw std::runtime_error("source enumeration interrupted");}catch(...){FindClose(search);throw;}if(!FindClose(search))throw std::runtime_error("source enumeration close failed");
    }
}
inline std::set<std::wstring>CurrentSources(Job&job){
    std::string cfg;if(!Small(job.game+"data/mods.cfg",cfg,true))throw std::runtime_error("enabled mod list missing/oversize");if(cfg.compare(0,3,"\xef\xbb\xbf")==0)cfg.erase(0,3);std::set<std::wstring>names,found,result;
    std::istringstream lines(cfg);std::string line;while(std::getline(lines,line)){size_t a=line.find_first_not_of(" \t\r"),b=line.find_last_not_of(" \t\r");if(a==line.npos||line[a]=='#'||line[a]==';')continue;line=line.substr(a,b-a+1);std::wstring name=Lower(Wide(line));if(name.find_first_of(L"/\\:")!=name.npos||name.size()<5||name.substr(name.size()-4)!=L".mod")throw std::runtime_error("enabled mod filename unsupported");names.insert(name);if(names.size()>512)throw std::runtime_error("enabled mod count exceeded");}
    const std::string mandatory[5]={job.engine,job.game+"data/mods.cfg",job.game+"data/gamedata.base",job.game+"data/Dialogue.mod",job.game+"data/newland/land/fullmap.tif"};for(unsigned i=0;i<5;++i)result.insert(Lower(Canonical(mandatory[i])));
    std::wstring game=Canonical(job.game.substr(0,job.game.size()-1));size_t common=game.find_last_of(L"\\");if(common==game.npos)throw std::runtime_error("Steam library directory unavailable");std::wstring steamapps=game.substr(0,common);common=steamapps.find_last_of(L"\\");if(common==steamapps.npos)throw std::runtime_error("Steam library directory unavailable");steamapps.erase(common);
    unsigned entries=0,dirs=0;CandidateTree(game+L"\\mods",names,found,result,job,entries,dirs);CandidateTree(steamapps+L"\\workshop\\content\\233860",names,found,result,job,entries,dirs);if(found!=names)throw std::runtime_error("enabled mod source unresolved");return result;
}
inline bool HashFile(const std::string&path,const std::string&expected,Job&job,AtlasView::U64&remaining){
    if(!GSM::Hash(expected)||Cancelled(job))return false;std::wstring w=Canonical(path);
    HANDLE f=CreateFileW(w.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OPEN_REPARSE_POINT,NULL);if(f==INVALID_HANDLE_VALUE)return false;
    bool ok=false;
    try{
        BY_HANDLE_FILE_INFORMATION info;LARGE_INTEGER size;wchar_t final[MAX_PATH+4]={0};DWORD count=GetFinalPathNameByHandleW(f,final,MAX_PATH+4,FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
        if(!count||count>=MAX_PATH+4||std::wcsncmp(final,L"\\\\?\\",4)||CompareStringOrdinal(final+4,-1,w.c_str(),-1,TRUE)!=CSTR_EQUAL||!GetFileInformationByHandle(f,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))||!GetFileSizeEx(f,&size)||size.QuadPart<0||static_cast<AtlasView::U64>(size.QuadPart)>remaining){}
        else{
            remaining-=static_cast<AtlasView::U64>(size.QuadPart);AtlasView::Hash h;ok=h.Begin();unsigned char bytes[65536],actual[32],wanted[32];AtlasView::U64 left=size.QuadPart;
            while(ok&&left){if(Cancelled(job)){ok=false;break;}DWORD take=left<sizeof(bytes)?static_cast<DWORD>(left):sizeof(bytes),done=0;ok=ReadFile(f,bytes,take,&done,NULL)&&done==take&&h.Add(bytes,done);left-=done;job.read.readBytes+=done;if(job.read.afterChunk)job.read.afterChunk(job.read.observer);}
            BY_HANDLE_FILE_INFORMATION after;ok=ok&&!Cancelled(job)&&h.Finish(actual)&&Hex(expected,wanted)&&!std::memcmp(actual,wanted,32)&&GetFileInformationByHandle(f,&after)&&info.dwVolumeSerialNumber==after.dwVolumeSerialNumber&&info.nFileIndexLow==after.nFileIndexLow&&info.nFileIndexHigh==after.nFileIndexHigh&&info.nFileSizeLow==after.nFileSizeLow&&info.nFileSizeHigh==after.nFileSizeHigh&&!CompareFileTime(&info.ftLastWriteTime,&after.ftLastWriteTime);
        }
    }catch(...){CloseHandle(f);throw;}
    if(!CloseHandle(f))ok=false;return ok;
}
inline void Validate(Job&job){
    std::string ready,capture;Manifest m;
    if(!Small(job.folder+"ready.json",ready)||!Ready(ready,m)||!Small(job.folder+"capture.json",capture))throw std::runtime_error("complete GST2 ready/capture metadata missing or invalid");
    unsigned char digest[32];if(!AtlasView::Digest(capture.data(),static_cast<DWORD>(capture.size()),digest)||std::memcmp(digest,m.captureMetadata,32))throw std::runtime_error("capture metadata SHA differs from ready manifest");
    boost::property_tree::ptree t;if(!Json(capture,t)||!Unique(t))throw std::runtime_error("capture identity JSON invalid");
    if(t.get<std::string>("format")!="GSCAP2"||t.get<unsigned>("version")!=2||t.get<std::string>("engineSha256")!=GsmEngineHash||t.get<std::string>("sourceManifestSha256")!=SatelliteRegion::ManifestSha256||t.get<unsigned>("grid")!=4||t.get<unsigned>("baseSide")!=8192||t.get<double>("span")!=294912)throw std::runtime_error("capture world/native source profile differs");
    const boost::property_tree::ptree&origin=t.get_child("origin");unsigned n=0;for(boost::property_tree::ptree::const_iterator i=origin.begin();i!=origin.end();++i){if(!i->first.empty()||n>=2||i->second.get_value<double>()!=-147456)throw std::runtime_error("capture bounds differ");++n;}if(n!=2)throw std::runtime_error("capture origin invalid");
    const std::string mods=t.get<std::string>("modsCfgSha256"),height=t.get<std::string>("heightSha256");if(!GSM::Hash(mods)||!GSM::Hash(height))throw std::runtime_error("capture source identities missing");
    std::wstring paths[3]={Canonical(t.get<std::string>("enginePath")),Canonical(t.get<std::string>("modsCfgPath")),Canonical(t.get<std::string>("heightPath"))};
    std::wstring current[3]={Canonical(job.engine),Canonical(job.game+"data/mods.cfg"),Canonical(job.game+"data/newland/land/fullmap.tif")};
    for(unsigned p=0;p<3;++p)if(CompareStringOrdinal(paths[p].c_str(),-1,current[p].c_str(),-1,TRUE)!=CSTR_EQUAL)throw std::runtime_error("capture dedicated source path differs");
    AtlasView::U64 remaining=8ULL*1024*1024*1024;
    if(!HashFile(job.engine,GsmEngineHash,job,remaining)||!HashFile(job.game+"data/mods.cfg",mods,job,remaining)||!HashFile(job.module[0],GsmOgreHash,job,remaining)||!HashFile(job.module[1],GsmD3d11Hash,job,remaining))throw std::runtime_error("current engine/mods/native renderer differs from capture profile");
    const std::set<std::wstring>currentSources=CurrentSources(job);
    std::set<std::wstring>seen;unsigned sources=0;bool dedicated[3]={false};const std::string expected[3]={GsmEngineHash,mods,height};
    const boost::property_tree::ptree&list=t.get_child("sources");
    for(boost::property_tree::ptree::const_iterator i=list.begin();i!=list.end();++i){if(!i->first.empty()||++sources>512)throw std::runtime_error("capture source count invalid");const std::string path=i->second.get<std::string>("path"),sha=i->second.get<std::string>("sha256");std::wstring key=Canonical(path);for(unsigned p=0;p<3;++p)if(CompareStringOrdinal(key.c_str(),-1,paths[p].c_str(),-1,TRUE)==CSTR_EQUAL){if(sha!=expected[p])throw std::runtime_error("capture dedicated source digest differs");dedicated[p]=true;}CharLowerBuffW(&key[0],static_cast<DWORD>(key.size()));if(!seen.insert(key).second)throw std::runtime_error("duplicate capture source path");if(!HashFile(path,sha,job,remaining))throw std::runtime_error("capture source file changed or unavailable: "+path);}
    if(!sources||!dedicated[0]||!dedicated[1]||!dedicated[2])throw std::runtime_error("capture dedicated source identities missing");
    if(seen!=currentSources)throw std::runtime_error("current enabled mod candidate set differs from capture");
    for(unsigned a=0;a<SatelliteRegion::Data.assetCount;++a){const SatelliteRegion::Asset&v=SatelliteRegion::Data.assets[a];if(!HashFile(job.game+v.path,v.sha256,job,remaining))throw std::runtime_error("native terrain material asset differs: "+std::string(v.path));}
    for(unsigned a=0;a<SatelliteRegion::Data.paintedCount;++a){const SatelliteRegion::Painted&v=SatelliteRegion::Data.painted[a];if(!HashFile(job.game+v.path,v.sha256,job,remaining))throw std::runtime_error("native terrain painted asset differs: "+std::string(v.path));}
    if(!HashFile(job.game+"data/newland/land/blendmap.png",GsmBlendHash,job,remaining))throw std::runtime_error("native global blend source differs");
    for(unsigned i=0;i<sizeof(GsmShaderInputs)/sizeof(GsmShaderInputs[0]);++i)if(!HashFile(job.game+GsmShaderInputs[i].path,GsmShaderInputs[i].sha256,job,remaining))throw std::runtime_error("native lighting shader/material source differs from viewer build");
    if(CurrentSources(job)!=currentSources||!HashFile(job.game+"data/mods.cfg",mods,job,remaining))throw std::runtime_error("enabled mod source set changed during validation");
    // Require all16 real finalized headers before showing any partial map.
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){if(Cancelled(job))throw std::runtime_error("capture validation cancelled");std::ostringstream p;p<<job.folder<<"tile_"<<x<<'_'<<z<<".gst";HANDLE f=CreateFileW(Wide(p.str()).c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)throw std::runtime_error("capture tile missing");unsigned char h[1024];DWORD done=0;LARGE_INTEGER size;bool ok=GetFileSizeEx(f,&size)&&ReadFile(f,h,1024,&done,NULL)&&done==1024&&AtlasView::ValidHeader(h,size.QuadPart,x,z)&&AtlasView::Digest(h,1024,digest)&&!std::memcmp(digest,m.header[z*4+x],32);if(!CloseHandle(f))ok=false;if(!ok)throw std::runtime_error("capture tile incomplete or header identity differs");}
    m.minX=m.minZ=-147456;m.maxX=m.maxZ=147456;job.manifest=m;
}
inline DWORD WINAPI Run(void*input){Job&job=*static_cast<Job*>(input);job.read.afterChunk=ObserveGeneration;job.read.observer=&job;try{if(Cancelled(job))throw std::runtime_error("GST2 job cancelled");if(job.validation){Validate(job);job.ok=!Cancelled(job);}else job.ok=AtlasView::ReadLevel(job.read);if(!job.ok&&!job.error[0])strcpy_s(job.error,sizeof(job.error),"GST2 file worker failed or cancelled");}catch(const std::exception&e){strncpy_s(job.error,sizeof(job.error),e.what(),_TRUNCATE);}catch(...){strcpy_s(job.error,sizeof(job.error),"GST2 file worker exception");}return job.ok?0:1;}
}
