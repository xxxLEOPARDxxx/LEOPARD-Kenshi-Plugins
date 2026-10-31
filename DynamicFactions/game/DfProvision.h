#pragma once
#include <map>
#include <string>
#include <vector>
#include <ogre/OgreVector3.h>
#include "game/DfHand.h"
#include "core/DfState.h"
#include "game/DfSell.h"

class Character;
class Faction;

struct DfProvision
{
    hand member;
    hand trader;
    std::string memberName;
    std::string lastItemName;
    std::vector<hand> failedShops;
    std::map<hand,unsigned long,DfWorkHandLess> cooldown;
    size_t scanCursor;
    unsigned long progressAt;
    unsigned long startedAt;
    Ogre::Vector3 progressPosition;
    Ogre::Vector3 destination;
    bool moving;
    bool warnedUnreadable;
    int spent;
    DfProvision():scanCursor(0),progressAt(0),startedAt(0),progressPosition(0,0,0),destination(0,0,0),
        moving(false),warnedUnreadable(false),spent(0) {}
};

// One active self-supply route and at most one physical purchase per faction
// step. Native food consumption remains untouched. No borrowed pointers persist.
void DfProvisionStep(DfFactionState& state,DfProvision& runtime,Faction* faction,
    const std::vector<Character*>& people,const std::vector<DfTrader>& traders,
    Character* envoy,int committed,double hour,unsigned long now,bool debug);
bool DfProvisionOwns(const DfProvision& runtime,Character* actor);
