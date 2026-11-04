#define _CRT_SECURE_NO_WARNINGS
#include "../../../GsmFineCoverage.h"
#include "../../../GsmCpuTiming.h"
#include <cstdio>
#include <string>
namespace Ogre {struct Vector3{float x,y,z;Vector3(float a,float b,float c):x(a),y(b),z(c){}};}
namespace MyGUI {
struct IntPoint{int left,top;IntPoint(int l=0,int t=0):left(l),top(t){}};
struct IntCoord{int left,top,width,height;IntCoord(int l=0,int t=0,int w=0,int h=0):left(l),top(t),width(w),height(h){}bool operator==(const IntCoord&o)const{return left==o.left&&top==o.top&&width==o.width&&height==o.height;}};
struct ImageBox{bool visible;float alpha;std::string texture;IntCoord coord;ImageBox():visible(true),alpha(1){}bool getVisible(){return visible;}float getAlpha(){return alpha;}const std::string&_getTextureName(){return texture;}const IntCoord&getCoord(){return coord;}};
}
struct MapScreen{MyGUI::IntPoint worldToMapCoords(const Ogre::Vector3&v){return MyGUI::IntPoint(int((v.x+147456)/256),int((v.z+147456)/256));}};
struct Entry{bool valid,opaque;struct{unsigned shaded;std::string name;}image;Entry():valid(true),opaque(true){image.shaded=1;}}fineCache[4];
struct WidgetRef{MyGUI::ImageBox*p;WidgetRef():p(NULL){}void*get(){return p;}}fineTiles[4];
struct Slots{bool bound[4];Slots(){for(unsigned n=0;n<4;++n)bound[n]=true;}int Find(unsigned n){return n<4&&bound[n]?int(n):-1;}}fineWidgetSlots;
MapScreen* fineMap=NULL;FineJob::Plan finePlan;FineJob::Demand currentDemand;bool fineCenter=false,context=true,geometry=true;unsigned finePhase=4;
bool debug=false;GsmCpuTiming::Stats cpuTiming;
static bool FineContext(){return context;}static bool FineGeometry(MapScreen*,FineJob::Demand&out){out=currentDemand;return geometry;}static int FineFind(const FineWindows::Key&key){return key.x<2&&key.z<2?int(key.z*2+key.x):-1;}
// EXTRACTED_VISITOR
// EXTRACTED_WRAPPER
static unsigned checks=0;static void C(bool value,const char*name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);throw 1;}std::printf("PASS %s\n",name);}
int main(){try{
 MapScreen first,second;fineMap=&first;finePlan.state=FineJob::Detail;finePlan.spacing=1;finePlan.count=4;finePlan.pairCount=2;FineJob::Demand&d=finePlan.demand;d.revision=2;d.scope.thread=GetCurrentThreadId();d.scope.world=0x100000001ULL;d.scope.sourceEpoch=0x200000002ULL;d.scope.job=0x300000003ULL;d.view.stamp.source=d.scope.sourceEpoch;d.view.stamp.material=7;d.view.world.x=d.view.world.z=-147456;d.view.world.w=d.view.world.h=512;d.view.screenWidth=d.view.screenHeight=512;currentDemand=d;
 MyGUI::ImageBox widgets[4];for(unsigned n=0;n<4;++n){FineWindows::Key key={1,n%2,n/2,d.view.stamp};finePlan.keys[n]=key;fineCache[n].image.name="correct";widgets[n].texture="correct";widgets[n].coord=MyGUI::IntCoord(n%2,n/2,1,1);fineTiles[n].p=&widgets[n];}
 C(FineCovers(&first),"actual wrapper accepts complete presented fine viewport");
 fineTiles[0].p=NULL;C(!FineCovers(&first),"missing actual widget resumes coarse shading");fineTiles[0].p=&widgets[0];fineWidgetSlots.bound[0]=false;C(!FineCovers(&first),"unbound pool slot resumes coarse shading");fineWidgetSlots.bound[0]=true;
 widgets[0].visible=false;C(!FineCovers(&first),"hidden actual widget resumes coarse shading");widgets[0].visible=true;widgets[0].texture="wrong";C(!FineCovers(&first),"wrong bound texture resumes coarse shading");widgets[0].texture="correct";
 widgets[0].alpha=.5f;C(!FineCovers(&first),"translucent actual widget resumes coarse shading");widgets[0].alpha=1;widgets[0].coord.left++;C(!FineCovers(&first),"stale actual projected rectangle resumes coarse shading");widgets[0].coord.left--;
 context=false;C(!FineCovers(&first),"actual context refusal precedes coverage proof");context=true;C(!FineCovers(&second)&&!FineCovers(NULL),"actual map identity and null checks precede coverage proof");
 currentDemand.view.world.x++;C(!FineCovers(&first),"actual current geometry change resumes coarse shading");currentDemand=d;geometry=false;C(!FineCovers(&first),"actual geometry read failure resumes coarse shading");geometry=true;fineCenter=true;C(!FineCovers(&first),"actual center-first state keeps fallback refreshing");fineCenter=false;finePhase=2;C(!FineCovers(&first),"actual drain state keeps fallback refreshing");finePhase=4;
 fineCache[0].opaque=false;C(!FineCovers(&first),"actual entry coverage flag checked");fineCache[0].opaque=true;fineCache[0].image.shaded=0;C(!FineCovers(&first),"actual entry must have successful shaded output");fineCache[0].image.shaded=1;fineCache[0].valid=false;C(!FineCovers(&first),"actual invalidated GPU entry checked");fineCache[0].valid=true;
 C(FineCovers(&first),"restored actual presentation permits coarse refresh suppression");std::printf("PASS %u extracted presentation controls\n",checks);return 0;
 }catch(...){return 1;}}
