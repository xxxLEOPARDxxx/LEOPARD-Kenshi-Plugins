"""Compile the actual identity-change retirement prefix; no engine/UI calls."""
from pathlib import Path
import os, subprocess

here = Path(__file__).resolve().parent
root = here.parents[1]
out = here / '_generated'
out.mkdir(exist_ok=True)
source = (root / 'GsmFineViewer.h').read_text(encoding='utf-8-sig')
start = source.index('if(fineOwner){fineOwner->Cancel();return;}', source.index('void FineObserve('))
stop = source.index('std::memset(&fineIdentity', start)
prefix = source[start:stop]
code = r'''
#include <cstdio>
namespace FineJob {enum {CacheSlots=256};inline unsigned ActiveSlots(){return CacheSlots;}}
struct Owner {unsigned cancelled;Owner():cancelled(0){}void Cancel(){++cancelled;}};
Owner* fineOwner=0;
struct Entry {bool valid;}fineCache[256];
unsigned drops=0,retired=0,published=0,checks=0;int refuse=-1;
void FineDrop(unsigned n){++drops;if(fineCache[n].valid&&int(n)!=refuse){fineCache[n].valid=false;++retired;}}
void Prepare(){PREFIX ++published;}
void C(bool b,const char*s){++checks;if(!b){printf("FAIL %s\n",s);throw 1;}}
int main(){try{
 Owner owner;fineOwner=&owner;fineCache[0].valid=true;Prepare();
 C(owner.cancelled==1&&drops==0&&published==0,"old worker must drain before changing identity");
 fineOwner=0;fineCache[3].valid=fineCache[255].valid=true;refuse=3;Prepare();
 C(published==0&&retired==1&&!fineCache[0].valid&&fineCache[3].valid&&fineCache[255].valid,"failed retirement cannot mix old keys with new identity");
 Prepare();C(published==0&&retired==1,"retry does not credit already retired bytes twice");
 refuse=-1;Prepare();C(published==1&&retired==3,"all old keys retire before new stamp publication");
 for(unsigned n=0;n<256;++n)C(!fineCache[n].valid,"no stale resident key survives identity change");
 Prepare();C(published==2&&retired==3,"empty identity transition has no invented GPU release");
 printf("PASS %u actual identity-retirement controls\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('PREFIX', prefix)
(out / 'fine-identity.cpp').write_text(code)
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{vc}/include;{sdk}/Include', LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for mode in ('Od', 'O2'):
    exe = out / 'fine-identity.exe'
    build = subprocess.run([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MT', '/'+mode,
                            str(out/'fine-identity.cpp'), '/Fo'+str(out/'fine-identity.obj'), '/Fe'+str(exe)],
                           cwd=out, env=env, capture_output=True, text=True)
    assert build.returncode == 0, build.stdout + build.stderr
    run = subprocess.run([str(exe)], capture_output=True, text=True)
    (out / ('fine-identity-'+mode+'.log')).write_text(run.stdout + run.stderr)
    print(run.stdout + run.stderr, end='')
    assert run.returncode == 0
