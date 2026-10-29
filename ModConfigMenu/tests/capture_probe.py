"""Compile current production MCM key/mouse capture against UI boundaries.
Checks opt-in, selection-click ordering, repeats/releases and legacy behavior.
No game process or installed files are accessed. Outputs are ignored.
"""
from pathlib import Path
import os,subprocess,sys
test_dir=Path(__file__).resolve().parent
module_dir=test_dir.parent
repository_dir=module_dir.parent
root=test_dir/'generated'
root.mkdir(parents=True,exist_ok=True)
source_path=module_dir/'ModConfigMenu.cpp'
source=source_path.read_text(encoding='utf-8-sig')
def function(signature):
    start=source.index(signature)
    while source.find(';',start)<source.find('{',start):start=source.index(signature,start+len(signature))
    opening=source.index('{',start);end=opening+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
table=source[source.index('    struct KeyName {'):source.index('    std::string FormatKey(')]
code=r'''
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ois/OISKeyboard.h>
#include <ModConfigMenu.h>
bool ctrl=false,shift=false,alt=false;
SHORT ProbeAsync(int vk){return (vk==VK_CONTROL&&ctrl)||(vk==VK_SHIFT&&shift)||(vk==VK_MENU&&alt)?SHORT(0x8000):0;}
#define GetAsyncKeyState ProbeAsync
namespace MyGUI {
struct Widget{bool visible;Widget():visible(true){}bool getInheritedVisible(){return visible;}};
struct MouseButton {int value;MouseButton(int n):value(n){}int getValue(){return value;}};
struct InputManager {Widget*focus;InputManager():focus(NULL){}static InputManager*getInstancePtr();Widget*getKeyFocusWidget(){return focus;}};
InputManager input;InputManager*InputManager::getInstancePtr(){return &input;}
}
struct WidgetRef{MyGUI::Widget*w;MyGUI::Widget*get(){return w;}};
MyGUI::Widget host;WidgetRef rootRef={&host};WidgetRef*g_panelRootRef=&rootRef;
struct Setting{void*line;unsigned flags;Setting(unsigned f=MCM_ALLOW_MOUSE):flags(f){line=this;}};
struct ModEntry{};ModEntry mod;ModEntry*g_current=&mod;
Setting*g_capture=NULL;int g_captureMod=0;int writes=0,applies=0,paint=0;std::string captured;
unsigned g_capturedMouseDown=0;int g_capturedKeyDown=-1;bool g_mouseCaptureReady=true;
void Commit(ModEntry*,Setting*,const std::string&s){++writes;captured=s;}
void ApplyMod(ModEntry*){++applies;}void PlaceOwnButtons(){++paint;}
struct InputHandler{};
void*g_panel=NULL;
bool InsideOurTab(MyGUI::Widget*){return true;}void Poll(bool){}void DropFocusInside(){g_capture=NULL;}
int originalPress=0,originalRelease=0,originalKeyDown=0,originalKeyUp=0;
Setting*armOnRelease=NULL;
void BeginCapture(Setting*);
bool NativePress(MyGUI::InputManager*,int,int,MyGUI::MouseButton){++originalPress;return false;}
bool NativeRelease(MyGUI::InputManager*,int,int,MyGUI::MouseButton){++originalRelease;if(armOnRelease){BeginCapture(armOnRelease);armOnRelease=NULL;}return false;}
void NativeKeyDown(InputHandler*,OIS::KeyCode){++originalKeyDown;}void NativeKeyUp(InputHandler*,OIS::KeyCode){++originalKeyUp;}
bool(*g_origMousePress)(MyGUI::InputManager*,int,int,MyGUI::MouseButton)=NativePress;
bool(*g_origMouseRelease)(MyGUI::InputManager*,int,int,MyGUI::MouseButton)=NativeRelease;
void(*g_origKeyDown)(InputHandler*,OIS::KeyCode)=NativeKeyDown;
void(*g_origKeyUp)(InputHandler*,OIS::KeyCode)=NativeKeyUp;
int failures=0;void check(bool ok,const char*s){printf("%s %s\n",ok?"PASS":"FAIL",s);if(!ok)++failures;}
'''
code+=table+'\n'
for name in ['    std::string FormatKey(', '    bool ParseKey(', '    void FinishCapture(',
             '    bool IsModifier(', '    const char* ModifierName(', '    bool CaptureActive(',
             '    unsigned CaptureModifiers(', '    void BeginCapture(', '    bool MousePress_hook(',
             '    bool MouseRelease_hook(', '    void KeyDown_hook(', '    void KeyUp_hook(']:
    code+=function(name)+'\n'
