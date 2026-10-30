#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "ui/DfWorkInfo.h"
#include <Windows.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Character.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Building/UseableStuff.h>

namespace
{
    typedef void (*GuiDataFn)(Character*,DatapanelGUI*,int);
    GuiDataFn g_original = NULL;
    bool (*g_managed)(Character*) = NULL;

    bool SameHand(const hand& a,const hand& b)
    {
        return a.type==b.type && a.container==b.container && a.containerSerial==b.containerSerial &&
            a.index==b.index && a.serial==b.serial;
    }

    bool FindSkill(Character* c,StatsEnumerated& stat)
    {
        if (c==NULL || g_managed==NULL || !g_managed(c) || c->currentSkillUsing!=STAT_NONE) return false;
        OrdersReceiver* const orders=c->getOrdersReciever();
        if (orders==NULL || orders->getCurrentGoal().key()!=OPERATE_MACHINERY) return false;
        Building* const b=orders->getCurrentGoal().subject.getBuilding();
        UseableStuff* const use=b!=NULL?b->getUseableStuff():NULL;
        if (use==NULL || use->usesStat==STAT_NONE || use->currentOperators.size()>256) return false;
        const hand h=c->getHandle();
        typedef std::set<hand,std::less<hand>,Ogre::STLAllocator<hand,Ogre::GeneralAllocPolicy> > Operators;
        for (Operators::const_iterator it=use->currentOperators.begin();it!=use->currentOperators.end();++it)
            if (SameHand(h,*it)) {stat=use->usesStat;return true;}
        return false;
    }

    bool SafeSkill(Character* c,StatsEnumerated& stat)
    {
        __try {return FindSkill(c,stat);}
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            static volatile LONG told=0;
            if(InterlockedExchange(&told,1)==0) ErrorLog("DynamicFactions: work-info lookup fault");
            return false;
        }
    }

    void GuiData_hook(Character* c,DatapanelGUI* panel,int category)
    {
        StatsEnumerated shown=STAT_NONE;
        const bool supplied=SafeSkill(c,shown);
        __try
        {
            if (supplied) c->currentSkillUsing=shown;
            g_original(c,panel,category);
        }
        __finally
        {
            // Native GUI consumes this transient field. Restore its original
            // empty value outside rendering; never change AI, XP or work rate.
            if (supplied && (c->currentSkillUsing==shown || c->currentSkillUsing==STAT_NONE))
                c->currentSkillUsing=STAT_NONE;
        }
    }
}

bool DfInstallWorkInfo(bool (*managed)(Character*))
{
    g_managed=managed;
    return KenshiLib::SUCCESS==KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_getGUIData),
        GuiData_hook,&g_original);
}
