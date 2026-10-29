#pragma once
#include <string>
#include <vector>
namespace MiningCore
{
    struct Rule
    {
        bool enabled;
        std::string name;
        float mult[3], cap[3];
        Rule() : enabled(true) { mult[0]=0.25f; mult[1]=mult[2]=0; cap[0]=15; cap[1]=cap[2]=0; }
    };
    struct SourceInfo { std::string sid, name; };
    void ApplySettings();
    void ReloadSources();
    std::vector<SourceInfo> Sources();
    bool ReadSource(const std::string& sid, Rule& value, Rule* defaults = NULL);
    bool WriteSource(const std::string& sid, const std::string& key, float value, int skill, bool cap);
}
