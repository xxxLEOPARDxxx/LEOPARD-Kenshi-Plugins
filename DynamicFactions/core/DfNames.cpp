#include "core/DfNames.h"

#include <algorithm>

namespace
{
    const int kWords = 16;
    const int kCombos = kWords * kWords;
    const int kAttempts = 64;
    const unsigned kStride = 97;            // нечётный - обход без повторов по 256

    const char* const kHumanAdj[kWords] = {
        "Iron", "Rust", "Dust", "Ashen", "Grey", "Silent", "Broken", "Black",
        "Red", "Free", "Hollow", "Bitter", "Wild", "Stone", "Sand", "Last" };
    const char* const kHumanNoun[kWords] = {
        "Jackals", "Drifters", "Lanterns", "Hounds", "Blades", "Wardens", "Crows", "Company",
        "Brotherhood", "Kin", "Covenant", "Riders", "Knives", "Banner", "Wolves", "Hand" };

    const char* const kShekAdj[kWords] = {
        "Horned", "Proud", "Stone", "Battle", "Red", "Honoured", "Fierce", "Iron",
        "Scarred", "Thunder", "Unbowed", "Blood", "Tall", "Brave", "War", "Old" };
    const char* const kShekNoun[kWords] = {
        "Horns", "Warband", "Spears", "Clan", "Shields", "Champions", "Oath", "Host",
        "Bulls", "Guard", "Legion", "Fists", "Kin", "Hammers", "Lineage", "Rams" };

    const char* const kSkelAdj[kWords] = {
        "Steel", "Chrome", "Rusted", "Silent", "Second", "Machine", "Cold", "Grim",
        "Ancient", "Hollow", "Null", "Grey", "Undying", "Static", "Lost", "Final" };
    const char* const kSkelNoun[kWords] = {
        "Frames", "Protocol", "Engines", "Circuit", "Legion", "Directive", "Core", "Array",
        "Sentinels", "Remnant", "Network", "Order", "Assembly", "Units", "Memory", "Signal" };

    const char* const kHiveAdj[kWords] = {
        "Swarming", "Golden", "Chittering", "Amber", "Many", "Restless", "Humming", "Pale",
        "Wandering", "Bright", "Hungry", "Busy", "Gilded", "Burrowing", "Sweet", "Distant" };
    const char* const kHiveNoun[kWords] = {
        "Hive", "Swarm", "Brood", "Colony", "Drones", "Comb", "Nest", "Workers",
        "Queenless", "Cluster", "Chorus", "Kin", "Mound", "Burrow", "Song", "Hum" };

    void Words(const std::string& race, const char* const*& adj, const char* const*& noun)
    {
        adj = kHumanAdj;
        noun = kHumanNoun;
        if (race == "shek")
        {
            adj = kShekAdj;
            noun = kShekNoun;
        }
        else if (race == "skeleton")
        {
            adj = kSkelAdj;
            noun = kSkelNoun;
        }
        else if (race == "hive")
        {
            adj = kHiveAdj;
            noun = kHiveNoun;
        }
    }

    bool Taken(const std::vector<std::string>& taken, const std::string& name)
    {
        return std::find(taken.begin(), taken.end(), name) != taken.end();
    }

    std::string Combo(const char* const* adj, const char* const* noun, unsigned index)
    {
        index %= kCombos;
        return std::string(adj[index / kWords]) + " " + noun[index % kWords];
    }

    const char* const kNumerals[] = { " II", " III", " IV", " V", " VI", " VII", " VIII", " IX", " X" };
}

std::string DfMakeName(const std::string& race, unsigned seed, const std::vector<std::string>& taken)
{
    const char* const* adj = NULL;
    const char* const* noun = NULL;
    Words(race, adj, noun);

    const unsigned start = seed * 2654435761u;      // разброс зёрен по сочетаниям
    for (int attempt = 0; attempt < kAttempts; ++attempt)
    {
        const std::string name = Combo(adj, noun, start + static_cast<unsigned>(attempt) * kStride);
        if (!Taken(taken, name))
            return name;
    }

    const std::string base = Combo(adj, noun, start);
    for (size_t i = 0; i < sizeof(kNumerals) / sizeof(kNumerals[0]); ++i)
    {
        const std::string name = base + kNumerals[i];
        if (!Taken(taken, name))
            return name;
    }
    return base + " X";
}
