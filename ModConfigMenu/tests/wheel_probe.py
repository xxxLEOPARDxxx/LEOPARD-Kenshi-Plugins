"""Compile current production wheel helpers against native input boundaries.
The target control's native wheel would modify its value before delegates;
MCM must consume the input and route exactly one native page scroll instead.
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
production=source_path.read_text(encoding='utf-8-sig')
def function(signature):
    start=production.index(signature)
    while production.find(';',start)<production.find('{',start):
        start=production.index(signature,start+len(signature))
    opening=production.index('{',start);end=opening+1;depth=1
    while depth:
        depth+=(production[end]=='{')-(production[end]=='}');end+=1
    return production[start:end]
stubs=r'''
#include <cstdio>
#include <algorithm>
namespace MyGUI {
struct IntPoint {int left,top;IntPoint(int a=0,int b=0):left(a),top(b){}};
struct FirstBase {virtual ~FirstBase(){}void*padding;};
struct WidgetInput {virtual ~WidgetInput(){}int nativeValue;WidgetInput():nativeValue(7){}};
struct Widget:FirstBase,WidgetInput {Widget*parent;bool visible;int nativeCalls,events;bool setting;
 Widget(Widget*p=NULL,bool s=false):parent(p),visible(true),nativeCalls(0),events(0),setting(s){}
 Widget*getParent(){return parent;}bool getInheritedVisible(){for(Widget*p=this;p;p=p->parent)if(!p->visible)return false;return true;}
 template<class T>T*castType(bool=true){return dynamic_cast<T*>(this);}
};
struct ScrollView:Widget {IntPoint offset;float thumb;ScrollView(Widget*p):Widget(p),thumb(0){}
 IntPoint getViewOffset(){return offset;}void setViewOffset(IntPoint p){offset=p;}
};
}
struct WidgetRef {MyGUI::Widget*w;WidgetRef():w(NULL){}MyGUI::Widget*get(){return w;}};
struct DatapanelGUI {MyGUI::Widget*scrollWin;MyGUI::ScrollView*scrollView;int routed;
 MyGUI::Widget*relay;void(*relayWheel)(MyGUI::WidgetInput*,int);int depth;
 DatapanelGUI():scrollWin(NULL),scrollView(NULL),routed(0),relay(NULL),relayWheel(NULL),depth(0){}
 void notifyMouseWheel(MyGUI::Widget*,int rel){++routed;if(!rel)return;if(relay&&depth<4){++depth;relayWheel(static_cast<MyGUI::WidgetInput*>(relay),rel);--depth;}int p=scrollView->offset.top+(rel>0?40:-40);scrollView->offset.top=std::max(-400,std::min(0,p));scrollView->thumb=float(-scrollView->offset.top)/400;}
};
WidgetRef rootRef;WidgetRef*g_panelRootRef=&rootRef;DatapanelGUI*g_panel=NULL;unsigned long g_scrollUntil=0;
__declspec(thread) bool g_forwardingPanelWheel=false;
void NativeDispatch(MyGUI::WidgetInput*self,int rel){MyGUI::Widget*w=static_cast<MyGUI::Widget*>(self);++w->nativeCalls;if(w->setting)w->nativeValue+=rel>0?1:-1;++w->events;}
void(*g_origWheelInput)(MyGUI::WidgetInput*,int)=NativeDispatch;
int failures=0;
void check(bool ok,const char*name){printf("%s %s\n",ok?"PASS":"FAIL",name);if(!ok)++failures;}
'''
helpers=function('    MyGUI::ScrollView* PanelScroll(')+'\n'
helpers+=function('    bool RoutePanelWheel(')+'\n'+function('    void WheelInput_hook(')+'\n'
helpers+=function('    void OnPanelWheel(MyGUI::Widget* sender,')
tests=r'''
int main(){
 MyGUI::Widget root,customRoot;MyGUI::ScrollView view(&root);DatapanelGUI panel;panel.scrollView=&view;g_panel=&panel;rootRef.w=&root;
 const char*types[]={"checkbox","slider","numeric editor","text editor","label","action button","key binding","palette cell","choice button"};
 for(int i=0;i<9;++i){MyGUI::Widget control(&view,i==1||i==2||i==3||i==8);int before=panel.routed;
  WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&control),-120);
  bool ok=panel.routed==before+1&&control.nativeCalls==0&&control.events==0&&control.nativeValue==7;
  printf("%s hover %s scrolls page exactly once without editing value\n",ok?"PASS":"FAIL",types[i]);if(!ok)++failures;
 }
 check(view.offset.top==-360&&view.thumb==.9f,"page scrollbar thumb follows all routed steps");
 g_scrollUntil=999;MyGUI::Widget cell(&view);WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&cell),120);
 check(g_scrollUntil==0,"manual wheel cancels delayed scroll restoration");
 int before=panel.routed;WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&cell),0);
 check(panel.routed==before&&cell.nativeCalls==0,"zero wheel does not move page or dispatch to control");
 MyGUI::Widget outside(NULL,true);WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&outside),-120);
 check(outside.nativeCalls==1&&outside.events==1&&outside.nativeValue==6,"unrelated widgets keep their original input behavior");
 MyGUI::Widget source(&customRoot,true);before=panel.routed;WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&source),-120);
 check(panel.routed==before&&source.nativeCalls==1,"MultiXPMining custom sibling area keeps independent wheel dispatch");
 MyGUI::ScrollView nested(&view);MyGUI::Widget nestedControl(&nested,true);before=panel.routed;WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&nestedControl),-120);
 check(panel.routed==before&&nestedControl.nativeCalls==1,"nested independent scroll areas retain native dispatch");
 root.visible=false;MyGUI::Widget hiddenControl(&view,true);WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&hiddenControl),-120);
 check(hiddenControl.nativeCalls==1,"hidden native settings host does not intercept input");root.visible=true;
 rootRef.w=NULL;MyGUI::Widget oldControl(&view,true);WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&oldControl),-120);
 check(oldControl.nativeCalls==1,"deleted host invalidation leaves no retained widget route");rootRef.w=&root;
 check(reinterpret_cast<void*>(static_cast<MyGUI::WidgetInput*>(&cell))!=reinterpret_cast<void*>(&cell),"fixture exercises adjusted WidgetInput base pointer");
 before=panel.routed;for(int i=0;i<10000;++i)WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&cell),-120);
 check(panel.routed==before+10000&&view.offset.top==-400&&view.thumb==1,"repeated events stay single-step and clamp at page limit");
 MyGUI::Widget pageBar(&view,true);panel.relay=&pageBar;panel.relayWheel=WheelInput_hook;before=panel.routed;
 WheelInput_hook(static_cast<MyGUI::WidgetInput*>(&cell),-120);
 check(panel.routed==before+1&&pageBar.nativeCalls==1&&!g_forwardingPanelWheel,"native scrollbar redispatch passes through once without recursive page routing");
 return failures?1:0;
}
'''
cpp=root/'WheelProbe.cpp';cpp.write_text(stubs+helpers+tests,encoding='utf-8')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include');env['LIB']=str(vc/'lib'/'amd64')+';'+str(sdk/'Lib'/'x64');env['PATH']=str(vc/'bin'/'amd64')+';'+env['PATH']
exe=root/'WheelProbe.exe';result=subprocess.run([str(vc/'bin'/'amd64'/'cl.exe'),'/nologo','/EHsc','/MD','/O2','/W1',str(cpp),'/Fo'+str(root/'WheelProbe.obj'),'/Fe'+str(exe)],env=env,capture_output=True,text=True)
print(result.stdout,result.stderr)
if result.returncode:sys.exit(result.returncode)
result=subprocess.run([str(exe)],env=env,capture_output=True,text=True);print(result.stdout,result.stderr);sys.exit(result.returncode)
