#include "core/DfBudget.h"
#include <climits>

static void RunBudgetTests()
{
    DfBudgetSnapshot b;
    b.money=14578;b.foodDeficitCost=800;b.medicalDeficitCost=200;
    b.committed=500;b.emergencyBuffer=300;b.forecastKnown=true;
    DfBudgetDecision d=DfPlanSpend(b,10000,false);
    CHECK(d.Allowed());CHECK(d.reserve==1800);CHECK(d.freeForUpgrades==12778);
    CHECK(d.cashAfterPurchase==4578);
    CHECK(DfPlanSpend(b,12778,false).Allowed());
    CHECK(DfPlanSpend(b,12779,false).reason==DF_BUDGET_KEEP_RESERVE);
    CHECK(DfPlanSpend(b,14579,true).reason==DF_BUDGET_NO_CASH);
    CHECK(DfPlanSpend(b,0,true).reason==DF_BUDGET_INVALID);
    CHECK(DfPlanSpend(b,-1,false).reason==DF_BUDGET_INVALID);

    // Replacing gear cannot spend food money; urgent food can use that reserve.
    b.money=1000;b.foodDeficitCost=800;b.medicalDeficitCost=100;b.committed=0;b.emergencyBuffer=200;
    CHECK(!DfPlanSpend(b,700,false).Allowed());
    CHECK(DfPlanSpend(b,700,true).Allowed());
    CHECK(DfPlanSpend(b,700,true).cashAfterPurchase==300);
    b.committed=400;
    CHECK(DfPlanSpend(b,600,true).Allowed());
    CHECK(DfPlanSpend(b,601,true).reason==DF_BUDGET_COMMITTED);
    b.forecastKnown=false;
    CHECK(DfPlanSpend(b,1,false).reason==DF_BUDGET_UNKNOWN_FORECAST);
    CHECK(DfPlanSpend(b,600,true).Allowed());

    // Cost forecasts from multiple planners must not wrap around to cheap.
    b.forecastKnown=true;b.money=INT_MAX;b.committed=0;
    b.foodDeficitCost=INT_MAX;b.medicalDeficitCost=INT_MAX;b.emergencyBuffer=INT_MAX;
    d=DfPlanSpend(b,1,false);CHECK(d.reserve==INT_MAX);CHECK(d.freeForUpgrades==0);
    CHECK(!d.Allowed());CHECK(DfPlanSpend(b,INT_MAX,true).Allowed());
    b.foodDeficitCost=b.medicalDeficitCost=b.emergencyBuffer=0;
    CHECK(DfPlanSpend(b,INT_MAX,false).Allowed());
    b.money=-1;CHECK(DfPlanSpend(b,1,true).reason==DF_BUDGET_INVALID);
    b.money=1000;b.foodDeficitCost=-1;
    CHECK(DfPlanSpend(b,1,true).reason==DF_BUDGET_INVALID);
    b.foodDeficitCost=0;b.medicalDeficitCost=-1;
    CHECK(DfPlanSpend(b,1,true).reason==DF_BUDGET_INVALID);
    b.medicalDeficitCost=0;b.committed=-1;
    CHECK(DfPlanSpend(b,1,true).reason==DF_BUDGET_INVALID);
    b.committed=0;b.emergencyBuffer=-1;
    CHECK(DfPlanSpend(b,1,true).reason==DF_BUDGET_INVALID);
}
