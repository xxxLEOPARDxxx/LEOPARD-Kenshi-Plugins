#include "../MiningRule.h"
#include <cstdio>
#include <cmath>

static int failed = 0;
static void Check(bool result, const char* name)
{
    if (!result) { ++failed; std::printf("FAIL: %s\n", name); }
}
int main()
{
    float carry = 0;
    int total = 0;
    for (int i = 0; i < 400; ++i) total += MiningRule::Ticks(0.25f, 5, 15, true, carry);
    Check(total == 100 && carry == 0, "quarter-rate produces exactly 100 ticks from 400");
    carry = 0;
    total = MiningRule::Ticks(1.5f, 5, 15, true, carry);
    Check(total == 1 && carry == 0.5f, "fraction retained for a multiplier above one");
    total += MiningRule::Ticks(1.5f, 5, 15, true, carry);
    Check(total == 3 && carry == 0, "fraction consumed on the following tick");
    carry = 0.75f;
    Check(MiningRule::Ticks(0.25f, 15, 15, true, carry) == 0 && carry == 0,
          "exact cap blocks the award and drops old carry");
    Check(MiningRule::Ticks(1, 150, 15, false, carry) == 1,
          "caps disabled allow extra ticks above the source cap");
    carry = 0.5f;
    Check(MiningRule::Ticks(0, 5, 15, true, carry) == 0 && carry == 0,
          "disabled skill cannot retain partial awards");
    carry = 0;
    total = 0;
    for (int i = 0; i < 10000; ++i) total += MiningRule::Ticks(0.1f, 5, 15, true, carry);
    Check(total == 1000, "long fractional run does not lose XP ticks");
    std::printf("Mining rule: %s\n", failed ? "FAILED" : "7 checks passed");
    return failed ? 1 : 0;
}
