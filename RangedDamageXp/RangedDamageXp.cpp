// RangedDamageXp - опыт стрелку ещё и за нанесённый урон.
//
// У игры опыт арбалетчика идёт за сам выстрел: попал ты слабым болтом
// или выбил противнику руку - одно и то же. Этот плагин добавляет к
// ванильному опыту прибавку, пропорциональную урону, который снаряд
// действительно нанёс.
//
// ГДЕ ЛОВИМ. MedicalSystem::addWound - игра зовёт его у цели, когда удар
// превращается в рану. В нём сразу всё: урон (Damages), атакующий
// (RootObject*) и снаряд (Harpoon*). Снаряд не пустой только у
// выстрелов - по нему и отличаем стрельбу от ближнего боя.
//
// КОГДА НАЧИСЛЯЕМ. Не в самом хуке: в каком потоке игра ведёт раны, мы не
// знаем, а трогать статы персонажа не из главного потока опасно. Хук
// только кладёт «кто, по кому, сколько» в очередь под замком, а опыт
// раздаёт покадровый хук главного цикла. Персонажей держим как hand -
// если стрелок за этот кадр исчез, getCharacter вернёт NULL.
//
// КАК НАЧИСЛЯЕМ. CharStats::xpStat_eventBased - той же функцией игра
// начисляет опыт за события. Значит, работает всё как обычно: расовый
// множитель, кривая и потолок XP_Overhaul, полосы LiveXpBars.
//
// Своей арифметики уровня нет: мы только решаем, сколько «события»
// отдать игре, остальное считает она.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define KLOC_DOMAIN "ranged_damage_xp"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Enums.h>
#include <kenshi/Damages.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/RootObject.h>
#include <kenshi/GunClass.h>
#include <kenshi/combat/RangedCombatClass.h>
#include <kenshi/util/hand.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
    // ---------------------------------------------------------------
    // Настройки (RangedDamageXp.ini рядом с DLL, читаются один раз)
    // ---------------------------------------------------------------

    bool  g_enabled = true;
    // Калибровка 02.10.2026 по журналу XP_Overhaul: xpStat_eventBased
    // переводит «событие» в сырой опыт примерно с коэффициентом 0.011,
    // а прицеливание у игры даёт Арбалетам ~0.014 сырого опыта в
    // секунду. Урон считаем после брони; у голой цели он в ~1.5 раза
    // больше сырого урона болта. При 0.02 попадание на ~70 нанесённого
    // урона - около секунды прицела.
    float g_xpPerDamage = 0.02f;      // «событие» за единицу урона
    float g_maxPerHit = 3.0f;         // потолок «события» за одно попадание
    float g_minDamage = 1.0f;         // меньше - не считаем (царапины)
    float g_perceptionShare = 0.25f;  // доля Восприятию от прибавки
    bool  g_turrets = true;           // турели качают навык «Турели»
    bool  g_allies = false;           // опыт за попадания по своим
    bool  g_downed = false;           // опыт за лежачих (без сознания)
    bool  g_debug = true;             // строка в журнал на каждое попадание

    std::string IniPath()
    {
        char path[MAX_PATH] = {};
        const DWORD n = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (n == 0 || n >= MAX_PATH)
            return std::string();
        std::string s(path, n);
        const std::string::size_type slash = s.find_last_of("\\/");
        return slash == std::string::npos
            ? std::string() : s.substr(0, slash + 1) + "RangedDamageXp.ini";
    }

    float ReadFloat(const std::string& ini, const char* key, float def)
    {
        char def_text[32];
        sprintf_s(def_text, "%g", def);
        char buf[64] = {};
        GetPrivateProfileStringA("RangedDamageXp", key, def_text, buf,
                                 sizeof(buf), ini.c_str());
        float v = def;
        if (sscanf_s(buf, "%f", &v) != 1 || v != v)
            return def;
        return v;
    }

    bool ReadBool(const std::string& ini, const char* key, bool def)
    {
        return GetPrivateProfileIntA("RangedDamageXp", key, def ? 1 : 0,
                                     ini.c_str()) != 0;
    }

    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        g_enabled = ReadBool(ini, "Enabled", g_enabled);
        g_xpPerDamage = ReadFloat(ini, "XpPerDamage", g_xpPerDamage);
        g_maxPerHit = ReadFloat(ini, "MaxPerHit", g_maxPerHit);
        g_minDamage = ReadFloat(ini, "MinDamage", g_minDamage);
        g_perceptionShare = ReadFloat(ini, "PerceptionShare", g_perceptionShare);
        g_turrets = ReadBool(ini, "Turrets", g_turrets);
        g_allies = ReadBool(ini, "Allies", g_allies);
        g_downed = ReadBool(ini, "DownedTargets", g_downed);
        g_debug = ReadBool(ini, "Debug", g_debug);

        if (g_xpPerDamage < 0.0f) g_xpPerDamage = 0.0f;
        if (g_maxPerHit < 0.0f) g_maxPerHit = 0.0f;
        if (g_perceptionShare < 0.0f) g_perceptionShare = 0.0f;

        char note[200];
        sprintf_s(note, "RangedDamageXp: xp/damage=%.5f max/hit=%.3f "
                  "min damage=%.1f perception=%.2f turrets=%d allies=%d",
                  g_xpPerDamage, g_maxPerHit, g_minDamage, g_perceptionShare,
                  g_turrets ? 1 : 0, g_allies ? 1 : 0);
        DebugLog(note);
    }


    // ---------------------------------------------------------------
    // Очередь попаданий: пишет хук раны, читает главный цикл
    // ---------------------------------------------------------------

    struct Hit
    {
        hand shooter;
        hand victim;
        float damageIn;    // урон, с которым игра вошла в addWound
        float damageOut;   // он же после addWound (если игра его правит)
    };

    CRITICAL_SECTION g_lock;
    std::vector<Hit> g_pending;
    std::vector<Hit> g_work;


    float Total(const Damages& d)
    {
        return d.cut + d.blunt + d.pierce;
    }


    typedef GameData* (*AddWoundFn)(MedicalSystem*, bool, CutDirection,
                                    Damages&, int&, RootObject*,
                                    AttackDirection::Enum&, Harpoon*);
    AddWoundFn g_origAddWound = NULL;

    GameData* AddWound_hook(MedicalSystem* self, bool lowBlow,
                            CutDirection area, Damages& damage, int& material,
                            RootObject* attacker,
                            AttackDirection::Enum& direction, Harpoon* harpoon)
    {
        // Не выстрел, или стрелка нет (ловушка, падение) - мимо.
        if (harpoon == NULL || attacker == NULL || self == NULL ||
            self->me == NULL || !g_enabled)
        {
            return g_origAddWound(self, lowBlow, area, damage, material,
                                  attacker, direction, harpoon);
        }

        // Лежачих отсекаем до раны: после неё цель может как раз упасть.
        const bool downed = self->unconcious || self->dead;
        const float before = Total(damage);

        GameData* const result = g_origAddWound(self, lowBlow, area, damage,
                                                material, attacker, direction,
                                                harpoon);

        if (downed && !g_downed)
            return result;
        if (attacker->getDataType() != CHARACTER)
            return result;   // турель-постройка без стрелка и прочее

        Hit hit;
        hit.shooter = attacker->handle;
        hit.victim = self->me->handle;
        hit.damageIn = before;
        hit.damageOut = Total(damage);

        EnterCriticalSection(&g_lock);
        if (g_pending.size() < 4096)
            g_pending.push_back(hit);
        LeaveCriticalSection(&g_lock);

        return result;
    }


    // ---------------------------------------------------------------
    // Раздача опыта
    // ---------------------------------------------------------------

    bool FromTurret(Character* shooter)
    {
        RangedCombatClass* const ranged = shooter->rangedCombat;
        if (ranged == NULL)
            return false;
        GunClass* const gun = ranged->getGun();
        return gun != NULL && gun->isTurret() != NULL;
    }


    void Award(CharStats* stats, StatsEnumerated stat, float amount)
    {
        if (amount > 0.0f)
            stats->xpStat_eventBased(stat, amount);
    }


    void Process(const Hit& hit)
    {
        Character* const shooter = hit.shooter.getCharacter();
        Character* const victim = hit.victim.getCharacter();
        if (shooter == NULL || victim == NULL || shooter == victim)
            return;

        // Урон - тот, что дошёл до тела: Damages после addWound. Внутри
        // игра (или KEP, если у него включено «Исправление расчёта урона»)
        // вычитает броню и стойкость и умножает на общий множитель урона.
        // Так выстрел в тяжёлый доспех даёт меньше опыта, чем в голого.
        const float damage = hit.damageOut;
        if (damage < g_minDamage)
            return;

        if (!g_allies && shooter->isAlly(victim, false))
            return;

        CharStats* const stats = shooter->getStats();
        if (stats == NULL)
            return;

        const bool turret = FromTurret(shooter);
        if (turret && !g_turrets)
            return;
        const StatsEnumerated stat = turret ? STAT_TURRETS : STAT_CROSSBOWS;

        // MaxPerHit=0 - без потолка.
        float amount = damage * g_xpPerDamage;
        if (g_maxPerHit > 0.0f && amount > g_maxPerHit)
            amount = g_maxPerHit;

        const float levelBefore = stats->getStat(stat, true);
        const float perceptionBefore = stats->getStat(STAT_PERCEPTION, true);

        Award(stats, stat, amount);
        Award(stats, STAT_PERCEPTION, amount * g_perceptionShare);

        if (g_debug)
        {
            char line[320];
            sprintf_s(line,
                "RangedDamageXp: %s hit %s, damage in=%.2f out=%.2f, %s event=%.4f "
                "level %.5f -> %.5f, perception %.5f -> %.5f",
                shooter->getName().c_str(), victim->getName().c_str(),
                hit.damageIn, hit.damageOut,
                turret ? "turrets" : "crossbows", amount,
                levelBefore, stats->getStat(stat, true),
                perceptionBefore, stats->getStat(STAT_PERCEPTION, true));
            DebugLog(line);
        }
    }


    void (*g_origMainLoop)(GameWorld* thisptr, float time) = NULL;

    void MainLoop_hook(GameWorld* thisptr, float time)
    {
        g_origMainLoop(thisptr, time);

        EnterCriticalSection(&g_lock);
        g_work.swap(g_pending);
        LeaveCriticalSection(&g_lock);

        for (size_t i = 0; i < g_work.size(); ++i)
            Process(g_work[i]);
        g_work.clear();
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "RangedDamageXp", "Ranged Damage Xp", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Shooters also get experience for the damage their shots deal."));
    api->section(api, Tr("Experience"));
    api->toggle(api, "RangedDamageXp", "Enabled", Tr("Enabled"), Tr("The shooter also gets experience for the damage dealt."), 1, MCM_RESTART);
    api->number(api, "RangedDamageXp", "XpPerDamage", Tr("Experience per damage"), Tr("Experience for each point of damage that got through armour. At 0.02 a hit of about 70 damage gives as much as a second of aiming."), 0.02f, 0.0f, 1.0f, 3, 0);
    api->number(api, "RangedDamageXp", "MaxPerHit", Tr("Limit per hit"), Tr("Caps the experience from one hit, so a lucky point-blank bolt does not give too much. 0 - no limit."), 3.0f, 0.0f, 50.0f, 1, 0);
    api->number(api, "RangedDamageXp", "MinDamage", Tr("Minimum damage"), Tr("Hits weaker than this are not counted - scratches on armour."), 1.0f, 0.0f, 50.0f, 1, 0);
    api->number(api, "RangedDamageXp", "PerceptionShare", Tr("Share for Perception"), Tr("Part of the extra experience that goes to Perception. 0 - Perception gets nothing."), 0.25f, 0.0f, 1.0f, 2, 0);
    api->section(api, Tr("Which hits count"));
    api->toggle(api, "RangedDamageXp", "Turrets", Tr("Turrets"), Tr("Shots from a turret train the Turrets skill."), 1, 0);
    api->toggle(api, "RangedDamageXp", "Allies", Tr("Hits on allies"), Tr("Give experience for hitting your own people too. Off by default, otherwise shooting allies would pay."), 0, 0);
    api->toggle(api, "RangedDamageXp", "DownedTargets", Tr("Hits on downed targets"), Tr("Give experience for hitting targets lying unconscious."), 0, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "RangedDamageXp", "Debug", Tr("Detailed log"), Tr("A line in RE_Kenshi_log.txt for every counted hit: who, whom, damage, experience, level before and after."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    InitializeCriticalSection(&g_lock);
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("RangedDamageXp: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&MedicalSystem::addWound),
                           AddWound_hook, &g_origAddWound))
    {
        ErrorLog("RangedDamageXp: could not hook MedicalSystem::addWound, "
                 "the plugin stays off");
        return;
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff),
            MainLoop_hook, &g_origMainLoop))
    {
        ErrorLog("RangedDamageXp: could not hook the game loop, "
                 "hits are seen but no xp is given");
        return;
    }

    DebugLog("RangedDamageXp: installed");
}
