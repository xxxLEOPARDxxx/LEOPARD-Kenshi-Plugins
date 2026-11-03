#define _CRT_SECURE_NO_WARNINGS
#include "StageAdapter.h"
#include <cstdio>
static unsigned checks=0;
static unsigned failCaptureSlot=4;
static void C(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct FakeStage{
 SourceReceipts::Scope scope;unsigned bound,captured,refreshed,begun,ended;bool active;SourceReceipts::Role roles[15];
 FakeStage():bound(0),captured(0),refreshed(0),begun(0),ended(0),active(false){std::memset(&scope,0,sizeof(scope));}
 HRESULT Init(const SourceReceipts::Scope&s){if(scope.job)return E_UNEXPECTED;scope=s;return S_OK;}
 HRESULT BindOwned(const SourceReceipts::Role&r){if(!scope.job||bound>=15)return E_INVALIDARG;roles[bound++]=r;return S_OK;}
 HRESULT Capture(const SourceReceipts::WorkerRequest&r,SourceReceipts::MainSnapshot&out){
  if(r.role.slot==failCaptureSlot)return E_ACCESSDENIED;
  if(bound!=15)return E_UNEXPECTED;bool found=false;for(unsigned i=0;i<bound;++i)found=SourceReceipts::SameRole(r.role,roles[i])||found;if(!found)return E_INVALIDARG;
  std::memset(&out,0,sizeof(out));out.request=r;out.manager=1;out.group=2;out.archive=3;out.module=4;out.profile=5;out.listener=0x100000000ULL;out.semantics=SourceReceipts::OwnedRawIndexedFile;out.request.lookup=SourceReceipts::Insensitive;
  std::strcpy(out.request.archive,"D:\\raw");std::strcpy(out.request.archiveType,"FileSystem");std::strcpy(out.request.cwd,"D:\\raw");std::strcpy(out.request.path,"D:\\raw\\asset.bin");++captured;return S_OK;
 }
 HRESULT Refresh(const SourceReceipts::MainSnapshot&in,SourceReceipts::MainSnapshot&out){++refreshed;return Capture(in.request,out);}
 HRESULT BeginBank(const SourceReceipts::MainSnapshot*,unsigned n,const SourceReceipts::Scope&){if(n!=15||active)return E_INVALIDARG;active=true;++begun;return S_OK;}
 void EndBank(){if(active){++ended;active=false;}}
};
struct Captures{SourceReceipts::MainToken token[4][15];};
struct ThreadCheck{WorldNative::StageAdapter<FakeStage>*adapter;const WorldRaster::Frame*frame;const WorldMaterial::Plan*plan;const SourceReceipts::Scope*scope;Captures*tokens;const SourceReceipts::MainSnapshot*bank;bool good;};
static DWORD WINAPI ForeignThread(void*value){ThreadCheck&t=*static_cast<ThreadCheck*>(value);SourceReceipts::WorkerRequest r={};SourceReceipts::MainSnapshot snapshot={},out={};t.good=t.adapter->Init(*t.scope)==RPC_E_WRONG_THREAD&&t.adapter->CaptureSources(*t.frame,*t.plan,*t.scope,t.tokens->token)==RPC_E_WRONG_THREAD&&t.adapter->Capture(r,out)==RPC_E_WRONG_THREAD&&t.adapter->Refresh(snapshot,out)==RPC_E_WRONG_THREAD&&t.adapter->BeginBank(t.bank,15,*t.scope)==RPC_E_WRONG_THREAD&&t.adapter->CapturedStageCount()==0;t.adapter->EndBank();return 0;}
int main(){try{using namespace WorldNative;
 SourceReceipts::Scope scope={GetCurrentThreadId(),4,7,9};WorldRaster::Frame frame={};frame.source.value=scope.sourceEpoch;frame.source.profile=WorldBank::PinnedBaseSnapshot;frame.generation=scope.job;frame.materialGeneration=3;frame.side=512;
 WorldMaterial::Planner p;C(p.Init(scope.sourceEpoch),"planner");WorldMaterial::Plan plan={};bool found=false;
 for(unsigned r=0;r<64&&!found;++r)for(unsigned c=0;c<16&&!found;++c)if(p.Build(r,c,scope.sourceEpoch,0,plan)&&plan.sidCount==4){frame.region=r;frame.cell=c;found=true;}
 C(found,"actual four SID fixture exists");unsigned sweptRequests=0,sweptSids=0;
 for(unsigned region=0;region<64;++region)for(unsigned cell=0;cell<16;++cell){WorldMaterial::Plan selected={};C(p.Build(region,cell,scope.sourceEpoch,0,selected),"all catalog plans build");WorldRaster::Frame selectedFrame=frame;selectedFrame.region=region;selectedFrame.cell=cell;SourceFactory sweep;C(sweep.Init(selectedFrame,selected,scope)==S_OK,"all region/cell source factories admit");sweptSids+=selected.sidCount;
  for(unsigned sid=0;sid<selected.sidCount;++sid)for(unsigned input=0;input<15;++input){SourceReceipts::WorkerRequest request={};C(sweep.Request(sid,input,request),"all selected stage sources build");C(sweep.Matches(request)&&SourceReceipts::ValidNamed(request),"all source identities stay exact");const char*canonical=CanonicalPath(request.role.asset);C(canonical&&ValidateSourceName(request,canonical,input),"every canonical identity maps to its approved logical pair");if(input<3){unsigned pin=input==2?128:input*64+region;C(request.role.asset==0x10000ULL+pin&&request.pin.bytes==WorldBank::RgbaPins[pin].encodedBytes,"all PNG canonical IDs and encoded sizes");}else{unsigned kind=(input-3)/6,layer=(input-3)%6;const WorldMaterial::Bank&bank=selected.stage[sid].banks[kind];unsigned asset=bank.upload[layer<bank.count?layer:0];C(request.role.asset==asset+1&&request.pin.bytes==SatelliteRegion::Data.assets[asset].bytes,"all DDS canonical IDs and padded bank layers");}++sweptRequests;}}
 C(!CanonicalPath(0)&&!CanonicalPath(65535)&&!CanonicalPath(0x10081ULL),"unknown canonical IDs refuse");SourceFactory f;C(f.Init(frame,plan,scope)==S_OK,"factory validates selected plan once");
 SourceFactory reused;SourceReceipts::WorkerRequest reusedRequest={};C(reused.Init(frame,plan,scope)==S_OK&&reused.Request(0,0,reusedRequest),"reusable controller factory init");reused.Clear();C(!reused.StageCount()&&!reused.Request(0,0,reusedRequest)&&!reused.Matches(reusedRequest),"Clear invalidates previous source scope");C(reused.Init(frame,plan,scope)==S_OK&&reused.Request(0,0,reusedRequest),"factory reinitializes once for next Submit");
 SourceReceipts::WorkerRequest r={};C(f.Request(0,0,r),"painted request");C(!std::strcmp(r.group,"Overlaymaps"),"painted group exact");C(ValidateSourceName(r,WorldBank::RgbaPins[frame.region].path,0),"painted basename admitted");
 std::strcpy(r.group,"General");C(!ValidateSourceName(r,WorldBank::RgbaPins[frame.region].path,0),"wrong group refuses");f.Request(0,0,r);std::strcpy(r.resource,WorldBank::RgbaPins[frame.region].path);C(!ValidateSourceName(r,WorldBank::RgbaPins[frame.region].path,0),"full canonical path alias refuses");
 C(f.Request(0,2,r)&&!std::strcmp(r.resource,"blendMap.png"),"native blend spelling preserved");C(ValidateSourceName(r,WorldBank::RgbaPins[128].path,2),"blend name identity");std::strcpy(r.resource,"blendmap.png");C(!ValidateSourceName(r,WorldBank::RgbaPins[128].path,2),"case is not normalized into another original request");
 C(f.Request(0,3,r)&&!std::strcmp(r.group,"Landscape")&&r.role.asset==r.pin.asset&&r.role.asset>0&&r.role.asset<65536,"DDS group and distinct canonical identity");C(!f.Request(4,0,r)&&!f.Request(0,15,r),"no fifth SID or sixteenth input");
 SourceReceipts::WorkerRequest wrong=r;wrong.pin.sha[0]^=1;C(!f.Matches(wrong),"wrong source hash refuses");wrong=r;wrong.scope.world++;C(!f.Matches(wrong),"old world refuses");wrong=r;std::memset(wrong.resource,'x',sizeof(wrong.resource));C(!f.Matches(wrong),"unterminated resource refuses before strcmp");
 StageAdapter<FakeStage>*a=new StageAdapter<FakeStage>;Captures*t=new Captures;
 C(a->Init(scope)==S_OK,"adapter owning scope init");C(a->CaptureSources(frame,plan,scope,t->token)==S_OK,"capture all four fifteen-role stages");C(a->CapturedStageCount()==4,"all selected SIDs captured");C(a->CaptureSources(frame,plan,scope,t->token)==E_UNEXPECTED,"one job lifetime cannot be recaptured");
 C(t->token[3][14].Request(r)&&r.role.slot==3,"fourth SID last token remains exact slot");
 SourceReceipts::MainSnapshot snapshot={};C(a->Capture(r,snapshot)==S_OK,"captured request routes to its SID");SourceReceipts::MainSnapshot fresh={};C(a->Refresh(snapshot,fresh)==S_OK&&SourceReceipts::SameSnapshot(snapshot,fresh),"refresh preserves exact original name and scope");
 r.role.slot=4;C(FAILED(a->Capture(r,fresh)),"slot four refuses before provider routing");snapshot.request.role.slot=2;C(FAILED(a->Refresh(snapshot,fresh)),"request cannot move across SID identity");
 SourceReceipts::MainSnapshot*bank=new SourceReceipts::MainSnapshot[64];for(unsigned i=0;i<15;++i){C(t->token[1][i].Request(r)&&a->Capture(r,bank[i])==S_OK,"stage bank snapshot");}
 C(a->BeginBank(bank,64,scope)==E_INVALIDARG,"full64 role bank refuses");C(a->BeginBank(bank,14,scope)==E_INVALIDARG,"missing stage role refuses");C(a->BeginBank(bank,15,scope)==S_OK,"one exact fifteen-role SID transaction");ThreadCheck threadCheck={a,&frame,&plan,&scope,t,bank,false};HANDLE thread=CreateThread(NULL,0,ForeignThread,&threadCheck,0,NULL);C(thread!=NULL,"foreign thread test starts");DWORD waited=WaitForSingleObject(thread,5000);C(waited==WAIT_OBJECT_0,"foreign thread test joined");CloseHandle(thread);C(threadCheck.good,"all native routing entry points reject foreign thread");C(FAILED(a->BeginBank(bank,15,scope)),"foreign EndBank cannot unlock Main transaction; no nested bank");a->EndBank();
 bank[14].request.role.slot=2;C(FAILED(a->BeginBank(bank,15,scope)),"mixed SID bank refuses");bank[14]=bank[13];C(FAILED(a->BeginBank(bank,15,scope)),"duplicate role refuses");
 delete[]bank;delete t;delete a;
 StageAdapter<FakeStage>*failed=new StageAdapter<FakeStage>;Captures*partial=new Captures;C(failed->Init(scope)==S_OK,"failure-path owner init");failCaptureSlot=3;C(failed->CaptureSources(frame,plan,scope,partial->token)==E_ACCESSDENIED,"late native capture failure propagates");C(failed->CapturedStageCount()==0,"partial capture cannot route");for(unsigned sid=0;sid<4;++sid)for(unsigned i=0;i<15;++i){SourceReceipts::WorkerRequest request;C(!partial->token[sid][i].Request(request),"late failure invalidates every token");}C(failed->CaptureSources(frame,plan,scope,partial->token)==E_UNEXPECTED,"failed owner remains terminal");failCaptureSlot=4;delete partial;delete failed;
 printf("PASS %u source factory / production adapter checks;64x16 cells,%u SIDs,%u source requests\n",checks,sweptSids,sweptRequests);return 0;
 }catch(...){return 1;}}
