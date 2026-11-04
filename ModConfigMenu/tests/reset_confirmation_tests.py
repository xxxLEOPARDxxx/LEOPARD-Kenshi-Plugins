"""Exercise extracted production reset entry/finish/apply and its identity controller."""
from pathlib import Path
import os,subprocess,sys
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'tests/generated/reset-confirmation';OUT.mkdir(parents=True,exist_ok=True)
source=(ROOT/'ModConfigMenu.cpp').read_text(encoding='utf-8-sig')
def body(sig):
 a=source.index(sig);b=source.index('{',a)+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[a:b]
modern=(ROOT/'ResetConfirmation.h').exists()
prefix=r"""
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <new>
static unsigned allocations=0;
void*operator new(size_t n){++allocations;void*p=malloc(n);if(!p)throw std::bad_alloc();return p;}
void*operator new[](size_t n){return ::operator new(n);}
void operator delete(void*p){free(p);}
void operator delete[](void*p){free(p);}
static unsigned checks=0,commits=0,setters=0,applies=0,creates=0,destroys=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);std::exit(1);}}
enum Kind {K_SECTION,K_ACTION,K_TEXT};
struct Setting {Kind kind;bool fn,fnHasDef;void*line;std::string def,label;Setting():kind(K_TEXT),fn(false),fnHasDef(false),line((void*)1),def("default") {}};
struct ModEntry {std::string id;std::vector<Setting*>settings;};
static ModEntry *g_current=NULL,*g_pendingShow=NULL;static bool g_hasPending=false;
static void Commit(ModEntry*,Setting*,const std::string&){++commits;}
static bool WriteFn(const std::string&,Setting*,const std::string&){++setters;return true;}
static void ErrorLog(const std::string&){}
static void ApplyMod(ModEntry*){++applies;}
struct DataPanelLine{};
"""
if modern:
 prefix+='#include "ResetConfirmation.h"\n'
 prefix+=r"""
static McmReset::Confirmation g_resetConfirmation;
static unsigned long long g_pageGeneration=1;
static bool live=true,createOk=true,modal=false;
namespace MyGUI {
struct Widget{};struct Window:Widget{};
struct TabControl {Widget*selected;Widget*getItemSelected(){return selected;}};
struct InputManager {static InputManager*getInstancePtr(){return NULL;}Widget*getKeyFocusWidget(){return NULL;}};
}
struct Ref {MyGUI::Widget*p;MyGUI::Widget*get(){return p;}};
static MyGUI::Window dialog;static MyGUI::Widget yes,cancel,tab,otherTab;
static Ref modalRef={&dialog},yesRef={&yes},cancelRef={&cancel};
static Ref *g_resetModalRef=&modalRef,*g_resetYesRef=&yesRef,*g_resetCancelRef=&cancelRef;
static MyGUI::Widget*g_tab=&tab;
namespace OIS{enum KeyCode{KC_ESCAPE=1,KC_RETURN=2,KC_NUMPADENTER=3,KC_BACK=4};}
struct InputHandler{};struct OptionsWindow{};
static OptionsWindow options;static OptionsWindow*g_window=&options;
static unsigned nativeKeys=0,nativeUps=0,hides=0,polls=0;
static void OrigKey(InputHandler*,OIS::KeyCode){++nativeKeys;}
static void OrigUp(InputHandler*,OIS::KeyCode){++nativeUps;}
static void OrigHide(OptionsWindow*){++hides;}
static void(*g_origHide)(OptionsWindow*)=OrigHide;
static void(*g_origKeyDown)(InputHandler*,OIS::KeyCode)=OrigKey;
static void(*g_origKeyUp)(InputHandler*,OIS::KeyCode)=OrigUp;
static unsigned g_resetBlockedKeys=0;
static int g_capturedKeyDown=-1,g_captureMod=0;static Setting*g_capture=NULL;struct DatapanelGUI{};static DatapanelGUI*g_panel=NULL;
struct MCM_Key{int key;unsigned mod;};
static bool CaptureActive(){return false;}static bool IsModifier(OIS::KeyCode){return false;}
static bool InsideOurTab(MyGUI::Widget*){return false;}
static void PlaceOwnButtons(){}static void FinishCapture(const std::string&){}
static unsigned CaptureModifiers(){return 0;}static std::string FormatKey(MCM_Key){return "";}
static std::string ModifierName(int){return "";}static void Poll(bool){++polls;}static void DropFocusInside(){}
static bool g_debug=false;static void DebugLog(const std::string&){}static void ForgetOwnButtons(){}
static std::vector<int>g_actionLines;static DataPanelLine*g_resetLine=NULL;
struct PanelBindings{bool destroying;PanelBindings():destroying(false){}};
static std::map<DatapanelGUI*,PanelBindings>g_panelBindings;static unsigned dtors=0,releases=0;
static void OriginalDtor(DatapanelGUI*){C(!g_resetConfirmation.IsPending()&&!modal,"native destructor sees reset already canceled");++dtors;}
static void(*g_origDatapanelDtor)(DatapanelGUI*)=OriginalDtor;
static void ReleasePanelBindings(DatapanelGUI*p){g_panelBindings.erase(p);++releases;}


static bool ResetPageLive(){return live&&g_current!=NULL;}
static bool ResetModalAlive(){return modal;}
static bool CreateResetConfirmation(){++creates;modal=createOk;return createOk;}
static unsigned destroyMutation=0;static ModEntry*destroyTarget=NULL;
static void DestroyResetModal(){if(modal)++destroys;modal=false;if(destroyMutation==1)g_current=destroyTarget;if(destroyMutation==2)++g_pageGeneration;if(destroyMutation==3)live=false;if(destroyMutation==4)g_hasPending=true;destroyMutation=0;}
static void CancelResetConfirmation(){g_resetConfirmation.Cancel();DestroyResetModal();}
"""
 code=body('void ResetModToDefaults(')+'\n'+body('void OnResetLine(DataPanelLine*)\n')+'\n'+body('void FinishResetConfirmation()\n')+'\n'+body('void InvalidateResetConfirmation()\n')+'\n'+body('void OnResetConfirm(')+'\n'+body('void OnResetCancel(')+'\n'+body('void OnResetWindowButton(')+'\n'+body('void OnMcmTabSelected(')+'\n'+body('void Hide_hook(')+'\n'+body('void DatapanelDtor_hook(')+'\n'+body('unsigned ResetCancelKeyBit(')+'\n'+body('void KeyDown_hook(')+'\n'+body('void KeyUp_hook(')
