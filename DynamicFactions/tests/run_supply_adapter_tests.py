"""Exercise extracted production routing, hunger scope and SEH boundaries."""
from pathlib import Path
import os
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root.parents[1] / 'build/DynamicFactionsSupplyCheck'
out.mkdir(parents=True, exist_ok=True)

def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

provision = (root / 'game/DfProvision.cpp').read_text(encoding='utf-8-sig')
sell = (root / 'game/DfSell.cpp').read_text(encoding='utf-8-sig')
labour = (root / 'game/DfLabour.cpp').read_text(encoding='utf-8-sig')
mine = (root / 'game/DfMine.cpp').read_text(encoding='utf-8-sig')
hunger = (root / 'game/DfHunger.cpp').read_text(encoding='utf-8-sig')
state = (root / 'game/DfProvision.h').read_text(encoding='utf-8-sig')
start = state.index('struct DfProvisionPlan\n') if 'struct DfProvisionPlan\n' in state else state.index('struct DfProvision\n')
end = state.index('\n};', state.index('struct DfProvision\n')) + len('\n};')
code = r'''
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdio>
#include <climits>
#include <float.h>
#include "core/DfSupply.h"
#include "core/DfOffer.h"
namespace Ogre {struct Vector3 {float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
float squaredDistance(const Vector3& b)const{float a=x-b.x,c=y-b.y,d=z-b.z;return a*a+c*c+d*d;}};}
struct Character;
std::map<int,Character*> actors;
struct hand{int type,container,containerSerial,index,serial;
hand(int i=0):type(1),container(0),containerSerial(0),index(i),serial(0){}
bool isNull()const{return index==0;}Character* getCharacter()const;};
struct DfWorkHandLess {bool operator()(const hand& a,const hand& b)const{return a.index<b.index || (a.index==b.index && a.serial<b.serial);}};
struct GameData {std::string stringID;GameData(const char* s):stringID(s){}};
struct Ownerships {int money;Ownerships(int n):money(n){}};
struct Faction {GameData* data;Ownerships* factionOwnerships;};
struct RaceData {bool robot;float hungerRate;RaceData():robot(false),hungerRate(1){}};
struct Platoon {Faction* f;float malnourishedLevel;Faction* getFaction(){return f;}};
struct ActivePlatoon {Platoon* me;};
const int NOT_SLAVE=0,IS_SLAVE=1,IN_PRISON=2;
struct StateBroadcastData{int _slaveState;StateBroadcastData():_slaveState(0){}};
const int NULL_TASK=0,MOVE_ON_FREE_WILL=1,OPERATE_MACHINERY=87;
struct Character {
 Faction* f;RaceData race;Platoon platoon;ActivePlatoon active;
 float firstAid,repair;int bags;bool wearing,robotLimbs,equipWorks;
 hand id;std::string name;Ogre::Vector3 pos;float rate,hunger,food,original;
 int goal,clears,moves,price,inSomething;StateBroadcastData state;bool dead,ko,combat,animal,noRoom;
 Character(int i,Faction* faction,const char* n,float x=0):f(faction),id(i),name(n),pos(x,0,0),rate(1),hunger(2.5f),food(0.4f),original(3),
 goal(87),clears(0),moves(0),price(100),inSomething(0),dead(false),ko(false),combat(false),animal(false),noRoom(false)
 {actors[i]=this;platoon.f=f;platoon.malnourishedLevel=3;active.me=&platoon;firstAid=150;repair=500;bags=1;wearing=true;robotLimbs=false;equipWorks=true;}
 Faction* getFaction(){return f;}ActivePlatoon* getPlatoon(){return &active;}RaceData* getRace(){return &race;}
 StateBroadcastData* getStateBroadcast(){return &state;}
 void* isAnimal(){return animal ? this : NULL;}bool isDead(){return dead;}bool isUnconcious(){return ko;}
 bool isInCombatMode(bool,bool){return combat;}bool isEnemy(Character*,bool){return false;}
 Ogre::Vector3 getPosition(){return pos;}const hand& getHandle(){return id;}std::string getName(){return name;}
 void setDestination(const Ogre::Vector3&,bool){goal=MOVE_ON_FREE_WILL;++moves;}
};
Character* hand::getCharacter()const {
 std::map<int,Character*>::iterator it=actors.find(index);
 if(it==actors.end())return NULL;
 const hand& actual=it->second->id;
 return type==actual.type && container==actual.container && containerSerial==actual.containerSerial &&
     index==actual.index && serial==actual.serial ? it->second : NULL;
}
bool fault=false;int buyMode=0,buyCalls=0,errors=0,depth=0,originalCalls=0;
void ErrorLog(const char*){++errors;}void DebugLog(const std::string&){}
bool DfTransferFaulted(){return fault;}bool DfTransferBusy(){return depth!=0;}
void DfQuarantineTransfer(const char*){fault=true;}
const char* Tr(const char* s){return s;}
const char* kDfFactionRecord="1-DynamicFactions.mod";
int DfGoalOf(Character* c){return c->goal;}
void DfClearOrders(Character* c){c->goal=NULL_TASK;++c->clears;}
struct DfFactionState {int events;DfFactionState():events(0){}};
void DfAddEvent(DfFactionState& s,double,const char*){++s.events;}
void DfShowTradeReceipt(Character*,int){}
struct DfTrader {hand who;};const float kDfTradeRange=60;
enum DfBuyStatus {DF_BUY_OK,DF_BUY_NO_FOOD,DF_BUY_NO_MONEY,DF_BUY_NO_ROOM,DF_BUY_PLAYER_TRADING,DF_BUY_INVALID,DF_BUY_FAULT};
struct SellGuard {SellGuard(){++depth;}~SellGuard(){--depth;}};
bool BuyFood(Character*,Character*,Faction*,int,int&,std::string&,DfBuyStatus&);
bool DfBuyFood(Character*,Character*,Faction*,int,int&,std::string&,DfBuyStatus*);
bool Food(Character* c,Faction* f,float& rate,float& hunger,float& stock)
{rate=c->rate;hunger=c->hunger;stock=c->food;return c->f==f && !c->dead;}
struct DfMarketCache{std::map<int,int> knownPrices;unsigned long revision;DfMarketCache():revision(0){}};bool forecastKnown=true;int remembered=0,forecastCalls=0,forecastReserve=0;
bool DfReadSupplyStock(Character* c,DfSupplyStock& s){s=DfSupplyStock();s.known=true;s.robot=c->race.robot;s.robotLimbs=c->robotLimbs;s.animal=c->animal;s.foodRate=c->rate;s.hunger=c->hunger;s.food=c->food;s.firstAid=c->firstAid;s.repair=c->repair;s.ownedBags=c->bags;s.wearingBag=c->wearing;return true;}
bool DfLightBagRole(Character*){return true;}
bool DfEquipOwnedBag(Character* c,bool,bool& equipped){equipped=c->wearing=c->equipWorks;return true;}
bool DfForecastSupplies(Faction* f,const std::vector<Character*>&,const DfMarketCache&,int committed,unsigned long,DfBudgetSnapshot& b){++forecastCalls;b=DfBudgetSnapshot();b.money=f->factionOwnerships->money;b.committed=committed;b.emergencyBuffer=forecastReserve;b.forecastKnown=forecastKnown;return forecastKnown;}
bool DfKnownSupplyBlocked(const DfMarketCache& m,const hand& t,DfSupplyKind k,bool,const DfBudgetSnapshot& b,unsigned long){std::map<int,int>::const_iterator p=m.knownPrices.find(t.index);return p!=m.knownPrices.end() && (p->second>b.money-b.committed || (b.forecastKnown && !DfPlanSupplySpend(b,p->second,k).Allowed()));}
void DfRememberShop(DfMarketCache& m,const hand& t,const std::vector<DfSupplyQuote>& offers,unsigned long){++remembered;++m.revision;if(!offers.empty() && offers[0].price>0)m.knownPrices[t.index]=offers[0].price;}
Character* DfChooseSupplyTrader(Character* c,DfSupplyKind kind,bool,const std::vector<DfTrader>& shops,const DfMarketCache& market,const std::vector<hand>& failed,unsigned long,const DfBudgetSnapshot* budget=NULL,bool allowUnknown=true,size_t* cursor=NULL,bool* discovery=NULL){Character* best=NULL;float distance=0;bool chosenUnknown=false;for(size_t i=0;i<shops.size();++i){Character* t=shops[i].who.getCharacter();bool skip=false;for(size_t k=0;k<failed.size();++k)if(failed[k].index==shops[i].who.index)skip=true;if(!t || skip)continue;bool unknown=false;if(budget && kind!=DF_SUPPLY_FOOD){std::map<int,int>::const_iterator price=market.knownPrices.find(t->id.index);unknown=price==market.knownPrices.end() || !budget->forecastKnown;if(price!=market.knownPrices.end() && (price->second>budget->money-budget->committed || (budget->forecastKnown && !DfPlanSupplySpend(*budget,price->second,kind).Allowed())))continue;if(unknown && !allowUnknown)continue;}float d=c->pos.squaredDistance(t->pos);if(!best || d<distance){best=t;distance=d;chosenUnknown=unknown;}}if(discovery)*discovery=chosenUnknown;if(cursor && best && chosenUnknown)++*cursor;return best;}
bool BuySupply(Character*,Character*,Faction*,DfSupplyKind,bool,const DfBudgetSnapshot&,int&,std::string&,std::vector<DfSupplyQuote>&,DfBuyStatus&);
'''
code += state[start:end] + '\n'
code += function(mine, 'bool DfEconomyAvailable(') + '\n'
code += function(sell, 'bool SafeBuyFood(') + '\n'
code += function(sell, 'bool DfBuyFood(') + '\n'
code += function(sell, 'bool SafeBuySupply(') + '\n'
code += function(sell, 'bool DfBuySupply(') + '\n'
code += r'''
bool BuyFood(Character* member,Character* trader,Faction* f,int committed,int& paid,std::string& name,DfBuyStatus& status){
 ++buyCalls;
 if(buyMode==1)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);
 if(buyMode==2){int n=0;std::string s;DfBuyStatus result;bool nested=DfBuyFood(member,trader,f,committed,n,s,&result);
 if(nested || n!=0)throw 1;status=DF_BUY_NO_FOOD;return true;}
 if(f->factionOwnerships->money-committed<trader->price){status=DF_BUY_NO_MONEY;return true;}
 if(trader->noRoom){status=DF_BUY_NO_ROOM;return true;}
 paid=trader->price;f->factionOwnerships->money-=paid;member->food+=0.7f;name="Food";status=DF_BUY_OK;return true;
}
bool BuySupply(Character* member,Character* trader,Faction* f,DfSupplyKind kind,bool,const DfBudgetSnapshot& budget,int& paid,std::string& name,std::vector<DfSupplyQuote>& observed,DfBuyStatus& status){
 observed.push_back(DfSupplyQuote());
 if(kind==DF_SUPPLY_FOOD)return BuyFood(member,trader,f,budget.committed,paid,name,status);
 ++buyCalls;if(buyMode==1)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);
 if(!DfPlanSupplySpend(budget,trader->price,kind).Allowed()){status=DF_BUY_NO_MONEY;return true;}
 // Native BuyOtherSupplyFromStock reports NO_ROOM when its ranked fit fails.
 // Model only this native inventory boundary; route/cooldown policy stays production code.
 if(member->noRoom){status=DF_BUY_NO_ROOM;return true;}
 paid=trader->price;f->factionOwnerships->money-=paid;name="Supply";status=DF_BUY_OK;
 if(kind==DF_SUPPLY_FIRSTAID)member->firstAid+=150;else if(kind==DF_SUPPLY_REPAIR)member->repair+=500;else ++member->bags;
 return true;}
'''
for signature in ('bool Same(', 'bool Future(', 'void ClearRoute(', 'void Delay(',
                  'bool ReadStock(', 'int SupplyPriority(', 'DfSupplyKind NextKind(', 'const DfBudgetSnapshot& PlannedBudget(', 'bool DfProvisionOwns(', 'void DfProvisionStep('):
    code += function(provision, signature) + '\n'
