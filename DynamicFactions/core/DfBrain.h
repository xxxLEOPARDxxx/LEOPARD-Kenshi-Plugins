#pragma once
// Решения фракции этапа 1: снимок игры на входе, действия на выходе.
// Без заголовков игры - деньги и люди меняет адаптер (game/).

#include <vector>

#include "core/DfState.h"

struct DfSnapshot
{
    int members;
    int money;
    double hour;
    bool homeLoaded;
};

enum DfActionType
{
    DF_FOUND,
    DF_RECRUIT,
    DF_INCOME,
    DF_RESEARCH,
    DF_EXTINCT
};

struct DfAction
{
    DfActionType type;
    int amount;                 // деньги (доход, найм) или очки (исследование)
};

struct DfRules
{
    int startMembers;
    int startMoney;
    int recruitCost;
    int maxMembers;
    int incomePerMemberHour;
    int techPointCost;
    int maxTech;
};

const int kDfReserve = 1000;            // неприкосновенный запас денег
const int kDfMaxResearchPerTick = 5;
const int kDfMaxTickHours = 24;

DfRules DfPhase1Rules();

// Тик - раз в целый игровой час с прошлого тика (пропущенное - не больше суток).
std::vector<DfAction> DfDecide(const DfFactionState& s, const DfSnapshot& snap, const DfRules& r);

// done - действия, которые адаптер реально выполнил. Техника, события,
// lastTickHour, founded/extinct.
void DfApplyBookkeeping(DfFactionState& s, const std::vector<DfAction>& done, double hour,
                        const DfRules& r = DfPhase1Rules());
