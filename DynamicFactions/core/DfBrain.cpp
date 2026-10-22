#include "core/DfBrain.h"

#include <cmath>
#include <cstdio>

namespace
{
    DfAction Make(DfActionType type, int amount)
    {
        DfAction a;
        a.type = type;
        a.amount = amount;
        return a;
    }

    int WholeHours(double from, double to)
    {
        const double h = std::floor(to - from);
        return h < 1.0 ? 0 : static_cast<int>(h < 1e6 ? h : 1e6);
    }

    std::string Text(const char* fmt, int value)
    {
        char buf[64];
        sprintf_s(buf, sizeof(buf), fmt, value);
        return buf;
    }
}

DfRules DfPhase1Rules()
{
    DfRules r;
    r.startMembers = 4;
    r.startMoney = 5000;
    r.recruitCost = 1500;
    r.maxMembers = 30;
    r.incomePerMemberHour = 15;
    r.techPointCost = 200;
    r.maxTech = 10;
    return r;
}

std::vector<DfAction> DfDecide(const DfFactionState& s, const DfSnapshot& snap, const DfRules& r)
{
    std::vector<DfAction> acts;
    if (s.extinct)
        return acts;
    if (!s.founded)
    {
        if (snap.homeLoaded)
            acts.push_back(Make(DF_FOUND, r.startMoney));
        return acts;
    }
    if (snap.members <= 0)
    {
        acts.push_back(Make(DF_EXTINCT, 0));
        return acts;
    }

    int hours = WholeHours(s.lastTickHour, snap.hour);
    if (hours == 0)
        return acts;
    if (hours > kDfMaxTickHours)
        hours = kDfMaxTickHours;

    const int income = hours * snap.members * r.incomePerMemberHour;
    acts.push_back(Make(DF_INCOME, income));
    int money = snap.money + income;

    // Найм - как у игрока: кто сидит в баре и сколько просит (его диалог),
    // адаптер узнаёт заранее (snap.recruitPrice).
    bool recruited = false;
    if (snap.homeLoaded && snap.recruitPrice >= 0 && snap.members < r.maxMembers &&
        money >= snap.recruitPrice + kDfReserve)
    {
        acts.push_back(Make(DF_RECRUIT, snap.recruitPrice));
        money -= snap.recruitPrice;
        recruited = true;
    }

    // На технику - только сверх запаса и, пока людей меньше предела, сверх
    // денег на найм: рекрут виден, но не по карману - копим на его цену;
    // не виден или уже нанят в этот час - держим фонд на следующего (23.10:
    // всё уходило в технику, рекрут за 6000 оставался недоступен).
    int fund = 0;
    if (snap.members < r.maxMembers)
        fund = (snap.recruitPrice >= 0 && !recruited) ? snap.recruitPrice : kDfHireFund;
    const int keep = kDfReserve + fund;
    if (s.techLevel < r.maxTech && r.techPointCost > 0 && money > keep)
    {
        int points = (money - keep) / r.techPointCost;
        if (points > kDfMaxResearchPerTick)
            points = kDfMaxResearchPerTick;
        if (points > 0)
            acts.push_back(Make(DF_RESEARCH, points));
    }
    return acts;
}

void DfApplyBookkeeping(DfFactionState& s, const std::vector<DfAction>& done, double hour, const DfRules& r)
{
    for (size_t i = 0; i < done.size(); ++i)
    {
        const DfAction& a = done[i];
        switch (a.type)
        {
        case DF_FOUND:
            s.founded = true;
            s.foundedHour = hour;
            s.lastTickHour = hour;
            DfAddEvent(s, hour, "found");
            break;
        case DF_EXTINCT:
            s.extinct = true;
            DfAddEvent(s, hour, "extinct");
            break;
        case DF_RECRUIT:
            break;              // событие - когда человек и правда перешёл (адаптер)
        case DF_RESEARCH:
            if (s.techLevel >= r.maxTech)
                break;
            s.techPoints += a.amount;
            while (s.techLevel < r.maxTech && s.techPoints >= 100 * (s.techLevel + 1))
            {
                s.techPoints -= 100 * (s.techLevel + 1);
                ++s.techLevel;
                DfAddEvent(s, hour, Text("level %d", s.techLevel));
            }
            if (s.techLevel >= r.maxTech)
                s.techPoints = 0;
            break;
        case DF_INCOME:
            break;
        }
    }

    // целые часы - дробная часть ждёт следующего тика
    s.lastTickHour += WholeHours(s.lastTickHour, hour);
}
