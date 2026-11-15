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
int skinLeft=2,skinTop=1,glyphTop=5,glyphOverhang=0,glyphLeft=1,fontQueries=0;
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
    bool empty()const{return points.empty();}
    unsigned operator[](size_t i)const{return points[i];}
    UString substr(size_t start,size_t count)const{UString r;for(size_t i=start;i<points.size() && i<start+count;++i)r.points.push_back(points[i]);return r;}
    UString& append(const char* s){UString r(s);points.insert(points.end(),r.points.begin(),r.points.end());return *this;}
};
struct IntSize {int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct Align { enum Value { Default=0, Left=1, Top=2, HCenter=4, VCenter=8 }; };
struct Event { template<class T> void operator+=(T){} };
template<class T> int newDelegate(T){return 0;}
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
    std::string skin; Widget* parent; int depth;
    Event eventMouseButtonPressed,eventMouseDrag,eventMouseButtonReleased,eventMouseWheel,eventMouseButtonDoubleClick;
    std::vector<Widget*> children;
    Widget():visible(true),parent(NULL),depth(0){++liveWidgets;++createdWidgets;}
    virtual ~Widget(){while(!children.empty())delete children.back();if(parent){for(size_t i=0;i<parent->children.size();++i)if(parent->children[i]==this){parent->children.erase(parent->children.begin()+i);break;}}WidgetManager::getInstancePtr()->unlink(this);--liveWidgets;}
    template<class T> T* createWidget(const char* name,const IntCoord& c,Align::Value,const char*){T* w=new T;w->skin=name;w->parent=this;w->setCoord(c);children.push_back(w);return w;}
    template<class T> T* castType(bool=false){return dynamic_cast<T*>(this);}
    void setCoord(const IntCoord& c){coord=c;} const IntCoord& getCoord()const{return coord;}
    void setCoord(int l,int t,int w,int h){coord=IntCoord(l,t,w,h);}
    void setSize(int w,int h){coord.width=w;coord.height=h;}
    void setPosition(int l,int t){coord.left=l;coord.top=t;}
    void setDepth(int v){depth=v;}
    int getWidth()const{return coord.width;} int getHeight()const{return coord.height;}
    bool mouseFocus; void setNeedMouseFocus(bool v){mouseFocus=v;}
    bool getVisible()const{return visible;} void setVisible(bool v){visible=v;}
};
class ImageBox:public Widget {
public:
    std::string texture;
    const std::string& _getTextureName(){return texture;}
    void setImageInfo(const std::string& t,const IntCoord&,const IntSize&){texture=t;}
    void setImageIndex(int){}
};
class RenderManager {public:static RenderManager& getInstance(){static RenderManager r;return r;}IntSize getViewSize(){return IntSize(2560,1440);}};
class Gui {public:static Gui& getInstance(){static Gui g;return g;}void destroyWidget(Widget* w){delete w;}
template<class T>T* createWidget(const char* name,const IntCoord& c,Align::Value,const char*,const char*){T* w=new T;w->skin=name;w->setCoord(c);return w;}};
struct GlyphInfo { float width,height,advance,bearingX,bearingY; };
class IFont {
public:
    int getDefaultHeight(){return nativeLineHeight;}
    GlyphInfo* getGlyphInfo(unsigned c){static GlyphInfo g;float width=c>=0x4e00?18.0f:(c>=128?10.0f:9.0f);
        g.width=width-1;g.advance=width-glyphLeft;g.bearingX=(float)glyphLeft;g.bearingY=(float)glyphTop;
        g.height=(float)(nativeLineHeight-glyphTop-4+glyphOverhang);
        if(c=='g' || c=='p' || c==0x440 || c==0x443)g.height+=3;
        return &g;}
};
class FontManager {public:static FontManager& getInstance(){static FontManager f;return f;}IFont* getByName(const std::string&){++fontQueries;static IFont f;return &f;}};
class TextBox:public Widget {
public:
    UString caption;int fontHeight,alignment;std::string font;bool shadow;
    TextBox():fontHeight(22),alignment(0),shadow(false){}
    void setTextShadow(bool v){shadow=v;} void setFontName(const std::string& v){font=v;} void setFontHeight(int v){fontHeight=v;}
    void setTextAlign(int v){alignment=v;}
    const std::string& getFontName()const{return font;} int getFontHeight()const{return fontHeight;}
    const UString& getCaption(){return caption;}
    IntCoord getTextRegion(){return IntCoord(skinLeft,skinTop,getWidth()-skinHorizontal,getHeight()-skinVertical);}
    IntSize getTextSize(){float width=0;IFont* f=FontManager::getInstance().getByName(font);for(size_t i=0;i<caption.size();++i){GlyphInfo* g=f->getGlyphInfo(caption[i]);float scale=(float)fontHeight/f->getDefaultHeight();width+=g->bearingX*scale+g->advance*scale;}return IntSize((int)std::ceil(width)+(shadow?1:0),fontHeight+(shadow?1:0));}
    // Independent rendering model follows native SimpleText/EditText: alignment
    // uses advance/line height, while quads use bearings and scaled ink bounds.
    IntCoord rasterBounds(){if(!caption.size())return IntCoord();
        IntCoord r=getTextRegion();IntSize line=getTextSize();int x=r.left,y=r.top;
        if(alignment&Align::HCenter)x+=(r.width-line.width+(shadow?1:0))/2;
        if(alignment&Align::VCenter)y+=(r.height-fontHeight)/2;
        float pen=0,left=1e6f,top=1e6f,right=-1e6f,bottom=-1e6f;
        IFont* f=FontManager::getInstance().getByName(font);float scale=(float)fontHeight/f->getDefaultHeight();
        for(size_t i=0;i<caption.size();++i){GlyphInfo g=*f->getGlyphInfo(caption[i]);
            float l=pen+g.bearingX*scale,t=g.bearingY*scale;
            if(l<left)left=l;if(t<top)top=t;if(l+g.width*scale>right)right=l+g.width*scale;if(t+g.height*scale>bottom)bottom=t+g.height*scale;
            pen+=g.bearingX*scale+g.advance*scale;}
        int l=(int)std::floor(left),t=(int)std::floor(top);
        return IntCoord(x+l,y+t,(int)std::ceil(right+(shadow?1:0))-l,(int)std::ceil(bottom+(shadow?1:0))-t);
    }
    void setCaption(const UString& text){caption=text;++captionChanges;}
};
}
MyGUI::IntCoord PlacedInk(MyGUI::Widget* widget){MyGUI::TextBox* b=widget->castType<MyGUI::TextBox>();MyGUI::IntCoord ink=b->rasterBounds();ink.left+=b->getCoord().left;ink.top+=b->getCoord().top;return ink;}
bool FitsRegion(MyGUI::TextBox* box){MyGUI::IntCoord i=box->rasterBounds(),r=box->getTextRegion();return !box->caption.size() || (i.left>=r.left && i.top>=r.top && i.left+i.width<=r.left+r.width && i.top+i.height<=r.top+r.height);}
#include "info_widget_ref_under_test.h"
WidgetRef g_rootRef,g_infoBackRef,g_infoTextRefs[4],g_backRef,g_imageRef;
std::string g_infoCaptions[4];
MyGUI::IntSize g_infoSizes[4];MyGUI::IntCoord g_infoRegions[4];BestMinimapInfo::TextMetrics g_infoMetrics[4];int g_infoLimits[4]={},g_infoHeight=0;bool g_infoFontReported=false,g_debug=false,g_infoJoinedFrame=false;
void DebugLog(const std::string&){}
DWORD g_infoAt=0,g_infoClockAt=0,g_infoClockWait=0;
char g_infoClock[6]={};
unsigned g_infoFields=0;bool g_infoDirty=true;double g_range=2000;
int viewWidth=420,viewHeight=80,gamePad=0; int ViewWidth(){return viewWidth;} int ViewHeight(){return viewHeight;} int GamePad(){return gamePad;}
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
int g_left=0,g_top=0,g_viewW=0,g_viewH=0;bool g_placed=false,g_dirtyMask=false;
std::string g_texName="fixture";
void ClampPosition(int,int){}void WriteInt(const char*,int){}bool EnsureTexture(int,int){return true;}
void OnPressed(){}void OnDragged(){}void OnReleased(){}void OnWheel(){}void OnDoubleClick(){}
#include "map_widgets_under_test.h"
#include "info_widget_runtime_under_test.h"
int main(){int failures=0,checks=0;
#define C(x) do {++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
    MyGUI::Widget* root=new MyGUI::Widget;g_rootRef.set(root);
    UpdateInfo(Ogre::Vector3(),1000);C(root->children.empty());C(clockReads==0);
    g_infoFields=15;UpdateInfo(Ogre::Vector3(150,0,-150),1001);
    C(createdWidgets==6 && liveWidgets==6);C(root->children.size()==1);
    C(g_infoBackRef.get()->mouseFocus);C(requestedFont==22);
    C(g_infoBackRef.get()->getCoord().top==79);C(g_infoHeight<=88);
    C(g_infoCaptions[0]=="Zoom:100%");C(g_infoCaptions[1]=="PC time:23:59");
    C(g_infoCaptions[2]=="X:15 Z:-15");C(g_infoCaptions[3]=="Dir:090");
    C(clockReads==1 && cameraReads==1);
    int captions=captionChanges;
    UpdateInfo(Ogre::Vector3(200,0,-150),1100);C(captionChanges==captions && cameraReads==1 && clockReads==1);
    UpdateInfo(Ogre::Vector3(200,0,-150),1251);C(captionChanges==captions+1 && cameraReads==2 && clockReads==1);
    g_infoFields=BestMinimapInfo::Clock;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1260);
    C(g_infoHeight==PlacedInk(g_infoTextRefs[1].get()).height+24);C(PlacedInk(g_infoTextRefs[1].get()).top==12);
    for(int i=0;i<4;++i)C(g_infoTextRefs[i].get()->getVisible()==(i==1));
    C(createdWidgets==6 && clockReads==1);
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),1261);
    C(!g_infoBackRef.get()->getVisible());C(g_infoHeight==0);C(createdWidgets==6 && clockReads==1);
    for(int w=80;w<=420;w+=20){viewWidth=w;g_infoFields=15;g_infoDirty=true;UpdateInfo(Ogre::Vector3(99999999,0,-99999999),1300+w);
        C(createdWidgets==6);MyGUI::Widget* back=g_infoBackRef.get();C(back->getVisible());
        for(int i=0;i<4;++i){MyGUI::TextBox* box=g_infoTextRefs[i].get()->castType<MyGUI::TextBox>();
            const MyGUI::IntCoord r=PlacedInk(box);C(box->getVisible());C(r.left>=12 && r.left+r.width<=back->getWidth()-12);
            C(FitsRegion(box));
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
    C(box->caption.size()>=3);C(box->caption[box->caption.size()-1]=='.');C(FitsRegion(box));
    delete root;C(g_rootRef.get()==NULL && g_infoBackRef.get()==NULL);C(liveWidgets==0);
    for(int i=0;i<4;++i)C(g_infoTextRefs[i].get()==NULL);
    root=new MyGUI::Widget;g_rootRef.set(root);g_infoDirty=true;g_infoFields=15;UpdateInfo(Ogre::Vector3(200,0,-150),3010);
    C(liveWidgets==6);C(g_infoCaptions[0]=="Zoom:100%");C(clockReads==2);delete root;C(liveWidgets==0);
    // Every toggle combination, three locales, two text-region/line-height
    // models, and the user's 80/100/420-pixel sizes. Renderer QA stays in-game.
    const unsigned flags[4]={1,8,2,4};const int widths[3]={80,100,420};
    for(language=0;language<3;++language)for(int skin=0;skin<2;++skin){
        skinHorizontal=skin?8:0;skinVertical=skin?4:0;skinLeft=skinHorizontal/2;skinTop=skinVertical/2;nativeLineHeight=skin?28:24;
        root=new MyGUI::Widget;g_rootRef.set(root);
        for(int wi=0;wi<3;++wi)for(unsigned mask=1;mask<=15;++mask){
            viewWidth=widths[wi];g_infoFields=mask;g_infoDirty=true;g_range=30;
            UpdateInfo(Ogre::Vector3(-999999999,0,999999999),4000);
            MyGUI::Widget* back=g_infoBackRef.get();C(liveWidgets<=6);C(back->getVisible());
            if(viewWidth==420)C(g_infoHeight<=2*(nativeLineHeight+skinVertical+2)+28);
            int expectedWidths[4],expectedHeights[4],expectedCount=0,fieldIndex=0;
            for(int i=0;i<4;++i)if(mask&flags[i]){expectedWidths[expectedCount]=g_infoSizes[i].width;expectedHeights[expectedCount++]=g_infoSizes[i].height;}
            const BestMinimapInfo::FlowLayout expected=BestMinimapInfo::Flow(expectedWidths,expectedHeights,expectedCount,back->getWidth());
            for(int i=0;i<4;++i){MyGUI::Widget* w=g_infoTextRefs[i].get();
                if(!(mask&flags[i])){C(w==NULL || !w->getVisible());continue;}
                MyGUI::TextBox* box=w->castType<MyGUI::TextBox>();const MyGUI::IntCoord r=PlacedInk(box);
                C(box->getVisible());C(r.left>=12 && r.left+r.width<=back->getWidth()-12);
                C(r.top>=12 && r.top+r.height<=g_infoHeight-12);
                C(FitsRegion(box));
                const BestMinimapInfo::FieldRect& cell=expected.fields[fieldIndex++];
                C(r.width<=cell.width && r.height<=cell.height);
                C(r.left==cell.left+(cell.width-r.width)/2);
                C(r.top==cell.top+(cell.height-r.height)/2);
            }
        }
        delete root;C(liveWidgets==0);
    }
    // The same live root can switch game frame <-> bare/circular map. Do not
    // retain the previous skin or stale child references across the switch.
    root=new MyGUI::Widget;g_rootRef.set(root);g_infoFields=15;
    for(int pass=0;pass<6;++pass){
        gamePad=pass%2?0:8;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),6000+pass);
        MyGUI::Widget* back=g_infoBackRef.get();
        C(back->skin==(gamePad?"PanelEmpty":"Kenshi_FloatingPanelThinSkin"));
        C(root->children.size()==1 && liveWidgets==6);
        C(back->mouseFocus && back->getCoord().top==(gamePad?88:79));
        for(int i=0;i<4;++i)C(g_infoTextRefs[i].get()->parent==back);
        int created=createdWidgets;UpdateInfo(Ogre::Vector3(),6000+pass);
        C(createdWidgets==created);
    }
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),7000);C(!g_infoBackRef.get()->getVisible());
    delete root;C(liveWidgets==0);gamePad=0;
    // Exercise actual EnsureWidgets, not just the geometry policy. A single
    // backdrop covers the image/footer seam, including hidden round trips.
    viewWidth=viewHeight=420;gamePad=8;g_infoFields=9;g_infoDirty=true;
    C(EnsureWidgets());UpdateInfo(Ogre::Vector3(),8000);C(EnsureWidgets());
    C(g_backRef.get()->getCoord().height==g_rootRef.get()->getHeight());
    C(g_backRef.get()->depth>g_infoBackRef.get()->depth);
    C(g_infoBackRef.get()->skin=="PanelEmpty");
    C(g_imageRef.get()->getCoord().top+g_imageRef.get()->getHeight()==g_infoBackRef.get()->getCoord().top);
    // Same image dimensions, changed padding: texture need not be replaced.
    gamePad=0;g_infoDirty=true;C(EnsureWidgets());UpdateInfo(Ogre::Vector3(),8001);C(EnsureWidgets());
    C(g_backRef.get()==NULL);C(g_imageRef.get()->getCoord().left==0 && g_imageRef.get()->getCoord().top==0);
    C(g_infoBackRef.get()->skin=="Kenshi_FloatingPanelThinSkin");
    // Backdrop gets recreated after a retained hidden footer. Its explicit
    // lower draw priority must keep it behind the footer when shown again.
    gamePad=8;g_infoDirty=true;C(EnsureWidgets());UpdateInfo(Ogre::Vector3(),8002);C(EnsureWidgets());
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),8003);C(EnsureWidgets());
    gamePad=0;C(EnsureWidgets());gamePad=8;C(EnsureWidgets());
    g_infoFields=9;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),8004);C(EnsureWidgets());
    C(g_backRef.get()->depth>g_infoBackRef.get()->depth);
    C(g_backRef.get()->getHeight()==g_rootRef.get()->getHeight());
    C(g_infoBackRef.get()->getVisible());C(g_imageRef.get()->getCoord().top==8);
    // All info off restores an image-only native frame.
    g_infoFields=0;g_infoDirty=true;UpdateInfo(Ogre::Vector3(),8005);C(EnsureWidgets());
    C(g_backRef.get()->getHeight()==436);C(!g_infoBackRef.get()->getVisible());
    delete g_rootRef.get();C(liveWidgets==0);C(g_backRef.get()==NULL && g_imageRef.get()==NULL);
    // Regression: nominal line metrics do not describe rendered glyphs. Test
    // lower overhang beyond line height, asymmetric skin origin, full and
    // shortened Cyrillic/Chinese captions, and both shared/bare footer modes.
    for(int profile=0;profile<3;++profile){
        skinHorizontal=profile?9:0;skinVertical=profile?7:0;
        skinLeft=profile?7:0;skinTop=profile?1:0;
        nativeLineHeight=24;glyphTop=profile==2?-3:7;glyphOverhang=profile?9:0;glyphLeft=profile==2?-3:1;
        for(language=0;language<3;++language)for(int pad=0;pad<2;++pad)for(int narrow=0;narrow<2;++narrow){
            viewWidth=narrow?80:420;viewHeight=420;gamePad=pad?8:0;g_infoFields=9;g_infoDirty=true;
            C(EnsureWidgets());UpdateInfo(Ogre::Vector3(),9000);C(EnsureWidgets());
            int left=10000,top=10000,right=0,bottom=0;
            for(int i=0;i<2;++i){MyGUI::TextBox* b=g_infoTextRefs[i].get()->castType<MyGUI::TextBox>();
                MyGUI::IntCoord ink=b->rasterBounds(),region=b->getTextRegion(),coord=b->getCoord();
                C(ink.left>=region.left && ink.top>=region.top);
                C(ink.left+ink.width<=region.left+region.width && ink.top+ink.height<=region.top+region.height);
                int l=coord.left+ink.left,t=coord.top+ink.top,r=l+ink.width,bot=t+ink.height;
                if(l<left)left=l;if(t<top)top=t;if(r>right)right=r;if(bot>bottom)bottom=bot;
            }
            C(std::abs(left-(g_infoBackRef.get()->getWidth()-right))<=1);
            C(std::abs(top-(g_infoHeight-bottom))<=1);
            C(top>=12 && bottom<=g_infoHeight-12);
            int queries=fontQueries,captionsBefore=captionChanges;
            g_infoDirty=true;UpdateInfo(Ogre::Vector3(),9001);
            C(fontQueries==queries && captionChanges==captionsBefore);
            delete g_rootRef.get();C(liveWidgets==0);
        }
    }
    std::printf("%d actual info widget/WidgetRef lifecycle checks, %d failures\n",checks,failures);
    return failures?1:0;
}
