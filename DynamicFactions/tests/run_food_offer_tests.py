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
const int ITEM_FOOD=3,ITEM_FOOD_RESTRICTED=15;
struct GameData{int id;GameData(int n=0):id(n){}};
struct Item{int quantity,itemFunction,price;bool isEquipped,deathItem,isTradeItem,stolen,edible;
 float chargesLeft,originalFullChargeAmount,quality;GameData* data;
 Item(GameData* d,int f=3):quantity(4),itemFunction(f),price(100),isEquipped(false),deathItem(false),isTradeItem(false),stolen(false),edible(true),chargesLeft(75),originalFullChargeAmount(75),quality(1),data(d){}
 bool isStolen(bool){return stolen;}GameData* getGameData(){return data;}std::string getName(){return "Food";}};
struct Inventory {int count;Inventory():count(0){}};
struct InventorySection {struct SectionItem{Item* item;SectionItem(Item* p):item(p){}};
 std::vector<SectionItem> items;const std::vector<SectionItem>& getItems(){return items;}};
namespace Ogre {template<class T>struct vector{typedef std::vector<T> type;};}
typedef std::vector<InventorySection*> StockSections;
struct Ownerships{int money;Ownerships(int n):money(n){}};
struct Faction{Ownerships* factionOwnerships;};
struct RaceData{bool canEat(Item* p,bool){return p->edible;}};
struct Character{Ownerships* purse;RaceData race;Inventory inventory;
 RaceData* getRace(){return &race;}void* isAnimal(){return NULL;}Inventory* getInventory(){return &inventory;}};
Ownerships* TraderPurse(Character* c){return c->purse;}
int UnitPrice(Item* p,Character*){return p->price;}
bool valid=true,trading=false,fault=false;int fit=0;
bool ValidDeal(Character*,Character*,Faction*){return valid;}bool PlayerTradingWith(Character*){return trading;}
int DfFirstInventoryFit(Inventory*,const std::vector<GameData*>& ranked){return ranked.empty()?-1:fit;}
void DfQuarantineTransfer(const char*){fault=true;}
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
start=source.index('    struct FoodOffer');end=source.index('    bool BuyFoodFromStock',start)
code+=source[start:end]+function('bool BuyFoodFromStock(')+r'''
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
 CHECK(!fault);printf("%d food offer checks, %d failed\n",checks,failed);return failed;}
'''
(out/'food_offer_tests.cpp').write_text(code,encoding='utf-8')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ);env['INCLUDE']=';'.join(map(str,(vc/'include',sdk/'Include',root)))
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(out/'food_offer_tests.cpp'),str(root/'core/DfTransfer.cpp'),str(root/'core/DfBudget.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'food_offer_tests.exe')],env=env,check=True)
subprocess.run([str(out/'food_offer_tests.exe')],check=True)
