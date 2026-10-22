#define WIN32_LEAN_AND_MEAN
#include "game/DfGame.h"

#include <Windows.h>

#include <cstdio>

#include <Debug.h>

#include <kenshi/CharStats.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/SharedKing.h>
#include <kenshi/Town.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

#include "game/DfRecruit.h"

const char* const kDfFactionRecord = "1-DynamicFactions.mod";
const char* const kDfSquadRecord = "2-DynamicFactions.mod";
const char* const kDfHubRecord = "18919-Newwworld.mod";
const char* const kDfFounderRecord = "2205-gamedata.base";

namespace
{
    // Один раз на место: журнал не забивается, если сбой повторяется каждый тик.
    void TellOnce(volatile LONG& told, const char* what)
    {
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog(what);
    }

    // ---- фракция ----

    Faction* BindFaction(const std::string& recordId)
    {
        if (ou == NULL || ou->factionMgr == NULL)
            return NULL;
        GameData* const data = ou->gamedata.getData(recordId);
        if (data == NULL)
            return NULL;
        return ou->factionMgr->getOrCreateFaction(data);
    }

    Faction* SafeBindFaction(const std::string& recordId)
    {
        __try
        {
            return BindFaction(recordId);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: binding the faction failed once - it skips this tick");
            return NULL;
        }
    }

    // ---- Хаб ----

    TownBase* FindHub()
    {
        if (shou == NULL || shou->townList == NULL)
            return NULL;
        const lektor<RootObject*>& towns = shou->townList->getAllTowns();
        for (uint32_t i = 0; i < towns.size(); ++i)
        {
            TownBase* const t = static_cast<TownBase*>(towns.stuff[i]);
            if (t == NULL || t->data == NULL || t->data->stringID != kDfHubRecord)
                continue;
            return t->isActive() ? t : NULL;
        }
        return NULL;
    }

    TownBase* SafeFindHub()
    {
        __try
        {
            return FindHub();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: looking for the Hub failed once - nothing is founded this tick");
            return NULL;
        }
    }

    // ---- время ----

    double GameHour()
    {
        if (ou == NULL)
            return -1.0;
        // getTimeFromStamp_inGameHours(stamp) - сколько прошло С stamp, не
        // текущее время (первая проверка 22.10: час не наступал никогда).
        return ou->getTimeStamp_inGameHours().getTotalHours();
    }

    double SafeGameHour()
    {
        __try
        {
            return GameHour();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: reading the game time failed once");
            return -1.0;
        }
    }

    // ---- численность ----

    Character* AsLivingCharacter(RootObject* o)
    {
        if (o == NULL)
            return NULL;
        Character* const c = o->getHandle().getCharacter();
        return c != NULL && !c->isDead() ? c : NULL;
    }

    // Живые члены фракции по её взводам: у загруженного - персонажи взвода,
    // у незагруженного - его счётчик. countPopulation(NULL) сразу после
    // основания отдал 10 при 4 созданных (22.10) - не годится.
    int CountMembers(Faction* f)
    {
        int n = 0;
        const lektor<Platoon*>* const lists[2] = { &f->activePlatoons, &f->unloadedPlatoons };
        for (int l = 0; l < 2; ++l)
            for (uint32_t i = 0; i < lists[l]->size(); ++i)
            {
                Platoon* const p = lists[l]->stuff[i];
                if (p == NULL || p->isDead)
                    continue;
                ActivePlatoon* const ap = l == 0 ? p->getActivePlatoon() : NULL;
                if (ap == NULL)
                {
                    n += p->_characterCountCurrent;
                    continue;
                }
                for (uint32_t k = 0; k < ap->things.size(); ++k)
                    if (AsLivingCharacter(ap->things.stuff[k]) != NULL)
                        ++n;
            }
        return n;
    }

    // ---- снимок ----

    bool TakeSnapshot(Faction* f, DfSnapshot& out)
    {
        if (f == NULL)
            return false;
        out.members = CountMembers(f);
        out.money = f->factionOwnerships != NULL ? f->factionOwnerships->money : 0;
        out.hour = GameHour();
        out.homeLoaded = FindHub() != NULL;
        return out.hour >= 0.0;
    }

    bool SafeTakeSnapshot(Faction* f, DfSnapshot& out)
    {
        __try
        {
            return TakeSnapshot(f, out);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: reading the faction failed once - it skips this tick");
            return false;
        }
    }

    // ---- основание ----

    // Все навыки персонажа = level (сложность в MCM).
    void SetAllStats(Character* c, float level)
    {
        CharStats* const st = c->getStats();
        if (st == NULL)
            return;
        for (int s = STAT_NONE + 1; s < STAT_END; ++s)
            st->getStatRef(static_cast<StatsEnumerated>(s)) = level;
    }

