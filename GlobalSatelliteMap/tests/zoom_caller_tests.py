"""Production-extracted ApplyZoomLimit/UpdateHook lifecycle controls.
Native dispatch/profile/WidgetRef edges are fakes. No process/UI/native game calls.
Only build/zoom-caller-tests outputs are written; default runs28checks+8mutants.
"""
from pathlib import Path
import os,subprocess,hashlib,json,argparse,sys,re
parser=argparse.ArgumentParser(description="Compile actual zoom caller guards/rollback with a fake native clamp edge; never invoke game APIs")
parser.add_argument('--skip-mutants',action='store_true')
parser.add_argument('--case-source',type=Path)
parser.add_argument('--case-name',default='positive')
args=parser.parse_args()
if not re.fullmatch(r"[A-Za-z0-9_-]{1,64}",args.case_name):
    parser.error("case-name must be a bounded output-folder name")
P=Path(__file__).resolve().parents[1]
rootOut=P/'build/zoom-caller-tests';H=rootOut/args.case_name;H.mkdir(parents=True,exist_ok=True)
sourcePath=args.case_source or P/'GlobalSatelliteMap.cpp'
source=sourcePath.read_text(encoding='utf-8-sig')
a=source.index('void ApplyZoomLimit(MapScreen*map)');b=source.index('void UpdateHook(MapScreen*map)',a);rawApply=source[a:b];apply=rawApply
# Replace only the audited native UI clamp boundary. Caller guards/state/rollback are verbatim.
edge='reinterpret_cast<Clamp>(reinterpret_cast<BYTE*>(GetModuleHandleA(NULL))+GsmZoomInRva)(map,NULL)';assert apply.count(edge)==1;apply=apply.replace(edge,'FakeClamp(map,NULL)')
start=source.index('void Clear(){');end=source.index('\nvoid ',start+1);assert 'zoomLease' not in source[start:end] and 'zoomOwner' not in source[start:end], 'ordinary GSM Clear must not forget an active native-field lease'
hook=source[b:source.index('void LoadHook(',b)]
text=r"""#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include "__POLICY__"
unsigned checks=0,visibilityQueries=0,widthQueries=0,heightQueries=0,profileQueries=0,ownerWrites=0,clamps=0,errors=0,diagnostics=0,originals=0,ticks=0;bool profileOK=true,managerReady=true,throwClamp=false;
namespace MyGUI {struct Widget {int width,height;bool alive;Widget(int w=16384,int h=16384):width(w),height(h),alive(true){}int getWidth(){++widthQueries;if(!alive)throw std::runtime_error("deadwidget");return width;}int getHeight(){++heightQueries;if(!alive)throw std::runtime_error("deadwidget");return height;}};}
namespace Ogre {struct Vector4{float v[4];Vector4(){v[0]=.3f;v[1]=.4f;v[2]=443;v[3]=464;}};}
struct MapScreen {MyGUI::Widget*mainWidget,*mapImage,*mapScrollView;float zoomValueBase,zoomValueMax,zoomValueCurrent,zoomLevels[3];Ogre::Vector4 zoomCenterOffset;bool zoomUpdated,visible;MapScreen(MyGUI::Widget*m,MyGUI::Widget*i,MyGUI::Widget*s):mainWidget(m),mapImage(i),mapScrollView(s),zoomValueBase(.1209716796875f),zoomValueMax(2),zoomValueCurrent(2),zoomUpdated(false),visible(true){zoomLevels[0]=.2f;zoomLevels[1]=.53333336f;zoomLevels[2]=1;}bool getVisible(){++visibilityQueries;return visible;}};
struct WidgetRef {MyGUI::Widget*p;WidgetRef():p(NULL){}MyGUI::Widget*get(){return p;}void set(MyGUI::Widget*w){++ownerWrites;if(!managerReady)return;p=w;}void unlink(MyGUI::Widget*w){if(p==w)p=NULL;}void reset(){p=NULL;}};
bool enabled=false,zoomFailed=false;int zoomMultiplier=1;GsmZoom::Lease zoomLease;WidgetRef zoomOwner;
bool ZoomNativeProfile(){++profileQueries;return profileOK;}void ErrorLog(const std::string&){++errors;}void Diagnostic(const std::string&){++diagnostics;}
void FakeClamp(MapScreen*m,MyGUI::Widget*){++clamps;m->zoomValueCurrent=(std::min)(m->zoomValueCurrent+m->zoomValueBase,m->zoomValueMax);m->zoomCenterOffset.v[0]=.5f;m->zoomCenterOffset.v[1]=.5f;m->zoomCenterOffset.v[2]=1080;m->zoomCenterOffset.v[3]=522;m->zoomUpdated=true;if(throwClamp)throw std::runtime_error("native clamp edge failure");}
void Original(MapScreen*m){++originals;if(m&&m->zoomUpdated){m->mapImage->width=m->mapImage->height=int(m->zoomValueCurrent*8192+.5f);m->zoomUpdated=false;}}void(*originalUpdate)(MapScreen*)=&Original;void Tick(MapScreen*){++ticks;}void Fail(MapScreen*,const char*){++errors;}
__APPLY__
__HOOK__
void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s check%u\n",s,checks);throw 1;}}
void Reset(){zoomLease.Reset();zoomOwner.reset();zoomFailed=false;enabled=false;zoomMultiplier=1;profileOK=managerReady=true;throwClamp=false;visibilityQueries=widthQueries=heightQueries=profileQueries=ownerWrites=clamps=errors=diagnostics=originals=ticks=0;}
void SetupActive(MapScreen&m){enabled=true;zoomMultiplier=2;ApplyZoomLimit(&m);C(m.zoomValueMax==4&&m.zoomValueCurrent==2&&zoomLease.Owned(),"actualcaller raises cap withoutmovingcurrent");}
int main(){try{
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);ApplyZoomLimit(&m);C(visibilityQueries==0&&profileQueries==0&&widthQueries==0&&heightQueries==0&&ownerWrites==0,"defaultOFF performszeroqueries andnoWidgetManagerregistration");C(m.zoomValueMax==2&&m.zoomValueCurrent==2&&!m.zoomUpdated,"OFF fields unchanged");enabled=true;zoomMultiplier=1;ApplyZoomLimit(&m);C(!visibilityQueries&&!profileQueries&&!ownerWrites,"native1 unowned alsozeroqueries");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);float levels[3];std::memcpy(levels,m.zoomLevels,12);for(unsigned i=0;i<20;++i)ApplyZoomLimit(&m);C(m.zoomValueMax==4&&clamps==0&&diagnostics==1,"idempotent norepeat nativeclamp/writeevent");C(!std::memcmp(levels,m.zoomLevels,12)&&m.zoomValueBase==.1209716796875f,"markerthreshold/min fields preserved");m.zoomValueCurrent=4;image.width=image.height=32768;enabled=false;ApplyZoomLimit(&m);C(m.zoomValueMax==2&&m.zoomValueCurrent==2&&clamps==1&&!zoomLease.Owned(),"OFF restores originalcap viaexactlyone nativeanchorclamp");C(m.zoomUpdated&&m.zoomCenterOffset.v[0]==.5f,"nativecentreanchor used");Original(&m);ApplyZoomLimit(&m);C(clamps==1,"OFF restored norepeat");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);m.zoomValueCurrent=4;image.width=image.height=32768;m.zoomValueCurrent=m.zoomValueBase;m.zoomUpdated=true;ApplyZoomLimit(&m);C(!zoomFailed&&m.zoomValueMax==4&&clamps==0,"worldclear pending nativeresize doesnotrefuse/compound");Original(&m);ApplyZoomLimit(&m);C(!zoomFailed&&m.zoomValueMax==4&&m.zoomValueCurrent==m.zoomValueBase,"worldclear resize completes andpreserves stablecap");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);m.zoomValueCurrent=m.zoomValueBase;image.width=image.height=32768;m.zoomUpdated=true;zoomMultiplier=8;ApplyZoomLimit(&m);C(!zoomFailed,"pending nativeresize plusnewfactor mustnotrefuse stale dimensions");Original(&m);ApplyZoomLimit(&m);C(!zoomFailed&&m.zoomValueMax==16&&m.zoomValueCurrent==m.zoomValueBase,"deferrednewfactor applied aftercorrect nativeresize");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);zoomOwner.unlink(&main);main.alive=false;MyGUI::Widget freshMain,freshImage,freshScroll;MapScreen fresh(&freshMain,&freshImage,&freshScroll);ApplyZoomLimit(&fresh);C(zoomOwner.get()==&freshMain&&fresh.zoomValueMax==4&&zoomLease.Owned(),"destroyedowner discarded andnewowner acquired");enabled=false;ApplyZoomLimit(&fresh);C(fresh.zoomValueMax==2&&m.zoomValueMax==4,"newowner restore neverdereferences/writesoldmap");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);m.zoomValueCurrent=4;image.width=image.height=32768;Ogre::Vector4 before=m.zoomCenterOffset;const bool dirty=m.zoomUpdated;enabled=false;throwClamp=true;bool threw=false;try{ApplyZoomLimit(&m);}catch(...){threw=true;}C(threw&&m.zoomValueMax==4&&m.zoomValueCurrent==4&&!std::memcmp(&before,&m.zoomCenterOffset,sizeof(before))&&m.zoomUpdated==dirty&&zoomLease.Owned(),"nativeclamp exception exactrollback allfields/lease/dirty");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);m.zoomValueCurrent=4;image.width=image.height=32768;enabled=false;throwClamp=true;UpdateHook(&m);C(zoomFailed&&errors==1&&originals==1&&ticks==1,"outercatch latchesfailure butnativeupdate+tick stillrun");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);enabled=true;zoomMultiplier=2;profileOK=false;ApplyZoomLimit(&m);C(zoomFailed&&m.zoomValueMax==2&&clamps==0&&errors==1,"profilefail no mutation");unsigned n=profileQueries;ApplyZoomLimit(&m);C(profileQueries==n&&errors==1,"latchedprofilefail nohotretry");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);enabled=true;zoomMultiplier=2;managerReady=false;ApplyZoomLimit(&m);C(m.zoomValueMax==2&&!zoomLease.Owned()&&zoomOwner.get()==NULL,"declinedWidgetRef ownership mustnotmutate max");}
 {Reset();MyGUI::Widget main,image,scroll;MapScreen m(&main,&image,&scroll);SetupActive(m);m.zoomValueMax=6;ApplyZoomLimit(&m);C(m.zoomValueMax==6&&!zoomLease.Owned(),"foreignwriter notoverwritten");for(unsigned i=0;i<10;++i)ApplyZoomLimit(&m);C(m.zoomValueMax==6,"foreignwriter suspension nofight");}
 std::printf("PASS actual zoom caller %u checks\n",checks);return 0;}catch(...){return 1;}}
""".replace('__POLICY__',str(P/'GsmZoomPolicy.h')).replace('__APPLY__',apply).replace('__HOOK__',hook)
(H/'caller.cpp').write_text(text,encoding='utf-8');VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2',str(H/'caller.cpp'),'/Fo'+str(H/'caller.obj'),'/Fe'+str(H/'caller.exe')],env=env,capture_output=True,text=True);(H/'build.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stdout+r.stderr
r=subprocess.run([str(H/'caller.exe')],capture_output=True,text=True);(H/'run.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='');assert r.returncode==0
(H/'identity.json').write_text(json.dumps(dict(policy=hashlib.sha256((P/'GsmZoomPolicy.h').read_bytes()).hexdigest(),main=hashlib.sha256((P/'GlobalSatelliteMap.cpp').read_bytes()).hexdigest(),applySourceSha256=hashlib.sha256(rawApply.encode()).hexdigest(),applyCompiledSeamSha256=hashlib.sha256(apply.encode()).hexdigest(),hookSha256=hashlib.sha256(hook.encode()).hexdigest()),indent=2)+'\n')

if not args.skip_mutants:
    cases=[
        ('offqueries','if((!enabled||zoomMultiplier==1)&&!zoomLease.Owned())return;',''),
        ('pendingresize','if(map->zoomUpdated)return;',''),
        ('rollbackdirty','map->zoomUpdated=oldDirty;','map->zoomUpdated=true;'),
        ('untrackedowner','if(zoomOwner.get()!=map->mainWidget)return;',''),
        ('missingclamp','if(action.clamp){typedef void(__fastcall*Clamp)','if(false){typedef void(__fastcall*Clamp)'),
        ('profilebypass','if(!ZoomNativeProfile())','if(false)'),
        ('profilehotretry','if(zoomFailed)return;',''),
        ('wrongthreshold','map->zoomLevels[2],map->zoomValueMax','map->zoomLevels[1],map->zoomValueMax'),
    ]
    results=[]
    for name,old,new in cases:
        assert source.count(old)==1,(name,source.count(old))
        folder=rootOut/('mutant-'+name);folder.mkdir(exist_ok=True)
        mutant=folder/'Main-mutant.cpp';mutant.write_text(source.replace(old,new),encoding='utf-8')
        run=subprocess.run([sys.executable,str(Path(__file__).resolve()),'--skip-mutants','--case-name','mutant-'+name,'--case-source',str(mutant)],capture_output=True,text=True)
        (folder/'expected-red.log').write_text(run.stdout+run.stderr,encoding='utf-8')
        assert run.returncode!=0 and 'FAIL ' in run.stdout,(name,run.stdout+run.stderr)
        results.append(dict(name=name,result=run.stdout.strip()))
        print('Expected RED',name)
    (rootOut/'mutants.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
    print('PASS',len(results),'compiled actual zoom caller mutants')
