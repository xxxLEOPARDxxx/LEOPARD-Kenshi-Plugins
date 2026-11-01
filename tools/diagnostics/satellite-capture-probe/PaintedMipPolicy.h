#ifndef SATELLITE_CAPTURE_PAINTED_MIP_POLICY_H
#define SATELLITE_CAPTURE_PAINTED_MIP_POLICY_H
#include <stddef.h>
namespace PaintedMipPolicy
{
struct Plan {unsigned additionalMips,levels,dxgiFormat;size_t gpuBytes;};
// Preserve the existing texture-array aggregate cap. Native manager getSize
// reports base levels only, so it must not stand in for this explicit GPU sum.
static const size_t GpuLimit=128u*1024u*1024u;
static const size_t FullChainGpuBytes=111848096;
inline bool Select(unsigned kind,size_t width,size_t height,unsigned format,Plan& out)
{
    const unsigned side=kind==0?1024:2048;
    if(kind>1||width!=side||height!=side||(format!=12&&format!=13))return false;
    Plan result={kind==0?10u:11u,kind==0?11u:12u,format==12?87u:28u,
        kind==0?size_t(22369616):size_t(89478480)};
    out=result;return true;
}
inline bool Budget(const Plan (&plans)[2],size_t& total)
{
    const size_t first=plans[0].gpuBytes,second=plans[1].gpuBytes;
    if(first>GpuLimit||second>GpuLimit-first)return false;
    const size_t sum=first+second;
    if(sum!=FullChainGpuBytes||first!=22369616||second!=89478480||
        plans[0].additionalMips!=10||plans[0].levels!=11||
        plans[1].additionalMips!=11||plans[1].levels!=12)return false;
    for(unsigned kind=0;kind<2;++kind)
        if(plans[kind].dxgiFormat!=28&&plans[kind].dxgiFormat!=87)return false;
    total=sum;return true;
}
// Preflight is read-only and must prove owned identity, actual creator and
// unloaded state. All potentially submitted GPU/native operations happen with
// uncertainty latched. Notify may credit before it throws; never retry or
// destruct that partially credited resource. Native setToLoaded adds no credit.
template<class Operations> bool CompleteUpload(Operations& ops,bool& uncertain)
{
    if(uncertain||!ops.Preflight())return false;
    uncertain=true;
    if(!ops.Upload())return false;
    ops.Notify();
    ops.SetLoaded();
    if(!ops.Loaded())return false;
    uncertain=false;return true;
}
}
#endif
