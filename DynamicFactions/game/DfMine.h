#pragma once
// Добыча: жилы у города (любые, в том числе модовые - золото, уран, серебро),
// копание, руда в инвентаре. Только главный поток; всё под SEH.

#include <string>
#include <vector>

#include <ogre/OgreVector3.h>

#include <kenshi/util/hand.h>

class Character;
class Building;
class Faction;

// Same ownership rule for discovery and pickup, using FactionManager::getEmptyFaction.
bool DfCanHarvestNode(Building* node, Faction* workerFaction);
class Faction;
class GameData;
class TownBase;
class Inventory;
class Item;

const float kDfTakeRange = 6.0f;
const int kDfTransferMaxUnits = 64;

// Жила: постройка с функцией BF_MINE_NATURAL или BF_MINE с «is foliage» в
// данных (естественные залежи: железо, медь, камень, модовые из GenMod -
// олово, уран, золото, серебро; без флага BF_MINE - фермы и колодцы). Что даёт - по
// данным: запись постройки -> «functionality» -> «produces» -> предмет
// (у меди «Copper Resource» -> «copper mine» -> предмет 42158). Ничего не
// зашито - модовые жилы находятся так же.
struct DfNode
{
    hand building;
    Ogre::Vector3 pos;
    GameData* item;             // что даёт; NULL - не нашли
    std::string itemName;
    int itemValue;              // value предмета; <= 0 - жила не берётся
    float distance;             // от центра города
    int function;               // BF_MINE / BF_MINE_NATURAL - для журнала разведки
    bool freeSlot;              // есть место для члена фракции (UseableStuff::isFreeSlot)
    int capacity;               // actual output footprint/stack/quota; -1 = unknown
    int maxMiners;              // native numOperatorsMax, 0 = unavailable
    int freeSlots;              // max minus all current native operators
    float takeRange;           // native UseableStuff::maxUseRange; fallback6
};

// Жилы в радиусе от точки (ZoneManager::buildingGrid). Точка - где люди
// фракции: точка города (TownBase::getPosition) - не центр (разведка 23.10:
// торговцы Хаба «в 2 км» от неё). Берутся и постройки-«растительность»
// (FOLIAGE_BUILDING: у «Copper Resource» is foliage = 1). probe - член
// фракции, для которого проверяется свободное место; NULL - не проверять.
void DfFindNodes(const Ogre::Vector3& from, float radius, Character* probe, std::vector<DfNode>& out);

// Разведка: что вокруг точки - сколько объектов каждого типа (itemType) и
// функций построек (BuildingFunction), строкой для журнала.
std::string DfDescribeArea(const Ogre::Vector3& from, float radius);

// Руда этого предмета в инвентаре.
int DfCountItem(Character* c, GameData* item);

// Сколько этого предмета лежит в самой жиле (её инвентарь); -1 - у жилы нет инвентаря.
int DfCountInNode(const hand& node, GameData* item);

// Разведка: что игра сама предлагает этому персонажу для работы на добыче
// (AI::findMineToWorkAt, как у шахтёров городов) - имя постройки, функция,
// расстояние; пусто - ничего.
std::string DfGameMineFor(Character* c);

// Разведка: постройки в радиусе от точки, имя которых (в данных) не пустое,
// с функцией добычи (1, 27) - имя, функция, foliage, тип; до max штук.
std::string DfListMinesNear(const Ogre::Vector3& from, float radius, int max);

// Жила в этой точке сейчас загружена (в пределах 10 м)? Тогда она - в out.
bool DfLoadedNodeAt(const Ogre::Vector3& pos, DfNode& out);

// Предмет по stringID и его value (0 - нет такого или без цены).
GameData* DfItemBySid(const std::string& sid, int& value);

// Actual confirmed delivered units, bounded by kDfTransferMaxUnits. Never drop
// or destroy on failure. An ambiguous native mutation quarantines transfers.
int DfTakeOreFromNode(Character* c, const hand& node, GameData* item, int maxUnits);

// Recover actual, non-equipped cargo from main inventory/containers. All known
// types are considered. false+units0 = none; false+units-1 = unknown/fault.
bool DfFindCarriedOre(Character* c, const std::vector<GameData*>& types, GameData*& item, int& units);

// Session quarantine: false means the caller must stop work and reconcile.
bool DfTransferFaulted();
bool DfTransferBusy();
void DfResetTransferFault(); // lifecycle only, before handles of the old world expire

// Shared native primitives for DfSell. Borrowed SDK inventory view, no lektor
// allocation. A true add is never retried/rolled back (KEO may merge/move it).
Item* DfFirstInventoryItem(Inventory* root, GameData* item, Inventory*& actual, bool onlyClean = false);
int DfCountInventoryItem(Inventory* root, GameData* item);
bool DfInventoryHasRoom(Inventory* root, GameData* item);
bool DfMoveInventoryUnit(Inventory* from, Inventory* actual, Item* source, Inventory* to, GameData* item);
void DfQuarantineTransfer(const char* reason);

// Снять приказы персонажа (работа кончилась).
void DfClearOrders(Character* c);

// Приказ копать, как игрок: OrdersReceiver::addOrder(OPERATE_MACHINERY, жила).
bool DfOrderMine(Character* c, const DfNode& node);

// Текущая цель персонажа (TaskType) - для журнала; -1 - нет приказов.
int DfGoalOf(Character* c);
