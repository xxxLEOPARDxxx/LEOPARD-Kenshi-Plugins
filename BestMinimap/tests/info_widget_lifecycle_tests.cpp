// Actual InfoRow/UpdateInfo + shared WidgetRef are extracted by build_tests.ps1.
// A bounded MyGUI surface models parent ownership and unlink notifications;
// this verifies plugin behavior, not the game's renderer or skin rasterization.
#include <cstdio>
#include <string>
#include <vector>
#include "../MinimapInfoPolicy.h"
typedef unsigned DWORD;
struct SYSTEMTIME { unsigned wHour,wMinute,wSecond,wMilliseconds; };
int clockReads=0;
void GetLocalTime(SYSTEMTIME* t){++clockReads;t->wHour=23;t->wMinute=59;t->wSecond=58;t->wMilliseconds=0;}
namespace Ogre { struct Vector3 { double x,y,z; Vector3(double a=0,double b=0,double c=0):x(a),y(b),z(c){} }; }
int liveWidgets=0,createdWidgets=0,captionChanges=0,cameraReads=0;
namespace MyGUI {
struct IntCoord {
    int left,top,width,height;
    IntCoord(int l=0,int t=0,int w=0,int h=0):left(l),top(t),width(w),height(h){}
    bool operator!=(const IntCoord& o)const{return left!=o.left || top!=o.top || width!=o.width || height!=o.height;}
};
typedef std::string UString;
struct Align { enum Value { Default }; };
class Widget;
struct IUnlinkWidget { virtual void _unlinkWidget(Widget*)=0; };
class WidgetManager {
    std::vector<IUnlinkWidget*> refs;
public:
    static WidgetManager* getInstancePtr(){static WidgetManager manager;return &manager;}
    void registerUnlinker(IUnlinkWidget* ref){refs.push_back(ref);}
    void unlink(Widget* w){for(size_t i=0;i<refs.size();++i)refs[i]->_unlinkWidget(w);}
};
class Widget {
    IntCoord coord; bool visible;
public:
    std::vector<Widget*> children;
    Widget():visible(true){++liveWidgets;++createdWidgets;}
    virtual ~Widget(){while(!children.empty()){delete children.back();children.pop_back();}WidgetManager::getInstancePtr()->unlink(this);--liveWidgets;}
    template<class T> T* createWidget(const char*,const IntCoord& c,Align::Value,const char*){T* w=new T;w->setCoord(c);children.push_back(w);return w;}
    template<class T> T* castType(bool=false){return dynamic_cast<T*>(this);}
    void setCoord(const IntCoord& c){coord=c;} const IntCoord& getCoord()const{return coord;}
    int getWidth()const{return coord.width;}
    void setNeedMouseFocus(bool){}
    bool getVisible()const{return visible;} void setVisible(bool v){visible=v;}
};
class TextBox:public Widget {
public:
    std::string caption;
    void setTextShadow(bool){} void setFontName(const std::string&){} void setFontHeight(int){}
    void setCaption(const UString& text){caption=text;++captionChanges;}
};
}
#include "info_widget_ref_under_test.h"
WidgetRef g_rootRef,g_infoBackRef,g_infoTextRefs[5];
std::string g_infoCaptions[5];
DWORD g_infoAt=0,g_infoClockAt=0,g_infoClockWait=0;
char g_infoClock[6]={};
unsigned g_infoFields=0;bool g_infoDirty=true;double g_range=2000;
int ViewWidth(){return 80;} int ViewHeight(){return 80;} int GamePad(){return 0;}
template<class T> T Hi(T a,T b){return a>b?a:b;}
const char* Tr(const char* text){return text;}
std::string FontFor(int){return "SmallFont";}
struct Camera { Ogre::Vector3 getDerivedDirection(){++cameraReads;return Ogre::Vector3(1,0,0);} };
struct CameraClass { Camera* camera; };
struct Player { CameraClass* camera; };
struct World { Player* player; };
Camera camera; CameraClass cameraClass={&camera}; Player player={&cameraClass};World world={&player};World* ou=&world;
struct Affine { double au,bu,av,bv; }g_map={1,0,0,1};
struct Source { int w,h; }g_src={100,100};
#include "info_widget_runtime_under_test.h"
int main(){int failures=0,checks=0;
#define C(x) do {++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
    MyGUI::Widget* root=new MyGUI::Widget;g_rootRef.set(root);
    UpdateInfo(Ogre::Vector3(),1000);C(root->children.empty());C(clockReads==0);
    g_infoFields=15;UpdateInfo(Ogre::Vector3(150,0,-150),1001);
    C(createdWidgets==7 && liveWidgets==7);C(root->children.size()==1);
    C(g_infoBackRef.get()->getCoord().top==83);C(g_infoBackRef.get()->getCoord().height==88);
    C(g_infoCaptions[0]=="Zoom:100%");C(g_infoCaptions[1]=="X:15");C(g_infoCaptions[2]=="Z:-15");
    C(g_infoCaptions[3]=="Dir:090");C(g_infoCaptions[4]=="Time:23:59");
    C(clockReads==1 && cameraReads==1 && captionChanges==5);
    UpdateInfo(Ogre::Vector3(200,0,-150),1100);C(captionChanges==5 && cameraReads==1 && clockReads==1);
    UpdateInfo(Ogre::Vector3(200,0,-150),1251);C(captionChanges==6 && cameraReads==2 && clockReads==1);
    g_infoFields=BestMinimapInfo::Clock;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1260);
    C(g_infoBackRef.get()->getCoord().height==24);
    C(g_infoTextRefs[4].get()->getCoord().top==4);
    for(int i=0;i<4;++i)C(!g_infoTextRefs[i].get()->getVisible());
    C(g_infoTextRefs[4].get()->getVisible());C(createdWidgets==7 && clockReads==1);
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1261);
    C(!g_infoBackRef.get()->getVisible());C(createdWidgets==7 && clockReads==1);
    g_infoFields=15;UpdateInfo(Ogre::Vector3(200,0,-150),1300);
    C(g_infoBackRef.get()->getVisible());C(createdWidgets==7 && liveWidgets==7);
    for(int i=0;i<5;++i)C(g_infoTextRefs[i].get()->getVisible());
    UpdateInfo(Ogre::Vector3(200,0,-150),3001);C(clockReads==2);
    delete root;C(g_rootRef.get()==NULL && g_infoBackRef.get()==NULL);C(liveWidgets==0);
    for(int i=0;i<5;++i)C(g_infoTextRefs[i].get()==NULL);
    root=new MyGUI::Widget;g_rootRef.set(root);g_infoDirty=true;UpdateInfo(Ogre::Vector3(200,0,-150),3010);
    C(liveWidgets==7);C(g_infoTextRefs[0].get()->castType<MyGUI::TextBox>()->caption=="Zoom:100%");
    C(clockReads==2);delete root;C(liveWidgets==0);
    std::printf("%d actual info widget/WidgetRef lifecycle checks, %d failures\n",checks,failures);
    return failures?1:0;
}
