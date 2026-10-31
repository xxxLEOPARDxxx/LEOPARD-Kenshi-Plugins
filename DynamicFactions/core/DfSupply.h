#pragma once
#include "core/DfBudget.h"

enum DfFoodNeed { DF_FOOD_UNKNOWN, DF_FOOD_NOT_NEEDED, DF_FOOD_STOCKED,
                  DF_FOOD_RESTOCK, DF_FOOD_URGENT };
// Native nutrition/hunger units (1.0 = 100 displayed nutrition points).
// This decides procurement; it never changes hunger or creates food.
DfFoodNeed DfPlanFood(float hungerRate, float hunger, float nutrition,
                     bool alreadyRestocking, float& target);
float DfNaturalHungerFloor(float nativeFloor,float platoonFloor,bool ownFaction,bool organic,bool animal);

enum DfSupplyKind { DF_SUPPLY_FOOD, DF_SUPPLY_FIRSTAID, DF_SUPPLY_REPAIR, DF_SUPPLY_BACKPACK };
struct DfSupplyStock
{
    bool known,robot,robotLimbs,animal;
    float foodRate,hunger,food,firstAid,repair;
    int ownedBags;
    bool wearingBag,light;
    DfSupplyStock():known(false),robot(false),robotLimbs(false),animal(false),foodRate(0),hunger(0),
        food(0),firstAid(0),repair(0),ownedBags(0),wearingBag(false),light(true) {}
};
struct DfBagSpec
{
    int cells,combatBonus;
    float stackMult,weight,weightMult,athletics,combatSpeed,stealth;
    DfBagSpec():cells(0),combatBonus(0),stackMult(1),weight(0),weightMult(1),athletics(1),combatSpeed(1),stealth(1) {}
};
DfFoodNeed DfPlanSupply(const DfSupplyStock& stock,DfSupplyKind kind,bool restocking,float& target);
int DfDeficitCost(float deficit,float unitAmount,int price);
int DfOperatingBuffer(int members);
double DfBagScore(const DfBagSpec& bag,bool light,int price);
DfBudgetDecision DfPlanSupplySpend(const DfBudgetSnapshot& budget,int price,DfSupplyKind kind);
