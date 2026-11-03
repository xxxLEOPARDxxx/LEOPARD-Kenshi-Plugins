from pathlib import Path
import run_tests as r
source=(r.HERE/'WorldWriteWorker.h').read_text()
def header(name,old,new):
 assert source.count(old)==1,old
 data=source.replace(old,new)
 for relative in ['../world-window-session/WorldWindowSession.h','../../atlas-terrain/world-cohort/WorldCohort.h']:
  data=data.replace('"'+relative+'"','"'+(r.HERE/relative).resolve().as_posix()+'"')
 path=r.OUT/(name+'.h');path.write_text(data);return path
common='''
#define main OriginalWorkerHostMain
#include "%s"
#undef main
'''
body='''
int main(){try{
 SourceReceipts::Scope scope={GetCurrentThreadId(),7,0x100000007ULL,0x300000001ULL};Stats stats={};Config config={scope.thread,&stats,NULL,NULL,0};WorldWindow::Ledger ledger;Writer w;C(w.Start(scope,9,config,ledger),"fault worker starts");Sleep(30);WorldWindow::Location where={};
 %s
 printf("PASS %%u isolated writer-fault controls\\n",checks);return 0;
}catch(...){return 1;}}
'''
path=header('wait-failed','DWORD wait=WaitForSingleObject(thread_,0);','DWORD wait=WAIT_FAILED;')
text='#define WORLD_WRITE_HEADER "'+path.as_posix()+'"\n'+common%(r.HERE/'worker_tests.cpp').as_posix()+body%('C(w.Poll(where)==-1&&w.Quarantined(),"unknown worker wait retains owner");w.Cancel();C(!w.Join(),"unknown borrower completion never licenses source release");')
cpp=r.OUT/'wait-failed.cpp';cpp.write_text(text);r.run('wait-failed',cpp)
path=header('missing-finish-marker','InterlockedExchange(&w.finished,1);','/* injected abnormal exit before finish marker */')
text='#define WORLD_WRITE_HEADER "'+path.as_posix()+'"\n'+common%(r.HERE/'worker_tests.cpp').as_posix()+body%('w.Cancel();C(!w.Join()&&w.Quarantined(),"joined thread without EOF marker conservatively retains complete borrow owner");')
cpp=r.OUT/'missing-finish-marker.cpp';cpp.write_text(text);r.run('missing-finish-marker',cpp)
