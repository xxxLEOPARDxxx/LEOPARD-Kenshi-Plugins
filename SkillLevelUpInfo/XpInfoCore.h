// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cmath>
#include <float.h>
namespace XpInfoCore {
typedef unsigned long Tick;
struct Skill {
 float value, pending; Tick first, last; bool live, active; int levels, emittedLevels;
 Skill():value(0),pending(0),first(0),last(0),live(false),active(false),levels(0),emittedLevels(0){}
 void Observe(float n, Tick now) {
  if(!_finite(n)||n < -1000 || n > 100000) return;
  if(live && n>value) {
   if(pending==0) first=now;
   pending+=n-value; levels+=(int)std::floor(n)-(int)std::floor(value);
   last=now;active=true;
  }
  value=n;live=true;
 }
 int Take(Tick now,float minimum,Tick interval) {
  if(pending<=0) return 0;
  int event=levels>0?2:((pending*100>=minimum && Tick(now-first)>=interval)?1:0);
  if(event){emittedLevels=levels;pending=0;levels=0;return event;}
  if(Tick(now-last)>=120000){pending=0;levels=0;}
  return 0;
 }
 bool Recent(Tick now,Tick lifetime) const { return active && Tick(now-last)<=lifetime; }
};
struct Slots {
 int ids[5],count;
 Slots():count(0){for(int i=0;i<5;++i)ids[i]=-1;}
 void Select(const Skill* skills,int size,Tick now,Tick lifetime,int capacity) {
  if(capacity<0)capacity=0;if(capacity>5)capacity=5;
  int kept=0;
  for(int i=0;i<count && kept<capacity;++i)
   if(ids[i]>=0 && ids[i]<size && skills[ids[i]].Recent(now,lifetime))ids[kept++]=ids[i];
  count=kept;
  // Only 33 skills and five slots: bounded selection avoids temporary vectors.
  bool considered[64]={false};
  if(size>64)size=64;
  for(int pass=0;pass<size;++pass){
   int newest=-1;
   for(int j=0;j<size;++j)
    if(!considered[j] && skills[j].Recent(now,lifetime) &&
       (newest<0 || Tick(now-skills[j].last)<Tick(now-skills[newest].last)))newest=j;
   if(newest<0)break;considered[newest]=true;
   bool present=false;for(int i=0;i<count;++i)if(ids[i]==newest)present=true;
   if(present)continue;
   if(count<capacity){ids[count++]=newest;continue;}
   int oldest=-1;
   for(int i=0;i<count;++i)
    if(Tick(now-skills[ids[i]].last)>5000 &&
       Tick(now-skills[newest].last)<Tick(now-skills[ids[i]].last) &&
       (oldest<0 || Tick(now-skills[ids[i]].last)>Tick(now-skills[ids[oldest]].last)))oldest=i;
   if(oldest>=0)ids[oldest]=newest;
  }
 }
};
}
