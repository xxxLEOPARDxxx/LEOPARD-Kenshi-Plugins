#pragma once
// Работа фракции в игре (этап 2а): жилы, копатели, условное копание.
// Роли решает ядро (core/DfWork); здесь - что видно в мире и приказы.
// Только главный поток; обращения к игре - через game/DfMine (под SEH).

#include <map>
#include <string>
#include <vector>

#include "core/DfState.h"
#include "core/DfWork.h"
#include "game/DfMine.h"

class Character;
class Faction;
class TownBase;

const float kDfDiscoverRadius = 3000.0f;    // жилы вокруг дома
const float kDfNearNode = 60.0f;            // ближе - копатель «у жилы» (условное копание)
const int kDfNodeCap = 50;                  // хранилище жилы (пользователь, 23.10)
const int kDfMaxMinersPerNode = 3;
const float kDfOrePerMinerHour = 4.0f;      // условное копание; уточняется замером при загруженной жиле

struct DfMemberWork
{
    DfRole role;
    int node;
    int virtualCarried;         // условный груз носильщика (жила была не загружена)
    bool ordered;               // приказ копать/идти отдан
    DfMemberWork() : role(DF_ROLE_IDLE), node(-1), virtualCarried(0), ordered(false) {}
};

struct DfLabour
{
    std::map<Character*, DfMemberWork> members;     // на эту игру; при смене игры - заново
    double lastHour;                                // для условного копания (игровые часы)
    std::vector<float> virtualFraction;             // дробная руда условного копания по жилам
    unsigned long discoveredMs;
    unsigned long loggedMs;
    DfLabour() : lastHour(-1.0), discoveredMs(0), loggedMs(0) {}
};

// Раз в 5 с (часы найма - стоят на паузе). envoy - посланник найма (работу не получает).
void DfLabourStep(DfFactionState& s, DfLabour& rt, Faction* f, TownBase* hub, Character* envoy,
                  bool traderHasMoney, double hour, unsigned long clockMs, bool debug);

// Для окна: копают N, носят N.
void DfLabourCounts(const DfLabour& rt, int& miners, int& haulers);
