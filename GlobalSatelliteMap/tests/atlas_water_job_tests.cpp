#ifndef WATER_JOB_HEADER
#define WATER_JOB_HEADER "../GsmAtlasJob.h"
#endif
#include WATER_JOB_HEADER
#include <cstdio>
#include <limits>
namespace GSM {volatile LONG generation=0;}
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static std::vector<std::wstring> files,dirs;
static void Directory(const std::wstring&path){
    for(size_t i=3;i<=path.size();++i)if(i==path.size()||path[i]==L'\\'){
        std::wstring p=path.substr(0,i);
        if(GetFileAttributesW(p.c_str())==INVALID_FILE_ATTRIBUTES){Check(CreateDirectoryW(p.c_str(),NULL)!=FALSE,"fixture directory create");dirs.push_back(p);}
    }
}
static void Cleanup(){for(size_t i=files.size();i;--i)DeleteFileW(files[i-1].c_str());for(size_t i=dirs.size();i;--i)RemoveDirectoryW(dirs[i-1].c_str());}
static void Changed(void*){InterlockedIncrement(&GSM::generation);}
namespace Dispatch {
typedef GsmAtlas::Job Job;
static unsigned terrainCalls=0,waterCalls=0,windowCalls=0,minimumCalls=0;static bool success=true;
using GsmAtlas::ObserveGeneration;using GsmAtlas::Cancelled;
static void Validate(Job&){++terrainCalls;}
static void ValidateWaterSources(Job&job){++waterCalls;if(!success)throw std::runtime_error("fixture water validation failure");job.manifest.waterVerified=true;}
static bool WindowMinimum(Job&job){++minimumCalls;return GsmAtlas::WindowMinimum(job);}
namespace AtlasView {
static bool ReadWindow(::AtlasView::ReadRequest&,const ::AtlasView::WindowKey&,const unsigned char[3][32]){++windowCalls;return success;}
}
#include "ExtractedWaterJobRun.h"
static void Reset(){terrainCalls=waterCalls=windowCalls=minimumCalls=0;success=true;}
}
static void Window(GsmAtlas::Job&job){job.read.plane[1].assign(256*256*4,0);job.read.plane[2].assign(256*256*4,0);}
static void Pixel(GsmAtlas::Job&job,unsigned i,float h,unsigned coverage){job.read.plane[1][i*4+3]=static_cast<unsigned char>(coverage);std::memcpy(&job.read.plane[2][i*4],&h,4);}
static void Minimum(){
    GsmAtlas::Job job;Check(!job.waterValidation&&!job.manifest.waterVerified&&!job.windowHasFiniteHeight&&job.windowMinHeight==0,"new job metadata defaults false/zero");
    Window(job);Check(GsmAtlas::WindowMinimum(job)&&!job.windowHasFiniteHeight&&job.windowMinHeight==0,"all zero coverage has no height");
    Pixel(job,0,140,255);Pixel(job,65535,-12.5f,1);Pixel(job,4096,-999,0);Pixel(job,9,std::numeric_limits<float>::quiet_NaN(),255);Pixel(job,10,std::numeric_limits<float>::infinity(),255);
    Check(GsmAtlas::WindowMinimum(job)&&job.windowHasFiniteHeight&&job.windowMinHeight==-12.5f,"covered finite minimum includes last pixel and fractional coverage, excludes NaN/Inf/empty");
    Window(job);Pixel(job,0,std::numeric_limits<float>::quiet_NaN(),1);Pixel(job,1,-std::numeric_limits<float>::infinity(),255);Check(GsmAtlas::WindowMinimum(job)&&!job.windowHasFiniteHeight&&job.windowMinHeight==0,"all covered nonfinite samples do not invent a height");
    Window(job);Pixel(job,42,155.25f,255);Check(GsmAtlas::WindowMinimum(job)&&job.windowMinHeight==155.25f,"positive-only minimum does not default to zero");
    job.read.plane[2].pop_back();Check(!GsmAtlas::WindowMinimum(job)&&!job.windowHasFiniteHeight&&job.windowMinHeight==0,"short height plane refuses and resets previous minimum");
    Window(job);job.read.plane[1].push_back(0);Check(!GsmAtlas::WindowMinimum(job),"oversized coverage plane refused");Window(job);InterlockedExchange(&job.read.cancel,1);Check(!GsmAtlas::WindowMinimum(job)&&!job.windowHasFiniteHeight,"cancelled scan does not publish metadata");
    InterlockedExchange(&job.read.cancel,0);InterlockedIncrement(&GSM::generation);Check(!GsmAtlas::WindowMinimum(job)&&job.read.cancel,"generation mismatch cancels bounded scan");InterlockedExchange(&GSM::generation,0);
}
static void Branches(){
    Dispatch::Reset();GsmAtlas::Job water;water.waterValidation=true;Check(Dispatch::Run(&water)==0&&water.ok&&water.manifest.waterVerified,"actual worker dispatch accepts dedicated water job");Check(Dispatch::waterCalls==1&&!Dispatch::terrainCalls&&!Dispatch::windowCalls&&!Dispatch::minimumCalls,"water validation never reads terrain or scans window heights");
    Dispatch::Reset();Dispatch::success=false;GsmAtlas::Job failed;failed.waterValidation=true;Check(Dispatch::Run(&failed)==1&&!failed.ok&&!failed.manifest.waterVerified&&std::strstr(failed.error,"water validation failure"),"water-only error is retained for main-thread branch");
    Dispatch::Reset();GsmAtlas::Job terrain;terrain.validation=true;Check(Dispatch::Run(&terrain)==0&&terrain.ok&&Dispatch::terrainCalls==1&&!Dispatch::waterCalls&&!Dispatch::minimumCalls,"terrain validation branch remains separate");
    Dispatch::Reset();GsmAtlas::Job window;Window(window);Pixel(window,3,88,255);Check(Dispatch::Run(&window)==0&&window.ok&&Dispatch::windowCalls==1&&Dispatch::minimumCalls==1&&window.windowMinHeight==88,"successful actual Run reads then summarizes one window");
    Dispatch::Reset();Dispatch::success=false;GsmAtlas::Job bad;Window(bad);Pixel(bad,2,-22,255);Check(Dispatch::Run(&bad)==1&&!bad.ok&&!bad.windowHasFiniteHeight&&!Dispatch::minimumCalls,"failed window read never scans or publishes heights");
    Dispatch::Reset();GsmAtlas::Job cancelled;cancelled.waterValidation=true;cancelled.manifest.waterVerified=true;cancelled.ok=true;InterlockedExchange(&cancelled.read.cancel,1);Check(Dispatch::Run(&cancelled)==1&&!Dispatch::waterCalls&&!cancelled.ok&&!cancelled.manifest.waterVerified,"cancelled water job performs no validation and retires stale success");
    Dispatch::Reset();GsmAtlas::Job ambiguous;ambiguous.validation=ambiguous.waterValidation=true;Check(Dispatch::Run(&ambiguous)==1&&!Dispatch::waterCalls&&!Dispatch::terrainCalls&&!Dispatch::windowCalls,"ambiguous worker flags refused before work");
}
static void Sources(const std::string&base){
    const std::string game="D:/SteamLibrary/steamapps/common/Kenshi/";std::ostringstream name;name<<base<<"/water-worker-fixture-"<<GetCurrentProcessId();std::string root=name.str()+"/";
    const unsigned count=sizeof(GsmWaterShaderInputs)/sizeof(GsmWaterShaderInputs[0]);Check(count==5,"exact five generated water shader pins");
    for(unsigned i=0;i<count;++i){const std::string relative=GsmWaterShaderInputs[i].path;Check(relative.find("data/materials/")==0&&relative.find("..") ==relative.npos&&relative.find(':')==relative.npos,"fixture pin path stays within dedicated directory");const std::wstring target=GsmAtlas::Wide(root+relative);Directory(target.substr(0,target.find_last_of(L'\\')));Check(CopyFileW(GsmAtlas::Wide(game+relative).c_str(),target.c_str(),TRUE)!=FALSE,"copy actual pinned source into private fixture");files.push_back(target);}
    GsmAtlas::Job good;good.game=root;good.waterValidation=true;Check(GsmAtlas::Run(&good)==0&&good.ok&&good.manifest.waterVerified,"actual water-only worker hashes all generated source pins");Check(good.read.readBytes>0&&good.read.readBytes<=16*1024*1024&&!good.windowHasFiniteHeight,"water validation has bounded bytes and no window data");
    GsmAtlas::Job cancel;cancel.game=root;cancel.read.afterChunk=Changed;bool stopped=false;try{GsmAtlas::ValidateWaterSources(cancel);}catch(...){stopped=true;}Check(stopped&&cancel.read.cancel&&!cancel.manifest.waterVerified,"generation change during actual source hashing cancels without publishing validation");InterlockedExchange(&GSM::generation,0);
    HANDLE f=CreateFileW(files[0].c_str(),GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);Check(f!=INVALID_HANDLE_VALUE,"corrupt owned source fixture open");DWORD n=0;const char c='?';Check(WriteFile(f,&c,1,&n,NULL)&&n==1&&CloseHandle(f),"corrupt owned source fixture byte");GsmAtlas::Job bad;bad.game=root;bad.waterValidation=true;Check(GsmAtlas::Run(&bad)==1&&!bad.ok&&!bad.manifest.waterVerified&&bad.error[0],"corrupt source refuses water validation only");
    Check(CopyFileW(GsmAtlas::Wide(game+GsmWaterShaderInputs[0].path).c_str(),files[0].c_str(),FALSE)!=FALSE,"restore private source fixture");f=CreateFileW(files[0].c_str(),GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);Check(f!=INVALID_HANDLE_VALUE,"large private fixture open");LARGE_INTEGER end;end.QuadPart=16*1024*1024+1;Check(SetFilePointerEx(f,end,NULL,FILE_BEGIN)&&SetEndOfFile(f)&&CloseHandle(f),"bounded source oversize fixture");GsmAtlas::Job large;large.game=root;large.waterValidation=true;Check(GsmAtlas::Run(&large)==1&&!large.ok&&!large.manifest.waterVerified&&large.read.readBytes==0,"sixteen MiB source cap refuses before reading oversized file");
    Check(CopyFileW(GsmAtlas::Wide(game+GsmWaterShaderInputs[0].path).c_str(),files[0].c_str(),FALSE)&&DeleteFileW(files[1].c_str()),"prepare missing owned source fixture");GsmAtlas::Job missing;missing.game=root;missing.waterValidation=true;missing.manifest.waterVerified=true;Check(GsmAtlas::Run(&missing)==1&&!missing.ok&&!missing.manifest.waterVerified&&missing.error[0],"missing source failure retires prior water verification");
}
int main(int argc,char**argv){if(argc!=2)return 2;try{Minimum();Branches();Sources(argv[1]);Cleanup();std::printf("Water source/window metadata PASS %u checks\n",checks);return 0;}catch(...){Cleanup();return 1;}}
