#define WIN32_LEAN_AND_MEAN
#include "ui/DfTradeReceipt.h"
#include <Windows.h>
#include <cstdio>
#include <Debug.h>
#include <kenshi/Character.h>
#include <kenshi/Globals.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/ScreenLabel.h>

namespace
{
    void ShowReceipt(Character* actor,int paid)
    {
        if (actor==NULL || gui==NULL || paid<=0) return;
        char text[40];sprintf_s(text,sizeof(text),"c.%d",paid);
        // Native AI::buySomething uses this currency style and tracking offset.
        ScreenLabel* const label=gui->createScreenLabel(text,MyGUI::Colour(1.0f,1.0f,0.6f,0.9f),
            ScreenLabel::LS_LARGE,ScreenLabel::RS_SLOW);
        if (label!=NULL) label->setTracking(actor->getHandle(),Ogre::Vector3(0,15,0));
    }
}

void DfShowTradeReceipt(Character* actor,int paid)
{
    __try {ShowReceipt(actor,paid);}
    __except(EXCEPTION_EXECUTE_HANDLER)
    {ErrorLog("DynamicFactions: completed trade receipt could not be displayed");}
}
