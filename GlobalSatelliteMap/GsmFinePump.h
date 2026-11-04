#pragma once
namespace GsmFinePump {
typedef unsigned long long U64;
enum {MaxSteps=8};
// Pure QPC-tick policy. No wait, clock query, native call or allocation here.
// A single already-entered operation is not preempted; its elapsed time is
// checked before another step may begin. An unchanged snapshot ends the batch.
class Budget {
 U64 started_,ticks_;unsigned steps_;bool stopped_;
public:
 Budget(U64 started,U64 frequency):started_(started),ticks_((frequency/2000)*3+(frequency%2000)*3/2000),steps_(0),stopped_(false){}
 bool CanStep(U64 now)const{return !stopped_&&steps_<MaxSteps&&ticks_&&now>=started_&&now-started_<ticks_;}
 bool Finished(U64 now,bool progressed){
  if(stopped_)return false;if(steps_<MaxSteps)++steps_;
  if(!progressed||!CanStep(now))stopped_=true;return !stopped_;
 }
 unsigned Steps()const{return steps_;}
};
}