    // Отряд по шаблону - только главный: число бойцов шаблона игра умножает
    // своим размером отрядов (22.10: вышло 10 вместо 4). Остальных добираем
    // поштучно тем же персонажем.
    bool Found(Faction* f, TownBase* hub, int members, int money, float statLevel)
    {
        if (f == NULL || hub == NULL || ou == NULL || ou->theFactory == NULL)
            return false;
        GameData* const squad = ou->gamedata.getData(kDfSquadRecord);
        GameData* const tmpl = ou->gamedata.getData(kDfFounderRecord);
        if (squad == NULL || tmpl == NULL)
            return false;
        const Ogre::Vector3 pos = hub->getPosition();
        Platoon* const p = ou->theFactory->createRandomSquad(
            f, pos, hub, 1, NULL, squad, NULL, NULL, NULL,
            true, hand(), NULL, 1.0f, SQ_RESIDENT, false);
        if (p == NULL)
            return false;
        ActivePlatoon* const ap = p->getActivePlatoon();
        if (ap != NULL)
        {
            for (int tries = 0; tries < members && static_cast<int>(ap->things.size()) < members; ++tries)
            {
                RootObject* const root = ou->theFactory->createRandomCharacter(f, pos, ap, tmpl, NULL, 0.0f);
                Character* const c = root != NULL ? root->getHandle().getCharacter() : NULL;
                if (c == NULL)
                    continue;
                c->setFaction(f, ap);
                c->setRagdollNavmeshSafePos();
            }
            for (uint32_t k = 0; k < ap->things.size(); ++k)
                if (Character* const c = AsLivingCharacter(ap->things.stuff[k]))
                    SetAllStats(c, statLevel);
        }
        if (f->factionOwnerships != NULL)
            f->factionOwnerships->addMoney(money);
        return true;
    }

    bool SafeFound(Faction* f, TownBase* hub, int members, int money, float statLevel)
    {
        __try
        {
            return Found(f, hub, members, money, statLevel);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: creating the starting squad failed once");
            return false;
        }
    }

    // ---- действия ----

    // Списать деньги, только если их хватает.
    bool Spend(Faction* f, int cost)
    {
        Ownerships* const o = f->factionOwnerships;
        if (o == NULL || cost < 0 || o->money < cost)
            return false;
        o->setMoney(o->money - cost);
        return true;
    }

    // Загруженный взвод фракции - туда встаёт новичок.
    ActivePlatoon* LoadedPlatoon(Faction* f)
    {
        const lektor<Platoon*>& all = f->activePlatoons;
        for (uint32_t i = 0; i < all.size(); ++i)
        {
            Platoon* const p = all.stuff[i];
            if (p == NULL || p->isDead)
                continue;
            ActivePlatoon* const ap = p->getActivePlatoon();
            if (ap != NULL)
                return ap;
        }
        return NULL;
    }

    // Как нанял бы игрок: кто-то из сидящих в барах Хаба (DfFindBarRecruit)
    // переходит во взвод фракции. Нет никого - найма нет, деньги целы.
    bool Recruit(Faction* f, TownBase* hub, int cost)
    {
        if (hub == NULL)
            return false;
        Ownerships* const o = f->factionOwnerships;
        if (o == NULL || o->money < cost)
            return false;
        ActivePlatoon* const ap = LoadedPlatoon(f);
        if (ap == NULL)
            return false;
        Character* const c = DfFindBarRecruit(hub, f);
        if (c == NULL)
            return false;
        c->setFaction(f, ap);
        return Spend(f, cost);
    }

    bool Execute(Faction* f, TownBase* hub, const DfAction& a, const DfRules& r)
    {
        if (f == NULL)
            return false;
        switch (a.type)
        {
        case DF_INCOME:
            if (f->factionOwnerships == NULL || a.amount < 0)
                return false;
            f->factionOwnerships->addMoney(a.amount);
            return true;
        case DF_RECRUIT:
            return Recruit(f, hub, a.amount);
        case DF_RESEARCH:
            return a.amount > 0 && Spend(f, a.amount * r.techPointCost);
        case DF_EXTINCT:
            return true;
        case DF_FOUND:
            return false;               // основание - DfFound
        }
        return false;
    }

    bool SafeExecute(Faction* f, TownBase* hub, const DfAction& a, const DfRules& r)
    {
        __try
        {
            return Execute(f, hub, a, r);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: carrying out a decision failed once - it is skipped");
            return false;
        }
    }

    // ---- имя ----

    void ApplyName(Faction* f, const std::string& name)
    {
        if (f != NULL && !name.empty() && f->name != name)
            f->setName(name);
    }

    void SafeApplyName(Faction* f, const std::string& name)
    {
        __try
        {
            ApplyName(f, name);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: renaming the faction failed once");
        }
    }

    void FactionNames(std::vector<std::string>& out)
    {
        if (ou == NULL || ou->factionMgr == NULL)
            return;
        const lektor<Faction*>& all = ou->factionMgr->participants;
        for (uint32_t i = 0; i < all.size(); ++i)
            if (all.stuff[i] != NULL)
                out.push_back(all.stuff[i]->name);
    }

    void SafeFactionNames(std::vector<std::string>& out)
    {
        __try
        {
            FactionNames(out);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: listing the game's factions failed once");
        }
    }
}

Faction* DfBindFaction(const std::string& recordId)
{
    return SafeBindFaction(recordId);
}

TownBase* DfFindHub()
{
    return SafeFindHub();
}

double DfGameHour()
{
    return SafeGameHour();
}

bool DfTakeSnapshot(Faction* f, DfSnapshot& out)
{
    return SafeTakeSnapshot(f, out);
}

bool DfFound(Faction* f, TownBase* hub, int members, int money, float statLevel)
{
    return SafeFound(f, hub, members, money, statLevel);
}

bool DfExecute(Faction* f, TownBase* hub, const DfAction& a, const DfRules& r)
{
    return SafeExecute(f, hub, a, r);
}

void DfApplyName(Faction* f, const std::string& name)
{
    SafeApplyName(f, name);
}

void DfGameFactionNames(std::vector<std::string>& out)
{
    out.clear();
    SafeFactionNames(out);
}

std::string DfStatePath(const std::string& location, const std::string& name)
{
    if (name.empty())
        return std::string();
    std::string dir = location;
    if (!dir.empty() && dir[dir.size() - 1] != '\\' && dir[dir.size() - 1] != '/')
        dir += '\\';
    return dir + name + "\\dynamicfactions.txt";
}
