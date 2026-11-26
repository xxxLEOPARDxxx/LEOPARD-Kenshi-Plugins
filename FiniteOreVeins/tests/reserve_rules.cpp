#include "../ReserveRules.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using namespace finite_ore;
    const Rules rules;
    assert(Capacity(rules, 0.8f) == 80);
    assert(Capacity(rules, 0.0f) == 25);
    assert(Capacity(rules, 99.0f) == 300);

    Reserve rock(100);
    for (unsigned i = 0; i < 100; ++i)
        assert(Spend(rock, 100, 100));
    assert(!Spend(rock, 100, 100));
    assert(rock.closed && Units(rock, 100) == 0);
    Refresh(rock, rules, 100, 111);
    assert(rock.closed && Units(rock, 100) == 0);
    Refresh(rock, rules, 100, 171);
    assert(rock.closed && Units(rock, 100) == 24);
    Refresh(rock, rules, 100, 172);
    assert(!rock.closed && Units(rock, 100) == 25);
    Refresh(rock, rules, 100, 352);
    assert(!rock.closed && Units(rock, 100) == 100);

    Rules permanent;
    permanent.refillDays = 0;
    Reserve small(3);
    assert(Spend(small, 1, 3));
    Refresh(small, permanent, 1, 5000);
    assert(small.closed && !Spend(small, 1, 5000));
    Refresh(small, rules, 1, 2);
    assert(small.closed && Units(small, 1) == 0);
    Refresh(small, rules, 1, 254);
    assert(!small.closed && Units(small, 1) == 1);

    Rules immediate;
    immediate.restHours = 0;
    immediate.reopenPercent = 0;
    Reserve early(0);
    assert(Spend(early, 1, 0));
    Refresh(early, immediate, 1, 12);
    assert(early.closed);
    Refresh(early, immediate, 1, 240);
    assert(!early.closed && Units(early, 1) == 1);
    std::puts("Reserve limits, exact final unit, cooldown, reopen, permanence and clock rollback passed");
}
