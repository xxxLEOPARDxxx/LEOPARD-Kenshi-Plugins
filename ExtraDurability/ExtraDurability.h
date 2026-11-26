#pragma once
#include <string>

class Character;
class Gear;
class GameData;
class Item;

namespace durability
{
bool IsSupported(Gear* item);
bool IsDamaged(Gear* item);
std::string ConditionLabel(Gear* item);
bool AppliesTo(Character* character);
float Effect(Gear* item);
float ArmourEffectAndWear(Gear* item);
void Wear(Gear* item);
bool HasPendingPlayerDestruction(Gear* item);
void QueueRestoredBrokenEquipment(Gear* item, bool playerPending);
bool HasRepairKit(Character* character);
bool TryRepair(Character* repairer, Gear* item);
std::string Tr(const char* english);

void InstallCombat();
void InstallRepairMenu();

bool IsRepairTool(const GameData* data);
Item* FindRepairTool(Character* character);
void InstallRepairToolState();

int ConditionBarPosition();
bool HasConditionBar(const Item* item);
float EquipmentConditionRatio(Item* item);
void InstallConditionBar();
void TickConditionBars(float time);
}
