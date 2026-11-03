from pathlib import Path
import subprocess,os
here=Path(__file__).resolve().parent;out=here/'_generated';vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
source=(here/'PinnedPng.h').read_text().replace('"../world-bank/WorldSourceBank.h"','"'+(here.parent/'world-bank/WorldSourceBank.h').as_posix()+'"')
test=(here/'decode_tests.cpp').read_text()
changes=[('BGRA','GUID_WICPixelFormat32bppRGBA','GUID_WICPixelFormat32bppBGRA'),('cancel','return event&&WaitForSingleObject(event,0)!=WAIT_TIMEOUT;','return false;'),('alias','||(input<output+capacity&&output<input+bytes)',''),('encoded-hash','||std::memcmp(actual,expected,32)','')]
changes.append(('tu-identity','p.bytes==q.bytes&&p.encodedBytes==q.encodedBytes&&!std::memcmp(p.sha,q.sha,32)&&!std::strcmp(p.path,q.path)&&!std::strcmp(p.encodedSha,q.encodedSha)','&p==&q'))
for name,old,new in changes:
 assert source.count(old)==1;header=out/(name+'.h');header.write_text(source.replace(old,new));cpp=out/(name+'.cpp');cpp.write_text(test.replace('"PinnedPng.h"','"'+header.as_posix()+'"'));exe=out/(name+'.exe')
 p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(cpp),str(here/'cross_pin.cpp'),'/Fo'+str(out)+'/', '/Fe'+str(exe),'/link','ole32.lib','windowscodecs.lib','advapi32.lib'],cwd=here,env=env,capture_output=True,text=True);assert not p.returncode,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=here,capture_output=True,text=True);(out/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL 'in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 print('Expected RED',name,p.stderr.strip(),flush=True)
print('PASS5 compiled PNG decoder mutants rejected')
