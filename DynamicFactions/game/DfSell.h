#pragma once
// Продажа добычи торговцам города. Только главный поток; всё под SEH.

#include <string>
#include <vector>

#include <kenshi/util/hand.h>

class TownBase;

// Торговец - живой персонаж города с флагом шаблона «is trader» (как в
// RecruitMarkers: не Character::isATrader - та «да» и охране лавки).
struct DfTrader
{
    hand who;
    std::string name;
    int money;                  // деньги его взвода (Ownerships) - сколько может заплатить
    float distance;             // от центра города
};

void DfFindTraders(TownBase* town, std::vector<DfTrader>& out);
