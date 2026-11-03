"""Extract the real publication branch; mocks supply only coordinator callbacks.

The actual WIC/WARP coordinator suite proves acknowledgement/drain/freshness.
This narrow caller test proves a non-center pair is published when PeekRead
admits it, without waiting for every later pair. The legacy gate is killed.
"""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE.parent/'build';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
text=(HERE.parent/'GsmFineViewer.h').read_text();a=text.index('else if(result==3||fineOwner->ReadReady())');b=text.index('else if(result==4)',a);branch=text[a+5:b]
prefix=r'''#include <cstdio>
namespace FineStore{struct View{unsigned proof;};}
struct Owner{bool ready,allowed;unsigned releases;Owner():ready(true),allowed(true),releases(0){}bool ReadReady(){return ready;}bool PeekRead(bool,int,int,int,int,FineStore::View&v){v.proof=allowed?7:0;return allowed;}void ReleaseRead(){++releases;ready=false;}}owner,*fineOwner=&owner;
int fineDemand=1,fineIdentity=2;unsigned fineRead=0,uploads=0;bool fineCenter=false;unsigned finePair=1;struct Plan{unsigned pairCount;Plan():pairCount(14){}}finePlan;bool succeed=true,fail=false;
int FineResidency(){return 3;}int FineCpuLedger(){return 4;}bool FineUpload(const FineStore::View&v){if(v.proof!=7)throw 2;if(fail)throw 3;if(succeed)++uploads;return succeed;}
void C(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
void Complete(int result){'''
suffix=r'''}
void Reset(){owner=Owner();fineRead=uploads=0;succeed=true;fail=false;}
int main(){try{
 Reset();Complete(3);C(uploads==1&&owner.releases==1&&fineRead==1,"non-center completed pair published before14pair batch end");
 Reset();owner.allowed=false;Complete(3);C(!uploads&&!owner.releases&&!fineRead,"unadmitted current read remains pending");owner.allowed=true;Complete(0);C(uploads==1&&owner.releases==1&&fineRead==1,"held read retried without requiring a second completion edge");
 Reset();succeed=false;Complete(3);C(!uploads&&owner.releases==1&&!fineRead,"failed GPU admission releases synchronous view without skipping key");
 Reset();fail=true;bool caught=false;try{Complete(3);}catch(...){caught=true;}C(caught&&owner.releases==1&&!fineRead,"upload exception releases exposed view before outer cancellation");
 puts("PASS extracted progressive pair publication and read-release controls");return 0;}catch(...){return 1;}}'''
def run(label,body,opt,red=False):
 source=OUT/(label+'.cpp');source.write_text(prefix+body+suffix);exe=source.with_suffix('.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(source),'/Fo'+str(source.with_suffix('.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(OUT/(label+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,p.stdout+p.stderr
 p=subprocess.run([str(exe)],env=env,capture_output=True,text=True);(OUT/(label+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert (p.returncode!=0)==red
legacy=branch.replace('if(!FineUpload(view))','if((fineCenter||finePair>=finePlan.pairCount)&&!FineUpload(view))');assert legacy!=branch
run('fine-progressive-legacy-batch-red',legacy,'/Od',True)
run('fine-progressive-Od',branch,'/Od');run('fine-progressive-O2',branch,'/O2')
