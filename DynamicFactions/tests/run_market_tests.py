"""Production market knowledge/reserve with native reads represented by fixtures."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[1];out=root.parents[1]/'build/DynamicFactionsMarketCheck';out.mkdir(parents=True,exist_ok=True)
text=(root/'game/DfMarket.cpp').read_text(encoding='utf-8-sig')
def function(signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
state=(root/'game/DfMarket.h').read_text(encoding='utf-8-sig');state=state[state.index('struct DfKnownPrices'):state.index('void DfRememberShop')]
code=r'''
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <climits>
#include <float.h>
#include <cstdio>
#include "core/DfOffer.h"
struct Character;std::map<int,Character*> actors;
struct hand{int type,container,containerSerial,index,serial;hand(int i=0):type(1),container(0),containerSerial(0),index(i),serial(1){}bool isNull()const{return index==0;}Character* getCharacter()const{return actors[index];}};
struct DfWorkHandLess{bool operator()(const hand& a,const hand& b)const{return a.index<b.index || (a.index==b.index && a.serial<b.serial);}};
namespace Ogre{struct Vector3{float x;Vector3(float a=0):x(a){}float squaredDistance(const Vector3& b)const{float d=x-b.x;return d*d;}};}
struct GameData{std::string id;GameData(const char* s="food"):id(s){}};GameData item;
struct RaceData{bool canEat(GameData* d,bool){return d && d->id=="food";}};
struct Ownerships{int money;Ownerships(int n):money(n){}};struct Faction{Ownerships* factionOwnerships;};
struct Character{hand id;Ogre::Vector3 pos;RaceData race;DfSupplyStock stock;Faction* faction;bool dead;
 Character(int n,Faction* f,float x=0):id(n),pos(x),faction(f),dead(false){actors[n]=this;stock.known=true;stock.foodRate=1;stock.hunger=3;stock.food=1;stock.firstAid=150;}
 const hand& getHandle(){return id;}Ogre::Vector3 getPosition(){return pos;}RaceData* getRace(){return &race;}void* isAnimal(){return NULL;}
 bool isDead(){return dead;}bool isUnconcious(){return false;}bool isInCombatMode(bool,bool){return false;}bool isEnemy(Character*,bool){return false;}Faction* getFaction(){return faction;}};
struct DfTrader{hand who;};
bool DfReadSupplyStock(Character* c,DfSupplyStock& s){s=c->stock;return s.known;}
GameData* DfItemBySid(const std::string& sid,int& value){value=100;return sid=="food"?&item:NULL;}
struct LARGE_INTEGER{__int64 QuadPart;};long long clockStep=1,clockNow=0;
bool QueryPerformanceFrequency(LARGE_INTEGER* p){p->QuadPart=1000000;return true;}
bool QueryPerformanceCounter(LARGE_INTEGER* p){p->QuadPart=(clockNow+=clockStep);return true;}
'''+state
for signature in ('bool SameShop(', 'const DfKnownShop* ObservedShop(', 'const DfKnownShop* KnownShop(', 'bool EligibleQuote(', 'bool ObservedPriceBlocks(', 'bool KnownSupplyBlocked(', 'bool CanUseFood(', 'bool WithinForecast(', 'int CheapestDeficit(', 'bool ForecastSupplies(', 'void DfRememberShop(', 'Character* ChooseSupplyTrader('):code+=function(signature)+'\n'
code+='\n#define DfChooseSupplyTrader ChooseSupplyTrader\n'
code+=r'''
int checks=0,failed=0;
#define CHECK(x) do{++checks;if(!(x)){++failed;printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
int main(){Ownerships wallet(10000);Faction f={&wallet};Character member(1,&f),nearby(2,&f,50),bar(3,&f,100);
 std::vector<DfTrader> traders(2);traders[0].who=nearby.id;traders[1].who=bar.id;DfMarketCache market;std::vector<hand> failedShops;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_FOOD,true,traders,market,failedShops,1000)==&nearby);
 std::vector<DfSupplyQuote> empty;DfRememberShop(market,nearby.id,empty,1000);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_FOOD,true,traders,market,failedShops,1000)==&bar);
 DfSupplyQuote q;q.sid="food";q.kind=DF_SUPPLY_FOOD;q.price=100;q.amount=.5f;q.quantity=2;std::vector<DfSupplyQuote> food(1,q);
 DfRememberShop(market,bar.id,food,1000);failedShops.push_back(bar.id);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_FOOD,true,traders,market,failedShops,1000)==NULL);failedShops.clear();
 std::vector<Character*> people(1,&member);DfBudgetSnapshot b;
 CHECK(ForecastSupplies(&f,people,market,200,1000,b));CHECK(b.forecastKnown && b.foodDeficitCost==0 && b.medicalDeficitCost==0 && b.emergencyBuffer==1000 && b.committed==200);
 member.stock.food=0;CHECK(ForecastSupplies(&f,people,market,200,1000,b));CHECK(b.foodDeficitCost==200);
 CHECK(!ForecastSupplies(&f,people,market,200,121001,b));CHECK(!b.forecastKnown);
 member.stock.robot=true;member.stock.foodRate=0;member.stock.repair=0;
 q.kind=DF_SUPPLY_REPAIR;q.sid="repair";q.price=4000;q.amount=500;q.quantity=1;q.pool.index=100;
 std::vector<DfSupplyQuote> repair(1,q);DfRememberShop(market,bar.id,repair,1000);
 CHECK(ForecastSupplies(&f,people,market,0,1000,b));CHECK(b.foodDeficitCost==0 && b.medicalDeficitCost==4000);
 Character second(44,&f);second.stock.robot=true;second.stock.foodRate=0;second.stock.repair=0;people.push_back(&second);
 CHECK(!ForecastSupplies(&f,people,market,0,1000,b));CHECK(!b.forecastKnown);
 // Another shopkeeper at the SAME backing pool cannot multiply its lone kit.
 DfRememberShop(market,nearby.id,repair,1001);
 CHECK(!ForecastSupplies(&f,people,market,0,1001,b));CHECK(!b.forecastKnown);
 q.price=6000;q.pool.index=101;repair[0]=q;DfRememberShop(market,bar.id,repair,1001);
 CHECK(ForecastSupplies(&f,people,market,0,1001,b));CHECK(b.medicalDeficitCost==10000);
 // A physical visit to the now-empty shared pool invalidates its old offer.
 DfSupplyQuote emptyPool;emptyPool.pool.index=100;std::vector<DfSupplyQuote> exhausted(1,emptyPool);
 DfRememberShop(market,nearby.id,exhausted,1002);CHECK(!ForecastSupplies(&f,people,market,0,1002,b));
 people.pop_back();member.stock.repair=500;CHECK(ForecastSupplies(&f,people,market,0,1000,b));CHECK(b.medicalDeficitCost==0);
 clockStep=5000;CHECK(!ForecastSupplies(&f,people,market,0,1000,b));CHECK(!b.forecastKnown);clockStep=1;
 // Planned trips use actual observed prices and protect the real reserve.
 DfMarketCache gear;DfSupplyQuote bag;bag.kind=DF_SUPPLY_BACKPACK;bag.sid="bag";bag.price=5000;bag.quantity=1;bag.amount=1;bag.bag.cells=24;bag.pool.index=555;
 std::vector<DfSupplyQuote> bags(1,bag);DfRememberShop(gear,nearby.id,bags,1000);
 std::vector<DfTrader> onlyKnown(1);onlyKnown[0].who=nearby.id;
 b=DfBudgetSnapshot();b.forecastKnown=true;b.money=3348;b.emergencyBuffer=2631;size_t cursor=0;bool discovery=false;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,true,&cursor,&discovery)==NULL);
 b.money=6000;CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,true,&cursor,&discovery)==NULL);
 b.money=8000;CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,false,&cursor,&discovery)==&nearby && !discovery);
 b.money=3348;CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,traders,gear,failedShops,1000,&b,true,&cursor,&discovery)==&bar && discovery);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,traders,gear,failedShops,1000,&b,false,&cursor,&discovery)==NULL);
 // Unknown shops are physically explored in a fair bounded round-robin,
 // including a farther potentially cheaper shop, rather than nearest forever.
 Character farShop(70,&f,2000);std::vector<DfTrader> unknown(2);unknown[0].who=bar.id;unknown[1].who=farShop.id;cursor=0;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,unknown,gear,failedShops,1000,&b,true,&cursor,&discovery)==&bar);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,unknown,gear,failedShops,1000,&b,true,&cursor,&discovery)==&farShop);
 // An incomplete forecast permits discovery but cannot retry it every worker.
 b.forecastKnown=false;b.money=10000;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,true,&cursor,&discovery)==&nearby && discovery);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,false,&cursor,&discovery)==NULL);
 b.money=3348;CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,1000,&b,true,&cursor,&discovery)==NULL);
 // Finite stock expires at120s, but600s price-only negative evidence cannot
 // become an endlessly rediscovered5000-cat bag merely because stock expired.
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,121001,&b,true,&cursor,&discovery)==NULL);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,onlyKnown,gear,failedShops,601001,&b,true,&cursor,&discovery)==&nearby && discovery);
 // Real observations of shared and even empty pools preserve price-only
 // evidence without duplicating their quantities in the forecast.
 DfRememberShop(gear,bar.id,bags,1010);
 CHECK(gear.shops[0].offers.size()==1 && gear.shops[0].offers[0].quantity==0);
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,traders,gear,failedShops,121011,&b,true,&cursor,&discovery)==NULL);
 DfSupplyQuote pool;pool.pool=bag.pool;std::vector<DfSupplyQuote> poolOnly(1,pool);
 DfRememberShop(gear,bar.id,poolOnly,1020);DfRememberShop(gear,farShop.id,poolOnly,1030);
 CHECK(gear.shops[2].prices.price[4]==5000 && gear.shops[2].prices.quotedAt[4]==1010);
 traders.push_back(DfTrader());traders.back().who=farShop.id;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,traders,gear,failedShops,121031,&b,true,&cursor,&discovery)==NULL);
 // A later real cheaper quote can permit the trip, including its reserve.
 bags[0].price=500;DfRememberShop(gear,farShop.id,bags,1040);b.forecastKnown=true;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_BACKPACK,true,traders,gear,failedShops,1040,&b,false,&cursor,&discovery)==&farShop && !discovery);
 CHECK(gear.revision==5);
 // Planned medical supplies can use their own reserve, while food and the
 // operating buffer remain protected by the unchanged production policy.
 DfMarketCache aidMarket;DfSupplyQuote aid;aid.kind=DF_SUPPLY_FIRSTAID;aid.sid="aid";aid.price=294;aid.quantity=3;aid.amount=150;aid.pool.index=556;
 std::vector<DfSupplyQuote> aidQuotes(1,aid);DfRememberShop(aidMarket,nearby.id,aidQuotes,1000);
 b=DfBudgetSnapshot();b.money=3470;b.foodDeficitCost=510;b.medicalDeficitCost=882;b.emergencyBuffer=1000;b.forecastKnown=true;
 CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_FIRSTAID,true,onlyKnown,aidMarket,failedShops,1000,&b,false,&cursor,&discovery)==&nearby);
 b.money=1510;CHECK(DfChooseSupplyTrader(&member,DF_SUPPLY_FIRSTAID,true,onlyKnown,aidMarket,failedShops,1000,&b,false,&cursor,&discovery)==NULL);
 // An old price from a different backing pool cannot migrate through a newly
 // shared empty pool merely because both pools belonged to the same NPC.
 DfMarketCache unrelated;std::vector<DfSupplyQuote> oldBag(1,bag);DfRememberShop(unrelated,nearby.id,oldBag,1000);
 DfSupplyQuote otherPool;otherPool.pool.index=999;std::vector<DfSupplyQuote> otherEmpty(1,otherPool);
 DfRememberShop(unrelated,nearby.id,otherEmpty,1010);DfRememberShop(unrelated,bar.id,otherEmpty,1020);
 CHECK(unrelated.shops[1].prices.price[4]==0);
 for(int i=4;i<44;++i){Character* extra=new Character(i,&f);DfRememberShop(market,extra->id,empty,1000+i);}
 CHECK(market.shops.size()==32);
 printf("%d market checks, %d failed\n",checks,failed);return failed;}
'''
(out/'market.cpp').write_text(code,encoding='utf-8')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ);env['INCLUDE']=';'.join(map(str,(vc/'include',sdk/'Include',root)));env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(out/'market.cpp'),str(root/'core/DfSupply.cpp'),str(root/'core/DfBudget.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'market.exe')],env=env,check=True)
subprocess.run([str(out/'market.exe')],check=True)
