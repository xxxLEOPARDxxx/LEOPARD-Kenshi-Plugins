#ifndef SATELLITE_MIP_COMPARISON_POLICY_H
#define SATELLITE_MIP_COMPARISON_POLICY_H
#include <stddef.h>
namespace ProbeMipComparison
{
static const size_t TexelLimit=128u*1024u*1024u;
struct Budget {size_t arrayBytes,nativeBytes,stagingBytes,totalBytes;unsigned levels;};
inline bool Plan(unsigned side,unsigned levels,size_t retainedTexelBytes,Budget& out)
{
    if(!side||side>2048||(side&(side-1)))return false;
    unsigned count=1;for(unsigned n=side;n>1;n>>=1)++count;
    if(levels!=count)return false;
    size_t nativeBytes=0;
    for(unsigned n=side;n;n>>=1)nativeBytes+=size_t(n)*n*4;
    Budget b={nativeBytes*4,nativeBytes,size_t(side)*side*4,0,count};
    const size_t local=b.arrayBytes+b.nativeBytes+b.stagingBytes;
    if(local>TexelLimit||retainedTexelBytes>TexelLimit-local)return false;
    b.totalBytes=local+retainedTexelBytes;out=b;return true;
}
struct Schedule
{
    unsigned kind,layer,comparisons;bool active,complete;
    Schedule():kind(0),layer(0),comparisons(0),active(false),complete(false){}
    bool Begin(bool explicitOptIn,bool captureBusy,bool lifecycleSafe)
    {
        if(active||!explicitOptIn||captureBusy||!lifecycleSafe)return false;
        kind=layer=comparisons=0;active=true;complete=false;return true;
    }
    bool PairComplete(unsigned mipCount)
    {
        if(!active||mipCount!=(kind?12u:11u))return false;
        comparisons+=mipCount;
        if(++layer==4){layer=0;if(++kind==2){active=false;complete=true;}}
        return true;
    }
    void Cancel(){active=false;complete=false;}
};
}
#endif
