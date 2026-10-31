#define WIN32_LEAN_AND_MEAN
#include "game/DfHunger.h"
#include "game/DfGame.h"
#include "core/DfSupply.h"
#include <Windows.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/Faction.h>
#include <kenshi/GameData.h>
#include <kenshi/Platoon.h>
#include <kenshi/RaceData.h>
#include <kenshi/StateBroadcastData.h>
#include <float.h>

namespace
{
    float (*g_originalHunger)(Character*)=NULL;
    float OwnFloor(Character* actor,float original)
    {
        __try
        {
            Faction* const f=actor!=NULL ? actor->getFaction() : NULL;
            // World-owned pointers are never retained. This works during load,
            // before the delayed faction registry/UI metadata becomes ready.
            if(f==NULL || f->data==NULL || f->data->stringID!=kDfFactionRecord) return original;
            StateBroadcastData* const state=actor->getStateBroadcast();
            if(state==NULL || state->_slaveState==IS_SLAVE || actor->inSomething==IN_PRISON) return original;
            ActivePlatoon* const ap=actor->getPlatoon();
            RaceData* const race=actor->getRace();
            if(ap==NULL || ap->me==NULL || ap->me->getFaction()!=f || race==NULL) return original;
            const bool organic=!race->robot && _finite(race->hungerRate) && race->hungerRate>0;
            return DfNaturalHungerFloor(original,ap->me->malnourishedLevel,true,organic,actor->isAnimal()!=NULL);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG warned=0;
            if(InterlockedExchange(&warned,1)==0) ErrorLog("DynamicFactions: natural hunger scope unreadable; native behavior retained");
            return original;
        }
    }
    float HungerHook(Character* actor)
    {
        const float original=g_originalHunger(actor);
        return OwnFloor(actor,original);
    }
}

bool DfInstallNaturalHunger()
{
    // Taking a virtual member's address produces our local vtable thunk, not
    // the SDK import. GetRealAddress explicitly requires the nonvirtual entry.
    return KenshiLib::SUCCESS==KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_getMagicHungerSetting),
        HungerHook,&g_originalHunger);
}
