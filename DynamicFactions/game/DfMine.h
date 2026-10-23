#pragma once
// Добыча: жилы у города (любые, в том числе модовые - золото, уран, серебро),
// копание, руда в инвентаре. Только главный поток; всё под SEH.

#include <string>
#include <vector>

#include <ogre/OgreVector3.h>

#include <kenshi/util/hand.h>

class Character;
class Faction;
class GameData;
class TownBase;

// Жила: постройка с функцией BF_MINE или BF_MINE_NATURAL. Что даёт - по
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
};

// Жилы в радиусе от центра города (ZoneManager::getObjects). probe - член
// фракции, для которого проверяется свободное место; NULL - не проверять.
void DfFindNodes(TownBase* town, float radius, Character* probe, std::vector<DfNode>& out);

// Руда этого предмета в инвентаре.
int DfCountItem(Character* c, GameData* item);

// Приказ копать, как игрок: OrdersReceiver::addOrder(OPERATE_MACHINERY, жила).
bool DfOrderMine(Character* c, const DfNode& node);

// Текущая цель персонажа (TaskType) - для журнала; -1 - нет приказов.
int DfGoalOf(Character* c);
