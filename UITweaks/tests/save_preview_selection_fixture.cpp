#include <string>
#include <vector>
#include <cstdio>
#include <stdexcept>
namespace MyGUI
{
    struct Widget{bool visible;Widget():visible(true){}bool getVisible(){return visible;}bool getInheritedVisible(){return true;}};
    struct MultiListBox:Widget{size_t index;MultiListBox():index(0){}size_t getIndexSelected(){return index;}
        struct Event{template<class T>void operator+=(T){}}eventListChangePosition;};
    struct ListBox:Widget{};
    template<class T>T newDelegate(T value){return value;}
}
struct SaveInfo{std::string location,name;SaveInfo(const char*l,const char*n):location(l),name(n){}};
struct LoadSaveWindow{MyGUI::Widget*mMainWidget;MyGUI::MultiListBox*list;std::vector<SaveInfo>games;};
struct SaveManager{LoadSaveWindow*loadMenu;static SaveManager*getSingleton();};
static SaveManager manager;SaveManager*SaveManager::getSingleton(){return &manager;}
struct Ref{MyGUI::Widget*p;Ref():p(NULL){}MyGUI::Widget*get(){return p;}void set(MyGUI::Widget*w){p=w;}};
static Ref root,saveList;static bool enabled=true,selectionActive=false;
static unsigned draws=0,nativeShows=0,nativeInfos=0,subscriptions=0;static std::string selectedPath;
static void Hide(){selectionActive=false;selectedPath.clear();}
static void Cancel(){}static void ErrorLog(const std::string&){}
static void EnsurePanel(LoadSaveWindow*m){root.set(m->mMainWidget);}
static void LoadPreview(const std::string&p){selectedPath=p;}
static std::string FullPath(const std::string&p){return p;}
static void Tick(){if(selectionActive)++draws;}
static void SubscribeColumns(MyGUI::Widget*,unsigned&,unsigned){++subscriptions;}
static void NativeShow(SaveManager*){++nativeShows;}
static void NativeInfo(LoadSaveWindow*,size_t){++nativeInfos;}
static void (*originalShow)(SaveManager*)=NativeShow;static void (*originalInfo)(LoadSaveWindow*,size_t)=NativeInfo;
// EXTRACT_SELECTION
static unsigned checks=0;
#define CHECK(x)do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    MyGUI::Widget window;MyGUI::MultiListBox list;LoadSaveWindow menu;menu.mMainWidget=&window;menu.list=&list;
    menu.games.push_back(SaveInfo("C:/root-a","same"));menu.games.push_back(SaveInfo("D:/root-b","same"));manager.loadMenu=&menu;
    Show(&manager);CHECK(nativeShows==1&&subscriptions==1&&!selectionActive&&draws==0&&selectedPath.empty());
    Info(&menu,0);CHECK(nativeInfos==1&&!selectionActive&&draws==0&&selectedPath.empty());
    MouseActivated(NULL,999);CHECK(selectionActive&&selectedPath=="C:/root-a/same"&&draws==1); // same highlighted first row
    list.index=1;ListChanged(&list,1);CHECK(selectedPath=="D:/root-b/same"&&draws==2);
    Info(&menu,0);CHECK(selectedPath=="C:/root-a/same"); // native mapped index is authoritative for refresh
    list.index=size_t(-1);ListChanged(&list,list.index);CHECK(!selectionActive&&selectedPath.empty());
    list.index=0;Show(&manager);CHECK(!selectionActive&&draws==3&&selectedPath.empty());
    root.set(NULL);MouseActivated(NULL,0);CHECK(!selectionActive&&selectedPath.empty()); // menu already destroyed by earlier callback
    root.set(&window);window.visible=false;MouseActivated(NULL,0);CHECK(!selectionActive);window.visible=true;
    enabled=false;MouseActivated(NULL,0);CHECK(!selectionActive);enabled=true;
    std::printf("PASS actual save preview selection adapters: %u checks; initial/default/same-row/different-root/deletion\n",checks);return 0;
}
