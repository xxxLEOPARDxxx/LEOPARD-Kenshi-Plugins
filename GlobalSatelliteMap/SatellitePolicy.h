#pragma once
// C++03 production policy shared by the plugin and native tests; no game calls.
#include <string>
#include <vector>
#include <sstream>
#include <cmath>
#include <set>
#include <cstdlib>
namespace GSM
{
const unsigned CacheBudget = 96u * 1024u * 1024u;
struct Rect
{
    int x, y, w, h;
    Rect(int X = 0, int Y = 0, int W = 0, int H = 0) : x(X), y(Y), w(W), h(H) {}
};
inline bool Intersects(const Rect &a, const Rect &b)
{
    return a.w > 0 && a.h > 0 && b.w > 0 && b.h > 0 && a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}
inline Rect TileRect(int x, int z, int w, int h)
{
    return Rect(x * w / 4, z * h / 4, (x + 1) * w / 4 - x * w / 4, (z + 1) * h / 4 - z * h / 4);
}
inline bool Finite(double v)
{
    return v == v && v > -1e9 && v < 1e9;
}
inline int Tile(double v, double lo, double hi)
{
    if (!Finite(v) || !Finite(lo) || !Finite(hi) || hi <= lo || v < lo || v >= hi)
        return -1;
    return int(std::floor((v - lo) * 4 / (hi - lo)));
}
inline unsigned Bytes(int size)
{
    return unsigned(size) * unsigned(size) / 2;
}
inline int ChooseLevel(int count, int display, unsigned budget)
{
    int l = 0;
    while (l < 5 && ((8192 >> l) > display * 2 ||
                     static_cast<unsigned long long>(count) * Bytes(8192 >> l) > budget))
        ++l;
    return l;
}
inline bool Hash(const std::string &s)
{
    if (s.size() != 64)
        return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
            return false;
    return true;
}
inline bool SafeRelative(const std::string &s)
{
    if (s.empty() || s.size() > 220 || s[0] == '/' || s[0] == '\\' || s.find(':') != s.npos ||
        s.find('\\') != s.npos)
        return false;
    std::istringstream in(s);
    std::string part;
    while (std::getline(in, part, '/'))
        if (part.empty() || part == "." || part == "..")
            return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (static_cast<unsigned char>(s[i]) < 32)
            return false;
    return s[s.size() - 1] != '/';
}
inline bool SafeName(const std::string &s)
{
    return SafeRelative(s) && s.find('/') == s.npos && s.size() > 4 &&
           s.substr(s.size() - 4) == ".dds";
}
inline bool Path(std::istream &in, std::string &s)
{
    in >> std::ws;
    if (in.peek() == '"')
    {
        in.get();
        std::getline(in, s, '"');
        return !in.fail();
    }
    in >> s;
    return !in.fail();
}
inline bool DDSHeader(const unsigned d[32], unsigned long long length, int size)
{
    return d[0] == 0x20534444 && d[1] == 124 && (d[2] & 0x1007) == 0x1007 &&
           d[27] == 0x1000 && d[3] == unsigned(size) && d[4] == unsigned(size) &&
           d[6] <= 1 && d[7] <= 1 && d[19] == 32 && d[20] == 4 && d[21] == 0x31545844 &&
           d[28] == 0 && length == 128ull + Bytes(size);
}
struct Asset
{
    int x, z, level, size;
    std::string file, hash;
    unsigned long long stamp;
    Asset() : x(0), z(0), level(0), size(0), stamp(0) {}
};
struct Source
{
    std::string file, hash;
};
struct Manifest
{
    std::string world, save;
    double minX, minZ, maxX, maxZ;
    std::vector<Asset> assets;
    std::vector<Source> sources;
    Manifest() : minX(0), minZ(0), maxX(0), maxZ(0) {}
    const Asset &At(int tile, int level) const
    {
        return assets[tile * 6 + level];
    }
};
inline bool Parse(const std::string &text, Manifest &out, std::string &err)
{
    out = Manifest();
    err = "invalid capture manifest";
    if (text.size() > 65536)
        return false;
    std::istringstream all(text);
    std::string line;
    if (!std::getline(all, line))
        return false;
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    if (line != "GSM1")
        return false;
    Manifest m;
    bool world = false, save = false, bounds = false, grid = false, light = false;
    std::set<std::string> names, sources;
    bool seen[96] = {false};
    m.assets.resize(96);
    int count = 0;
    while (std::getline(all, line))
    {
        if (line.empty())
            continue;
        std::istringstream in(line);
        std::string kind, extra;
        in >> kind;
        if (kind == "world")
        {
            if (world || !(in >> m.world) || !Hash(m.world))
                return false;
            world = true;
        }
        else if (kind == "save")
        {
            if (save || !(in >> m.save) || !Hash(m.save))
                return false;
            save = true;
        }
        else if (kind == "bounds")
        {
            if (bounds || !(in >> m.minX >> m.minZ >> m.maxX >> m.maxZ) || !Finite(m.minX) ||
                !Finite(m.minZ) || !Finite(m.maxX) || !Finite(m.maxZ) || m.minX >= m.maxX ||
                m.minZ >= m.maxZ)
                return false;
            bounds = true;
        }
        else if (kind == "grid")
        {
            int x = 0, z = 0, s = 0;
            if (grid || !(in >> x >> z >> s) || x != 4 || z != 4 || s != 8192)
                return false;
            grid = true;
        }
        else if (kind == "lighting")
        {
            std::string s;
            if (light || !(in >> s) || s != "static")
                return false;
            light = true;
        }
        else if (kind == "source")
        {
            Source s;
            if (!Path(in, s.file) || !(in >> s.hash) || !SafeRelative(s.file) || !Hash(s.hash) ||
                m.sources.size() >= 512 || !sources.insert(s.file).second)
                return false;
            m.sources.push_back(s);
        }
        else if (kind == "asset")
        {
            Asset a;
            if (!(in >> a.x >> a.z >> a.level >> a.size) || !Path(in, a.file) || !(in >> a.hash) ||
                a.x < 0 || a.x >= 4 || a.z < 0 || a.z >= 4 || a.level < 0 || a.level >= 6 ||
                a.size != (8192 >> a.level) || !SafeName(a.file) || !Hash(a.hash) ||
                !names.insert(a.file).second)
                return false;
            int i = (a.z * 4 + a.x) * 6 + a.level;
            if (seen[i])
                return false;
            seen[i] = true;
            m.assets[i] = a;
            ++count;
        }
        else
            return false;
        in >> std::ws;
        if (in.peek() != std::char_traits<char>::eof())
            return false;
    }
    if (!world || !save || !bounds || !grid || !light || count != 96 ||
        !sources.count("data/mods.cfg") || !sources.count("data/gamedata.base") ||
        !sources.count("data/Dialogue.mod"))
        return false;
    out = m;
    err.clear();
    return true;
}
struct Session
{
    std::string key;
    unsigned long long world;
    unsigned epoch;
    Session() : world(0), epoch(0) {}
    void Reset()
    {
        key.clear();
        world = 0;
        ++epoch;
    }
    bool Change(const std::string &k, unsigned long long w)
    {
        if (k == key && w == world)
            return false;
        key = k;
        world = w;
        ++epoch;
        return true;
    }
};
} // namespace GSM
