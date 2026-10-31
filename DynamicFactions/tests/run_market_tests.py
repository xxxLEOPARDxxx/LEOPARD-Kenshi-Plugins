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
state=(root/'game/DfMarket.h').read_text(encoding='utf-8-sig');state=state[state.index('struct DfKnownShop'):state.index('void DfRememberShop')]
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
struct hand{int type,container,containerSerial,index,serial;hand(int i=0):type(1),container(0),containerSerial(0),index(i),serial(1){}Character* getCharacter()const{return actors[index];}};
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
for signature in ('bool SameShop(', 'const DfKnownShop* KnownShop(', 'bool CanUseFood(', 'bool WithinForecast(', 'int CheapestDeficit(', 'bool ForecastSupplies(', 'void DfRememberShop(', 'Character* ChooseSupplyTrader('):code+=function(signature)+'\n'
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
 for(int i=4;i<44;++i){Character* extra=new Character(i,&f);DfRememberShop(market,extra->id,empty,1000+i);}
 CHECK(market.shops.size()==32);
 printf("%d market checks, %d failed\n",checks,failed);return failed;}
'''
(out/'market.cpp').write_text(code,encoding='utf-8')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ);env['INCLUDE']=';'.join(map(str,(vc/'include',sdk/'Include',root)));env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(out/'market.cpp'),str(root/'core/DfSupply.cpp'),str(root/'core/DfBudget.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'market.exe')],env=env,check=True)
subprocess.run([str(out/'market.exe')],check=True)
