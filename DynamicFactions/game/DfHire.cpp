#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "dynamic_factions"
#include <Localization.h>

#include "game/DfHire.h"

#include <Windows.h>

#include <cstdio>

#include <Debug.h>

#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Character.h>
#include <kenshi/Enums.h>
#include <kenshi/Faction.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Globals.h>
#include <kenshi/Platoon.h>
#include <kenshi/Town.h>
#include <kenshi/util/lektor.h>

#include "game/DfRecruit.h"

namespace
{
    const float kTalkDistance = 5.0f;       // подошёл - разговор
    const float kNearDistance = 10.0f;      // рядом, но ближе не подойти (стол, стойка)
    const DWORD kStuckMs = 8000;            // столько не приближался - говорим отсюда
    const float kLostDistance = 12.0f;      // во время разговора ушли дальше - снова идти
    const DWORD kOrderEveryMs = 5000;       // повтор приказа «следовать»
    const DWORD kLineMs = 3000;             // реплика держится над головой
    const DWORD kGiveUpMs = 240000;         // не дошёл за 4 минуты - срыв
    const int kLines = 4;

    bool g_hireDebug = false;

    void TellOnce(volatile LONG& told, const char* what)
    {
        if (InterlockedExchange(&told, 1) == 0)
            ErrorLog(what);
    }

    bool Able(Character* c)
    {
        return c != NULL && !c->isDead() && !c->isUnconcious() && !c->isDown();
    }

    // Ближайший к точке живой член фракции в загруженном взводе.
    Character* NearestMember(Faction* f, const Ogre::Vector3& to)
    {
        Character* best = NULL;
        float bestDistance = 0.0f;
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
                if (!Able(c) || c->isInCombatMode(true, true))
                    continue;
                const float d = c->getPosition().distance(to);
                if (best == NULL || d < bestDistance)
                {
                    best = c;
                    bestDistance = d;
                }
            }
        }
        return best;
    }

    void ClearOrder(Character* envoy)
    {
        if (envoy == NULL)
            return;
        if (OrdersReceiver* const orders = envoy->getOrdersReciever())
            orders->clearOrders();
    }

    void Order(DfHireTask& t, Character* envoy, Character* recruit, DWORD now)
    {
        OrdersReceiver* const orders = envoy->getOrdersReciever();
        if (orders != NULL)
        {
            if (g_hireDebug)
            {
                char line[160];
                sprintf_s(line, sizeof(line), "DynamicFactions: hire - envoy %.1f m from the recruit, goal 0x%X",
                          envoy->getPosition().distance(recruit->getPosition()),
                          static_cast<int>(orders->getCurrentGoal().key()));
                DebugLog(line);
            }
            orders->addOrder(FOLLOW_PLAYER_ORDER, recruit->getHandle(), recruit->getPosition(), true, false);
        }
        else if (g_hireDebug)
            DebugLog("DynamicFactions: hire - the envoy takes no orders");
        t.phaseMs = now;
    }

    std::string Line(const DfHireTask& t, const std::string& factionName)
    {
        char buf[256];
        switch (t.line)
        {
        case 0:
            sprintf_s(buf, sizeof(buf), Tr("Looking for work? %s could use another pair of hands."), factionName.c_str());
            return buf;
        case 1:
            return Tr("Depends. What does it pay?");
        case 2:
            if (t.price <= 0)
                return Tr("Food, a roof and a share of the loot.");
            sprintf_s(buf, sizeof(buf), Tr("%d cats now, food and a roof later."), t.price);
            return buf;
        default:
            return Tr("Deal. Lead the way.");
        }
    }

    bool Start(DfHireTask& t, Faction* f, TownBase* town, int maxPrice)
    {
        Character* recruit = NULL;
        int price = -1;
        if (!DfFindBarRecruit(town, f, recruit, price) || price > maxPrice)
            return false;
        Character* const envoy = NearestMember(f, recruit->getPosition());
        if (envoy == NULL)
            return false;
        const DWORD now = GetTickCount();
        t.phase = DF_HIRE_GO;
        t.envoy = envoy->getHandle();
        t.recruit = recruit->getHandle();
        t.price = price;
        t.startedMs = now;
        t.line = 0;
        t.closest = envoy->getPosition().distance(recruit->getPosition());
        t.closerMs = now;
        t.why.clear();
        Order(t, envoy, recruit, now);
        return true;
    }

    // Рекрут во взвод посланника; деньги - ещё раз проверка.
    bool Join(DfHireTask& t, Faction* f, Character* envoy, Character* recruit)
    {
        Ownerships* const o = f->factionOwnerships;
        ActivePlatoon* const ap = envoy->getPlatoon();
        if (o == NULL || ap == NULL || o->money < t.price)
        {
            t.why = "not enough money any more";
            return false;
        }
        recruit->setFaction(f, ap);
        DfLockRecruitLines(recruit);
        o->setMoney(o->money - t.price);
        if (recruit->getPlatoon() != ap)
            ErrorLog("DynamicFactions: a recruit changed faction but is not in the envoy's squad");
        return true;
    }

    DfHireResult Fail(DfHireTask& t, Character* envoy, const char* why)
    {
        ClearOrder(envoy);
        t.phase = DF_HIRE_IDLE;
        if (t.why.empty())
            t.why = why;
        return DF_HIRE_FAILED;
    }

    DfHireResult Step(DfHireTask& t, Faction* f, const std::string& factionName)
    {
        Character* const envoy = t.envoy.getCharacter();
        Character* const recruit = t.recruit.getCharacter();
        const DWORD now = GetTickCount();
        if (!Able(envoy) || envoy->getFaction() != f)
            return Fail(t, envoy, "the envoy is down or gone");
        if (!Able(recruit))
            return Fail(t, envoy, "the recruit is down or gone");
        if (recruit->getFaction() == f || recruit->isPlayerCharacter())
            return Fail(t, envoy, "the recruit was hired by someone else");
        if (envoy->isInCombatMode(true, true))
            return Fail(t, envoy, "the envoy is fighting");

        const float d = envoy->getPosition().distance(recruit->getPosition());
        if (t.phase == DF_HIRE_GO)
        {
            if (d < t.closest - 0.5f)
            {
                t.closest = d;
                t.closerMs = now;
            }
            const bool stuckNear = d <= kNearDistance && now - t.closerMs >= kStuckMs;
            if (d <= kTalkDistance || stuckNear)
            {
                ClearOrder(envoy);
                t.phase = DF_HIRE_TALK;
                t.line = 0;
                t.phaseMs = now - kLineMs;          // первая реплика - сразу
                return DF_HIRE_RUNNING;
            }
            if (now - t.startedMs >= kGiveUpMs)
                return Fail(t, envoy, "the envoy did not reach the recruit in 4 minutes");
            if (now - t.phaseMs >= kOrderEveryMs)
                Order(t, envoy, recruit, now);
            return DF_HIRE_RUNNING;
        }

        // TALK
        if (d > kLostDistance)
        {
            t.phase = DF_HIRE_GO;
            t.closest = d;
            t.closerMs = now;
            Order(t, envoy, recruit, now);
            return DF_HIRE_RUNNING;
        }
        if (now - t.phaseMs < kLineMs)
            return DF_HIRE_RUNNING;
        if (t.line >= kLines)
        {
            t.phase = DF_HIRE_IDLE;
            return Join(t, f, envoy, recruit) ? DF_HIRE_DONE : Fail(t, envoy, "joining failed");
        }
        Character* const speaker = (t.line % 2 == 0) ? envoy : recruit;
        speaker->say(Line(t, factionName));
        ++t.line;
        t.phaseMs = now;
        return DF_HIRE_RUNNING;
    }
}

