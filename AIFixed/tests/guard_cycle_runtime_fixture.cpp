#include <cstdio>
#include <float.h>
#include <GatePolicy.h>

enum TaskType { NULL_TASK, IDLE, STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE, PATROL_TOWN, OTHER_TASK };
enum { BF_FLUFF, BF_BED };
struct Vector { struct { float m128_f32[4]; } m_quad; };
struct Building
{
    int function;
    bool disabled;
    struct { bool isComplete; } _buildState;
    Building():function(BF_FLUFF),disabled(false) { _buildState.isComplete=true; }
    int getSpecialFunction() { return function; }
    bool isDisabled() { return disabled; }
};
struct StandingHandle
{
    Building* object;
    Building* getBuilding() { return object; }
};
struct HavokCharacter
{
    enum CharacterState { IDLE, GOAL_REACHED, FOLLOWING_PATH, MANUAL };
    enum PathState { NONE, COMPLETE, INCOMPLETE, FAILED, WAITING, UPDATING, BROKEN };
    CharacterState state;
    PathState path;
    Vector goal,position;
    HavokCharacter():state(IDLE),path(WAITING)
    {
        for (unsigned i=0;i<4;++i) position.m_quad.m128_f32[i]=goal.m_quad.m128_f32[i]=0;
        goal.m_quad.m128_f32[0]=57.0f;
        goal.m_quad.m128_f32[1]=14.25f;
    }
    CharacterState getState() { return state; }
    PathState getPathState() { return path; }
};
struct CharMovement
{
    HavokCharacter* havokCharacter;
    StandingHandle standing;
    StandingHandle isStandingOnSomething() { return standing; }
};
struct Character
{
    bool animal;
    Character():animal(false) {}
    bool isAnimal() { return animal; }
};

// @PRODUCTION@

int checks=0,failures=0;
void Check(bool value,const char* name)
{
    ++checks;
    if (!value) { ++failures;printf("FAIL: %s\n",name); }
}
int main()
{
    Character actor;
    Building support;
    HavokCharacter navigation;
    CharMovement movement={&navigation,{&support}};
    bool attempt=false,failed=false;
    Check(ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,false,attempt,failed)==&support &&
        attempt && !failed,"captured distant waiting post request begins observation");
    navigation.path=HavokCharacter::FAILED;
    Check(ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed)==&support &&
        attempt && failed,"captured active post failure supplies fresh evidence");
    Check(ReadGuardCycle(&actor,&movement,NULL_TASK,true,attempt,failed)==&support &&
        !attempt && !failed,"captured no-goal gap retains history without renewing a stale failure");
    Check(ReadGuardCycle(&actor,&movement,IDLE,true,attempt,failed)==&support &&
        !attempt && !failed,"idle gap cannot renew retained failed navigation");
    Check(!ReadGuardCycle(&actor,&movement,NULL_TASK,false,attempt,failed),
        "untracked idle actor never starts a guard episode");
    navigation.state=HavokCharacter::GOAL_REACHED;
    Check(!ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed),
        "reached active post retires the old episode");
    Check(!ReadGuardCycle(&actor,&movement,NULL_TASK,true,attempt,failed),
        "reached state cannot be mistaken for an idle gap");
    navigation.state=HavokCharacter::IDLE;
    navigation.path=HavokCharacter::COMPLETE;
    Check(!ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed),
        "completed active post cannot reuse old failure evidence");
    navigation.path=HavokCharacter::FAILED;
    navigation.goal=navigation.position;
    Check(!ReadGuardCycle(&actor,&movement,PATROL_TOWN,true,attempt,failed),
        "a destination already at the feet cannot continue a blocked patrol");
    Check(!ReadGuardCycle(&actor,&movement,OTHER_TASK,true,attempt,failed),
        "another task cancels the guard episode");
    printf("AIFixed extracted guard cycle: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
