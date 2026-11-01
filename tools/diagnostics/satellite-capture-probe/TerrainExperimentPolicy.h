#pragma once
namespace TerrainExperimentPolicy
{
struct Region {double x,z,span,altitude;};
struct RecycleState
{
    bool enabled,pending;
    RecycleState():enabled(false),pending(false){}
    void Start(bool requested,bool batch){enabled=requested&&batch;pending=false;}
    bool Next(){pending=enabled;return pending;}
    void Ready(){pending=false;}
    // Scheduling state only. Resource ownership remains in TerrainExperiment.
    void Abort(){enabled=false;pending=false;}
};
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
        if(s.memory>1024ULL*1024*1024||s.patches>131072||s.ogre>65536)return Exceeded;
        if(frames>=600||static_cast<unsigned long>(now-start)>=60000UL)return Expired;
        if(s.triangles>16000000)
        {
            // Native getDebugInfo estimates visible*FIRST geometry indexCount-2.
            // The worker publishes vertices before indexCount. Exact0-2 can thus
            // appear before index publication, even with unbuilt==0. Native Info
            // counts deferred GPU drawables, not CPU-worker queue completion.
            // Do not treat it as ready or clamp
            // the reported value. Patch64's maximum indexCount is8572; retain a
            // conservative16M bound while waiting for real telemetry to recover.
            if(s.triangles!=~0ULL-1||!s.visible||s.visible>16000000ULL/8572)return Exceeded;
            stable=0;previous=s;return Waiting;
        }
        const bool same=s.patches==previous.patches&&s.visible==previous.visible&&
                        s.triangles==previous.triangles&&s.ogre==previous.ogre;
        if(!s.unbuilt&&s.visible&&s.triangles)stable=same?stable+1:1;
        else stable=0;
        previous=s;return stable>=3?Ready:Waiting;
    }
};
inline bool CanDestroy(const void *owner,const void *current){return owner&&owner==current;}
}
