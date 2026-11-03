from pathlib import Path
import os,subprocess,sys,json,hashlib
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
src=(root/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig');wrappers=src[src.index('bool RelativeZoomEnabled('):src.index('bool ZoomNativeProfile(){')]
apply=src[src.index('void ApplyZoomLimit('):src.index('void UpdateHook(')]
fallback='reinterpret_cast<Clamp>(reinterpret_cast<BYTE*>(GetModuleHandleA(NULL))+GsmZoomInRva)(map,NULL)';assert apply.count(fallback)==1;apply=apply.replace(fallback,'NativeIn(map,NULL)')
tickEntry=src[src.index('void Tick(MapScreen*map){'):src.index('FineObserve(map);',src.index('void Tick(MapScreen*map){'))]
program=r'''
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include "__STEP__"
#include "__POLICY__"
namespace MyGUI{struct Widget{int getWidth(){return 16384;}int getHeight(){return 16384;}};}
namespace Ogre{struct Vector4{float x,y,z,w;};}
struct MapScreen{MyGUI::Widget*mainWidget,*mapImage,*mapScrollView;float zoomValueBase,zoomValueCurrent,zoomValueMax,zoomLevels[3];bool zoomUpdated;float anchor;Ogre::Vector4 zoomCenterOffset;bool getVisible(){return true;}};
struct Ref{MyGUI::Widget*p;MyGUI::Widget*get(){return p;}void set(MyGUI::Widget*w){p=w;}};Ref zoomOwner={};GsmZoom::Lease zoomLease;
bool zoomStepHooksReady=true,enabled=true,zoomFailed=false,inside=true,throws=false,nested=false;unsigned zoomStepDepth=0;int zoomMultiplier=8;unsigned calls[3]={};MyGUI::Widget*seenSender=NULL;float entryCurrent=0,zoomInputMinimum=.1209716796875f,zoomInputPeak=1;
void ZoomOutHook(MapScreen*,MyGUI::Widget*);void ApplyZoomLimit(MapScreen*);void Tick(MapScreen*);unsigned profiles=0,observed=0;bool reenterLifecycle=false;
void NativeIn(MapScreen*m,MyGUI::Widget*s){++calls[0];seenSender=s;if(!m)return;entryCurrent=m->zoomValueCurrent;if(reenterLifecycle){ApplyZoomLimit(m);Tick(m);}if(nested){nested=false;ZoomOutHook(m,s);}float next=(std::min)(m->zoomValueCurrent+m->zoomValueBase,m->zoomValueMax);if(next!=m->zoomValueCurrent){m->zoomValueCurrent=next;m->zoomUpdated=true;m->anchor=.5f;}if(throws)throw std::runtime_error("native");}
void NativeOut(MapScreen*m,MyGUI::Widget*s){++calls[1];seenSender=s;if(!m)return;m->zoomValueCurrent=(std::max)(m->zoomValueCurrent-m->zoomValueBase,m->zoomValueBase);if(throws)throw std::runtime_error("native");}
void NativeWheel(MapScreen*m,MyGUI::Widget*s,int rel){++calls[2];seenSender=s;if(!m||!inside)return;entryCurrent=m->zoomValueCurrent;float next=(std::max)(m->zoomValueBase,(std::min)(m->zoomValueCurrent+rel*.0006250000442378223f,m->zoomValueMax));if(next!=m->zoomValueCurrent){m->zoomValueCurrent=next;m->zoomUpdated=true;m->anchor=.375f;}if(throws)throw std::runtime_error("native");}
void(*originalZoomIn)(MapScreen*,MyGUI::Widget*)=NativeIn;void(*originalZoomOut)(MapScreen*,MyGUI::Widget*)=NativeOut;void(*originalZoomWheel)(MapScreen*,MyGUI::Widget*,int)=NativeWheel;
bool ZoomNativeProfile(){++profiles;return true;}void ErrorLog(const std::string&){}void Diagnostic(const std::string&){}const unsigned GsmZoomInRva=0x48bb50;
__WRAPPERS__
__APPLY__
__TICK_ENTRY__++observed;}
unsigned checks=0;void C(bool ok,const char*why){++checks;if(!ok){printf("FAIL %s\n",why);throw 1;}}
void Reset(MapScreen&m,MyGUI::Widget&w){m.mainWidget=m.mapImage=m.mapScrollView=&w;m.zoomValueBase=.1209716796875f;m.zoomValueCurrent=2;m.zoomValueMax=2;m.zoomLevels[2]=1;m.zoomUpdated=false;m.anchor=0;zoomOwner.p=&w;zoomLease.Reset();GsmZoom::Action a;if(zoomLease.Plan(true,8,m.zoomValueBase,1,2,2,16384,16384,a)!=GsmZoom::Apply)throw 1;m.zoomValueMax=a.maximum;calls[0]=calls[1]=calls[2]=0;inside=true;throws=nested=reenterLifecycle=false;profiles=observed=0;enabled=zoomStepHooksReady=true;zoomFailed=false;zoomMultiplier=8;zoomStepDepth=0;}
int main(){try{MapScreen m={};MyGUI::Widget w,s;Reset(m,w);ZoomInHook(&m,&s);C(m.zoomValueCurrent==2.5f&&calls[0]==1&&calls[1]==0&&seenSender==&s,"button In exact1.25 via one native center clamp");C(entryCurrent==2&&m.anchor==.5f&&m.zoomValueBase==.1209716796875f&&m.zoomValueMax==16&&zoomStepDepth==0,"button old scalar/anchor preserved and both bounds restored");ZoomOutHook(&m,&s);C(m.zoomValueCurrent==2&&calls[0]==2&&calls[1]==0&&m.anchor==.5f,"button Out exact1 via native center clamp");
 const int deltas[]={1,120,240,-1,-120,-240,2147483647,-2147483647-1};const float targets[]={2.5f,2.5f,3,1.5f,1.5f,1,16,.1209716796875f};for(unsigned i=0;i<8;++i){Reset(m,w);ZoomWheelHook(&m,&s,deltas[i]);C(m.zoomValueCurrent==targets[i]&&calls[2]==1&&calls[0]==0&&entryCurrent==2&&m.anchor==.375f,"wheel exact bounded target with original cursor anchor");C(m.zoomValueBase==.1209716796875f&&m.zoomValueMax==16&&zoomStepDepth==0,"wheel bounds and reentry depth restored");}
 Reset(m,w);inside=false;ZoomWheelHook(&m,&s,120);C(m.zoomValueCurrent==2&&!m.zoomUpdated&&calls[2]==1&&m.zoomValueMax==16,"native outside-viewport wheel stays no-op");Reset(m,w);ZoomWheelHook(&m,&s,0);C(m.zoomValueCurrent==2&&!m.zoomUpdated&&calls[2]==1,"zero wheel stays native no-op");
 for(unsigned k=0;k<3;++k){Reset(m,w);throws=true;bool caught=false;try{if(k==0)ZoomInHook(&m,&s);else if(k==1)ZoomOutHook(&m,&s);else ZoomWheelHook(&m,&s,120);}catch(const std::runtime_error&){caught=true;}C(caught&&m.zoomValueBase==.1209716796875f&&m.zoomValueMax==16&&zoomStepDepth==0,"native throw restores exact temporary ownership");}
 Reset(m,w);enabled=false;ZoomOutHook(&m,&s);C(calls[1]==1&&calls[0]==0,"OFF original matching Out preserved");Reset(m,w);zoomStepDepth=2;ZoomWheelHook(&m,&s,120);C(calls[2]==1&&m.zoomValueMax==16&&zoomStepDepth==2&&m.zoomValueCurrent!=2.5f,"nested raw wheel is not compounded");
 Reset(m,w);reenterLifecycle=true;ZoomInHook(&m,&s);C(m.zoomValueCurrent==2.5f&&zoomLease.Owned()&&profiles==0&&observed==0&&m.zoomValueMax==16&&zoomStepDepth==0,"nested native getter cannot misread temporary limit or run render demand");m.zoomUpdated=false;ApplyZoomLimit(&m);Tick(&m);C(zoomLease.Owned()&&profiles==1&&observed==1,"normal lifecycle resumes after scoped input returns");
 Reset(m,w);zoomMultiplier=1;zoomLease.Reset();m.zoomValueMax=2;m.zoomValueCurrent=1.49f;ZoomWheelHook(&m,&s,120);C(m.zoomValueCurrent==1.5f&&!zoomLease.Owned()&&m.zoomValueMax==2&&m.anchor==.375f,"native1x limit still has exact fixed steps without fabricated lease");
 Reset(m,w);zoomMultiplier=1;zoomLease.Reset();m.zoomValueMax=2;inside=false;ZoomWheelHook(&m,&s,120);C(m.zoomValueCurrent==2&&!m.zoomUpdated,"native1x outside viewport remains no-op");
 Reset(m,w);zoomMultiplier=1;zoomLease.Reset();m.zoomValueMax=3;m.zoomValueCurrent=1.49f;ZoomWheelHook(&m,&s,120);C(m.zoomValueCurrent!=1.5f&&m.zoomValueMax==3,"foreign native maximum passes raw input through");
 Reset(m,w);zoomMultiplier=1;zoomLease.Reset();m.zoomValueMax=2;m.zoomValueBase=.2f;m.zoomValueCurrent=1.49f;ZoomWheelHook(&m,&s,120);C(m.zoomValueCurrent!=1.5f&&m.zoomValueBase==.2f,"foreign native minimum passes raw input through");
 printf("PASS %u actual fixed zoom caller controls\n",checks);return 0;}catch(...){return 1;}}
'''.replace('__STEP__',str(root/'GsmZoomStep.h')).replace('__POLICY__',str(root/'GsmZoomPolicy.h')).replace('__WRAPPERS__',wrappers).replace('__APPLY__',apply).replace('__TICK_ENTRY__',tickEntry)
(out/'caller.cpp').write_text(program,encoding='utf-8');vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64');results={}
for mode in ('Od','O2'):
 exe=out/('caller-'+mode+'.exe');p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,str(out/'caller.cpp'),'/Fo'+str(out/('caller-'+mode+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(out/('caller-build-'+mode+'.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);sys.exit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(out/('caller-run-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 results[mode]=p.stdout.strip()
(here/'caller-verification.json').write_text(json.dumps({'wrapperSha256':hashlib.sha256(wrappers.encode()).hexdigest(),'results':results,'scope':'Actual production wrapper bodies; bounded fake native methods matching audited arithmetic/anchors. No game/UI.'},indent=2)+'\n')
