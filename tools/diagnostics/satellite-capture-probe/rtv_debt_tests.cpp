#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "OwnedRtvDebt.h"
static int checks=0;
static void Check(bool v,const char* what){++checks;if(!v){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
struct Fake
{
    unsigned textures[6],buffers[6],released[6],textureDrops[6],bufferDrops[6];
    const ProbeOwnership::RtvDebt* seen[6];
    bool sameView,failRelease,throwRelease;int extraBufferAfterTexture;
    Fake():sameView(true),failRelease(false),throwRelease(false),extraBufferAfterTexture(-1)
    {for(unsigned i=0;i<6;++i){textures[i]=1;buffers[i]=2;released[i]=textureDrops[i]=bufferDrops[i]=0;seen[i]=NULL;}}
    bool Exclusive(unsigned i,const ProbeOwnership::RtvDebt& e){seen[i]=&e;return sameView&&textures[i]==1&&buffers[i]==2;}
    bool DropTexture(unsigned i){++textureDrops[i];textures[i]=0;--buffers[i];if(int(i)==extraBufferAfterTexture)++buffers[i];return true;}
    unsigned BufferCount(unsigned i){return buffers[i];}
    bool DropBuffer(unsigned i){++bufferDrops[i];buffers[i]=0;return true;}
    bool ReleaseView(unsigned i,void*){Check(seen[i]&&seen[i]->spent,"debt spent before Release callback");Check(buffers[i]==0&&textures[i]==0,"view release strictly after both owners");++released[i];if(throwRelease)throw 1;return !failRelease;}
};
static void Fill(ProbeOwnership::OwnedRtvDebt& set,unsigned count)
{
    Check(set.Begin(),"new debt set begins");
    for(unsigned i=0;i<count;++i)Check(set.Register(i,(void*)(100+4*i),(void*)(101+4*i),(void*)(102+4*i),(void*)(103+4*i)),"register unique owned identities");
}
int main()
{
    using namespace ProbeOwnership;
    {OwnedRtvDebt s;Fake f;Fill(s,6);Check(s.Clean(f,true),"all six creation debts discharged");for(unsigned i=0;i<6;++i)Check(f.released[i]==1,"one release per factory reference");Check(s.Clean(f,true),"retry is harmless");Check(f.released[0]==1,"no retry double release");Check(s.Begin(),"next request allowed");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);f.textures[0]=2;Check(!s.Clean(f,true),"external texture owner quarantines");Check(!f.textureDrops[0]&&!f.released[0],"not freed with external owner");Check(!s.Begin(),"quarantine blocks new captures");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);f.buffers[0]=3;Check(!s.Clean(f,true),"external pixel buffer owner quarantines");Check(!f.textureDrops[0],"preflight before destruction");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);Check(!s.Clean(f,false),"unknown workspace lifetime quarantines");Check(!f.released[0],"unknown workspace gets no Release");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);f.extraBufferAfterTexture=0;Check(!s.Clean(f,true),"post-texture buffer identity proof");Check(!f.bufferDrops[0]&&!f.released[0],"retain buffer and debt on failed proof");}
    {OwnedRtvDebt s;Fake f;Fill(s,2);f.failRelease=true;Check(!s.Clean(f,true),"failed Release quarantines");Check(!s.Clean(f,true)&&f.released[0]==1&&!f.released[1],"spent before throwing/failing callback");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);Check(!s.Register(1,(void*)200,(void*)201,(void*)202,(void*)103),"different RTT cannot alias same view");Check(!s.Clean(f,true),"alias rejects cleanup");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);s.UncertainAllocation();Check(!s.Clean(f,true)&&!s.Begin(),"native partial construction is bounded quarantine");}
    {OwnedRtvDebt s;Fake f;Check(s.Begin(),"empty attempt begins");Check(s.Clean(f,true),"failure before allocation has no debts");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);f.sameView=false;Check(!s.Clean(f,true)&&!f.released[0],"rebound native view rejected");}
    {OwnedRtvDebt s;Fake f;Fill(s,6);f.textures[5]=2;Check(!s.Clean(f,true)&&!f.textureDrops[0],"preflight last entry before first destruction");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);f.throwRelease=true;Check(!s.Clean(f,true)&&!s.Clean(f,true)&&f.released[0]==1,"throwing Release never retried");}
    {OwnedRtvDebt s;Fake f;Fill(s,1);s.UncertainAllocation();s.AllocationConfirmed();Check(s.Clean(f,true),"confirmed successful native allocation can clean");}
    {OwnedRtvDebt s;Fake f;Fill(s,6);Check(!s.Register(6,(void*)1,(void*)2,(void*)3,(void*)4),"seventh surface refused");Check(!s.Begin(),"no unbounded registry growth");}
    std::printf("%d owned RTV debt checks passed\n",checks);
}
