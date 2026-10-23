#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "game/DfLabour.h"

#include <Windows.h>

#include <cstdio>

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/Town.h>

#include "game/DfGame.h"

namespace
{
    const unsigned long kDiscoverEveryMs = 60000;
    const unsigned long kLogEveryMs = 60000;

    Ogre::Vector3 Pos(const DfKnownNode& n)
    {
        return Ogre::Vector3(n.x, n.y, n.z);
    }

    // Новые жилы - в память фракции (по месту, 5 м).
    void Remember(DfFactionState& s, const std::vector<DfNode>& found, bool debug)
    {
        for (size_t i = 0; i < found.size(); ++i)
        {
            const DfNode& n = found[i];
            if (n.item == NULL || n.itemValue <= 0)
                continue;
            bool seen = false;
            for (size_t k = 0; k < s.nodes.size() && !seen; ++k)
                seen = Pos(s.nodes[k]).distance(n.pos) < 5.0f;
            if (seen)
                continue;
            DfKnownNode k;
            k.x = n.pos.x;
            k.y = n.pos.y;
            k.z = n.pos.z;
            k.itemSid = n.item->stringID;
            k.stored = 0;
            s.nodes.push_back(k);
            if (debug)
            {
                char line[256];
                sprintf_s(line, sizeof(line), "DynamicFactions: work - new node %s (value %d) at (%.0f %.0f %.0f)",
                          n.itemName.c_str(), n.itemValue, n.pos.x, n.pos.y, n.pos.z);
                DebugLog(line);
            }
        }
    }

    struct NodeNow
    {
        GameData* item;
        bool loaded;
        DfNode live;            // когда загружена
        int real;               // руды в самой жиле
    };
}

