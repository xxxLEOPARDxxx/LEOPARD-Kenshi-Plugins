#pragma once
#include "AtlasPolicy.h"
#include <cmath>
#include <algorithm>
namespace AtlasView {
const unsigned WindowSide=256, FallbackSlots=16, DetailSlots=48, WindowSlots=64;
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
    WindowKey keys[DetailSlots];unsigned count,level;bool visible[16];
    WindowPlan():count(0),level(5){std::memset(visible,0,sizeof(visible));}
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
    WindowPlan p;double display=1;for(unsigned i=0;i<16;++i)if(WindowVisible(cells[i],clip)){p.visible[i]=true;display=std::max(display,std::max(cells[i].w,cells[i].h));}
    unsigned l=0;while(l<5&&Side(l+1)>=display)++l;
    while(l<5&&EnumerateWindows(cells,clip,l,NULL,DetailSlots)>DetailSlots)++l;
    p.level=l;if(l<5)p.count=EnumerateWindows(cells,clip,l,p.keys,DetailSlots);return p;
}
// Retain coarse/fine images across zoom. Evict only an unneeded detail slot
// when admitting a new visible window; fixed fallback slots are never victims.
inline int WindowVictim(const WindowKey slots[WindowSlots],const unsigned age[WindowSlots],const WindowPlan&p){
    int victim=-1;for(unsigned i=FallbackSlots;i<WindowSlots;++i){if(!slots[i].Valid())return int(i);if(!p.Contains(slots[i])&&(victim<0||age[i]<age[victim]))victim=int(i);}return victim;
}
inline U64 WindowGpuBytes(){return WindowSlots*CellBytes(5)+U64(WindowSide)*WindowSide*4+UniformGpuBytes;}
}
