#include "core/DfState.h"

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <limits>
#include <set>

DfFactionState::DfFactionState()
    : founded(false), extinct(false), workFault(false), foundedHour(0.0), lastTickHour(0.0), techLevel(0), techPoints(0)
{
}

void DfAddEvent(DfFactionState& s, double hour, const std::string& text)
{
    DfEvent e;
    e.hour = hour;
    e.text = text;
    s.events.push_back(e);
    if (s.events.size() > kDfMaxEvents)
        s.events.erase(s.events.begin(), s.events.end() - kDfMaxEvents);
}

namespace
{
    std::string Escape(const std::string& v)
    {
        std::string r;
        r.reserve(v.size());
        for (size_t i = 0; i < v.size(); ++i)
        {
            const char c = v[i];
            if (c == '\\')
                r += "\\\\";
            else if (c == '\n')
                r += "\\n";
            else if (c == '\r')
                r += "\\r";
            else
                r += c;
        }
        return r;
    }

    bool Unescape(const std::string& v, std::string& r)
    {
        r.clear();
        for (size_t i = 0; i < v.size(); ++i)
        {
            if (v[i] != '\\')
            {
                r += v[i];
                continue;
            }
            if (++i >= v.size())
                return false;
            if (v[i] == '\\')
                r += '\\';
            else if (v[i] == 'n')
                r += '\n';
            else if (v[i] == 'r')
                r += '\r';
            else
                return false;
        }
        return true;
    }

    std::string Num(double v)
    {
        char buf[64];
        sprintf_s(buf, sizeof(buf), "%.17g", v);
        return buf;
    }

    std::string Num(int v)
    {
        char buf[32];
        sprintf_s(buf, sizeof(buf), "%d", v);
        return buf;
    }

    bool ParseDouble(const std::string& v, double& out)
    {
        if (v.empty())
            return false;
        char* end = NULL;
        errno = 0;
        out = std::strtod(v.c_str(), &end);
        const double max = (std::numeric_limits<double>::max)();
        return end == v.c_str() + v.size() && errno != ERANGE && out == out && out >= -max && out <= max;
    }

    bool ParseInt(const std::string& v, int& out)
    {
        if (v.empty())
            return false;
        char* end = NULL;
        errno = 0;
        const long n = std::strtol(v.c_str(), &end, 10);
        if (end != v.c_str() + v.size() || errno == ERANGE ||
            n < (std::numeric_limits<int>::min)() || n > (std::numeric_limits<int>::max)())
            return false;
        out = static_cast<int>(n);
        return true;
    }

    bool ParseBool(const std::string& v, bool& out)
    {
        if (v != "0" && v != "1")
            return false;
        out = v == "1";
        return true;
    }

    void Line(std::string& out, const char* key, const std::string& value)
    {
        out += key;
        out += '=';
        out += value;
        out += '\n';
    }

    // Одна строка key=value в текущую фракцию. Незнакомые ключи пропускаем
    // (файл от более новой версии), испорченные значения - ошибка.
    bool ApplyField(DfFactionState& s, const std::string& key, const std::string& raw)
    {
        // Future keys may use a future value syntax; do not interpret them.
        if (key != "id" && key != "name" && key != "race" && key != "founded" && key != "extinct" &&
            key != "workFault" && key != "foundedHour" && key != "lastTickHour" && key != "techLevel" &&
            key != "techPoints" && key != "node" && key != "event")
            return true;
        std::string value;
        if (!Unescape(raw, value))
            return false;
        if (key == "id")
            s.recordId = value;
        else if (key == "name")
            s.name = value;
        else if (key == "race")
            s.race = value;
        else if (key == "founded")
            return ParseBool(value, s.founded);
        else if (key == "extinct")
            return ParseBool(value, s.extinct);
        else if (key == "workFault")
            return ParseBool(value, s.workFault);
        else if (key == "foundedHour")
            return ParseDouble(value, s.foundedHour) && s.foundedHour >= 0.0;
        else if (key == "lastTickHour")
            return ParseDouble(value, s.lastTickHour) && s.lastTickHour >= 0.0;
        else if (key == "techLevel")
            return ParseInt(value, s.techLevel) && s.techLevel >= 0 && s.techLevel <= kDfMaxStateTechLevel;
        else if (key == "techPoints")
            return ParseInt(value, s.techPoints) && s.techPoints >= 0;
        else if (key == "node")
        {
            if (s.nodes.size() >= kDfMaxStateNodesPerFaction)
                return false;
            // x|y|z|stringID|stored
            std::vector<std::string> parts;
            size_t from = 0;
            while (true)
            {
                const size_t bar = value.find('|', from);
                parts.push_back(value.substr(from, bar == std::string::npos ? std::string::npos : bar - from));
                if (bar == std::string::npos)
                    break;
                from = bar + 1;
            }
            double x = 0.0, y = 0.0, z = 0.0;
            DfKnownNode n;
            if (parts.size() != 5 || !ParseDouble(parts[0], x) || !ParseDouble(parts[1], y) ||
                !ParseDouble(parts[2], z) || parts[3].empty() || !ParseInt(parts[4], n.stored) || n.stored < 0)
                return false;
            const double max = (std::numeric_limits<float>::max)();
            if (x < -max || x > max || y < -max || y > max || z < -max || z > max)
                return false;
            n.x = static_cast<float>(x);
            n.y = static_cast<float>(y);
            n.z = static_cast<float>(z);
            n.itemSid = parts[3];
            s.nodes.push_back(n);
        }
        else if (key == "event")
        {
            const size_t bar = value.find('|');
            double hour = 0.0;
            if (bar == std::string::npos || !ParseDouble(value.substr(0, bar), hour) || hour < 0.0)
                return false;
            DfAddEvent(s, hour, value.substr(bar + 1));
        }
        return true;
    }
}