else:code=body('void OnResetLine(DataPanelLine*)\n')
suffix=r"""
int main(){
 ModEntry a,b;a.id="first";b.id="second";Setting ini,fn,noDef,section,action;fn.fn=fn.fnHasDef=true;noDef.fn=true;section.kind=K_SECTION;action.kind=K_ACTION;a.settings.push_back(&ini);a.settings.push_back(&fn);a.settings.push_back(&noDef);a.settings.push_back(&section);a.settings.push_back(&action);b.settings=a.settings;g_current=&a;
 OnResetLine(NULL);C(commits==0&&setters==0&&applies==0,"reset click must wait for explicit confirmation");
"""
if modern:suffix+=r"""
 C(g_resetConfirmation.IsPending()&&creates==1,"one pending modal");OnResetLine(NULL);C(creates==1,"repeat click cannot duplicate modal");
 g_resetConfirmation.Decide(false);FinishResetConfirmation();C(commits==0&&setters==0&&applies==0&&!modal,"cancel makes no setter, INI or apply call");
 OnResetLine(NULL);g_resetConfirmation.Decide(true);C(commits==0&&setters==0&&applies==0,"confirm callback only queues decision");FinishResetConfirmation();C(commits==1&&setters==1&&applies==1&&!modal,"confirmed original id resets INI and v5 exactly once");C(ini.line==NULL&&fn.line==NULL,"old bound controls cannot restore pre-reset values");FinishResetConfirmation();C(applies==1,"decision consumed before reset callback");
 g_hasPending=false;g_pendingShow=NULL;
 g_capture=&fn;g_captureMod=1;OnResetLine(NULL);C(g_capture==NULL&&g_captureMod==0,"opening confirmation cancels hotkey capture without writing");unsigned ac=allocations;for(int i=0;i<100;++i)FinishResetConfirmation();C(allocations==ac,"waiting modal allocates no per-frame objects");
 OnResetCancel(&yes);C(!g_resetConfirmation.IsDecided(),"wrong cancel sender ignored");OnResetCancel(&cancel);FinishResetConfirmation();C(applies==1,"actual cancel callback cannot reset");
 OnResetLine(NULL);OnResetWindowButton(&dialog,"close");FinishResetConfirmation();C(applies==1,"actual window X callback cancels");
 for(unsigned key=1;key<=3;++key){OnResetLine(NULL);KeyDown_hook(NULL,(OIS::KeyCode)key);C(g_resetConfirmation.IsDecided()&&nativeKeys==0&&polls==0,"Escape and both Enter keys consume input without Poll or native activation");FinishResetConfirmation();KeyDown_hook(NULL,(OIS::KeyCode)key);KeyUp_hook(NULL,(OIS::KeyCode)key);C(applies==1&&nativeKeys==0&&nativeUps==0,"cancel key repeat and matching release suppressed after modal closes");}
 OnResetLine(NULL);KeyDown_hook(NULL,OIS::KC_RETURN);KeyDown_hook(NULL,OIS::KC_ESCAPE);FinishResetConfirmation();KeyUp_hook(NULL,OIS::KC_RETURN);KeyUp_hook(NULL,OIS::KC_ESCAPE);C(nativeUps==0&&g_resetBlockedKeys==0,"overlapping cancel keys suppress each matching key-up");
 OnResetLine(NULL);MyGUI::TabControl tabs;tabs.selected=&otherTab;OnMcmTabSelected(&tabs,1);C(!g_resetConfirmation.IsPending()&&!modal,"tab change cancels synchronously without update");
 OnResetLine(NULL);Hide_hook(&options);C(!g_resetConfirmation.IsPending()&&!modal&&hides==1&&applies==1,"native mapped hide cancels before options stop updating");
 DatapanelGUI panel;g_panel=&panel;OnResetLine(NULL);DatapanelDtor_hook(&panel);C(!g_resetConfirmation.IsPending()&&!modal&&g_panel==NULL&&dtors==1&&releases==1,"external panel destruction cancels without waiting for update");
 OnResetLine(NULL);OnResetConfirm(&cancel);C(!g_resetConfirmation.IsDecided(),"wrong confirm sender ignored");OnResetConfirm(&yes);FinishResetConfirmation();C(applies==2,"actual reset button callback confirms original page");g_hasPending=false;const unsigned baselineApplies=applies;
 for(unsigned reason=0;reason<5;++reason){g_current=&a;live=true;OnResetLine(NULL);g_resetConfirmation.Decide(true);if(reason==0)g_current=&b;if(reason==1)++g_pageGeneration;if(reason==2)live=false;if(reason==3)modal=false;if(reason==4)g_hasPending=true;FinishResetConfirmation();C(applies==baselineApplies&&!g_resetConfirmation.IsPending(),"selection/generation/closed panel/deleted modal/queued page invalidates confirmation");g_hasPending=false;}
 for(unsigned mutation=1;mutation<=4;++mutation){g_current=&a;live=true;g_hasPending=false;OnResetLine(NULL);g_resetConfirmation.Decide(true);destroyTarget=&b;destroyMutation=mutation;FinishResetConfirmation();C(applies==baselineApplies&&!g_resetConfirmation.IsPending(),"modal destruction cannot retarget reset through synchronous UI delegates");}g_hasPending=false;
 live=true;g_current=&a;createOk=false;OnResetLine(NULL);C(!g_resetConfirmation.IsPending()&&!modal,"failed modal creation cannot leave a reset armed");createOk=true;
 McmReset::Confirmation c;C(!c.Request("",1),"empty identity refused");C(c.Request("first",7),"request snapshot");C(!c.Request("second",8),"pending identity cannot be retargeted");c.Decide(false);c.Decide(true);C(!c.Take("first",7),"cancel cannot be upgraded by later event");C(c.Request("first",7),"new request after cancel");c.Decide(true);C(!c.Take("second",7),"wrong target refused and consumed");C(!c.IsPending(),"mismatch consumes request");c.Request("first",7);c.Decide(true);C(!c.Take("first",8),"same id rebuilt generation refused");c.Request("first",7);c.Cancel();C(!c.Take("first",7),"panel cancellation cannot be confirmed later");
"""
suffix+='std::printf("PASS %u reset confirmation checks\\n",checks);return 0;}\n'
cpp=OUT/'reset.cpp';cpp.write_text(prefix+code+suffix,encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{ROOT}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ['/Od','/O2']:
 exe=OUT/('reset-'+opt[1:]+'.exe');p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(cpp),'/Fo'+str(OUT)+'/', '/Fe'+str(exe)],cwd=OUT,env=env,capture_output=True,text=True)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);print(opt,p.stdout,end='');(OUT/('run-'+opt[1:]+'.log')).write_text(p.stdout+p.stderr)
 if '--expect-red' in sys.argv:assert p.returncode==1 and 'wait for explicit confirmation' in p.stdout
 else:assert p.returncode==0,(p.stdout,p.stderr)
