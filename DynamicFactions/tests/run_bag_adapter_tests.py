"""Compile production bag inspection/equip against bounded native-port fixtures."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[1];out=root.parents[1]/'build/DynamicFactionsBagCheck';out.mkdir(parents=True,exist_ok=True)
text=(root/'game/DfMine.cpp').read_text(encoding='utf-8-sig')
def function(signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
code=r'''
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <vector>
#include <climits>
#include <float.h>
#include <cstdio>
#include "core/DfSupply.h"
typedef unsigned int uint32_t;
template<class T>struct lektor{std::vector<T> v;T* stuff;lektor():stuff(NULL){}unsigned size()const{return (unsigned)v.size();}void sync(){stuff=v.empty()?NULL:&v[0];}};
namespace Ogre{template<class T>struct vector{typedef std::vector<T> type;};}
const int CONTAINER=46,ITEM_CONTAINER=4,ATTACH_BACKPACK=12,STAT_STRENGTH=1,STAT_ATHLETICS=2;
struct Item;struct Inventory;Item* resolved=NULL;
int transferDepth=0;bool equipSawBusy=false,equipFault=false;
struct TransferGuard{TransferGuard(){++transferDepth;}~TransferGuard(){--transferDepth;}};
bool DfTransferBusy(){return transferDepth!=0;}
struct hand{int type,container,containerSerial,index,serial;hand(int i=0):type(2),container(0),containerSerial(0),index(i),serial(1){}Item* getItem()const{return resolved;}};
struct GameData{int type;GameData():type(CONTAINER){}};
struct InventorySection {struct SectionItem{Item* item;SectionItem(Item* p):item(p){}};std::vector<SectionItem> items;
 int width,height,stackingBonusMin,limitedSlot;float stackingBonusMult;bool enabled,isAnEquippedItemSection,compatible;
 Inventory* parent;
 InventorySection():width(6),height(6),stackingBonusMin(1),limitedSlot(ATTACH_BACKPACK),stackingBonusMult(1),enabled(true),isAnEquippedItemSection(true),compatible(true),parent(NULL){}
 Inventory* getInventory(){return parent;}
 const std::vector<SectionItem>& getItems()const{return items;}bool isLimitedSlotCompatible(Item*){return compatible;}
 bool _NV_hasRoomForItem(GameData*,int){return items.empty();}};
struct Item{GameData data;Inventory* inventory;bool isEquipped,isTradeItem,deathItem;int itemFunction,slotType,quantity;float weight;hand id;
 Item():inventory(NULL),isEquipped(false),isTradeItem(false),deathItem(false),itemFunction(ITEM_CONTAINER),slotType(ATTACH_BACKPACK),quantity(1),weight(2),id(1){}
 GameData* getGameData(){return &data;}Inventory* getInventory(){return inventory;}const hand& getHandle(){return id;}};
struct ContainerItem:Item{float athleticsMult,weightMult,combatSpeedMult,stealthMult;int combatSkillBonus;bool wearable;
 ContainerItem():athleticsMult(1),weightMult(.5f),combatSpeedMult(1),stealthMult(1),combatSkillBonus(0),wearable(true){}
 bool _NV_canEquip(GameData*){return wearable;}};
struct Inventory{lektor<Item*> all;lektor<InventorySection*> sections;bool equipSuccess,lose;int calls;
 Inventory():equipSuccess(true),lose(false),calls(0){}const lektor<Item*>& getAllItems(){all.sync();return all;}
 const lektor<InventorySection*>& getAllSections(){sections.sync();return sections;}
 bool hasItem(Item* p){for(size_t i=0;i<all.v.size();++i)if(all.v[i]==p)return true;for(size_t i=0;i<sections.v.size();++i)for(size_t j=0;j<sections.v[i]->items.size();++j)if(sections.v[i]->items[j].item==p)return true;return false;}
 bool equipItem(Item* p){++calls;equipSawBusy=DfTransferBusy();if(equipFault)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,NULL);if(lose){all.v.clear();resolved=NULL;return false;}if(equipSuccess){p->isEquipped=true;sections.v[0]->items.push_back(InventorySection::SectionItem(p));}return equipSuccess;}};
struct RaceData{GameData dataValue;GameData* data;RaceData():data(&dataValue){}};
struct CharStats{float strength,athletics;CharStats():strength(10),athletics(30){}float getStat(int s,bool){return s==STAT_STRENGTH?strength:athletics;}};
struct Character{Inventory* inv;RaceData race;CharStats stats;Character(Inventory* p):inv(p){}Inventory* getInventory(){return inv;}RaceData* getRace(){return &race;}CharStats* getStats(){return &stats;}void* isAnimal(){return NULL;}};
bool fault=false;void DfQuarantineTransfer(const char*){fault=true;}
bool DfTransferFaulted(){return fault;}
int DfCountOwnedInventoryItem(Inventory* inv,GameData*){return resolved && inv->hasItem(resolved)?1:0;}
bool SameTitle(const hand& a,const hand& b){return a.index==b.index && a.serial==b.serial;}
'''
for signature in ('bool ReadBagSpec(', 'bool BagCanWear(', 'bool EquipOwnedBag(', 'bool LightBagRole(', 'static bool SafeEquipOwnedBag(', 'bool DfEquipOwnedBag('):code+=function(signature)+'\n'
code+=r'''
int checks=0,failed=0;
#define CHECK(x) do{++checks;if(!(x)){++failed;printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
int main(){Inventory root,inside;InventorySection slot,content;slot.parent=&root;content.parent=&inside;content.isAnEquippedItemSection=false;root.sections.v.push_back(&slot);inside.sections.v.push_back(&content);
 ContainerItem bag;bag.inventory=&inside;root.all.v.push_back(&bag);resolved=&bag;Character actor(&root);DfBagSpec spec;
 CHECK(ReadBagSpec(&bag,spec,true));CHECK(spec.cells==36 && spec.weight==2 && spec.weightMult==.5f);
 Item cargo;content.items.push_back(InventorySection::SectionItem(&cargo));CHECK(!ReadBagSpec(&bag,spec,true));CHECK(ReadBagSpec(&bag,spec,false));content.items.clear();
 bag.wearable=false;CHECK(!BagCanWear(&actor,&bag));bag.wearable=true;CHECK(BagCanWear(&actor,&bag));
 CHECK(LightBagRole(&actor));actor.stats.strength=50;actor.stats.athletics=30;CHECK(!LightBagRole(&actor));
 bool equipped=false;root.equipSuccess=false;CHECK(EquipOwnedBag(&actor,true,equipped));CHECK(!equipped && root.calls==1 && !fault);
 root.equipSuccess=true;CHECK(EquipOwnedBag(&actor,true,equipped));CHECK(equipped && root.calls==2 && !fault);
 CHECK(EquipOwnedBag(&actor,true,equipped));CHECK(equipped && root.calls==2);
 bag.isEquipped=false;slot.items.clear();root.lose=true;CHECK(!EquipOwnedBag(&actor,true,equipped));CHECK(fault);
 fault=false;root.lose=false;root.all.v.push_back(&bag);resolved=&bag;
 CHECK(DfEquipOwnedBag(&actor,true,equipped));CHECK(equipped && equipSawBusy && transferDepth==0);
 bag.isEquipped=false;slot.items.clear();equipFault=true;
 CHECK(!DfEquipOwnedBag(&actor,true,equipped));CHECK(fault && transferDepth==0);
 printf("%d bag adapter checks, %d failed\n",checks,failed);return failed;}
'''
(out/'bag.cpp').write_text(code,encoding='utf-8')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ);env['INCLUDE']=';'.join(map(str,(vc/'include',sdk/'Include',root)));env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(out/'bag.cpp'),str(root/'core/DfSupply.cpp'),str(root/'core/DfBudget.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(out/'bag.exe')],env=env,check=True)
subprocess.run([str(out/'bag.exe')],check=True)
