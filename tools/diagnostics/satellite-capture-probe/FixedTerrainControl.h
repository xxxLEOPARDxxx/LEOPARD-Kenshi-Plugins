#pragma once
// PRIVATE fixed base-fixture diagnostic. No Ogre/native camera/scene ownership.
#include "../../../GlobalSatelliteMap/atlas-terrain/raster/FixedTerrainRaster.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/compact-material/_generated/Shaders.h"
#include <string>
#include <vector>
#include <cstdio>
#include <stdexcept>
namespace FixedTerrainControl {
typedef AtlasView::U64 U64;
const U64 RasterBytes=1746024,StageBytes=524288,GpuBytes=CompactMaterial::SourceBytes+RasterBytes+StageBytes;
enum{PlaneBytes=256*256*4,MaxBlob=2048*2048*4,CpuLimit=64*1024*1024};
// Fixed-size artifact header, LE on the pinned x64 host. Never a GST/GSI file.
struct Header{char magic[8];unsigned version,side,view,token,pid,epoch;float x,z,spacing,cameraY,farClip;unsigned format[3],planeBytes;unsigned char heightSha[32],materialSha[32];unsigned reserved[3];};
typedef char HeaderSize[(sizeof(Header)==144)?1:-1];
inline bool Guard(bool startupOff,bool currentOff,bool texturesAbsent,bool world,bool map,bool saveIdle,bool profile,bool context){return startupOff&&currentOff&&texturesAbsent&&world&&map&&saveIdle&&profile&&context;}
inline void Require(bool yes,const char*what){if(!yes)throw std::runtime_error(what);}
inline void Hr(HRESULT hr,const char*what){Require(SUCCEEDED(hr),what);}
struct Handle{HANDLE h;Handle():h(NULL){}~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}private:Handle(const Handle&);Handle&operator=(const Handle&);};
inline void Hex(const char*text,unsigned char(&out)[32]){for(unsigned i=0;i<32;++i){unsigned v=0;std::sscanf(text+i*2,"%2x",&v);out[i]=static_cast<unsigned char>(v);}}
struct Job{
 volatile LONG cancel,state;HANDLE wake;DWORD error;unsigned index,token,pid,epoch;char message[128];std::string inputs,output;std::vector<BYTE>blob;
 Job():cancel(0),state(0),wake(NULL),error(0),index(0),token(0),pid(0),epoch(0){message[0]=0;}
};
// States:0 reading,1 typed input ready,2 awaiting captured output,3 writing,
// 4 completed,5 failed/cancelled. Event wakes only this owned worker.
inline bool Cancelled(Job&j){return InterlockedCompareExchange(&j.cancel,0,0)!=0;}
inline void ReleaseBlob(Job&j){std::vector<BYTE>().swap(j.blob);}
inline void Read(Job&j,const std::string&path,unsigned bytes,const unsigned char(&pin)[32]){
 Require(bytes&&bytes<=MaxBlob,"fixed input bound");Handle f;f.h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);LARGE_INTEGER n={};Require(f.h!=INVALID_HANDLE_VALUE&&GetFileSizeEx(f.h,&n)&&n.QuadPart==bytes,"fixed input size/open");
 j.blob.resize(bytes);for(unsigned at=0;at<bytes;){Require(!Cancelled(j),"fixed input cancelled");DWORD want=bytes-at;if(want>65536)want=65536;DWORD got=0;Require(ReadFile(f.h,&j.blob[at],want,&got,NULL)&&got==want,"fixed input read");at+=got;}
 unsigned char actual[32];Require(AtlasView::Digest(&j.blob[0],bytes,actual)&&!std::memcmp(actual,pin,32),"fixed input hash");
}
inline bool Wait(Job&j,LONG wanted){while(!Cancelled(j)){if(InterlockedCompareExchange(&j.state,0,0)==wanted)return true;DWORD r=WaitForSingleObject(j.wake,100);if(r!=WAIT_OBJECT_0&&r!=WAIT_TIMEOUT)return false;}return false;}
inline void Write(Job&j,const std::string&name){
 const std::string path=j.output+name+".partial";Handle f;f.h=CreateFileA(path.c_str(),GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);Require(f.h!=INVALID_HANDLE_VALUE,"fixed private output create");
 for(unsigned at=0;at<j.blob.size();){Require(!Cancelled(j),"fixed output cancelled");DWORD want=unsigned(j.blob.size()-at);if(want>65536)want=65536;DWORD done=0;Require(WriteFile(f.h,&j.blob[at],want,&done,NULL)&&done==want,"fixed output write");at+=done;}
 Require(FlushFileBuffers(f.h)!=FALSE,"fixed output flush");CloseHandle(f.h);f.h=NULL;Require(!Cancelled(j)&&MoveFileExA(path.c_str(),(j.output+name).c_str(),0),"fixed private output publish");
}
inline DWORD WINAPI Worker(void*value){Job&j=*static_cast<Job*>(value);try{
 Require(!Cancelled(j)&&CreateDirectoryA(j.output.substr(0,j.output.size()-1).c_str(),NULL),"fixed fresh output directory");
 for(unsigned input=0;input<16;++input){ReleaseBlob(j);unsigned char height[32];Hex("4c2943629387364ee0bfd9f630656affd94fdc7671ab0754b5199e8f3109d3f2",height);char name[40];const unsigned char*pin=NULL;unsigned bytes=0;
  if(input==0){std::strcpy(name,"height.u16");bytes=133*133*2;pin=height;}else if(input<=12){std::sprintf(name,"dds%02u.bin",input-1);bytes=CompactMaterial::DdsPins[input-1].bytes;pin=CompactMaterial::DdsPins[input-1].sha;}else{std::sprintf(name,"image%u.rgba",input-13);bytes=CompactMaterial::RgbaPins[input-13].bytes;pin=CompactMaterial::RgbaPins[input-13].sha;}
  unsigned char expected[32];std::memcpy(expected,pin,32);Read(j,j.inputs+name,bytes,expected);j.index=input;InterlockedExchange(&j.state,1);Require(Wait(j,0),"fixed upload acknowledgement cancelled");
 }
 ReleaseBlob(j);InterlockedExchange(&j.state,2);
 for(unsigned view=0;view<4;++view){Require(Wait(j,3),"fixed capture cancelled");Require(j.blob.size()==sizeof(Header)+3*PlaneBytes,"fixed captured payload bound");char name[32];std::sprintf(name,"view-%u.bin",view);Write(j,name);ReleaseBlob(j);InterlockedExchange(&j.state,2);}
 const char text[]="Fixed terrain base-fixture diagnostic v1\r\n4 views:1m,3m,9m,shift3m;256x256;packedRGBA8/nativeNormalRGBA8/R32radial\r\nGeometry18m/native40B/morph0;material cell10,base source pins/frozen defaults only.\r\nNot active resource winner proof; no native lighting/water/models/final atlas.\r\n";j.blob.assign(text,text+sizeof(text)-1);Write(j,"complete.txt");ReleaseBlob(j);InterlockedExchange(&j.state,4);return 0;
 }catch(const std::exception&e){strncpy_s(j.message,sizeof(j.message),e.what(),_TRUNCATE);j.error=Cancelled(j)?ERROR_CANCELLED:ERROR_INVALID_DATA;InterlockedExchange(&j.state,5);return j.error;}catch(...){strcpy_s(j.message,sizeof(j.message),"unknown fixed worker error");j.error=Cancelled(j)?ERROR_CANCELLED:ERROR_INVALID_DATA;InterlockedExchange(&j.state,5);return j.error;}}
