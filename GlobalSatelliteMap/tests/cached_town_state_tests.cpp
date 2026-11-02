#define NOMINMAX
#define GSM_CACHED_TOWN_POLICY_ONLY
#include <Windows.h>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <cstring>
namespace GsmTownValues {
struct TownValue {unsigned id;float storedXYZ[3];bool hasProxy;TownValue():id(0),hasProxy(false){std::memset(storedXYZ,0,12);}};
struct Result {LONG generation;unsigned count;TownValue towns[4096];Result():generation(1),count(0){}};
}
#ifdef GSM_CACHED_TOWN_HEADER
#include GSM_CACHED_TOWN_HEADER
#else
#include "../GsmCachedTownState.h"
#endif
struct FakeBounds {float lo[3],hi[3];FakeBounds(){std::memset(this,0,sizeof(*this));}};
struct FakePacket {unsigned generation,count;FakePacket():generation(0),count(0){}};
struct FakeOps {
 typedef FakeBounds Bounds;typedef FakePacket Packet;
 static FakeOps*self;static bool loaded[256],proxy[256];static unsigned checks,reads,retire;static bool clearOk,readFail,throwInspect,raceRead,proxyAfter;static volatile LONG*live;
 AtlasView::U64 owned;ID3D11Device*device;Packet packet;float matrix[16];
 FakeOps():owned(0),device(NULL){self=this;}
 HRESULT Inspect(const GsmCachedTownState::Context&c,const GsmCachedTown::Request&r){++checks;if(throwInspect)throw std::runtime_error("inspection fixture failure");if(!live||*live!=c.generation)return E_ABORT;if(r.townId>=256||!loaded[r.townId]||proxy[r.townId])return S_FALSE;return S_OK;}
 bool Clear(){if(owned){++retire;if(!clearOk)return false;}owned=0;device=NULL;packet=Packet();return true;}
 HRESULT Read(const GsmCachedTown::Request&r,const GsmCachedTown::Result&value,unsigned generation,const float*world,const float*,ID3D11Device*d,bool,AtlasView::U64){++reads;owned=value.info.GpuReservation();device=d;packet.generation=generation;packet.count=1;std::memcpy(matrix,world,64);if(raceRead)InterlockedIncrement(live);if(proxyAfter)proxy[r.townId]=true;return readFail?E_FAIL:S_OK;}
 bool Valid(unsigned g)const{return packet.count&&packet.generation==g;}
 bool DeviceMatches(ID3D11Device*d)const{return device==d;}
 const Packet&Geometry()const{return packet;}
 AtlasView::U64 OwnedGpuBytes()const{return owned;}SIZE_T CpuPayloadBytes()const{return 0;}
 const char*LastError()const{return "native fixture refused";}
 bool ComputeBounds(const float*w,const float*origin,const float*local,Bounds&out){for(unsigned k=0;k<3;++k){out.lo[k]=w[k*4+3]+origin[k]+local[k];out.hi[k]=w[k*4+3]+origin[k]+local[k+3];}return true;}
 static void Reset(volatile LONG*l){live=l;checks=reads=retire=0;clearOk=true;readFail=throwInspect=raceRead=proxyAfter=false;std::memset(loaded,1,sizeof(loaded));std::memset(proxy,0,sizeof(proxy));}
};
FakeOps*FakeOps::self=NULL;bool FakeOps::loaded[256],FakeOps::proxy[256];unsigned FakeOps::checks=0,FakeOps::reads=0,FakeOps::retire=0;bool FakeOps::clearOk=true,FakeOps::readFail=false,FakeOps::throwInspect=false,FakeOps::raceRead=false,FakeOps::proxyAfter=false;volatile LONG*FakeOps::live=NULL;
typedef GsmCachedTownState::Control<FakeOps> State;
static unsigned assertions=0;
struct OwnerFixture {static unsigned born,cleared,destroyed;static bool allowed;OwnerFixture(){++born;}~OwnerFixture(){++destroyed;}bool Clear(){++cleared;return allowed;}};
unsigned OwnerFixture::born=0;unsigned OwnerFixture::cleared=0;unsigned OwnerFixture::destroyed=0;bool OwnerFixture::allowed=true;
static void Check(bool b,const char*s){++assertions;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static ID3D11Device*Device(){return reinterpret_cast<ID3D11Device*>(7);}
static GsmTownValues::Result Values(unsigned n=2){GsmTownValues::Result v;v.count=n;for(unsigned i=0;i<n;++i){v.towns[i].id=i+2;v.towns[i].storedXYZ[0]=1010.f+i*2000;v.towns[i].storedXYZ[1]=100;v.towns[i].storedXYZ[2]=0;}return v;}
static const float origin[3]={1000,0,0},camera[3]={10,0,0};
static HRESULT Select(State&s,const GsmTownValues::Result&v,volatile LONG*live,const float*cam=camera,const float*o=origin,const void*scene=reinterpret_cast<void*>(3)){return s.Select(v,cam,o,scene,1,live,reinterpret_cast<void*>(4),reinterpret_cast<void*>(5));}
static HRESULT Adopt(State&s,const GsmCachedTown::Request&r,const GsmCachedTown::Result&v,LONG gen=1,const float*o=origin,AtlasView::U64 other=0){return s.Adopt(r,v,gen,o,reinterpret_cast<void*>(3),Device(),true,other);}
static GsmCachedTown::Request Launch(State&s){Check(s.WantsRequest(),"eligible pending request");GsmCachedTown::Request r=s.RequestValue();s.MarkLaunched();Check(!s.WantsRequest(),"launched request cannot launch twice");return r;}
static void Load(State&s,GsmTownValues::Result&v,volatile LONG*live,const GsmCachedTown::Result&ready){Check(SUCCEEDED(Select(s,v,live)),"select succeeds");GsmCachedTown::Request r=Launch(s);Check(Adopt(s,r,ready)==S_OK&&s.Valid(1),"one ready result imports exactly once");}
int main(int argc,char**argv){try{
 {typedef GsmCachedTownState::ExplicitOwner<OwnerFixture> Owner;{Owner empty;Check(!empty.Get()&&empty.Clear()&&!OwnerFixture::born&&!OwnerFixture::cleared,"OFF owner allocates and calls nothing");}OwnerFixture*retained=NULL;{Owner live;retained=&live.Ensure();Check(live.Get()==retained&&&live.Ensure()==retained&&OwnerFixture::born==1,"one lazy owner remains at stable address");}Check(!OwnerFixture::cleared&&!OwnerFixture::destroyed,"late owner destructor makes zero native cleanup calls");delete retained;
  {Owner live;live.Ensure();Check(live.Clear()&&!live.Get()&&OwnerFixture::cleared==1&&OwnerFixture::destroyed==2,"explicit alive cleanup clears then destroys once");Check(live.Clear()&&OwnerFixture::cleared==1,"repeated explicit clear is empty");}
  {Owner live;retained=&live.Ensure();OwnerFixture::allowed=false;Check(!live.Clear()&&live.Get()==retained&&OwnerFixture::destroyed==2,"uncertain cleanup retains stable owner and native graph");}Check(OwnerFixture::destroyed==2&&OwnerFixture::cleared==2,"quarantined destructor cannot retry native cleanup");OwnerFixture::allowed=true;delete retained;}
 Check(argc==2,"actual Hub source argument");FILE*f=NULL;fopen_s(&f,argv[1],"rb");Check(f!=NULL,"actual Hub fixture available");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);GsmCachedTown::Result ready;ready.bytes.resize(n);Check(fread(&ready.bytes[0],1,n,f)==SIZE_T(n),"actual Hub bytes read");fclose(f);Check(PrivateTownLod::Parser(&ready.bytes[0],ready.bytes.size()).Read(ready.info),"actual production blob preflight");ready.state=GsmCachedTown::Ready;ready.sha256[0]=1;
 {volatile LONG live=1;FakeOps::Reset(&live);State s;Check(!s.WantsRequest()&&!s.Valid(1)&&!s.OwnedGpuBytes(),"default is inactive");Check(s.Clear()&&!FakeOps::checks&&!FakeOps::reads,"fresh clear no native queries");}
 {volatile LONG live=2;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Check(FAILED(Select(s,v,&live))&&!FakeOps::checks&&!FakeOps::reads,"stale generation refuses before queries");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();v.towns[1].storedXYZ[0]=-990;Select(s,v,&live);Check(s.WantsRequest()&&s.RequestValue().townId==2&&FakeOps::checks==1,"nearest uses raw camera plus proven origin");GsmCachedTown::Request r=s.RequestValue();Check(FAILED(Adopt(s,r,ready))&&!FakeOps::reads,"unlaunched completion refused");Load(s,v,&live,ready);Check(FakeOps::reads==1&&FakeOps::self->matrix[3]==10&&FakeOps::self->matrix[7]==100,"identity pose uses absolute minus origin");Check(s.Bounds().lo[0]==1010+ready.info.localBounds[0],"world bounds use actual parsed values");unsigned rev=s.Revision();Select(s,v,&live);Check(!s.WantsRequest()&&FakeOps::reads==1&&s.Revision()==rev,"same valid source is stable");v.towns[0].hasProxy=true;Select(s,v,&live);Check(!s.Valid(1)&&!s.OwnedGpuBytes()&&s.Revision()!=rev,"native proxy appearance retires private source");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values(20);for(unsigned i=0;i<20;++i){v.towns[i].storedXYZ[0]=1020.f+i*10;FakeOps::loaded[i+2]=false;}FakeOps::loaded[9]=true;Select(s,v,&live);Check(FakeOps::checks==4&&!s.WantsRequest(),"at most four native checks per selection");Select(s,v,&live);Check(FakeOps::checks<=8&&s.WantsRequest()&&s.RequestValue().townId==9,"unloaded cooldown cannot starve next loaded candidate");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();v.towns[0].storedXYZ[1]=1;v.towns[1].hasProxy=true;Select(s,v,&live);Check(!s.WantsRequest()&&!FakeOps::checks,"unresolved Y and proxy excluded before queries");v.towns[0].storedXYZ[1]=100;v.towns[0].storedXYZ[0]=20000;Select(s,v,&live);Check(!s.WantsRequest()&&!FakeOps::checks,"outside8192 excluded before queries");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Load(s,v,&live,ready);float nearNext[3]={2010,0,0};Select(s,v,&live,nearNext);GsmCachedTown::Request r=Launch(s);Check(r.townId==3,"nearer replacement selected");GsmCachedTown::Result unavailable;unavailable.state=GsmCachedTown::Unavailable;unsigned reads=FakeOps::reads;Check(Adopt(s,r,unavailable)==S_FALSE&&s.Valid(1)&&s.TownId()==2&&FakeOps::reads==reads,"unavailable replacement keeps ready old slot");Select(s,v,&live,nearNext);Check(!s.WantsRequest(),"missing source memo prevents repeat IO");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s),wrong=r;wrong.townId=99;Check(FAILED(Adopt(s,wrong,ready))&&!FakeOps::reads,"wrong completion ID refuses before native read");GsmCachedTown::Result bad=ready;bad.info.localBounds[0]+=1;Check(FAILED(Adopt(s,r,bad))&&!FakeOps::reads,"worker metadata must match reparsed bytes");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);Check(FAILED(Adopt(s,r,ready,1,origin,AtlasView::GpuCap-ready.info.GpuReservation()+1))&&!FakeOps::reads,"inclusive GPU bound plus1 refuses before import");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);Check(Adopt(s,r,ready,1,origin,AtlasView::GpuCap-ready.info.GpuReservation())==S_OK,"inclusive GPU budget accepted");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);FakeOps::raceRead=true;Check(FAILED(Adopt(s,r,ready))&&!s.Valid(1)&&!s.OwnedGpuBytes()&&FakeOps::retire==1,"generation change during import clears and never publishes");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);FakeOps::proxyAfter=true;Check(FAILED(Adopt(s,r,ready))&&!s.Valid(1)&&!s.OwnedGpuBytes(),"proxy appearance during import clears new source");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Load(s,v,&live,ready);FakeOps::clearOk=false;Check(!s.Clear()&&!s.Valid(1)&&s.OwnedGpuBytes()==ready.info.GpuReservation(),"uncertain cleanup stays charged and invalid");unsigned checks=FakeOps::checks;FakeOps::clearOk=true;Select(s,v,&live);Check(!s.WantsRequest()&&FakeOps::checks==checks,"quarantine refuses new native work");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);FakeOps::readFail=true;Check(FAILED(Adopt(s,r,ready))&&!s.Valid(1)&&!s.OwnedGpuBytes()&&*s.LastError(),"partial import failure is cleaned and locally latched");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Load(s,v,&live,ready);float shifted[3]={1001,0,0};Select(s,v,&live,camera,shifted);Check(!s.Valid(1)&&!s.OwnedGpuBytes(),"origin change retires old pose");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Load(s,v,&live,ready);v.count=0;Select(s,v,&live);Check(!s.Valid(1)&&!s.OwnedGpuBytes(),"removed town retires old source");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);InterlockedIncrement(&live);Check(!s.WantsRequest(),"generation change cancels unlaunched request immediately");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Load(s,v,&live,ready);InterlockedIncrement(&live);Check(!s.Valid(1)&&s.OwnedGpuBytes()==ready.info.GpuReservation(),"generation invalidates display immediately while owner remains charged");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values(65);for(unsigned k=0;k<65;++k)v.towns[k].storedXYZ[0]=1020.f+k*10;GsmCachedTown::Result missing;missing.state=GsmCachedTown::Unavailable;
  for(unsigned k=0;k<64;++k){Select(s,v,&live);GsmCachedTown::Request r=Launch(s);Check(Adopt(s,r,missing)==S_FALSE,"bounded missing memo records exactly one completed attempt");}
  unsigned checks=FakeOps::checks;Select(s,v,&live);Check(!s.WantsRequest()&&FakeOps::checks==checks&&*s.LastError(),"64 memo limit refuses growth and repeated queries");s.Clear();Select(s,v,&live);Check(s.WantsRequest(),"explicit clear releases attempt memo");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);float wrongOrigin[3]={1001,0,0};Check(FAILED(Adopt(s,r,ready,1,wrongOrigin))&&!FakeOps::reads,"completion wrong origin refuses before import");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();Select(s,v,&live);GsmCachedTown::Request r=Launch(s);FakeOps::proxy[r.townId]=true;Check(Adopt(s,r,ready)==S_FALSE&&!FakeOps::reads,"proxy appearing before adoption prevents duplicate model");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();v.towns[1]=v.towns[0];Select(s,v,&live);Check(!s.WantsRequest()&&!FakeOps::checks,"duplicate candidate ID refused before native queries");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values();FakeOps::throwInspect=true;Check(FAILED(Select(s,v,&live))&&!s.WantsRequest()&&*s.LastError(),"native inspection exception is private-layer failure");unsigned before=FakeOps::checks;FakeOps::throwInspect=false;Select(s,v,&live);Check(FakeOps::checks==before&&!s.WantsRequest(),"native failure latches without hot retries until clear");s.Clear();Select(s,v,&live);Check(s.WantsRequest(),"explicit clear resets nonquarantined failure latch");}
 {volatile LONG live=1;FakeOps::Reset(&live);State s;GsmTownValues::Result v=Values(1);v.towns[0].storedXYZ[0]=5010;Load(s,v,&live,ready);v.count=5;for(unsigned k=1;k<5;++k){v.towns[k].id=k+2;v.towns[k].storedXYZ[0]=1020.f+k*10;v.towns[k].storedXYZ[1]=100;FakeOps::loaded[k+2]=false;}unsigned before=FakeOps::checks;Select(s,v,&live);Check(FakeOps::checks-before==4,"active validation shares four-check budget with candidates");}
 std::printf("Actual cached town State PASS %u checks\n",assertions);return 0;
 }catch(...){return 1;}}
