#include "head_boundary_fakes.h"
#include <algorithm>
namespace MyGUI {
typedef std::string UString;
struct IntCoord {
 int left,top,width,height;
 IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}
 bool operator!=(const IntCoord& o)const{return left!=o.left||top!=o.top||width!=o.width||height!=o.height;}
};
struct IntPoint {int left,top;IntPoint(int x=0,int y=0):left(x),top(y){}};
struct IntSize {int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct Align{enum{Default};bool isBottom()const{return false;}bool isVCenter()const{return true;}};
struct ISubWidgetText {
 char nativePrefix[0xe0];void*nativeFont;
 std::string fontName;UString caption;int fontHeight,fontWrites,heightWrites,measures,measuredHeight;bool hasFont,shadow;IntCoord coord;IntPoint absolute,viewOffset;
 ISubWidgetText():nativeFont(reinterpret_cast<void*>(1)),fontName("Regular"),fontHeight(20),fontWrites(0),heightWrites(0),measures(0),measuredHeight(-1),hasFont(true),shadow(false),coord(0,0,300,26){}
 template<class T>bool isType(){return true;}
 const std::string&getFontName(){if(!hasFont)abort();return fontName;}int getFontHeight(){return fontHeight;}
 void setFontName(const std::string&v){fontName=v;++fontWrites;}void setFontHeight(int v){fontHeight=v;++heightWrites;}
 IntSize getTextSize(){++measures;return IntSize(100,measuredHeight>=0?measuredHeight:fontHeight);}Align getTextAlign(){return Align();}
 const UString&getCaption()const{return caption;}bool getShadow()const{return shadow;}
 const IntCoord&getCoord(){return coord;}const IntPoint&getAbsolutePosition(){return absolute;}IntPoint getViewOffset(){return viewOffset;}
};
struct EditText:ISubWidgetText{};
struct Widget {
 IntCoord coord;Widget*parent,*client;bool visible,textEnabled,isEditBox,editWordWrap;int moves,clientReads;std::string caption;EditText text;
 Widget():coord(0,0,400,300),parent(NULL),client(NULL),visible(true),textEnabled(true),isEditBox(false),editWordWrap(false),moves(0),clientReads(0){}
 template<class T>T*castType(bool){return isEditBox?static_cast<T*>(this):NULL;}bool getEditWordWrap(){return editWordWrap;}
 Widget*getClientWidget(){++clientReads;return client;}ISubWidgetText*getSubWidgetText(){text.nativeFont=text.hasFont?reinterpret_cast<void*>(1):NULL;text.absolute.top=getAbsoluteTop()+text.coord.top;return textEnabled?&text:NULL;}
 const std::string&getFontName(){static std::string empty;return empty;}int getFontHeight(){return 0;}
 int getWidth(){return coord.width;}int getHeight(){return coord.height;}
 int getLeft(){return coord.left;}int getTop(){return coord.top;}int getBottom(){return coord.top+coord.height;}
 int getAbsoluteTop(){return coord.top+(parent?parent->getAbsoluteTop():0);}
 bool getVisible(){return visible;}Widget*getParent(){return parent;}
 IntCoord getCoord(){return coord;}
 void setCoord(IntCoord c){coord=c;++moves;}void setVisible(bool b){visible=b;}
 void setCaption(const std::string& c){caption=c;text.caption=c;}void setNeedMouseFocus(bool){}void setColour(Colour){}
 template<class T>T* createWidget(const std::string&,IntCoord c,int){T*p=new T;p->parent=this;p->coord=c;return p;}
};
typedef Widget EditBox;typedef Widget ProgressBar;
void UnlinkWidget(Widget*);
struct Gui{static Gui&getInstance(){static Gui g;return g;}void destroyWidget(Widget*w){UnlinkWidget(w);delete w;}};
}
struct WidgetRef;
std::vector<WidgetRef*>widgetRefs;
struct WidgetRef{MyGUI::Widget*w;WidgetRef():w(NULL){widgetRefs.push_back(this);}~WidgetRef(){widgetRefs.erase(std::find(widgetRefs.begin(),widgetRefs.end(),this));}MyGUI::Widget*get(){return w;}void set(MyGUI::Widget*p){w=p;}void reset(){w=NULL;}};
void MyGUI::UnlinkWidget(Widget*w){for(size_t i=0;i<widgetRefs.size();++i)if(widgetRefs[i]->w==w)widgetRefs[i]->w=NULL;}
#include "PanelTextStyle.h"
bool XpPanelTextStyle::g_nativeProfile=false;
struct DataPanelLine {
 enum{DPL_BASE,DPL_PROGRESS};MyGUI::Widget*w1,*w2;std::string keyValue;int type;
 DataPanelLine():w1(NULL),w2(NULL),type(DPL_BASE){}
 int getType(){return type;}
 virtual void setVisible(bool b){if(w1)w1->setVisible(b);if(w2)w2->setVisible(b);}
};
struct DataPanelLine_Progress:DataPanelLine {
 MyGUI::EditBox*textBox,*progressText;MyGUI::ProgressBar*progress;float value;int updates;
 DataPanelLine_Progress():textBox(NULL),progressText(NULL),progress(NULL),value(0),updates(0){type=DPL_PROGRESS;}
 void setProgress(float v,const std::string&t){value=v;progressText->caption=t;++updates;}
 virtual void setVisible(bool b){textBox->setVisible(b);progress->setVisible(b);progressText->setVisible(b);}
};
struct DatapanelGUI {
 MyGUI::Widget root;std::vector<DataPanelLine*>lines;std::map<std::string,DataPanelLine*>named;
 MyGUI::Widget*getWidget(){return &root;}int getHeight(){return root.getHeight();}
 int getNumLines(int){return (int)lines.size();}DataPanelLine*getLineByNum(int,int i){return lines[i];}
 bool lineExists(const std::string&k,int){return named.find(k)!=named.end();}
 DataPanelLine*getLine(const std::string&k,int){return named[k];}
 DataPanelLine_Progress*setLineProgress(const std::string&k,int,float v,const std::string&t,bool){
  DataPanelLine_Progress*r=new DataPanelLine_Progress;r->keyValue=k;
  r->textBox=new MyGUI::Widget;r->progress=new MyGUI::Widget;r->progressText=new MyGUI::Widget;
  r->w1=r->textBox;r->w2=r->progress;r->textBox->parent=r->progress->parent=r->progressText->parent=lines.empty()?&root:lines[0]->w1->parent;
  r->setProgress(v,t);lines.push_back(r);named[k]=r;return r;
 }
 void Vanilla(int top){DataPanelLine*r=new DataPanelLine;r->w1=new MyGUI::Widget;r->w2=new MyGUI::Widget;
  r->w1->coord=MyGUI::IntCoord(4,top,186,18);r->w2->coord=MyGUI::IntCoord(200,top,180,18);
  r->w1->parent=r->w2->parent=&root;lines.push_back(r);
 }
};
