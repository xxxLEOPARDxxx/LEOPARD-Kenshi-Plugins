#pragma once
#include <cstring>
namespace GsmLocalRevision {
// Fixed copied storage, compared only after the native one-second snapshot.
// Exact bytes, not a lossy hash; no allocation or native pointer retention.
template<class Value,unsigned Limit>class State {
    Value previous_[Limit];float origin_[3];unsigned count_;long generation_;bool valid_;
public:
    State():count_(0),generation_(0),valid_(false){std::memset(origin_,0,sizeof(origin_));}
    void Reset(){valid_=false;count_=0;generation_=0;}
    template<class Snapshot>bool Observe(const Snapshot&s,long generation){
        if(!s.Valid(generation)||s.Count()>Limit){bool changed=valid_;Reset();return changed;}
        bool changed=!valid_||count_!=s.Count()||generation_!=generation||std::memcmp(origin_,s.Info().SourceSceneOrigin,12)!=0;
        for(unsigned i=0;i<s.Count();++i){const Value*v=s.Light(i);if(!v){Reset();return true;}if(!changed&&std::memcmp(previous_+i,v,sizeof(Value)))changed=true;previous_[i]=*v;}
        count_=s.Count();generation_=generation;std::memcpy(origin_,s.Info().SourceSceneOrigin,12);valid_=true;return changed;
    }
};
}
