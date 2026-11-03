#define _CRT_SECURE_NO_WARNINGS
#include "WorldStream.h"
#include <cstdio>
#include <fstream>
using namespace WorldStream;
static unsigned checks=0;static void C(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static std::vector<BYTE>Read(const char*path){std::ifstream f(path,std::ios::binary|std::ios::ate);C(!!f,"raw fixture open");size_t bytes=size_t(f.tellg());std::vector<BYTE>b(bytes);f.seekg(0);f.read(reinterpret_cast<char*>(&b[0]),bytes);C(!!f,"raw fixture read");return b;}
static void Send(Work&w,Command command){InterlockedExchange(&w.state,WBusy);InterlockedExchange(&w.command,command);C(SetEvent(w.wake)!=FALSE,"worker command event");}
static LONG Wait(Work&w){DWORD start=GetTickCount();LONG state;do{state=InterlockedCompareExchange(&w.state,0,0);if(state!=WBusy&&state!=WBoot)return state;Sleep(1);}while(GetTickCount()-start<10000);throw 2;}
int main(){try{
 Work*w=new Work;w->wake=CreateEvent(NULL,FALSE,FALSE,NULL);w->cancel=CreateEvent(NULL,TRUE,FALSE,NULL);C(w->wake&&w->cancel,"owned worker events");HANDLE thread=CreateThread(NULL,0,Work::Run,w,0,NULL);C(thread&&Wait(*w)==WIdle,"actual WIC worker boot");
 w->epoch=0x100000007ULL;w->side=256;w->region.x0=w->region.z0=7360;w->region.x1=w->region.z1=7488;std::wcscpy(w->path,L"D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif");Send(*w,Height);C(Wait(*w)==WHeight&&w->heightOpens==1&&w->window.Ready(),"actual worker authenticated height window");
 std::vector<BYTE>material=Read("../raw-rgb/_generated/control3-material.rgba16f"),normal=Read("../raw-rgb/_generated/control3-normal.rgba8"),radial=Read("../raw-rgb/_generated/control3-radial.r32f");const size_t n=256*256;C(w->pixels.size()==n*16,"one raw readback slab");std::memcpy(&w->pixels[0],&material[0],n*8);std::memcpy(&w->pixels[n*8],&normal[0],n*4);std::memcpy(&w->pixels[n*12],&radial[0],n*4);
 SourceReceipts::Scope scope={GetCurrentThreadId(),7,w->epoch,0x300000001ULL};WorldRaster::Frame frame={};frame.source.value=w->epoch;frame.source.profile=WorldBank::PinnedBaseSnapshot;frame.generation=scope.job;frame.materialGeneration=0x200000001ULL;frame.region=27;frame.cell=10;frame.side=256;frame.x=frame.z=-13824;frame.spacing=3;frame.cameraY=10000;frame.farClip=20000;frame.waterHeight=-10000;
 w->pixelScope=scope;w->pixelFrame=frame;w->pixelOtherCpu=sizeof(Work)+w->window.OwnedBytes()+w->pixels.capacity()+material.capacity()+normal.capacity()+radial.capacity()+WorldHeight::AuthScratch;const U64 other=w->pixelOtherCpu;WorldPixels::View view={};
 Send(*w,ValidatePixels);C(Wait(*w)==WValidated&&w->accepted.OwnedBytes()==WorldPixels::Accepted::RequiredBytes(256),"worker alone constructs exact charged acceptance slab");C(w->accepted.Acquire(true,scope,frame,w->window,view)&&!std::memcmp(view.material,&material[0],material.size())&&!std::memcmp(view.normal,&normal[0],normal.size()),"actual worker validated material/normal bits preserved");
 for(unsigned edit=0;edit<3;++edit){SourceReceipts::Scope changedScope=scope;WorldRaster::Frame changedFrame=frame;if(edit==0)changedScope.world^=1ULL<<40;if(edit==1)changedFrame.generation^=1ULL<<40;if(edit==2)changedFrame.materialGeneration^=1ULL<<40;C(!w->accepted.Acquire(true,changedScope,changedFrame,w->window,view)&&!view.material&&!view.height,"accepted worker output remains exact-scope/frame bound");}
 C(!w->accepted.Acquire(false,scope,frame,w->window,view),"OFF cannot acquire worker output");
 const unsigned short invalid=0x7c00;std::memcpy(&w->pixels[0],&invalid,2);Send(*w,ValidatePixels);C(Wait(*w)==WError&&w->error==E_INVALIDARG&&!w->accepted.OwnedBytes(),"invalid raw readback fails actual worker and withdraws old acceptance");std::memcpy(&w->pixels[0],&material[0],2);
 w->pixelOtherCpu=SourceReceipts::CpuCap-WorldPixels::Accepted::RequiredBytes(256)+1;Send(*w,ValidatePixels);C(Wait(*w)==WError&&w->error==E_OUTOFMEMORY&&!w->accepted.OwnedBytes(),"worker enforces aggregate CPU cap before output allocation");w->pixelOtherCpu=other;
 Send(*w,ValidatePixels);C(Wait(*w)==WValidated&&w->accepted.Acquire(true,scope,frame,w->window,view),"worker recovers valid acceptance after refusal");C(w->heightOpens==1,"pixel revalidation never reauthenticates height file");
 SetEvent(w->cancel);Send(*w,Close);C(Wait(*w)==WClosed&&!w->accepted.OwnedBytes()&&!w->window.Ready()&&!w->height.Ready()&&!w->pixels.capacity(),"Close clears accepted/raw/source owners on the worker despite cancel event");
 Send(*w,Stop);C(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0&&InterlockedCompareExchange(&w->state,0,0)==WStopped,"joined worker exits after cleanup");CloseHandle(thread);CloseHandle(w->wake);CloseHandle(w->cancel);delete w;printf("PASS %u real worker acceptance/refusal/CPU/close controls\n",checks);return 0;
 }catch(...){return 1;}}
