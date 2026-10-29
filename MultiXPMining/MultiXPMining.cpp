#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "multi_xp_mining"
#include <PluginSettings.h>
#include <PluginDiagnostics.h>
#include "MiningCore.h"
#include <sstream>
#include <Localization.h>
#include <ModConfigMenu.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Tasker.h>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/Building.h>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <set>
#include "MiningRule.h"

namespace
{
    CRITICAL_SECTION g_lock;
    struct Lock { Lock() { EnterCriticalSection(&g_lock); } ~Lock() { LeaveCriticalSection(&g_lock); } };
    const StatsEnumerated kStats[] = { STAT_STRENGTH, STAT_TOUGHNESS, STAT_DEXTERITY };
    const char* kSkills[] = { "Strength", "Toughness", "Dexterity" };
    using MiningCore::Rule;
    Rule g_default;
    bool g_enabled = true, g_caps = true;
    bool g_recordUnknown = true;
    bool g_deliveryReady = false, g_catalogueReady = false;
    volatile LONG g_running = 0, g_pending = 0, g_epoch = 0;
    DWORD g_lastPrune = 0, g_unknownRetryAt = 0;
    std::set<std::string> g_unknownSeen;
    std::map<std::string, std::string> g_unknownQueued;
    float g_xp = 1, g_cap = 1;
    std::map<std::string, Rule> g_rules, g_baseRules;
    DWORD g_lastDeliveryLog = 0;
    std::set<std::string> g_unknownLogged, g_disabledLogged, g_notMines;
    struct Carry
    {
        float n[3]; DWORD stamp, logged; std::string source;
        Carry() : stamp(0), logged(0) { n[0] = n[1] = n[2] = 0; }
    };
    std::map<hand, Carry> g_carry;
    struct Award
    {
        int ticks[3]; float limits[3]; bool caps;
        Award() : caps(true) { for (int i = 0; i < 3; ++i) { ticks[i] = 0; limits[i] = 1000000; } }
    };
    std::map<hand, Award> g_awards;
    __declspec(thread) bool g_extra = false;
    void (*g_origXp)(CharStats*, StatsEnumerated) = NULL;