void DfLabourStep(DfFactionState& s, DfLabour& rt, Faction* f, TownBase* hub, Character* envoy,
                  bool traderHasMoney, double hour, unsigned long clockMs, bool debug)
{
    std::vector<Character*> people;
    DfFactionMembers(f, people);
    const Ogre::Vector3 home = hub != NULL ? hub->getPosition() : Ogre::Vector3(0.0f, 0.0f, 0.0f);

    // 1. Жилы: вокруг дома раз в минуту, вокруг людей - каждый шаг (видны,
    // пока загружен их участок).
    if (hub != NULL && (rt.discoveredMs == 0 || clockMs - rt.discoveredMs >= kDiscoverEveryMs))
    {
        rt.discoveredMs = clockMs;
        std::vector<DfNode> found;
        DfFindNodes(home, kDfDiscoverRadius, NULL, found);
        Remember(s, found, debug);
    }
    for (size_t i = 0; i < people.size(); ++i)
    {
        std::vector<DfNode> found;
        DfFindNodes(people[i]->getPosition(), 300.0f, NULL, found);
        Remember(s, found, debug);
    }

    // 2. Состояние жил: настоящая руда (загружена) + условная.
    const size_t n = s.nodes.size();
    std::vector<NodeNow> now(n);
    std::vector<DfNodeState> states(n);
    for (size_t k = 0; k < n; ++k)
    {
        int value = 0;
        now[k].item = DfItemBySid(s.nodes[k].itemSid, value);
        now[k].loaded = DfLoadedNodeAt(Pos(s.nodes[k]), now[k].live);
        now[k].real = now[k].loaded ? DfCountInNode(now[k].live.building, now[k].item) : 0;
        if (now[k].real < 0)
            now[k].real = 0;
        states[k].value = now[k].item != NULL ? value : 0;
        states[k].distance = Pos(s.nodes[k]).distance(home);
        states[k].stored = now[k].real + s.nodes[k].stored;
        states[k].cap = kDfNodeCap;
        states[k].maxMiners = kDfMaxMinersPerNode;
    }

    // 3. Роли - ядро.
    std::vector<DfMember> ms(people.size());
    for (size_t i = 0; i < people.size(); ++i)
    {
        Character* const c = people[i];
        const DfMemberWork& w = rt.members[c];
        ms[i].id = static_cast<int>(i);
        ms[i].able = !c->isInCombatMode(true, true);
        ms[i].hiring = c == envoy;
        ms[i].node = w.node;
        ms[i].role = w.role;
        ms[i].carried = w.virtualCarried +
                        (w.node >= 0 && w.node < static_cast<int>(n) ? DfCountItem(c, now[w.node].item) : 0);
    }
    DfAssignRoles(ms, states, hub != NULL, traderHasMoney);

    // 4. Копатели: загружена - приказ копать (как игрок); нет - к точке
    // жилы, рядом с ней копание условное.
    const double hours = rt.lastHour >= 0.0 && hour > rt.lastHour ? hour - rt.lastHour : 0.0;
    rt.lastHour = hour;
    if (rt.virtualFraction.size() != n)
        rt.virtualFraction.resize(n, 0.0f);
    std::vector<int> nearMiners(n, 0);
    for (size_t i = 0; i < people.size(); ++i)
    {
        Character* const c = people[i];
        DfMemberWork& w = rt.members[c];
        const bool changed = w.role != ms[i].role || w.node != ms[i].node;
        if (changed && (w.role == DF_ROLE_MINE || w.role == DF_ROLE_HAUL) && w.ordered)
        {
            DfClearOrders(c);
            w.ordered = false;
        }
        w.role = ms[i].role;
        w.node = ms[i].node;
        if (w.role != DF_ROLE_MINE || w.node < 0)
            continue;
        const int k = w.node;
        if (now[k].loaded)
        {
            if (DfGoalOf(c) != OPERATE_MACHINERY)
                w.ordered = DfOrderMine(c, now[k].live);
        }
        else if (c->getPosition().distance(Pos(s.nodes[k])) > kDfNearNode)
        {
            c->setDestination(Pos(s.nodes[k]), false);
            w.ordered = true;
        }
        else
            ++nearMiners[k];
    }
    for (size_t k = 0; k < n; ++k)
    {
        if (nearMiners[k] == 0 || hours <= 0.0)
            continue;
        rt.virtualFraction[k] += static_cast<float>(hours * kDfOrePerMinerHour * nearMiners[k]);
        const int whole = static_cast<int>(rt.virtualFraction[k]);
        rt.virtualFraction[k] -= static_cast<float>(whole);
        const int room = kDfNodeCap - now[k].real - s.nodes[k].stored;
        s.nodes[k].stored += whole < room ? whole : (room > 0 ? room : 0);
    }

    // 5. Журнал раз в минуту: запас жил с копателями - по нему замер темпа.
    if (debug && (rt.loggedMs == 0 || clockMs - rt.loggedMs >= kLogEveryMs))
    {
        rt.loggedMs = clockMs;
        int miners = 0, haulers = 0;
        DfLabourCounts(rt, miners, haulers);
        char line[200];
        sprintf_s(line, sizeof(line), "DynamicFactions: work - hour %.2f, %d nodes known, %d mining, %d hauling",
                  hour, static_cast<int>(n), miners, haulers);
        DebugLog(line);
        for (size_t k = 0; k < n; ++k)
        {
            int miningHere = 0;
            for (size_t i = 0; i < ms.size(); ++i)
                if (ms[i].role == DF_ROLE_MINE && ms[i].node == static_cast<int>(k))
                    ++miningHere;
            if (miningHere == 0 && states[k].stored == 0)
                continue;
            sprintf_s(line, sizeof(line), "DynamicFactions: work - node %d %s: loaded %d, real %d, virtual %d, miners %d",
                      static_cast<int>(k), s.nodes[k].itemSid.c_str(), now[k].loaded ? 1 : 0, now[k].real,
                      s.nodes[k].stored, miningHere);
            DebugLog(line);
        }
    }
}

void DfLabourCounts(const DfLabour& rt, int& miners, int& haulers)
{
    miners = haulers = 0;
    for (std::map<Character*, DfMemberWork>::const_iterator it = rt.members.begin(); it != rt.members.end(); ++it)
    {
        if (it->second.role == DF_ROLE_MINE)
            ++miners;
        else if (it->second.role == DF_ROLE_HAUL)
            ++haulers;
    }
}
