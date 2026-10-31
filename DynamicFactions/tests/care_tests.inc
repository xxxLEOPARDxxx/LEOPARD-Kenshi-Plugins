#include "core/DfSupply.h"

static void RunCareTests()
{
    DfSupplyStock s;s.known=true;s.foodRate=1;s.hunger=3;s.food=1;
    float target=0;
    CHECK(DfPlanSupply(s,DF_SUPPLY_FIRSTAID,false,target)==DF_FOOD_RESTOCK);
    CHECK(target==150);
    s.firstAid=50;CHECK(DfPlanSupply(s,DF_SUPPLY_FIRSTAID,false,target)==DF_FOOD_STOCKED);
    CHECK(DfPlanSupply(s,DF_SUPPLY_FIRSTAID,true,target)==DF_FOOD_RESTOCK);
    s.firstAid=150;CHECK(DfPlanSupply(s,DF_SUPPLY_FIRSTAID,true,target)==DF_FOOD_STOCKED);
    CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_NOT_NEEDED);
    s.robotLimbs=true;CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_RESTOCK);
    CHECK(target==500);s.repair=100;
    CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_STOCKED);
    CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,true,target)==DF_FOOD_RESTOCK);
    s.robot=true;s.foodRate=0;
    CHECK(DfPlanSupply(s,DF_SUPPLY_FOOD,false,target)==DF_FOOD_NOT_NEEDED);
    CHECK(DfPlanSupply(s,DF_SUPPLY_FIRSTAID,false,target)==DF_FOOD_NOT_NEEDED);
    s.repair=0;CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_RESTOCK);
    s.known=false;CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_UNKNOWN);
    s.known=true;s.repair=-1;CHECK(DfPlanSupply(s,DF_SUPPLY_REPAIR,false,target)==DF_FOOD_UNKNOWN);
    CHECK(DfDeficitCost(0,1,250)==0);CHECK(DfDeficitCost(0.5f,1,250)==250);
    CHECK(DfDeficitCost(1.01f,1,250)==500);CHECK(DfDeficitCost(500,500,4000)==4000);
    CHECK(DfDeficitCost(501,500,4000)==8000);CHECK(DfDeficitCost(1,0,10)==-1);
    CHECK(DfDeficitCost(1,1,0)==-1);CHECK(DfDeficitCost(-1,1,10)==-1);
    CHECK(DfDeficitCost(100,1,INT_MAX)==-1);
    CHECK(DfDeficitCost(std::numeric_limits<float>::quiet_NaN(),1,10)==-1);
    CHECK(DfOperatingBuffer(0)==1000);CHECK(DfOperatingBuffer(6)==1500);
    CHECK(DfOperatingBuffer(100)==25000);CHECK(DfOperatingBuffer(-1)==-1);
    DfBagSpec bag;bag.cells=36;bag.stackMult=1;bag.weight=2;bag.weightMult=.5f;
    bag.athletics=1;bag.combatSpeed=1;bag.combatBonus=0;bag.stealth=1;
    CHECK(DfBagScore(bag,true,1000)>0);
    const double small=DfBagScore(bag,true,1000);
    bag.cells=72;CHECK(DfBagScore(bag,true,1000)>small);
    bag.cells=36;CHECK(DfBagScore(bag,true,2000)<small);
    bag.combatBonus=-5;CHECK(DfBagScore(bag,true,1000)<0);
    CHECK(DfBagScore(bag,false,1000)>0);
    bag.combatBonus=0;bag.athletics=.5f;CHECK(DfBagScore(bag,false,1000)<0);
    bag.athletics=1;bag.weight=20;CHECK(DfBagScore(bag,false,1000)<0);
    bag.weight=2;bag.weightMult=std::numeric_limits<float>::infinity();
    CHECK(DfBagScore(bag,true,1000)<0);
    bag.weightMult=.5f;bag.cells=0;CHECK(DfBagScore(bag,true,1000)<0);
    bag.cells=36;CHECK(DfBagScore(bag,true,-1)<0);
    DfBudgetSnapshot budget;budget.money=1000;budget.foodDeficitCost=200;budget.medicalDeficitCost=500;
    budget.committed=100;budget.emergencyBuffer=100;budget.forecastKnown=true;
    CHECK(DfPlanSupplySpend(budget,600,DF_SUPPLY_REPAIR).Allowed());
    CHECK(!DfPlanSupplySpend(budget,601,DF_SUPPLY_REPAIR).Allowed());
    CHECK(DfPlanSupplySpend(budget,900,DF_SUPPLY_FOOD).Allowed());
    CHECK(DfPlanSupplySpend(budget,100,DF_SUPPLY_BACKPACK).Allowed());
    CHECK(!DfPlanSupplySpend(budget,101,DF_SUPPLY_BACKPACK).Allowed());
    budget.forecastKnown=false;CHECK(!DfPlanSupplySpend(budget,1,DF_SUPPLY_FIRSTAID).Allowed());
    CHECK(DfPlanSupplySpend(budget,900,DF_SUPPLY_FOOD).Allowed());
}