    std::string Directory()
    {
        const std::string p = PluginSettings::Path();
        return p.substr(0, p.find_last_of("\\/") + 1);
    }
    void ReadRule(Rule& r, const std::string& p, const char* section)
    {
        r.enabled = PluginSettings::Bool(section, "Enabled", r.enabled, p);
        r.name = PluginSettings::Text(section, "DisplayName", r.name.c_str(), p);
        for (int i = 0; i < 3; ++i)
        {
            r.mult[i] = PluginSettings::Number(section, (std::string(kSkills[i]) + "Multiplier").c_str(), r.mult[i], 0, 10, p);
            r.cap[i] = PluginSettings::Number(section, (std::string(kSkills[i]) + "Cap").c_str(), r.cap[i], 0, 1000, p);
        }
    }
    void LoadGlobals()
    {
        const std::string ini = PluginSettings::Path();
        g_enabled = PluginSettings::Bool("General", "Enabled", true, ini);
        g_caps = PluginSettings::Bool("General", "EnableCaps", true, ini);
        g_recordUnknown = PluginSettings::Bool("General", "GenerateUnknownSourcesIni", true, ini);
        g_xp = PluginSettings::Number("General", "GlobalXpMultiplier", 1, 0, 10, ini);
        g_cap = PluginSettings::Number("General", "GlobalCapMultiplier", 1, 0, 10, ini);
        PluginDiagnostics::Configure("General", ini);
        g_carry.clear();
        g_awards.clear();
        g_unknownQueued.clear();g_unknownSeen.clear();
        InterlockedIncrement(&g_epoch);
        InterlockedExchange(&g_running, g_enabled && g_deliveryReady ? 1 : 0);
        InterlockedExchange(&g_pending, 0);
        if (PluginDiagnostics::Enabled()) {
            std::ostringstream msg;
            msg << "MultiXPMining: settings enabled=" << g_enabled << " xp=" << g_xp
                << " caps=" << g_caps << " capMultiplier=" << g_cap;
            DebugLog(msg.str());
        }
    }
    void LoadSettings()
    {
        Lock lock;
        LoadGlobals();
        const std::string ini = PluginSettings::Path();
        g_default = Rule();
        ReadRule(g_default, ini, "Defaults");
        g_rules.clear();
        g_baseRules.clear();
        g_catalogueReady = false;
        g_unknownLogged.clear(); g_disabledLogged.clear(); g_notMines.clear();
        const std::string dir = Directory() + "SourceData\\";
        WIN32_FIND_DATAA info;
        HANDLE find = FindFirstFileA((dir + "*.ini").c_str(), &info);
        std::vector<std::string> files;
        if (find != INVALID_HANDLE_VALUE)
        {
            do { if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.push_back(info.cFileName); } while (FindNextFileA(find, &info));
            FindClose(find);
        }
        std::sort(files.begin(), files.end());
        for (size_t i = 0; i < files.size(); ++i)
        {
            const std::string path = dir + files[i];
            std::vector<char> sections(65536, 0);
            const DWORD got = GetPrivateProfileSectionNamesA(&sections[0], (DWORD)sections.size(), path.c_str());
            if (got >= sections.size() - 2) { ErrorLog("MultiXPMining: too many source sections: " + path); continue; }
            for (const char* s = &sections[0]; *s; s += strlen(s) + 1)
            {
                if (!_stricmp(s, "Metadata")) continue;
                std::map<std::string, Rule>::iterator found = g_rules.find(s);
                Rule rule = found == g_rules.end() ? g_default : found->second;
                ReadRule(rule, path, s);
                g_rules[s] = rule;
                if (_stricmp(files[i].c_str(), "99_UserOverride.ini") != 0)
                {
                    std::map<std::string, Rule>::iterator base = g_baseRules.find(s);
                    Rule baseline = base == g_baseRules.end() ? g_default : base->second;
                    ReadRule(baseline, path, s);
                    g_baseRules[s] = baseline;
                }
            }
        }
        g_carry.clear();
        g_awards.clear();
        if (PluginDiagnostics::Enabled()) {
            std::ostringstream msg; msg << "MultiXPMining: loaded " << g_rules.size() << " source rules from " << files.size() << " files";
            DebugLog(msg.str());
        }
    }
    // Catalog data is already merged by the game in mods.cfg order.
    // Farms can also use BF_MINE, so require Labouring, not just function=1.
    bool IsMiningTemplate(GameData* data)
    {
        if (data == NULL || ou == NULL || !data->listExistsAndNotEmpty("functionality")) return false;
        GameData* f=data->getFromListAsData("functionality",0,&ou->gamedata,BUILDING_FUNCTIONALITY);
        if (f == NULL) return false;
        // find does not mutate missing fields in the shared game database.
        const auto fn=f->idata.find("function"), skill=f->idata.find("stat used");
        return fn!=f->idata.end() && skill!=f->idata.end() &&
               (fn->second==BF_MINE || fn->second==BF_MINE_NATURAL) && skill->second==STAT_LABOURING;
    }
    void AddAutomatic(GameData* data)
    {
        if (g_rules.find(data->stringID) == g_rules.end())
        {
            Rule r=g_default; r.name=data->name;
            g_rules[data->stringID]=r;
            g_baseRules[data->stringID]=r;
        }
    }
    void EnsureCatalogue()
    {
        if (g_catalogueReady || ou == NULL) return;
        static lektor<GameData*> templates;
        templates.clear();
        ou->gamedata.getDataOfType(templates,BUILDING);
        if (templates.size()==0) return;
        for (unsigned i=0;i<templates.size();++i)
            if (IsMiningTemplate(templates[i])) AddAutomatic(templates[i]);
        g_catalogueReady=true;
    }
    Building* Source(Character* c)
    {
        AI* const ai = c->getAI();
        if (ai == NULL || ai->taskSystemAI == NULL) return NULL;
        Tasker* tasks[2] = { ai->taskSystemAI->actions.getFirstTask(), ai->taskSystemAI->currentlySubTasking };
        for (int i = 0; i < 2; ++i)
        {
            if (tasks[i] == NULL) continue;
            Building* b = tasks[i]->currentSubTarget.getBuilding();
            if (b == NULL) b = tasks[i]->subject.getBuilding();
            if (b != NULL) return b;
        }
        OrdersReceiver* const orders = c->getOrdersReciever();
        if (orders == NULL) return NULL;
        const TaskMatch& goal = orders->getCurrentGoal();
        Building* b = goal.subtarget.getBuilding();
        return b != NULL ? b : goal.subject.getBuilding();
    }
    Building* SafeSource(Character* c)
    {
        __try { return Source(c); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return NULL; }
    }
    void Apply(CharStats* stats)
    {
        Character* const c = stats->me;
        if (c == NULL || !c->isPlayerCharacter() || c->isDead()) return;
        Building* const source = SafeSource(c);
        if (source == NULL || source->data == NULL) return;
        float xp, cap;
        bool caps;
        int ticks[3] = {};
        {
            Lock lock;
            if (!g_enabled || !g_deliveryReady) return;
            const DWORD now=GetTickCount();
            const std::string& sid = source->data->stringID;
            std::map<std::string, Rule>::const_iterator found = g_rules.find(sid);
            if (found == g_rules.end() && g_notMines.find(sid)==g_notMines.end())
            {
                if (IsMiningTemplate(source->data)) { AddAutomatic(source->data); found=g_rules.find(sid); }
                else if (g_notMines.size()<8192) g_notMines.insert(sid);
            }
            // Unknown non-mining sources still need an explicit rule.
            if (found == g_rules.end())
            {
                if (PluginDiagnostics::Enabled() && g_unknownLogged.size() < 4096 && g_unknownLogged.insert(sid).second)
                    DebugLog("MultiXPMining: no rule, extra XP skipped: " + source->data->name + " [" + sid + "]");
                if (g_recordUnknown && g_unknownSeen.size() < 4096 && g_unknownSeen.insert(sid).second)
                {
                    g_unknownQueued[sid] = source->data->name;
                    InterlockedExchange(&g_pending,1);
                }
                return;
            }
            if (!found->second.enabled) {
                if (PluginDiagnostics::Enabled() && g_disabledLogged.insert(sid).second)
                    DebugLog("MultiXPMining: configured source is disabled: " + sid);
                return;
            }
            const Rule& rule=found->second; xp = g_xp; cap = g_cap; caps = g_caps;
            Carry& carry = g_carry[c->handle];
            if (carry.source != sid || now - carry.stamp > 1500) carry = Carry();
            carry.source = sid; carry.stamp = now;
            for (int i = 0; i < 3; ++i)
            {
                if (rule.mult[i] <= 0 || xp <= 0 || (caps && rule.cap[i]*cap <= 0)) { carry.n[i]=0; continue; }
                ticks[i] = MiningRule::Ticks(rule.mult[i] * xp, stats->getStat(kStats[i], true), rule.cap[i] * cap, caps, carry.n[i]);
            }
            if (PluginDiagnostics::Enabled() && (!carry.logged || now - carry.logged >= 5000))
            {
                carry.logged = now;
                std::ostringstream msg;
                msg << "MultiXPMining: " << c->displayName << " source=" << sid << " rawTicks="
                    << ticks[0] << ',' << ticks[1] << ',' << ticks[2] << " caps="
                    << rule.cap[0]*cap << ',' << rule.cap[1]*cap << ',' << rule.cap[2]*cap
                    << " current=" << stats->getStat(kStats[0],true) << ','
                    << stats->getStat(kStats[1],true) << ',' << stats->getStat(kStats[2],true);
                DebugLog(msg.str());
            }
            if (ticks[0]==0 && ticks[1]==0 && ticks[2]==0) return;
            InterlockedExchange(&g_pending,1);
            Award& award = g_awards[c->handle];
            award.caps = caps;
            for (int i = 0; i < 3; ++i)
            {
                if (ticks[i] == 0) continue;
                award.ticks[i] += ticks[i];
                const float limit = rule.cap[i] * cap;
                if (limit < award.limits[i]) award.limits[i] = limit;
            }

        }
    }
    void AwardTicks(Character* c, const Award& award)
    {
        CharStats* const stats = c->getStats();
        if (stats == NULL || c->isDead()) return;
        g_extra = true;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < award.ticks[i]; ++j)
            {
                const float limit = award.limits[i];
                if (award.caps && stats->getStat(kStats[i], true) >= limit) break;
                // Enter through the public function, so XP_Overhaul and racial
                // multipliers keep working whatever the hook order is.
                stats->xpStat_timeBased(kStats[i]);
                if (award.caps && stats->getStat(kStats[i], true) > limit)
                    stats->getStatRef(kStats[i]) = limit;
            }
        g_extra = false;
    }
    bool DiscoveryValue(const std::string& path,const char* sid,const char* key,const char* value)
    {
        // User-edited discovery sections are not settings defaults: preserve
        // every existing nonempty key, and repair only missing partial writes.
        return !PluginSettings::Text(sid,key,"",path).empty() ||
               WritePrivateProfileStringA(sid,key,value,path.c_str())!=0;
    }
    void (*g_origWorld)(GameWorld*, float) = NULL;
    void World(GameWorld* self, float time)
    {
        g_origWorld(self,time);
        struct Delivery { hand target; Award award; };
        static std::vector<Delivery> pending;
        static std::map<std::string,std::string> unknown;
        static LONG seenEpoch=0;
        LONG epoch=g_epoch;
        if(seenEpoch!=epoch) { pending.clear();unknown.clear();seenEpoch=epoch; }
        if (!g_running) return;
        const DWORD now=GetTickCount();
        const bool queued=InterlockedExchange(&g_pending,0)!=0;
        const bool prune=now-g_lastPrune>=5000;
        if (!queued && !prune && unknown.empty()) return;
        pending.clear();
        {
            Lock lock;
            epoch=g_epoch;
            if(seenEpoch!=epoch) { pending.clear();unknown.clear();seenEpoch=epoch; }
            if (queued)
            {
                for (std::map<hand,Award>::iterator i=g_awards.begin();i!=g_awards.end();++i)
                {
                    if (!i->second.ticks[0] && !i->second.ticks[1] && !i->second.ticks[2]) continue;
                    Delivery d; d.target=i->first; d.award=i->second; pending.push_back(d);
                    i->second=Award(); // keep the node for the next tick
                }
                if (unknown.empty()) unknown.swap(g_unknownQueued);
                else if (!g_unknownQueued.empty()) InterlockedExchange(&g_pending,1);
            }
            if (prune)
            {
                g_lastPrune=now;
                for (std::map<hand,Carry>::iterator i=g_carry.begin();i!=g_carry.end();)
                    if (now-i->second.stamp>10000) { g_awards.erase(i->first); g_carry.erase(i++); } else ++i;
            }
        }
        // File IO is never in the XP hook. Retry failed writes at most once
        // per five seconds and retain the item until every key is written.
        if (!unknown.empty() && epoch==g_epoch &&
            (g_unknownRetryAt==0 || (LONG)(now-g_unknownRetryAt)>=0))
        {
            const std::string path=Directory()+"UnknownSources.ini";
            unsigned count=0;
            while (!unknown.empty() && count++<16 && epoch==g_epoch)
            {
                std::map<std::string,std::string>::iterator i=unknown.begin();
                const char* sid=i->first.c_str();
                bool ok=DiscoveryValue(path,sid,"Enabled","0");
                ok=DiscoveryValue(path,sid,"DisplayName",i->second.c_str()) && ok;
                for (int j=0;j<3;++j)
                {
                    char n[48];sprintf_s(n,"%.6g",g_default.mult[j]);
                    ok=DiscoveryValue(path,sid,(std::string(kSkills[j])+"Multiplier").c_str(),n) && ok;
                    sprintf_s(n,"%.6g",g_default.cap[j]);
                    ok=DiscoveryValue(path,sid,(std::string(kSkills[j])+"Cap").c_str(),n) && ok;
                }
                if(!ok) { ErrorLog("MultiXPMining: cannot write discovery data: "+path);g_unknownRetryAt=now+5000;break; }
                g_unknownRetryAt=0;
                unknown.erase(i);
            }
        }
        if (PluginDiagnostics::Enabled() && !pending.empty() && now-g_lastDeliveryLog>=5000)
        {
            g_lastDeliveryLog=now;
            std::ostringstream msg;msg << "MultiXPMining: deliver extra XP to " << pending.size() << " worker(s)";DebugLog(msg.str());
        }
        for (size_t i=0;i<pending.size() && epoch==g_epoch;++i)
        {
            Character* c=pending[i].target.getCharacter();
            if (c && c->isPlayerCharacter()) AwardTicks(c,pending[i].award);
        }
    }
    void Xp(CharStats* stats, StatsEnumerated skill)
    {
        g_origXp(stats, skill);
        if (stats != NULL && skill == STAT_LABOURING && !g_extra && g_running) Apply(stats);
    }
    void ResetSession()
    {
        Lock lock;
        InterlockedIncrement(&g_epoch);
        g_carry.clear();g_awards.clear();g_unknownQueued.clear();g_unknownSeen.clear();
        g_unknownRetryAt=0;
        InterlockedExchange(&g_pending,0);
    }
    void (*g_origNewGame)(SaveFileSystem*)=NULL;
    void NewGame(SaveFileSystem* self) { ResetSession();g_origNewGame(self); }
    void (*g_origLoadGame)(SaveFileSystem*,const std::string&)=NULL;
    void LoadGame(SaveFileSystem* self,const std::string& path) { ResetSession();g_origLoadGame(self,path); }
    void (*g_origLoad)(Character*, GameSaveState*) = NULL;
    void LoadCharacter(Character* c, GameSaveState* state)
    {
        g_origLoad(c, state);
        if (c != NULL && c->isPlayerCharacter())
        {
            Lock lock;
            g_carry.erase(c->handle);
            g_awards.erase(c->handle);
        }
    }
}
namespace MiningCore
{
    void ApplySettings() { Lock lock; LoadGlobals(); }
    void ReloadSources() { LoadSettings(); }
    std::vector<SourceInfo> Sources()
    {
        Lock lock;
        EnsureCatalogue();
        std::vector<SourceInfo> result;
        result.reserve(g_rules.size());
        for (std::map<std::string, Rule>::const_iterator i=g_rules.begin(); i!=g_rules.end(); ++i)
        {
            SourceInfo info; info.sid=i->first;
            GameData* const data = ou == NULL ? NULL : ou->gamedata.getData(i->first);
            if (data == NULL) continue; // stale compatibility rows for absent mods
            info.name = data != NULL && !data->name.empty() ? data->name :
                        i->second.name.empty() ? i->first : i->second.name;
            result.push_back(info);
        }
        return result;
    }
    bool ReadSource(const std::string& sid, Rule& value, Rule* defaults)
    {
        Lock lock;
        std::map<std::string, Rule>::const_iterator i=g_rules.find(sid);
        if (i==g_rules.end()) return false;
        value=i->second;
        if (defaults) {
            i=g_baseRules.find(sid);
            *defaults=i==g_baseRules.end() ? g_default : i->second;
        }
        return true;
    }
    bool WriteSource(const std::string& sid, const std::string& key, float value, int skill, bool cap)
    {
        if (!_finite(value) || value < 0 || value > (cap ? 1000 : 10) || skill < -1 || skill > 2) return false;
        Lock lock;
        std::map<std::string, Rule>::iterator i=g_rules.find(sid);
        if (i==g_rules.end()) return false;
        const std::string dir=Directory()+"SourceData";
        CreateDirectoryA(dir.c_str(),NULL);
        char text[48]; sprintf_s(text,"%.6g",value);
        if (!WritePrivateProfileStringA(sid.c_str(),key.c_str(),text,(dir+"\\99_UserOverride.ini").c_str())) return false;
        if (skill<0) i->second.enabled=value!=0;
        else if (cap) i->second.cap[skill]=value;
        else i->second.mult[skill]=value;
        g_carry.clear(); g_awards.clear();
        PluginDiagnostics::Write("MultiXPMining: ["+sid+"] "+key+"="+text);
        return true;
    }
}
__declspec(dllexport) void startPlugin()
{
    InitializeCriticalSection(&g_lock);
    LoadSettings();
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&CharStats::xpStat_timeBased), Xp, &g_origXp) != KenshiLib::SUCCESS)
        ErrorLog("MultiXPMining: cannot hook time-based XP");
    g_deliveryReady = KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), World, &g_origWorld) == KenshiLib::SUCCESS;
    if (!g_deliveryReady)
    {
        g_enabled = false;
        ErrorLog("MultiXPMining: cannot hook XP delivery, disabled");
    }
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_loadFromSerialise), LoadCharacter, &g_origLoad) != KenshiLib::SUCCESS)
    {
        g_deliveryReady = false;
        ErrorLog("MultiXPMining: cannot clear pending XP on load, disabled");
    }
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),NewGame,&g_origNewGame)!=KenshiLib::SUCCESS ||
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),LoadGame,&g_origLoadGame)!=KenshiLib::SUCCESS)
    {
        g_deliveryReady=false;ErrorLog("MultiXPMining: cannot invalidate session queues, disabled");
    }
    InterlockedExchange(&g_running,g_enabled && g_deliveryReady ? 1 : 0);
}
