from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/adversarial';OUT.mkdir(parents=True,exist_ok=True);VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WorldStream.h').read_text()
source=re.sub(r'^\s*#include\s+"(\.\./[^"\n]+)"',lambda m:'#include "'+(HERE/m[1]).resolve().as_posix()+'"',source,flags=re.M)
prefix=r'''namespace Review {static volatile LONG failWake=0,failWakeAfter=0,failSend=0,failQuery=0;static unsigned bankInits=0;static bool FailQuery(){return InterlockedExchange(&failQuery,0)!=0;}static bool forceFence=false,leaveAfterValidation=false;static DWORD WakeWait(HANDLE h){if(InterlockedExchange(&failWake,0))return WAIT_FAILED;DWORD result=::WaitForSingleObject(h,INFINITE);return InterlockedExchange(&failWakeAfter,0)?WAIT_FAILED:result;}static BOOL WakeSend(HANDLE h){if(InterlockedExchange(&failSend,0)){SetLastError(ERROR_INVALID_HANDLE);return FALSE;}return ::SetEvent(h);} }
'''
source=source.replace('namespace WorldStream {',prefix+'namespace WorldStream {')
assert source.count(' explicit Controller(Provider&p)')==1
source=source.replace(' explicit Controller(Provider&p)',' Work*ReviewWork(){return work_;}HANDLE ReviewThread(){return thread_;}\n explicit Controller(Provider&p)')
assert source.count('done=WValidated;')==1
source=source.replace('done=WValidated;','done=WValidated;if(Review::leaveAfterValidation)InterlockedExchange(&Review::failWake,1);')
for old,new in [('WaitForSingleObject(w.wake,INFINITE)','Review::WakeWait(w.wake)'),('if(SetEvent(work_->wake))return true;','if(Review::WakeSend(work_->wake))return true;'),('return context_.p->GetData(query_.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);','return Review::forceFence?S_FALSE:context_.p->GetData(query_.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);'),('GetTickCount()-start<5000','GetTickCount()-start<25'),('HRESULT hr=d->CreateQuery(&q,&query_.p);','HRESULT hr=Review::FailQuery()?E_OUTOFMEMORY:d->CreateQuery(&q,&query_.p);'),('hr=bank_.Init(d,job_.frame.source,ledger);','hr=(++Review::bankInits,bank_.Init(d,job_.frame.source,ledger));')]:
 assert source.count(old)==1,old;source=source.replace(old,new)
header=OUT/'instrumented.h';header.write_text(source)
def execute(name,code,h=header,green=True):
 cpp=OUT/(name+'.cpp');cpp.write_text(code.replace('"WorldStream.h"','"'+h.as_posix()+'"'));exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (p.returncode==0 if green else p.returncode==1 and 'FAIL 'in p.stderr),(name,p.returncode,p.stdout,p.stderr);print(name,(p.stdout+p.stderr).strip(),flush=True)
