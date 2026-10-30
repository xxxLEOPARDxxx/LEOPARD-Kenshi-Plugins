#include <cstdio>
#include <vector>
namespace Ogre { struct Vector3 {float x;Vector3(float a=0):x(a){} float squaredDistance(const Vector3& b)const{return (x-b.x)*(x-b.x);} }; }
enum {CHARACTER=1,BUILDING=2};
struct ActivePlatoon;
struct RootObject {int type;RootObject():type(CHARACTER){} virtual ~RootObject(){} int getDataType(){return type;} };
struct Character:RootObject {
 bool dead,player,animal,unique,trader;int platoonReads,deadReads;ActivePlatoon* platoon;Ogre::Vector3 pos;
 Character():dead(false),player(false),animal(false),unique(false),trader(false),platoonReads(0),deadReads(0),platoon(0){}
 Ogre::Vector3 getPosition(){return pos;}bool isDead(){++deadReads;return dead;}bool isPlayerCharacter(){return player;}
 void* isAnimal(){return animal?this:0;}ActivePlatoon* getPlatoon(){++platoonReads;return platoon;}
};
struct Roster {std::vector<RootObject*> data;int forcedSize,reads;Roster():forcedSize(-1),reads(0){}
 int size()const{return forcedSize<0?(int)data.size():forcedSize;}RootObject* operator[](int i){++reads;return data[i];}};
struct ActivePlatoon {Character* squadleader;Roster things;ActivePlatoon():squadleader(0){} };
bool g_showLeaders=true,g_playerLeaders=true,g_showAnimals=true,g_showUnique=false,g_showGeneric=false,g_showTraders=true;
float g_maxDistance=150;const char* g_traderKey="is trader";
bool TemplateUnique(Character*c){return c->unique;}bool TemplateFlag(Character*c,const char*){return c->trader;}
#define EXCEPTION_EXECUTE_HANDLER 1
#include "leader_filter_under_test.h"
int checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::printf("FAIL %s\n",name);}}
bool leader(Character& c){bool u=false,r=false,t=false,l=false;float d=0;SafeFilter(&c,Ogre::Vector3(),u,r,t,l,d);return l;}
void join(ActivePlatoon&p,Character&c){c.platoon=&p;p.things.data.push_back(&c);}
int main(){
 ActivePlatoon p;Character c,other;join(p,c);p.squadleader=&c;
 check(!leader(c),"singleton is not a meaningful squad leader");
 c.trader=true;check(!leader(c),"solo merchant retains coin eligibility without star");c.trader=false;
 g_showGeneric=true;check(!leader(c),"solo recruit eligibility cannot turn into a leader star");g_showGeneric=false;
 join(p,other);check(leader(c),"real leader with one living follower is shown");
 check(!leader(other),"ordinary member is not a leader");
 other.dead=true;check(!leader(c),"dead follower does not turn singleton into useful squad");other.dead=false;
 ActivePlatoon foreign;other.platoon=&foreign;check(!leader(c),"stale member from another squad does not count");other.platoon=&p;
 p.things.data[1]=&c;check(!leader(c),"duplicate self pointer is not another person");p.things.data[1]=&other;
 p.things.data[1]=0;check(!leader(c),"null roster slot does not count");p.things.data[1]=&other;
 other.type=BUILDING;check(!leader(c),"non-character object does not count");other.type=CHARACTER;
 c.player=true;g_playerLeaders=false;int before=p.things.reads;check(!leader(c)&&p.things.reads==before,"player exclusion avoids roster walk");g_playerLeaders=true;
 check(leader(c),"player leader with living squad remains supported");c.player=false;
 g_showLeaders=false;int oldReads=c.platoonReads;check(!leader(c)&&c.platoonReads==oldReads,"disabled stars do not query platoon");g_showLeaders=true;
 before=p.things.reads;check(!leader(other)&&p.things.reads==before,"non-leaders do not scan roster");
 p.things.forcedSize=2048;before=p.things.reads;check(!leader(c)&&p.things.reads==before,"malformed unbounded roster fails closed");p.things.forcedSize=-1;
 p.things.data.clear();join(p,other);Character second;join(p,second);check(!leader(c),"leader must itself be in the active roster");
 p.things.data.clear();join(p,c);join(p,other);before=p.things.reads;for(int i=0;i<100;++i)check(leader(c),"stable live squad remains marked");
 check(p.things.reads-before<=200,"normal two-member squad early-exits after two entries");
 std::printf("%d actual leader filter checks, %d failures\n",checks,failures);return failures?1:0;
}
