#include "core/DfBudget.h"
#include <climits>

DfBudgetDecision DfPlanSpend(const DfBudgetSnapshot& b, int price, bool essential)
{
    DfBudgetDecision d;
    if (b.money < 0 || b.foodDeficitCost < 0 || b.medicalDeficitCost < 0 ||
        b.committed < 0 || b.emergencyBuffer < 0 || price <= 0)
        return d;
    const __int64 total = static_cast<__int64>(b.foodDeficitCost) +
        b.medicalDeficitCost + b.committed + b.emergencyBuffer;
    d.reserve = total > INT_MAX ? INT_MAX : static_cast<int>(total);
    d.freeForUpgrades = b.forecastKnown && b.money > d.reserve ? b.money - d.reserve : 0;
    d.cashAfterPurchase = b.money;
    if (price > b.money)
        d.reason = DF_BUDGET_NO_CASH;
    else if (b.committed > b.money - price)
        d.reason = DF_BUDGET_COMMITTED;
    else if (!essential && !b.forecastKnown)
        d.reason = DF_BUDGET_UNKNOWN_FORECAST;
    else if (!essential && price > d.freeForUpgrades)
        d.reason = DF_BUDGET_KEEP_RESERVE;
    else
    {
        d.reason = DF_BUDGET_ALLOWED;
        d.cashAfterPurchase = b.money - price;
    }
    return d;
}
