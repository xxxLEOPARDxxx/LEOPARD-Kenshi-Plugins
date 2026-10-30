#include "core/DfHaul.h"

DfHaulAction DfChooseHaulAction(const DfHaulSnapshot& s)
{
    if (!s.able)
        return DF_HAUL_WAIT;
    if (s.carried > 0)
    {
        if (!s.hasTrader)
            return DF_HAUL_WAIT;
        if (!s.atTrader)
            return DF_HAUL_GO_TRADER;
        return s.greeted ? DF_HAUL_SELL : DF_HAUL_TALK;
    }
    if (!s.hasRoom)
        return DF_HAUL_WAIT;
    if (!s.atNode)
        return DF_HAUL_GO_NODE;
    return s.nodeLoaded && s.nodeHasOre ? DF_HAUL_TAKE : DF_HAUL_WAIT;
}
