#include "../ReserveRules.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using namespace finite_ore;
    const Rules rules;
    const unsigned full = Capacity(5, 1600, -4800);
    assert(full >= 7500 && full <= 17500);
    assert(Capacity(5, 1600, -4800) == full);
    assert(Capacity(5, 1600.01f, -4799.99f) == full);
    assert(Capacity(0, 1600, -4800) == Capacity(1, 1600, -4800));
    assert(Capacity(-1, 1600, -4800) == Capacity(1, 1600, -4800));
    bool varies = false;
    for (int x = -3200; x < 3200; x += 128) {
        unsigned previous = 0;
        for (int workers = 1; workers <= 10; ++workers) {
            const unsigned capacity = Capacity(workers, static_cast<float>(x), 3200);
            assert(capacity - previous >= 1500 && capacity - previous <= 3500);
            previous = capacity;
        }
        varies |= previous != Capacity(10, 1600, -4800);
    }
    assert(varies);
    const unsigned large = Capacity(256, 1600, -4800);
    assert(Capacity(2147483647, 1600, -4800) == large);
    Reserve migrated;
    migrated.share = 0.5;
    assert(Units(migrated, full) == full / 2);
    for (unsigned left = full; left != 0; --left) {
        const int saved = static_cast<int>(migrated.share * 1000000000 + 0.5);
        Reserve reloaded;
        reloaded.share = saved / 1000000000.0;
        assert(Units(reloaded, full) == Units(migrated, full));
        migrated.share = static_cast<double>(left) / full;
    }
    Reserve finalUnit;
    finalUnit.share = 1.0 / large;
    finalUnit.share = static_cast<int>(finalUnit.share * 1000000000 + 0.5) / 1000000000.0;
    assert(Units(finalUnit, large) == 1 && Spend(finalUnit, large, 1) && finalUnit.closed);

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