code += r'''
struct DfLabour{DfProvision provision;int carried,revenue;DfLabour():carried(0),revenue(0){}};
void DfLabourCounts(const DfLabour&,int& m,int& h){m=2;h=1;}
'''
code += function(labour, 'std::string DfLabourStatus(') + '\n'
code += r'''
float NativeHunger(Character* c){++originalCalls;return c->original;}
float (*g_originalHunger)(Character*)=&NativeHunger;
'''
code += function(hunger, 'float OwnFloor(') + '\n' + function(hunger, 'float HungerHook(') + '\n'
code += r'''
int checks=0,failed=0;
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
bool NativeFaultProbe(Character* member,Character* trader,Faction* f,int& paid,std::string& name,DfBuyStatus& status){
 __try {return DfBuyFood(member,trader,f,0,paid,name,&status);}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
int main(){
GameData df("1-DynamicFactions.mod"),other("204-gamedata.base");Ownerships wallet(1000),shopMoney(1000);
Faction faction={&df,&wallet},foreign={&other,&shopMoney};
Character member(1,&faction,"Worker"),expensive(2,&foreign,"Expensive",1),cheap(3,&foreign,"Cheap",2);
expensive.price=2000;cheap.price=100;std::vector<Character*> people(1,&member);std::vector<DfTrader> shops(2);
shops[0].who=expensive.id;shops[1].who=cheap.id;DfProvision route;DfFactionState s;
DfProvisionStep(s,route,&faction,people,shops,NULL,0,1,5000,false);
CHECK(route.failedShops.size()==1 && !route.member.isNull());CHECK(wallet.money==1000 && buyCalls==1);
DfProvisionStep(s,route,&faction,people,shops,NULL,0,1,10000,false);
CHECK(wallet.money==900 && route.spent==100 && buyCalls==2);CHECK(s.events==1);
DfProvisionStep(s,route,&faction,people,shops,NULL,0,1,15000,false);
CHECK(route.member.isNull() && buyCalls==2);
// A shape failure is local to the offered stock; another shop can fit.
member.food=0.4f;expensive.price=100;expensive.noRoom=true;DfProvision shapes;
DfProvisionStep(s,shapes,&faction,people,shops,NULL,0,2,20000,false);
CHECK(shapes.failedShops.size()==1 && !shapes.member.isNull());
DfProvisionStep(s,shapes,&faction,people,shops,NULL,0,2,25000,false);
CHECK(shapes.spent==100 && wallet.money==800);
// A truly empty usable budget may delay, but never spends a commitment.
member.food=0.4f;wallet.money=100;DfProvision reserved;
DfProvisionStep(s,reserved,&faction,people,shops,NULL,100,2,30000,false);
CHECK(reserved.member.isNull() && wallet.money==100);
// Native medicine/combat supersedes a route and is not cleared by it.
DfProvision healing;healing.member=member.id;healing.moving=true;member.goal=58;int before=member.clears;
DfProvisionStep(s,healing,&faction,people,shops,NULL,0,2,35000,false);
CHECK(member.goal==58 && member.clears==before && healing.member.isNull());
member.goal=OPERATE_MACHINERY;CHECK(DfEconomyAvailable(&member));member.goal=MOVE_ON_FREE_WILL;CHECK(DfEconomyAvailable(&member));
member.combat=true;CHECK(!DfEconomyAvailable(&member));member.combat=false;
// The public purchase guard must unwind normally after a native AV.
int paid=0;std::string name;DfBuyStatus status;buyMode=1;
CHECK(!NativeFaultProbe(&member,&cheap,&faction,paid,name,status));CHECK(fault && depth==0 && status==DF_BUY_FAULT);
fault=false;buyMode=2;int calls=buyCalls;CHECK(DfBuyFood(&member,&cheap,&faction,0,paid,name,&status));
CHECK(depth==0 && buyCalls==calls+1 && paid==0);buyMode=0;
// Very long names, including printf tokens, are data rather than a format.
DfLabour rt;rt.provision.memberName=std::string(20000,'x')+"%d";rt.provision.spent=77;
std::string text=DfLabourStatus(rt);CHECK(text.find(rt.provision.memberName)!=std::string::npos);
CHECK(text.find("spent this session: 77")!=std::string::npos);
// Hunger scoping does not wait for metadata and always calls original once.
member.goal=87;originalCalls=0;CHECK(HungerHook(&member)==-10 && originalCalls==1);
CHECK(member.platoon.malnourishedLevel==3);CHECK(HungerHook(&cheap)==3 && originalCalls==2);
member.race.robot=true;CHECK(HungerHook(&member)==3);member.race.robot=false;
member.animal=true;CHECK(HungerHook(&member)==3);member.animal=false;
member.original=1.5f;CHECK(HungerHook(&member)==1.5f);
member.original=3;member.state._slaveState=IS_SLAVE;CHECK(HungerHook(&member)==3);member.state._slaveState=NOT_SLAVE;
member.inSomething=IN_PRISON;CHECK(HungerHook(&member)==3);member.inSomething=0;
// Moving forever without arriving must yield the single supply route.
member.food=0.4f;member.goal=87;member.pos=Ogre::Vector3();member.hunger=2.2f;wallet.money=1000;
Character next(4,&faction,"Next");std::vector<Character*> group;group.push_back(&member);group.push_back(&next);
std::vector<DfTrader> distant(1);distant[0].who=cheap.id;cheap.pos=Ogre::Vector3(10000,0,0);
DfProvision chase;DfProvisionStep(s,chase,&faction,group,distant,NULL,0,3,1000,false);
for(unsigned long now=6000;now<=181000;now+=5000){member.pos.x+=100;cheap.pos.x+=200;
 DfProvisionStep(s,chase,&faction,group,distant,NULL,0,3,now,false);}
CHECK(chase.member.isNull());CHECK(chase.cooldown.find(member.id)!=chase.cooldown.end());
DfProvisionStep(s,chase,&faction,group,distant,NULL,0,3,186000,false);
CHECK(!chase.member.isNull() && chase.member.index==next.id.index);
// Robots replenish repair resource, never food or first-aid stock.
fault=false;buyMode=0;wallet.money=10000;cheap.pos=Ogre::Vector3();member.pos=Ogre::Vector3();
Character robot(8,&faction,"Robot");robot.race.robot=true;robot.rate=0;robot.food=0;robot.repair=0;robot.firstAid=0;
std::vector<Character*> robots(1,&robot);DfProvision care;int oldCalls=buyCalls;
DfProvisionStep(s,care,&faction,robots,shops,NULL,0,4,1000,false);
CHECK(care.kind==DF_SUPPLY_REPAIR && robot.repair==500 && robot.food==0 && robot.firstAid==0);CHECK(buyCalls==oldCalls+1);
// Bought but unworn bags are reused after a refused equip, never repurchased.
Character carrier(9,&faction,"Carrier");carrier.food=1;carrier.bags=0;carrier.wearing=false;carrier.equipWorks=false;
std::vector<Character*> carriers(1,&carrier);DfProvision gear;oldCalls=buyCalls;
DfProvisionStep(s,gear,&faction,carriers,shops,NULL,0,5,1000,false);CHECK(carrier.bags==1 && buyCalls==oldCalls+1);
DfProvisionStep(s,gear,&faction,carriers,shops,NULL,0,5,6000,false);CHECK(carrier.bags==1 && buyCalls==oldCalls+1 && !carrier.wearing);
DfProvisionStep(s,gear,&faction,carriers,shops,NULL,0,5,40000,false);CHECK(carrier.bags==1 && buyCalls==oldCalls+1);
carrier.equipWorks=true;DfProvisionStep(s,gear,&faction,carriers,shops,NULL,0,5,80000,false);CHECK(carrier.wearing && buyCalls==oldCalls+1);
// Incomplete budgets cannot buy optional gear.
Character unknownBudget(10,&faction,"Unknown");unknownBudget.food=1;unknownBudget.bags=0;unknownBudget.wearing=false;
carriers[0]=&unknownBudget;forecastKnown=false;DfProvision blocked;oldCalls=wallet.money;
DfProvisionStep(s,blocked,&faction,carriers,shops,NULL,0,6,1000,false);CHECK(wallet.money==oldCalls && unknownBudget.bags==0);forecastKnown=true;
// A hungry member preempts a distant backpack route, without a gear purchase.
Character gearWalker(11,&faction,"Walking");gearWalker.food=1;gearWalker.bags=0;gearWalker.wearing=false;
Character hungry(12,&faction,"Hungry");hungry.food=1;std::vector<Character*> priorityPeople;
cheap.pos=Ogre::Vector3(10000,0,0);
priorityPeople.push_back(&gearWalker);priorityPeople.push_back(&hungry);DfProvision preempt;
DfProvisionStep(s,preempt,&faction,priorityPeople,distant,NULL,0,7,1000,false);
CHECK(preempt.kind==DF_SUPPLY_BACKPACK && preempt.member.index==gearWalker.id.index);
hungry.food=0;hungry.hunger=1.2f;
DfProvisionStep(s,preempt,&faction,priorityPeople,distant,NULL,0,7,6000,false);
CHECK(preempt.kind==DF_SUPPLY_FOOD && preempt.member.index==hungry.id.index && gearWalker.bags==0);
// The traveller's own hunger also switches the route before buying gear.
hungry.food=1;gearWalker.food=1;DfProvision ownFood;
std::vector<Character*> oneWalker(1,&gearWalker);
DfProvisionStep(s,ownFood,&faction,oneWalker,distant,NULL,0,8,1000,false);
CHECK(ownFood.kind==DF_SUPPLY_BACKPACK);
gearWalker.food=0;gearWalker.hunger=1.2f;
DfProvisionStep(s,ownFood,&faction,oneWalker,distant,NULL,0,8,6000,false);
CHECK(ownFood.kind==DF_SUPPLY_FOOD && gearWalker.bags==0);
// Known unaffordable bags must not claim a worker or clear real work orders.
Character saver(60,&faction,"Saver");saver.food=1;saver.bags=0;saver.wearing=false;
std::vector<Character*> savers(1,&saver);std::vector<DfTrader> bagShops(1);bagShops[0].who=cheap.id;
cheap.price=5000;cheap.pos=Ogre::Vector3(1000,0,0);wallet.money=3348;forecastReserve=2631;
DfProvision saving;saving.market.knownPrices[cheap.id.index]=5000;int oldClears=saver.clears;oldCalls=buyCalls;
DfProvisionStep(s,saving,&faction,savers,bagShops,NULL,0,9,1000,false);
CHECK(saving.member.isNull() && saver.clears==oldClears && saver.moves==0 && buyCalls==oldCalls);
// Gross cash is insufficient evidence: food/medical/operating reserve remains protected.
wallet.money=6000;DfProvision protectedBag;protectedBag.market.knownPrices[cheap.id.index]=5000;
DfProvisionStep(s,protectedBag,&faction,savers,bagShops,NULL,0,9,2000,false);
CHECK(protectedBag.member.isNull() && saver.clears==oldClears && saver.moves==0);
// A real wallet increase invalidates the cached plan and permits an affordable trip.
wallet.money=10000;DfProvisionStep(s,saving,&faction,savers,bagShops,NULL,0,9,3000,false);
CHECK(saving.member.index==saver.id.index && saver.moves==1);
// Unknown/partial forecasts permit one real discovery, not repeated workers.
forecastKnown=false;DfProvision probing;probing.market.knownPrices[cheap.id.index]=5000;saver.goal=87;
DfProvisionStep(s,probing,&faction,savers,bagShops,NULL,0,9,1000,false);
CHECK(probing.member.index==saver.id.index);
DfProvisionStep(s,probing,&faction,savers,bagShops,NULL,0,9,181000,false);
CHECK(probing.member.isNull());
DfProvisionStep(s,probing,&faction,savers,bagShops,NULL,0,9,216000,false);
CHECK(probing.member.isNull());
// Urgent food bypasses optional planning deferral and the old worker cooldown.
saver.food=0;saver.hunger=1.2f;cheap.price=100;cheap.pos=saver.pos;oldCalls=buyCalls;wallet.money=100;
probing.cooldown[saver.id]=250000;
DfProvisionStep(s,probing,&faction,savers,bagShops,NULL,0,9,220000,false);
CHECK(buyCalls==oldCalls+1 && probing.kind==DF_SUPPLY_FOOD && wallet.money==0);
forecastKnown=true;forecastReserve=0;
// Forecast work is shared across candidate/arrival checks and cached for5s.
saver.food=1;saver.hunger=3;saver.goal=87;wallet.money=3348;forecastReserve=2631;
DfProvision cached;cached.market.knownPrices[cheap.id.index]=5000;forecastCalls=0;
for(unsigned long now=1000;now<2000;now+=50)DfProvisionStep(s,cached,&faction,savers,bagShops,NULL,0,9,now,false);
CHECK(forecastCalls==1);
wallet.money=3349;DfProvisionStep(s,cached,&faction,savers,bagShops,NULL,0,9,2000,false);CHECK(forecastCalls==2);
++cached.market.revision;DfProvisionStep(s,cached,&faction,savers,bagShops,NULL,0,9,2100,false);CHECK(forecastCalls==3);
DfProvisionStep(s,cached,&faction,savers,bagShops,NULL,1,9,2200,false);CHECK(forecastCalls==4);
Character replacement(61,&faction,"Replacement");replacement.food=1;replacement.bags=0;replacement.wearing=false;
savers[0]=&replacement;DfProvisionStep(s,cached,&faction,savers,bagShops,NULL,1,9,2300,false);CHECK(forecastCalls==5);
forecastReserve=0;
// A failed FOOD route must yield to another equally urgent member, not merely
// clear and immediately reclaim the single route through the urgency bypass.
Character urgentA(80,&faction,"Urgent unreachable"),urgentB(81,&faction,"Urgent beside shop");
urgentA.food=urgentB.food=0;urgentA.hunger=urgentB.hunger=1.2f;
urgentA.pos=Ogre::Vector3();cheap.pos=Ogre::Vector3(10000,0,0);urgentB.pos=cheap.pos;
std::vector<Character*> urgentGroup;urgentGroup.push_back(&urgentA);urgentGroup.push_back(&urgentB);
std::vector<DfTrader> urgentShops(1);urgentShops[0].who=cheap.id;
cheap.price=100;cheap.noRoom=false;wallet.money=10000;DfProvision urgentFair;
DfProvisionStep(s,urgentFair,&faction,urgentGroup,urgentShops,NULL,0,10,1000,false);
CHECK(urgentFair.member.index==urgentA.id.index && urgentB.food==0);
for(unsigned long tick=6000;tick<=181000;tick+=5000)
{
 urgentA.pos.x+=100;cheap.pos.x+=200;urgentB.pos=cheap.pos;
 DfProvisionStep(s,urgentFair,&faction,urgentGroup,urgentShops,NULL,0,10,tick,false);
}
CHECK(urgentFair.member.isNull());
DfProvisionStep(s,urgentFair,&faction,urgentGroup,urgentShops,NULL,0,10,186000,false);
CHECK(urgentB.food>0 && urgentA.food==0);
// A failed food route is deferred, not permanently banned after the next peer.
urgentB.food=1;urgentA.pos=cheap.pos;
DfProvisionStep(s,urgentFair,&faction,urgentGroup,urgentShops,NULL,0,10,191000,false);
DfProvisionStep(s,urgentFair,&faction,urgentGroup,urgentShops,NULL,0,10,216000,false);
CHECK(urgentA.food>0);

// A known affordable optional purchase with no inventory fit must return the
// worker to real work, then defer the identical retry instead of clearing that
// work again every5s. Exercise both medicine and first-backpack purchases.
for(int optional=0;optional<2;++optional)
{
 Character fullWorker(90+optional,&faction,optional ? "Full backpack buyer" : "Full aid buyer");
 fullWorker.food=1;fullWorker.firstAid=optional ? 150.0f : 0.0f;
 fullWorker.bags=optional ? 0 : 1;fullWorker.wearing=!optional;fullWorker.noRoom=true;
 fullWorker.pos=Ogre::Vector3();cheap.pos=fullWorker.pos;cheap.price=100;
 std::vector<Character*> fullGroup(1,&fullWorker);std::vector<DfTrader> fullShops(1);fullShops[0].who=cheap.id;
 wallet.money=10000;forecastKnown=true;DfProvision fullRoute;fullRoute.market.knownPrices[cheap.id.index]=100;
 const int beforeAttempt=buyCalls;
 DfProvisionStep(s,fullRoute,&faction,fullGroup,fullShops,NULL,0,11,1000,false);
 CHECK(fullRoute.member.isNull() && buyCalls==beforeAttempt+1 && wallet.money==10000);
 // Labour has resumed its native mining/hauling goal between economic ticks.
 fullWorker.goal=OPERATE_MACHINERY;const int resumedClears=fullWorker.clears;
 DfProvisionStep(s,fullRoute,&faction,fullGroup,fullShops,NULL,0,11,6000,false);
 CHECK(buyCalls==beforeAttempt+1 && fullWorker.clears==resumedClears && fullRoute.member.isNull());
 // The optional fit deferral must still permit urgent food for this same actor.
 fullWorker.noRoom=false;fullWorker.food=0;fullWorker.hunger=1.2f;
 const int beforeUrgent=buyCalls;
 DfProvisionStep(s,fullRoute,&faction,fullGroup,fullShops,NULL,0,11,11000,false);
 CHECK(buyCalls==beforeUrgent+1 && fullWorker.food>0 && fullRoute.kind==DF_SUPPLY_FOOD);
 fullWorker.food=1;
 const int afterUrgent=buyCalls;
 DfProvisionStep(s,fullRoute,&faction,fullGroup,fullShops,NULL,0,11,16000,false);
 CHECK(buyCalls==afterUrgent && (optional ? fullWorker.bags==0 : fullWorker.firstAid==0));
 // A bounded retry window expires; improved fit may then buy the real supply.
 DfProvisionStep(s,fullRoute,&faction,fullGroup,fullShops,NULL,0,11,301000,false);
 CHECK(optional ? fullWorker.bags==1 : fullWorker.firstAid==150);
}
// The two retry caches share native handle lifetime and DWORD clock behavior.
// A deadline spanning GetTickCount wrap blocks briefly, then permits food again.
Character wrapped(110,&faction,"Wrapped deadline");wrapped.food=0;wrapped.hunger=1.2f;
wrapped.pos=Ogre::Vector3();cheap.pos=wrapped.pos;cheap.price=100;wallet.money=10000;
std::vector<Character*> lifecyclePeople(1,&wrapped);DfProvision wrapRoute;
const unsigned long nearWrap=0xffffff00UL;
wrapRoute.member=wrapped.id;wrapRoute.kind=DF_SUPPLY_FOOD;wrapRoute.startedAt=nearWrap-180000UL;
DfProvisionStep(s,wrapRoute,&faction,lifecyclePeople,urgentShops,NULL,0,12,nearWrap,false);
const int beforeWrapRetry=buyCalls;
DfProvisionStep(s,wrapRoute,&faction,lifecyclePeople,urgentShops,NULL,0,12,nearWrap+5000UL,false);
CHECK(wrapRoute.member.isNull() && buyCalls==beforeWrapRetry && wrapped.food==0);
DfProvisionStep(s,wrapRoute,&faction,lifecyclePeople,urgentShops,NULL,0,12,nearWrap+30000UL,false);
CHECK(buyCalls==beforeWrapRetry+1 && wrapped.food>0 && wrapRoute.cooldown.empty() && wrapRoute.foodCooldown.empty());

// Reusing an object index with a new serial must not inherit the old actor's wait.
Character previousMember(111,&faction,"Old serial");DfProvision recycled;
recycled.cooldown[previousMember.id]=31000;recycled.foodCooldown[previousMember.id]=31000;
Character replacementMember(111,&faction,"New serial");replacementMember.id.serial=previousMember.id.serial+1;
replacementMember.food=0;replacementMember.hunger=1.2f;replacementMember.pos=cheap.pos;
lifecyclePeople[0]=&replacementMember;
DfProvisionStep(s,recycled,&faction,lifecyclePeople,urgentShops,NULL,0,12,6000,false);
CHECK(replacementMember.food>0 && recycled.cooldown.empty() && recycled.foodCooldown.empty());

// Entries for a still-resolvable character who left the faction are pruned too.
Character departed(112,&faction,"Departed");DfProvision departure;
departure.cooldown[departed.id]=31000;departure.foodCooldown[departed.id]=31000;departed.f=&foreign;
replacementMember.food=1;
DfProvisionStep(s,departure,&faction,lifecyclePeople,urgentShops,NULL,0,12,6000,false);
CHECK(departure.cooldown.empty() && departure.foodCooldown.empty());

// World/load reset constructs a new ephemeral runtime; old food waits must not
// delay the same physical actor after that reset or recreate a saved purchase.
Character afterLoad(113,&faction,"After load");afterLoad.food=0;afterLoad.hunger=1.2f;afterLoad.pos=cheap.pos;
DfProvision reloaded;reloaded.cooldown[afterLoad.id]=31000;reloaded.foodCooldown[afterLoad.id]=31000;
lifecyclePeople[0]=&afterLoad;const int beforeLoad=buyCalls;
DfProvisionStep(s,reloaded,&faction,lifecyclePeople,urgentShops,NULL,0,12,6000,false);
CHECK(buyCalls==beforeLoad && afterLoad.food==0);
reloaded=DfProvision();
DfProvisionStep(s,reloaded,&faction,lifecyclePeople,urgentShops,NULL,0,12,11000,false);
CHECK(buyCalls==beforeLoad+1 && afterLoad.food>0);
std::printf("%d supply adapter checks, %d failed\n",checks,failed);return failed;
}
'''
(out / 'supply_adapter_tests.cpp').write_text(code, encoding='utf-8')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env = dict(os.environ)
env['INCLUDE'] = ';'.join((str(vc / 'include'), str(sdk / 'Include'), str(root)))
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2',
                str(out / 'supply_adapter_tests.cpp'), str(root / 'core/DfSupply.cpp'),str(root / 'core/DfBudget.cpp'),
                '/Fo' + str(out) + '\\', '/Fe' + str(out / 'supply_adapter_tests.exe')],
               env=env, check=True)
subprocess.run([str(out / 'supply_adapter_tests.exe')], check=True)
