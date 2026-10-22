#define WIN32_LEAN_AND_MEAN
#include "game/DfGame.h"

#include <Windows.h>

#include <cstdio>

#include <Debug.h>

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

const char* const kDfFactionRecord = "1-DynamicFactions.mod";
const char* const kDfSquadRecord = "2-DynamicFactions.mod";
const char* const kDfHubRecord = "18919-Newwworld.mod";

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
        return ou->getTimeFromStamp_inGameHours(ou->getTimeStamp());
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

    // ---- снимок ----

    bool TakeSnapshot(Faction* f, DfSnapshot& out)
    {
        if (f == NULL)
            return false;
        out.members = f->countPopulation(NULL);
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

    bool Found(Faction* f, TownBase* hub, int members, int money)
    {
        if (f == NULL || hub == NULL || ou == NULL || ou->theFactory == NULL)
            return false;
        GameData* const squad = ou->gamedata.getData(kDfSquadRecord);
        if (squad == NULL)
            return false;
        Platoon* const p = ou->theFactory->createRandomSquad(
            f, hub->getPosition(), hub, members, NULL, squad, NULL, NULL, NULL,
            true, hand(), NULL, 1.0f, SQ_RESIDENT, false);
        if (p == NULL)
            return false;
        if (f->factionOwnerships != NULL)
            f->factionOwnerships->addMoney(money);
        return true;
    }

    bool SafeFound(Faction* f, TownBase* hub, int members, int money)
    {
        __try
        {
            return Found(f, hub, members, money);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told = 0;
            TellOnce(told, "DynamicFactions: creating the starting squad failed once");
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

bool DfFound(Faction* f, TownBase* hub, int members, int money)
{
    return SafeFound(f, hub, members, money);
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