code=r'''#define _CRT_SECURE_NO_WARNINGS
#include "WorldStream.h"
#include <cstdio>
using namespace WorldStream;
struct Provider{HRESULT Capture(const SourceReceipts::WorkerRequest&,SourceReceipts::MainSnapshot&){return E_FAIL;}HRESULT Refresh(const SourceReceipts::MainSnapshot&,SourceReceipts::MainSnapshot&){return E_FAIL;}HRESULT BeginBank(const SourceReceipts::MainSnapshot*,unsigned,const SourceReceipts::Scope&){return E_FAIL;}void EndBank(){}};
static void C(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{Provider p;Controller<Provider>*c=new Controller<Provider>(p);Review::failWake=1;C(c->Start()==S_OK,"start real worker with failed wake boundary");SourceReceipts::Scope scope={GetCurrentThreadId(),1,1,1};WorldBank::Ledger l={};DWORD start=GetTickCount();while(c->State()!=Failed&&GetTickCount()-start<2000){c->Pump(false,NULL,scope,l);Sleep(1);}C(c->State()==Failed&&FAILED(c->Error())&&!c->OwnedGpuBytes()&&!Quarantine().marked,"unexpected worker exit becomes terminal after joined cleanup");delete c;Controller<Provider>*next=new Controller<Provider>(p);C(next->Start()==S_OK,"safe completed worker exit releases process slot");next->Shutdown();C(next->State()==Stopped,"replacement worker joins normally");delete next;puts("PASS actual wake-wait failure, terminal exit and bounded restart");return 0;}catch(...){return 1;}}
'''
execute('worker-exit',code)
worker=(HERE/'worker_acceptance_tests.cpp').read_text()
worker=re.sub(r'^\s*#include\s+"(\.\./[^"\n]+)"',lambda m:'#include "'+(HERE/m[1]).resolve().as_posix()+'"',worker,flags=re.M)
start=worker.index(' Send(*w,ValidatePixels);C(Wait(*w)==WValidated&&w->accepted.Acquire')
end=worker.index(' }catch(...){return 1;}}',start)
worker=worker[:start]+r'''
 Review::leaveAfterValidation=true;Send(*w,ValidatePixels);C(WaitForSingleObject(thread,10000)==WAIT_OBJECT_0&&InterlockedCompareExchange(&w->state,0,0)==WStopped,"unsolicited wait failure exits after validation");C(w->accepted.OwnedBytes()==WorldPixels::Accepted::RequiredBytes(256)&&w->window.Ready()&&w->pixels.capacity()>=n*16,"unexpected exit preserves accepted/raw/window owners until Main join");C(w->accepted.Acquire(true,scope,frame,w->window,view),"borrowed output stays intact behind prior WValidated observation");CloseHandle(thread);CloseHandle(w->wake);CloseHandle(w->cancel);delete w;printf("PASS %u real worker abnormal-exit acceptance lifetime controls\n",checks);return 0;
'''+worker[end:]
execute('validated-worker-exit',worker)
mut=OUT/'validated-exit-loss.h';assert source.count('if(stopped){w.DropInput();w.accepted.Clear();w.DropHeight();std::vector<BYTE>().swap(w.pixels);}')==1
mut.write_text(source.replace('if(stopped){w.DropInput();w.accepted.Clear();w.DropHeight();std::vector<BYTE>().swap(w.pixels);}','if(stopped)w.DropInput();w.accepted.Clear();w.DropHeight();std::vector<BYTE>().swap(w.pixels);'))
execute('validated-exit-loss',worker,mut,green=False)
flow=(HERE/'flow_tests.cpp').read_text()
flow=re.sub(r'^\s*#include\s+"(\.\./[^"\n]+)"',lambda m:'#include "'+(HERE/m[1]).resolve().as_posix()+'"',flow,flags=re.M)
sourceExtra=r'''
 Work*w=c->ReviewWork();DWORD pending=GetTickCount();while(GetTickCount()-pending<30000){LONG state=InterlockedCompareExchange(&w->state,0,0);if(SOURCE_OPENED?(c->State()==LoadingSource&&state==WOpened):(c->State()==UploadingSource&&state==WDecoded))break;C(SUCCEEDED(c->Pump(true,d.p,scope,external)),"actual source read/accept/decode before Main borrow");Sleep(1);}C(w->lease.Ready()&&(SOURCE_OPENED?!w->lease.Data():(w->lease.Data()&&!w->rgba.empty())),"source borrow phase precondition");const BYTE*encoded=w->lease.Data(),*decoded=w->rgba.empty()?NULL:&w->rgba[0];const size_t decodedBytes=w->rgba.size();const U64 owned=w->lease.OwnedBytes();SourceReceipts::WorkerRequest request=w->lease.Request();InterlockedExchange(&Review::failWakeAfter,1);C(SetEvent(w->wake)&&WaitForSingleObject(c->ReviewThread(),5000)==WAIT_OBJECT_0,"unsolicited worker exit after source handoff");C(w->lease.Ready()&&w->lease.OwnedBytes()==owned&&w->lease.Data()==encoded&&SourceReceipts::SameRequest(request,w->lease.Request())&&w->rgba.size()==decodedBytes&&(decodedBytes?&w->rgba[0]==decoded:true),"unexpected exit preserves encoded lease and decoded RGBA until Main joins");c->Cancel();HRESULT ended=S_FALSE;DWORD until=GetTickCount();while(c->State()==Cancelling&&GetTickCount()-until<5000){ended=c->Pump(false,NULL,scope,external);Sleep(1);}C(c->State()==Failed&&c->Error()==E_FAIL&&ended==E_FAIL&&!c->OwnedGpuBytes(),"source exit joins and reports original worker error");delete tokens;delete j;delete c;printf("PASS %u source-handoff abnormal-exit lifetime checks\n",checks);return 0;
'''
point='C(SUCCEEDED(c->Submit(true,temporary,*tokens,scope)),"submit actual bounded stream");';assert flow.count(point)==1
sourceFlows=[]
for name,opened in [('opened-source-exit',True),('decoded-source-exit',False)]:
 test=flow.replace(point,point+sourceExtra.replace('SOURCE_OPENED','true'if opened else'false'));sourceFlows.append(test);execute(name,test,green='--expect-red-source'not in sys.argv)
