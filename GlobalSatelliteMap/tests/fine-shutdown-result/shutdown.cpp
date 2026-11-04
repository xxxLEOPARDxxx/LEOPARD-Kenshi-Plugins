#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstring>
namespace FineJob {struct Plan{};struct Demand{};struct Budget{};bool CanRender(const Plan&,const Demand&,const Budget&){return true;}}
namespace FineStore {struct Identity{};struct Ledger{};}
struct Residency{};
namespace WorldCohort { enum Status{Stopped,Idle,Failed,Stopping};struct Buffer{const void*data;};struct Shaders{Buffer vertex,partial,tail;};struct Job{Shaders shaders;}; }
static unsigned checks=0,failures=0,deleted=0;
void Check(bool value,const char*name){++checks;if(!value){++failures;std::printf("FAIL %s\n",name);}}
struct Native {
 WorldCohort::Status state;HRESULT error,request,poll;bool known,quarantined;unsigned callsRequest,callsPoll;unsigned long long gpu;
 HRESULT rebind,submit;unsigned callsRebind,callsSubmit;
 Native():state(WorldCohort::Idle),error(S_OK),request(S_OK),poll(S_FALSE),known(false),quarantined(false),callsRequest(0),callsPoll(0),gpu(0),rebind(S_OK),submit(S_OK),callsRebind(0),callsSubmit(0){}
 Native&Cohort(){return *this;}WorldCohort::Status State()const{return state;}HRESULT Error()const{return error;}bool Quarantined()const{return quarantined;}
 bool KnownStoppedAndJoined()const{return known&&!quarantined&&!gpu&&(state==WorldCohort::Stopped||state==WorldCohort::Failed);}
 unsigned long long ReservedGpuBytes()const{return gpu;}
 HRESULT RequestShutdown(){++callsRequest;return request;}
 HRESULT PollShutdown(){++callsPoll;return poll;}
 HRESULT RebindCapture(const WorldCohort::Job&,unsigned&){++callsRebind;return rebind;}
 HRESULT Submit(bool,const WorldCohort::Job&,unsigned,unsigned){++callsSubmit;return submit;}
};
struct Owner {Native native;bool terminated,readExposed;FineJob::Plan plan;WorldCohort::Job job;FineStore::Ledger external;bool soft,queued;unsigned pair,tokens,scope;BYTE shaders[21920];Owner():terminated(true),readExposed(false),soft(false),queued(false),pair(0),tokens(0),scope(0){}~Owner(){++deleted;}};
class Controller {
public:Owner*owner_;HRESULT error_;bool quarantined_,main,retryPending_;unsigned retained,hard;
 Controller():owner_(new Owner),error_(S_OK),quarantined_(false),main(true),retryPending_(false),retained(0),hard(0){}
 ~Controller(){delete owner_;}bool Main()const{return main;}void Retain(){++retained;quarantined_=true;if(SUCCEEDED(error_))error_=E_FAIL;}
 bool Ready()const{return main&&owner_&&!quarantined_&&!hard&&!owner_->queued;}bool CurrentBase(const FineJob::Demand&,const FineStore::Identity&)const{return true;}bool PairMatches(const FineJob::Plan&,unsigned,const FineJob::Demand&,const WorldCohort::Job&)const{return true;}bool MakeBudget(const Residency&,const FineStore::Ledger&,FineJob::Budget&)const{return true;}
 void Hard(HRESULT hr){retryPending_=false;++hard;if(SUCCEEDED(error_))error_=hr;}
 bool Queue(const FineJob::Plan&p,unsigned pair,const FineJob::Demand&now,const FineStore::Identity&id,const WorldCohort::Job&job,const Residency&r,const FineStore::Ledger&external){QUEUE_BODY}
 HRESULT RequestShutdown(){REQUEST_BODY}
 HRESULT PollShutdown(){POLL_BODY}
};
int main(){const HRESULT busy=HRESULT_FROM_WIN32(ERROR_BUSY);
 {Controller c;c.owner_->native.state=WorldCohort::Failed;c.owner_->native.known=true;c.owner_->native.error=busy;c.owner_->native.poll=busy;Check(c.PollShutdown()==busy,"original native operation HRESULT retained");Check(!c.quarantined_&&!c.retained&&!c.owner_,"historical ERROR_BUSY known join is not quarantined");Check(c.error_==busy,"known join latches historical error");}
 {Controller c;c.owner_->native.state=WorldCohort::Failed;c.owner_->native.known=true;c.owner_->native.error=busy;Check(c.RequestShutdown()==S_OK,"already joined Failed accepts cleanup request");Check(!c.owner_&&!c.quarantined_,"already joined owner deleted once");Check(c.error_==busy,"request preserves native operation error");Check(c.PollShutdown()==S_OK,"empty owner poll is idempotent");}
 {Controller c;c.error_=E_ACCESSDENIED;c.owner_->native.state=WorldCohort::Failed;c.owner_->native.known=true;c.owner_->native.poll=busy;Check(c.PollShutdown()==busy,"poll returns native HRESULT");Check(c.error_==E_ACCESSDENIED,"first controller error is not overwritten");}
 {Controller c;c.owner_->native.state=WorldCohort::Stopped;c.owner_->native.known=true;c.owner_->native.poll=S_OK;Check(c.PollShutdown()==S_OK,"normal stopped completion");Check(!c.owner_&&!c.quarantined_,"normal completion frees owner");}
 {Controller c;c.owner_->native.state=WorldCohort::Failed;c.owner_->native.poll=busy;Owner*before=c.owner_;Check(c.PollShutdown()==busy,"unknown failure returned");Check(c.owner_==before&&c.quarantined_&&c.retained==1,"unknown failure retains owner");}
 {Controller c;c.owner_->native.quarantined=true;c.owner_->native.known=true;c.owner_->native.state=WorldCohort::Failed;c.owner_->native.poll=E_FAIL;Owner*before=c.owner_;Check(FAILED(c.PollShutdown()),"native quarantine failure");Check(c.owner_==before&&c.quarantined_,"native quarantine cannot use known shortcut");}
 {Controller c;c.owner_->native.poll=S_FALSE;Check(c.PollShutdown()==S_FALSE,"pending remains pending");Check(c.owner_&&!c.quarantined_,"pending owner remains live");}
 {Controller c;c.owner_->native.request=E_FAIL;Check(c.RequestShutdown()==E_FAIL,"unproven request failure returned");Check(c.owner_&&c.quarantined_,"request failure retained");}
 for(unsigned mode=0;mode<5;++mode){Controller c;c.owner_->native.state=WorldCohort::Stopped;c.owner_->native.known=true;c.owner_->native.poll=S_OK;
  if(mode==0)c.main=false;if(mode==1)c.quarantined_=true;if(mode==2)c.owner_->terminated=false;if(mode==3)c.owner_->readExposed=true;if(mode==4)c.owner_->native.gpu=1;
  Owner*before=c.owner_;Check(FAILED(c.RequestShutdown()),"request guard refuses unsafe owner");Check(c.owner_==before&&c.owner_->native.callsRequest==0,"request guard has no native call/delete");
  if(mode<4){Check(FAILED(c.PollShutdown()),"poll guards main/join/exposed");Check(c.owner_==before&&c.owner_->native.callsPoll==0,"poll guard has no native call/delete");}}
 {Controller c;c.owner_->native.gpu=1;c.owner_->native.poll=E_FAIL;Check(FAILED(c.PollShutdown()),"GPU owned failure returned");Check(c.quarantined_&&c.owner_,"GPU owned owner retained");}
 BYTE shader[21920]={0};WorldCohort::Job j={};j.shaders.vertex.data=shader;j.shaders.partial.data=shader+4440;j.shaders.tail.data=shader+17648;FineJob::Plan p;FineJob::Demand demand;FineStore::Identity id;Residency resident;FineStore::Ledger ledger;
 {Controller c;Owner*before=c.owner_;c.owner_->native.rebind=busy;Check(!c.Queue(p,0,demand,id,j,resident,ledger),"transient queue not published");Check(c.retryPending_&&!c.hard&&c.error_==S_OK&&!c.owner_->queued&&c.owner_==before,"busy capture leaves reusable healthy owner");Check(c.owner_->native.callsSubmit==0,"busy capture never submits unbound source");c.owner_->native.rebind=S_OK;Check(c.Queue(p,0,demand,id,j,resident,ledger),"next queue retries successfully");Check(!c.retryPending_&&!c.hard&&c.owner_->queued&&c.owner_->native.callsSubmit==1,"successful retry resets transient flag and submits once");}
 for(unsigned mode=0;mode<3;++mode){Controller c;c.owner_->native.rebind=busy;if(mode==0)c.owner_->native.state=WorldCohort::Failed;if(mode==1)c.owner_->native.gpu=1;if(mode==2)c.owner_->native.rebind=E_ACCESSDENIED;Check(!c.Queue(p,0,demand,id,j,resident,ledger),"unsafe or fatal capture not queued");Check(!c.retryPending_&&c.hard==1&&FAILED(c.error_),"non-idle/GPU/fatal cannot masquerade as transient retry");}
 {Controller c;c.owner_->native.submit=busy;Check(!c.Queue(p,0,demand,id,j,resident,ledger),"submit BUSY is not capture retry");Check(!c.retryPending_&&c.hard==1,"post-capture failure remains fatal");}
 {Controller c;c.retryPending_=true;c.quarantined_=true;Check(!c.Queue(p,0,demand,id,j,resident,ledger)&&!c.retryPending_,"rejected new queue clears old retry flag");}
 std::printf("shutdown production-body controls=%u failures=%u deletes=%u\n",checks,failures,deleted);return failures?1:0;
}
