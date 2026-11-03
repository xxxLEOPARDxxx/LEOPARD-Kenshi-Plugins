from pathlib import Path
import os,subprocess,sys
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
policy=(HERE/'ReceiptPolicy.h').read_text();worker=(HERE/'WorkerFileLease.h').read_text();gate=(HERE/'MainGate.h').read_text();test=(HERE/'tests.cpp').read_text()
def run(name,pp=policy,ww=worker,gg=gate,tt=test,red=False):
 folder=OUT/name;folder.mkdir(exist_ok=True)
 for file,s in [('ReceiptPolicy.h',pp),('WorkerFileLease.h',ww),('MainGate.h',gg),('tests.cpp',tt)]:
  (folder/file).write_text(s)
 exe=folder/'tests.exe'
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32',str(folder/'tests.cpp'),'/Fo'+str(folder/'tests.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=ENV,capture_output=True,text=True);(folder/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,('compile failure is not a valid rejection',name,p.stdout,p.stderr)
 pe=pefile.PE(str(exe));assert all('ogre' not in d.dll.decode().lower() and 'kenshi' not in d.dll.decode().lower() for d in pe.DIRECTORY_ENTRY_IMPORT)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr)
 if red:assert p.returncode==1 and 'FAIL ' in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 else:assert p.returncode==0,(name,p.returncode,p.stdout,p.stderr)
 print(('Expected RED: '+name+' ' if red else '')+(p.stderr.strip() if red else p.stdout.strip()),flush=True)

faultPrefix=r'''
#include "ReceiptPolicy.h"
namespace Fault {
static int at=-1,count=0,reads=0;static HANDLE cancel=NULL;
static bool Next(){bool hit=count++==at;if(hit)SetLastError(ERROR_ACCESS_DENIED);return hit;}
static HANDLE File(LPCSTR p,DWORD a,DWORD s,LPSECURITY_ATTRIBUTES x,DWORD c,DWORD f,HANDLE t){return Next()?INVALID_HANDLE_VALUE:CreateFileA(p,a,s,x,c,f,t);}
static BOOL Info(HANDLE h,BY_HANDLE_FILE_INFORMATION*i){return Next()?FALSE:GetFileInformationByHandle(h,i);}
static BOOL Read(HANDLE h,LPVOID p,DWORD n,LPDWORD got,LPOVERLAPPED o){if(Next())return FALSE;BOOL ok=ReadFile(h,p,n,got,o);++reads;if(cancel)SetEvent(cancel);return ok;}
static DWORD Final(HANDLE h,LPWSTR p,DWORD n,DWORD f){return Next()?0:GetFinalPathNameByHandleW(h,p,n,f);}
static bool Digest(const BYTE*p,size_t n,BYTE(&out)[32]){return !Next()&&SourceReceipts::Hash(p,n,out);}
}
'''
instrumented=worker.replace('CreateFileA(','Fault::File(').replace('GetFileInformationByHandle(','Fault::Info(').replace('ReadFile(','Fault::Read(').replace('GetFinalPathNameByHandleW(','Fault::Final(').replace('Hash(bytes_,size_t(size),hash)','Fault::Digest(bytes_,size_t(size),hash)').replace('new(std::nothrow) BYTE[size_t(size)]','(Fault::Next()?NULL:new(std::nothrow) BYTE[size_t(size)])')
instrumented=faultPrefix+instrumented
extra=r'''
 Fault::at=-1;Fault::count=0;{WorkerFileLease warm;C(SUCCEEDED(warm.Open(r,NULL,0)),"actual instrumented warm lease");}int cuts=Fault::count;C(cuts>15,"all ancestor/file/read/hash/final-path boundaries exercised");
 DWORD baseline=0;GetProcessHandleCount(GetCurrentProcess(),&baseline);
 for(int cut=0;cut<cuts;++cut){Fault::at=cut;Fault::count=0;WorkerFileLease failed;C(FAILED(failed.Open(r,NULL,0)),"fault must surface");DWORD current=0;GetProcessHandleCount(GetCurrentProcess(),&current);C(!failed.Ready()&&!failed.Data()&&!failed.OwnedBytes()&&current==baseline,"every partial lease handle/blob cleaned before return");Fault::at=-1;Fault::count=0;C(SUCCEEDED(failed.Open(r,NULL,0)),"actual retry after every I/O/alloc/hash fault");failed.Clear();}
 for(unsigned bad=0;bad<8;++bad){WorkerRequest invalid=r;if(bad==0)invalid.scope.world=0;if(bad==1)invalid.scope.job=0;if(bad==2)invalid.role.kind=0;if(bad==3)invalid.pin.epoch++;if(bad==4)invalid.pin.bytes=BlobCap+1;if(bad==5)invalid.pin.bytes=0;if(bad==6)strcpy(invalid.path,"C:\\NUL");if(bad==7)invalid.path[3]=':';Fault::at=-1;Fault::count=0;WorkerFileLease failed;C(FAILED(failed.Open(invalid,NULL,0))&&Fault::count==0,"invalid guard refuses before any file I/O or hash");}
 HANDLE cancelRead=CreateEventW(NULL,TRUE,FALSE,NULL);C(cancelRead!=NULL,"mid-read cancel event");Fault::at=-1;Fault::count=Fault::reads=0;Fault::cancel=cancelRead;WorkerFileLease interrupted;C(interrupted.Open(r,cancelRead,0)==HRESULT_FROM_WIN32(ERROR_CANCELLED)&&Fault::reads==1&&!interrupted.Ready()&&!interrupted.OwnedBytes(),"actual cancellation after first64KiB read cleans lease");Fault::cancel=NULL;CloseHandle(cancelRead);DWORD after=0;GetProcessHandleCount(GetCurrentProcess(),&after);C(after==baseline,"mid-read cancellation handle balance");
 printf("PASS %d real Win32 lease fault cutpoints plus no-I/O guards and real mid-read cancellation\n",cuts);
'''
assert test.count(' printf("PASS %u actual source receipt')==1
faultTest=test.replace(' printf("PASS %u actual source receipt',extra+' printf("PASS %u actual source receipt')
run('faults',ww=instrumented,tt=faultTest)
bad=instrumented.replace('if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}');assert bad!=instrumented
run('cleanup-mutant',ww=bad,tt=faultTest,red=True)

def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
run('winner-mutant',gg=edit(gate,'a.archive==b.archive','true'),red=True)
run('world64-mutant',pp=edit(policy,'a.world==b.world','DWORD(a.world)==DWORD(b.world)'),red=True)
run('hash-mutant',ww=edit(worker,'std::memcmp(hash,r.pin.sha,32)','false'),red=True)
run('reparse-mutant',ww=edit(worker,'(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)||','false||'),red=True)
run('sharing-mutant',ww=edit(worker,'FILE_SHARE_READ,NULL,OPEN_EXISTING','FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING'),red=True)
run('thread-mutant',gg=edit(gate,'&&GetCurrentThreadId()==now.thread',''),red=True)
run('last-only-mutant',gg=edit(gate,'for(unsigned i=0;i<count;++i){hr=Fresh','for(unsigned i=count-1;i<count;++i){hr=Fresh'),red=True)
run('publication-mutant',gg=edit(gate,'HRESULT hr=Fresh(t.snapshot_,lease.Stamp(),now);if(FAILED(hr))return hr;','HRESULT hr=Fresh(t.snapshot_,lease.Stamp(),now);if(FAILED(hr)){lease.accepted_=true;return hr;}'),red=True)
print('PASS 9 separately compiled cleanup/guard/ownership/publication negative controls')
