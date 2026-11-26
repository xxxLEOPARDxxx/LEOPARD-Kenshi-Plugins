#pragma once

#include <string>

namespace durability
{
inline std::string BreakageNoticeName(const std::string& name, const std::string& locale)
{
    if (locale.compare(0, 2, "ru") != 0 || name.size() <= 4 ||
        name.find_first_of(" \t\r\n-") != std::string::npos)
        return name;

    // A single name ending in Cyrillic ka takes ki in the Russian genitive.
    // Keep other display names as supplied by the game.
    const std::string::size_type suffix = name.size() - 4;
    if (name.compare(suffix, 4, "\xD0\xBA\xD0\xB0") == 0)
        return name.substr(0, name.size() - 2) + "\xD0\xB8";
    if (name.compare(suffix, 4, "\xD0\x9A\xD0\x90") == 0)
        return name.substr(0, name.size() - 2) + "\xD0\x98";
    return name;
}
}
