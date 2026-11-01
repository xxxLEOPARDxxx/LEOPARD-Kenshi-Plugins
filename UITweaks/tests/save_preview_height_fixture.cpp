#include "../../SavePreviewPolicy.h"
#include <cstdio>
#include <string>
namespace MyGUI
{
    struct IntSize {int width,height;};
    struct Align {bool stretch;bool isVStretch(){return stretch;}};
    struct Widget
    {
        int width,height,writes;Widget* parent;Align align;
        Widget(int h=325):width(680),height(h),writes(0),parent(NULL){align.stretch=true;}
        int getHeight(){return height;}int getWidth(){return width;}
        Widget* getParent(){return parent;}Align getAlign(){return align;}
        void setSize(int w,int h){width=w;height=h;++writes;}
    };
    struct RenderManager {static RenderManager&getInstance(){static RenderManager manager;return manager;}IntSize getViewSize(){IntSize s={1920,1080};return s;}};
}
struct LoadSaveWindow{MyGUI::Widget*mMainWidget,*list;};
struct Ref{MyGUI::Widget*p;Ref():p(NULL){}MyGUI::Widget*get(){return p;}void set(MyGUI::Widget*w){p=w;}};
static Ref sizedRoots[2];static SavePreviewPolicy::DialogHeightState dialogHeights[2];
static int dialogHeightPercent=120;static bool enabled=true;static unsigned errors=0;
static void ErrorLog(const std::string&){++errors;}
// EXTRACT_HEIGHT
static unsigned checks=0;
#define CHECK(x)do{++checks;if(!(x)){std::printf("FAIL line%u: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    MyGUI::Widget loadRoot,saveRoot,listA,listB;listA.parent=&loadRoot;listB.parent=&saveRoot;
    LoadSaveWindow load={&loadRoot,&listA},save={&saveRoot,&listB};
    SizeMenu(&load);CHECK(loadRoot.height==390&&loadRoot.writes==1);
    SizeMenu(&save,true);CHECK(saveRoot.height==390&&saveRoot.writes==1);
    SizeMenu(&load);SizeMenu(&save,true);CHECK(loadRoot.height==390&&saveRoot.height==390&&loadRoot.writes==1&&saveRoot.writes==1);
    dialogHeightPercent=100;SizeMenu(&load);CHECK(loadRoot.height==325&&saveRoot.height==390); // next-open, per root
    dialogHeightPercent=120;SizeMenu(&load);CHECK(loadRoot.height==390);
    loadRoot.height=400;SizeMenu(&load);CHECK(loadRoot.height==480); // external/native resize becomes base
    sizedRoots[0].p=NULL;loadRoot.height=480;SizeMenu(&load);CHECK(loadRoot.height==576); // destroyed/recreated at same address
    listA.align.stretch=false;dialogHeightPercent=160;int before=loadRoot.writes;SizeMenu(&load);
    CHECK(errors==1&&loadRoot.writes==before&&loadRoot.height==576); // no empty frame growth in unsupported layout
    listA.align.stretch=true;listA.parent=&saveRoot;SizeMenu(&load);CHECK(errors==2&&loadRoot.writes==before); // ownership guard
    enabled=false;dialogHeightPercent=120;SizeMenu(&save,true);CHECK(saveRoot.height==390); // height setting independent of capture
    SizeMenu(NULL);CHECK(errors==2);
    std::printf("PASS actual save/load height adapter: %u checks; separate roots, no compounding, next-open, foreign layout\n",checks);return 0;
}
