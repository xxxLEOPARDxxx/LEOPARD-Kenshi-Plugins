#include "core/DfSupply.h"
#include <float.h>

float DfNaturalHungerFloor(float nativeFloor,float platoonFloor,bool own,bool organic,bool animal)
{
    // Preserve animal options, robots and special native slave/player states.
    // No world-ready cache: loading must not grant food before our first tick.
    return own && organic && !animal && _finite(nativeFloor) && _finite(platoonFloor) &&
        nativeFloor>=0 && nativeFloor==platoonFloor ? -10.0f : nativeFloor;
}

DfFoodNeed DfPlanFood(float rate, float hunger, float nutrition, bool restocking, float& target)
{
    target = 0.0f;
    if (!_finite(rate) || !_finite(hunger) || !_finite(nutrition) || rate < 0 || nutrition < 0)
        return DF_FOOD_UNKNOWN;
    if (rate == 0) return DF_FOOD_NOT_NEEDED;
    // Bounded stock target, with a low-water mark to avoid a shopping trip
    // for each mouthful. Race/exertion rate is supplied by the native engine.
    target = rate > 4.0f ? 4.0f : rate;
    if (nutrition >= (restocking ? target : target * 0.5f)) return DF_FOOD_STOCKED;
    return hunger < 2.1f ? DF_FOOD_URGENT : DF_FOOD_RESTOCK;
}
