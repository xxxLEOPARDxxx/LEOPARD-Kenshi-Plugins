// SPDX-License-Identifier: GPL-3.0-or-later
// Independently authored by LEOPARD. Inspiration: requizm's Gate Push Fix.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define KLOC_DOMAIN "gate_push_guard"
#include <Windows.h>
#include <Localization.h>
#include <ModConfigMenu.h>
#include <PluginDiagnostics.h>
#include <PluginSettings.h>
#include <core/Functions.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Faction.h>
#include <kenshi/Character.h>
#include <kenshi/CharMovement.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Building/DoorStuff.h>
#include <kenshi/PhysicsCollection.h>
#include <ogre/OgreMovableObject.h>
#include <ogre/OgreSceneNode.h>
#include "GuardEvents.h"
#include <cstdio>

extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace {
using namespace GatePushGuard;
const char* const Section="GatePushGuard";
enum { MaxDoors=64, DoorsPerFrame=8, ActorsPerDoor=128 };
SRWLOCK g_lock=SRWLOCK_INIT;
SRWLOCK g_cooldownLock=SRWLOCK_INIT;
SRWLOCK g_eventLock=SRWLOCK_INIT;
volatile LONG g_ready=0,g_enabled=1,g_fault=0,g_resetSamples=0;
volatile LONG g_insideMain=0;
unsigned g_cursor=0;
volatile LONG g_overflowWarned=0;
volatile LONG g_cooldownOverflow=0;
volatile LONG g_eventOverflow=0;
bool g_actorOverflowWarned=false,g_sampleOverflowWarned=false,g_conflictWarned=false;
ULONG_PTR g_worldIdentity=0; // numeric identity token; no retained game pointer
ULONGLONG g_lastFrame=0;
struct DoorEntry {
    Key key; unsigned epoch; bool used,closed;
    DoorEntry():epoch(1),used(false),closed(false){}
};
DoorEntry g_doors[MaxDoors];
struct MainEntry {
    Gate gate;Samples samples;unsigned epoch;bool geometry;
    Vec plane,outside,inside;
    MainEntry():epoch(0),geometry(false){}
};
MainEntry g_main[MaxDoors];
Cooldowns g_cooldowns;
RegionIndex g_regions;
EventQueue g_events;
ActorEvent g_batch[EventQueue::Capacity];
unsigned g_batchCount=0;

bool Stamp(const Key& key,double now,unsigned& revision) {
    AcquireSRWLockExclusive(&g_cooldownLock);const bool recent=g_cooldowns.Stamp(key,now,revision);ReleaseSRWLockExclusive(&g_cooldownLock);return recent;
}
bool Suppress(const Key& key,double now,bool reserveOnly) {
    AcquireSRWLockExclusive(&g_cooldownLock);
    const bool recent=reserveOnly&&g_cooldowns.Contains(key,now);
    const bool ok=!recent&&(reserveOnly?g_cooldowns.RecordCorrection(key,now):g_cooldowns.Record(key,now));
    ReleaseSRWLockExclusive(&g_cooldownLock);
    if(!ok&&!recent)InterlockedCompareExchange(&g_cooldownOverflow,1,0);
    return ok;
}

bool SupportedNative() {
    const unsigned char* base=(const unsigned char*)GetModuleHandleA(0);
    typedef void (Character::*Teleport)(const Ogre::Vector3&);
    const unsigned char* teleport=(const unsigned char*)KenshiLib::GetRealAddress((Teleport)&Character::teleport);
    const unsigned char* position=(const unsigned char*)KenshiLib::GetRealAddress(&DoorStuff::getDoorPosition);
    const unsigned char* outside=(const unsigned char*)KenshiLib::GetRealAddress(&DoorStuff::getDoorPosOutside);
    const unsigned char* inside=(const unsigned char*)KenshiLib::GetRealAddress(&DoorStuff::getDoorPosInside);
    const unsigned char* movementCharacter=(const unsigned char*)KenshiLib::GetRealAddress(&CharMovement::getCharacter);
    // Actual Steam1.0.65 / RE_Kenshi compatible executable504b362c..., mapped
    // from installed manual KenshiLib exports and Steam RVA slots. Unknown
    // versions, GOG offsets or an incompatible replacement stay inactive.
    static const unsigned char teleportPrefix[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x83,0xc1,0x48,0x48,0x8b,0xda,0xff,0x15,0xd6,0xc5,0xc7,0x01};
    static const unsigned char posGetter[]={0x48,0x8d,0x81,0xb0,0x03,0x00,0x00,0xc3};
    static const unsigned char outGetter[]={0x48,0x8d,0x81,0xbc,0x03,0x00,0x00,0xc3};
    static const unsigned char inGetter[]={0x48,0x8d,0x81,0xc8,0x03,0x00,0x00,0xc3};
    static const unsigned char movementGetter[]={0x48,0x8b,0x81,0xa8,0x03,0x00,0x00,0xc3};
    return base && teleport==base+0x5c9bf0 && position==base+0x332810 &&
        outside==base+0x2969f0 && inside==base+0x338630 && movementCharacter==base+0x2ae030 &&
        !std::memcmp(teleport,teleportPrefix,sizeof(teleportPrefix)) &&
        !std::memcmp(position,posGetter,sizeof(posGetter)) &&
        !std::memcmp(outside,outGetter,sizeof(outGetter)) &&
        !std::memcmp(inside,inGetter,sizeof(inGetter)) &&
        !std::memcmp(movementCharacter,movementGetter,sizeof(movementGetter));
}

Key Identity(const hand& h) {
    Key k;k.type=(unsigned)h.type;k.container=h.container;k.containerSerial=h.containerSerial;
    k.index=h.index;k.serial=h.serial;return k;
}
hand Handle(const Key& k) { return hand(k.index,k.serial,(itemType)k.type,k.container,k.containerSerial); }
Vec V(const Ogre::Vector3& p) { return Vec(p.x,p.y,p.z); }
bool Closed(DoorStuff* d) {
    return d && d->isSetupComplete && !d->_isBroken && !d->destroyed &&
           d->state==DOORSTATE_CLOSED && d->doorOpenAmount<=.001f &&
           d->doorOpenAmount>=0 && d->parent && d->parent->_buildState.isComplete;
}
bool Owned(DoorStuff* d) { if(!d||!d->parent)return false;Faction* f=d->getFaction();return f && f->isPlayer!=0; }

void Fault() { InterlockedExchange(&g_fault,1); }
void ClearRegistry() {
    AcquireSRWLockExclusive(&g_lock);
    for(unsigned i=0;i<MaxDoors;++i)g_doors[i]=DoorEntry();
    g_regions.Clear();
    ReleaseSRWLockExclusive(&g_lock);
    AcquireSRWLockExclusive(&g_eventLock);g_events.Clear();ReleaseSRWLockExclusive(&g_eventLock);
    InterlockedExchange(&g_resetSamples,1);
}
void Register(DoorStuff* d,bool force) {
    if(!InterlockedCompareExchange(&g_ready,0,0)||!d)return;
    Key key;bool closed=false,owned=false;
    __try {key=Identity(d->handle);closed=Closed(d);owned=Owned(d);}
    __except(EXCEPTION_EXECUTE_HANDLER) {Fault();return;}
    if(!owned)return;
    bool full=true;
    AcquireSRWLockExclusive(&g_lock);
    unsigned free=MaxDoors;
    for(unsigned i=0;i<MaxDoors;++i) {
        DoorEntry& e=g_doors[i];
        if(!e.used) {if(free==MaxDoors)free=i;continue;}
        if(e.key==key) {
            if(force||e.closed!=closed) {++e.epoch;g_regions.Set(i,false,Vec(),0);}
            e.closed=closed;full=false;break;
        }
    }
    if(full&&free<MaxDoors) {
        DoorEntry& e=g_doors[free];e.key=key;e.closed=closed;e.used=true;++e.epoch;full=false;
    }
    ReleaseSRWLockExclusive(&g_lock);
    if(full && !InterlockedExchange(&g_overflowWarned,1))ErrorLog("GatePushGuard: 64-door capacity reached; additional doors are skipped");
}

void (*g_origMovement)(CharMovement*,float)=0;
void Movement(CharMovement* movement,float time) {
    g_origMovement(movement,time);
    if(!InterlockedCompareExchange(&g_ready,0,0)||!InterlockedCompareExchange(&g_enabled,0,0))return;
    ActorEvent event;bool valid=false;
    // Callback may be off the main thread. Read scalar/POD values only; native
    // handle resolution, geometry and movement correction stay in Main.
    __try {
        Character* c=movement?movement->character:0;
        if(c && c->movement==movement && c->isPhysicalMode) {event.key=Identity(c->handle);event.position=V(c->pos);valid=RegionIndex::Supported(event.position);}
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {Fault();return;}
    if(!valid)return;
    AcquireSRWLockShared(&g_lock);const bool watched=g_regions.Contains(event.position);ReleaseSRWLockShared(&g_lock);
    if(!watched)return;
    event.seconds=(double)GetTickCount64()*.001;
    AcquireSRWLockExclusive(&g_eventLock);const bool queued=g_events.Push(event);ReleaseSRWLockExclusive(&g_eventLock);
    if(!queued)InterlockedCompareExchange(&g_eventOverflow,1,0);
}

void Watch(unsigned slot,bool active,const Gate& gate) {
    const Vec center(gate.center.x,(gate.minY+gate.maxY)*.5f,gate.center.z);
    const float radius=WatchRadius(gate);
    AcquireSRWLockExclusive(&g_lock);g_regions.Set(slot,active,center,radius);ReleaseSRWLockExclusive(&g_lock);
}

// Transforms the closed leaf mesh's local box corners, rather than projecting
// an inflated world AABB or using the whole parent gate/building bounds.
bool Geometry(DoorStuff* d,Gate& result) {
    if(!Closed(d)||!Owned(d)||!d->physical)return false;
    const lektor<PhysicsCollection::StaticEnt*>& parts=d->physical->staticEnts;
    if(parts.size()==0||parts.size()>16)return false;
    Leaf leaf;
    for(unsigned i=0;i<parts.size();++i) {
        PhysicsCollection::StaticEnt* part=parts[i];
        if(!part || !part->ent || part->parent)return false;
        Ogre::MovableObject* ent=part->ent;
        Ogre::Node* node=ent->getParentNode();if(!node)return false;
        const Ogre::Aabb box=ent->getLocalAabb();
        const Ogre::Matrix4 transform=node->_getFullTransformUpdated();
        if(!Finite(V(box.mCenter))||!Finite(V(box.mHalfSize))||
           box.mHalfSize.x<0||box.mHalfSize.y<0||box.mHalfSize.z<0)return false;
        for(unsigned corner=0;corner<8;++corner) {
            const Ogre::Vector3 local(box.mCenter.x+((corner&1)?1:-1)*box.mHalfSize.x,
                box.mCenter.y+((corner&2)?1:-1)*box.mHalfSize.y,
                box.mCenter.z+((corner&4)?1:-1)*box.mHalfSize.z);
            leaf.Add(V(transform.transformAffine(local)));
        }
    }
    return FitLeaf(result,V(d->getDoorPosition()),V(d->getDoorPosOutside()),V(d->getDoorPosInside()),leaf);
}
bool Eligible(Character* c) {
    return c && c->isPhysicalMode && c->movement && !c->isDead() &&
        !c->_NV_isUnconcious() && !c->isRagdoll() && !c->isBeingCarried() &&
        !c->isCarryingSomething && !c->isUsingTurret && !c->inWhat &&
        !c->movement->animationOverride && !c->movement->isTrackingAnimationMode();
}
__declspec(thread) Character* g_ownTeleportActor=0;
void (*g_origTeleport)(Character*,const Ogre::Vector3&)=0;
void ExternalTeleport(Character* c) {
    if(!InterlockedCompareExchange(&g_ready,0,0)||!InterlockedCompareExchange(&g_enabled,0,0)||c==g_ownTeleportActor)return;
    // Per-actor invalidation: a distant NPC sitting down must not repeatedly
    // reset every protected gate. Saturation safely clears all history.
    __try {if(!Suppress(Identity(c->handle),(double)GetTickCount64()*.001,false))InterlockedExchange(&g_resetSamples,1);}
    __except(EXCEPTION_EXECUTE_HANDLER) {Fault();}
}
void Teleport(Character* c,const Ogre::Vector3& target) {
    // External teleports are discontinuities even when shorter than four metres.
    // Invalidate before the original call, including a nested foreign actor.
    ExternalTeleport(c);
    g_origTeleport(c,target);
}
void (*g_origTeleportRot)(Character*,const Ogre::Vector3&,const Ogre::Quaternion&)=0;
void TeleportRot(Character* c,const Ogre::Vector3& target,const Ogre::Quaternion& rotation) {
    ExternalTeleport(c);
    g_origTeleportRot(c,target,rotation);
}
void ReturnOutside(Character* c,const Ogre::Vector3& target) {
    Character* previous=g_ownTeleportActor;g_ownTeleportActor=c;
    __try {c->teleport(target);}
    __finally {g_ownTeleportActor=previous;}
}

// All native access and the eventual correction happen on the existing game
// main-loop callback. Every pointer is resolved anew from the complete handle.
bool Scan(unsigned slot,const DoorEntry& entry,double seconds) {
    MainEntry& m=g_main[slot];
    hand dh=Handle(entry.key);Building* b=dh.getBuilding();
    if(!b || !b->imADoor)return false;
    DoorStuff* door=b->doorStuff();
    if(!door || !(Identity(door->handle)==entry.key)||!Closed(door)||!Owned(door)) {
        Watch(slot,false,m.gate);m.geometry=false;m.samples.Clear();return true;
    }
    if(!(m.gate.key==entry.key)||m.epoch!=entry.epoch) {
        m.geometry=false;m.samples.Clear();m.epoch=entry.epoch;
    }
    const Vec plane=V(door->getDoorPosition()),outside=V(door->getDoorPosOutside()),inside=V(door->getDoorPosInside());
    if(m.geometry && (Distance2(m.plane,plane)>.0001f || Distance2(m.outside,outside)>.0001f || Distance2(m.inside,inside)>.0001f)) {
        m.geometry=false;m.samples.Clear();
    }
    if(!m.geometry) {
        Watch(slot,false,m.gate);
        Gate gate;gate.key=entry.key;gate.epoch=entry.epoch;gate.closed=true;
        if(!Geometry(door,gate))return true;
        m.gate=gate;m.geometry=true;
        m.plane=plane;m.outside=outside;m.inside=inside;
        Watch(slot,true,m.gate);
        if(PluginDiagnostics::Enabled()) {
            char line[180];sprintf_s(line,"GatePushGuard: door %u/%u width %.2f height %.2f",entry.key.index,entry.key.serial,gate.halfWidth*2,gate.maxY-gate.minY);DebugLog(line);
        }
    }
    m.samples.Sweep(seconds);
    const Vec center(m.gate.center.x,(m.gate.minY+m.gate.maxY)*.5f,m.gate.center.z);
    const float radius=WatchRadius(m.gate);
    unsigned considered=0;
    for(unsigned i=0;i<g_batchCount;++i) {
        const ActorEvent& event=g_batch[i];
        if(event.seconds>seconds || seconds-event.seconds>.35 || Distance2(event.position,center)>radius*radius)continue;
        if(considered++>=ActorsPerDoor) {
            if(!g_actorOverflowWarned) {g_actorOverflowWarned=true;ErrorLog("GatePushGuard: 128 movement observations per door reached; crowded-door coverage may be incomplete");}
            break;
        }
        const Key key=event.key;
        hand ch=Handle(key);Character* c=ch.getCharacter();
        if(!c || !(Identity(c->handle)==key))continue;
        Observation observation;observation.key=key;observation.seconds=seconds;
        const bool suppressed=Stamp(key,seconds,observation.revision);
        observation.eligible=Eligible(c)&&!suppressed;observation.position=V(c->getPosition());
        Sample* sample=m.samples.Find(key,seconds);
        if(!sample) {
            if(!g_sampleOverflowWarned) {g_sampleOverflowWarned=true;ErrorLog("GatePushGuard: door history reached 256 actors; additional histories are skipped until stale entries expire");}
            continue;
        }
        const Action action=Observe(*sample,m.gate,observation);
        if(!action.correct)continue;
        // Recheck current native state immediately before a write. A native
        // gate opening or actor state change always wins over cached history.
        AcquireSRWLockShared(&g_lock);
        const bool continuous=g_doors[slot].used && g_doors[slot].key==entry.key && g_doors[slot].epoch==entry.epoch;
        ReleaseSRWLockShared(&g_lock);
        if(!continuous || InterlockedCompareExchange(&g_resetSamples,0,0) ||
           !Closed(door)||!Owned(door)||!Eligible(c) ||
           Distance2(V(c->getPosition()),observation.position)>.0001f) {sample->valid=false;continue;}
        // A correction at door A must never look like a new crossing at door B.
        // Publish an actor revision before calling into the game, without
        // suppressing true repeated crowd pressure at this door.
        if(!Suppress(key,seconds,true)) {sample->valid=false;continue;}
        // Steam native5c9bf0 assigns this absolute argument to Character::pos,
        // then passes it to CharMovement::_setPositionAndTeleport. SDK's
        // parameter name "moveBy" is misleading; do not pass a delta.
        const Ogre::Vector3 target(action.target.x,action.target.y,action.target.z);
        ReturnOutside(c,target);
        if(InterlockedCompareExchange(&g_resetSamples,0,0)) {sample->valid=false;break;}
        const bool afterSuppressed=Stamp(key,seconds,observation.revision);
        observation.position=V(c->getPosition());
        observation.eligible=Eligible(c)&&!afterSuppressed&&Distance2(observation.position,action.target)<.0001f;
        // Keep THIS confirmed outside target armed against sustained pressure;
        // the changed actor revision seeds every OTHER door's old history.
        Seed(*sample,m.gate,observation);
        if(PluginDiagnostics::Enabled()) {
            char line[160];sprintf_s(line,"GatePushGuard: returned actor %u/%u outside door %u/%u",key.index,key.serial,entry.key.index,entry.key.serial);DebugLog(line);
        }
    }
    return true;
}
bool SafeScan(unsigned slot,const DoorEntry& e,double seconds) {
    __try {return Scan(slot,e,seconds);}
    __except(EXCEPTION_EXECUTE_HANDLER) {Fault();return true;}
}
void Reload() {
    const std::string path=PluginSettings::Path();
    if(path.empty()) {ErrorLog("GatePushGuard: cannot resolve settings path");Fault();return;}
    InterlockedExchange(&g_enabled,PluginSettings::Bool(Section,"Enabled",true,path)?1:0);
    PluginDiagnostics::Configure(Section,path);
    InterlockedExchange(&g_resetSamples,1);
}
void (*g_origDoorUpdate)(DoorStuff*)=0;
void DoorUpdate(DoorStuff* d) {g_origDoorUpdate(d);Register(d,false);}
void (*g_origLoaded)(DoorStuff*)=0;
void Loaded(DoorStuff* d) {g_origLoaded(d);Register(d,true);}
void (*g_origState)(DoorStuff*,DoorState)=0;
void State(DoorStuff* d,DoorState s) {g_origState(d,s);Register(d,true);}
void (*g_origReset)(GameWorld*)=0;
void Reset(GameWorld* w) {ClearRegistry();g_origReset(w);}
void (*g_origLoad)(SaveFileSystem*,const std::string&)=0;
void Load(SaveFileSystem* s,const std::string& path) {ClearRegistry();g_origLoad(s,path);}
void (*g_origNew)(SaveFileSystem*)=0;
void New(SaveFileSystem* s) {ClearRegistry();g_origNew(s);}
void (*g_origMain)(GameWorld*,float)=0;
struct MainWorkGuard {~MainWorkGuard(){InterlockedExchange(&g_insideMain,0);} };
void Main(GameWorld* w,float time) {
    g_origMain(w,time);
    if(!InterlockedCompareExchange(&g_ready,0,0))return;
    if(InterlockedCompareExchange(&g_insideMain,1,0))return;
    MainWorkGuard workGuard;
    if(InterlockedCompareExchange(&g_eventOverflow,2,1)==1)ErrorLog("GatePushGuard: 2048-actor movement queue reached; additional events are skipped for this frame");
    if(InterlockedCompareExchange(&g_cooldownOverflow,2,1)==1)ErrorLog("GatePushGuard: 1024-actor teleport cooldown capacity reached; affected histories are skipped or reseeded");
    if(InterlockedCompareExchange(&g_fault,0,0)) {
        if(InterlockedExchange(&g_ready,0))ErrorLog("GatePushGuard: native access failed; protection disabled for this game session");return;
    }
    if(!w || ou!=w || !w->initialized || w->gameResetting || w->isLoadingFromASaveGame()) {
        InterlockedExchange(&g_resetSamples,1);return;
    }
    if(g_worldIdentity!=(ULONG_PTR)w) {g_worldIdentity=(ULONG_PTR)w;InterlockedExchange(&g_resetSamples,1);}
    if(InterlockedExchange(&g_resetSamples,0)) {
        AcquireSRWLockExclusive(&g_cooldownLock);g_cooldowns.Clear();ReleaseSRWLockExclusive(&g_cooldownLock);
        for(unsigned i=0;i<MaxDoors;++i) {g_main[i].geometry=false;g_main[i].samples.Clear();}
        AcquireSRWLockExclusive(&g_lock);g_regions.Clear();ReleaseSRWLockExclusive(&g_lock);
        AcquireSRWLockExclusive(&g_eventLock);g_events.Clear();ReleaseSRWLockExclusive(&g_eventLock);
    }
    if(!InterlockedCompareExchange(&g_enabled,0,0) || w->isPaused()) {
        InterlockedExchange(&g_resetSamples,1);return;
    }
    if(GetModuleHandleA("GatePushFix.dll")) {
        if(!g_conflictWarned) {g_conflictWarned=true;ErrorLog("GatePushGuard: original GatePushFix.dll is loaded; analogue protection is suspended to avoid competing corrections");}
        InterlockedExchange(&g_resetSamples,1);return;
    }
    const ULONGLONG now=GetTickCount64();
    if(now==g_lastFrame)return;g_lastFrame=now;
    AcquireSRWLockExclusive(&g_eventLock);g_batchCount=g_events.Drain(g_batch);ReleaseSRWLockExclusive(&g_eventLock);
    AcquireSRWLockExclusive(&g_cooldownLock);g_cooldowns.Sweep((double)now*.001);ReleaseSRWLockExclusive(&g_cooldownLock);
    for(unsigned n=0;n<DoorsPerFrame;++n) {
        const unsigned slot=g_cursor++%MaxDoors;DoorEntry e;
        AcquireSRWLockShared(&g_lock);e=g_doors[slot];ReleaseSRWLockShared(&g_lock);
        if(!e.used)continue;
        if(!SafeScan(slot,e,(double)now*.001)) {
            AcquireSRWLockExclusive(&g_lock);
            if(g_doors[slot].key==e.key && g_doors[slot].epoch==e.epoch)g_doors[slot].used=false;
            g_regions.Set(slot,false,Vec(),0);
            ReleaseSRWLockExclusive(&g_lock);
            g_main[slot].geometry=false;g_main[slot].samples.Clear();
        }
        if(InterlockedCompareExchange(&g_fault,0,0)||InterlockedCompareExchange(&g_resetSamples,0,0))break;
    }
    g_batchCount=0;
}
}

extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api) {
    if(!api)return;
    static const std::string path=PluginSettings::Path();
    api->beginMod(api,Section,"Gate Push Guard",path.c_str(),&Reload);
    if(api->version>=2)api->info(api,Tr("Stops observed crowd-push entry through closed player gates and doors. Independently authored; use instead of Gate Push Fix."));
    api->section(api,Tr("Protection"));
    api->toggle(api,Section,"Enabled",Tr("Enabled"),Tr("Return a character only after an observed outside-to-inside crossing through a continuously closed player door. Existing occupants and gate closure are never enough to move anyone."),1,0);
    api->section(api,Tr("Diagnostics"));
    api->toggle(api,Section,"Debug",Tr("Detailed log"),Tr("Log recognized door geometry and corrected crossings in RE_Kenshi_log.txt. Errors are always logged."),0,0);
}
__declspec(dllexport) void startPlugin() {
    Reload();
    if(!SupportedNative()) {ErrorLog("GatePushGuard: unsupported or modified native layout; protection remains disabled");return;}
#define INSTALL(method,hook,original) if(KenshiLib::AddHook(KenshiLib::GetRealAddress(method),hook,&original)!=KenshiLib::SUCCESS) {ErrorLog("GatePushGuard: could not install " #hook "; protection remains disabled");return;}
    typedef void (Character::*TeleportMethod)(const Ogre::Vector3&);
    typedef void (Character::*TeleportRotMethod)(const Ogre::Vector3&,const Ogre::Quaternion&);
    INSTALL((TeleportMethod)&Character::teleport,Teleport,g_origTeleport);
    INSTALL((TeleportRotMethod)&Character::teleport,TeleportRot,g_origTeleportRot);
    INSTALL(&SaveFileSystem::loadGame,Load,g_origLoad);
    INSTALL(&SaveFileSystem::newGame,New,g_origNew);
    INSTALL(&GameWorld::resetGame,Reset,g_origReset);
    INSTALL(&DoorStuff::_NV_onBuildingLoaded,Loaded,g_origLoaded);
    INSTALL(&DoorStuff::_NV_update,DoorUpdate,g_origDoorUpdate);
    INSTALL(&DoorStuff::setDoorState,State,g_origState);
    INSTALL(&CharMovement::_NV_update,Movement,g_origMovement);
    INSTALL(&GameWorld::_NV_mainLoop_GPUSensitiveStuff,Main,g_origMain);
#undef INSTALL
    InterlockedExchange(&g_ready,1);
    PluginDiagnostics::Write("GatePushGuard: v1 hooks installed");
}
