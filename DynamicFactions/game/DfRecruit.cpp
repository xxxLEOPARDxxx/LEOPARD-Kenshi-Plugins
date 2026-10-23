#define WIN32_LEAN_AND_MEAN
#include "game/DfRecruit.h"

#include <Windows.h>

#include <cstdio>
#include <map>
#include <set>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

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
#include <kenshi/Platoon.h>
#include <kenshi/Globals.h>
#include <kenshi/Town.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

namespace
{
    // ---- обход диалога: перенесено из RecruitMarkers.cpp один в один;
    // собеседник при проверке условий - член фракции, а не игрок (23.10: без
    // условий нашлась бесплатная ветка «для особого характера» - наняли
    // того, кого игроку нанять нельзя, без значка RecruitMarkers).
    // Добавлено: цена (наибольшее списание среди открытых реплик найма).
    //
    // Цена (23.10): реплика найма сама снимает деньги (DA_TAKE_MONEY 3000 у
    // «Hire me Outlaw 700»), но в том же дереве бывают бесплатные ветки для
    // особого характера («I'll join YOU for free!» по тегу личности). Условия
    // мы не смотрим, поэтому цена - наибольшее списание среди ВСЕХ реплик
    // найма: обычная цена, как у игрока без особых обстоятельств.

    const int kNodeLimit = 4000;        // реплик за один обход диалога
    const int kDepthLimit = 40;

    struct ScanState
    {
        Dialogue* dialogue;
        GameData* me;                   // данные самого персонажа
        Character* talker;              // собеседник - член фракции; NULL - условия не смотреть
        int nodes;
        int price;                      // наибольшая цена среди найденных реплик найма
        bool all;                       // обойти все реплики найма, а не до первой
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
        if (s.talker != NULL && !line->checkConditions(s.dialogue, s.talker, false))
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
                if (price > s.price)
                    s.price = price;
                if (!s.all)
                    return true;
                continue;
            }
            if (FindJoin(s, line->children, depth + 1, false, price) && !s.all)
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
    int RecruitPrice(Character* c, Character* talker)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return -1;
        ScanState s;
        s.dialogue = dialogue;
        s.me = c->data;
        s.talker = talker;
        s.nodes = 0;
        s.price = -1;
        s.all = true;
        FindJoin(s, TalkToMe(dialogue), 0, true, 0);
        return s.price;
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

    // Обычные рекруты бара - персонажи, которых игра сама сажает в бары
    // города: шаблоны из «choosefrom list» его отрядов «bar squads» (у Хаба -
    // «Recruits list outlaw town generic»: Sword for hire 700/800, hire medic,
    // Skeleton recruit...). Стражники, наёмники гильдии и уникальные из
    // модов на бары сюда не попадают (23.10: нанимали стражника).
    void BarRecruitTemplates(TownBase* town, std::set<GameData*>& out)
    {
        out.clear();
        GameData* const td = town->data;
        if (td == NULL)
            return;
        const Ogre::vector<GameDataReference>::type* const squads = td->getReferenceListIfExists("bar squads");
        if (squads == NULL)
            return;
        for (size_t i = 0; i < squads->size(); ++i)
        {
            GameData* const sq = (*squads)[i].getPtr(&ou->gamedata);
            if (sq == NULL)
                continue;
            const Ogre::vector<GameDataReference>::type* const list = sq->getReferenceListIfExists("choosefrom list");
            if (list == NULL)
                continue;
            for (size_t k = 0; k < list->size(); ++k)
                if (GameData* const ch = (*list)[k].getPtr(&ou->gamedata))
                    out.insert(ch);
        }
    }

    Character* AnyMember(Faction* f)
    {
        const lektor<Platoon*>& all = f->activePlatoons;
        for (uint32_t i = 0; i < all.size(); ++i)
        {
            Platoon* const p = all.stuff[i];
            ActivePlatoon* const ap = p != NULL && !p->isDead ? p->getActivePlatoon() : NULL;
            if (ap == NULL)
                continue;
            for (uint32_t k = 0; k < ap->things.size(); ++k)
            {
                RootObject* const o = ap->things.stuff[k];
                Character* const c = o != NULL ? o->getHandle().getCharacter() : NULL;
                if (c != NULL && !c->isDead())
                    return c;
            }
        }
        return NULL;
    }

    bool InABar(Character* c)
    {
        Building* const b = c->isIndoors().getBuilding();
        return b != NULL && b->getBuildingDesignation() == BD_BAR;
    }

    DfRecruitSearchStats g_stats;
    std::map<Character*, DWORD> g_skipUntil;    // недостижимые - до какого GetTickCount

    bool Skipped(Character* c)
    {
        std::map<Character*, DWORD>::iterator it = g_skipUntil.find(c);
        if (it == g_skipUntil.end())
            return false;
        if (static_cast<LONG>(GetTickCount() - it->second) >= 0)
        {
            g_skipUntil.erase(it);
            return false;
        }
        return true;
    }

