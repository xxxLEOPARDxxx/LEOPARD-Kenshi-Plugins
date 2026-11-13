#include <cstdio>
#include <float.h>
#include <limits>
#include <GatePolicy.h>

enum TaskType { NULL_TASK, IDLE, STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE, PATROL_TOWN, OTHER_TASK, MOVE_ON_FREE_WILL };
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
struct Tasker {bool taskData;TaskType key()const{return MOVE_ON_FREE_WILL;}};
struct CharBody {Tasker action;bool moving;CharBody():moving(true){action.taskData=true;}Tasker* getCurrentAction(){return moving?&action:NULL;}};
struct Character
{
    CharBody body;
    CharBody* getBody(){return &body;}
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
    Check(ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed)==&support && !attempt && !failed,
        "reached acknowledgement retains only the stationary clock, never relocation evidence");
    Check(ReadGuardCycle(&actor,&movement,NULL_TASK,true,attempt,failed)==&support && !attempt && !failed,
        "captured reached no-goal acknowledgement cannot refresh a failure");
    Check(!ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,false,attempt,failed),
        "final relocation revalidation rejects a reached active guard goal");
    Check(!ReadGuardCycle(&actor,&movement,NULL_TASK,false,attempt,failed),
        "final relocation revalidation rejects a no-goal acknowledgement");
    navigation.state=HavokCharacter::IDLE;
    navigation.path=HavokCharacter::COMPLETE;
    Check(ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed)==&support &&
        !attempt && !failed,"a completed path phase with a still distant native goal preserves but cannot refresh the episode");
    navigation.path=HavokCharacter::NONE;
    Check(ReadGuardCycle(&actor,&movement,PATROL_TOWN,true,attempt,failed)==&support && !attempt && !failed,
        "a native no-path phase during the same blocked patrol preserves the stationary episode");
    navigation.path=HavokCharacter::UPDATING;
    Check(ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,true,attempt,failed)==&support && !attempt && !failed,
        "an asynchronous refresh cannot restart four hours or manufacture a failure");
    Check(!ReadGuardCycle(&actor,&movement,STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE,false,attempt,failed),
        "an untracked successful path phase cannot begin recovery");
    navigation.path=HavokCharacter::FAILED;
    navigation.goal=navigation.position;
    Check(ReadGuardCycle(&actor,&movement,PATROL_TOWN,true,attempt,failed)==&support && !attempt && !failed,
        "at-feet reset can retain only the existing stationary clock");
    Check(!ReadGuardCycle(&actor,&movement,PATROL_TOWN,false,attempt,failed),
        "a destination already at the feet cannot admit relocation");
    Check(ReadGuardCycle(&actor,&movement,OTHER_TASK,true,attempt,failed)==&support,
        "another task retains physical history on the same platform");
    navigation.goal.m_quad.m128_f32[0]=57.0f;
    Check(ReadGuardCycle(&actor,&movement,OTHER_TASK,false,attempt,failed)==&support && attempt && failed,
        "a native move action under an arbitrary goal can begin physical observation");
    actor.body.moving=false;
    Check(ReadGuardCycle(&actor,&movement,OTHER_TASK,true,attempt,failed)==&support && !attempt && !failed,
        "a stale native failure after movement ended cannot refresh evidence");
    Check(!ReadGuardCycle(&actor,&movement,OTHER_TASK,false,attempt,failed),
        "a stale native failure without current movement cannot start observation");
    navigation.goal.m_quad.m128_f32[0]=std::numeric_limits<float>::quiet_NaN();
    Check(!ReadGuardCycle(&actor,&movement,NULL_TASK,true,attempt,failed),
        "invalid native goal data cannot retain guard history");
    printf("AIFixed extracted guard cycle: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
