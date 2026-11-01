#include "../../SavePreviewAlpha.h"
#include "../../SavePreviewPolicy.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
struct ID3D11Texture2D{};static ID3D11Texture2D nativeTexture;
namespace MyGUI{struct Widget;}
struct WeakRef;static std::vector<WeakRef*> refs;static void Dispatch(MyGUI::Widget*);
namespace MyGUI
{
    struct Widget
    {
        float alpha,effective;bool inherits,visible;Widget* parent;std::string name;std::vector<Widget*> children,skin;
        Widget(const char*n="",float a=1,bool i=true):alpha(a),effective(a),inherits(i),visible(true),parent(NULL),name(n){}
        float getAlpha()const{return alpha;}void setAlpha(float a){if(a!=alpha){alpha=a;Dispatch(this);}}
        bool getInheritsAlpha()const{return inherits;}void setInheritsAlpha(bool i){inherits=i;Dispatch(this);}
        bool getVisible()const{return visible;}
        bool getInheritedVisible()const{return !parent||(parent->getVisible()&&parent->getInheritedVisible());}
        const std::string& getName()const{return name;}size_t getChildCount()const{return children.size();}
        Widget* getChildAt(size_t i){return children[i];}
    };
    static std::vector<Widget*> guiRoots;
    struct EnumeratorWidgetPtr{size_t i;EnumeratorWidgetPtr():i(0){}bool next(){return i++<guiRoots.size();}Widget*current(){return guiRoots[i-1];}};
    struct Gui{static Gui&getInstance(){static Gui g;return g;}EnumeratorWidgetPtr getEnumerator(){return EnumeratorWidgetPtr();}};
}
struct WeakRef
{
    MyGUI::Widget*p;WeakRef():p(NULL){}MyGUI::Widget*get()const{return p;}
    void set(MyGUI::Widget*w){p=w;if(std::find(refs.begin(),refs.end(),this)==refs.end())refs.push_back(this);}
    void release(){refs.erase(std::remove(refs.begin(),refs.end(),this),refs.end());p=NULL;}
};
typedef SavePreviewAlpha<MyGUI::Widget,WeakRef> AlphaMask;
typedef WeakRef PreviewWidgetRef;
static AlphaMask* activeAlpha=NULL;static MyGUI::Widget* settingAlpha=NULL;
static void NativeAlpha(MyGUI::Widget*w)
{w->effective=w->alpha*((w->inherits&&w->parent)?w->parent->effective:1);for(size_t i=0;i<w->children.size();++i)Dispatch(w->children[i]);for(size_t i=0;i<w->skin.size();++i)Dispatch(w->skin[i]);}
static void(*originalAlpha)(MyGUI::Widget*)=NativeAlpha;
// EXTRACT_ALPHA
static void Dispatch(MyGUI::Widget*w){AlphaUpdate(w);}
struct Frame{bool hud,modal;Frame():hud(false),modal(false){}};
static Frame frame;static bool backupAllowed=true,throwRead=false,nested=false,nestedDone=false,cancelInDraw=false;
static unsigned mainDraws=0,rttDraws=0,reads=0,submits=0,errors=0;static std::vector<std::string> events;
namespace SavePreviewGpu
{
    struct SceneBackup
    {
        Frame old;bool saved;SceneBackup():saved(false){}~SceneBackup(){Restore();}
        bool Begin(ID3D11Texture2D*){if(!backupAllowed)return false;old=frame;saved=true;events.push_back("backup");return true;}
        bool Restore(){if(saved){saved=false;frame=old;events.push_back("restore");}return true;}
    };
}
struct Window{void getCustomAttribute(const char*,ID3D11Texture2D**out){*out=&nativeTexture;}};
struct Renderer{Window*window;};struct GameWorld{};struct Owner{Renderer*render;void*titleScreen;};
static Window window;static Renderer render={&window},*renderer=&render;static GameWorld game,*ou=&game,*world=&game;static Owner owner={&render,NULL},*au=&owner;
struct LoadSaveWindow{MyGUI::Widget*mMainWidget;};
struct SaveManager{LoadSaveWindow*saveMenu,*loadMenu,*importMenu;static SaveManager*getSingleton();};
static SaveManager sm={};SaveManager*SaveManager::getSingleton(){return &sm;}
static WeakRef panel;static bool enabled=true,debug=false,inCapture=false;static SavePreviewPolicy::Pending pending;static std::vector<unsigned char> thumbnail;
static bool OnMain(){return true;}static bool MainTarget(void*target){return target==(void*)1;}
static void Cancel(){pending.Reset();thumbnail.clear();renderer=NULL;world=NULL;}
static void ErrorLog(const std::string&){++errors;}static void DebugLog(const std::string&){}
static void Submit(){if(pending.Ready()){++submits;events.push_back("submit");pending.Reset();}}
// EXTRACT_REQUIRE
static void CapturePixels(unsigned ticket)
{RequireCapture(ticket);++reads;events.push_back("read");if(throwRead)throw std::runtime_error("readback fixture failure");if(!frame.hud||frame.modal)throw std::runtime_error("HUD missing or modal captured");}
typedef void(*DrawFunction)(void*,void*,bool);static void DrawGui(void*,void*,bool,DrawFunction);
static MyGUI::Widget saveRoot("save",.8f),saveName("name",1,false),pauseRoot("pause",.75f),pausePanel("p_MainMenuPopupPanel",1,false),hud("hud");
static void NativeDraw(void*self,void*target,bool update)
{
    if(target!=(void*)1){++rttDraws;events.push_back("rtt");return;}
    ++mainDraws;frame.hud=true;
    frame.modal=(saveRoot.visible&&(saveRoot.effective>0||saveName.effective>0))||(pauseRoot.visible&&(pauseRoot.effective>0||pausePanel.effective>0));
    events.push_back(frame.modal?"FULL":"HUD");
    if(nested&&!nestedDone){nestedDone=true;DrawGui(self,(void*)2,true,NativeDraw);}
    if(cancelInDraw&&mainDraws==1)Cancel();
}
// EXTRACT_CAPTURE
static void Setup(bool modal)
{
    renderer=&render;world=ou=&game;au=&owner;enabled=true;inCapture=false;activeAlpha=NULL;settingAlpha=NULL;
    mainDraws=rttDraws=reads=submits=errors=0;events.clear();frame=Frame();pending.Begin("fixture",17);pending.Complete(true);
    backupAllowed=true;throwRead=nested=nestedDone=cancelInDraw=false;saveRoot.visible=pauseRoot.visible=modal;
    saveRoot.alpha=.8f;saveName.alpha=1;pauseRoot.alpha=.75f;pausePanel.alpha=1;NativeAlpha(&saveRoot);NativeAlpha(&pauseRoot);
    static LoadSaveWindow menu={&saveRoot};sm.saveMenu=modal?&menu:NULL;sm.loadMenu=sm.importMenu=NULL;
    MyGUI::guiRoots.clear();MyGUI::guiRoots.push_back(&hud);MyGUI::guiRoots.push_back(&saveRoot);MyGUI::guiRoots.push_back(&pauseRoot);
}
static unsigned checks=0;
#define CHECK(x)do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    saveRoot.children.push_back(&saveName);saveName.parent=&saveRoot;pauseRoot.children.push_back(&pausePanel);pausePanel.parent=&pauseRoot;
    Setup(true);DrawGui(NULL,(void*)1,false,NativeDraw);
    CHECK(mainDraws==2&&reads==1&&submits==1&&errors==0);CHECK(frame.hud&&frame.modal&&saveRoot.alpha==.8f&&pausePanel.alpha==1);
    const char* order[]={"backup","HUD","read","restore","FULL","submit"};CHECK(events.size()==6);for(unsigned i=0;i<6;++i)CHECK(events[i]==order[i]);
    CHECK(!inCapture&&activeAlpha==NULL&&settingAlpha==NULL&&refs.empty());
    Setup(false);DrawGui(NULL,(void*)1,false,NativeDraw);CHECK(mainDraws==1&&reads==1&&submits==1&&errors==0&&events[0]=="HUD");
    Setup(true);throwRead=true;DrawGui(NULL,(void*)1,false,NativeDraw);CHECK(mainDraws==2&&reads==1&&submits==0&&errors==1);
    CHECK(frame.hud&&frame.modal&&saveRoot.alpha==.8f&&pausePanel.alpha==1&&refs.empty()&&!inCapture);
    Setup(true);backupAllowed=false;DrawGui(NULL,(void*)1,false,NativeDraw);CHECK(mainDraws==1&&reads==0&&submits==0&&errors==1&&frame.modal);
    Setup(true);cancelInDraw=true;DrawGui(NULL,(void*)1,false,NativeDraw);CHECK(mainDraws==2&&reads==0&&submits==0&&frame.modal&&refs.empty());
    Setup(true);nested=true;DrawGui(NULL,(void*)1,false,NativeDraw);CHECK(mainDraws==2&&rttDraws==1&&reads==1&&submits==1&&errors==0);
    std::printf("PASS actual HUD capture sequencing: %u checks; full-HUD, modal mask, restore, cancellation and nested RTT\n",checks);return 0;
}
