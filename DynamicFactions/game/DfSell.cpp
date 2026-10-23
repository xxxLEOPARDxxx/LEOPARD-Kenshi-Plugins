#define WIN32_LEAN_AND_MEAN
#include "game/DfSell.h"

#include <Windows.h>

#include <Debug.h>

#include <kenshi/Character.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/Town.h>

namespace
{
    // Флаг шаблона персонажа - как TemplateFlag в RecruitMarkers.
    bool TemplateFlag(Character* c, const std::string& key)
    {
        GameData* const d = c->data;
        if (d == NULL)
            return false;
        boost::unordered::unordered_map<std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, bool>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->bdata.find(key);
        return it != d->bdata.end() && it->second;
    }

    void Find(TownBase* town, std::vector<DfTrader>& out)
    {
        out.clear();
        if (ou == NULL || town == NULL)
            return;
        static const std::string kTrader("is trader");
        const Ogre::Vector3 centre = town->getPosition();
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL || c->isDead() || c->isUnconcious() || c->isPlayerCharacter())
                continue;
            if (c->getCurrentTownLocation() != town || !TemplateFlag(c, kTrader))
                continue;
            ActivePlatoon* const ap = c->getPlatoon();
            Ownerships* const own = ap != NULL && ap->me != NULL ? ap->me->getOwnerships() : NULL;
            DfTrader t;
            t.who = c->getHandle();
            t.name = c->getName();
            t.money = own != NULL ? own->money : 0;
            t.distance = c->getPosition().distance(centre);
            out.push_back(t);
        }
    }
}

void DfFindTraders(TownBase* town, std::vector<DfTrader>& out)
{
    __try
    {
        Find(town, out);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: looking for traders failed once");
    }
}
