#pragma once
#include <map>
#include <string>
#include <vector>
#include "core/DfState.h"
#include "core/DfWork.h"
#include "game/DfMine.h"

class Character;
class Faction;
class TownBase;

struct DfWorkHandLess
{
    bool operator()(const hand& a, const hand& b) const
    {
        if (a.type != b.type) return a.type < b.type;
        if (a.container != b.container) return a.container < b.container;
        if (a.containerSerial != b.containerSerial) return a.containerSerial < b.containerSerial;
        if (a.index != b.index) return a.index < b.index;
        return a.serial < b.serial;
    }
};
struct DfMemberWork
{
    DfRole role;
    int node;
    bool ordered;
    bool moving;
    Ogre::Vector3 destination;
    hand trader;
    unsigned long talkedAt;
    unsigned long progressAt;
    unsigned long blockedUntil;
    hand failedTrader;
    Ogre::Vector3 progressPosition;
    DfMemberWork() : role(DF_ROLE_IDLE), node(-1), ordered(false), moving(false),
        destination(0,0,0), talkedAt(0), progressAt(0), blockedUntil(0), progressPosition(0,0,0) {}
};
struct DfLabour
{
    std::map<hand, DfMemberWork, DfWorkHandLess> members;
    unsigned long loggedMs;
    int carried;
    int sold;
    int revenue;
    DfLabour() : loggedMs(0), carried(0), sold(0), revenue(0) {}
};
// Work only with real inventories in the loaded world. Legacy virtual stock is
// preserved in the save, quarantined from production, pickup and sale.
void DfLabourStep(DfFactionState& s, DfLabour& rt, Faction* f, TownBase* hub,
    Character* envoy, bool traderHasMoney, double hour, unsigned long clockMs,
    bool debug, float radius = 3000.0f);
void DfLabourStop(DfLabour& rt, Faction* faction=NULL, Character* envoy=NULL);
void DfLabourCounts(const DfLabour& rt, int& miners, int& haulers);
std::string DfLabourStatus(const DfLabour& rt);
