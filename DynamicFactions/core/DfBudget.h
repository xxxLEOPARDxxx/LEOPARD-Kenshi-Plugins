#pragma once

// Forecast costs are the remaining deficits after counting real owned supplies.
// This is a spending decision, never another wallet or a source of money.
struct DfBudgetSnapshot
{
    int money;
    int foodDeficitCost;
    int medicalDeficitCost;
    int committed;
    int emergencyBuffer;
    bool forecastKnown;
    DfBudgetSnapshot() : money(0), foodDeficitCost(0), medicalDeficitCost(0),
        committed(0), emergencyBuffer(0), forecastKnown(false) {}
};

enum DfBudgetReason
{
    DF_BUDGET_INVALID,
    DF_BUDGET_NO_CASH,
    DF_BUDGET_COMMITTED,
    DF_BUDGET_UNKNOWN_FORECAST,
    DF_BUDGET_KEEP_RESERVE,
    DF_BUDGET_ALLOWED
};

struct DfBudgetDecision
{
    int reserve;
    int freeForUpgrades;
    int cashAfterPurchase;
    DfBudgetReason reason;
    DfBudgetDecision() : reserve(0), freeForUpgrades(0), cashAfterPurchase(0),
        reason(DF_BUDGET_INVALID) {}
    bool Allowed() const { return reason == DF_BUDGET_ALLOWED; }
};

// Essential means actual urgent food/medical supply, not a desired gear upgrade.
// An essential purchase can use its operating reserve, but not committed money.
DfBudgetDecision DfPlanSpend(const DfBudgetSnapshot& budget, int price, bool essential);
