#pragma once
#include <windows.h>
namespace GsmViewer {
// Only short local cadence intervals, no wall-clock/simulation-time assumption.
// Reset at visibility/world boundaries; subtraction handles DWORD rollover.
class Cadence {
    DWORD last_;bool initialized_;
public:
    Cadence():last_(0),initialized_(false){}
    void Reset(){last_=0;initialized_=false;}
    bool Due(DWORD now,DWORD interval){if(initialized_&&static_cast<DWORD>(now-last_)<interval)return false;last_=now;initialized_=true;return true;}
};
typedef Cadence TickClock;
typedef Cadence LightClock;
}
