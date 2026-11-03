#pragma once
#include "atlas-terrain/fine-viewer/FineJob.h"
namespace GsmFinePrefetch {
// At most two same-cell pairs beyond the leading edge, under the current
// exact demand/base identity. This authorises cache production, never display.
inline FineJob::Plan Make(const FineJob::Plan&visible,const FineJob::Demand&now,const FineJob::Budget&budget,double dx,double dz,unsigned ordinal){
 FineJob::Plan out;if(ordinal>=2||!FineJob::Internal::Fresh(visible,now)||!FineJob::Internal::ProducerFits(budget)||!_finite(dx)||!_finite(dz)||(std::abs(dx)+std::abs(dz)<.01))return out;
 const unsigned spacing=visible.spacing;const double width=256.0*spacing;const FineWindows::Rect&w=now.view.world;
 const bool horizontal=std::abs(dx)>=std::abs(dz);const double movement=horizontal?dx:dz;const bool positive=movement>0;
 const double edge=horizontal?(positive?w.x+w.w:w.x):(positive?w.z+w.h:w.z);
 const double middle=horizontal?w.z+w.h*.5:w.x+w.w*.5;
 int lead=positive?int(std::ceil((edge+147456)/width))+int(ordinal):int(std::floor((edge+147456)/width))-1-int(ordinal);
 int cross=int(std::floor((middle+147456)/width));const int side=int(FineWindows::GridSide(spacing));
 if(lead<0||lead>=side||cross<0||cross>=side)return out;
 FineWindows::Key first={spacing,unsigned(horizontal?lead:cross),unsigned(horizontal?cross:lead),now.view.stamp};FineJob::Placement a;
 if(!FineJob::Map(first,a))return out;FineWindows::Key second=first;bool mate=false;
 for(int sign=1;sign>=-1;sign-=2){int at=cross+sign;if(at<0||at>=side)continue;FineWindows::Key key=first;if(horizontal)key.z=unsigned(at);else key.x=unsigned(at);if(FineJob::Internal::SameCell(first,key)){second=key;mate=true;break;}}
 if(!mate)return out;
 out.demand=now;out.spacing=out.densitySpacing=spacing;out.count=2;out.worstVisible=2;out.keys[0]=first;out.keys[1]=second;
 if(!FineJob::Internal::Group(out)||out.pairCount!=1)return FineJob::Plan();out.producerPeak=budget.viewerGpu+budget.fineGpu+WorldCohort::GpuReservation(2);out.publicationPeak=budget.viewerGpu+budget.fineGpu;out.cpuPeak=budget.producerCpu+budget.otherCpu;out.state=FineJob::Detail;return out;
}
inline bool Eligible(DWORD now,DWORD changed,bool visibleProduced,bool center,unsigned ordinal,bool ownerReady,const FineJob::Plan&plan,const FineJob::Demand&demand,const FineJob::Budget&budget,double dx,double dz){
 return visibleProduced&&!center&&ordinal<2&&ownerReady&&DWORD(now-changed)>=600&&Make(plan,demand,budget,dx,dz,ordinal).state==FineJob::Detail;
}
}
