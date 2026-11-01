#pragma once
namespace RasterPolicy
{
struct Layout
{
    int side;
    unsigned __int64 pixels,targetBytes,readbackBytes,bmpBytes;
    float detail;bool resolutionOnly;
};
inline bool Select(int side,Layout &out,bool control=false)
{
    // High-resolution allocation control preserves the already proven2048 LOD.
    // It is not final mesh-quality acceptance and cannot enable the legacy path.
    const bool high=side==4096||side==8192;
    if(side!=512&&side!=1024&&side!=2048&&!(control&&high))return false;
    Layout value;value.side=side;value.pixels=static_cast<unsigned __int64>(side)*side;
    value.targetBytes=value.pixels*48; // Named color surfaces only; driver/depth excluded.
    value.readbackBytes=value.pixels*24; // Depth + Sun/position float RGBA + Save staging.
    value.bmpBytes=value.pixels*4+54;
    value.detail=high?8.0f*(side/2048):8.0f;value.resolutionOnly=high;
    out=value;return true;
}
inline unsigned __int64 RequiredHostBytes(const Layout &value)
{
    //1GiB native geometry cap +2GiB reserve; excludes GPU availability.
    return value.targetBytes+value.readbackBytes+3ULL*1024*1024*1024;
}
inline bool HostFits(const Layout &value,unsigned __int64 physical,unsigned __int64 commit)
{const unsigned __int64 need=RequiredHostBytes(value);return physical>=need&&commit>=need;}
inline bool ViewportMatches(const Layout &value,int actualHeight)
{return actualHeight==value.side;}
}
