#include "head_boundary_fakes.h"
#include <algorithm>
namespace MyGUI {
bool vanillaUiProfile=false;int widgetCreates=0,widgetDestroys=0,typeReads=0;
typedef std::string UString;
struct IntCoord {
 int left,top,width,height;
 IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}
 bool operator!=(const IntCoord& o)const{return left!=o.left||top!=o.top||width!=o.width||height!=o.height;}
 bool operator==(const IntCoord&o)const{return !(*this!=o);}int bottom()const{return top+height;}int right()const{return left+width;}
};
struct IntPoint {int left,top;IntPoint(int x=0,int y=0):left(x),top(y){}};
struct IntSize {int width,height;IntSize(int w=0,int h=0):width(w),height(h){}};
struct Align{enum{Default};int vertical;Align(int v=1):vertical(v){}bool isBottom()const{return vertical==2;}bool isVCenter()const{return vertical==1;}bool isTop()const{return vertical==0;}};
struct ISubWidgetText {
 char nativePrefix[0xe0];void*nativeFont;
 std::string fontName;UString caption;int fontHeight,fontWrites,heightWrites,measures,measuredHeight,vertical;bool hasFont,shadow;IntCoord coord;IntPoint absolute,viewOffset;
 ISubWidgetText():nativeFont(reinterpret_cast<void*>(1)),fontName("Regular"),fontHeight(20),fontWrites(0),heightWrites(0),measures(0),measuredHeight(-1),vertical(1),hasFont(true),shadow(false),coord(0,0,300,26){}
 template<class T>bool isType(){return true;}
 const std::string&getFontName(){if(!hasFont)abort();return fontName;}int getFontHeight(){return fontHeight;}
 void setFontName(const std::string&v){fontName=v;++fontWrites;}void setFontHeight(int v){fontHeight=v;++heightWrites;}
 IntSize getTextSize(){++measures;return IntSize(100,measuredHeight>=0?measuredHeight:fontHeight);}Align getTextAlign(){return Align(vertical);}
 const UString&getCaption()const{return caption;}bool getShadow()const{return shadow;}
 const IntCoord&getCoord(){return coord;}const IntPoint&getAbsolutePosition(){return absolute;}IntPoint getViewOffset(){return viewOffset;}
};
struct EditText:ISubWidgetText{};
struct Widget;typedef std::vector<Widget*>VectorWidgetPtr;
struct Widget {
 IntCoord coord;Widget*parent,*client;bool visible,textEnabled,isEditBox,editWordWrap,stretch,stretchText,needMouse,needKey,inheritsPick;int moves,clientReads,skinQueries;std::string caption,skinName,skinResource,typeName;EditText text;VectorWidgetPtr children,skinChildren;
 Widget():coord(0,0,400,300),parent(NULL),client(NULL),visible(true),textEnabled(true),isEditBox(false),editWordWrap(false),stretch(false),stretchText(false),needMouse(true),needKey(true),inheritsPick(true),moves(0),clientReads(0),skinQueries(0),typeName("TextBox"){}
 const std::string&getTypeName(){++typeReads;return typeName;}
 size_t getChildCount(){return children.size();}Widget*getChildAt(size_t n){return children[n];}
 VectorWidgetPtr getSkinWidgetsByName(const std::string&name){++skinQueries;VectorWidgetPtr result;for(size_t n=0;n<skinChildren.size();++n)if(skinChildren[n]->skinName==name)result.push_back(skinChildren[n]);return result;}
 template<class T>T*castType(bool){return isEditBox?static_cast<T*>(this):NULL;}bool getEditWordWrap(){return editWordWrap;}
 Widget*getClientWidget(){++clientReads;return client;}ISubWidgetText*getSubWidgetText(){text.nativeFont=text.hasFont?reinterpret_cast<void*>(1):NULL;text.absolute.top=getAbsoluteTop()+text.coord.top;return textEnabled?&text:NULL;}
 const std::string&getFontName(){static std::string empty;return empty;}int getFontHeight(){return 0;}
 int getWidth(){return coord.width;}int getHeight(){return coord.height;}
 int getLeft(){return coord.left;}int getTop(){return coord.top;}int getBottom(){return coord.top+coord.height;}
 int getAbsoluteTop(){return coord.top+(parent?parent->getAbsoluteTop():0);}
 bool getVisible(){return visible;}Widget*getParent(){return parent;}
 IntCoord getCoord(){return coord;}
 void setCoord(IntCoord c){const int dx=c.width-coord.width,dy=c.height-coord.height;if(stretchText){text.coord.width+=dx;text.coord.height+=dy;}for(size_t n=0;n<children.size();++n)if(children[n]->stretch){IntCoord a=children[n]->coord;a.width+=dx;a.height+=dy;children[n]->setCoord(a);}for(size_t n=0;n<skinChildren.size();++n)if(skinChildren[n]->stretch){IntCoord a=skinChildren[n]->coord;a.width+=dx;a.height+=dy;skinChildren[n]->setCoord(a);}coord=c;++moves;}void setVisible(bool b){visible=b;}
 void setCaption(const std::string& c){caption=c;text.caption=c;if(client)client->setCaption(c);}void setNeedMouseFocus(bool v){needMouse=v;}void setNeedKeyFocus(bool v){needKey=v;}void setInheritsPick(bool v){inheritsPick=v;}void setEditStatic(bool){}void setEditWordWrap(bool v){editWordWrap=v;}void setColour(Colour){}
 Widget*createWidgetT(const std::string&type,const std::string&skin,IntCoord c,int,const std::string&name=""){
  Widget*p=new Widget;++widgetCreates;p->typeName=type;p->skinResource=skin;p->skinName=name;p->coord=c;p->parent=this;children.push_back(p);
  p->stretchText=true;p->text.coord=IntCoord(0,0,c.width,c.height);
  if(type=="EditBox"){
   p->isEditBox=true;p->editWordWrap=true;p->textEnabled=false;Widget*q=new Widget;++widgetCreates;
   const int inset=skin=="InsetProfile"?2:0;q->coord=IntCoord(0,inset,c.width,c.height-inset*2);q->parent=p;q->stretch=true;q->stretchText=true;
   q->text.coord=IntCoord(0,0,c.width,c.height-inset*2);q->text.vertical=0;q->text.shadow=!vanillaUiProfile;p->client=q;p->children.push_back(q);
  }return p;
 }
 template<class T>T* createWidget(const std::string&skin,IntCoord c,int){T*p=new T;++widgetCreates;p->parent=this;p->coord=c;p->skinResource=skin;children.push_back(p);return p;}
};
typedef Widget TextBox;typedef Widget EditBox;typedef Widget ProgressBar;
void UnlinkWidget(Widget*);
struct Gui{static Gui&getInstance(){static Gui g;return g;}void destroyWidget(Widget*w){while(!w->children.empty())destroyWidget(w->children.back());if(w->parent){VectorWidgetPtr&v=w->parent->children;v.erase(std::remove(v.begin(),v.end(),w),v.end());}UnlinkWidget(w);++widgetDestroys;delete w;}};
}
struct WidgetRef;
std::vector<WidgetRef*>widgetRefs;
struct WidgetRef{MyGUI::Widget*w;WidgetRef():w(NULL){widgetRefs.push_back(this);}~WidgetRef(){widgetRefs.erase(std::find(widgetRefs.begin(),widgetRefs.end(),this));}MyGUI::Widget*get(){return w;}void set(MyGUI::Widget*p){w=p;}void reset(){w=NULL;}};
void MyGUI::UnlinkWidget(Widget*w){for(size_t i=0;i<widgetRefs.size();++i)if(widgetRefs[i]->w==w)widgetRefs[i]->w=NULL;}
#include "PanelTextStyle.h"
bool XpPanelTextStyle::g_nativeProfile=false;
struct DataPanelLine {
 enum{DPL_BASE,DPL_PROGRESS};MyGUI::Widget*w1,*w2;std::string keyValue,skinW1,skinW2;int type;
 DataPanelLine():w1(NULL),w2(NULL),skinW1("Kenshi_PaintedWordWrapEmpty"),skinW2("Kenshi_WordWrapEmpty"),type(DPL_BASE){}
 int getType(){return type;}
 virtual void setVisible(bool b){if(w1)w1->setVisible(b);if(w2)w2->setVisible(b);}
};
struct DataPanelLine_Progress:DataPanelLine {
 MyGUI::EditBox*textBox,*progressText;MyGUI::ProgressBar*progress;float value;int updates;
 DataPanelLine_Progress():textBox(NULL),progressText(NULL),progress(NULL),value(0),updates(0){type=DPL_PROGRESS;}
 void setProgress(float v,const std::string&t){value=v;progressText->setCaption(t);++updates;}
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
  r->textBox->skinResource="Kenshi_GenericTextBoxFlat";r->textBox->stretchText=true;
  r->textBox->text.coord=MyGUI::vanillaUiProfile?MyGUI::IntCoord(17,9,365,280):MyGUI::IntCoord(0,0,400,300);
  r->progressText->text.vertical=0;r->progressText->text.shadow=!MyGUI::vanillaUiProfile;r->progressText->stretchText=true;r->progressText->text.coord=MyGUI::IntCoord(0,0,400,300);
  const int w=r->progress->getWidth(),h=r->progress->getHeight();
  const MyGUI::IntCoord skin[]={MyGUI::vanillaUiProfile?MyGUI::IntCoord(13,2,w-25,h-7):MyGUI::IntCoord(10,2,w-19,h-5),MyGUI::vanillaUiProfile?MyGUI::IntCoord(14,4,w-28,h-9):MyGUI::IntCoord(11,3,w-21,h-7),MyGUI::IntCoord(0,0,w,h)};
  for(int i=0;i<3;++i){MyGUI::Widget*c=new MyGUI::Widget;c->coord=skin[i];c->parent=r->progress;c->stretch=true;if(i==1)c->skinName="TrackPlace";r->progress->skinChildren.push_back(c);}
  r->w1=r->textBox;r->w2=r->progress;r->textBox->parent=r->progress->parent=r->progressText->parent=lines.empty()?&root:lines[0]->w1->parent;
  r->setProgress(v,t);lines.push_back(r);named[k]=r;return r;
 }
 void Vanilla(int top){DataPanelLine*r=new DataPanelLine;r->w1=new MyGUI::Widget;r->w2=new MyGUI::Widget;
  r->w1->coord=MyGUI::IntCoord(4,top,186,18);r->w2->coord=MyGUI::IntCoord(200,top,180,18);
  r->w1->parent=r->w2->parent=&root;lines.push_back(r);
 }
};
