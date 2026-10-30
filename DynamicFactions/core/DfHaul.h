#pragma once

// Rebuilt from real inventories every step; no payment or cargo replay log.
enum DfHaulAction
{
    DF_HAUL_WAIT,
    DF_HAUL_GO_NODE,
    DF_HAUL_TAKE,
    DF_HAUL_GO_TRADER,
    DF_HAUL_SELL
};

struct DfHaulSnapshot
{
    bool able;
    int carried;
    bool nodeLoaded;
    bool atNode;
    bool nodeHasOre;
    bool hasRoom;
    bool hasTrader;
    bool atTrader;
};

DfHaulAction DfChooseHaulAction(const DfHaulSnapshot& snap);
