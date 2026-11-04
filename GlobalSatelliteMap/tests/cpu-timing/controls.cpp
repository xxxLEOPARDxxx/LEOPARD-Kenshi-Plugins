#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <new>
// MODE
#ifdef LEGACY
namespace GsmCpuTiming {typedef unsigned long long U64;enum Stage{Lighting,Water,StageCount};template<class Clock>struct BasicStats{U64 Calls(Stage)const{return 0;}};template<class Stats>struct BasicScope{BasicScope(bool,Stats&,Stage){}};}
#else
#include "../../../GsmCpuTiming.h"
#endif
static unsigned checks=0,allocations=0;
void*operator new(size_t n){++allocations;void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}void*operator new[](size_t n){++allocations;void*p=std::malloc(n);if(!p)throw std::bad_alloc();return p;}void operator delete(void*p){std::free(p);}void operator delete[](void*p){std::free(p);}
static void C(bool value,const char*name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);throw 1;}std::printf("PASS %s\n",name);}
struct Clock {
 static unsigned frequencyCalls,counterCalls,index;static GsmCpuTiming::U64 frequency,ticks[8];static bool counterOk[8];
 static void Reset(){frequencyCalls=counterCalls=index=0;frequency=1000;for(unsigned n=0;n<8;++n){ticks[n]=n*10;counterOk[n]=true;}}
 static bool Frequency(GsmCpuTiming::U64&value){++frequencyCalls;value=frequency;return frequency!=0;}
 static bool Counter(GsmCpuTiming::U64&value){++counterCalls;unsigned n=index++;if(n>=8)return false;value=ticks[n];return counterOk[n];}
};
unsigned Clock::frequencyCalls=0,Clock::counterCalls=0,Clock::index=0;GsmCpuTiming::U64 Clock::frequency=0,Clock::ticks[8];bool Clock::counterOk[8];
int main(){try{
 using namespace GsmCpuTiming;typedef BasicStats<Clock> S;typedef BasicScope<S> Scope;Clock::Reset();S s;
 {Scope timer(true,s,Lighting);}C(s.Calls(Lighting)==1,"enabled stage records actual elapsed sample");
#ifndef LEGACY
 C(Clock::frequencyCalls==1&&Clock::counterCalls==2&&s.TotalTicks(Lighting)==10&&s.MaxTicks(Lighting)==10,"frequency once and entry/exit counter pair");
 {Scope timer(true,s,Water);}C(Clock::frequencyCalls==1&&Clock::counterCalls==4,"subsequent stage reuses cached frequency");
 unsigned before=allocations,queries=Clock::counterCalls,freq=Clock::frequencyCalls;{Scope timer(false,s,Lighting);}C(Clock::counterCalls==queries&&Clock::frequencyCalls==freq&&s.Calls(Lighting)==1&&allocations==before,"disabled scope performs no queries allocation or mutation");
 {Scope timer(true,s,StageCount);}C(Clock::counterCalls==queries&&Clock::frequencyCalls==freq,"invalid stage performs no queries");
 s.Reset();C(!s.Any()&&s.Calls(Lighting)==0&&s.TotalTicks(Water)==0,"report interval reset clears fixed aggregates");C(s.Frequency()==1000,"interval reset retains session frequency");
 Clock::Reset();s.ResetSession();before=allocations;{Scope outer(true,s,Lighting);{Scope inner(true,s,Water);}}C(s.TotalTicks(Lighting)==30&&s.TotalTicks(Water)==10&&s.Calls(Lighting)==1&&s.Calls(Water)==1,"nested stages record inclusive independent elapsed time");C(allocations==before,"enabled scope path allocates no heap");
 std::string report=FormatReport(s);C(report.find("Lighting")!=std::string::npos&&report.find("Water")!=std::string::npos&&report.find("30.000")!=std::string::npos,"report formats actual stage milliseconds");C(s.Calls(Lighting)==1,"formatting does not implicitly reset aggregates");
 Clock::Reset();s.ResetSession();Clock::frequency=0;{Scope a(true,s,Lighting);Scope b(true,s,Water);}C(Clock::frequencyCalls==1&&!Clock::counterCalls&&!s.Any(),"missing frequency is cached without counter calls");Clock::frequency=1000;{Scope a(true,s,Lighting);}C(Clock::frequencyCalls==1&&!s.Any(),"failed session frequency is not retried hot");s.ResetSession();{Scope a(true,s,Lighting);}C(Clock::frequencyCalls==2&&s.Calls(Lighting)==1,"new session permits frequency retry");
 Clock::Reset();s.ResetSession();Clock::counterOk[0]=false;{Scope a(true,s,Lighting);}C(!s.Any()&&Clock::counterCalls==1,"entry counter failure records no sample or exit query");Clock::Reset();s.ResetSession();Clock::counterOk[1]=false;{Scope a(true,s,Lighting);}C(!s.Any()&&Clock::counterCalls==2,"exit counter failure records no sample");
 Clock::Reset();s.ResetSession();Clock::ticks[0]=100;Clock::ticks[1]=99;{Scope a(true,s,Lighting);}C(!s.Any(),"counter rollover or backwards sample is rejected");
 Clock::Reset();s.ResetSession();Clock::ticks[0]=0;Clock::ticks[1]=~U64(0);Clock::ticks[2]=0;Clock::ticks[3]=10;{Scope a(true,s,Lighting);}{Scope b(true,s,Lighting);}C(s.Calls(Lighting)==2&&s.TotalTicks(Lighting)==~U64(0)&&s.MaxTicks(Lighting)==~U64(0),"tick aggregate saturates instead of wrapping");
 GsmCpuTiming::Stats real;{GsmCpuTiming::Scope a(true,real,FineCoverage);}C(real.Calls(FineCoverage)==1&&real.Frequency()>0,"production Windows QPC records enabled scope");
 std::printf("PASS %u CPU timing controls\n",checks);
#endif
 return 0;}catch(...){return 1;}}
