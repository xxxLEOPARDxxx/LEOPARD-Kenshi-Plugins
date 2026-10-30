#include "head_boundary_fakes.h"
#include <algorithm>
namespace MyGUI {
struct IntCoord {
 int left,top,width,height;
 IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}
 bool operator!=(const IntCoord& o)const{return left!=o.left||top!=o.top||width!=o.width||height!=o.height;}
};
struct Align{enum{Default};};
struct Widget {
 IntCoord coord;Widget*parent;bool visible;int moves;std::string caption;
 Widget():coord(0,0,400,300),parent(NULL),visible(true),moves(0){}
 int getWidth(){return coord.width;}int getHeight(){return coord.height;}
 int getLeft(){return coord.left;}int getTop(){return coord.top;}int getBottom(){return coord.top+coord.height;}
 bool getVisible(){return visible;}Widget*getParent(){return parent;}
 IntCoord getCoord(){return coord;}
 void setCoord(IntCoord c){coord=c;++moves;}void setVisible(bool b){visible=b;}
 void setCaption(const std::string& c){caption=c;}void setNeedMouseFocus(bool){}void setColour(Colour){}
 template<class T>T* createWidget(const std::string&,IntCoord c,int){T*p=new T;p->parent=this;p->coord=c;return p;}
};
typedef Widget EditBox;typedef Widget ProgressBar;
struct Gui{static Gui&getInstance(){static Gui g;return g;}void destroyWidget(Widget*w){delete w;}};
}
struct WidgetRef{MyGUI::Widget*w;WidgetRef():w(NULL){}MyGUI::Widget*get(){return w;}void set(MyGUI::Widget*p){w=p;}void reset(){w=NULL;}};
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
