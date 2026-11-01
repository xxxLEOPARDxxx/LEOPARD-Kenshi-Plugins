#include "../../SavePreviewAlpha.h"
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cmath>
namespace MyGUI{struct Widget;}
struct WeakRef;
static std::vector<WeakRef*> refs;
static void Dispatch(MyGUI::Widget*);
namespace MyGUI
{
    struct Widget
    {
        float alpha,effective;bool inherits;Widget* parent;std::vector<Widget*> children,skin;
        Widget(float a=1,bool i=true):alpha(a),effective(a),inherits(i),parent(NULL){}
        float getAlpha()const{return alpha;}
        void setAlpha(float a){if(a!=alpha){alpha=a;Dispatch(this);}}
        bool getInheritsAlpha()const{return inherits;}
        void setInheritsAlpha(bool i){inherits=i;Dispatch(this);}
    };
}
struct WeakRef
{
    MyGUI::Widget* p;WeakRef():p(NULL){}void set(MyGUI::Widget*w){p=w;if(std::find(refs.begin(),refs.end(),this)==refs.end())refs.push_back(this);}
    MyGUI::Widget* get()const{return p;}void release(){refs.erase(std::remove(refs.begin(),refs.end(),this),refs.end());p=NULL;}
};
typedef SavePreviewAlpha<MyGUI::Widget,WeakRef> AlphaMask;
static AlphaMask* activeAlpha=NULL;static MyGUI::Widget* settingAlpha=NULL;
static void NativeAlpha(MyGUI::Widget*w)
{
    w->effective=w->alpha*((w->inherits&&w->parent)?w->parent->effective:1);
    for(size_t i=0;i<w->children.size();++i)Dispatch(w->children[i]);
    for(size_t i=0;i<w->skin.size();++i)Dispatch(w->skin[i]);
}
static void (*originalAlpha)(MyGUI::Widget*)=NativeAlpha;
// EXTRACT_ALPHA_HOOK
static void Dispatch(MyGUI::Widget*w){AlphaUpdate(w);}
static unsigned checks=0;
#define CHECK(x)do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
static void Add(MyGUI::Widget& parent,MyGUI::Widget& child,bool skin=false)
{child.parent=&parent;(skin?parent.skin:parent.children).push_back(&child);}
int main()
{
    MyGUI::Widget root(.75f),normal(.8f),independent(.65f,false),skin(.6f,false),leaf(.5f);
    Add(root,normal);Add(root,independent);Add(root,skin,true);Add(skin,leaf,true);NativeAlpha(&root);
    MyGUI::Widget* all[]={&root,&normal,&independent,&skin,&leaf};float old[5],effective[5];
    for(unsigned i=0;i<5;++i){old[i]=all[i]->alpha;effective[i]=all[i]->effective;}
    MyGUI::Widget* keyFocus=&independent;MyGUI::Widget* mouseFocus=&normal;
    {
        AlphaMask mask;activeAlpha=&mask;root.setInheritsAlpha(root.getInheritsAlpha());activeAlpha=NULL;
        CHECK(mask.Count()==5);for(unsigned i=0;i<5;++i)CHECK(all[i]->alpha==0&&all[i]->effective==0);
        CHECK(keyFocus==&independent&&mouseFocus==&normal&&!independent.inherits&&!skin.inherits);
        CHECK(mask.Restore());for(unsigned i=0;i<5;++i)CHECK(all[i]->alpha==old[i]&&fabs(all[i]->effective-effective[i])<.00001f);
    }
    CHECK(refs.empty()&&settingAlpha==NULL);
    {
        AlphaMask mask;activeAlpha=&mask;root.setInheritsAlpha(root.getInheritsAlpha());activeAlpha=NULL;
        for(size_t i=0;i<refs.size();++i)if(refs[i]->get()==&independent)refs[i]->p=NULL;
        root.children.erase(std::remove(root.children.begin(),root.children.end(),&independent),root.children.end());
        CHECK(mask.Restore()&&root.alpha==.75f&&skin.alpha==.6f); // removed widget weak ref is skipped
    }
    CHECK(refs.empty());
    MyGUI::Widget many;std::vector<MyGUI::Widget*> children;
    for(unsigned i=0;i<AlphaMask::Capacity+1;++i){MyGUI::Widget*w=new MyGUI::Widget(.5f,false);Add(many,*w);children.push_back(w);}
    {
        AlphaMask mask;bool caught=false;activeAlpha=&mask;
        try{many.setInheritsAlpha(true);}catch(const std::runtime_error&){caught=true;}activeAlpha=NULL;
        CHECK(caught&&settingAlpha==NULL&&mask.Count()==AlphaMask::Capacity);CHECK(mask.Restore());
        CHECK(many.alpha==1);for(size_t i=0;i<children.size();++i)CHECK(children[i]->alpha==.5f);
    }
    CHECK(refs.empty());for(size_t i=0;i<children.size();++i)delete children[i];
    std::printf("PASS actual modal alpha transaction/hook: %u checks; ordinary+skin opt-outs, weak lifetime, budget unwind\n",checks);return 0;
}
