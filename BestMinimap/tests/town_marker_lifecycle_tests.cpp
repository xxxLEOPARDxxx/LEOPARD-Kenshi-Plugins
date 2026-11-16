#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstdio>
typedef unsigned long DWORD;
struct TownBase{};
namespace MyGUI {
struct Widget {virtual ~Widget(){}};
struct ImageBox:Widget{};
struct Caption {std::string text;std::string asUTF8()const{return text;}};
struct Label:Widget {Caption caption;const Caption&getCaption()const{return caption;}};
struct IUnlinkWidget {virtual void _unlinkWidget(Widget*)=0;};
struct WidgetManager {
 std::vector<IUnlinkWidget*> refs;static bool available;
 static WidgetManager*getInstancePtr(){static WidgetManager m;return available?&m:NULL;}
 void registerUnlinker(IUnlinkWidget*p){refs.push_back(p);}
 void destroy(Widget*w){for(size_t i=0;i<refs.size();++i)refs[i]->_unlinkWidget(w);}
};bool WidgetManager::available=true;
}
struct hand {TownBase*t;hand(TownBase*p=NULL):t(p){}TownBase*getTown()const{return t;}bool operator<(const hand&o)const{return std::less<TownBase*>()(t,o.t);}};
template<class K,class V>struct ogre_unordered_map{typedef std::map<K,V>type;};
struct MapScreen {struct MapMarkerTown{MyGUI::ImageBox*image;MyGUI::Label*label;hand handle;};ogre_unordered_map<hand,MapMarkerTown*>::type mapMarkersTowns;void refreshTowns(){}};
bool g_townGameIcons=true;void DebugLog(const char*){}
#include "town_marker_runtime_under_test.h"
static MyGUI::ImageBox*Image(MyGUI::ImageBox*p){return p;}
static MyGUI::ImageBox*Image(MapScreen::MapMarkerTown*p){return p?p->image:NULL;}
#define C(x) do{if(!(x)){printf("CHECK line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){TownBase a,b;MyGUI::ImageBox oldImage,newImage,sharedImage;MyGUI::Label oldLabel,newLabel;oldLabel.caption.text="old";newLabel.caption.text="new";
 MapScreen::MapMarkerTown oldMarker={&oldImage,&oldLabel,hand(&a)},newMarker={&newImage,&newLabel,hand(&b)};MapScreen map;
 map.mapMarkersTowns[hand(&a)]=&oldMarker;MyGUI::WidgetManager::available=false;IndexMarkers(&map,500);C(g_markerByTown.empty()&&g_markerByName.empty()&&g_markersIndexedAt==0);MyGUI::WidgetManager::available=true;IndexMarkers(&map,1000);C(MyGUI::WidgetManager::getInstancePtr()->refs.size()==1);C(Image(g_markerByTown[&a])==&oldImage);
 // Native UI rebuild: old widgets disappear, new list has the same count.
 MyGUI::WidgetManager::getInstancePtr()->destroy(&oldImage);map.mapMarkersTowns.clear();map.mapMarkersTowns[hand(&b)]=&newMarker;IndexMarkers(&map,2000);
 C(g_markerByTown.find(&a)==g_markerByTown.end());C(Image(g_markerByTown[&b])==&newImage);C(g_markerByName.find("old")==g_markerByName.end());C(Image(g_markerByName["new"])==&newImage);
 // Cached identity must not depend on native marker-object lifetime.
 MapScreen::MapMarkerTown transient={&sharedImage,NULL,hand(&a)};map.mapMarkersTowns.clear();map.mapMarkersTowns[hand(&a)]=&transient;IndexMarkers(&map,13000);transient.image=NULL;C(Image(g_markerByTown[&a])==&sharedImage);
 // A different MapScreen with equal marker count is a different index.
 MapScreen other;other.mapMarkersTowns[hand(&b)]=&newMarker;IndexMarkers(&other,14000);C(g_markerByTown.find(&a)==g_markerByTown.end());C(Image(g_markerByTown[&b])==&newImage);
 MyGUI::ImageBox unrelated;MyGUI::WidgetManager::getInstancePtr()->destroy(&unrelated);C(Image(g_markerByTown[&b])==&newImage);
 MyGUI::WidgetManager::getInstancePtr()->destroy(&newImage);C(g_markerByTown.empty()&&g_markerByName.empty()&&g_markersIndexedAt==0);
 IndexMarkers(&map,15000);g_townGameIcons=false;IndexMarkers(&map,16000);C(g_markerByTown.empty()&&g_markerByName.empty()&&g_markersIndexedAt==0);g_townGameIcons=true;
 IndexMarkers(&map,17000);IndexMarkers(NULL,18000);C(g_markerByTown.empty()&&g_markerByName.empty()&&g_markersIndexedAt==0);
 C(MyGUI::WidgetManager::getInstancePtr()->refs.size()==1);
 puts("PASS town-marker widget lifetime / same-count rebuild / marker and map replacement");return 0;
}
