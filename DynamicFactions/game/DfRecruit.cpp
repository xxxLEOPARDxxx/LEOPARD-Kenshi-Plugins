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

#include <kenshi/Building/Building.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Town.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

namespace
{
    // ---- обход диалога: перенесено из RecruitMarkers.cpp один в один,
    // кроме проверки условий - она за собеседника-игрока, у фракции его нет.
    // Добавлено: цена (DA_TAKE_MONEY на пути) и запирание найденных реплик.

    const int kNodeLimit = 4000;        // реплик за один обход диалога
    const int kDepthLimit = 40;

    struct ScanState
    {
        Dialogue* dialogue;
        GameData* me;                   // данные самого персонажа
        int nodes;
        int price;                      // цена найденного пути
        bool lock;                      // запирать все реплики найма, а не искать первую
        int locked;
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

    int TakenMoney(DialogLineData* line)
    {
        int most = 0;
        const lektor<DialogLineData::DialogAction*>& actions = line->actions;
        for (uint32_t i = 0; i < actions.size(); ++i)
        {
            const DialogLineData::DialogAction* a = actions[i];
            if (a != NULL && a->key == DA_TAKE_MONEY && a->value > most)
                most = a->value;
        }
        return most;
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

    // pathPrice - самое большое DA_TAKE_MONEY от корня до этой ветки.
    bool FindJoin(ScanState& s, DialogChoiceList* list, int depth, bool root, int pathPrice)
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
            const int taken = TakenMoney(line);
            const int price = taken > pathPrice ? taken : pathPrice;
            if (IsJoinLine(line))
            {
                if (!s.lock)
                {
                    s.price = price;
                    return true;
                }
                s.dialogue->setLineLocked(line, true);
                ++s.locked;
                continue;
            }
            if (FindJoin(s, line->children, depth + 1, false, price) && !s.lock)
                return true;
        }
        return false;
    }

    typedef std::map<EventTriggerEnum, DialogChoiceList*, std::less<EventTriggerEnum>,
                     Ogre::STLAllocator<std::pair<EventTriggerEnum const, DialogChoiceList*>,
                                        Ogre::GeneralAllocPolicy> > Lists;

    DialogChoiceList* TalkToMe(Dialogue* dialogue)
    {
        const Lists& lists = dialogue->conversationsMain;
        Lists::const_iterator it = lists.find(EV_PLAYER_TALK_TO_ME);
        return it != lists.end() ? it->second : NULL;
    }

    // -1 - нанять нельзя, иначе цена.
    int RecruitPrice(Character* c)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return -1;
        ScanState s;
        s.dialogue = dialogue;
        s.me = c->data;
        s.nodes = 0;
        s.price = -1;
        s.lock = false;
        s.locked = 0;
        return FindJoin(s, TalkToMe(dialogue), 0, true, 0) ? s.price : -1;
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

    bool InABar(Character* c)
    {
        Building* const b = c->isIndoors().getBuilding();
        return b != NULL && b->getBuildingDesignation() == BD_BAR;
    }

    // -1 - не кандидат, иначе цена.
    int Candidate(Character* c, TownBase* town, Faction* ours)
    {
        if (c->isDead() || c->isUnconcious() || c->isPlayerCharacter() || c->isAnimal() != NULL)
            return -1;
        if (c->getFaction() == ours || c->getCurrentTownLocation() != town || !InABar(c))
            return -1;
        if (TemplateUnique(c))
            return -1;
        return RecruitPrice(c);
    }

    bool Find(TownBase* town, Faction* ours, Character*& who, int& price)
    {
        who = NULL;
        price = -1;
        if (ou == NULL || town == NULL)
            return false;
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL)
                continue;
            const int p = Candidate(c, town, ours);
            if (p >= 0 && (who == NULL || p < price))
            {
                who = c;
                price = p;
            }
        }
        return who != NULL;
    }

    void Lock(Character* c)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return;
        ScanState s;
        s.dialogue = dialogue;
        s.me = c->data;
        s.nodes = 0;
        s.price = -1;
        s.lock = true;
        s.locked = 0;
        FindJoin(s, TalkToMe(dialogue), 0, true, 0);
    }
}

bool DfFindBarRecruit(TownBase* town, Faction* ours, Character*& who, int& price)
{
    __try
    {
        return Find(town, ours, who, price);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: looking for a bar recruit failed once - no recruit this hour");
        who = NULL;
        price = -1;
        return false;
    }
}

void DfLockRecruitLines(Character* c)
{
    __try
    {
        if (c != NULL)
            Lock(c);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog("DynamicFactions: locking the hire lines of a recruit failed once");
    }
}
