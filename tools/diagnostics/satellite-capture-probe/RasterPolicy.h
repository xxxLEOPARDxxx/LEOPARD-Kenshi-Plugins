#pragma once
namespace RasterPolicy
{
struct Layout
{
    int side;
    unsigned __int64 pixels,targetBytes,readbackBytes,bmpBytes;
};
inline bool Select(int side,Layout &out)
{
    // Fixed diagnostic presets. Final8K requires separate memory/refinement proof.
    if(side!=512&&side!=1024&&side!=2048)return false;
    Layout value;value.side=side;value.pixels=static_cast<unsigned __int64>(side)*side;
    value.targetBytes=value.pixels*48; // Named color surfaces only; driver/depth excluded.
    value.readbackBytes=value.pixels*24; // Depth + Sun/position float RGBA + Save staging.
    value.bmpBytes=value.pixels*4+54;
    out=value;return true;
}
}
