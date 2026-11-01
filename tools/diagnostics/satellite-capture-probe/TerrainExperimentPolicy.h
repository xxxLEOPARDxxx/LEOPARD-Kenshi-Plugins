#pragma once
namespace TerrainExperimentPolicy
{
struct Region {double x,z,span,altitude;};
struct GridBatch
{
    bool enabled;unsigned index;unsigned long started;
    GridBatch():enabled(false),index(0),started(0){}
    void Begin(bool requested,unsigned long now){enabled=requested;index=0;started=now;}
    bool Cell(int &x,int &z)const
    {if(!enabled||index>=16)return false;x=int(index%4);z=int(index/4);return true;}
    bool Advance(){if(!enabled||index>=15)return false;++index;return true;}
    bool Expired(unsigned long now)const
    {return enabled&&static_cast<unsigned long>(now-started)>=120000UL;}
};
inline bool PrimaryCell(int x,int z,int altitude,Region &out)
{
    if(x<0||x>3||z<0||z>3||altitude<10000||altitude>30000)return false;
    out.span=73728;out.x=-147456+(x+.5)*out.span;
    out.z=-147456+(z+.5)*out.span;out.altitude=altitude;return true;
}
struct Stats {unsigned __int64 memory,patches,visible,triangles,ogre,unbuilt;};
enum Result {Waiting,Ready,Expired,Exceeded};
struct Warmup
{
    unsigned frames,stable;Stats previous;
    Warmup():frames(0),stable(0){Stats zero={0};previous=zero;}
    Result Observe(unsigned long start,unsigned long now,const Stats &s)
    {
        ++frames;
        if(s.memory>1024ULL*1024*1024||s.patches>131072||s.triangles>16000000||s.ogre>65536)return Exceeded;
        if(frames>=600||static_cast<unsigned long>(now-start)>=60000UL)return Expired;
        const bool same=s.patches==previous.patches&&s.visible==previous.visible&&
                        s.triangles==previous.triangles&&s.ogre==previous.ogre;
        if(!s.unbuilt&&s.visible&&s.triangles)stable=same?stable+1:1;
        else stable=0;
        previous=s;return stable>=3?Ready:Waiting;
    }
};
inline bool CanDestroy(const void *owner,const void *current){return owner&&owner==current;}
}