std::string DfSerialize(const std::vector<DfFactionState>& all)
{
    std::string out;
    for (size_t i = 0; i < all.size(); ++i)
    {
        const DfFactionState& s = all[i];
        out += "[faction " + Num(static_cast<int>(i)) + "]\n";
        Line(out, "id", Escape(s.recordId));
        Line(out, "name", Escape(s.name));
        Line(out, "race", Escape(s.race));
        Line(out, "founded", s.founded ? "1" : "0");
        Line(out, "extinct", s.extinct ? "1" : "0");
        Line(out, "workFault", s.workFault ? "1" : "0");
        Line(out, "foundedHour", Num(s.foundedHour));
        Line(out, "lastTickHour", Num(s.lastTickHour));
        Line(out, "techLevel", Num(s.techLevel));
        Line(out, "techPoints", Num(s.techPoints));
        for (size_t e = 0; e < s.events.size(); ++e)
            Line(out, "event", Num(s.events[e].hour) + "|" + Escape(s.events[e].text));
        for (size_t k = 0; k < s.nodes.size(); ++k)
        {
            const DfKnownNode& n = s.nodes[k];
            Line(out, "node", Num(static_cast<double>(n.x)) + "|" + Num(static_cast<double>(n.y)) + "|" +
                              Num(static_cast<double>(n.z)) + "|" + Escape(n.itemSid) + "|" + Num(n.stored));
        }
    }
    return out;
}

bool DfDeserialize(const std::string& text, std::vector<DfFactionState>& out)
{
    out.clear();
    if (text.size() > kDfMaxStateBytes || text.find('\0') != std::string::npos)
        return false;
    std::vector<DfFactionState> result;
    size_t pos = 0;
    while (pos < text.size())
    {
        size_t end = text.find('\n', pos);
        if (end == std::string::npos)
            end = text.size();
        if (end - pos > kDfMaxStateLineBytes)
            return false;
        std::string line = text.substr(pos, end - pos);
        pos = end + 1;
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        if (line[0] == '[')
        {
            int index = 0;
            if (line.size() < 11 || line.compare(0, 9, "[faction ") != 0 || line[line.size() - 1] != ']' ||
                !ParseInt(line.substr(9, line.size() - 10), index) || index < 0 ||
                result.size() >= kDfMaxStateFactions)
                return false;
            result.push_back(DfFactionState());
            continue;
        }

        const size_t eq = line.find('=');
        if (eq == std::string::npos || result.empty())
            return false;
        if (!ApplyField(result.back(), line.substr(0, eq), line.substr(eq + 1)))
            return false;
    }

    std::set<std::string> ids;
    for (size_t i = 0; i < result.size(); ++i)
    {
        const DfFactionState& s = result[i];
        if (s.recordId.empty() || !ids.insert(s.recordId).second)
            return false;
        // A completed legacy level consumes all its points immediately.
        if ((s.techLevel == kDfMaxStateTechLevel && s.techPoints != 0) ||
            s.techPoints >= 100 * (s.techLevel + 1))
            return false;
    }
    out.swap(result);
    return true;
}
