#pragma once
#include "AtlasPolicy.h"
#include <cmath>
#include <algorithm>
namespace AtlasView {
const unsigned WindowSide=256, FallbackSlots=16, DetailSlots=56, WindowSlots=FallbackSlots+DetailSlots;
struct WindowKey {
    unsigned tile,level,x,z;
    WindowKey():tile(16),level(6),x(0),z(0){}
    WindowKey(unsigned t,unsigned l,unsigned a,unsigned b):tile(t),level(l),x(a),z(b){}
    bool Valid()const{return tile<16&&level<6&&x<Side(level)/WindowSide&&z<Side(level)/WindowSide;}
    bool operator==(const WindowKey&b)const{return tile==b.tile&&level==b.level&&x==b.x&&z==b.z;}
};
inline unsigned WindowRecord(const WindowKey&k){unsigned n=k.tile*1365;for(unsigned l=0;l<k.level;++l){unsigned s=Side(l)/WindowSide;n+=s*s;}return n+k.z*(Side(k.level)/WindowSide)+k.x;}
struct WindowRect {double x,y,w,h;WindowRect():x(0),y(0),w(0),h(0){}WindowRect(double a,double b,double c,double d):x(a),y(b),w(c),h(d){}};
inline bool WindowVisible(const WindowRect&a,const WindowRect&b){return a.w>0&&a.h>0&&b.w>0&&b.h>0&&a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
inline WindowRect SubWindow(const WindowRect&r,const WindowKey&k){double n=Side(k.level)/WindowSide;return WindowRect(r.x+r.w*k.x/n,r.y+r.h*k.z/n,r.w/n,r.h/n);}
struct WindowPlan {
    WindowKey keys[DetailSlots];unsigned count,visibleCount,level,densityLevel;bool visible[16];
    WindowPlan():count(0),visibleCount(0),level(5),densityLevel(5){std::memset(visible,0,sizeof(visible));}
    bool Contains(const WindowKey&k)const{if(k.level==5)return k.Valid()&&visible[k.tile];for(unsigned i=0;i<count;++i)if(keys[i]==k)return true;return false;}
};
inline unsigned EnumerateWindows(const WindowRect cells[16],const WindowRect&clip,unsigned level,WindowKey*out,unsigned cap){
    unsigned total=0,n=Side(level)/WindowSide;
    for(unsigned t=0;t<16;++t){const WindowRect&r=cells[t];if(!WindowVisible(r,clip))continue;
        int x0=int(std::floor(std::max(0.0,(clip.x-r.x)/r.w)*n));
        int z0=int(std::floor(std::max(0.0,(clip.y-r.y)/r.h)*n));
        int x1=int(std::ceil(std::min(1.0,(clip.x+clip.w-r.x)/r.w)*n));
        int z1=int(std::ceil(std::min(1.0,(clip.y+clip.h-r.y)/r.h)*n));
        x0=std::max(0,std::min(int(n),x0));z0=std::max(0,std::min(int(n),z0));
        x1=std::max(0,std::min(int(n),x1));z1=std::max(0,std::min(int(n),z1));
        for(int z=z0;z<z1;++z)for(int x=x0;x<x1;++x){if(total>=cap)return cap+1;if(out)out[total]=WindowKey(t,level,unsigned(x),unsigned(z));++total;}
    }return total;
}
inline WindowPlan PlanWindows(const WindowRect cells[16],const WindowRect&clip){
    // Native projection rounds primary boundaries to pixels. Both density and
    // admission use the whole projected grid, never the currently visible set.
    // Otherwise panning one pixel can change the mip without changing zoom.
    WindowPlan p;double display=1,minW=1e100,minH=1e100;
    for(unsigned i=0;i<16;++i){
        if(cells[i].w<=0||cells[i].h<=0)continue;
        display=std::max(display,std::max(cells[i].w,cells[i].h));
        minW=std::min(minW,cells[i].w);minH=std::min(minH,cells[i].h);
        p.visible[i]=WindowVisible(cells[i],clip);
    }
    if(clip.w<=0||clip.h<=0||minW==1e100)return p;
    unsigned l=0;while(l<5&&Side(l+1)>=display)++l;
    p.densityLevel=l;
    unsigned worst=0;
    for(;;){
        unsigned n=Side(l)/WindowSide;
        double spanX=minW/n,spanZ=minH/n;
        unsigned nx=unsigned(std::min(double(4*n),std::ceil(clip.w/spanX)+1));
        unsigned nz=unsigned(std::min(double(4*n),std::ceil(clip.h/spanZ)+1));
        worst=nx*nz;
        if(l==5||worst<=DetailSlots)break;
        ++l;
    }
    p.level=l;if(l==5)return p;
    p.count=EnumerateWindows(cells,clip,l,p.keys,DetailSlots);p.visibleCount=p.count;
    // A one-window apron is opportunistic. It consumes only the spare capacity
    // beyond the WORST visible phase, and cannot force a coarser mip.
    unsigned reserve=DetailSlots-worst,n=Side(l)/WindowSide;
    double spanX=minW/n,spanZ=minH/n;
    WindowRect apron(clip.x-spanX,clip.y-spanZ,clip.w+2*spanX,clip.h+2*spanZ);
    WindowKey candidates[256];unsigned candidatesN=EnumerateWindows(cells,apron,l,candidates,256);
    if(candidatesN>256)return p;
    while(reserve--&&p.count<DetailSlots){
        int best=-1;double distance=1e100;
        for(unsigned k=0;k<candidatesN;++k){
            const WindowKey&key=candidates[k];if(p.Contains(key))continue;
            WindowRect r=SubWindow(cells[key.tile],key);
            double cx=r.x+r.w*.5,cz=r.y+r.h*.5;
            double dx=std::max(clip.x-cx,std::max(0.0,cx-clip.x-clip.w));
            double dz=std::max(clip.y-cz,std::max(0.0,cz-clip.y-clip.h));
            double tx=(cx-clip.x-clip.w*.5)/std::max(1.0,clip.w);
            double tz=(cz-clip.y-clip.h*.5)/std::max(1.0,clip.h);
            double score=dx*dx+dz*dz+.001*(tx*tx+tz*tz);
            if(score<distance){distance=score;best=int(k);}
        }
        if(best<0)break;p.keys[p.count++]=candidates[best];
    }
    return p;
}
// Retain coarse/fine images across zoom. Evict only an unneeded detail slot
// when admitting a new visible window; fixed fallback slots are never victims.
inline int WindowVictim(const WindowKey slots[WindowSlots],const unsigned age[WindowSlots],const WindowPlan&p){
    int victim=-1;for(unsigned i=FallbackSlots;i<WindowSlots;++i){if(!slots[i].Valid())return int(i);if(!p.Contains(slots[i])&&(victim<0||age[i]<age[victim]))victim=int(i);}return victim;
}
inline bool NeedsWindowShade(bool onScreen,bool wanted,unsigned shaded,unsigned lightVersion,unsigned pass){
    if(pass==0)return onScreen&&shaded==0;
    if(pass==1)return onScreen&&shaded!=0&&shaded!=lightVersion;
    return pass==2&&!onScreen&&wanted&&shaded==0;
}
inline U64 WindowGpuBytes(){return WindowSlots*CellBytes(5)+U64(WindowSide)*WindowSide*4+UniformGpuBytes;}
}
