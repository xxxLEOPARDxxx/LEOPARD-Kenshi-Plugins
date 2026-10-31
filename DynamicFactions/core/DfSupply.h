#pragma once

enum DfFoodNeed { DF_FOOD_UNKNOWN, DF_FOOD_NOT_NEEDED, DF_FOOD_STOCKED,
                  DF_FOOD_RESTOCK, DF_FOOD_URGENT };
// Native nutrition/hunger units (1.0 = 100 displayed nutrition points).
// This decides procurement; it never changes hunger or creates food.
DfFoodNeed DfPlanFood(float hungerRate, float hunger, float nutrition,
                     bool alreadyRestocking, float& target);
float DfNaturalHungerFloor(float nativeFloor,float platoonFloor,bool ownFaction,bool organic,bool animal);
