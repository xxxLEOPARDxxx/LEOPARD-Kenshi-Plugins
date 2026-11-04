from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
s=(root/'atlas-terrain/world-cohort/WorldCohort.h').read_text(encoding='utf-8')
def body(signature):
 a=s.index('{',s.index(signature));b=a+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a+1:b-1]
nextinput=body(' HRESULT NextInput(');joined=body(' bool KnownStoppedAndJoined()const')
a=s.index('else if(state_==LoadingSource&&w==WorldStream::WOpened){');a=s.index('{',a);b=a+1;depth=1
while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
accept=s[a+1:b-1]
code=r"""
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstring>
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
namespace SourceReceipts {struct Scope{};}
namespace WorldBank {struct Ledger{};}
namespace WorldStream {enum {Decode,Release};}
enum {Stopped,Booting,Idle,LoadingHeight,InitializingViews,LoadingSource,DecodingSource,UploadingSource,ReleasingSource,BeginningViews,DrawingStage,FinishingViews,QueueReadback,AwaitingGpu,ValidatingPixels,Complete,Cancelling,Failed,Stopping};
struct Gate {HRESULT accept,commit;unsigned accepts,commits;Gate():accept(S_OK),commit(S_OK),accepts(0),commits(0){}template<class T>HRESULT Accept(int,int,const T&,int&out){out=0;++accepts;if(SUCCEEDED(accept))out=7;return accept;}template<class T>HRESULT CommitBank(int*,unsigned count,const T&){++commits;C(count==15,"all fifteen receipts revalidated");return commit;}};
struct Bank {unsigned completes;Bank():completes(0){}bool NeedsRgba(unsigned){return true;}template<class T>HRESULT Complete(int,unsigned,const T&){++completes;return S_OK;}};
struct Frame{int source;unsigned region;};struct Job{Frame frame[1];};
struct Gpu {Bank bank;}gpu;
struct Work {int lease;unsigned pin;}work;
struct Captured {int token[4][15];};
struct Fixture {
 Gate gate_;Gpu*gpu_;Work*work_;HANDLE thread_;Job job_;Captured captured_;int accepted_[15];unsigned input_,slot_,cursor_,requests,sends,lastSend;int state_;bool quarantined_,queryPending_,main;unsigned owned;
 Fixture():gpu_(&gpu),work_(&work),thread_(NULL),input_(14),slot_(0),cursor_(7),requests(0),sends(0),lastSend(0),state_(ReleasingSource),quarantined_(false),queryPending_(false),main(true),owned(0){std::memset(accepted_,0,sizeof(accepted_));gpu.bank.completes=0;}
 HRESULT RequestInput(){++requests;state_=LoadingSource;return S_OK;}
 bool Send(unsigned c){++sends;lastSend=c;return true;}
 bool OnMain()const{return main;}unsigned OwnedGpuBytes()const{return owned;}
 HRESULT NextInput(const SourceReceipts::Scope&now,const WorldBank::Ledger&l){NEXT}
 HRESULT AcceptStep(const SourceReceipts::Scope&now){HRESULT hr=S_OK;ACCEPT return hr;}
 bool KnownStoppedAndJoined()const{JOINED}
};
int main(){try{
 SourceReceipts::Scope now;WorldBank::Ledger ledger;Fixture f;
 f.gate_.commit=HRESULT_FROM_WIN32(ERROR_BUSY);
 for(unsigned n=0;n<5;++n)C(f.NextInput(now,ledger)==S_FALSE&&f.input_==14&&f.state_==ReleasingSource&&!gpu.bank.completes&&!f.requests,"busy commit restores final-input cursor and never completes bank");
 f.gate_.commit=S_OK;C(f.NextInput(now,ledger)==S_OK&&f.input_==15&&f.state_==BeginningViews&&f.cursor_==0&&gpu.bank.completes==1,"same retained receipts succeed after lock released");
 Fixture other;other.input_=3;C(other.NextInput(now,ledger)==S_OK&&other.input_==4&&other.requests==1,"ordinary next-input request unchanged");
 Fixture reject;reject.gate_.commit=E_ACCESSDENIED;C(reject.NextInput(now,ledger)==E_ACCESSDENIED&&!gpu.bank.completes,"changed native identity remains fatal");
 Fixture a;a.state_=LoadingSource;a.input_=8;a.accepted_[8]=19;a.gate_.accept=HRESULT_FROM_WIN32(ERROR_BUSY);
 for(unsigned n=0;n<5;++n)C(a.AcceptStep(now)==S_FALSE&&a.state_==LoadingSource&&a.input_==8&&!a.sends&&!a.accepted_[8],"busy metadata accepts neither receipt nor GPU/worker transition");
 a.gate_.accept=S_OK;C(a.AcceptStep(now)==S_OK&&a.state_==UploadingSource&&a.accepted_[8]==7&&!a.sends,"held source lease resumes after lock released");
 Fixture png;png.input_=1;png.state_=LoadingSource;C(png.AcceptStep(now)==S_OK&&png.state_==DecodingSource&&png.sends==1&&png.lastSend==WorldStream::Decode,"ordinary PNG path unchanged");
 Fixture deny;deny.gate_.accept=E_ACCESSDENIED;C(deny.AcceptStep(now)==E_ACCESSDENIED&&!deny.sends,"nontransient metadata error preserved");
 Fixture j;j.work_=NULL;j.state_=Stopped;C(j.KnownStoppedAndJoined(),"known normal stopped/joined");j.state_=Failed;C(j.KnownStoppedAndJoined(),"known failed operation after real join");
 j.work_=&work;C(!j.KnownStoppedAndJoined(),"live work cannot masquerade as join");j.work_=NULL;j.thread_=(HANDLE)1;C(!j.KnownStoppedAndJoined(),"live thread handle cannot masquerade as join");j.thread_=NULL;j.queryPending_=true;C(!j.KnownStoppedAndJoined(),"unknown GPU query forbids join");j.queryPending_=false;j.owned=1;C(!j.KnownStoppedAndJoined(),"owned GPU forbids join");j.owned=0;j.quarantined_=true;C(!j.KnownStoppedAndJoined(),"quarantine forbids join");j.quarantined_=false;j.state_=Idle;C(!j.KnownStoppedAndJoined(),"idle worker is not terminal");j.state_=Stopped;j.main=false;C(!j.KnownStoppedAndJoined(),"foreign thread cannot establish join");
 printf("PASS %u extracted production busy/join controls\n",checks);return 0;
}catch(...){return 1;}}
""".replace('NEXT',nextinput).replace('ACCEPT',accept).replace('JOINED',joined)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ['Od','O2']:
 f=out/'busy.cpp';f.write_text(code,encoding='ascii');exe=out/'busy.exe'
 r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(f),'/Fo'+str(out/'busy.obj'),'/Fe'+str(exe)],env=env,capture_output=True,text=True);assert r.returncode==0,r.stdout+r.stderr
 r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout,end='');assert r.returncode==0,r.stdout+r.stderr
