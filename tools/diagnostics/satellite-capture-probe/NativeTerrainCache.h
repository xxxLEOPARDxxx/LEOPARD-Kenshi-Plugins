#pragma once
#include <cstring>
#include <cstddef>
namespace NativeTerrainCache
{
// Offsets belong to the guarded installed Plugin_Terrain, not SDK class layout.
const std::size_t CameraOffset=0x178;
const std::size_t PositionOffset=0x180;
struct State { const void *camera; float position[3]; };
struct BloodState { const void *object,*camera; };
struct WorkerState {void *landscape,*handle;unsigned long id;volatile unsigned char *request;};
inline bool ReadWorker(const void *object,WorkerState &out)
{
    std::memset(&out,0,sizeof(out));
    if(!object)return false;
    const unsigned char *bytes=static_cast<const unsigned char*>(object);
    std::memcpy(&out.landscape,bytes+0x1c0,sizeof(out.landscape));
    if(!out.landscape)return false;
    unsigned char *land=static_cast<unsigned char*>(out.landscape);
    std::memcpy(&out.handle,land+0x178,sizeof(out.handle));
    // Native ctor initializes HANDLE, but not TID. Never inspect TID without a worker.
    if(out.handle)std::memcpy(&out.id,land+0x180,sizeof(out.id));
    out.request=land+0x188;return true;
}
inline bool ReadBlood(const void *object,BloodState &out)
{
    out.object=NULL;out.camera=NULL;
    if(!object)return false;
    const unsigned char *bytes=static_cast<const unsigned char*>(object);
    std::memcpy(&out.object,bytes+0x2f0,sizeof(out.object));
    if(!out.object)return false;
    std::memcpy(&out.camera,out.object,sizeof(out.camera));
    return out.camera!=NULL;
}
inline bool BloodMatches(const BloodState &before,const BloodState &after,const void *mainCamera)
{
    return mainCamera && before.object && before.object==after.object &&
           before.camera==mainCamera && after.camera==mainCamera;
}
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