bool DfHireStart(DfHireTask& t, Faction* f, TownBase* town, int maxPrice)
{
    __try
    {
        return f != NULL && town != NULL && !t.busy() && Start(t, f, town, maxPrice);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        TellOnce(told, "DynamicFactions: starting a hire failed once");
        t.phase = DF_HIRE_IDLE;
        return false;
    }
}

DfHireResult DfHireStep(DfHireTask& t, Faction* f, const std::string& factionName)
{
    if (!t.busy())
        return DF_HIRE_FAILED;
    __try
    {
        return Step(t, f, factionName);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        static volatile LONG told = 0;
        TellOnce(told, "DynamicFactions: a hire step failed once - the hire is dropped");
        t.phase = DF_HIRE_IDLE;
        return DF_HIRE_FAILED;
    }
}

void DfHireSetDebug(bool on)
{
    g_hireDebug = on;
}

std::string DfHireStatus(const DfHireTask& t)
{
    char buf[128];
    switch (t.phase)
    {
    case DF_HIRE_GO:
        sprintf_s(buf, sizeof(buf), Tr("going to a recruit in the bar (%d)"), t.price);
        return buf;
    case DF_HIRE_TALK:
        sprintf_s(buf, sizeof(buf), Tr("talking to a recruit (%d)"), t.price);
        return buf;
    default:
        return std::string();
    }
}

void DfHireCancel(DfHireTask& t)
{
    if (!t.busy())
        return;
    __try
    {
        ClearOrder(t.envoy.getCharacter());
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
    t.phase = DF_HIRE_IDLE;
}
