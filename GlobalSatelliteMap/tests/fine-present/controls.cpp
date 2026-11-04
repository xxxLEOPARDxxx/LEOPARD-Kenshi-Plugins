#define _CRT_SECURE_NO_WARNINGS
#include "../../../atlas-terrain/fine-viewer/FineJob.h"
#include "../../../GsmFineIndex.h"
#include "../../../GsmFineWidgetSlots.h"
#include "../../../GsmCpuTiming.h"
#include <cstdio>
#include <string>
#include <stdexcept>
#include <new>
#include <cstdlib>
static unsigned projections=0,writes=0,depthWrites=0,creates=0,checks=0,allocations=0;static bool throwProjection=false;static void(*onCoord)()=NULL;
void*operator new(size_t n){++allocations;void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}void*operator new[](size_t n){++allocations;void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete(void*p){std::free(p);}void operator delete[](void*p){std::free(p);}
namespace Ogre{struct Vector3{float x,y,z;Vector3(float a,float b,float c):x(a),y(b),z(c){}};}
namespace MyGUI {
struct IntPoint{int left,top;IntPoint(int a=0,int b=0):left(a),top(b){}};
struct IntCoord{int left,top,width,height;IntCoord(int a=0,int b=0,int c=0,int d=0):left(a),top(b),width(c),height(d){}bool operator==(const IntCoord&o)const{return left==o.left&&top==o.top&&width==o.width&&height==o.height;}bool operator!=(const IntCoord&o)const{return !(*this==o);}};
namespace Align{enum Enum{Default};}
struct Widget{IntCoord coord;Widget*parent;bool visible,alive;int depth;std::string texture;Widget():parent(NULL),visible(false),alive(true),depth(0){}void Touch(){if(!alive)throw std::runtime_error("dead widget dereference");}int getWidth(){Touch();return coord.width;}int getHeight(){Touch();return coord.height;}IntCoord getCoord(){Touch();return coord;}IntCoord getAbsoluteCoord(){Touch();return coord;}Widget*getParent(){Touch();return parent;}bool getVisible(){Touch();return visible;}int getDepth(){Touch();return depth;}const std::string&_getTextureName(){Touch();return texture;}void setCoord(IntCoord c){Touch();coord=c;++writes;if(onCoord)onCoord();}void setVisible(bool v){Touch();visible=v;++writes;}void setDepth(int d){Touch();depth=d;++writes;++depthWrites;}void setImageTexture(const std::string&s){Touch();texture=s;++writes;}void setNeedMouseFocus(bool){}void setNeedKeyFocus(bool){}template<class T>T*createWidget(const char*,IntCoord,Align::Enum,const char*);};
typedef Widget ImageBox;Widget created[GsmFineWidgets::MaxWidgets];template<class T>T*Widget::createWidget(const char*,IntCoord,Align::Enum,const char*){if(creates>=GsmFineWidgets::MaxWidgets)throw 1;Widget*w=&created[creates++];*w=Widget();w->parent=this;return static_cast<T*>(w);}
}
struct WidgetRef{MyGUI::Widget*p;WidgetRef():p(NULL){}MyGUI::Widget*get(){return p&&p->alive?p:NULL;}void set(MyGUI::Widget*w){p=w;}void reset(){if(p)p->alive=false;p=NULL;}};
struct MapScreen{MyGUI::Widget*mainWidget,*mapImage,*mapScrollView;MyGUI::IntPoint worldToMapCoords(const Ogre::Vector3&p){++projections;if(throwProjection)throw std::runtime_error("controlled projection failure");return MyGUI::IntPoint(int((p.x+147456)*mapImage->getWidth()/294912),int((p.z+147456)*mapImage->getHeight()/294912));}};
struct FineEntry{bool valid;FineWindows::Key key;struct{unsigned shaded;std::string name;}image;FineEntry():valid(false){image.shaded=0;}}fineCache[GsmCacheProfile::MaxFineCacheSlots];
WidgetRef layer,fineTiles[GsmFineWidgets::MaxWidgets];GsmFineWidgets::Slots fineWidgetSlots;GsmFineIndex::Index fineIndex;unsigned fineVisible[GsmFineIndex::MaxVisibleIndices],fineVisibleCount=0;FineWindows::U64 fineVisibleDemand=0,fineVisibleIndex=0;FineJob::Demand fineDemand={};MapScreen*fineMap=NULL;bool fineDeferPaint=false,debug=false,context=true;GsmCpuTiming::Stats cpuTiming;
namespace GsmFinePolicy{inline bool Intersects(const FineWindows::Key&key,const FineWindows::Request&view){FineWindows::Rect b;return FineWindows::Same(key.stamp,view.stamp)&&FineWindows::Bounds(key,b)&&b.x<view.world.x+view.world.w&&view.world.x<b.x+b.w&&b.z<view.world.z+view.world.h&&view.world.z<b.z+b.h;}}
bool FineContext(){return context;}
// EXTRACTED_CACHE
// EXTRACTED_FUNCTIONS
static void C(bool v,const char*s){++checks;if(!v){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}std::printf("PASS %s\n",s);}
static void Reenter(){FinePresent(fineMap);}
static void Add(unsigned slot,unsigned x,unsigned z,unsigned shaded=1){FineEntry&f=fineCache[slot];f.valid=true;FineWindows::Key key={1,x,z,fineDemand.view.stamp};f.key=key;f.image.shaded=shaded;char name[20];std::sprintf(name,"tile%u",slot);f.image.name=name;C(fineIndex.Insert(key,slot),"insert production index entry");}
int main(){try{
 MapScreen map,other;MyGUI::Widget main,image,view,overlay,marker;main.visible=image.visible=view.visible=overlay.visible=marker.visible=true;image.coord=MyGUI::IntCoord(0,0,294912,294912);view.coord=MyGUI::IntCoord(0,0,512,512);overlay.parent=&image;map.mainWidget=&main;map.mapImage=&image;map.mapScrollView=&view;other=map;layer.set(&overlay);fineMap=&map;fineDemand.revision=1;fineDemand.view.stamp.source=1;fineDemand.view.stamp.material=2;fineDemand.view.world.x=fineDemand.view.world.z=-147456;fineDemand.view.world.w=fineDemand.view.world.h=512;fineDemand.view.screenWidth=fineDemand.view.screenHeight=512;
 Add(0,0,0);Add(1,1,0);Add(2,0,1);Add(3,1,1,0);FinePresent(&map);C(creates==4&&fineTiles[0].get()->visible&&!fineTiles[3].get()->visible,"actual first pass presents shaded entries only");
 projections=writes=depthWrites=0;unsigned priorAllocations=allocations;for(unsigned n=0;n<100;++n)FinePresent(&map);C(!projections&&!writes,"unchanged100 frames perform zero native projection and UI writes");C(allocations==priorAllocations,"stable presentation path allocates no heap");
 fineCache[0].image.shaded++;FinePresent(&map);C(!projections&&!writes,"new lighting epoch keeps texture binding and geometry unchanged");
 fineCache[3].image.shaded=1;FinePresent(&map);C(fineTiles[3].get()->visible&&projections==2,"newly shaded hidden entry projects once and becomes visible");C(depthWrites==1,"newly ready texture changes depth at most once");
 projections=writes=0;image.coord.width/=2;image.coord.height/=2;FinePresent(&map);C(projections==8&&fineTiles[0].get()->coord.width==128,"canvas resize refreshes all exact native projections");
 projections=0;view.coord.width++;FinePresent(&map);C(projections==8,"viewport-only resize invalidates native projection token");
 projections=0;onCoord=&Reenter;image.coord.width+=256;FinePresent(&map);onCoord=NULL;C(projections==8,"reentrant UI callback cannot duplicate active presentation pass");
 image.coord.width+=256;throwProjection=true;bool threw=false;try{FinePresent(&map);}catch(const std::exception&){threw=true;}throwProjection=false;projections=0;FinePresent(&map);C(threw&&projections==8,"projection exception releases guard and permits complete retry");
 projections=writes=0;fineTiles[0].get()->coord.left++;FinePresent(&map);C(!projections&&writes==1&&fineTiles[0].get()->coord.left==0,"externally changed coordinate repaired from cached native rectangle");
 context=false;projections=0;FinePresent(&map);C(!projections&&!fineTiles[0].get()->visible&&marker.visible,"context loss hides only fine widgets without touching markers");context=true;FinePresent(&map);C(fineTiles[0].get()->visible&&marker.visible,"same-scope context restoration shows existing tiles");
 projections=writes=0;FinePresent(&other);C(!projections&&!fineTiles[0].get()->visible&&marker.visible,"foreign map is never used to project current fine widgets");FinePresent(&map);
 MyGUI::Widget foreign;overlay.parent=&foreign;projections=writes=0;FinePresent(&map);C(!projections&&!writes,"foreign layer owner receives no native/UI mutation");overlay.parent=&image;FinePresent(&map);
 projections=0;++fineDemand.revision;fineDemand.view.world.x+=256;FinePresent(&map);C(projections==4&&!fineTiles[0].get()->visible,"pan detaches offscreen widgets and refreshes remaining projections");
 FineDetachWidget(1);C(fineIndex.Erase(fineCache[1].key,1),"retired index entry removed");fineCache[1].valid=false;FinePresent(&map);C(fineWidgetSlots.Find(1)<0,"retired slot never remains bound");Add(1,2,0);FinePresent(&map);C(fineWidgetSlots.Find(1)>=0&&fineTiles[fineWidgetSlots.Find(1)].get()->visible,"reused cache slot gets correct fresh widget binding");
 int pool=fineWidgetSlots.Find(1);fineTiles[pool].get()->alive=false;projections=0;FinePresent(&map);C(fineTiles[pool].get()&&fineTiles[pool].get()->visible&&projections==2,"destroyed widget reference recreates safely at unchanged demand");
 // A real world/layer teardown invalidates only copied presentation metadata.
 for(unsigned n=0;n<GsmFineWidgets::MaxWidgets;++n)fineTiles[n].reset();fineWidgetSlots.Clear();fineIndex.Clear();fineVisibleDemand=fineVisibleIndex=0;
 // RESET_CACHE
 MyGUI::Widget freshOverlay;freshOverlay.parent=&image;freshOverlay.visible=true;layer.set(&freshOverlay);Add(5,1,0);FinePresent(&map);C(fineWidgetSlots.Find(5)>=0&&fineTiles[fineWidgetSlots.Find(5)].get()->visible,"new world/layer with same dimensions rebinds fresh entries");
 std::printf("PASS %u actual FinePresent controls\n",checks);return 0;}catch(...){return 1;}}
