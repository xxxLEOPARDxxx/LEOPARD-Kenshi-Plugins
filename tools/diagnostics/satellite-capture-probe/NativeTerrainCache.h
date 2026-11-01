#pragma once
#include <cstring>
#include <cstddef>
namespace NativeTerrainCache
{
// Offsets belong to the guarded installed Plugin_Terrain, not SDK class layout.
const std::size_t CameraOffset=0x178;
const std::size_t PositionOffset=0x180;
struct State { const void *camera; float position[3]; };
inline bool Matches(const void *object,const void *primary,const void *listener)
{
    if(!object || !primary || !listener)return false;
    const unsigned char *bytes=static_cast<const unsigned char*>(object);
    const void *a=NULL,*b=NULL;
    std::memcpy(&a,bytes,sizeof(a));std::memcpy(&b,bytes+0x170,sizeof(b));
    return a==primary && b==listener;
}
inline bool Read(const void *object,State &out)
{
    if(!object)return false;
    const unsigned char *bytes=static_cast<const unsigned char*>(object);
    std::memcpy(&out.camera,bytes+CameraOffset,sizeof(out.camera));
    std::memcpy(out.position,bytes+PositionOffset,sizeof(out.position));
    return true;
}
inline bool Write(void *object,const State &state)
{
    if(!object)return false;
    unsigned char *bytes=static_cast<unsigned char*>(object);
    std::memcpy(bytes+CameraOffset,&state.camera,sizeof(state.camera));
    std::memcpy(bytes+PositionOffset,state.position,sizeof(state.position));
    return true;
}
}
