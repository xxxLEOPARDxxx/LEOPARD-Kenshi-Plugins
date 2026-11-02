"""Compile/run actual main diagnostic guards against owned and inaccessible pages."""
from pathlib import Path
import os
import subprocess

base = Path(__file__).resolve().parents[1]
out = base / 'build/diagnostic-guard-review'
out.mkdir(exist_ok=True)
main = (base / 'GlobalSatelliteMap.cpp').read_text()
guards = main[main.index('bool DiagnosticScene('):main.index('void LocalDiagnostic(')]
local = main[main.index('void LocalDiagnostic('):main.index('void OriginDiagnostic(')]
origin = (base / 'atlas-lights/OriginProof.h').read_text()
shared = origin[origin.index('inline void SharedImpl('):origin.index('class Reader {')]
assert origin.count('if(!Shared(') == 2
assert 'origin proof shared scalar getter failed' in origin
assert 'origin proof shared scalar reread failed' in origin
source = r'''
#include <Windows.h>
#include <cstdio>
#include <sstream>
#include <string>
#include <stdexcept>
struct Render {const void*scene;const void*camera;};
struct All {Render*render;}; static All*au=NULL;
static unsigned checks=0,townReads=0,reads=0,logs=0,errors=0;
namespace NativeLocalLights {
inline int NativeFault(DWORD c){return c==EXCEPTION_ACCESS_VIOLATION||c==EXCEPTION_IN_PAGE_ERROR||c==EXCEPTION_DATATYPE_MISALIGNMENT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}
struct LightValue {float worldPosition[3],range,effectivePower;};
struct Information {unsigned registered,hidden,daySuppressed;float rendererGain;struct{float time;}clock;float SourceSceneOrigin[3];};
struct NativeSnapshot {
    bool valid;Information info;LightValue value;
    NativeSnapshot():valid(false){ZeroMemory(&info,sizeof(info));ZeroMemory(&value,sizeof(value));}
    void Reset(){valid=false;}
    HRESULT Read(bool,LONG,const void*scene,bool verified,const volatile LONG*,DWORD){++reads;valid=scene&&verified;return valid?S_OK:E_FAIL;}
    bool Valid(LONG)const{return valid;}
    const char*LastError()const{return "injected native refusal";}
    const Information&Info()const{return info;}
    unsigned Count()const{return valid?1:0;}
    const LightValue*Light(unsigned)const{return &value;}
};
}
#include "../../GsmLocalDiagnostic.h"
struct Town;
struct TownBase {Town*value;Town*isTown(){++townReads;return value;}};
struct Town:TownBase {};
struct hand {TownBase*const*source;TownBase*getTown()const{++townReads;return *source;}};
namespace GSM {static volatile LONG generation=1;}
static bool debug=false,ready=true,localDiagnosticLogged=false;
static DWORD localDiagnosticAt=0;static LONG observedGeneration=1;
static struct {float referenceSide[4],worldOffset[4];}lighting={};
static GsmLocalDiagnostic::Bridge<NativeLocalLights::NativeSnapshot>localDiagnostic;
static void ErrorLog(const std::string&){++errors;}
static void Diagnostic(const std::string&){++logs;}
'''
source = source.replace('#include "../../GsmLocalDiagnostic.h"', '#include "' + (base / 'GsmLocalDiagnostic.h').as_posix() + '"')
source += guards + local
source += r'''
static int sharedMode=0;static const volatile float*sharedBad=NULL;
namespace Ogre {struct Vector4{float x,y,z,w;};}
namespace GsmNative {
Ogre::Vector4 Shared(const char*,const char*,unsigned){
    if(sharedMode==2)throw std::runtime_error("injected native C++ exception");
    Ogre::Vector4 v={1,2,3,0};if(sharedMode==1)v.x=*sharedBad;return v;
}}
namespace NativeOriginProof {
'''+shared+'}\n'
source += r'''
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
    SYSTEM_INFO si;GetSystemInfo(&si);const SIZE_T page=si.dwPageSize;
    BYTE*p=static_cast<BYTE*>(VirtualAlloc(NULL,page*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    Check(p!=NULL,"owned pages allocated");DWORD old=0;
    Check(VirtualProtect(p+page,page,PAGE_NOACCESS,&old)!=FALSE,"inaccessible second page installed");
    const void*scene=reinterpret_cast<const void*>(9),*camera=scene;
    Check(!DiagnosticScene(scene,camera)&&!scene&&!camera,"null owner retires both outputs");
    au=reinterpret_cast<All*>(p+page);scene=camera=reinterpret_cast<void*>(9);
    Check(!DiagnosticScene(scene,camera)&&!scene&&!camera,"inaccessible owner caught");
    All owner={NULL};au=&owner;scene=camera=reinterpret_cast<void*>(9);
    Check(!DiagnosticScene(scene,camera)&&!scene&&!camera,"null render caught");
    owner.render=reinterpret_cast<Render*>(p+page);scene=camera=reinterpret_cast<void*>(9);
    Check(!DiagnosticScene(scene,camera)&&!scene&&!camera,"inaccessible render caught");
    owner.render=reinterpret_cast<Render*>(p+page-sizeof(void*));owner.render->scene=reinterpret_cast<void*>(11);
    Check(!DiagnosticScene(scene,camera)&&!scene&&!camera,"partial scene then camera fault retires both outputs");
    Render render={reinterpret_cast<void*>(11),reinterpret_cast<void*>(12)};owner.render=&render;
    Check(DiagnosticScene(scene,camera)&&scene==render.scene&&camera==render.camera,"owned scene/camera copied unchanged");
    au=reinterpret_cast<All*>(p+page);debug=false;LocalDiagnostic(1);
    Check(reads==0&&logs==0&&errors==0,"OFF actual caller touches no inaccessible scene or reader");
    au=&owner;debug=true;LocalDiagnostic(2);
    Check(reads==1&&logs==1&&errors==0,"actual caller reaches reader with owned scene");
    au=reinterpret_cast<All*>(p+page);LocalDiagnostic(3);
    Check(errors==1,"guard refusal reported through reader without escaping");LocalDiagnostic(4);
    Check(errors==1,"same failed bridge latched");
    TownBase*none=NULL;hand empty={&none};Check(DiagnosticTown(empty)==NULL,"absent town handled");
    Town town;town.value=&town;TownBase*base=&town;hand good={&base};
    Check(DiagnosticTown(good)==&town,"owned town dispatch copied unchanged");
    hand invalid={reinterpret_cast<TownBase*const*>(p+page)};
    Check(DiagnosticTown(invalid)==NULL,"inaccessible hand dispatch caught");
    TownBase*bad=reinterpret_cast<TownBase*>(p+page);hand badTown={&bad};
    Check(DiagnosticTown(badTown)==NULL,"inaccessible town dispatch caught");
    float scalars[3]={0};sharedBad=reinterpret_cast<const volatile float*>(p+page);
    Check(NativeOriginProof::Shared("set","value",scalars)&&scalars[0]==1&&scalars[1]==2&&scalars[2]==3,"actual origin Shared copies bounded owned scalars");
    sharedMode=1;Check(!NativeOriginProof::Shared("set","value",scalars),"actual origin Shared contains native access fault");
    sharedMode=2;bool cpp=false;try{NativeOriginProof::Shared("set","value",scalars);}catch(const std::exception&){cpp=true;}
    Check(cpp,"actual origin Shared preserves C++ exception for Reader catch");
    Check(VirtualFree(p,0,MEM_RELEASE)!=FALSE,"owned pages released");
    std::printf("actual main diagnostic guards: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
'''
cpp = out / 'guards.cpp'
cpp.write_text(source)
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include')
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', '/W3', '/Fe' + str(out / 'guards.exe'), '/Fo' + str(out / 'guards.obj'), str(cpp)], env=env, check=True)
subprocess.run([str(out / 'guards.exe')], check=True)