if '--source-only'in sys.argv:sys.exit(0)
stopCleanup='if(stopped){w.DropInput();w.accepted.Clear();w.DropHeight();std::vector<BYTE>().swap(w.pixels);}'
assert source.count(stopCleanup)==1
mut=OUT/'source-exit-loss.h';mut.write_text(source.replace(stopCleanup,'w.DropInput();if(stopped){w.accepted.Clear();w.DropHeight();std::vector<BYTE>().swap(w.pixels);}'))
execute('source-exit-loss',sourceFlows[1],mut,green=False)
cancelExtra=r'''
  const BYTE*borrowed=c->Plane(2,true,scope,j->frame),*radial=c->RawPlane(2,true,scope,j->frame);C(borrowed&&radial,"completed pixel pointers before unsolicited exit");std::vector<BYTE>saved(borrowed,borrowed+256*256*4);Work*w=c->ReviewWork();InterlockedExchange(&Review::failWakeAfter,1);C(SetEvent(w->wake)&&WaitForSingleObject(c->ReviewThread(),5000)==WAIT_OBJECT_0,"actual unsolicited worker exit after Main Complete");C(w->accepted.OwnedBytes()&&w->window.Ready()&&w->pixels.capacity()&&!std::memcmp(borrowed,&saved[0],saved.size()),"completed owners stay valid until Main join");C(!c->Result(true,scope,j->frame)&&!c->Plane(2,true,scope,j->frame),"fresh consumption rejects dead worker");c->Cancel();C(c->State()==Cancelling,"explicit Cancel precedes dead-worker observation");HRESULT ended=S_FALSE;DWORD until=GetTickCount();while(c->State()==Cancelling&&GetTickCount()-until<5000){ended=c->Pump(false,NULL,scope,external);Sleep(1);}C(c->State()==Failed&&!c->OwnedGpuBytes(),"abnormal exit joined and GPU owners drained");C(c->Error()==E_FAIL&&ended==E_FAIL,"Cancel cannot hide failed worker exit behind success");delete tokens;delete j;delete c;printf("PASS %u unsolicited Complete-exit then Cancel failure-latch checks\n",checks);return 0;
'''
assert flow.count('}AcceptedPlanes(*c,*j);')==1
cancelFlow=flow.replace('}AcceptedPlanes(*c,*j);','}AcceptedPlanes(*c,*j);'+cancelExtra)
execute('complete-exit-cancel',cancelFlow,green='--expect-red-cancel'not in sys.argv)
if '--cancel-only'in sys.argv:sys.exit(0)
fixed='if(exited){error_=FAILED(work_->error)?work_->error:E_FAIL;if(state_!=Cancelling){raster_.Cancel();Fence();state_=Cancelling;}}'
lost='if(exited&&state_!=Cancelling){error_=FAILED(work_->error)?work_->error:E_FAIL;raster_.Cancel();Fence();state_=Cancelling;}'
assert source.count(fixed)==1
mut=OUT/'cancel-failure-latch-loss.h';mut.write_text(source.replace(fixed,lost));execute('cancel-failure-latch-loss',cancelFlow,mut,green=False)
extra=r'''
 ++scope.job;j->scope=scope;j->frame.generation=scope.job;Tokens(provider,*j,*tokens);unsigned bankBefore=Review::bankInits;InterlockedExchange(&Review::failQuery,1);C(SUCCEEDED(c->Submit(true,*j,*tokens,scope)),"submit actual worker job for query-create failure");start=GetTickCount();HRESULT queryError=S_OK;while(c->State()!=Cancelling&&GetTickCount()-start<30000){queryError=c->Pump(true,d.p,scope,external);if(FAILED(queryError))break;Sleep(1);}C(queryError==E_OUTOFMEMORY&&c->State()==Cancelling&&!c->OwnedGpuBytes()&&Review::bankInits==bankBefore,"failed event query prevents every bank initialization command");C(c->ReservedGpuBytes()==Controller<Provider>::MaximumGpuReservation(),"cancel admission remains charged before drain");start=GetTickCount();while(c->State()==Cancelling&&GetTickCount()-start<2000){c->Pump(false,NULL,scope,external);Sleep(1);}C(c->State()==Idle&&!c->ReservedGpuBytes()&&!Quarantine().marked,"query failure drains and safely releases reservation");
 ++scope.job;j->scope=scope;j->frame.generation=scope.job;Tokens(provider,*j,*tokens);InterlockedExchange(&Review::failSend,1);C(FAILED(c->Submit(true,*j,*tokens,scope)),"failed wake send surfaces instead of permanent Busy");start=GetTickCount();while(c->State()==Cancelling&&GetTickCount()-start<2000){c->Pump(false,NULL,scope,external);Sleep(1);}C(c->State()==Idle&&!c->OwnedGpuBytes()&&!Quarantine().marked,"failed Send drains and returns to reusable worker");
 ++scope.job;j->scope=scope;j->frame.generation=scope.job;Tokens(provider,*j,*tokens);C(SUCCEEDED(c->Submit(true,*j,*tokens,scope)),"submit real frame for pending-GPU quarantine");start=GetTickCount();while(c->State()!=DrawingStage&&GetTickCount()-start<30000){C(SUCCEEDED(c->Pump(true,d.p,scope,external)),"actual source uploads before quarantine");Sleep(1);}C(c->State()==DrawingStage&&c->OwnedGpuBytes()>WorldBank::SourceBytes,"source/raster/staging owners exist before forced unknown fence");Review::forceFence=true;c->Shutdown();C(c->State()==Failed&&Quarantine().marked&&Quarantine().work&&Quarantine().gpu,"unknown fence retains whole worker/GPU heap");C(Quarantine().gpuBytes==Controller<Provider>::MaximumGpuReservation()&&Quarantine().gpuBytes>=c->OwnedGpuBytes()&&Quarantine().cpuBytes==c->CpuReservation(),"unknown partial/retired work retains maximum admitted charges");GpuOwners*held=static_cast<GpuOwners*>(Quarantine().gpu);start=GetTickCount();while(ctx.p->GetData(held->query.p,NULL,0,D3D11_ASYNC_GETDATA_DONOTFLUSH)==S_FALSE&&GetTickCount()-start<2000)Sleep(1);ULONG before=d.p->AddRef();d.p->Release();delete c;c=NULL;ULONG after=d.p->AddRef();d.p->Release();C(before==after,"controller destructor does not release quarantined GPU references");C(held->bank.OwnedGpuBytes()==WorldBank::SourceBytes&&held->raster.OwnedGpuBytes()>0,"retained heap remains valid after controller destruction");Controller<Provider>*blocked=new Controller<Provider>(provider);C(blocked->Start()==E_UNEXPECTED,"quarantine prevents unaccounted repeated worker restart");delete blocked;delete tokens;delete j;printf("PASS %u streamed-frame plus Send/fence/destructor quarantine checks\n",checks);return 0;
'''
old=' c->Shutdown();C(c->State()==Stopped,"complete query/worker clean shutdown");delete tokens;delete j;delete c;printf("PASS %u actual streamed-frame checks; three completed frames, cache reuse/OFF/world cancel\\n",checks);return 0;'
assert flow.count(old)==1
flow=flow.replace(old,extra)
execute('send-fence-quarantine',flow)
mut=OUT/'destructor-loss.h';assert source.count('if(!quarantined_)delete gpu_;')==1;mut.write_text(source.replace('if(!quarantined_)delete gpu_;','delete gpu_;'));execute('destructor-loss',flow,mut,green=False)

mut=OUT/'reservation-loss.h';assert source.count('q.gpuBytes=MaximumGpuReservation();')==1;mut.write_text(source.replace('q.gpuBytes=MaximumGpuReservation();','q.gpuBytes=OwnedGpuBytes();'));execute('reservation-loss',flow,mut,green=False)