class Controller{
 Job*job_;HANDLE thread_;AtlasView::Com<ID3D11Device>device_;CompactMaterial::Inputs inputs_;FixedTerrainRaster::Raster raster_;AtlasView::Com<ID3D11Texture2D>stage_[2];unsigned view_,epoch_;bool uploaded_,quarantined_;
 Controller(const Controller&);Controller&operator=(const Controller&);
 void GpuClear(){stage_[0].Reset();stage_[1].Reset();raster_.Clear();inputs_.Clear();device_.Reset();view_=epoch_=0;uploaded_=false;}
public:
 Controller():job_(NULL),thread_(NULL),view_(0),epoch_(0),uploaded_(false),quarantined_(false){}
 ~Controller(){Cancel();if(!quarantined_&&thread_&&WaitForSingleObject(thread_,0)==WAIT_OBJECT_0){CloseHandle(thread_);CloseHandle(job_->wake);delete job_;}/* Active/unknown CPU job is retained until process exit; never delete beneath worker. */}
 bool Active()const{return job_!=NULL;}
 bool Stopping()const{return job_&&Cancelled(*job_);}
 bool Quarantined()const{return quarantined_;}
 unsigned Epoch()const{return epoch_;}
 bool SameDevice(ID3D11Device*d)const{return AtlasView::Identity(device_.p,d);}
 const std::string&Output()const{static const std::string empty;return job_?job_->output:empty;}
 void Cancel(){if(job_){InterlockedExchange(&job_->cancel,1);SetEvent(job_->wake);}GpuClear();}
 // True means exact join OR the single newly quarantined event. Check Quarantined.
 // Unknown thread identity is never waited/closed/deleted again, or replaced.
 bool Poll(DWORD&error,std::string&output){if(!thread_||quarantined_)return false;DWORD wait=WaitForSingleObject(thread_,0);if(wait==WAIT_TIMEOUT)return false;if(wait!=WAIT_OBJECT_0){error=wait==WAIT_FAILED?GetLastError():ERROR_GEN_FAILURE;if(!error)error=ERROR_INVALID_HANDLE;output=job_->output;Cancel();quarantined_=true;return true;}error=job_->error;output=job_->output;CloseHandle(thread_);thread_=NULL;CloseHandle(job_->wake);delete job_;job_=NULL;GpuClear();return true;}
 void Begin(ID3D11Device*d,const std::string&folder,unsigned token,unsigned epoch,bool admitted){
  Require(admitted&&d&&token&&epoch&&!job_&&GpuBytes<=CompactMaterial::Limit&&GpuBytes<=AtlasView::GpuCap,"fixed control admission");Job*j=new Job;try{j->inputs=folder+"fixed-terrain-inputs/";char name[100];std::sprintf(name,"fixed-terrain-%u-%u-%u/",GetCurrentProcessId(),token,GetTickCount());j->output=folder+name;j->token=token;j->pid=GetCurrentProcessId();j->epoch=epoch;j->wake=CreateEventA(NULL,FALSE,FALSE,NULL);Require(j->wake!=NULL,"fixed worker event");device_.p=d;d->AddRef();epoch_=epoch;thread_=CreateThread(NULL,0,Worker,j,0,NULL);Require(thread_!=NULL,"fixed CPU worker");job_=j;}catch(...){if(j->wake)CloseHandle(j->wake);delete j;GpuClear();throw;}
 }
 void Step(bool admitted){
  Require(job_&&admitted&&!Cancelled(*job_)&&device_.p,"fixed capture context invalidated");Job&j=*job_;LONG state=InterlockedCompareExchange(&j.state,0,0);if(state==5)throw std::runtime_error(j.message[0]?j.message:"fixed CPU input/output refused");if(state==1){const void*data=&j.blob[0];unsigned bytes=unsigned(j.blob.size());if(j.index==0){FixedTerrain::Samples s={7358,7358,133,133,133,static_cast<const unsigned short*>(data),133*133};FixedTerrainRaster::Grid g={7360,7360,2,2,epoch_};FixedTerrainRaster::Shaders shaders={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{CompactMaterial::PixelShader,sizeof(CompactMaterial::PixelShader)}};Hr(raster_.Init(device_.p,shaders,s,g,CompactMaterial::SourceBytes+StageBytes),"fixed raster Init");Hr(inputs_.Init(device_.p,RasterBytes+StageBytes),"fixed source Init");D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=256;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;for(unsigned i=0;i<2;++i){td.Format=i?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;Hr(device_.p->CreateTexture2D(&td,NULL,&stage_[i].p),"fixed staging allocation");}}
   else if(j.index<=12)Hr(inputs_.UploadDds(j.index-1,data,bytes),"fixed typed DDS upload");else Hr(inputs_.UploadRgba(j.index-13,data,bytes),"fixed typed RGBA upload");if(j.index==15){Hr(inputs_.Complete(),"fixed staged source completion");uploaded_=true;}InterlockedExchange(&j.state,0);SetEvent(j.wake);return;}
  if(state!=2||!uploaded_||view_>=4)return;
  Require(inputs_.OwnedGpuBytes()+raster_.OwnedGpuBytes()+StageBytes==GpuBytes,"fixed actual GPU ledger");const float spacing[4]={1,3,9,3},center[4]={-13824,-13824,-13824,-13632};FixedTerrainRaster::Frame frame={epoch_,center[view_],center[view_],spacing[view_],10000,20000,-10000,0};
  j.blob.resize(sizeof(Header)+3*PlaneBytes);Header h={};std::memcpy(h.magic,"GFTC0001",8);h.version=1;h.side=256;h.view=view_;h.token=j.token;h.pid=j.pid;h.epoch=epoch_;h.x=frame.x;h.z=frame.z;h.spacing=frame.spacing;h.cameraY=frame.cameraY;h.farClip=frame.farClip;h.format[0]=h.format[1]=DXGI_FORMAT_R8G8B8A8_UNORM;h.format[2]=DXGI_FORMAT_R32_FLOAT;h.planeBytes=PlaneBytes;Hex("4c2943629387364ee0bfd9f630656affd94fdc7671ab0754b5199e8f3109d3f2",h.heightSha);Hex("afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9",h.materialSha);std::memcpy(&j.blob[0],&h,sizeof(h));
  Hr(raster_.Render(true,inputs_,frame,CompactMaterial::SourceBytes+StageBytes),"fixed context-safe raster draw");AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);CompactMaterial::SavedPredicate predicate(c.p);
  for(unsigned plane=0;plane<3;++plane){ID3D11Texture2D*read=stage_[plane==2?1:0].p;c.p->CopyResource(read,raster_.Texture(plane));D3D11_MAPPED_SUBRESOURCE m;Hr(c.p->Map(read,0,D3D11_MAP_READ,0,&m),"fixed private readback");for(unsigned y=0;y<256;++y)std::memcpy(&j.blob[sizeof(Header)+plane*PlaneBytes+y*1024],static_cast<const BYTE*>(m.pData)+y*m.RowPitch,1024);c.p->Unmap(read,0);}
  ++view_;InterlockedExchange(&j.state,3);SetEvent(j.wake);
 }
};
}
