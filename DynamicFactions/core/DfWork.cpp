#include "core/DfWork.h"

#include <algorithm>

namespace
{
    struct Ranked
    {
        int index;
        float score;
    };

    bool ByScore(const Ranked& a, const Ranked& b)
    {
        return a.score > b.score;
    }

    bool ValidNode(int node, const std::vector<DfNodeState>& nodes)
    {
        return node >= 0 && node < static_cast<int>(nodes.size());
    }
}

void DfRankNodes(const std::vector<DfNodeOffer>& nodes, std::vector<int>& order)
{
    std::vector<Ranked> ranked;
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].value <= 0)
            continue;
        Ranked r;
        r.index = static_cast<int>(i);
        r.score = nodes[i].value / (1.0f + nodes[i].distance / 100.0f);
        ranked.push_back(r);
    }
    std::stable_sort(ranked.begin(), ranked.end(), ByScore);
    order.clear();
    for (size_t i = 0; i < ranked.size(); ++i)
        order.push_back(ranked[i].index);
}

void DfAssignRoles(std::vector<DfMember>& members, const std::vector<DfNodeState>& nodes, bool homeLoaded,
                   bool traderHasMoney)
{
    const size_t n = nodes.size();
    std::vector<int> miners(n, 0);
    std::vector<bool> hauled(n, false);
    std::vector<bool> open(members.size(), false);     // ещё без работы

    // 1. Найм, дом, кто не может работать; кто продолжает своё.
    for (size_t i = 0; i < members.size(); ++i)
    {
        DfMember& m = members[i];
        if (m.hiring)
        {
            m.role = DF_ROLE_HIRE;
            m.node = -1;
            continue;
        }
        if (!homeLoaded || !m.able)
        {
            m.role = DF_ROLE_IDLE;
            m.node = -1;
            continue;
        }
        const bool valid = ValidNode(m.node, nodes);
        if (m.role == DF_ROLE_HAUL && valid && !hauled[m.node] &&
            (m.carried > 0 || (traderHasMoney && nodes[m.node].stored >= kDfHaulAt)))
        {
            hauled[m.node] = true;
            continue;
        }
        if (m.role == DF_ROLE_MINE && valid && nodes[m.node].stored < nodes[m.node].cap &&
            miners[m.node] < nodes[m.node].maxMiners)
        {
            ++miners[m.node];
            continue;
        }
        open[i] = true;
    }
    if (!homeLoaded)
        return;

    std::vector<DfNodeOffer> offers(n);
    for (size_t k = 0; k < n; ++k)
    {
        offers[k].value = nodes[k].value;
        offers[k].distance = nodes[k].distance;
    }
    std::vector<int> order;
    DfRankNodes(offers, order);

    // 2. Носильщики к жилам с запасом: из её копателей, иначе из свободных.
    if (traderHasMoney)
        for (size_t o = 0; o < order.size(); ++o)
        {
            const int k = order[o];
            if (hauled[k] || nodes[k].stored < kDfHaulAt)
                continue;
            int pick = -1;
            for (size_t i = 0; i < members.size() && pick < 0; ++i)
                if (!open[i] && members[i].role == DF_ROLE_MINE && members[i].node == k)
                    pick = static_cast<int>(i);
            for (size_t i = 0; i < members.size() && pick < 0; ++i)
                if (open[i])
                    pick = static_cast<int>(i);
            if (pick < 0)
                continue;
            if (!open[pick])
                --miners[k];
            open[pick] = false;
            members[pick].role = DF_ROLE_HAUL;
            members[pick].node = k;
            hauled[k] = true;
        }

    // 3. Свободные - копать к выгодным жилам с местом; места нет - ждать.
    for (size_t i = 0; i < members.size(); ++i)
    {
        if (!open[i])
            continue;
        DfMember& m = members[i];
        m.role = DF_ROLE_IDLE;
        m.node = -1;
        for (size_t o = 0; o < order.size(); ++o)
        {
            const int k = order[o];
            if (nodes[k].stored < nodes[k].cap && miners[k] < nodes[k].maxMiners)
            {
                m.role = DF_ROLE_MINE;
                m.node = k;
                ++miners[k];
                break;
            }
        }
    }
}
