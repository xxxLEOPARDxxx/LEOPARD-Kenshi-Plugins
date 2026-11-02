"""Extract and exercise actual passive town/proxy HeaderRead on owned memory."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/origin-header-tests';OUT.mkdir(parents=True,exist_ok=True)
text=(HERE/'OriginProof.h').read_text();start=text.index('inline bool HeaderRead(');opening=text.index('{',start);depth=1;end=opening+1
while depth:
    if text[end]=='{':depth+=1
    elif text[end]=='}':depth-=1
    end+=1
body=text[start:end]
source=r'''
#include <Windows.h>
#include <float.h>
#include <cstring>
#include <cstdio>
namespace NativeLocalLights {inline int NativeFault(DWORD c){return c==EXCEPTION_ACCESS_VIOLATION||c==EXCEPTION_IN_PAGE_ERROR||c==EXCEPTION_DATATYPE_MISALIGNMENT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}}
struct Header {const void*proxy;const void*entity;float town[3];};
BODY
unsigned checks=0;void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
 unsigned __int64 town[0x500/8]={0},proxy[3]={0};BYTE*t=reinterpret_cast<BYTE*>(town),*p=reinterpret_cast<BYTE*>(proxy);const void*owner=town,*ptr=proxy,*entity=reinterpret_cast<const void*>(0x12340);std::memcpy(t+0x4f8,&ptr,8);std::memcpy(p,&owner,8);unsigned state=3;std::memcpy(p+8,&state,4);std::memcpy(p+16,&entity,8);float position[3]={12345,0,-54321};std::memcpy(t+0x48,position,12);BYTE before[sizeof(town)];std::memcpy(before,t,sizeof(before));Header h;
 C(HeaderRead(town,h)&&h.proxy==proxy&&h.entity==entity,"existing state3 proxy identity admitted");C(h.town[0]==12345&&h.town[1]==0&&h.town[2]==-54321,"stored town position copied including lazy-height sentinel");C(!std::memcmp(before,t,sizeof(before)),"passive town copy performs no lazy height write");
 const void*wrong=reinterpret_cast<const void*>(0x999);std::memcpy(p,&wrong,8);C(!HeaderRead(town,h),"wrong owner refused");std::memcpy(p,&owner,8);state=2;std::memcpy(p+8,&state,4);C(!HeaderRead(town,h),"nonresident proxy state refused");state=3;std::memcpy(p+8,&state,4);entity=NULL;std::memcpy(p+16,&entity,8);C(!HeaderRead(town,h),"missing existing entity refused");entity=reinterpret_cast<const void*>(0x12340);std::memcpy(p+16,&entity,8);
 unsigned nan=0x7fc00000;std::memcpy(t+0x48,&nan,4);C(!HeaderRead(town,h),"nonfinite stored coordinate refused");C(!HeaderRead(NULL,h),"missing town refused");C(!HeaderRead(reinterpret_cast<const void*>(8),h),"invalid town pointer contained");ptr=reinterpret_cast<const void*>(8);std::memcpy(t+0x4f8,&ptr,8);C(!HeaderRead(town,h),"invalid existing proxy pointer contained");std::printf("Origin passive-header PASS %u checks; actual production body/owned memory/no native calls\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('BODY',body)
(OUT/'test.cpp').write_text(source)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4','/Fe'+str(OUT/'test.exe'),'/Fo'+str(OUT/'test.obj'),str(OUT/'test.cpp')],env=env,check=True)
subprocess.run([str(OUT/'test.exe')],check=True)
