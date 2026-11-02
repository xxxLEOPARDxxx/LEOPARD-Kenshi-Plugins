"""Compile production zoom wrappers/install/clamp with fake native boundaries.

No game module is loaded and no UI/process/native game method is called. Outputs
are confined to build/zoom-step-caller-tests. Mutants must compile and fail an
actual assertion; compiler failure is never accepted as a negative control.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--skip-mutants', action='store_true')
parser.add_argument('--case-source', type=Path)
parser.add_argument('--case-step', type=Path)
parser.add_argument('--case-name', default='positive')
args = parser.parse_args()
if not re.fullmatch(r'[A-Za-z0-9_-]{1,64}', args.case_name):
    parser.error('case-name must be a bounded output-folder name')
P = Path(__file__).resolve().parents[1]
out = P / 'build/zoom-step-caller-tests'
folder = out / args.case_name
folder.mkdir(parents=True, exist_ok=True)
source = (args.case_source or P / 'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig')
wrappers = source[source.index('bool RelativeZoomEnabled('):source.index('bool ZoomNativeProfile(){')]
install = source[source.index('void InstallZoomSteps(){'):source.index('void ApplyZoomLimit(')]
apply_raw = source[source.index('void ApplyZoomLimit('):source.index('void UpdateHook(')]
fallback = 'reinterpret_cast<Clamp>(reinterpret_cast<BYTE*>(GetModuleHandleA(NULL))+GsmZoomInRva)(map,NULL)'
assert apply_raw.count(fallback) == 1
apply = apply_raw.replace(fallback, 'FallbackClamp(map,NULL)')
program = r'''
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include "__STEP__"
#include "__POLICY__"
unsigned checks=0,queries=0,profiles=0,errors=0,calls[3]={0,0,0},fallbackCalls=0,depthSeen=0;
float seenBase=0;int seenRelative=0;bool throwNative=false,reenter=false,profileOK=true;
namespace MyGUI {struct Widget {int width,height;Widget():width(16384),height(16384){}int getWidth(){++queries;return width;}int getHeight(){++queries;return height;}};}
namespace Ogre {struct Vector4 {float x,y,z,w;Vector4():x(.2f),y(.3f),z(40),w(50){}};}
struct MapScreen {MyGUI::Widget*mainWidget,*mapImage,*mapScrollView;float zoomValueBase,zoomValueCurrent,zoomValueMax,zoomLevels[3];bool zoomUpdated;Ogre::Vector4 zoomCenterOffset;
 MapScreen(MyGUI::Widget*w):mainWidget(w),mapImage(w),mapScrollView(w),zoomValueBase(.125f),zoomValueCurrent(2),zoomValueMax(4),zoomUpdated(false){zoomLevels[0]=.2f;zoomLevels[1]=.5f;zoomLevels[2]=1;}
 bool getVisible(){++queries;return true;}};
struct WidgetRef {MyGUI::Widget*p;WidgetRef():p(NULL){}MyGUI::Widget*get(){++queries;return p;}void set(MyGUI::Widget*w){p=w;}};
bool enabled=false,zoomFailed=false,zoomStepHooksReady=false;unsigned zoomStepDepth=0;int zoomMultiplier=1;GsmZoom::Lease zoomLease;WidgetRef zoomOwner;
typedef void(*Button)(MapScreen*,MyGUI::Widget*);typedef void(*Wheel)(MapScreen*,MyGUI::Widget*,int);
Button originalZoomIn=NULL,originalZoomOut=NULL;Wheel originalZoomWheel=NULL;
MyGUI::Widget*seenSender=NULL;MapScreen*seenMap=NULL;
void ZoomInHook(MapScreen*,MyGUI::Widget*);void ZoomOutHook(MapScreen*,MyGUI::Widget*);void ZoomWheelHook(MapScreen*,MyGUI::Widget*,int);
void NativeCommon(MapScreen*m,MyGUI::Widget*s,unsigned kind){++calls[kind];seenMap=m;seenSender=s;depthSeen=zoomStepDepth;if(m)seenBase=m->zoomValueBase;}
void NativeIn(MapScreen*m,MyGUI::Widget*s){NativeCommon(m,s,0);if(reenter){reenter=false;ZoomOutHook(m,s);}if(m){m->zoomValueCurrent=(std::min)(m->zoomValueCurrent+m->zoomValueBase,m->zoomValueMax);m->zoomUpdated=true;m->zoomCenterOffset.x=.5f;}if(throwNative)throw std::runtime_error("fake native failure");}
void NativeOut(MapScreen*m,MyGUI::Widget*s){NativeCommon(m,s,1);if(m){m->zoomValueCurrent=(std::max)(m->zoomValueCurrent-m->zoomValueBase,m->zoomValueBase);m->zoomUpdated=true;m->zoomCenterOffset.x=.5f;}if(throwNative)throw std::runtime_error("fake native failure");}
void NativeWheel(MapScreen*m,MyGUI::Widget*s,int relative){NativeCommon(m,s,2);seenRelative=relative;if(m){m->zoomValueCurrent=(std::max)(m->zoomValueBase,(std::min)(m->zoomValueCurrent+relative*.0006250000442378223f,m->zoomValueMax));m->zoomUpdated=true;m->zoomCenterOffset.x=.375f;}if(throwNative)throw std::runtime_error("fake native failure");}
void FallbackClamp(MapScreen*m,MyGUI::Widget*s){++fallbackCalls;NativeIn(m,s);}
bool ZoomNativeProfile(){++profiles;return profileOK;}
void ErrorLog(const std::string&){++errors;}void Diagnostic(const std::string&){}
static const unsigned GsmZoomInRva=0x48bb50,GsmZoomOutRva=0x48bc90,GsmZoomWheelRva=0x48b9e0;
unsigned installIndex=0,successMask=7,nullMask=0,returnedFailureMask=0,probeCalls=0;MapScreen*probeMap=NULL;
namespace KenshiLib {enum HookStatus {SUCCESS,FAILURE};
template<class T> HookStatus AddHook(BYTE*,T hook,T*original){unsigned i=installIndex++;bool ok=(successMask&(1<<i))!=0;
 if((ok||(returnedFailureMask&(1<<i)))&&!(nullMask&(1<<i))){*original=i==0?reinterpret_cast<T>(&NativeIn):i==1?reinterpret_cast<T>(&NativeOut):reinterpret_cast<T>(&NativeWheel);
  // Simulate input between hook registrations, after trampoline publication.
  if(probeMap){++probeCalls;if(i==0)ZoomInHook(probeMap,NULL);else if(i==1)ZoomOutHook(probeMap,NULL);else ZoomWheelHook(probeMap,NULL,120);}}
 return ok?SUCCESS:FAILURE;}}
__WRAPPERS__
__INSTALL__
__APPLY__
void C(bool value,const char*name){++checks;if(!value){std::printf("FAIL %s check%u\n",name,checks);throw 1;}}
bool Near(float a,float b){return std::fabs(a-b)<.000001f;}
void Reset(MapScreen&m){enabled=true;zoomMultiplier=2;zoomFailed=false;zoomStepHooksReady=true;zoomStepDepth=0;zoomOwner.p=m.mainWidget;zoomLease.Reset();calls[0]=calls[1]=calls[2]=0;queries=profiles=errors=fallbackCalls=depthSeen=0;throwNative=reenter=false;profileOK=true;installIndex=0;successMask=7;nullMask=returnedFailureMask=probeCalls=0;probeMap=NULL;seenMap=NULL;seenSender=NULL;seenRelative=0;seenBase=0;originalZoomIn=NativeIn;originalZoomOut=NativeOut;originalZoomWheel=NativeWheel;m.zoomValueBase=.125f;m.zoomValueCurrent=2;m.zoomValueMax=2;m.zoomUpdated=false;GsmZoom::Action a;if(zoomLease.Plan(true,2,m.zoomValueBase,m.zoomLevels[2],m.zoomValueMax,m.zoomValueCurrent,16384,16384,a)!=GsmZoom::Apply||!zoomLease.Owned())throw std::runtime_error("fixture must establish real owned cap");m.zoomValueMax=a.maximum;}
int main(){try{MyGUI::Widget owner,sender;MapScreen m(&owner);
 // Positive wrappers preserve actual native call/anchor and restore scoped base.
 Reset(m);ZoomInHook(&m,&sender);C(calls[0]==1&&calls[1]==0&&calls[2]==0&&seenMap==&m&&seenSender==&sender,"In exactly once with map/sender");C(Near(seenBase,.4f)&&Near(m.zoomValueCurrent,2.4f)&&m.zoomValueBase==.125f&&zoomStepDepth==0&&depthSeen==1,"In relative step scoped and restored");C(m.zoomUpdated&&m.zoomCenterOffset.x==.5f,"In native center anchor retained");
 Reset(m);ZoomOutHook(&m,&sender);C(calls[1]==1&&Near(seenBase,.4f)&&Near(m.zoomValueCurrent,1.6f)&&m.zoomValueBase==.125f&&zoomStepDepth==0,"Out relative step and restoration");
 Reset(m);m.zoomValueCurrent=.1875f;ZoomOutHook(&m,&sender);C(m.zoomValueCurrent==.125f&&seenBase==.125f,"Out low-current native minimum reachable");
 for(unsigned k=0;k<3;++k){Reset(m);throwNative=true;bool threw=false;try{if(k==0)ZoomInHook(&m,&sender);else if(k==1)ZoomOutHook(&m,&sender);else ZoomWheelHook(&m,&sender,120);}catch(const std::runtime_error&){threw=true;}C(threw&&calls[k]==1&&m.zoomValueBase==.125f&&zoomStepDepth==0,"C++ native exception restores base/depth");}
 const int rels[]={0,1,-1,120,-120,240,-240,2147483647,(-2147483647-1)};
 for(unsigned i=0;i<sizeof(rels)/sizeof(rels[0]);++i){Reset(m);int raw=rels[i];ZoomWheelHook(&m,&sender,raw);int want=raw>8191?32767:raw<-8191?-32767:raw*4;C(calls[2]==1&&seenRelative==want&&seenMap==&m&&seenSender==&sender&&m.zoomValueBase==.125f&&zoomStepDepth==0,"Wheel raw sign/batch/extremes transformed exactly once");C(m.zoomCenterOffset.x==.375f,"Wheel native cursor anchor retained");}
 // These exits must short-circuit before owner/widget/profile queries.
 for(unsigned k=0;k<4;++k){Reset(m);if(k==0)enabled=false;if(k==1)zoomMultiplier=1;if(k==2)zoomFailed=true;if(k==3)zoomStepHooksReady=false;ZoomInHook(&m,&sender);ZoomOutHook(&m,&sender);ZoomWheelHook(&m,&sender,120);C(calls[0]==1&&calls[1]==1&&calls[2]==1&&seenRelative==120&&seenBase==.125f&&queries==0&&profiles==0,"OFF/1x/failed/incomplete zero-query native passthrough");}
 Reset(m);ZoomInHook(NULL,&sender);ZoomOutHook(NULL,&sender);ZoomWheelHook(NULL,&sender,-120);C(calls[0]==1&&calls[1]==1&&calls[2]==1&&seenMap==NULL&&seenRelative==-120&&queries==0,"null map native passthrough");
 Reset(m);zoomOwner.p=&sender;ZoomWheelHook(&m,&sender,120);C(seenRelative==120&&calls[2]==1,"different live owner passthrough");
 Reset(m);zoomLease.Reset();ZoomInHook(&m,&sender);ZoomOutHook(&m,&sender);ZoomWheelHook(&m,&sender,120);C(calls[0]==1&&calls[1]==1&&calls[2]==1&&seenBase==.125f&&seenRelative==120&&queries==0&&profiles==0,"unowned matching owner cannot accelerate");
 Reset(m);m.zoomValueMax=6;ApplyZoomLimit(&m);C(!zoomLease.Owned()&&m.zoomValueMax==6&&!zoomFailed,"actual caller suspends lease for foreign maximum");queries=profiles=0;ZoomInHook(&m,&sender);ZoomOutHook(&m,&sender);ZoomWheelHook(&m,&sender,-120);C(calls[0]==1&&calls[1]==1&&calls[2]==1&&seenBase==.125f&&seenRelative==-120&&m.zoomValueMax==6&&queries==0&&profiles==0,"foreign writer suspension also stops acceleration");
 Reset(m);m.zoomValueCurrent=std::numeric_limits<float>::quiet_NaN();ZoomWheelHook(&m,&sender,120);C(seenRelative==120,"non-finite native passthrough");
 Reset(m);zoomStepDepth=3;ZoomInHook(&m,&sender);ZoomWheelHook(&m,&sender,120);C(calls[0]==1&&calls[2]==1&&seenBase==.125f&&seenRelative==120&&zoomStepDepth==3&&queries==0,"nested entry preserves existing depth and raw input");
 Reset(m);reenter=true;ZoomInHook(&m,&sender);C(calls[0]==1&&calls[1]==1&&Near(m.zoomValueCurrent,2)&&m.zoomValueBase==.125f&&zoomStepDepth==0,"native callback reentry cannot compound step");
 // All eight success/failure combinations and returned-null originals.
 for(unsigned mask=0;mask<8;++mask){Reset(m);zoomStepHooksReady=false;originalZoomIn=originalZoomOut=NULL;originalZoomWheel=NULL;successMask=mask;probeMap=&m;InstallZoomSteps();C(installIndex==3&&profiles==1&&zoomStepHooksReady==(mask==7)&&errors==(mask==7?0:1),"three registration aggregate readiness");C(seenRelative==((mask&4)?120:0)&&seenBase==((mask&7)?.125f:0)&&zoomStepDepth==0,"between registration callbacks remain unaccelerated");
  if(mask!=7){m.zoomValueCurrent=2;if(originalZoomIn){ZoomInHook(&m,&sender);C(seenBase==.125f,"partially installed In passes native");}if(originalZoomOut){ZoomOutHook(&m,&sender);C(seenBase==.125f,"partially installed Out passes native");}if(originalZoomWheel){ZoomWheelHook(&m,&sender,120);C(seenRelative==120,"partially installed Wheel passes native");}}}
 for(unsigned i=0;i<3;++i){Reset(m);zoomStepHooksReady=false;originalZoomIn=originalZoomOut=NULL;originalZoomWheel=NULL;nullMask=1<<i;InstallZoomSteps();C(!zoomStepHooksReady&&errors==1&&installIndex==3,"SUCCESS with missing trampoline refuses acceleration");}
 for(unsigned i=0;i<3;++i){Reset(m);zoomStepHooksReady=false;originalZoomIn=originalZoomOut=NULL;originalZoomWheel=NULL;successMask=7^(1<<i);returnedFailureMask=1<<i;InstallZoomSteps();C(!zoomStepHooksReady&&errors==1&&originalZoomIn&&originalZoomOut&&originalZoomWheel,"enable failure with valid returned trampoline refuses acceleration");}
 Reset(m);zoomStepHooksReady=false;profileOK=false;InstallZoomSteps();C(installIndex==0&&!zoomStepHooksReady&&profiles==1&&errors==1,"profile failure never registers hooks");
 // Production ApplyZoomLimit must bypass the hooked entry for clamp.
 Reset(m);zoomLease.Reset();m.zoomValueMax=2;ApplyZoomLimit(&m);C(m.zoomValueMax==4&&zoomLease.Owned(),"actual caller establishes cap lease");m.zoomValueCurrent=4;enabled=false;ApplyZoomLimit(&m);C(calls[0]==1&&fallbackCalls==0&&m.zoomValueCurrent==2&&m.zoomValueBase==.125f&&depthSeen==0&&!zoomLease.Owned(),"cap restore clamps via saved original exactly once without wrapper scope");
 Reset(m);zoomLease.Reset();m.zoomValueMax=2;zoomMultiplier=4;ApplyZoomLimit(&m);m.zoomValueCurrent=8;zoomMultiplier=2;ApplyZoomLimit(&m);C(calls[0]==1&&fallbackCalls==0&&m.zoomValueCurrent==4&&seenBase==.125f&&depthSeen==0&&zoomStepDepth==0,"enabled cap reduction also bypasses acceleration wrapper");
 std::printf("PASS actual zoom step caller %u checks\n",checks);return 0;}catch(...){return 1;}}
'''
program = (program.replace('__STEP__', str(args.case_step or P / 'GsmZoomStep.h'))
           .replace('__POLICY__', str(P / 'GsmZoomPolicy.h'))
           .replace('__WRAPPERS__', wrappers).replace('__INSTALL__', install).replace('__APPLY__', apply))
(folder / 'caller.cpp').write_text(program, encoding='utf-8')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{vc}/include;{sdk}/Include', LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
run = subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MT', '/O2',
                      str(folder / 'caller.cpp'), '/Fo'+str(folder / 'caller.obj'),
                      '/Fe'+str(folder / 'caller.exe')], env=env, capture_output=True, text=True)
(folder / 'build.log').write_text(run.stdout+run.stderr)
assert run.returncode == 0, run.stdout+run.stderr
run = subprocess.run([str(folder / 'caller.exe')], capture_output=True, text=True)
(folder / 'run.log').write_text(run.stdout+run.stderr)
print(run.stdout+run.stderr, end='')
assert run.returncode == 0
(folder / 'identity.json').write_text(json.dumps({
    'main': hashlib.sha256(source.encode()).hexdigest(),
    'step': hashlib.sha256((args.case_step or P / 'GsmZoomStep.h').read_bytes()).hexdigest(),
    'wrappers': hashlib.sha256(wrappers.encode()).hexdigest(),
    'install': hashlib.sha256(install.encode()).hexdigest(),
    'applyRaw': hashlib.sha256(apply_raw.encode()).hexdigest(),
    'applyCompiledSeam': hashlib.sha256(apply.encode()).hexdigest()}, indent=2)+'\n')
if not args.skip_mutants:
    cases = [
        ('no_readiness', 'return zoomStepHooksReady&&enabled', 'return enabled'),
        ('no_enabled', 'zoomStepHooksReady&&enabled&&', 'zoomStepHooksReady&&'),
        ('no_multiplier', '&&zoomMultiplier>1', ''),
        ('no_depth_guard', '&&!zoomStepDepth', ''),
        ('no_owned_guard', '&&zoomLease.Owned()', ''),
        ('no_wheel_transform', 'GsmZoom::WheelStep(relative,map->zoomValueBase,map->zoomValueCurrent)', 'relative'),
        ('double_in_call', '));originalZoomIn(map,sender);', '));originalZoomIn(map,sender);originalZoomIn(map,sender);'),
        ('ignore_hook_failure', 'a==KenshiLib::SUCCESS&&b==KenshiLib::SUCCESS&&c==KenshiLib::SUCCESS&&', ''),
        ('clamp_via_wrapper', 'if(originalZoomIn)originalZoomIn(map,NULL)', 'if(originalZoomIn)ZoomInHook(map,NULL)'),
    ]
    results = []
    for name, old, new in cases:
        assert source.count(old) == 1, (name, source.count(old))
        target = out / ('mutant-'+name)
        target.mkdir(exist_ok=True)
        mutant = target / 'Main-mutant.cpp'
        mutant.write_text(source.replace(old, new), encoding='utf-8')
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--skip-mutants',
                                 '--case-name', 'mutant-'+name, '--case-source', str(mutant)],
                                capture_output=True, text=True)
        (target / 'expected-red.log').write_text(result.stdout+result.stderr, encoding='utf-8')
        assert result.returncode != 0 and 'FAIL ' in result.stdout, (name, result.stdout+result.stderr)
        results.append({'name': name, 'result': result.stdout.strip()})
        print('Expected RED', name)
    (out / 'mutants.json').write_text(json.dumps(results, indent=2)+'\n')
    step_source = (P / 'GsmZoomStep.h').read_text()
    for name, old, new in [
        ('base_not_restored', '~StepScope(){slot=previous;}', '~StepScope(){}'),
        ('depth_not_restored', '~DepthScope(){--slot;}', '~DepthScope(){}'),
    ]:
        assert step_source.count(old) == 1, (name, step_source.count(old))
        target = out / ('mutant-'+name)
        target.mkdir(exist_ok=True)
        mutant = target / 'Step-mutant.h'
        mutant.write_text(step_source.replace(old, new))
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()), '--skip-mutants',
                                 '--case-name', 'mutant-'+name, '--case-step', str(mutant)],
                                capture_output=True, text=True)
        (target / 'expected-red.log').write_text(result.stdout+result.stderr)
        assert result.returncode != 0 and 'FAIL ' in result.stdout, (name, result.stdout+result.stderr)
        results.append({'name': name, 'result': result.stdout.strip()})
        print('Expected RED', name)
    (out / 'mutants.json').write_text(json.dumps(results, indent=2)+'\n')
    print('PASS', len(results), 'compiled actual zoom step caller mutants')
