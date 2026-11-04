#pragma once
#include "atlas-terrain/fine-viewer/FineJob.h"
namespace GsmFinePrefetch {
// Worker-side RAM warmup around a completed viewport. Each operation remains
// one same-cell pair, below visible demand, and never uploads speculative GPU tiles.
inline unsigned PairLimit(){return (std::min)(64U,unsigned(GsmCacheProfile::RamBudgetBytes()/(1024ULL*1024*1024))*8);}
inline FineJob::Plan Make(const FineJob::Plan&visible,const FineJob::Demand&now,const FineJob::Budget&budget,double dx,double dz,unsigned ordinal){
 FineJob::Plan out;if(ordinal>=PairLimit()||!FineJob::Internal::Fresh(visible,now)||!FineJob::Internal::ProducerFits(budget)||!_finite(dx)||!_finite(dz))return out;
 const unsigned spacing=visible.spacing;const double width=256.0*spacing;const FineWindows::Rect&w=now.view.world;
 const unsigned primary=std::abs(dx)>=std::abs(dz)?(dx<0?2:0):(dz<0?3:1),direction=(primary+ordinal%4)%4;
 const bool horizontal=direction==0||direction==2,positive=direction==0||direction==1;
 const double edge=horizontal?(positive?w.x+w.w:w.x):(positive?w.z+w.h:w.z);
 const double middle=horizontal?w.z+w.h*.5:w.x+w.w*.5;
 const double crossLow=horizontal?w.z:w.x,crossHigh=crossLow+(horizontal?w.h:w.w);
 const int lo=(int(FineWindows::Internal::Index((std::max)(-147456.0,crossLow),spacing))+1)/2,
           hi=(int(FineWindows::Internal::Last((std::min)(147456.0,crossHigh),spacing))-1)/2;
 if(hi<lo)return out;const unsigned rows=unsigned(hi-lo+1),group=ordinal/4,depth=group/rows;
 int centre=int(std::floor((middle+147456)/width))/2;centre=(std::max)(lo,(std::min)(hi,centre));
 int cross=centre;unsigned wanted=group%rows,found=0;
 for(unsigned attempt=0;attempt<2*rows+1;++attempt){const int offset=attempt==0?0:(attempt%2?int((attempt+1)/2):-int(attempt/2));int candidate=centre+offset;if(candidate<lo||candidate>hi)continue;if(found++==wanted){cross=candidate;break;}}
 cross*=2;int lead=positive?int(std::ceil((edge+147456)/width))+int(depth):int(std::floor((edge+147456)/width))-1-int(depth);
 const int side=int(FineWindows::GridSide(spacing));
 if(lead<0||lead>=side||cross<0||cross>=side)return out;
 FineWindows::Key first={spacing,unsigned(horizontal?lead:cross),unsigned(horizontal?cross:lead),now.view.stamp};FineJob::Placement a;
 if(!FineJob::Map(first,a))return out;FineWindows::Key second=first;
 if(horizontal)second.z=unsigned(cross+1);else second.x=unsigned(cross+1);
 if(!FineWindows::Valid(second)||!FineJob::Internal::SameCell(first,second))return out;
 out.demand=now;out.spacing=out.densitySpacing=spacing;out.count=2;out.worstVisible=2;out.keys[0]=first;out.keys[1]=second;
 if(!FineJob::Internal::Group(out)||out.pairCount!=1)return FineJob::Plan();out.producerPeak=budget.viewerGpu+budget.fineGpu+WorldCohort::GpuReservation(2);out.publicationPeak=budget.viewerGpu+budget.fineGpu;out.cpuPeak=budget.producerCpu+budget.otherCpu;out.state=FineJob::Detail;return out;
}
inline bool Eligible(DWORD now,DWORD changed,bool visibleProduced,bool center,unsigned ordinal,bool ownerReady,const FineJob::Plan&plan,const FineJob::Demand&demand,const FineJob::Budget&budget,double dx,double dz){
 return visibleProduced&&!center&&ordinal<PairLimit()&&ownerReady&&DWORD(now-changed)>=600&&Make(plan,demand,budget,dx,dz,ordinal).state==FineJob::Detail;
}
}
