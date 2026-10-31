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
start = state.index('struct DfProvision\n')
end = state.index('\n};', start) + len('\n};')
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
 hand id;std::string name;Ogre::Vector3 pos;float rate,hunger,food,original;
 int goal,clears,moves,price,inSomething;StateBroadcastData state;bool dead,ko,combat,animal,noRoom;
 Character(int i,Faction* faction,const char* n,float x=0):f(faction),id(i),name(n),pos(x,0,0),rate(1),hunger(2.5f),food(0.4f),original(3),
 goal(87),clears(0),moves(0),price(100),inSomething(0),dead(false),ko(false),combat(false),animal(false),noRoom(false)
 {actors[i]=this;platoon.f=f;platoon.malnourishedLevel=3;active.me=&platoon;}
 Faction* getFaction(){return f;}ActivePlatoon* getPlatoon(){return &active;}RaceData* getRace(){return &race;}
 StateBroadcastData* getStateBroadcast(){return &state;}
 void* isAnimal(){return animal ? this : NULL;}bool isDead(){return dead;}bool isUnconcious(){return ko;}
 bool isInCombatMode(bool,bool){return combat;}bool isEnemy(Character*,bool){return false;}
 Ogre::Vector3 getPosition(){return pos;}const hand& getHandle(){return id;}std::string getName(){return name;}
 void setDestination(const Ogre::Vector3&,bool){goal=MOVE_ON_FREE_WILL;++moves;}
};
Character* hand::getCharacter()const {std::map<int,Character*>::iterator it=actors.find(index);return it==actors.end()?NULL:it->second;}
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
'''
code += state[start:end] + '\n'
code += function(mine, 'bool DfEconomyAvailable(') + '\n'
code += function(sell, 'bool SafeBuyFood(') + '\n'
code += function(sell, 'bool DfBuyFood(') + '\n'
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
'''
for signature in ('bool Same(', 'bool Future(', 'void ClearRoute(', 'void Delay(',
                  'bool Failed(', 'Character* ChooseTrader(', 'bool DfProvisionOwns(', 'void DfProvisionStep('):
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
                str(out / 'supply_adapter_tests.cpp'), str(root / 'core/DfSupply.cpp'),
                '/Fo' + str(out) + '\\', '/Fe' + str(out / 'supply_adapter_tests.exe')],
               env=env, check=True)
subprocess.run([str(out / 'supply_adapter_tests.exe')], check=True)
