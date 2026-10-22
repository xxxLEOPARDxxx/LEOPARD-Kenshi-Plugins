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

namespace
{
    // Один перебор на оба словаря: имя по номеру сочетания, 64 попытки с
    // шагом, потом приписка « II»... к имени по зерну.
    template <class NameAt>
    std::string Pick(unsigned combos, unsigned seed, const std::vector<std::string>& taken, NameAt nameAt)
    {
        const unsigned start = seed * 2654435761u;
        for (int attempt = 0; attempt < kAttempts; ++attempt)
        {
            const std::string name = nameAt((start + static_cast<unsigned>(attempt) * kStride) % combos);
            if (!Taken(taken, name))
                return name;
        }
        const std::string base = nameAt(start % combos);
        for (size_t i = 0; i < sizeof(kNumerals) / sizeof(kNumerals[0]); ++i)
        {
            const std::string name = base + kNumerals[i];
            if (!Taken(taken, name))
                return name;
        }
        return base + " X";
    }

    struct DictName
    {
        const DfNameList* list;
        std::string operator()(unsigned index) const
        {
            const DfNoun& noun = list->nouns[index % list->nouns.size()];
            const DfAdjective& adj = list->adj[(index / list->nouns.size()) % list->adj.size()];
            return adj.forms[noun.gender] + " " + noun.word;
        }
    };

    std::vector<std::string> Split(const std::string& s, char sep)
    {
        std::vector<std::string> out;
        size_t from = 0;
        while (true)
        {
            const size_t at = s.find(sep, from);
            out.push_back(s.substr(from, at == std::string::npos ? std::string::npos : at - from));
            if (at == std::string::npos)
                return out;
            from = at + 1;
        }
    }

    int Gender(const std::string& g)
    {
        static const char* const names[4] = { "m", "f", "n", "p" };
        for (int i = 0; i < 4; ++i)
            if (g == names[i])
                return i;
        return -1;
    }
}

bool DfParseNameDict(const std::string& text, DfNameDict& out)
{
    DfNameDict result;
    DfNameList* current = NULL;
    const std::vector<std::string> lines = Split(text, '\n');
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string line = lines[i];
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF)
            line.erase(0, 3);                                   // BOM
        if (line.empty() || line[0] == '#')
            continue;
        if (line[0] == '[')
        {
            if (line[line.size() - 1] != ']' || line.size() < 3)
                return false;
            current = &result[line.substr(1, line.size() - 2)];
            continue;
        }
        const size_t eq = line.find('=');
        if (current == NULL || eq == std::string::npos)
            return false;
        const std::string key = line.substr(0, eq);
        const std::vector<std::string> parts = Split(line.substr(eq + 1), '|');
        if (key == "noun")
        {
            if (parts.size() != 2 || parts[0].empty() || Gender(parts[1]) < 0)
                return false;
            DfNoun n;
            n.word = parts[0];
            n.gender = Gender(parts[1]);
            current->nouns.push_back(n);
        }
        else if (key == "adj")
        {
            if (parts.size() != 4)
                return false;
            DfAdjective a;
            for (int f = 0; f < 4; ++f)
                a.forms[f] = parts[f];
            current->adj.push_back(a);
        }
        else
            return false;
    }
    out.swap(result);
    return true;
}

std::string DfMakeNameFrom(const DfNameDict& dict, const std::string& race, unsigned seed,
                           const std::vector<std::string>& taken)
{
    DfNameDict::const_iterator it = dict.find(race);
    if (it == dict.end() || it->second.adj.empty() || it->second.nouns.empty())
        it = dict.find("human");
    if (it == dict.end() || it->second.adj.empty() || it->second.nouns.empty())
        return DfMakeName(race, seed, taken);
    DictName nameAt;
    nameAt.list = &it->second;
    return Pick(static_cast<unsigned>(it->second.adj.size() * it->second.nouns.size()), seed, taken, nameAt);
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
