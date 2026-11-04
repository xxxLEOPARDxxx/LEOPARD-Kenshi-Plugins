#pragma once
#include "atlas-terrain/fine-viewer/FineJob.h"
namespace GsmFineCoverage {
// Scheduling proof only. Caller supplies current map/context identity and a
// serialized Main visitor proving each actual GPU entry valid, shaded, opaque.
// No native pointers, allocations or retained state; never infer coverage from
// the producer's completed status alone.
template<class Ready>bool Covered(const FineJob::Plan&plan,const FineJob::Demand&current,
        bool contextCurrent,bool mapCurrent,bool center,unsigned phase,const Ready&ready){
 if(!contextCurrent||!mapCurrent||center||phase!=4||!FineJob::Internal::Fresh(plan,current))return false;
 const FineWindows::Rect&w=current.view.world;
 const double x0=FineWindows::Internal::High(-147456,w.x),z0=FineWindows::Internal::High(-147456,w.z);
 const double x1=FineWindows::Internal::Low(147456,w.x+w.w),z1=FineWindows::Internal::Low(147456,w.z+w.h);
 if(x1<=x0||z1<=z0)return false;
 const unsigned firstX=FineWindows::Internal::Index(x0,plan.spacing),firstZ=FineWindows::Internal::Index(z0,plan.spacing);
 const unsigned lastX=FineWindows::Internal::Last(x1,plan.spacing),lastZ=FineWindows::Internal::Last(z1,plan.spacing);
 if(lastX<firstX||lastZ<firstZ)return false;
 const unsigned columns=lastX-firstX+1,rows=lastZ-firstZ+1;
 if(columns>FineJob::Slots||rows>FineJob::Slots||columns*rows!=plan.count)return false;
 // Unique exact rectangular coverage, even if caller accidentally supplies a
 // center-only or malformed Plan. O(N), fixed512B scratch, no quadratic lookup.
 bool seen[FineJob::Slots]={};
 for(unsigned n=0;n<plan.count;++n){const FineWindows::Key&key=plan.keys[n];
  if(!FineWindows::Valid(key)||key.spacing!=plan.spacing||!FineWindows::Same(key.stamp,current.view.stamp)||
     key.x<firstX||key.x>lastX||key.z<firstZ||key.z>lastZ)return false;
  const unsigned at=(key.z-firstZ)*columns+key.x-firstX;if(seen[at])return false;seen[at]=true;
  if(!ready(key))return false;
 }return true;
}
}