code+=r'''
int main(){
 Setting binding,legacy(0);InputHandler handler;
 MCM_Key parsed;check(ParseKey("CTRL+MMB",&parsed)&&parsed.keycode==MCM_KEY_MOUSE_MIDDLE&&parsed.modifiers==MCM_KEY_CTRL,"common parser represents CTRL+mouse outside OIS range");
 check(FormatKey(parsed)=="CTRL+MMB","common formatter round-trips CTRL+MMB");
 check(ParseKey("CTRL+R",&parsed)&&FormatKey(parsed)=="CTRL+R","common keyboard format remains compatible");
 MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(0));armOnRelease=&binding;MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(0));
 check(g_capture==&binding&&writes==0&&originalPress==1&&originalRelease==1,"selecting click arms capture on release without binding itself");
 MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));
 check(captured=="MMB"&&writes==1&&applies==1&&g_capture==NULL,"next MMB press assigns the one binding exactly once");
 MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));
 check(writes==1&&originalPress==1,"duplicate mouse press before release cannot leak to native UI");
 MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));check(originalRelease==1&&g_capturedMouseDown==0,"captured mouse release is consumed with its press");
 MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));
 check(originalPress==2&&originalRelease==2,"ordinary mouse events resume after captured release");
 BeginCapture(&binding);ctrl=true;MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(3));MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(3));
 check(captured=="CTRL+X1"&&writes==2,"mouse capture preserves CTRL modifier");ctrl=false;
 BeginCapture(&legacy);int before=writes;MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));
 check(g_capture==&legacy&&writes==before,"legacy callback stays keyboard-only and armed after mouse click");
 armOnRelease=&legacy;MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(0));MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(0));
 check(g_capture==NULL&&writes==before,"legacy click on its capture button retains click-cancel behavior");BeginCapture(&legacy);
 KeyDown_hook(&handler,OIS::KC_BACK);KeyUp_hook(&handler,OIS::KC_BACK);check(captured=="NONE"&&g_capture==NULL,"Backspace clears a keyboard-only binding");
 BeginCapture(&binding);ctrl=true;KeyDown_hook(&handler,OIS::KC_LCONTROL);KeyDown_hook(&handler,OIS::KC_R);before=writes;
 KeyDown_hook(&handler,OIS::KC_R);check(captured=="CTRL+R"&&writes==before&&originalKeyDown==0,"CTRL+key captures once and consumes keyboard repeat");
 KeyUp_hook(&handler,OIS::KC_R);KeyUp_hook(&handler,OIS::KC_LCONTROL);ctrl=false;
 KeyDown_hook(&handler,OIS::KC_R);KeyUp_hook(&handler,OIS::KC_R);check(originalKeyDown==1,"keyboard original resumes after captured key is released");
 BeginCapture(&binding);before=writes;KeyDown_hook(&handler,OIS::KC_ESCAPE);KeyUp_hook(&handler,OIS::KC_ESCAPE);check(writes==before&&g_capture==NULL,"Escape cancels without assigning or closing options");
 BeginCapture(&binding);host.visible=false;MousePress_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));MouseRelease_hook(&MyGUI::input,0,0,MyGUI::MouseButton(2));
 check(writes==before,"hidden capture host does not assign input");host.visible=true;g_capture=NULL;
 before=originalKeyDown;KeyDown_hook(&handler,OIS::KC_R);check(originalKeyDown==before+1,"without capture keyboard text input passes through");
 return failures?1:0;
}
'''
cpp=root/'CaptureProbe.cpp';cpp.write_text(code,encoding='utf-8')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['INCLUDE']=';'.join([str(vc/'include'),str(sdk/'Include'),str(repository_dir/'shared'),str(repository_dir.parent/'KenshiLib-src'/'Include')]);env['LIB']=str(vc/'lib'/'amd64')+';'+str(sdk/'Lib'/'x64');env['PATH']=str(vc/'bin'/'amd64')+';'+env['PATH']
exe=root/'CaptureProbe.exe';result=subprocess.run([str(vc/'bin'/'amd64'/'cl.exe'),'/nologo','/EHsc','/MD','/O2','/W1',str(cpp),'/Fo'+str(root/'CaptureProbe.obj'),'/Fe'+str(exe)],env=env,capture_output=True,text=True);print(result.stdout,result.stderr)
if result.returncode:sys.exit(result.returncode)
result=subprocess.run([str(exe)],env=env,capture_output=True,text=True);print(result.stdout,result.stderr);sys.exit(result.returncode)