    // -1 - не кандидат, иначе цена.
    int Candidate(Character* c, TownBase* town, Faction* ours, Character* talker,
                  const std::set<GameData*>& generic)
    {
        if (c->isDead() || c->isUnconcious() || c->isPlayerCharacter() || c->isAnimal() != NULL)
            return -1;
        if (generic.find(c->data) == generic.end())
            return -1;
        if (c->getFaction() == ours || c->getCurrentTownLocation() != town)
            return -1;
        ++g_stats.generic;
        if (TemplateUnique(c))
            return -1;
        if (InABar(c))
            ++g_stats.inBar;
        const int price = RecruitPrice(c, talker);
        if (price < 0)
            return -1;
        ++g_stats.recruitable;
        if (g_stats.recruitable <= 8)
        {
            char buf[96];
            sprintf_s(buf, sizeof(buf), "%s %d%s%s; ", c->getName().c_str(), price, InABar(c) ? "" : " (street)",
                      Skipped(c) ? " (skipped)" : "");
            g_stats.list += buf;
        }
        return Skipped(c) ? -1 : price;
    }

    bool Find(TownBase* town, Faction* ours, Character*& who, int& price)
    {
        who = NULL;
        price = -1;
        if (ou == NULL || town == NULL)
            return false;
        // Условия разговора - как для члена фракции (кто-нибудь живой).
        Character* const talker = AnyMember(ours);
        if (talker == NULL)
            return false;
        std::set<GameData*> generic;
        BarRecruitTemplates(town, generic);
        g_stats.templates = static_cast<int>(generic.size());
        g_stats.generic = g_stats.inBar = g_stats.recruitable = 0;
        g_stats.list.clear();
        if (generic.empty())
            return false;
        const Ogre::Vector3 centre = town->getPosition();
        float bestDistance = 0.0f;
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL)
                continue;
            const int p = Candidate(c, town, ours, talker, generic);
            if (p < 0)
                continue;
            const float d = c->getPosition().distance(centre);
            if (who == NULL || p < price || (p == price && d < bestDistance))
            {
                who = c;
                price = p;
                bestDistance = d;
            }
        }
        return who != NULL;
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

void DfRecruitSkip(Character* c, unsigned long ms)
{
    if (c != NULL)
        g_skipUntil[c] = GetTickCount() + ms;
}

const DfRecruitSearchStats& DfLastRecruitSearch()
{
    return g_stats;
}

namespace
{
    std::vector<Faction*> g_own;
    bool (*g_origCheckConditions)(DialogLineData*, Dialogue*, Character*, bool) = NULL;

    // Есть ли найм в реплике или под ней. Реплики - данные игры, общие для
    // всех; ответ запоминается до смены игры (DfSetOwnFactions с пустым).
    std::map<DialogLineData*, bool> g_hireBelow;

    bool HireBelow(DialogLineData* line, int depth, int& nodes)
    {
        if (line == NULL || depth > kDepthLimit || ++nodes > kNodeLimit)
            return false;
        if (IsJoinLine(line))
            return true;
        DialogChoiceList* const list = line->children;
        if (list == NULL)
            return false;
        const lektor<DialogLineData*>& choices = list->conversationChoices;
        for (uint32_t i = 0; i < choices.size(); ++i)
            if (HireBelow(choices[i], depth + 1, nodes))
                return true;
        return false;
    }

    bool LeadsToHire(DialogLineData* line)
    {
        std::map<DialogLineData*, bool>::iterator it = g_hireBelow.find(line);
        if (it != g_hireBelow.end())
            return it->second;
        int nodes = 0;
        const bool yes = HireBelow(line, 0, nodes);
        g_hireBelow[line] = yes;
        return yes;
    }

    bool OwnersFactionIsOurs(Dialogue* d)
    {
        if (d == NULL || g_own.empty())
            return false;
        Character* const owner = d->getCharacter();
        if (owner == NULL)
            return false;
        Faction* const f = owner->getFaction();
        for (size_t i = 0; i < g_own.size(); ++i)
            if (g_own[i] == f)
                return true;
        return false;
    }

    bool BlockHire(DialogLineData* line, Dialogue* d)
    {
        __try
        {
            return OwnersFactionIsOurs(d) && LeadsToHire(line);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool CheckConditions_hook(DialogLineData* line, Dialogue* d, Character* target, bool isWordswap)
    {
        const bool ok = g_origCheckConditions(line, d, target, isWordswap);
        if (!ok || g_own.empty() || line == NULL)
            return ok;
        return !BlockHire(line, d);
    }
}

bool DfInstallHireBlock()
{
    return KenshiLib::SUCCESS == KenshiLib::AddHook(KenshiLib::GetRealAddress(&DialogLineData::checkConditions),
                                                    CheckConditions_hook, &g_origCheckConditions);
}

void DfSetOwnFactions(const std::vector<Faction*>& factions)
{
    g_own = factions;
    if (factions.empty())
        g_hireBelow.clear();                // другая игра - реплики могли пересоздаться
}
