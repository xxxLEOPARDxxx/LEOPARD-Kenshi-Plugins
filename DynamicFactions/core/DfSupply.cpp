#include "core/DfSupply.h"
#include <float.h>
#include <climits>
#include <cmath>

DfBudgetDecision DfPlanSupplySpend(const DfBudgetSnapshot& b,int price,DfSupplyKind kind)
{
    if(kind==DF_SUPPLY_FOOD) return DfPlanSpend(b,price,true);
    if(kind==DF_SUPPLY_BACKPACK) return DfPlanSpend(b,price,false);
    if(kind!=DF_SUPPLY_FIRSTAID && kind!=DF_SUPPLY_REPAIR) return DfBudgetDecision();
    if(!b.forecastKnown) {DfBudgetDecision d;d.reason=DF_BUDGET_UNKNOWN_FORECAST;return d;}
    // Planned medicine may use its own reserve, but not food, commitments or
    // the operating buffer. Revalidate all amounts before combining them.
    if(b.foodDeficitCost<0 || b.medicalDeficitCost<0 || b.committed<0 || b.emergencyBuffer<0) return DfBudgetDecision();
    const __int64 protectedCash=static_cast<__int64>(b.foodDeficitCost)+b.committed+b.emergencyBuffer;
    if(protectedCash>INT_MAX) return DfBudgetDecision();
    DfBudgetSnapshot spend=b;spend.committed=static_cast<int>(protectedCash);
    spend.foodDeficitCost=spend.medicalDeficitCost=spend.emergencyBuffer=0;
    return DfPlanSpend(spend,price,true);
}

DfFoodNeed DfPlanSupply(const DfSupplyStock& s,DfSupplyKind kind,bool restocking,float& target)
{
    target=0;
    if(!s.known) return DF_FOOD_UNKNOWN;
    if(kind==DF_SUPPLY_FOOD)
        return s.robot ? DF_FOOD_NOT_NEEDED : DfPlanFood(s.foodRate,s.hunger,s.food,restocking,target);
    if(kind!=DF_SUPPLY_FIRSTAID && kind!=DF_SUPPLY_REPAIR) return DF_FOOD_UNKNOWN;
    if(s.animal || (kind==DF_SUPPLY_FIRSTAID ? s.robot : !s.robot && !s.robotLimbs)) return DF_FOOD_NOT_NEEDED;
    const float held=kind==DF_SUPPLY_FIRSTAID ? s.firstAid : s.repair;
    if(!_finite(held) || held<0) return DF_FOOD_UNKNOWN;
    target=kind==DF_SUPPLY_FIRSTAID ? 150.0f : 500.0f;
    const float low=kind==DF_SUPPLY_FIRSTAID ? 50.0f : 100.0f;
    return held>=(restocking ? target : low) ? DF_FOOD_STOCKED : DF_FOOD_RESTOCK;
}
int DfDeficitCost(float deficit,float amount,int price)
{
    if(!_finite(deficit) || !_finite(amount) || deficit<0 || amount<=0 || price<=0) return -1;
    const double units=std::ceil(static_cast<double>(deficit)/amount);
    if(units>INT_MAX/price) return -1;
    return static_cast<int>(units)*price;
}
int DfOperatingBuffer(int members)
{
    if(members<0 || members>INT_MAX/250) return -1;
    const int scaled=members*250;
    return scaled>1000 ? scaled : 1000;
}
double DfBagScore(const DfBagSpec& b,bool light,int price)
{
    if(price<0 || b.cells<=0 || b.cells>4096 || !_finite(b.stackMult) || b.stackMult<1 || b.stackMult>256 ||
       !_finite(b.weight) || b.weight<0 || b.weight>(light ? 5.0f : 12.0f) ||
       !_finite(b.weightMult) || b.weightMult<0 || b.weightMult>1 ||
       !_finite(b.athletics) || b.athletics<(light ? .9f : .8f) || b.athletics>4 ||
       !_finite(b.combatSpeed) || b.combatSpeed<(light ? .95f : .8f) || b.combatSpeed>4 ||
       !_finite(b.stealth) || b.stealth<(light ? .9f : .1f) || b.stealth>4 ||
       b.combatBonus<(light ? -2 : -6) || b.combatBonus>100) return -1;
    const double combat=1.0+b.combatBonus*.03;
    const double capacity=static_cast<double>(b.cells)*b.stackMult;
    // Useful capacity and burden/penalties matter; price prevents automatically
    // buying the most expensive modded bag when a cheaper one serves the role.
    return capacity*(2.0-b.weightMult)*b.athletics*b.combatSpeed*combat/
        ((1.0+price/1000.0)*(1.0+b.weight*.1));
}

float DfNaturalHungerFloor(float nativeFloor,float platoonFloor,bool own,bool organic,bool animal)
{
    // Preserve animal options, robots and special native slave/player states.
    // No world-ready cache: loading must not grant food before our first tick.
    return own && organic && !animal && _finite(nativeFloor) && _finite(platoonFloor) &&
        nativeFloor>=0 && nativeFloor==platoonFloor ? -10.0f : nativeFloor;
}

DfFoodNeed DfPlanFood(float rate, float hunger, float nutrition, bool restocking, float& target)
{
    target = 0.0f;
    if (!_finite(rate) || !_finite(hunger) || !_finite(nutrition) || rate < 0 || nutrition < 0)
        return DF_FOOD_UNKNOWN;
    if (rate == 0) return DF_FOOD_NOT_NEEDED;
    // Bounded stock target, with a low-water mark to avoid a shopping trip
    // for each mouthful. Race/exertion rate is supplied by the native engine.
    target = rate > 4.0f ? 4.0f : rate;
    if (nutrition >= (restocking ? target : target * 0.5f)) return DF_FOOD_STOCKED;
    return hunger < 2.1f ? DF_FOOD_URGENT : DF_FOOD_RESTOCK;
}
