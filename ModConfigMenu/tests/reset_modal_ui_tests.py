"""Compile actual modal creation/destruction at the MyGUI boundary; inspect both real skin templates."""
from pathlib import Path
import os,subprocess,xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'tests/generated/reset-modal';OUT.mkdir(parents=True,exist_ok=True)
s=(ROOT/'ModConfigMenu.cpp').read_text(encoding='utf-8-sig')
def body(sig):
 a=s.index(sig);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
skins=[]
for name,path in [('vanilla','data/gui/templates/kenshi_templates.xml'),('Dark UI','mods/Dark UI Advanced/gui/templates/kenshi_templates.xml')]:
 res=next(x for x in ET.parse(Path('D:/SteamLibrary/steamapps/common/Kenshi')/path).getroot().findall('Resource')if x.get('name')=='Kenshi_WindowCX')
 root=res.find('Widget');outer=list(map(int,root.get('position').split()));client=next(x for x in root.iter('Widget')if x.get('name')=='Client');parent=next(x for x in root.iter('Widget')if client in list(x));pcoord=list(map(int,parent.get('position').split()));ccoord=list(map(int,client.get('position').split()));assert parent.get('align')=='Stretch'and client.get('align')=='Stretch'
 skins.append((name,outer[2]-ccoord[2],outer[3]-ccoord[3]))
