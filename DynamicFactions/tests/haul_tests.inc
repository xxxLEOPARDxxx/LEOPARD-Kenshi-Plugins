#include "core/DfHaul.h"

static void RunHaulTests()
{
    DfHaulSnapshot s = {};
    s.able = true;
    s.hasRoom = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_GO_NODE);
    s.atNode = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_WAIT);
    s.nodeLoaded = s.nodeHasOre = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_TAKE);
    s.hasRoom = false;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_WAIT);
    s.carried = 3;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_WAIT);
    s.hasTrader = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_GO_TRADER);
    s.atTrader = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_TALK);
    s.greeted = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_SELL);
    s.able = false;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_WAIT);
    s.able = true;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_SELL);
    // Reconstructing after load from the same native stock cannot mint cargo.
    s.carried = 0;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_WAIT);
    s.hasRoom = true;
    s.atNode = false;
    CHECK(DfChooseHaulAction(s) == DF_HAUL_GO_NODE);
}
