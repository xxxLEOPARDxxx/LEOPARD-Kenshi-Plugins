#pragma once

#include <cmath>
#include <cstdio>
#include <string>

namespace finite_ore {

inline std::string DepletionMarker(const std::string& templateId, float x, float z)
{
    if (templateId.empty() || !(x >= -100000000 && x <= 100000000) ||
        !(z >= -100000000 && z <= 100000000))
        return std::string();

    const int column = static_cast<int>(std::floor(x * 4.0 + 0.5));
    const int row = static_cast<int>(std::floor(z * 4.0 + 0.5));
    char coordinates[72];
    sprintf_s(coordinates, "finite_ore_veins_depleted_%d_%d_", column, row);
    return coordinates + templateId;
}

}
