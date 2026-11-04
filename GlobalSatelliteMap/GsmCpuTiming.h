#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include <sstream>
#include <iomanip>
namespace GsmCpuTiming {
typedef unsigned long long U64;
enum Stage {Lighting,Water,LocalSnapshot,ModelSnapshot,FineSourceTick,FineModelPrepare,FineLighting,FinePresent,FineCoverage,TownCatalog,TownSelection,LoadedCapture,DistantCapture,StageCount};
struct QpcClock {
 static bool Frequency(U64&out){LARGE_INTEGER value;if(!QueryPerformanceFrequency(&value)||value.QuadPart<=0)return false;out=U64(value.QuadPart);return true;}
 static bool Counter(U64&out){LARGE_INTEGER value;if(!QueryPerformanceCounter(&value)||value.QuadPart<0)return false;out=U64(value.QuadPart);return true;}
};
// One serialized Main/session owner. Fixed storage; no per-call allocation,
// synchronization, logging, driver interaction or wall-clock scheduling.
template<class Clock>class BasicStats {
 struct Sample {U64 calls,total,max;};Sample sample_[StageCount];U64 frequency_;bool attempted_;
 BasicStats(const BasicStats&);BasicStats&operator=(const BasicStats&);
 static bool Valid(Stage stage){return unsigned(stage)<unsigned(StageCount);}
public:
 BasicStats():frequency_(0),attempted_(false){Reset();}
 void Reset(){std::memset(sample_,0,sizeof(sample_));}
 void ResetSession(){Reset();frequency_=0;attempted_=false;}
 U64 Frequency()const{return frequency_;}
 U64 Calls(Stage stage)const{return Valid(stage)?sample_[stage].calls:0;}
 U64 TotalTicks(Stage stage)const{return Valid(stage)?sample_[stage].total:0;}
 U64 MaxTicks(Stage stage)const{return Valid(stage)?sample_[stage].max:0;}
 bool Any()const{for(unsigned i=0;i<StageCount;++i)if(sample_[i].calls)return true;return false;}
 bool Begin(Stage stage,U64&started){
  if(!Valid(stage))return false;
  if(!attempted_){attempted_=true;if(!Clock::Frequency(frequency_)||!frequency_)frequency_=0;}
  return frequency_&&Clock::Counter(started);
 }
 void End(Stage stage,U64 started){
  U64 ended=0;if(!Valid(stage)||!frequency_||!Clock::Counter(ended)||ended<started)return;
  const U64 elapsed=ended-started,limit=~U64(0);Sample&s=sample_[stage];
  if(s.calls<limit)++s.calls;s.total=elapsed>limit-s.total?limit:s.total+elapsed;if(elapsed>s.max)s.max=elapsed;
 }
};
template<class StatsType>class BasicScope {
 StatsType*stats_;Stage stage_;U64 started_;BasicScope(const BasicScope&);BasicScope&operator=(const BasicScope&);
public:
 BasicScope(bool enabled,StatsType&stats,Stage stage):stats_(NULL),stage_(stage),started_(0){if(enabled&&stats.Begin(stage,started_))stats_=&stats;}
 ~BasicScope(){if(stats_)stats_->End(stage_,started_);}
};
typedef BasicStats<QpcClock> Stats;
typedef BasicScope<Stats> Scope;
inline const char*StageName(Stage stage){static const char*const names[]={"Lighting","Water","LocalSnapshot","ModelSnapshot","FineSourceTick","FineModelPrepare","FineLighting","FinePresent","FineCoverage","TownCatalog","TownSelection","LoadedCapture","DistantCapture"};return unsigned(stage)<unsigned(StageCount)?names[stage]:"Unknown";}
// Formatting allocates only when explicitly called for the aggregated report.
// Caller logs once per reporting interval and then calls Reset(). Nested stage
// timings are inclusive and must not be added as if they were exclusive time.
template<class Clock>std::string FormatReport(const BasicStats<Clock>&stats){
 std::ostringstream out;out<<"CPU stages ms (inclusive):"<<std::fixed<<std::setprecision(3);
 const U64 frequency=stats.Frequency();bool any=false;
 for(unsigned n=0;n<StageCount;++n){const Stage stage=Stage(n);const U64 calls=stats.Calls(stage);if(!calls||!frequency)continue;any=true;
  const double total=double(stats.TotalTicks(stage))*1000.0/double(frequency),maximum=double(stats.MaxTicks(stage))*1000.0/double(frequency);
  out<<' '<<StageName(stage)<<" calls="<<calls<<" total="<<total<<" avg="<<total/double(calls)<<" max="<<maximum<<';';
 }if(!any)out<<" none";return out.str();
}
}