prefix=r"""
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "ResetConfirmation.h"
static unsigned checks=0,liveWidgets=0,modalAdds=0,modalRemoves=0,errors=0;
static int insetW=6,insetH=45;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);std::exit(1);}}
namespace MyGUI{
struct IntSize{int width,height;IntSize(int w=1280,int h=720):width(w),height(h){}};
struct IntCoord{int left,top,width,height;IntCoord(int x=0,int y=0,int w=0,int h=0):left(x),top(y),width(w),height(h){}};
namespace Align{enum{Default,Center};}
struct Colour{float r,g,b;Colour(float R=0,float G=0,float B=0):r(R),g(G),b(B){}};
struct Event{template<class T>void operator+=(T){}};template<class T>int newDelegate(T){return 0;}
struct ILayer{std::string getName(){return "Window";}};
struct Widget{std::vector<Widget*>children;Widget*parent;IntCoord c;bool key;std::string caption,skin;Widget():parent(NULL),key(true){++liveWidgets;}virtual~Widget();
 ILayer*getLayer(){static ILayer l;return &l;}IntCoord getClientCoord(){return IntCoord(3,41,c.width-insetW,c.height-insetH);}
 template<class T>T*createWidget(const char*sk,IntCoord pos,int){T*w=new T;w->c=pos;w->skin=sk;w->parent=this;children.push_back(w);return w;}
 void setCaption(const std::string&v){caption=v;}void setNeedKeyFocus(bool b){key=b;}void setNeedMouseFocus(bool){}
};
struct Window:Widget{Event eventWindowButtonPressed;};
struct Button:Widget{Event eventMouseButtonClick;};
struct EditBox:Widget{Colour colour;bool multiline,wrap,stat;void setEditMultiLine(bool b){multiline=b;}void setEditWordWrap(bool b){wrap=b;}void setEditStatic(bool b){stat=b;}void setTextAlign(int){}void setTextColour(Colour c){colour=c;}};
struct Gui{static Gui*getInstancePtr(){return &getInstance();}static Gui&getInstance(){static Gui g;return g;}template<class T>T*createWidget(const char*sk,IntCoord pos,int,const std::string&,const char*){T*w=new T;w->c=pos;w->skin=sk;return w;}void destroyWidget(Widget*w){delete w;}};
struct InputManager{Widget*modal,*focus;InputManager():modal(NULL),focus(NULL){}static InputManager&getInstance(){static InputManager i;return i;}static InputManager*getInstancePtr(){return &getInstance();}void addWidgetModal(Widget*w){C(w->parent==NULL,"modal is root as required by native MyGUI");modal=w;++modalAdds;}void removeWidgetModal(Widget*w){C(modal==w,"remove exact modal from InputManager");modal=NULL;focus=NULL;++modalRemoves;}void setKeyFocusWidget(Widget*w){focus=w;}};
struct RenderManager{IntSize view;static RenderManager&getInstance(){static RenderManager r;return r;}IntSize getViewSize(){return view;}};
}
struct WidgetRef{MyGUI::Widget*p;WidgetRef():p(NULL){refs().push_back(this);}void set(MyGUI::Widget*w){p=w;}MyGUI::Widget*get(){return p;}void reset(){p=NULL;}static std::vector<WidgetRef*>&refs(){static std::vector<WidgetRef*>r;return r;}};
MyGUI::Widget::~Widget(){for(size_t i=0;i<WidgetRef::refs().size();++i)if(WidgetRef::refs()[i]->p==this)WidgetRef::refs()[i]->p=NULL;for(size_t i=0;i<children.size();++i)delete children[i];--liveWidgets;}
static WidgetRef mr,yr,cr;static WidgetRef*g_resetModalRef=&mr,*g_resetYesRef=&yr,*g_resetCancelRef=&cr;
static McmReset::Confirmation g_resetConfirmation;static bool g_resetUiReady=true;
struct ModEntry{std::string title;};static ModEntry m;static ModEntry*g_current=&m;static MyGUI::Widget tab;static MyGUI::Widget*g_tab=&tab;
static const char*Tr(const char*s){return s;}static void ErrorLog(const std::string&){++errors;}
"""
code='\n'.join(body(x)for x in ['std::string EscapeTags(const std::string& v)\n','bool ResetModalAlive()','void DestroyResetModal()\n','void OnResetConfirm(','void OnResetCancel(','void OnResetWindowButton(','bool CreateResetConfirmation()\n'])
cases=','.join('{"%s",%d,%d}'%x for x in skins)
suffix=r"""
int main(){
 struct Skin{const char*name;int w,h;};Skin skins[]={SKINS};m.title="Test #FF0000 mod";unsigned baseline=liveWidgets;
 for(unsigned skin=0;skin<2;++skin)for(unsigned small=0;small<2;++small){insetW=skins[skin].w;insetH=skins[skin].h;MyGUI::RenderManager::getInstance().view=MyGUI::IntSize(small?480:1920,small?300:1080);
 C(CreateResetConfirmation(),"actual modal construction succeeds for real skin metrics");MyGUI::Window*box=static_cast<MyGUI::Window*>(mr.get());C(box&&box->children.size()==3,"one warning and exactly two buttons");MyGUI::EditBox*warning=static_cast<MyGUI::EditBox*>(box->children[0]);C(warning->colour.r==1.0f&&warning->colour.g<0.3f&&warning->colour.b<0.3f,"intentional red warning survives either skin");C(warning->multiline&&warning->wrap&&warning->stat&&!warning->key,"warning static wrapped and unfocusable");C(warning->caption.find("##FF0000")!=std::string::npos,"mod title cannot inject colour tags");C(!yr.get()->key&&MyGUI::InputManager::getInstance().focus==cr.get(),"Cancel is default focus and Reset cannot gain key focus");MyGUI::IntCoord client=box->getClientCoord();C(warning->c.width>0&&warning->c.height>0,"positive warning text area");for(unsigned j=1;j<3;++j){MyGUI::IntCoord b=box->children[j]->c;C(b.left>=0&&b.top>=warning->c.top+warning->c.height&&b.left+b.width<=client.width&&b.top+b.height<=client.height,"buttons fit client and do not cover warning");}DestroyResetModal();C(liveWidgets==baseline&&!mr.get()&&!yr.get()&&!cr.get()&&!MyGUI::InputManager::getInstance().modal,"destroy unlinks all refs and releases every widget");}
 g_resetUiReady=false;C(!CreateResetConfirmation()&&errors==1&&liveWidgets==baseline,"missing native lifecycle capability fails closed without a modal");g_resetUiReady=true;MyGUI::RenderManager::getInstance().view=MyGUI::IntSize(200,150);C(!CreateResetConfirmation()&&liveWidgets==baseline,"unusable viewport creates no partial modal");C(modalAdds==modalRemoves,"modal stack balanced across all normal opens");std::printf("PASS %u extracted reset UI checks, vanilla/Dark templates\n",checks);return 0;}
""".replace('SKINS',cases)
cpp=OUT/'modal.cpp';cpp.write_text(prefix+code+suffix,encoding='utf-8');vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{ROOT}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ['/Od','/O2']:
 exe=OUT/('modal-'+opt[1:]+'.exe');p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(cpp),'/Fo'+str(OUT)+'/', '/Fe'+str(exe)],cwd=OUT,env=env,capture_output=True,text=True)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);print(opt,p.stdout,end='');(OUT/('run-'+opt[1:]+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0
