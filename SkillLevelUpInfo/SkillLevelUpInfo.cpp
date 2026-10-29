#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "skill_level_up_info"
#include <PluginSettings.h>
#include <PluginDiagnostics.h>
#include <Localization.h>
#include <ModConfigMenu.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/gui/CharacterStatsWindow.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/ScreenLabel.h>
#include <map>
#include <string>
#include <sstream>
#include <cmath>

namespace
{
    bool g_enabled = true, g_selectedOnly = false, g_log = false;
    const StatsEnumerated kStats[] = {
        STAT_STRENGTH, STAT_TOUGHNESS, STAT_DEXTERITY, STAT_PERCEPTION,
        STAT_KATANAS, STAT_SABRES, STAT_HACKERS, STAT_HEAVYWEAPONS, STAT_BLUNT, STAT_POLEARMS,
        STAT_MELEE_ATTACK, STAT_MELEE_DEFENCE, STAT_DODGE, STAT_MARTIALARTS,
        STAT_TURRETS, STAT_CROSSBOWS, STAT_FRIENDLY_FIRE,
        STAT_STEALTH, STAT_LOCKPICKING, STAT_THIEVING, STAT_ASSASSINATION,
        STAT_ATHLETICS, STAT_SWIMMING, STAT_MEDIC, STAT_ENGINEERING, STAT_ROBOTICS,
        STAT_SCIENCE, STAT_SMITHING_WEAPON, STAT_SMITHING_ARMOUR, STAT_SMITHING_BOW,
        STAT_LABOURING, STAT_FARMING, STAT_COOKING
    };
    const size_t kCount = sizeof(kStats) / sizeof(kStats[0]);
    const char* const kStatNames[kCount] = {
        "Strength", "Toughness", "Dexterity", "Perception",
        "Katanas", "Sabres", "Hackers", "Heavy weapons", "Blunt", "Polearms",
        "Melee attack", "Melee defence", "Dodge", "Martial arts",
        "Turrets", "Crossbows", "Precision shooting",
        "Stealth", "Lockpicking", "Thievery", "Assassination",
        "Athletics", "Swimming", "Medic", "Engineering", "Robotics",
        "Science", "Weapon smith", "Armour smith", "Crossbow smith",
        "Labouring", "Farming", "Cooking"
    };
    struct Snapshot { int levels[kCount]; bool live; };
    std::map<hand, Snapshot> g_seen;
    DWORD g_last = 0;
    volatile LONG g_loadGeneration = 0;
    LONG g_seenGeneration = 0;
    void LoadSettings()
    {
        const std::string p = PluginSettings::Path();
        g_enabled = PluginSettings::Bool("SkillLevelUpInfo", "Enabled", true, p);
        g_selectedOnly = PluginSettings::Bool("SkillLevelUpInfo", "SelectedOnly", false, p);
        g_log = PluginSettings::Bool("SkillLevelUpInfo", "LogToGame", false, p);
        PluginDiagnostics::Configure("SkillLevelUpInfo", p);
        if (PluginDiagnostics::Enabled())
            DebugLog(std::string("SkillLevelUpInfo: settings enabled=") + (g_enabled ? "1" : "0") +
                     " selectedOnly=" + (g_selectedOnly ? "1" : "0"));
        // Switching the notification scope never announces old gains.
        g_seen.clear();
        g_last = 0;
    }
    std::string Name(size_t index)
    {
        CharacterStatsWindow::Stat* const s = CharacterStatsWindow::getStat(kStats[index]);
        return s != NULL && !s->name.empty() ? s->name : Tr(kStatNames[index]);
    }
    void Scan()
    {
        if (!g_enabled) return;
        if (ou == NULL || ou->player == NULL || gui == NULL) { g_seen.clear(); return; }
        for (std::map<hand, Snapshot>::iterator i = g_seen.begin(); i != g_seen.end(); ++i)
            i->second.live = false;
        const lektor<Character*>& characters = ou->player->playerCharacters;
        for (unsigned i = 0; i < characters.size(); ++i)
        {
            Character* const c = characters[i];
            if (c == NULL || c->isDead()) continue;
            CharStats* const stats = c->getStats();
            if (stats == NULL) continue;
            const hand h(c);
            Snapshot next;
            next.live = true;
            bool valid = true;
            for (size_t j = 0; j < kCount; ++j)
            {
                const float n = stats->getStat(kStats[j], true);
                if (!_finite(n) || n < -1000.0f || n > 100000.0f) { valid = false; break; }
                next.levels[j] = (int)std::floor(n);
            }
            if (!valid) continue;
            std::map<hand, Snapshot>::iterator old = g_seen.find(h);
            if (old != g_seen.end())
            {
                bool changed = false;
                for (size_t j = 0; j < kCount; ++j)
                    if (next.levels[j] > old->second.levels[j]) { changed = true; break; }
                if (changed && (!g_selectedOnly || ou->player->isObjectSelected(c)))
                {
                    std::ostringstream message;
                    for (size_t j = 0; j < kCount; ++j)
                        if (next.levels[j] > old->second.levels[j])
                        {
                            if (message.tellp() > 0) message << "\n";
                            message << Name(j) << " " << next.levels[j];
                        }
                    const std::string text = message.str();
                    // Match the installed xp_info: gold (1,.84,.35), medium,
                    // RS_NORMAL, tracked offset (0,2,0). ScreenLabel owns the
                    // upward motion, fade and destruction; Debug labels do not.
                    ScreenLabel* const label = gui->createScreenLabel(text,
                        MyGUI::Colour(1.0f, 0.84f, 0.35f), ScreenLabel::LS_MEDIUM, ScreenLabel::RS_NORMAL);
                    if (label != NULL)
                    {
                        label->setTracking(h, Ogre::Vector3(0, 2, 0));
                    }
                    if (PluginDiagnostics::Enabled())
                        DebugLog("SkillLevelUpInfo: " + c->displayName + " -> " + text);
                    if (g_log) ou->showPlayerAMessage_withLog(c->displayName + ": " + text, true);
                }
            }
            if (old != g_seen.end()) old->second = next;
            else g_seen.insert(std::make_pair(h, next));
        }
        for (std::map<hand, Snapshot>::iterator i = g_seen.begin(); i != g_seen.end();)
            if (!i->second.live) g_seen.erase(i++); else ++i;
    }
    void (*g_origUpdate)(ForgottenGUI*) = NULL;
    void Update(ForgottenGUI* self)
    {
        g_origUpdate(self);
        if (!g_enabled) return;
        const DWORD now = GetTickCount();
        if (now - g_last < 250) return;
        const LONG generation = InterlockedCompareExchange(&g_loadGeneration, 0, 0);
        if (generation != g_seenGeneration)
        {
            g_seen.clear();
            PluginDiagnostics::Write("SkillLevelUpInfo: save load, skill baselines reset");
            g_seenGeneration = generation;
        }
        if (g_last && now - g_last > 2000) g_seen.clear();
        g_last = now;
        Scan();
    }
    // Re-baseline on save load, including a quick load with the same handles.
    void (*g_origLoad)(Character*, GameSaveState*) = NULL;
    void Load(Character* c, GameSaveState* state)
    {
        g_origLoad(c, state);
        if (g_enabled && c != NULL && c->isPlayerCharacter())
            InterlockedIncrement(&g_loadGeneration);
    }
    void (*g_origNewGame)(SaveFileSystem*) = NULL;
    void NewGame(SaveFileSystem* self)
    {
        InterlockedIncrement(&g_loadGeneration);
        g_origNewGame(self);
    }
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string p = PluginSettings::Path();
    api->beginMod(api, "SkillLevelUpInfo", "Skill Level Up Info", p.c_str(), LoadSettings);
    if (api->version >= 2) api->info(api, Tr("Only whole skill level increases above your characters. No XP bars or fractional gain messages."));
    api->toggle(api, "SkillLevelUpInfo", "Enabled", Tr("Enabled"), Tr("Show a message when a skill reaches a new whole level."), 1, 0);
    api->toggle(api, "SkillLevelUpInfo", "SelectedOnly", Tr("Selected characters only"), Tr("Track the whole player faction, but only announce gains for selected characters."), 0, 0);
    api->toggle(api, "SkillLevelUpInfo", "LogToGame", Tr("Copy to game log"), Tr("Also add the character name and new skill level to the game's message log."), 0, 0);
    api->toggle(api, "SkillLevelUpInfo", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: settings, skill level increases and save-load baseline resets."), 0, 0);
}
__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update), Update, &g_origUpdate) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook GUI update");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_loadFromSerialise), Load, &g_origLoad) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook character load");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame), NewGame, &g_origNewGame) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook new game");
}
