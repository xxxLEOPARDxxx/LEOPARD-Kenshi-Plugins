#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <vector>
#include <set>
#include <cstdio>
#include <RestPolicy.h>
namespace Ogre{struct Vector3{float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}Vector3 operator-(const Vector3& p)const{return Vector3(x-p.x,y-p.y,z-p.z);}float squaredLength()const{return x*x+y*y+z*z;}};}
struct Building;struct Character;
struct hand
{
 int type;unsigned container,containerSerial,index,serial;
 hand():type(0),container(0),containerSerial(0),index(0),serial(0){}
 hand(Building*);bool isNull()const{return type==0;}
};
struct Relations{bool ally;Relations():ally(false){}bool isAlly(void*){return ally;}};
struct Faction{Relations* relations;Faction():relations(NULL){}};
struct TownBase{};
enum BuildingFunction{BF_BED=6,BF_GATE=21,BF_SKELETON_BED=25};
unsigned slotCalls=0,powerCalls=0;
struct UseableStuff{bool free,operable;UseableStuff():free(true),operable(true){}bool isFreeSlot(const hand&){++slotCalls;return free;}bool couldIOperate(const hand&){return operable;}};
struct Building
{
 unsigned serial;BuildingFunction function;bool furniture;Faction* owner;TownBase* town;Ogre::Vector3 position;UseableStuff* useable;float power;hand handle,isInsideBuilding;int floorNum;
 Building():serial(1),function(BF_BED),furniture(true),owner(NULL),town(NULL),useable(NULL),power(1),floorNum(0){handle=hand(this);}
 BuildingFunction getSpecialFunction(){return function;}bool isFurniture(){return furniture;}UseableStuff* getUseableStuff(){return useable;}
 Faction* getFaction(){return owner;}TownBase* getTown(){return town;}Ogre::Vector3 getPosition(){return position;}
};
hand::hand(Building* b):type(2),container(1),containerSerial(1),index((unsigned)(uintptr_t)b),serial(b?b->serial:0){}
struct Character{TownBase* town;Faction* faction;Ogre::Vector3 position;hand handle,isInsideBuilding;int floorNum;Character():town(NULL),faction(NULL),floorNum(0){}TownBase* getCurrentTownLocation(){return town;}Faction* getFaction(){return faction;}Ogre::Vector3 getPosition(){return position;}};
template<class T>struct lektor:std::vector<T>{};
lektor<Building*> scratch;
lektor<Building*>& Machines(){scratch.clear();return scratch;}
struct Blackboard{bool reserved;Blackboard():reserved(false){}int howManyGuysWorkingOnThisSubTarget(const hand&,const hand&){return reserved?1:0;}};
struct AI{lektor<Building*> beds;Blackboard board;bool fail;AI():fail(false){}void getAllLocalMachines(lektor<Building*>& list,unsigned){if(fail)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);list=beds;}Blackboard* getBlackboard(){return &board;}};
bool g_ownBedsOnly=true;int g_bedRadius=100;
float RepairRate(Building* b){++powerCalls;return b->power;}
unsigned errors=0;void ErrorLog(const char*){++errors;}
int NativeFaultFilter(unsigned code){return code==0xe06d7363?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER;}
// @PRODUCTION@
unsigned checks=0,failures=0;
void Check(bool value,const char* what){++checks;if(!value){++failures;printf("FAIL: %s\n",what);}}
int main()
{
 Faction mine,ally,neutral;Relations friends;friends.ally=true;ally.relations=&friends;
 TownBase town,other;Character c;c.faction=&mine;c.town=&town;
 AI ai;Building bed;UseableStuff useable;bed.owner=&mine;bed.town=&town;bed.useable=&useable;bed.position=Ogre::Vector3(99);ai.beds.push_back(&bed);
 Check(FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep)==&bed,"real ordinary bed selected inside configured radius");
 bed.position.x=101;slotCalls=powerCalls=0;
 Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep)&&slotCalls==0&&powerCalls==0,"distant bed rejected before native slot or power work");bed.position.x=99;
 bed.function=BF_GATE;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"barrel/wrong-function object rejected even if native cache contains it");bed.function=BF_BED;
 bed.furniture=false;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"non-furniture building rejected");bed.furniture=true;
 bed.owner=&ally;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"ownBedsOnly enforced by production selector");
 g_ownBedsOnly=false;Check(FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep)==&bed,"allied bed optional");
 bed.owner=&neutral;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"neutral bed never accepted");bed.owner=&mine;
 bed.town=&other;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"other-town bed rejected");bed.town=&town;
 bed.position.y=11;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"vertically separated bed rejected");bed.position.y=0;
 Building house;c.isInsideBuilding=bed.isInsideBuilding=hand(&house);bed.floorNum=1;
 Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"different floor in same building rejected");bed.floorNum=0;
 bed.useable=NULL;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"missing native usable object rejected");bed.useable=&useable;
 useable.free=false;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"occupied slot rejected");useable.free=true;
 useable.operable=false;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"inoperable modded bed rejected");useable.operable=true;
 ai.board.reserved=true;Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"native blackboard reservations respected");ai.board.reserved=false;
 ReservedSet reserved;reserved.insert(hand(&bed));Check(!FindRestBed(&ai,&c,&reserved,AIFixedRest::Sleep),"another owned episode cannot choose the same bed");
 bed.function=BF_SKELETON_BED;bed.power=0;Check(!FindRepairBed(&ai,&c,NULL),"unpowered repair bed rejected");bed.power=1;
 Check(FindRepairBed(&ai,&c,NULL)==&bed,"powered real repair bed accepted");
 Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"robotic bed not mistaken for biological sleep bed");
 bed.function=BF_BED;Building nearAlly;nearAlly.owner=&ally;nearAlly.town=&town;nearAlly.useable=&useable;nearAlly.position.x=1;ai.beds.push_back(&nearAlly);
 Check(FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep)==&bed,"own bed has priority over nearer allied bed");
 bed.town=NULL;c.town=NULL;ai.beds.pop_back();Check(FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep)==&bed,"own camp bed supported without a town pointer");
 const RestBedContext approach(&c);AIFixedRest::Bed current;bed.owner=&ally;
 Check(RestBedAllowed(&bed,approach,AIFixedRest::Sleep,current),"shared approach check accepts still-allied bed when permitted");
 bed.owner=&neutral;Check(!RestBedAllowed(&bed,approach,AIFixedRest::Sleep,current),"shared approach check rejects an ally becoming neutral");bed.owner=&mine;
 useable.operable=false;Check(!RestBedAllowed(&bed,approach,AIFixedRest::Sleep,current),"shared approach check rejects lost operating permission");useable.operable=true;
 bed.floorNum=1;Check(!RestBedAllowed(&bed,approach,AIFixedRest::Sleep,current),"shared approach check rejects changed floor");bed.floorNum=0;
 ai.beds.resize(4097,&bed);Check(!FindRestBed(&ai,&c,NULL,AIFixedRest::Sleep),"excessive native cached list declined");
 ai.fail=true;bool lookupFailed=false;Check(!SafeFindRestBed(&ai,&c,NULL,AIFixedRest::Sleep,&lookupFailed)&&errors==1&&lookupFailed,"actual lookup wrapper contains native fault and marks episode for suppression");
 printf("AIFixed extracted bed selector: %s (%u checks)\n",failures?"FAIL":"PASS",checks);return failures?1:0;
}
