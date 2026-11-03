from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WindowWriter512.h').read_text().replace('"../Gst2Writer.h"','"'+(HERE.parent/'Gst2Writer.h').as_posix()+'"')
test=(HERE/'writer_tests.cpp').read_text()
changes=[
 ('source-high64','a.source==b.source','DWORD(a.source)==DWORD(b.source)'),
 ('material-high64','a.material==b.material','DWORD(a.material)==DWORD(b.material)'),
 ('capture-high64','a.capture==b.capture','DWORD(a.capture)==DWORD(b.capture)'),
 ('early-ledger','if(CoarseDone(plane,x,z))','if(true)'),
 ('duplicate','if(done_[plane][bit/64]&flag)','if(false)'),
 ('transpose','(U64(frame.z+j)*8192+frame.x)','(U64(frame.x+j)*8192+frame.z)'),
 ('data-flush','if(!io_->Flush())return Stop(ERROR_WRITE_FAULT);',''),
 ('ledger-flush','if(!io_->WriteAt(48,ledger,8)||!io_->Flush())','if(!io_->WriteAt(48,ledger,8))'),
 ('owner','if(owner_!=GetCurrentThreadId())return false;',''),
 ('poison','poisoned_=true;error_=error;','poisoned_=false;error_=error;'),
 ('plane-mask','plane*16+(z/4)*4+x/4','plane*15+(z/4)*4+x/4'),
 ('callback-unwind','mask_|=coarse;\n        }\n        error_=ERROR_SUCCESS;return true;\n        }catch(...){return Stop(ERROR_UNHANDLED_EXCEPTION);}', 'mask_|=coarse;\n        }\n        error_=ERROR_SUCCESS;return true;\n        }catch(...){return false;}'),
 ('begin-status','owner_=GetCurrentThreadId();error_=ERROR_SUCCESS;return true;','owner_=GetCurrentThreadId();return true;'),
 ('cancel-flag','InterlockedExchange(&cancelRequested_,1);','InterlockedExchange(&cancelRequested_,0);'),
]
for name,old,new in changes:
 assert source.count(old)==1,(name,'mutation is not unique')
 header=OUT/(name+'.h');header.write_text(source.replace(old,new))
 cpp=OUT/(name+'.cpp');cpp.write_text(test.replace('"WindowWriter512.h"','"'+header.as_posix()+'"'))
 exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],cwd=HERE,env=env,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,'compile failure is not detection',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==1 and 'FAIL ' in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 print('Expected RED',name,(p.stdout+p.stderr).strip(),flush=True)
print('PASS14 compiled production writer mutants rejected')
