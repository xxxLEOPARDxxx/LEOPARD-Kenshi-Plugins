#define WIN32_LEAN_AND_MEAN
#include "game/DfRecruit.h"

#include <Windows.h>

#include <map>
#include <string>

#include <Debug.h>

// conversationsMain у Dialogue закрытый
#define private public
#define protected public
#include <kenshi/Dialogue.h>
#undef private
#undef protected

#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Town.h>
#include <kenshi/util/lektor.h>

namespace
{
    // ---- обход диалога: перенесено из RecruitMarkers.cpp один в один,
    // кроме проверки условий - она за собеседника-игрока, у фракции его нет.

    const int kNodeLimit = 4000;        // реплик за один обход диалога
    const int kDepthLimit = 40;

    struct ScanState
    {
        Dialogue* dialogue;
        GameData* me;                   // данные самого персонажа
        int nodes;
    };

    bool IsJoinLine(DialogLineData* line)
    {
        const lektor<DialogLineData::DialogAction*>& actions = line->actions;
        for (uint32_t i = 0; i < actions.size(); ++i)
        {
            const DialogLineData::DialogAction* a = actions[i];
            if (a != NULL && (a->key == DA_JOIN_SQUAD_WITH_EDIT || a->key == DA_JOIN_SQUAD_FAST))
                return true;
        }
        return false;
    }

    bool LineOpen(ScanState& s, DialogLineData* line, bool root)
    {
        if (s.dialogue->isLocked(line))
            return false;
        if (line->onceOnly && line->saidItBefore())
            return false;
        // Разговор для другого конкретного персонажа из общего пакета.
        if (root && s.me != NULL && line->hasSpecificCharacterRequirement() &&
            !line->isForSpecificCharacter(s.me))
            return false;
        return true;
    }

    bool FindJoin(ScanState& s, DialogChoiceList* list, int depth, bool root)
    {
        if (list == NULL || depth > kDepthLimit)
            return false;
        const lektor<DialogLineData*>& choices = list->conversationChoices;
        for (uint32_t i = 0; i < choices.size(); ++i)
        {
            DialogLineData* const line = choices[i];
            if (line == NULL || ++s.nodes > kNodeLimit)
                return false;
            if (!LineOpen(s, line, root))
                continue;
            if (IsJoinLine(line))
                return true;
            if (FindJoin(s, line->children, depth + 1, false))
                return true;
        }
        return false;
    }

    bool Recruitable(Character* c)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return false;
        ScanState s;
        s.dialogue = dialogue;
        s.me = c->data;
        s.nodes = 0;
        typedef std::map<EventTriggerEnum, DialogChoiceList*, std::less<EventTriggerEnum>,
                         Ogre::STLAllocator<std::pair<EventTriggerEnum const, DialogChoiceList*>,
                                            Ogre::GeneralAllocPolicy> > Lists;
        const Lists& lists = dialogue->conversationsMain;
        Lists::const_iterator it = lists.find(EV_PLAYER_TALK_TO_ME);
        return it != lists.end() && FindJoin(s, it->second, 0, true);
    }

    // Уникальный - по флагу «unique» шаблона (RecruitMarkers: isUnique() врёт).
    bool TemplateUnique(Character* c)
    {
        GameData* const d = c->data;
        if (d == NULL)
            return false;
        static const std::string key("unique");
        boost::unordered::unordered_map<std::string, bool, boost::hash<std::string>, std::equal_to<std::string>,
                                         Ogre::STLAllocator<std::pair<std::string const, bool>,
                                                            Ogre::GeneralAllocPolicy> >::const_iterator it =
            d->bdata.find(key);
        return it != d->bdata.end() && it->second;
    }

    bool Candidate(Character* c, TownBase* town, Faction* ours)
    {
        if (c->isDead() || c->isUnconcious() || c->isPlayerCharacter() || c->isAnimal() != NULL)
            return false;
        if (c->getFaction() == ours || c->getCurrentTownLocation() != town)
            return false;
        return !TemplateUnique(c) && Recruitable(c);
    }

    Character* Find(TownBase* town, Faction* ours)
    {
        if (ou == NULL || town == NULL)
            return NULL;
        const Ogre::Vector3 centre = town->getPosition();
        Character* best = NULL;
        float bestDistance = 0.0f;
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL || !Candidate(c, town, ours))
                continue;
            const float d = c->getPosition().distance(centre);
            if (best == NULL || d < bestDistance)
            {
                best = c;
                bestDistance = d;
            }
        }
        return best;
    }
}

Character* DfFindBarRecruit(TownBase* town, Faction* ours)
{
    __try
    {
        return Find(town, ours);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: looking for a bar recruit failed once - no recruit this hour");
        return NULL;
    }
}
