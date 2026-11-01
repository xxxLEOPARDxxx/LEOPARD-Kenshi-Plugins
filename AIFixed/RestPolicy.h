#pragma once
#include <cmath>
#include <algorithm>

// Adapted medical/scheduling ideas from Basusu's SleepFix (GPLv3), with a
// single owner shared with AI Fixed's LEOPARD repair scheduler. See UPSTREAM.md.
namespace AIFixedRest
{
    enum Care { None=0, Repair=1, Sleep=2 };
    inline bool Finite(float value){return _finite(value)!=0;}
    // The native getter is 1 when nutrition has no movement penalty. Use its
    // policy rather than guessing a raw hunger scale or checking only hunger KO.
    inline bool NutritionAllows(bool robot,float hungerSpeed)
    {return robot||(Finite(hungerSpeed)&&hungerSpeed>=.999f);}
    struct SimulationClock
    {
        double previous,milliseconds;bool anchored,wasPaused;
        SimulationClock(){Reset();}
        void Reset(){previous=0;milliseconds=1;anchored=false;wasPaused=false;}
        bool Sample(double seconds,bool paused,unsigned& tick)
        {
            if(!::_finite(seconds)||seconds<0){anchored=false;return false;}
            // Native game hours freeze on normal pause. Explicitly discard
            // paused intervals and the first resume gap as well, so a different
            // clock supplier cannot charge paused time to our deadlines.
            if(anchored&&!paused&&!wasPaused&&seconds>=previous)
            {
                const double next=milliseconds+(seconds-previous)*1000.0;
                if(!::_finite(next)){anchored=false;return false;}
                milliseconds=next;
            }
            previous=seconds;anchored=true;wasPaused=paused;
            tick=static_cast<unsigned>(std::fmod(milliseconds,4294967296.0));
            return !paused;
        }
    };
    struct Health
    {
        bool valid,untreated;unsigned fleshParts,robotParts;
        float fleshRatio,robotRatio,wear,robotMissing;
        Health():valid(true),untreated(false),fleshParts(0),robotParts(0),fleshRatio(1),robotRatio(1),wear(0),robotMissing(0){}
        void Part(bool robotic,bool missing,bool selfHealing,float ratio,float wearDamage,
                  float flesh,float bandaging,float maximum,float fleshStun=0)
        {
            if(missing)return;
            if(!Finite(ratio)||!Finite(wearDamage)||!Finite(flesh)||!Finite(fleshStun)||!Finite(bandaging)||!Finite(maximum)||
               maximum<=0||ratio>1.05f||wearDamage<0){valid=false;return;}
            ratio=(std::max)(0.0f,ratio);
            if(robotic){++robotParts;robotRatio=(std::min)(robotRatio,ratio);wear=(std::max)(wear,wearDamage);
                robotMissing=(std::max)(robotMissing,maximum-(flesh-fleshStun));}
            else
            {
                ++fleshParts;fleshRatio=(std::min)(fleshRatio,ratio);
                if(!selfHealing&&flesh+bandaging<maximum-wearDamage-1.0f)untreated=true;
            }
        }
    };
    inline Care Needed(const Health& h,bool sleep,bool repairWear,bool repairHealth,
                       float sleepBelow,float repairBelow,float wearAbove,float fleshHealed=.99f)
    {
        if(!h.valid)return None;
        // Ordinary beds heal flesh faster; robotic parts are a separate visit.
        if(sleep&&h.fleshParts&&h.fleshRatio<sleepBelow)return h.untreated?None:Sleep;
        if(h.robotParts&&((repairWear&&h.wear>wearAbove)||
                          (repairHealth&&h.robotRatio<repairBelow)))
        {
            // Never divert a partly healed biological patient to a repair bed.
            if(h.fleshParts&&h.fleshRatio<fleshHealed)return None;
            return Repair;
        }
        return None;
    }
    inline bool Complete(const Health& h,Care care,float healAbove)
    {
        if(!h.valid)return false;
        if(care==Sleep)return h.fleshParts&&h.fleshRatio>=healAbove;
        if(care==Repair)return h.robotParts&&h.robotMissing<=.5f&&h.wear<=.5f;
        return true;
    }
    struct Bed
    {
        bool properFunction,furniture,useable,free,powered,own,allied,sameTown,sameFloor;
        float distanceSq;
        Bed():properFunction(false),furniture(false),useable(false),free(false),powered(false),own(false),allied(false),sameTown(false),sameFloor(false),distanceSq(0){}
    };
    inline bool LocationAllowed(const Bed& b,bool ownOnly,float radius)
    {
        return radius>0&&Finite(radius)&&Finite(b.distanceSq)&&b.distanceSq>=0&&
            b.distanceSq<=radius*radius&&b.properFunction&&b.furniture&&
            b.sameTown&&b.sameFloor&&(b.own||(!ownOnly&&b.allied));
    }
    inline bool Allowed(const Bed& b,Care care,bool ownOnly,float radius)
    {return LocationAllowed(b,ownOnly,radius)&&b.useable&&b.free&&(care!=Repair||b.powered);}
    inline bool IntentAllows(bool externalOrder,bool ownExactOrder,bool interrupted)
    {return !interrupted&&(!externalOrder||ownExactOrder);}
}
