#include "../ReserveRules.h"
#include "../VeinIdentity.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using namespace finite_ore;
    const std::string exhausted = DepletionMarker("natural-iron", -1234.0f, 1504.0f);
    assert(!exhausted.empty());
    assert(DepletionMarker("natural-iron", -1234.01f, 1504.01f) == exhausted);
    assert(DepletionMarker("natural-iron", -1234.4f, 1504.0f) != exhausted);
    assert(DepletionMarker("natural-copper", -1234.0f, 1504.0f) != exhausted);
    assert(DepletionMarker("natural-iron", 1234.0f, 1504.0f) != exhausted);
    assert(DepletionMarker("", 1234.0f, 1504.0f).empty());
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
    assert(!Spend(rock, 100, 100 + 365 * 24));

    // The ten-unit test vein and a restored near-empty production vein
    // must both stop immediately after their exact final unit.
    Reserve small(3);
    for (unsigned i = 0; i < 10; ++i)
        assert(Spend(small, 10, 3));
    assert(small.closed && Units(small, 10) == 0);
    assert(!Spend(small, 10, 5000));
    std::puts("Worker limits, saved reserve precision, exact exhaustion and permanence passed");
}
