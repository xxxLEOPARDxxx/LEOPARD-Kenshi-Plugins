#pragma once
#include "GsmCacheProfile.h"
namespace GsmFineShadeCursor {
// Mixed 1m/3m fallback visibility can contain both complete visible sets.
enum {Capacity=2*GsmCacheProfile::MaxVisible,Invalid=Capacity};
// Main-owned scheduling state only. Visible indices are a per-pass snapshot;
// publication advances the NEXT pass without moving the current traversal.
class State {
 unsigned next_,count_,start_;
public:
 State():next_(0),count_(0),start_(0){}
 unsigned Cursor(unsigned count){count_=count<=Capacity?count:0;start_=count_?next_%count_:0;return start_;}
 unsigned Index(unsigned scanned)const{return scanned<count_?(start_+scanned)%count_:Invalid;}
 bool Published(unsigned index){if(index>=count_)return false;next_=index+1==count_?0:index+1;return true;}
};
}
