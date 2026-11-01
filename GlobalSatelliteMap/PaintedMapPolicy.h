#pragma once
#include <float.h>
#include <math.h>
#include <stddef.h>

// Native ZoneMapOverlay A136E0 grid; pure, bounded, and allocation-free.
// Core ownership is half-open, except the closed outer world edge. Native UV
// footprints overlap by18m on either side and are never substituted for cores.
namespace SatellitePaintedMap
{
    static const double Origin=-147456.0,WorldMax=147456.0;
    static const double Core=36864.0,Border=18.0,Span=36900.0;
    struct Tile {int x,z;double minX,minZ,maxX,maxZ;};
    inline bool ImageShape(unsigned kind,size_t width,size_t height,size_t depth,
                           size_t faces,size_t mips,unsigned format,size_t bytes)
    {
        const unsigned side=kind==0?1024:2048;
        return kind<2&&width==side&&height==side&&depth==1&&faces==1&&mips==0&&
            (format==12||format==13)&&bytes==size_t(side)*side*4;
    }
    inline bool Finite(double x){return x==x&&x>=-DBL_MAX&&x<=DBL_MAX;}
    inline bool FromIndex(int x,int z,Tile &tile)
    {
        if(x<0||x>=8||z<0||z>=8)return false;
        Tile result={x,z,Origin+x*Core-Border,Origin+z*Core-Border,
            Origin+(x+1)*Core+Border,Origin+(z+1)*Core+Border};
        tile=result;return true;
    }
    inline bool Point(double x,double z,Tile &tile)
    {
        if(!Finite(x)||!Finite(z)||x<Origin||x>WorldMax||z<Origin||z>WorldMax)return false;
        // Divide before the integer offset: adding147456 first would erase a
        // tiny signed coordinate near the central seam through cancellation.
        const int ix=x==WorldMax?7:static_cast<int>(floor(x/Core))+4;
        const int iz=z==WorldMax?7:static_cast<int>(floor(z/Core))+4;
        return FromIndex(ix,iz,tile);
    }
    inline bool Uv(const Tile &tile,double x,double z,double &u,double &v)
    {
        Tile native={0};
        if(!FromIndex(tile.x,tile.z,native)||
            tile.minX!=native.minX||tile.minZ!=native.minZ||tile.maxX!=native.maxX||tile.maxZ!=native.maxZ||
            !Finite(x)||!Finite(z)||x<tile.minX||x>tile.maxX||z<tile.minZ||z>tile.maxZ)return false;
        u=(x-tile.minX)/Span;v=(z-tile.minZ)/Span;return true;
    }
    inline bool Cover(double minX,double minZ,double maxX,double maxZ,Tile (&tiles)[4],unsigned &count)
    {
        count=0;
        if(!Finite(minX)||!Finite(minZ)||!Finite(maxX)||!Finite(maxZ)||
            minX<Origin||minZ<Origin||maxX>WorldMax||maxZ>WorldMax||maxX<=minX||maxZ<=minZ)return false;
        const int firstX=static_cast<int>(floor(minX/Core))+4;
        const int firstZ=static_cast<int>(floor(minZ/Core))+4;
        const int lastX=static_cast<int>(ceil(maxX/Core))+3;
        const int lastZ=static_cast<int>(ceil(maxZ/Core))+3;
        const int size=(lastX-firstX+1)*(lastZ-firstZ+1);
        if(size<1||size>4)return false;
        for(int z=firstZ;z<=lastZ;++z)for(int x=firstX;x<=lastX;++x)
        {
            if(!FromIndex(x,z,tiles[count])){count=0;return false;}
            ++count;
        }
        return true;
    }
    inline bool Patch(double minX,double minZ,double maxX,double maxZ,Tile &tile)
    {
        Tile covered[4];unsigned count=0;
        if(!Cover(minX,minZ,maxX,maxZ,covered,count)||count!=1)return false;
        tile=covered[0];return true;
    }
}
