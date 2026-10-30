#pragma once
// Продажа добычи торговцам города. Только главный поток; всё под SEH.

#include <string>
#include <vector>

#include <kenshi/util/hand.h>

class Character;
class Faction;
class GameData;
class TownBase;

// Торговец - живой персонаж города с флагом шаблона «is trader» (как в
// RecruitMarkers: не Character::isATrader - та «да» и охране лавки).
struct DfTrader
{
    hand who;
    std::string name;
    int money;                  // деньги его взвода (Ownerships) - сколько может заплатить
    float distance;             // от центра города
};

void DfFindTraders(TownBase* town, std::vector<DfTrader>& out);

// Read-only selection predicate. Excludes distance/current hauler town so the
// worker can approach from an ore node; checks actual price/purse/capacity.
bool DfTraderCanBuy(Character* hauler, Character* trader, Faction* f, GameData* item);

// Conservative subset of native interaction ranges: InventoryGUI uses 60 for
// restricted interactions, DialogueController allows 260 for ordinary talking.
const float kDfTradeRange = 60.0f;

// Only real items, maxUnits <= 64, delivered to the merchant. Price follows the
// verified native NPC receiving formula with the explicit trader/town, independent
// of the player's trading GUI. Merchant purse -> faction purse, money conserved.
// false means stop/reconcile; outputs retain prior confirmed deals. No replay.
bool DfSellOre(Character* hauler, Character* trader, Faction* f, GameData* item, int maxUnits, int& sold,
               int& paid);
