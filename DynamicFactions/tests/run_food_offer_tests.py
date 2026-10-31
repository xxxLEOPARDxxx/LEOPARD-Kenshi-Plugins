"""Exercise the real shop-food selection and paid transfer coordinator."""
from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[1]
out=root.parents[1]/'build/DynamicFactionsFoodOfferCheck'
out.mkdir(parents=True,exist_ok=True)
source=(root/'game/DfSell.cpp').read_text(encoding='utf-8-sig')
def function(signature):
 start=source.index(signature);brace=source.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
code=r'''
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <float.h>
#include "core/DfBudget.h"
#include "core/DfTransfer.h"
#include "core/DfOffer.h"
const int ITEM_FOOD=3,ITEM_FOOD_RESTRICTED=15,ITEM_FIRSTAID=1,ITEM_ROBOTREPAIR=12,ITEM_CONTAINER=4,ATTACH_BACKPACK=12,ITEM=4,CONTAINER=46;
struct GameData{int id,type;std::string stringID;GameData(int n=0):id(n),type(ITEM),stringID("item") {}};
struct Item{int quantity,itemFunction,price;bool isEquipped,deathItem,isTradeItem,stolen,edible;
 int slotType;float chargesLeft,originalFullChargeAmount,quality;GameData* data;
 Item(GameData* d,int f=3):quantity(4),itemFunction(f),price(100),isEquipped(false),deathItem(false),isTradeItem(false),stolen(false),edible(true),slotType(0),chargesLeft(75),originalFullChargeAmount(75),quality(1),data(d){}
 bool isStolen(bool){return stolen;}GameData* getGameData(){return data;}std::string getName(){return "Food";}};
struct hand{unsigned type,container,containerSerial,index,serial;hand():type(1),container(0),containerSerial(0),index(42),serial(1){}};
struct Inventory {int count;hand id;Inventory():count(0){}const hand& getHandle(){return id;}};
struct InventorySection {struct SectionItem{Item* item;SectionItem(Item* p):item(p){}};
 Inventory root;Inventory* getInventory(){return &root;}
 std::vector<SectionItem> items;const std::vector<SectionItem>& getItems(){return items;}};
namespace Ogre {template<class T>struct vector{typedef std::vector<T> type;};}
typedef std::vector<InventorySection*> StockSections;
struct Ownerships{int money;Ownerships(int n):money(n){}};
struct Faction{Ownerships* factionOwnerships;};
struct RaceData{bool robot;RaceData():robot(false){}bool canEat(Item* p,bool){return p->edible;}};
struct Medical{bool robotic;Medical():robotic(false){}bool hasRobotics(){return robotic;}};
struct Character{Ownerships* purse;RaceData race;Medical medical;Inventory inventory;
 RaceData* getRace(){return &race;}Medical* getMedical(){return &medical;}void* isAnimal(){return NULL;}Inventory* getInventory(){return &inventory;}};
Ownerships* TraderPurse(Character* c){return c->purse;}
int UnitPrice(Item* p,Character*){return p->price;}
bool valid=true,trading=false,fault=false;int fit=0;
bool ValidDeal(Character*,Character*,Faction*){return valid;}bool PlayerTradingWith(Character*){return trading;}
int DfFirstInventoryFit(Inventory*,const std::vector<GameData*>& ranked){return ranked.empty()?-1:fit;}
void DfQuarantineTransfer(const char*){fault=true;}
bool DfReadBagSpec(Item*,DfBagSpec& spec,bool){spec.cells=36;spec.weight=2;spec.weightMult=.5f;return true;}
bool DfBagCanWear(Character*,Item*){return true;}
enum DfBuyStatus{DF_BUY_OK,DF_BUY_NO_FOOD,DF_BUY_NO_MONEY,DF_BUY_NO_ROOM,DF_BUY_PLAYER_TRADING,DF_BUY_INVALID,DF_BUY_FAULT};
class NativeFoodPurchase:public DfTradePort{
 Character* member_;Ownerships* buyer_;Ownerships* seller_;Item* item_;
 public:NativeFoodPurchase(Character* c,Ownerships* b,Ownerships* s,InventorySection*,Item* i):member_(c),buyer_(b),seller_(s),item_(i){}
 bool Read(int& b,int& s){b=buyer_->money;s=seller_->money;return true;}
 void WriteBuyer(int n){buyer_->money=n;}void WriteSeller(int n){seller_->money=n;}
 bool ReadGoods(int& s,int& b){s=item_->quantity;b=member_->inventory.count;return true;}
 DfUnitResult Deliver(){--item_->quantity;++member_->inventory.count;return DF_UNIT_MOVED;}
 bool ReturnGoods(){return false;}};
'''
start=source.index('    struct FoodOffer');end=source.index('    bool SupplyQuoteFromItem',start)
code+=source[start:end]+function('bool BuyFoodFromStock(')+'\n'
for signature in ('bool SupplyQuoteFromItem(', 'bool CollectSupplyQuotes(', 'bool BuyOtherSupplyFromStock(', 'bool SupplyFromStock('):code+=function(signature)+'\n'
code+=r'''
int checks=0,failed=0;
#define CHECK(x) do{++checks;if(!(x)){++failed;printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
int main(){Ownerships wallet(1000),shopMoney(5000);Faction faction={&wallet};Character member,trader;trader.purse=&shopMoney;
 GameData ordinary(1),modded(2);Item food(&ordinary),mod(&modded,ITEM_FOOD_RESTRICTED);InventorySection shelf;
 shelf.items.push_back(&food);StockSections sections(1,&shelf);std::vector<FoodOffer> offers;std::vector<GameData*> ranked;
 int paid=0;std::string name;DfBuyStatus status=DF_BUY_INVALID;
 // Captured real bartender food has isTradeItem=0. This is not a sale gate.
 CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));
 CHECK(status==DF_BUY_OK && paid==100);CHECK(food.quantity==3 && member.inventory.count==1);
 CHECK(wallet.money==900 && shopMoney.money==5100);
 // Native edibility, not an ID/name allowlist, admits modded restricted food.
 shelf.items.clear();shelf.items.push_back(&mod);paid=0;
 CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));
 CHECK(status==DF_BUY_OK && paid==100 && mod.quantity==3);
 mod.edible=false;paid=0;CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_FOOD && paid==0);
 mod.edible=true;mod.stolen=true;CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_FOOD);
 mod.stolen=false;mod.chargesLeft=40;CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_FOOD);
 mod.chargesLeft=75;trading=true;CHECK(BuyFoodFromStock(&member,&trader,&faction,0,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_PLAYER_TRADING && wallet.money==800);
 trading=false;CHECK(BuyFoodFromStock(&member,&trader,&faction,800,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_MONEY && wallet.money==800);
 // The shared physical purchase supports a robot repair kit, but does not buy
 // first aid for that robot. Planned medicine protects food and the buffer.
 wallet.money=1000;shopMoney.money=5000;member.inventory.count=0;member.race.robot=true;
 mod.itemFunction=ITEM_ROBOTREPAIR;mod.quantity=2;mod.chargesLeft=mod.originalFullChargeAmount=500;
 DfBudgetSnapshot budget;budget.money=1000;budget.foodDeficitCost=200;budget.emergencyBuffer=100;budget.forecastKnown=true;
 std::vector<DfSupplyQuote> observed;paid=0;
 CHECK(CollectSupplyQuotes(&trader,sections,observed));CHECK(observed.size()==2 && observed[1].kind==DF_SUPPLY_REPAIR && observed[1].amount==500);
 CHECK(BuyOtherSupplyFromStock(&member,&trader,&faction,DF_SUPPLY_REPAIR,true,budget,sections,offers,ranked,paid,name,status));
 CHECK(status==DF_BUY_OK && paid==100 && mod.quantity==1 && wallet.money==900);
 mod.itemFunction=ITEM_FIRSTAID;paid=0;
 CHECK(BuyOtherSupplyFromStock(&member,&trader,&faction,DF_SUPPLY_FIRSTAID,true,budget,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_FOOD && paid==0);
 member.race.robot=false;mod.itemFunction=ITEM_ROBOTREPAIR;
 CHECK(BuyOtherSupplyFromStock(&member,&trader,&faction,DF_SUPPLY_REPAIR,true,budget,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_FOOD);
 member.medical.robotic=true;budget.foodDeficitCost=850;
 CHECK(BuyOtherSupplyFromStock(&member,&trader,&faction,DF_SUPPLY_REPAIR,true,budget,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_NO_MONEY && wallet.money==900);
 // A backpack uses the full reserve and keeps the food/medicine transaction path.
 budget.foodDeficitCost=0;mod.data->type=CONTAINER;mod.itemFunction=ITEM_CONTAINER;mod.slotType=ATTACH_BACKPACK;mod.quantity=1;mod.chargesLeft=mod.originalFullChargeAmount=1;
 CHECK(BuyOtherSupplyFromStock(&member,&trader,&faction,DF_SUPPLY_BACKPACK,true,budget,sections,offers,ranked,paid,name,status));CHECK(status==DF_BUY_OK && paid==100 && mod.quantity==0 && wallet.money==800);
 CHECK(!fault);
 // Post-purchase market knowledge must not retain the exhausted last unit.
 mod.data->type=ITEM;mod.itemFunction=ITEM_FIRSTAID;mod.quantity=1;mod.chargesLeft=mod.originalFullChargeAmount=100;paid=0;
 observed.clear();
 CHECK(SupplyFromStock(&member,&trader,&faction,DF_SUPPLY_FIRSTAID,true,budget,sections,offers,ranked,paid,name,observed,status));
 CHECK(paid==100 && observed.size()==1 && observed[0].quantity==0 && observed[0].pool.index==42 && mod.quantity==0);
 mod.quantity=3;Item another(&modded,ITEM_FIRSTAID);another.chargesLeft=another.originalFullChargeAmount=100;another.quantity=2;shelf.items.push_back(&another);
 CHECK(CollectSupplyQuotes(&trader,sections,observed));CHECK(observed.size()==2 && observed[1].quantity==5);
 // Even a full observation retains the last empty pool's invalidation marker.
 std::vector<InventorySection> many(128);StockSections lots;
 for(int i=0;i<128;++i){many[i].root.id.index=2000+i;lots.push_back(&many[i]);}
 many[0].items.push_back(&mod);CHECK(CollectSupplyQuotes(&trader,lots,observed));
 CHECK(observed.size()==128 && observed.back().pool.index==2127 && observed.back().quantity==0);
 printf("%d food/medical/backpack offer checks, %d failed\n",checks,failed);return failed;}
'''
(out/'food_offer_tests.cpp').write_text(code,encoding='utf-8')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ);env['INCLUDE']=';'.join(map(str,(vc/'include',sdk/'Include',root)))
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(out/'food_offer_tests.cpp'),str(root/'core/DfTransfer.cpp'),str(root/'core/DfBudget.cpp'),str(root/'core/DfSupply.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'food_offer_tests.exe')],env=env,check=True)
subprocess.run([str(out/'food_offer_tests.exe')],check=True)
