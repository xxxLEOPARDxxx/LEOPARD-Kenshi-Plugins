#include "core/DfSupply.h"

static void RunSupplyTests()
{
    CHECK(DfNaturalHungerFloor(3,3,false,true,false)==3);
    CHECK(DfNaturalHungerFloor(3,3,true,true,false)==-10);
    CHECK(DfNaturalHungerFloor(0.8f,0.8f,true,true,false)==-10);
    CHECK(DfNaturalHungerFloor(-10,-10,true,true,false)==-10);
    CHECK(DfNaturalHungerFloor(1.5f,3,true,true,false)==1.5f);
    CHECK(DfNaturalHungerFloor(3,3,true,false,false)==3);
    CHECK(DfNaturalHungerFloor(3,3,true,true,true)==3);
    CHECK(DfNaturalHungerFloor(3,std::numeric_limits<float>::quiet_NaN(),true,true,false)==3);
    float target = 0;
    CHECK(DfPlanFood(1,3,0.9f,false,target)==DF_FOOD_STOCKED);
    CHECK(target==1.0f);
    CHECK(DfPlanFood(1,3,0.49f,false,target)==DF_FOOD_RESTOCK);
    CHECK(DfPlanFood(1,3,0.5f,false,target)==DF_FOOD_STOCKED);
    CHECK(DfPlanFood(1,3,0.9f,true,target)==DF_FOOD_RESTOCK);
    CHECK(DfPlanFood(1,3,1.0f,true,target)==DF_FOOD_STOCKED);
    CHECK(DfPlanFood(1,2.0f,0,false,target)==DF_FOOD_URGENT);
    CHECK(DfPlanFood(1,2.1f,0,false,target)==DF_FOOD_RESTOCK);
    CHECK(DfPlanFood(0,3,0,false,target)==DF_FOOD_NOT_NEEDED);
    CHECK(DfPlanFood(-1,3,0,false,target)==DF_FOOD_UNKNOWN);
    CHECK(DfPlanFood(1,3,-1,false,target)==DF_FOOD_UNKNOWN);
    CHECK(DfPlanFood(1,std::numeric_limits<float>::quiet_NaN(),0,false,target)==DF_FOOD_UNKNOWN);
    CHECK(DfPlanFood(std::numeric_limits<float>::infinity(),3,0,false,target)==DF_FOOD_UNKNOWN);
    CHECK(DfPlanFood(2,3,0.9f,false,target)==DF_FOOD_RESTOCK);
    CHECK(target==2.0f);
    CHECK(DfPlanFood(0.1f,3,0.1f,true,target)==DF_FOOD_STOCKED);
    CHECK(target==0.1f);
    std::vector<DfNodeState> nodes(1);
    nodes[0].value=100;nodes[0].stored=1;nodes[0].distance=1;nodes[0].cap=10;nodes[0].maxMiners=4;
    std::vector<DfMember> members(1);members[0].able=true;members[0].supplying=true;
    members[0].node=0;members[0].carried=3;
    DfAssignRoles(members,nodes,true,true);
    CHECK(members[0].role==DF_ROLE_SUPPLY);CHECK(members[0].node==0);
    members[0].supplying=false;DfAssignRoles(members,nodes,true,true);
    CHECK(members[0].role==DF_ROLE_HAUL);CHECK(members[0].carried==3);
}
