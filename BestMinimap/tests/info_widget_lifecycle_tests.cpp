// Actual InfoRow/UpdateInfo + shared WidgetRef are extracted by build_tests.ps1.
// A bounded MyGUI surface models parent ownership and unlink notifications;
// this verifies plugin behavior, not the game's renderer or skin rasterization.
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>
#include <cstdarg>
#include "../MinimapInfoPolicy.h"
typedef unsigned DWORD;
struct SYSTEMTIME { unsigned wHour,wMinute,wSecond,wMilliseconds; };
int clockReads=0;
void GetLocalTime(SYSTEMTIME* t){++clockReads;t->wHour=23;t->wMinute=59;t->wSecond=58;t->wMilliseconds=0;}
namespace Ogre { struct Vector3 { double x,y,z; Vector3(double a=0,double b=0,double c=0):x(a),y(b),z(c){} }; }
int liveWidgets=0,createdWidgets=0,captionChanges=0,cameraReads=0;
int skinHorizontal=4,skinVertical=2,nativeLineHeight=24,language=0;
namespace MyGUI {
struct IntCoord {
    int left,top,width,height;
    IntCoord(int l=0,int t=0,int w=0,int h=0):left(l),top(t),width(w),height(h){}
    bool operator!=(const IntCoord& o)const{return left!=o.left || top!=o.top || width!=o.width || height!=o.height;}
};
class UString {
    std::vector<unsigned> points;
public:
    UString(){} UString(const char* s){assign(s);} UString(const std::string& s){assign(s.c_str());}
    void assign(const char* s){while(*s){unsigned c=(unsigned char)*s++;if(c>=192){int n=c<224?1:(c<240?2:3);c&=n==1?31:(n==2?15:7);while(n-- && *s)c=(c<<6)|((unsigned char)*s++&63);}points.push_back(c);}}
    size_t size()const{return points.size();}
    unsigned operator[](size_t i)const{return points[i];}
    UString substr(size_t start,size_t count)const{UString r;for(size_t i=start;i<points.size() && i<start+count;++i)r.points.push_back(points[i]);return r;}
    UString& append(const char* s){UString r(s);points.insert(points.end(),r.points.begin(),r.points.end());return *this;}
};
struct IntSize {int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct Align { enum Value { Default=0, Left=1, Top=2 }; };
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
    int getWidth()const{return coord.width;} int getHeight()const{return coord.height;}
    bool mouseFocus; void setNeedMouseFocus(bool v){mouseFocus=v;}
    bool getVisible()const{return visible;} void setVisible(bool v){visible=v;}
};
class TextBox:public Widget {
public:
    UString caption;int fontHeight;std::string font;
    void setTextShadow(bool){} void setFontName(const std::string& v){font=v;} void setFontHeight(int v){fontHeight=v;}
    void setTextAlign(int){}
    const std::string& getFontName()const{return font;} int getFontHeight()const{return fontHeight;}
    IntCoord getTextRegion(){return IntCoord(skinHorizontal/2,skinVertical/2,getWidth()-skinHorizontal,getHeight()-skinVertical);}
    IntSize getTextSize(){int width=0;for(size_t i=0;i<caption.size();++i)width+=caption[i]>=0x4e00?18:(caption[i]>=128?10:9);return IntSize((width*fontHeight+17)/18,nativeLineHeight);}
    void setCaption(const UString& text){caption=text;++captionChanges;}

};
}
#include "info_widget_ref_under_test.h"
WidgetRef g_rootRef,g_infoBackRef,g_infoTextRefs[4];
std::string g_infoCaptions[4];
MyGUI::IntSize g_infoSizes[4],g_infoInsets[4];int g_infoLimits[4]={},g_infoHeight=0;bool g_infoFontReported=false,g_debug=false;
void DebugLog(const std::string&){}
DWORD g_infoAt=0,g_infoClockAt=0,g_infoClockWait=0;
char g_infoClock[6]={};
unsigned g_infoFields=0;bool g_infoDirty=true;double g_range=2000;
int viewWidth=420; int ViewWidth(){return viewWidth;} int ViewHeight(){return 80;} int GamePad(){return 0;}
template<class T> T Hi(T a,T b){return a>b?a:b;}
const char* Tr(const char* text){
    if(language==1){
        if(!std::strcmp(text,"Zoom:%s"))return "\xd0\x9c\xd0\xb0\xd1\x81\xd1\x88\xd1\x82\xd0\xb0\xd0\xb1:%s";
        if(!std::strcmp(text,"PC time:%s"))return "\xd0\x92\xd1\x80\xd0\xb5\xd0\xbc\xd1\x8f\x20\xd0\x9f\xd0\x9a:%s";
        if(!std::strcmp(text,"Dir:%03d"))return "\xd0\x9a\xd1\x83\xd1\x80\xd1\x81:%03d";
    }
    if(language==2){
        if(!std::strcmp(text,"Zoom:%s"))return "\xe7\xbc\xa9\xe6\x94\xbe:%s";
        if(!std::strcmp(text,"PC time:%s"))return "\xe7\x94\xb5\xe8\x84\x91\xe6\x97\xb6\xe9\x97\xb4:%s";
        if(!std::strcmp(text,"Dir:%03d"))return "\xe5\x90\x91:%03d";
    }
    return text;
}
int requestedFont=0;std::string FontFor(int px){requestedFont=px;return "NativeFont";}
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
    C(createdWidgets==6 && liveWidgets==6);C(root->children.size()==1);
    C(g_infoBackRef.get()->mouseFocus);C(requestedFont==22);
    C(g_infoBackRef.get()->getCoord().top==83);C(g_infoHeight<=88);
    C(g_infoCaptions[0]=="Zoom:100%");C(g_infoCaptions[1]=="PC time:23:59");
    C(g_infoCaptions[2]=="X:15 Z:-15");C(g_infoCaptions[3]=="Dir:090");
    C(clockReads==1 && cameraReads==1);
    int captions=captionChanges;
    UpdateInfo(Ogre::Vector3(200,0,-150),1100);C(captionChanges==captions && cameraReads==1 && clockReads==1);
    UpdateInfo(Ogre::Vector3(200,0,-150),1251);C(captionChanges==captions+1 && cameraReads==2 && clockReads==1);
    g_infoFields=BestMinimapInfo::Clock;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1260);
    C(g_infoHeight==52);C(g_infoTextRefs[1].get()->getCoord().top==12);
    for(int i=0;i<4;++i)C(g_infoTextRefs[i].get()->getVisible()==(i==1));
    C(createdWidgets==6 && clockReads==1);
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1261);
    C(!g_infoBackRef.get()->getVisible());C(g_infoHeight==0);C(createdWidgets==6 && clockReads==1);
    for(int w=80;w<=420;w+=20){viewWidth=w;g_infoFields=15;g_infoDirty=true;UpdateInfo(Ogre::Vector3(99999999,0,-99999999),1300+w);
        C(createdWidgets==6);MyGUI::Widget* back=g_infoBackRef.get();C(back->getVisible());
        for(int i=0;i<4;++i){MyGUI::TextBox* box=g_infoTextRefs[i].get()->castType<MyGUI::TextBox>();
            const MyGUI::IntCoord r=box->getCoord();C(box->getVisible());C(r.left>=12 && r.left+r.width<=back->getWidth()-12);
            C(box->getTextSize().width<=box->getTextRegion().width);C(box->getTextSize().height<=box->getTextRegion().height);
        }
    }
    captions=captionChanges;g_infoDirty=true;UpdateInfo(Ogre::Vector3(99999999,0,-99999999),1900);C(captionChanges==captions);
    UpdateInfo(Ogre::Vector3(200,0,-150),3001);C(clockReads==2);
    // Decode first, then clip code points. No byte fragments can survive.
    viewWidth=80;g_infoFields=BestMinimapInfo::Scale;g_infoDirty=true;
    InfoRow(g_infoBackRef.get(),0,"\xd0\x9c\xd0\xb0\xd1\x81\xd1\x88\xd1\x82\xd0\xb0\xd0\xb1:100%");
    FitInfoField(0,BestMinimapInfo::FieldRect());
    MyGUI::TextBox* box=g_infoTextRefs[0].get()->castType<MyGUI::TextBox>();C(box->caption.size()==0);
    BestMinimapInfo::FieldRect clipped={12,12,64,32};FitInfoField(0,clipped);
    C(box->caption.size()>=3);C(box->caption[box->caption.size()-1]=='.');C(box->getTextSize().width<=box->getTextRegion().width);
    delete root;C(g_rootRef.get()==NULL && g_infoBackRef.get()==NULL);C(liveWidgets==0);
    for(int i=0;i<4;++i)C(g_infoTextRefs[i].get()==NULL);
    root=new MyGUI::Widget;g_rootRef.set(root);g_infoDirty=true;g_infoFields=15;UpdateInfo(Ogre::Vector3(200,0,-150),3010);
    C(liveWidgets==6);C(g_infoCaptions[0]=="Zoom:100%");C(clockReads==2);delete root;C(liveWidgets==0);
    // Every toggle combination, three locales, two text-region/line-height
    // models, and the user's 80/100/420-pixel sizes. Renderer QA stays in-game.
    const unsigned flags[4]={1,8,2,4};const int widths[3]={80,100,420};
    for(language=0;language<3;++language)for(int skin=0;skin<2;++skin){
        skinHorizontal=skin?8:0;skinVertical=skin?4:0;nativeLineHeight=skin?28:24;
        root=new MyGUI::Widget;g_rootRef.set(root);
        for(int wi=0;wi<3;++wi)for(unsigned mask=1;mask<=15;++mask){
            viewWidth=widths[wi];g_infoFields=mask;g_infoDirty=true;g_range=30;
            UpdateInfo(Ogre::Vector3(-999999999,0,999999999),4000);
            MyGUI::Widget* back=g_infoBackRef.get();C(liveWidgets<=6);C(back->getVisible());
            if(viewWidth==420)C(g_infoHeight<=2*(nativeLineHeight+skinVertical+2)+28);
            int previous=-1;
            int rowFirst=-1,rowLast=-1;
            for(int i=0;i<4;++i){MyGUI::Widget* w=g_infoTextRefs[i].get();
                if(!(mask&flags[i])){C(w==NULL || !w->getVisible());continue;}
                MyGUI::TextBox* box=w->castType<MyGUI::TextBox>();const MyGUI::IntCoord r=box->getCoord();
                C(box->getVisible());C(r.left>=12 && r.left+r.width<=back->getWidth()-12);
                C(r.top>=12 && r.top+r.height<=g_infoHeight-12);
                C(box->getTextSize().width<=box->getTextRegion().width);C(box->getTextSize().height<=box->getTextRegion().height);
                if(previous>=0){const MyGUI::IntCoord p=g_infoTextRefs[previous].get()->getCoord();
                    if(p.top==r.top)C(p.left+p.width+12==r.left);
                }
                if(rowFirst<0)rowFirst=i;
                if(rowLast>=0 && g_infoTextRefs[rowLast].get()->getCoord().top!=r.top){
                    const MyGUI::IntCoord f=g_infoTextRefs[rowFirst].get()->getCoord();
                    const MyGUI::IntCoord l=g_infoTextRefs[rowLast].get()->getCoord();
                    C(std::abs(f.left-(back->getWidth()-l.left-l.width))<=1);rowFirst=i;
                }
                rowLast=i;previous=i;
            }
            if(rowFirst>=0){const MyGUI::IntCoord f=g_infoTextRefs[rowFirst].get()->getCoord();const MyGUI::IntCoord l=g_infoTextRefs[rowLast].get()->getCoord();C(std::abs(f.left-(back->getWidth()-l.left-l.width))<=1);}
        }
        delete root;C(liveWidgets==0);
    }
    std::printf("%d actual info widget/WidgetRef lifecycle checks, %d failures\n",checks,failures);
    return failures?1:0;
}
