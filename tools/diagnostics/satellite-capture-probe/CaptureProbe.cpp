#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <wincrypt.h>
#include <cstdio>
#include <cstring>
#include <float.h>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <Debug.h>
#include <core/Functions.h>
#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreTextureManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreImage.h>
#include <OgreDataStream.h>
#include <OgreRenderTexture.h>
#include <OgreGpuProgramManager.h>
#include <OgreGpuProgramParams.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreTextureUnitState.h>
#include <OgreRenderQueue.h>
#include <Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/OgreCompositorWorkspaceListener.h>
#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/OgreCompositorChannel.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassScene.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h>
#include <Compositor/Pass/PassClear/OgreCompositorPassClearDef.h>
#include <kenshi/Globals.h>
#include <kenshi/KingOfRenderThread.h>
#include <kenshi/Renderer.h>
#include <kenshi/GameWorld.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Terrain.h>
#define private public
#define protected public
#include <kenshi/gui/MapScreen.h>
#include <kenshi/Town.h>
#undef private
#undef protected
#include "GuardProfile.h"
#include "ProbePolicy.h"
#include "FixedProgramCache.h"
#include "OwnedCameraAttachment.h"
#include "OwnedCameraPose.h"
#include "PositionShader.h"
#include "SunShader.h"
#include "NativeTerrainCache.h"
#include "RenderAbiProfile.h"
#include "TerrainExperimentPolicy.h"
#include "NativeWorkerHandle.h"
#include "ExposurePolicy.h"
#include "RasterPolicy.h"
#include "OwnedRtvDebt.h"
#include "AtlasCapturePolicy.h"
#include "AtlasPixels.h"
#include "../../../GlobalSatelliteMap/TextureArrayPolicy.h"
#include "../../../GlobalSatelliteMap/GpuMipUpload.h"
#include "GpuCompressedUpload.h"
#include "GeographyCompletion.h"
#include "FixedTerrainControl.h"
#include "NativeReceiptControl.h"
#include "WorldFrameControl.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/world-native/NativeProvider.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/world-native-rebind/NativeSession.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/world-build/WorldBuildControl.h"
#include "../../../GlobalSatelliteMap/atlas-terrain/world-layout/WorldLayout.h"
#include "../../../GlobalSatelliteMap/atlas-cache/world-build-disk/WorldBuildDisk.h"
#include <WidgetRef.h>
#include "../../../GlobalSatelliteMap/atlas-terrain/staged-material/_generated/StagedShaders.h"
#pragma comment(lib, "advapi32.lib")
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace
{
int Side = 512; // Latched once at startup, before any compositor definition exists.
RasterPolicy::Layout diagnosticRaster;
DWORD mainThread = 0;
bool keyHeld = false, pending = false, cleanup = false, inside = false, profileOK = false,
     chainSafe = true;
int mode = 0, cx = 0, cz = 0;
bool proxies = false;
unsigned serial = 0;
bool fileRequests=false;
bool backgroundFileRequests=false; // Explicit/private opt-in; never affects keyboard input.
bool reusePrivateShaders=false;
ProbePolicy::FixedProgramCache programCache;
DWORD lastRequestPoll=0;
std::string requestIni;
ProbePolicy::RequestLatch requestLatch;
GameWorld *requestedWorld = NULL;
Ogre::CompositorWorkspace *gameWorkspace = NULL, *shot = NULL;
Renderer *renderer = NULL;
Ogre::Camera *own = NULL;
Ogre::SceneNode *ownNode = NULL;
Ogre::SceneManager *shotScene = NULL;
Ogre::IdType ownNodeId = 0;
Ogre::TexturePtr target;
Ogre::TexturePtr positionTarget;
Ogre::MaterialPtr positionMaterial;
Ogre::HighLevelGpuProgramPtr positionVertex, positionFragment;
Ogre::CompositorWorkspace *positionWorkspace = NULL;
Ogre::CompositorManager2 *positionDefinitionOwner = NULL;
bool positionNodeDefined = false, positionWorkspaceDefined = false;
bool positionDefinitionsFailed = false;
const char *PositionNodeName = "LEOPARD_PrivateCapturePositionNode";
const char *PositionWorkspaceName = "LEOPARD_PrivateCapturePositionWorkspace";
const char *PositionMaterialName = "LEOPARD_PrivateCapturePositionMaterial";
const char *PositionVertexName = "LEOPARD_PrivateCapturePositionVertex";
const char *PositionFragmentName = "LEOPARD_PrivateCapturePositionFragment";
const char *PositionTargetName = "LEOPARD_PrivateCapturePositionTarget";
Ogre::TexturePtr sunTarget;
Ogre::MaterialPtr sunMaterial;
Ogre::HighLevelGpuProgramPtr sunFragment;
Ogre::CompositorWorkspace *sunWorkspace = NULL;
bool sunDefinitionsReady = false, sunDefinitionsFailed = false;
const char *SunNodeName = "LEOPARD_PrivateCaptureSunNode";
const char *SunWorkspaceName = "LEOPARD_PrivateCaptureSunWorkspace";
const char *SunMaterialName = "LEOPARD_PrivateCaptureSunMaterial";
const char *SunFragmentName = "LEOPARD_PrivateCaptureSunFragment";
const char *SunTargetName = "LEOPARD_PrivateCaptureSunTarget";
Ogre::CompositorWorkspaceListener *previous = NULL;
Ogre::CompositorWorkspaceListener *ourListener = NULL;
std::vector<hand> handles;
bool restorationSafe = true;
bool fixedControlEnabled=false,fixedGsmStartupOff=false;
bool nativeReceiptEnabled=false;
bool worldFrameEnabled=false,worldFrameStop=false;
WorldFrameControl::Controller<WorldNative::Provider>*worldFrame=NULL; // Created on checked Main, never loader thread.
WorldStream::Job worldFrameJob;
WidgetRef worldFrameMapRoot,worldFrameMapImage;
ProbePolicy::RequestLatch worldFrameLatch;
DWORD worldFramePollAt=0,worldFrameToken=0;
unsigned worldFrameEpoch=0;
volatile LONG worldFrameLoadGeneration=0;
LONG worldFrameCapturedLoad=0;
typedef WorldBuild::Controller<WorldNativeRebind::NativeSession,WorldBuildDisk::Factory,WorldBuildDisk::Config> NativeWorldBuild;
NativeWorldBuild*worldBuild=NULL;
bool worldBuildEnabled=false,worldBuildStop=false;
SourceReceipts::Scope worldBuildScope={};WorldCohort::Job worldBuildJob;
WorldRaster::Material worldBuildMaterial;unsigned worldBuildMaterialRegion=64;
unsigned worldBuildCount=0,worldBuildDone=0,worldBuildCase=0;DWORD worldBuildToken=0,worldBuildPollAt=0;
LONG worldBuildCapturedLoad=0;ProbePolicy::RequestLatch worldBuildLatch;
std::string worldBuildRoot,worldBuildDirectory;
void (*worldFrameOriginalLoad)(SaveFileSystem*,const std::string&)=NULL;
void (*worldFrameOriginalNew)(SaveFileSystem*)=NULL;

DWORD nativeReceiptPollAt=0;
ProbePolicy::RequestLatch nativeReceiptLatch;
std::string fixedGsmIni;
ProbePolicy::RequestLatch fixedRequestLatch;
FixedTerrainControl::Controller fixedControl;
DWORD fixedPollAt=0;unsigned fixedEpoch=0;
struct NativeRenderApi
{
    typedef Ogre::Viewport *(__fastcall *GetLastFn)(const Ogre::Camera *);
    typedef void (__fastcall *NotifyFn)(Ogre::Camera *,Ogre::Viewport *);
    typedef bool (__fastcall *GetMinFn)(const Ogre::Camera *);
    typedef void (__fastcall *SetMinFn)(Ogre::Camera *,bool);
    typedef Ogre::Frustum *(__fastcall *GetCullFn)(const Ogre::Camera *);
    HMODULE ogre,terrain,d3d11;
    typedef unsigned (__fastcall *UseCountFn)(const void*);
    UseCountFn useCount;
    GetLastFn getLast;NotifyFn notify;GetMinFn getMin;SetMinFn setMin;GetCullFn getCull;
    const void *terrainPrimary,*terrainListener;
    bool ready;
    NativeRenderApi():ogre(NULL),terrain(NULL),d3d11(NULL),useCount(NULL),getLast(NULL),notify(NULL),getMin(NULL),setMin(NULL),
        getCull(NULL),terrainPrimary(NULL),terrainListener(NULL),ready(false){}
} nativeApi;
void (*originalUpdate)(MapScreen *) = NULL;
void Log(const std::string &s)
{
    DebugLog("SatelliteCaptureProbe: " + s);
}
std::string Folder()
{
    char p[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), p, MAX_PATH);
    if (!n || n >= MAX_PATH)
        throw std::runtime_error("probe path unavailable");
    std::string s(p, n);
    return s.substr(0, s.find_last_of("/\\") + 1);
}
std::string FileHash(const char *path)
{
    HANDLE f =
        CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (f == INVALID_HANDLE_VALUE)
        return "";
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    bool ok = CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
              CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash);
    BYTE buf[65536];
    DWORD n = 0;
    unsigned long long total = 0;
    while (ok)
    {
        if (!ReadFile(f, buf, sizeof(buf), &n, NULL))
        {
            ok = false;
            break;
        }
        if (!n)
            break;
        total += n;
        if (total > 64ull * 1024 * 1024)
        {
            ok = false;
            break;
        }
        ok = CryptHashData(hash, buf, n, 0) != FALSE;
    }
    BYTE digest[32];
    n = 32;
    ok = ok && CryptGetHashParam(hash, HP_HASHVAL, digest, &n, 0);
    if (hash)
        CryptDestroyHash(hash);
    if (provider)
        CryptReleaseContext(provider, 0);
    CloseHandle(f);
    if (!ok)
        return "";
    std::string out;
    const char *hex = "0123456789abcdef";
    for (int i = 0; i < 32; ++i)
    {
        out += hex[digest[i] >> 4];
        out += hex[digest[i] & 15];
    }
    return out;
}
bool LoadedGuard()
{
    const BYTE *base = reinterpret_cast<const BYTE *>(GetModuleHandleA(NULL));
    __try
    {
        const IMAGE_DOS_HEADER *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        const IMAGE_NT_HEADERS64 *nt =
            reinterpret_cast<const IMAGE_NT_HEADERS64 *>(base + dos->e_lfanew);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->FileHeader.TimeDateStamp != ExpectedTimestamp ||
            nt->OptionalHeader.SizeOfImage != ExpectedImageSize)
            return false;
        for (unsigned i = 0; i < sizeof(NativeChecks) / sizeof(NativeChecks[0]); ++i)
            if (std::memcmp(base + NativeChecks[i].rva, NativeChecks[i].code, 24))
                return false;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}
bool AbiModuleMatches(HMODULE module,DWORD timestamp,DWORD size,const AbiWindow *checks,size_t count)
{
    if(!module)return false;
    __try
    {
        const BYTE *base=reinterpret_cast<const BYTE *>(module);
        const IMAGE_DOS_HEADER *dos=reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<static_cast<LONG>(sizeof(IMAGE_DOS_HEADER)) || dos->e_lfanew>4096)return false;
        const IMAGE_NT_HEADERS64 *nt=reinterpret_cast<const IMAGE_NT_HEADERS64 *>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
           nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
           nt->FileHeader.TimeDateStamp!=timestamp || nt->OptionalHeader.SizeOfImage!=size)return false;
        for(size_t i=0;i<count;++i)
            if(checks[i].rva>size-24 || std::memcmp(base+checks[i].rva,checks[i].code,24))return false;
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool RenderAbiMatches()
{
    return nativeApi.ready && GetModuleHandleA(OgreAbiFile)==nativeApi.ogre &&
        GetModuleHandleA(TerrainAbiFile)==nativeApi.terrain &&
        GetModuleHandleA(D3d11AbiFile)==nativeApi.d3d11 &&
        AbiModuleMatches(nativeApi.ogre,OgreAbiTimestamp,OgreAbiSize,OgreAbiChecks,sizeof(OgreAbiChecks)/sizeof(OgreAbiChecks[0])) &&
        AbiModuleMatches(nativeApi.terrain,TerrainAbiTimestamp,TerrainAbiSize,TerrainAbiChecks,sizeof(TerrainAbiChecks)/sizeof(TerrainAbiChecks[0])) &&
        AbiModuleMatches(nativeApi.d3d11,D3d11AbiTimestamp,D3d11AbiSize,D3d11AbiChecks,sizeof(D3d11AbiChecks)/sizeof(D3d11AbiChecks[0]));
}
FARPROC AbiExport(HMODULE module,const char *name,DWORD rva)
{
    FARPROC function=GetProcAddress(module,name);
    if(!function || reinterpret_cast<const BYTE *>(function)!=reinterpret_cast<const BYTE *>(module)+rva)
        throw std::runtime_error("native render ABI export differs; capture refused");
    return function;
}
bool ModuleFileMatches(HMODULE module,const char *expected)
{
    char path[MAX_PATH]={0};DWORD size=GetModuleFileNameA(module,path,MAX_PATH);
    return size && size<MAX_PATH && FileHash(path)==expected;
}
void EnsureRenderAbi()
{
    if(nativeApi.ready)
    {
        if(!RenderAbiMatches())throw std::runtime_error("native render ABI changed; capture refused");
        return;
    }
    NativeRenderApi next;
    next.ogre=GetModuleHandleA(OgreAbiFile);next.terrain=GetModuleHandleA(TerrainAbiFile);
    next.d3d11=GetModuleHandleA(D3d11AbiFile);
    if(!next.ogre || !next.terrain || !next.d3d11 || !ModuleFileMatches(next.ogre,OgreAbiHash) || !ModuleFileMatches(next.terrain,TerrainAbiHash) || !ModuleFileMatches(next.d3d11,D3d11AbiHash))
        throw std::runtime_error("exact Ogre/Terrain/D3D11 file profile failed; capture refused");
    next.useCount=reinterpret_cast<NativeRenderApi::UseCountFn>(AbiExport(next.ogre,PrivateSharedUseCountName,PrivateSharedUseCountRva));
    next.getLast=reinterpret_cast<NativeRenderApi::GetLastFn>(AbiExport(next.ogre,GetLastName,GetLastRva));
    next.notify=reinterpret_cast<NativeRenderApi::NotifyFn>(AbiExport(next.ogre,NotifyViewportName,NotifyViewportRva));
    next.getMin=reinterpret_cast<NativeRenderApi::GetMinFn>(AbiExport(next.ogre,GetMinName,GetMinRva));
    next.setMin=reinterpret_cast<NativeRenderApi::SetMinFn>(AbiExport(next.ogre,SetMinName,SetMinRva));
    next.getCull=reinterpret_cast<NativeRenderApi::GetCullFn>(AbiExport(next.ogre,GetCullName,GetCullRva));
    next.terrainPrimary=reinterpret_cast<const void *>(AbiExport(next.terrain,TerrainPrimaryName,TerrainPrimaryRva));
    next.terrainListener=reinterpret_cast<const void *>(AbiExport(next.terrain,TerrainListenerName,TerrainListenerRva));
    next.ready=true;nativeApi=next;
    if(!RenderAbiMatches()){nativeApi.ready=false;throw std::runtime_error("loaded Ogre/Terrain code profile failed");}
    Log("exact native render ABI enabled: exported camera accessors and guarded Terrain178/180");
}
ProbeOwnership::OwnedRtvDebt rtvDebt;
struct OwnedTargetPins
{
    struct Slot { Ogre::TexturePtr texture; Ogre::HardwarePixelBufferSharedPtr buffer; Ogre::RenderTexture *target; Slot():target(NULL){} } slots[6];
};
OwnedTargetPins& TargetPins()
{
    // One bounded metadata allocation for this DLL. In quarantine its six pins
    // intentionally live to process exit; do not run native destructors after
    // OgreRoot shutdown from a C++ static destructor.
    static OwnedTargetPins *pins=new OwnedTargetPins;
    return *pins;
}
bool ExactPrivateRttType(const Ogre::RenderTexture *target)
{
    if(!target||!RenderAbiMatches())return false;
    __try
    {
        const BYTE *base=reinterpret_cast<const BYTE*>(nativeApi.d3d11);
        const void *const *vtable=*reinterpret_cast<const void *const *const *>(target);
        return vtable==reinterpret_cast<const void *const *>(base+PrivateRttVtableRva) &&
            vtable[0]==base+PrivateRttDestructorRva &&
            vtable[0x90/sizeof(void*)]==base+PrivateRttGetterRva;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void* BorrowPrivateView(Ogre::RenderTexture *target)
{
    if(!ExactPrivateRttType(target))throw std::runtime_error("owned D3D11 RTT type/getter differs");
    void *view=NULL;
    target->getCustomAttribute("ID3D11RenderTargetView",&view); // Native getter does NOT AddRef.
    if(!view)throw std::runtime_error("owned D3D11 RTV is null");
    return view;
}
bool ReleaseCreationView(void *view,unsigned long &remaining)
{
    if(!view||!RenderAbiMatches())return false;
    __try
    {
        typedef unsigned long (__stdcall *ReleaseFn)(void*);
        void **vtable=*reinterpret_cast<void***>(view);
        remaining=reinterpret_cast<ReleaseFn>(vtable[2])(view);
        return true;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void TrackPrivateTarget(const Ogre::TexturePtr &texture)
{
    if(!rtvDebt.Active())return; // Legacy diagnostic modes are outside this repair.
    if(texture.isNull()||rtvDebt.Count()>=6)throw std::runtime_error("owned RTV registry capacity/texture refused");
    const Ogre::CompositorChannel &mainGb=renderer->workspace->getGlobalTexture("global_gbuffer");
    for(size_t i=0;i<mainGb.textures.size();++i)
        if(texture==mainGb.textures[i])throw std::runtime_error("owned RTV aliases main GBuffer");
    const unsigned index=rtvDebt.Count();OwnedTargetPins::Slot &pin=TargetPins().slots[index];
    if(!pin.texture.isNull()||!pin.buffer.isNull())throw std::runtime_error("owned RTV slot still pinned");
    pin.texture=texture; // Retain immediately, including subsequent validation failure.
    pin.buffer=texture->getBuffer();
    pin.target=pin.buffer->getRenderTarget();
    void *view=BorrowPrivateView(pin.target);
    if(!rtvDebt.Register(index,pin.texture.get(),pin.buffer.get(),pin.target,view))
        throw std::runtime_error("owned RTV alias/identity registration refused");
}
struct PrivateViewCleanup
{
    bool Exclusive(unsigned i,const ProbeOwnership::RtvDebt& debt)
    {
        OwnedTargetPins::Slot &pin=TargetPins().slots[i];
        // The exact native accessor reads only generic SharedPtr's control block
        // ([this+8], then count+20), equally for TexturePtr and PixelBufferPtr.
        return RenderAbiMatches() && pin.texture.get()==debt.texture && pin.buffer.get()==debt.buffer &&
            pin.target==debt.target && BorrowPrivateView(pin.target)==debt.view &&
            nativeApi.useCount(&pin.texture)==1 && nativeApi.useCount(&pin.buffer)==2;
    }
    bool DropTexture(unsigned i){TargetPins().slots[i].texture.setNull();return true;}
    unsigned BufferCount(unsigned i){return nativeApi.useCount(&TargetPins().slots[i].buffer);}
    bool DropBuffer(unsigned i){TargetPins().slots[i].buffer.setNull();TargetPins().slots[i].target=NULL;return true;}
    bool ReleaseView(unsigned i,void *view)
    {
        unsigned long remaining=0;
        if(!ReleaseCreationView(view,remaining))return false;
        std::ostringstream line;line<<"owned RTV creation debt released slot="<<i<<" remaining="<<remaining;Log(line.str());return true;
    }
};
bool ReadTerrainCache(Terrain *terrain,NativeTerrainCache::State &state)
{
    if(!RenderAbiMatches())return false;
    __try
    {
        return NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener) &&
               NativeTerrainCache::Read(terrain,state);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool WriteTerrainCache(Terrain *terrain,const NativeTerrainCache::State &state)
{
    if(!RenderAbiMatches())return false;
    __try
    {
        return NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener) &&
               NativeTerrainCache::Write(terrain,state);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadTerrainBlood(Terrain *terrain,NativeTerrainCache::BloodState &state)
{
    if(!RenderAbiMatches())return false;
    __try
    {
        return NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener) &&
               NativeTerrainCache::ReadBlood(terrain,state);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadOwnedTerrainWorker(Terrain *terrain,NativeTerrainCache::WorkerState &state)
{
    if(!terrain||!au||terrain==au->terrain||!RenderAbiMatches())return false;
    __try
    {
        return NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener)&&
            NativeTerrainCache::ReadWorker(terrain,state);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
BOOL CALLBACK FindWindowThread(HWND w, LPARAM)
{
    DWORD pid = 0;
    DWORD thread = GetWindowThreadProcessId(w, &pid);
    if (pid == GetCurrentProcessId() && IsWindowVisible(w) && GetWindow(w, GW_OWNER) == NULL)
    {
        mainThread = thread;
        return FALSE;
    }
    return TRUE;
}
bool OnMain()
{
    return mainThread && GetCurrentThreadId() == mainThread;
}
struct WorkspaceAccess : Ogre::CompositorWorkspace
{
    static const Ogre::CompositorWorkspaceDef *Def(const Ogre::CompositorWorkspace *w)
    {
        return w->*(&WorkspaceAccess::mDefinition);
    }
};
struct DefinitionAccess : Ogre::CompositorWorkspaceDef
{
    static Ogre::IdString Name(const Ogre::CompositorWorkspaceDef *d)
    {
        return d->*(&DefinitionAccess::mName);
    }
};
struct ManagerAccess : Ogre::CompositorManager2
{
    static bool Contains(Ogre::CompositorManager2 *m, Ogre::CompositorWorkspace *w)
    {
        if (!w)
            return false;
        const WorkspaceVec &active = m->*(&ManagerAccess::mWorkspaces);
        const QueuedWorkspaceVec &queued = m->*(&ManagerAccess::mQueuedWorkspaces);
        // Cleanup is an infrequent identity scan over engine-owned vectors. A size cap
        // must never turn "membership unknown" into "absent" and strand owned resources.
        if (ProbePolicy::Contains(active.empty() ? NULL : &active[0], active.size(), w))
            return true;
        for (size_t i = 0; i < queued.size(); ++i)
            if (queued[i].workspace == w)
                return true;
        return false;
    }
};
bool Supported(Ogre::Camera *c)
{
    return c && !c->isCustomViewMatrixEnabled() && !c->isCustomProjectionMatrixEnabled() &&
           !c->isReflected() && !c->isWindowSet() && !c->isCustomNearClipPlaneEnabled();
}
// Manual-request telemetry only. Do not call Node's *Updated getters here:
// refreshing the new node before the baseline would hide the suspected stale cache.
void NodeSample(const char *stage, Ogre::SceneNode *node)
{
    if (!node)
        return;
    const Ogre::Vector3 local = node->getPosition(), cached = node->_getDerivedPosition(),
                        scale = node->_getDerivedScale();
    const Ogre::Quaternion localQ = node->getOrientation(), cachedQ = node->_getDerivedOrientation();
    std::ostringstream s;
    s.precision(9);
    s << "node sample " << stage << " node=" << node << " parent=" << node->getParent()
      << " local=" << local.x << ',' << local.y << ',' << local.z
      << " cached=" << cached.x << ',' << cached.y << ',' << cached.z
      << " localQ=" << localQ.w << ',' << localQ.x << ',' << localQ.y << ',' << localQ.z
      << " cachedQ=" << cachedQ.w << ',' << cachedQ.x << ',' << cachedQ.y << ',' << cachedQ.z
      << " cachedScale=" << scale.x << ',' << scale.y << ',' << scale.z;
    Log(s.str());
}
void CameraSample(const char *stage, const char *label, Ogre::Camera *c,
                  const Ogre::Matrix4 &view, const Ogre::Matrix4 &projection)
{
    const Ogre::Vector3 local = c->getPosition(), derived = c->getDerivedPosition();
    const Ogre::Quaternion q = c->getDerivedOrientation();
    std::ostringstream s;
    s.precision(9);
    s << "camera sample " << stage << ' ' << label << " camera=" << c
      << " parent=" << c->getParentSceneNode()
      << " local=" << local.x << ',' << local.y << ',' << local.z
      << " derived=" << derived.x << ',' << derived.y << ',' << derived.z
      << " derivedQ=" << q.w << ',' << q.x << ',' << q.y << ',' << q.z
      << " near/far=" << c->getNearClipDistance() << ',' << c->getFarClipDistance()
      << " fov/aspect=" << c->getFOVy().valueRadians() << ',' << c->getAspectRatio()
      << " cull=" << nativeApi.getCull(c) << " lod=" << c->getLodCamera() << " view=";
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
            s << view[row][col] << ',';
    s << " projection=";
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
            s << projection[row][col] << ',';
    Log(s.str());
}
void CameraPairSample(const char *stage, Ogre::Camera *mainCamera, Ogre::Camera *owned)
{
    NodeSample(stage, owned->getParentSceneNode());
    const Ogre::Matrix4 mainView = mainCamera->getViewMatrix(true),
                        ownView = owned->getViewMatrix(true),
                        mainProjection = mainCamera->getProjectionMatrix(),
                        ownProjection = owned->getProjectionMatrix();
    double viewDelta = 0, projectionDelta = 0;
    unsigned nonFinite = 0;
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned col = 0; col < 4; ++col)
        {
            if (!_finite(mainView[row][col]) || !_finite(ownView[row][col]) ||
                !_finite(mainProjection[row][col]) || !_finite(ownProjection[row][col]))
                ++nonFinite;
            else
            {
                viewDelta = (std::max)(viewDelta, std::fabs(double(mainView[row][col]) - ownView[row][col]));
                projectionDelta = (std::max)(projectionDelta,
                    std::fabs(double(mainProjection[row][col]) - ownProjection[row][col]));
            }
        }
    CameraSample(stage, "main", mainCamera, mainView, mainProjection);
    CameraSample(stage, "owned", owned, ownView, ownProjection);
    std::ostringstream s;
    s.precision(9);
    s << "camera pair " << stage << " max view delta=" << viewDelta
      << " projection delta=" << projectionDelta << " nonfinite entries=" << nonFinite;
    Log(s.str());
}
struct CameraScope
{
    Ogre::Camera *c;
    Renderer *r;
    Ogre::Camera *binding;
    Ogre::Vector3 position;
    Ogre::Quaternion orientation;
    Ogre::Radian fov;
    Ogre::ProjectionType type;
    float nearZ, farZ, aspect, width, height;
    bool autoAspect, useDistance, usePixels;
    Ogre::Viewport *last;
    CameraScope(Renderer *R)
        : c(R->camera), r(R), binding(R->camera), position(c->getPosition()),
          orientation(c->getOrientation()), fov(c->getFOVy()), type(c->getProjectionType()),
          nearZ(c->getNearClipDistance()), farZ(c->getFarClipDistance()),
          aspect(c->getAspectRatio()), width(c->getOrthoWindowWidth()),
          height(c->getOrthoWindowHeight()), autoAspect(c->getAutoAspectRatio()),
          useDistance(c->getUseRenderingDistance()), usePixels(nativeApi.getMin(c)),
          last(nativeApi.getLast(c))
    {
    }
    ~CameraScope()
    {
        r->camera = binding;
        c->setPosition(position);
        c->setOrientation(orientation);
        c->setOrthoWindow(width, height);
        c->setAspectRatio(aspect);
        c->setProjectionType(type);
        c->setFOVy(fov);
        c->setNearClipDistance(nearZ);
        c->setFarClipDistance(farZ);
        c->setAutoAspectRatio(autoAspect);
        c->setUseRenderingDistance(useDistance);
        nativeApi.setMin(c,usePixels);
        nativeApi.notify(c,last);
    }
};
struct ExposureScope
{
    Ogre::GpuSharedParametersPtr params;
    float lo, hi, rate;
    bool armed;
    float Read(const char *name)
    {
        const Ogre::GpuConstantDefinition &d = params->getConstantDefinition(name);
        if (d.constType != Ogre::GCT_FLOAT1 || d.arraySize != 1)
            throw std::runtime_error("HDR parameter ABI mismatch");
        return *static_cast<const Ogre::GpuSharedParameters *>(params.get())
                    ->getFloatPointer(d.physicalIndex);
    }
    ExposureScope() : armed(false)
    {
        params = Ogre::GpuProgramManager::getSingleton().getSharedParameters("HDRSettings");
        lo = Read("MIN_LUMINANCE");
        hi = Read("MAX_LUMINANCE");
        rate = Read("AUTOEXP_ADAPTATION_RATE");
        armed = true;
    }
    void Apply()
    {
        params->setNamedConstant("MIN_LUMINANCE", 0.9f);
        params->setNamedConstant("MAX_LUMINANCE", 0.9f);
        params->setNamedConstant("AUTOEXP_ADAPTATION_RATE", 1.0f);
    }
    ~ExposureScope()
    {
        if (armed)
        {
            params->setNamedConstant("MIN_LUMINANCE", lo);
            params->setNamedConstant("MAX_LUMINANCE", hi);
            params->setNamedConstant("AUTOEXP_ADAPTATION_RATE", rate);
        }
    }
};
struct TerrainCameraScope
{
    Terrain *terrain;
    NativeTerrainCache::State state;
    NativeTerrainCache::BloodState blood;
    const Ogre::Camera *mainCamera;
    TerrainCameraScope()
        : terrain(au ? au->terrain : NULL),mainCamera(au && au->render && au->render->camera ? au->render->camera->getLodCamera() : NULL)
    {
        std::memset(&state,0,sizeof(state));
        if(terrain && !ReadTerrainCache(terrain,state))
            throw std::runtime_error("Terrain instance/layout guard failed; capture refused");
        if(!terrain || !ReadTerrainBlood(terrain,blood) || !NativeTerrainCache::BloodMatches(blood,blood,mainCamera))
            throw std::runtime_error("Terrain helper must already borrow main LOD camera; capture refused");
    }
    ~TerrainCameraScope()
    {
        // A secondary viewport can leave this pointer referencing its temporary camera.
        // Restore the known camera cache without depending on a subsequent successful main pass.
        if (terrain)
        {
            NativeTerrainCache::BloodState after;
            if(!au || au->terrain!=terrain || !ReadTerrainBlood(terrain,after) ||
               !NativeTerrainCache::BloodMatches(blood,after,mainCamera) || !WriteTerrainCache(terrain,state))
            {
                restorationSafe=false;chainSafe=false;
                ErrorLog("SatelliteCaptureProbe: Terrain restore guard failed; resources retained; restart diagnostic");
            }
            else Log("Terrain cache restored; helper identity and borrowed main LOD camera verified");
        }
    }
};
Ogre::Entity *ReadExisting(Town *t)
{
    __try
    {
        char *p = reinterpret_cast<char *>(t->distantTown);
        if (!p || *reinterpret_cast<Town **>(p) != t || *reinterpret_cast<unsigned *>(p + 8) != 3)
            return NULL;
        return *reinterpret_cast<Ogre::Entity **>(p + 16);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }
}
struct ProxyScope
{
    std::vector<std::pair<Ogre::Entity *, bool> > changed;
    ProxyScope()
    {
        changed.reserve(8);
    }
    void Apply()
    {
        if (!proxies)
            return;
        for (size_t i = 0; i < handles.size() && changed.size() < 8; ++i)
        {
            TownBase *b = handles[i].getTown();
            Town *t = b ? b->isTown() : NULL;
            if (!t)
                continue;
            Ogre::Entity *e = ReadExisting(t);
            if (!e || e->getVisible())
                continue;
            changed.push_back(std::make_pair(e, false));
            e->setVisible(true);
        }
        std::ostringstream s;
        s << "existing hidden native entities temporarily shown=" << changed.size()
          << "; no zone load/Town allocation";
        Log(s.str());
    }
    ~ProxyScope()
    {
        for (size_t i = 0; i < changed.size(); ++i)
            changed[i].first->setVisible(changed[i].second);
    }
};
void Aim(Ogre::Camera *c)
{
    const Ogre::Vector3 world(float(ProbePolicy::Centre(cx)), 4000.0f,
                              float(ProbePolicy::Centre(cz)));
    const Ogre::Vector3 p = world - renderer->getSceneRelativeOrigin();
    const Ogre::Quaternion q(Ogre::Degree(-90), Ogre::Vector3::UNIT_X);
    Ogre::Node *parent = c->getParentNode();
    c->setPosition(parent ? parent->convertWorldToLocalPosition(p) : p);
    c->setOrientation(parent ? parent->convertWorldToLocalOrientation(q) : q);
    c->setAutoAspectRatio(false);
    // Only remove camera culling thresholds for resources ALREADY resident in the scene.
    // This neither loads geometry nor activates NPC zones.
    c->setUseRenderingDistance(false);
    nativeApi.setMin(c,false);
    c->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
    c->setOrthoWindow(1024, 1024);
    c->setNearClipDistance(1);
    c->setFarClipDistance(10000);
}
void Save(const Ogre::TexturePtr &texture, const std::string &name)
{
    if (texture.isNull() || texture->getWidth() != Side || texture->getHeight() != Side)
        throw std::runtime_error("unexpected probe texture dimensions");
    std::vector<unsigned> px(Side * Side);
    Ogre::PixelBox box(Side, Side, 1, Ogre::PF_A8R8G8B8, &px[0]);
    texture->getBuffer()->blitToMemory(box);
    unsigned char header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    unsigned bytes = 54 + Side * Side * 4, off = 54, info = 40;
    int side = Side;
    unsigned short one = 1, bits = 32;
    std::memcpy(header + 2, &bytes, 4);
    std::memcpy(header + 10, &off, 4);
    std::memcpy(header + 14, &info, 4);
    std::memcpy(header + 18, &side, 4);
    std::memcpy(header + 22, &side, 4);
    std::memcpy(header + 26, &one, 2);
    std::memcpy(header + 28, &bits, 2);
    FILE *f = NULL;
    if (fopen_s(&f, (Folder() + name + ".bmp").c_str(), "wb") || !f)
        throw std::runtime_error("cannot write probe evidence");
    bool ok = fwrite(header, 1, 54, f) == 54;
    unsigned dark = 0;
    for (size_t i = 0; i < px.size(); ++i)
    {
        if ((px[i] & 0xFFFFFFu) == 0)
            ++dark;
        px[i] |= 0xFF000000u;
    }
    for (int y = Side - 1; y >= 0; --y)
        ok = ok && fwrite(&px[y * Side], 4, Side, f) == Side;
    ok = fclose(f) == 0 && ok;
    if (!ok)
        throw std::runtime_error("incomplete probe evidence");
    std::ostringstream s;
    s << name << " black pixels=" << dark << "/" << px.size();
    Log(s.str());
}
void PreparePrivateProgram(unsigned slot,const char *name,const std::string &source,
                           const char *entry,const char *target,Ogre::GpuProgramType type,
                           Ogre::HighLevelGpuProgramPtr &owned,bool compatibility=false)
{
    Ogre::HighLevelGpuProgramManager &programs=Ogre::HighLevelGpuProgramManager::getSingleton();
    Ogre::HighLevelGpuProgramPtr existing;
    if(programs.resourceExists(name))existing=programs.getByName(name);
    const ProbePolicy::FixedProgramCache::Choice choice=reusePrivateShaders?
        programCache.Inspect(positionDefinitionOwner,slot,existing.get(),source):
        (existing.isNull()?ProbePolicy::FixedProgramCache::Create:ProbePolicy::FixedProgramCache::Refuse);
    if(choice==ProbePolicy::FixedProgramCache::Refuse)
        throw std::runtime_error("private shader identity/source changed; restart diagnostic");
    if(choice==ProbePolicy::FixedProgramCache::Reuse)
    {
        owned=existing;
        if(!owned->isSupported()||owned->hasCompileError())throw std::runtime_error("cached private shader unsupported");
        Log(std::string("reused fixed private shader ")+name);return;
    }
    // Assign before compilation so any exception remains owned by Drop.
    owned=programs.createProgram(name,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"hlsl",type);
    owned->setSource(source);owned->setParameter("entry_point",entry);owned->setParameter("target",target);
    if(compatibility)
    {owned->setParameter("enable_backwards_compatibility","true");owned->setParameter("preprocessor_defines","DX11=1");}
    owned->load();
    if(!owned->isSupported()||owned->hasCompileError())throw std::runtime_error("private shader unsupported");
    if(reusePrivateShaders&&!programCache.Record(positionDefinitionOwner,slot,owned.get(),source))
        throw std::runtime_error("private shader cache record refused");
    Log(std::string("compiled fixed private shader ")+name);
}
void PreparePositionPass(Ogre::CompositorManager2 *manager, Ogre::SceneManager *scene,
                         Ogre::Camera *camera, const Ogre::TexturePtr &depth)
{
    if (camera->getProjectionType() != Ogre::PT_ORTHOGRAPHIC)
        throw std::runtime_error("position pass requires orthographic camera");
    if (positionDefinitionOwner && positionDefinitionOwner != manager)
        throw std::runtime_error("position definition manager changed; restart diagnostic");
    if (positionDefinitionsFailed)
        throw std::runtime_error("partial position definition refused; restart diagnostic");
    if (!positionDefinitionOwner)
    {
        if (manager->hasNodeDefinition(Ogre::IdString(PositionNodeName)) ||
            manager->hasWorkspaceDefinition(Ogre::IdString(PositionWorkspaceName)))
            throw std::runtime_error("private position definition name collision");
        positionDefinitionOwner = manager;
        positionDefinitionsFailed = true;
        Ogre::CompositorNodeDef *node = manager->addNodeDefinition(PositionNodeName);
        node->addTextureSourceName("rt_output", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);
        node->setNumTargetPass(1);
        Ogre::CompositorTargetDef *rt = node->addTargetPass("rt_output");
        rt->setNumPasses(1);
        Ogre::CompositorPassQuadDef *quad =
            static_cast<Ogre::CompositorPassQuadDef *>(rt->addPass(Ogre::PASS_QUAD));
        quad->mUseQuad = true;
        quad->mMaterialName = PositionMaterialName;
        positionNodeDefined = true;
        Ogre::CompositorWorkspaceDef *workspace =
            manager->addWorkspaceDefinition(Ogre::IdString(PositionWorkspaceName));
        workspace->connectOutput(Ogre::IdString(PositionNodeName), 0);
        positionWorkspaceDefined = true;
        positionDefinitionsFailed = false;
    }
    if (!positionNodeDefined || !positionWorkspaceDefined)
        throw std::runtime_error("position definition incomplete");
    Ogre::HighLevelGpuProgramManager &programs = Ogre::HighLevelGpuProgramManager::getSingleton();
    Ogre::MaterialManager &materials = Ogre::MaterialManager::getSingleton();
    Ogre::TextureManager &textures = Ogre::TextureManager::getSingleton();
    if (materials.resourceExists(PositionMaterialName) || textures.resourceExists(PositionTargetName))
        throw std::runtime_error("private position resource name collision");
    const Ogre::String &group = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
    PreparePrivateProgram(0,PositionVertexName,PositionShaderSource,"main_vs","vs_4_0",Ogre::GPT_VERTEX_PROGRAM,positionVertex);
    PreparePrivateProgram(1,PositionFragmentName,PositionShaderSource,"main_ps","ps_4_0",Ogre::GPT_FRAGMENT_PROGRAM,positionFragment);
    positionMaterial = materials.create(PositionMaterialName, group);
    positionMaterial->removeAllTechniques();
    Ogre::Pass *pass = positionMaterial->createTechnique()->createPass();
    pass->setLightingEnabled(false);
    pass->setDepthCheckEnabled(false);
    pass->setDepthWriteEnabled(false);
    pass->setCullingMode(Ogre::CULL_NONE);
    pass->setSceneBlending(Ogre::SBT_REPLACE);
    pass->setVertexProgram(PositionVertexName);
    pass->setFragmentProgram(PositionFragmentName);
    Ogre::GpuProgramParametersSharedPtr parameters = positionFragment->createParameters();
    parameters->setNamedConstant("captureWindow", Ogre::Vector4(camera->getOrthoWindowWidth(),
        camera->getOrthoWindowHeight(), camera->getFarClipDistance(), float(Side)));
    pass->setFragmentProgramParameters(parameters);
    Ogre::TextureUnitState *unit = pass->createTextureUnitState();
    unit->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);
    unit->setTexture(depth);
    unit->setTextureFiltering(Ogre::TFO_NONE);
    unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
    positionMaterial->load();
    if(rtvDebt.Active())rtvDebt.UncertainAllocation();
    positionTarget = textures.createManual(PositionTargetName, group, Ogre::TEX_TYPE_2D,
        Side, Side, 1, 0, Ogre::PF_FLOAT32_RGBA, Ogre::TU_RENDERTARGET);
    TrackPrivateTarget(positionTarget);
    positionWorkspace = manager->addWorkspace(scene, positionTarget->getBuffer()->getRenderTarget(),
        camera, Ogre::IdString(PositionWorkspaceName), false);
    if(rtvDebt.Active())rtvDebt.AllocationConfirmed();
}
void InspectPositionPass(Ogre::Camera *camera, const std::vector<float> &depth)
{
    if (!positionWorkspace || positionTarget.isNull())
        throw std::runtime_error("private position workspace missing");
    positionWorkspace->_update();
    std::vector<float> pixels(Side * Side * 4);
    Ogre::PixelBox box(Side, Side, 1, Ogre::PF_FLOAT32_RGBA, &pixels[0]);
    positionTarget->getBuffer()->blitToMemory(box);
    const Ogre::Matrix4 projection = camera->getProjectionMatrix();
    unsigned valid = 0, invalid = 0, nonfinite = 0;
    double maxNdc = 0, maxRadialError = 0;
    for (unsigned y = 0; y < Side; ++y)
        for (unsigned x = 0; x < Side; ++x)
        {
            const unsigned index = y * Side + x;
            const float *p = &pixels[index * 4];
            if (!_finite(p[0]) || !_finite(p[1]) || !_finite(p[2]) || !_finite(p[3]))
            { ++nonfinite; continue; }
            if (p[3] < 0.5f) { ++invalid; continue; }
            ++valid;
            double clip[4] = {0,0,0,0};
            for (unsigned row = 0; row < 4; ++row)
                clip[row] = projection[row][0]*double(p[0]) + projection[row][1]*double(p[1]) +
                            projection[row][2]*double(p[2]) + projection[row][3];
            if (!_finite(clip[3]) || std::fabs(clip[3]) < 1e-12)
            { ++nonfinite; continue; }
            maxNdc = (std::max)(maxNdc, std::fabs(clip[0]/clip[3] - ((x+0.5)*2/Side-1)));
            maxNdc = (std::max)(maxNdc, std::fabs(clip[1]/clip[3] - (1-(y+0.5)*2/Side)));
            const double radius = std::sqrt(double(p[0])*p[0]+double(p[1])*p[1]+double(p[2])*p[2]);
            maxRadialError = (std::max)(maxRadialError,
                std::fabs(radius-double(depth[index])*camera->getFarClipDistance()));
        }
    std::ostringstream s;
    s.precision(9);
    s << "private ortho position valid=" << valid << " invalid=" << invalid
      << " nonfinite=" << nonfinite << " max NDC reprojection error=" << maxNdc
      << " max radial error metres=" << maxRadialError;
    Log(s.str());
}
std::string LoadSunSource()
{
    const std::string path=Folder()+"SunShaderOverride.hlsl";
    HANDLE h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(h==INVALID_HANDLE_VALUE)
    {
        const DWORD error=GetLastError();
        if(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND)
        { Log("using embedded sun shader");return SunShaderSource; }
        throw std::runtime_error("sun override unavailable; no fallback on read error");
    }
    struct LockedFile
    {
        HANDLE handle;
        LockedFile(HANDLE value):handle(value){}
        ~LockedFile(){CloseHandle(handle);}
    } file(h);
    LARGE_INTEGER size;
    if(!GetFileSizeEx(h,&size) || size.QuadPart<=0 || size.QuadPart>32768)
        throw std::runtime_error("sun override size outside bounds");
    std::vector<char> bytes(static_cast<size_t>(size.QuadPart));DWORD read=0;
    if(!ReadFile(h,&bytes[0],DWORD(bytes.size()),&read,NULL) || read!=bytes.size() ||
       !ProbePolicy::ValidShaderSource(&bytes[0],bytes.size()))
        throw std::runtime_error("sun override invalid or incomplete");
    // FILE_SHARE_READ keeps normal writes/replacement denied through read and hash.
    const std::string hash=FileHash(path.c_str());
    if(hash.size()!=64)throw std::runtime_error("sun override hash unavailable");
    Log("using sun override SHA256="+hash);
    return std::string(&bytes[0],bytes.size());
}
Ogre::Vector4 SharedVector(const char *set, const char *name, unsigned count)
{
    Ogre::GpuSharedParametersPtr p = Ogre::GpuProgramManager::getSingleton().getSharedParameters(set);
    const Ogre::GpuConstantDefinition &d = p->getConstantDefinition(name);
    if (d.arraySize != 1 || d.constType != (count == 1 ? Ogre::GCT_FLOAT1 : (count == 3 ? Ogre::GCT_FLOAT3 : Ogre::GCT_FLOAT4)))
        throw std::runtime_error("sun shared parameter type mismatch");
    const float *v = static_cast<const Ogre::GpuSharedParameters *>(p.get())->getFloatPointer(d.physicalIndex);
    Ogre::Vector4 result(0,0,0,0);
    for (unsigned i=0;i<count;++i)
    {
        if (!_finite(v[i])) throw std::runtime_error("sun shared parameter nonfinite");
        result[i] = v[i];
    }
    return result;
}
Ogre::Vector3 AsVector3(const Ogre::Vector4 &v) { return Ogre::Vector3(v.x,v.y,v.z); }
struct LightingSnapshot
{
    Ogre::Vector3 reference,direction,offset;
    Ogre::Vector4 sun,world,environment,ambient;
    bool preview;
    float key,luminance;
    LightingSnapshot():preview(false),key(0),luminance(0){}
};
LightingSnapshot isolatedLighting;
LightingSnapshot ReadLightingSnapshot(bool preview,Ogre::CompositorWorkspace *mainWorkspace)
{
    LightingSnapshot value;
    // Copy the reference camera and its paired ambient UV transform together.
    // No shared-parameter pointers or main texture references escape this call.
    value.reference=AsVector3(SharedVector("SharedDeferredParams","camera",3));
    value.direction=AsVector3(SharedVector("SharedSkyParams","sunDirection",3));
    value.sun=SharedVector("SharedSkyParams","sunColour",4);
    value.world=SharedVector("AmbientParams","worldSize",4);
    value.offset=AsVector3(SharedVector("SharedWaterParams","worldOffset",3));
    value.environment=SharedVector("AmbientParams","envColour",4);
    value.ambient=SharedVector("AmbientParams","ambientParams",4);
    if(value.direction.squaredLength()<1e-8f)throw std::runtime_error("sun direction invalid");
    if(preview)
    {
        if(!mainWorkspace)throw std::runtime_error("native exposure main workspace missing");
        typedef char IdSize[(sizeof(Ogre::IdString)==4)?1:-1];
        typedef char PtrSize[(sizeof(Ogre::TexturePtr)==16)?1:-1];
        typedef Ogre::CompositorNode *(__fastcall *Find)(const Ogre::CompositorWorkspace*,Ogre::IdString,bool);
        typedef Ogre::TexturePtr *(__fastcall *Texture)(const Ogre::CompositorNode*,Ogre::TexturePtr*,Ogre::IdString,size_t);
        Find find=reinterpret_cast<Find>(AbiExport(nativeApi.ogre,NativeFindNodeName,NativeFindNodeRva));
        Texture texture=reinterpret_cast<Texture>(AbiExport(nativeApi.ogre,NativeNodeTextureName,NativeNodeTextureRva));
        Ogre::CompositorNode *node=find(mainWorkspace,Ogre::IdString("Resolve_HDR"),false);
        if(!node)throw std::runtime_error("native exposure Resolve_HDR node missing");
        Ogre::TexturePtr adapted; // Native aggregate ABI initializes an EMPTY output only.
        if(texture(node,&adapted,Ogre::IdString("rt_adaptedLum"),0)!=&adapted||adapted.isNull()||
           adapted->getWidth()!=1||adapted->getHeight()!=1||adapted->getDepth()!=1||adapted->getFormat()!=Ogre::PF_FLOAT32_R)
            throw std::runtime_error("native exposure texture shape/format differs");
        float logLuminance=0;
        Ogre::PixelBox pixel(1,1,1,Ogre::PF_FLOAT32_R,&logLuminance);
        Ogre::HardwarePixelBufferSharedPtr buffer=adapted->getBuffer(0,0);
        buffer->blitToMemory(Ogre::Box(0,0,0,1,1,1),pixel);
        value.key=SharedVector("HDRSettings","EXPOSURE_KEY",1).x;
        if(!ExposurePolicy::Decode(value.key,logLuminance,value.luminance))
            throw std::runtime_error("native exposure key/luminance invalid");
        value.preview=true;
        std::ostringstream message;message.precision(9);
        message<<"native exposure frozen key="<<value.key<<" adaptedLog="<<logLuminance<<" luminance="<<value.luminance
               <<"; installed ToneMap/gamma settings, no bloom/shadows/fog/water";
        Log(message.str());
    }
    return value;
}
void CopyPrivateTechniqueConfiguration(const Ogre::MaterialPtr &source,const Ogre::MaterialPtr &destination)
{
    if(source.isNull()||destination.isNull()||source==destination||source->getNumTechniques()!=1||
       source->getTechnique(0)->getNumPasses()!=1)
        throw std::runtime_error("private material configuration requires one independent technique/pass");
    // Native Material::clone/operator= also copy Resource loaded-state/size without
    // crediting its manager. Copy only technique configuration into a fresh owned
    // Resource; Technique assignment retains its fresh parent and deep-copies Pass.
    typedef Ogre::Technique *(__fastcall *Copy)(Ogre::Technique*,const Ogre::Technique*);
    typedef Ogre::Material *(__fastcall *Parent)(const Ogre::Technique*);
    typedef bool (__fastcall *ReadFlag)(const Ogre::Material*);
    typedef void (__fastcall *WriteFlag)(Ogre::Material*,bool);
    Copy copy=reinterpret_cast<Copy>(AbiExport(nativeApi.ogre,PrivateTechniqueCopyName,PrivateTechniqueCopyRva));
    Parent parent=reinterpret_cast<Parent>(AbiExport(nativeApi.ogre,PrivateTechniqueParentName,PrivateTechniqueParentRva));
    ReadFlag receive=reinterpret_cast<ReadFlag>(AbiExport(nativeApi.ogre,PrivateReceiveShadowsName,PrivateReceiveShadowsRva));
    ReadFlag transparent=reinterpret_cast<ReadFlag>(AbiExport(nativeApi.ogre,PrivateTransparencyShadowsName,PrivateTransparencyShadowsRva));
    WriteFlag setReceive=reinterpret_cast<WriteFlag>(AbiExport(nativeApi.ogre,PrivateSetReceiveShadowsName,PrivateSetReceiveShadowsRva));
    WriteFlag setTransparent=reinterpret_cast<WriteFlag>(AbiExport(nativeApi.ogre,PrivateSetTransparencyShadowsName,PrivateSetTransparencyShadowsRva));
    setReceive(destination.get(),receive(source.get()));
    setTransparent(destination.get(),transparent(source.get()));
    destination->removeAllTechniques();
    Ogre::Technique *fresh=destination->createTechnique();
    if(parent(fresh)!=destination.get()||copy(fresh,source->getTechnique(0))!=fresh||parent(fresh)!=destination.get())
        throw std::runtime_error("private Technique assignment ABI differs");
}
void PrepareSunPass(Ogre::CompositorManager2 *manager, Ogre::SceneManager *scene,
                    Ogre::Camera *camera, const Ogre::CompositorChannel &gb)
{
    if (manager != positionDefinitionOwner || sunDefinitionsFailed)
        throw std::runtime_error("sun definition ownership incomplete; restart diagnostic");
    if (!sunDefinitionsReady)
    {
        if (manager->hasNodeDefinition(Ogre::IdString(SunNodeName)) ||
            manager->hasWorkspaceDefinition(Ogre::IdString(SunWorkspaceName)))
            throw std::runtime_error("private sun definition name collision");
        sunDefinitionsFailed = true;
        Ogre::CompositorNodeDef *node = manager->addNodeDefinition(SunNodeName);
        node->addTextureSourceName("rt_output",0,Ogre::TextureDefinitionBase::TEXTURE_INPUT);
        node->setNumTargetPass(1);
        Ogre::CompositorTargetDef *rt = node->addTargetPass("rt_output");
        rt->setNumPasses(1);
        Ogre::CompositorPassQuadDef *quad =
            static_cast<Ogre::CompositorPassQuadDef *>(rt->addPass(Ogre::PASS_QUAD));
        quad->mUseQuad = true;
        quad->mMaterialName = SunMaterialName;
        manager->addWorkspaceDefinition(Ogre::IdString(SunWorkspaceName))->connectOutput(Ogre::IdString(SunNodeName),0);
        sunDefinitionsReady = true;
        sunDefinitionsFailed = false;
    }
    Ogre::MaterialManager &materials = Ogre::MaterialManager::getSingleton();
    Ogre::HighLevelGpuProgramManager &programs = Ogre::HighLevelGpuProgramManager::getSingleton();
    Ogre::TextureManager &textures = Ogre::TextureManager::getSingleton();
    if (materials.resourceExists(SunMaterialName) || textures.resourceExists(SunTargetName))
        throw std::runtime_error("private sun resource name collision");
    Ogre::MaterialPtr source = materials.getByName("Main_Lighting_NoShadow");
    if (source.isNull()) throw std::runtime_error("native no-shadow material missing");
    PreparePrivateProgram(2,SunFragmentName,LoadSunSource(),"main_ps","ps_4_0",Ogre::GPT_FRAGMENT_PROGRAM,sunFragment,true);
    // Clone only resource configuration. Read installed native ambient/IBL textures;
    // replace every GPU parameter binding and all compositor texture bindings privately.
    sunMaterial = materials.create(SunMaterialName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    CopyPrivateTechniqueConfiguration(source,sunMaterial);
    if (sunMaterial->getNumTechniques()!=1 || sunMaterial->getTechnique(0)->getNumPasses()!=1)
        throw std::runtime_error("unexpected native no-shadow material structure");
    Ogre::Pass *pass = sunMaterial->getTechnique(0)->getPass(0);
    if (pass->getNumTextureUnitStates()!=6)
        throw std::runtime_error("unexpected native no-shadow texture layout");
    pass->setLightingEnabled(false);
    pass->setDepthCheckEnabled(false);
    pass->setDepthWriteEnabled(false);
    pass->setVertexProgram(PositionVertexName);
    pass->setFragmentProgram(SunFragmentName);
    for (unsigned i=0;i<3;++i)
    {
        Ogre::TextureUnitState *unit = pass->getTextureUnitState(i);
        unit->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);
        unit->setTexture(gb.textures[i]);
        unit->setTextureFiltering(Ogre::TFO_NONE);
    }
    sunMaterial->load();
    if(rtvDebt.Active())rtvDebt.UncertainAllocation();
    sunTarget = textures.createManual(SunTargetName,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,
        Side,Side,1,0,Ogre::PF_FLOAT32_RGBA,Ogre::TU_RENDERTARGET);
    TrackPrivateTarget(sunTarget);
    sunWorkspace = manager->addWorkspace(scene,sunTarget->getBuffer()->getRenderTarget(),camera,
        Ogre::IdString(SunWorkspaceName),false);
    if(rtvDebt.Active())rtvDebt.AllocationConfirmed();
}
void InspectSunPass(Ogre::Camera *camera, const std::string &prefix,bool absoluteWorld=false,const LightingSnapshot *frozen=NULL)
{
    if (!sunWorkspace || sunMaterial.isNull()) throw std::runtime_error("sun workspace missing");
    Ogre::GpuProgramParametersSharedPtr p = sunMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
    Ogre::Quaternion basis = camera->getDerivedOrientation();
    basis.normalise();
    const LightingSnapshot snapshot=frozen?*frozen:ReadLightingSnapshot(false,NULL);
    const Ogre::Vector3 reference = snapshot.reference;
    const Ogre::Vector3 delta = camera->getDerivedPosition()-reference;
    const Ogre::Vector3 direction = snapshot.direction;
    if (direction.squaredLength()<1e-8f) throw std::runtime_error("sun direction invalid");
    const Ogre::Vector4 sun = snapshot.sun,world = snapshot.world;
    const Ogre::Vector3 offset = absoluteWorld ? Ogre::Vector3::ZERO : snapshot.offset;
    p->clearAutoConstants();
    p->removeAllSharedParameters();
    p->setNamedConstant("captureWindow",Ogre::Vector4(camera->getOrthoWindowWidth(),camera->getOrthoWindowHeight(),camera->getFarClipDistance(),float(Side)));
    p->setNamedConstant("captureRight",basis.xAxis());
    p->setNamedConstant("captureUp",basis.yAxis());
    p->setNamedConstant("captureBack",basis.zAxis());
    p->setNamedConstant("captureCameraDelta",delta);
    p->setNamedConstant("sunDirection",direction);
    p->setNamedConstant("sunColour",sun);
    p->setNamedConstant("envColour",snapshot.environment);
    p->setNamedConstant("ambientParams",snapshot.ambient);
    p->setNamedConstant("worldSize",world);
    p->setNamedConstant("worldOffset",offset);
    std::ostringstream state;
    state.precision(9);
    state << "private sun snapshot reference="<<reference.x<<','<<reference.y<<','<<reference.z
          <<" capture delta="<<delta.x<<','<<delta.y<<','<<delta.z
          <<" worldOffset="<<offset.x<<','<<offset.y<<','<<offset.z
          <<" worldSize="<<world.x<<','<<world.y<<','<<world.z<<','<<world.w
          <<" sunDirection="<<direction.x<<','<<direction.y<<','<<direction.z
          <<" sunColour="<<sun.x<<','<<sun.y<<','<<sun.z<<','<<sun.w;
    Log(state.str());
    p->setNamedConstant("captureExposure",Ogre::Vector2(0,0));
    sunWorkspace->_update();
    std::vector<float> pixels(Side*Side*4);
    Ogre::PixelBox box(Side,Side,1,Ogre::PF_FLOAT32_RGBA,&pixels[0]);
    sunTarget->getBuffer()->blitToMemory(box);
    unsigned valid=0,nonfinite=0,belowZero=0,aboveOne=0;
    double total=0,maximum=0,channelSum[3]={0,0,0},channelMin[3]={1e30,1e30,1e30},channelMax[3]={-1e30,-1e30,-1e30};
    for (unsigned i=0;i<Side*Side;++i)
    {
        const float *v=&pixels[i*4];
        if (!_finite(v[0])||!_finite(v[1])||!_finite(v[2])||!_finite(v[3])) {++nonfinite;continue;}
        if (v[3]<.5f) continue;
        ++valid;
        bool low=false,high=false;
        for(unsigned component=0;component<3;++component)
        {
            channelSum[component]+=v[component];
            channelMin[component]=(std::min)(channelMin[component],double(v[component]));
            channelMax[component]=(std::max)(channelMax[component],double(v[component]));
            low=low||v[component]<0;high=high||v[component]>1;
        }
        if(low)++belowZero;if(high)++aboveOne;
        const double luminance=.2126*v[0]+.7152*v[1]+.0722*v[2];
        total+=luminance;maximum=(std::max)(maximum,luminance);
    }
    std::ostringstream s;s.precision(9);
    s<<"private sun linear HDR valid="<<valid<<" nonfinite="<<nonfinite
     <<" mean luminance="<<(valid?total/valid:0)<<" max luminance="<<maximum
     <<"; no shadows/fog/local lights/water or tonemapping";
    Log(s.str());
    std::ostringstream channels;channels.precision(9);
    channels<<"private sun RGB mean="<<(valid?channelSum[0]/valid:0)<<','<<(valid?channelSum[1]/valid:0)<<','<<(valid?channelSum[2]/valid:0)
            <<" min="<<channelMin[0]<<','<<channelMin[1]<<','<<channelMin[2]
            <<" max="<<channelMax[0]<<','<<channelMax[1]<<','<<channelMax[2]
            <<" pixels below0="<<belowZero<<" above1="<<aboveOne;
    Log(channels.str());
    if (nonfinite) throw std::runtime_error("private sun nonfinite output; no BMP saved");
    Save(sunTarget,prefix+"_sun_noShadow_linear_clamped");
    if(snapshot.preview)
    {
        p->setNamedConstant("captureExposure",Ogre::Vector2(snapshot.key,snapshot.luminance));
        sunWorkspace->_update();
        sunTarget->getBuffer()->blitToMemory(box);
        unsigned clipped=0,invalid=0;
        for(unsigned i=0;i<Side*Side;++i)
        {
            const float *v=&pixels[i*4];
            if(!_finite(v[0])||!_finite(v[1])||!_finite(v[2])||!_finite(v[3])){++invalid;continue;}
            if(v[3]>.5f&&(v[0]>1||v[1]>1||v[2]>1))++clipped;
        }
        std::ostringstream message;message<<"private native exposure preview nonfinite="<<invalid<<" clipped pixels="<<clipped<<'/'<<valid;
        Log(message.str());
        if(invalid)throw std::runtime_error("private exposure nonfinite output; no BMP saved");
        Save(sunTarget,prefix+"_sun_noShadow_native_exposure");
    }
}
#include "PrivateBiomeReference.h"
#include "PrivatePaintedReference.h"
#include "PrivateGeography.h"
#include "TerrainExperiment.h"
#include "AtlasRun.h"
DWORD lastAtlasPoll=0;
// Explicit exported calls avoid stale SDK inline fields. Diagnostic snapshots only;
// manager-reported bytes exclude native/driver caches. Never evict game resources.
void LogResourceTelemetry(const char *phase)
{
    try
    {
        if(!au||!au->render||!au->render->root||!RenderAbiMatches())return;
        typedef size_t (__fastcall *Bytes)(const Ogre::ResourceManager*);
        typedef bool (__fastcall *HasScene)(const Ogre::Root*,const Ogre::String&);
        typedef size_t (__fastcall *Workspaces)(const Ogre::CompositorManager2*);
        Bytes bytes=reinterpret_cast<Bytes>(AbiExport(nativeApi.ogre,NativeResourceBytesName,NativeResourceBytesRva));
        HasScene has=reinterpret_cast<HasScene>(AbiExport(nativeApi.ogre,HasPrivateSceneName,HasPrivateSceneRva));
        Workspaces workspaces=reinterpret_cast<Workspaces>(AbiExport(nativeApi.ogre,NativeWorkspaceCountName,NativeWorkspaceCountRva));
        Ogre::TextureManager &textures=Ogre::TextureManager::getSingleton();
        Ogre::MaterialManager &materials=Ogre::MaterialManager::getSingleton();
        Ogre::HighLevelGpuProgramManager &high=Ogre::HighLevelGpuProgramManager::getSingleton();
        Ogre::GpuProgramManager &low=Ogre::GpuProgramManager::getSingleton();
        unsigned names=0;
        names+=textures.resourceExists(PositionTargetName);names+=textures.resourceExists(SunTargetName);
        names+=textures.resourceExists("LEOPARD_PrivateTerrainOutput");
        names+=textures.resourceExists("LEOPARD_PrivateBiomeReferenceDiffuse");
        names+=textures.resourceExists("LEOPARD_PrivateBiomeReferenceNormals");
        names+=textures.resourceExists(PrivatePaintedReference::Name(0));
        names+=textures.resourceExists(PrivatePaintedReference::Name(1));
        for(unsigned bank=0;bank<5;++bank)names+=textures.resourceExists(PrivateGeography::Name(bank));
        names+=materials.resourceExists(PositionMaterialName);names+=materials.resourceExists(SunMaterialName);
        names+=materials.resourceExists("LEOPARD_PrivateTerrainMaterial");
        names+=high.resourceExists(PositionVertexName);names+=high.resourceExists(PositionFragmentName);names+=high.resourceExists(SunFragmentName);
        names+=high.resourceExists(PrivatePaintedReference::ProgramName());
        names+=high.resourceExists(PrivateGeography::ProgramName());
        names+=high.resourceExists(PrivateGeography::VertexName());
        std::ostringstream s;s<<"resource telemetry "<<phase
          <<" scene="<<has(au->render->root,"LEOPARD_PrivateTerrainScene")<<" privateNames="<<names
          <<" workspaces="<<workspaces(au->render->root->getCompositorManager2())
          <<" textureBytes="<<bytes(&textures)<<" materialBytes="<<bytes(&materials)
          <<" highProgramBytes="<<bytes(&high)<<" lowProgramBytes="<<bytes(&low)
          <<" cachedPrograms="<<programCache.Count()+PaintedReference().cache.Count()+GeographyReference().cache.Count()<<"; telemetry only; no resource eviction";Log(s.str());
    }
    catch(...){Log("resource telemetry unavailable; no resource eviction attempted");}
}
void Drop()
{
    if (!OnMain())
        return;
    const bool privateRun=terrainExperiment.root!=NULL;
    if(!restorationSafe)
    {
        pending=false;cleanup=false;chainSafe=false;
        return;
    }
    NativeTerrainCache::BloodState blood;
    if(own && (!au || !ReadTerrainBlood(au->terrain,blood) || blood.camera==own))
    {
        restorationSafe=false;chainSafe=false;pending=false;cleanup=false;
        ErrorLog("SatelliteCaptureProbe: unsafe borrowed camera at cleanup; resources retained");
        return;
    }
    Renderer *current = au ? au->render : NULL;
    Ogre::CompositorManager2 *manager =
        current && current->root ? current->root->getCompositorManager2() : NULL;
    if (!ProbePolicy::SafeAuxiliaryCleanup(positionWorkspace != NULL || sunWorkspace != NULL, positionDefinitionOwner, manager))
    {
        // An unknown manager is not proof that the workspace/its raw Pass* died.
        // Retain this single bounded resource set until process exit; no more shots.
        pending = false;
        cleanup = false;
        chainSafe = false;
        Log("auxiliary workspace ownership unknown; resources retained, restart diagnostic");
        return;
    }
    bool detached = false;
    if (manager && ManagerAccess::Contains(manager, gameWorkspace) &&
        gameWorkspace->getListener() == ourListener)
    {
        gameWorkspace->setListener(previous);
        detached = true;
    }
    else if (manager && ManagerAccess::Contains(manager, gameWorkspace))
    {
        chainSafe = false;
        Log("external listener wrapped probe; forwarding retained and further requests disabled");
    }
    else if(gameWorkspace)
    {
        chainSafe=false;
        Log("main workspace membership unknown at cleanup; listener forwarding retained, restart required before another request");
    }
    gameWorkspace = NULL;
    // Preserve forwarding if an external listener wrapped ours in the intervening frame.
    if (detached)
        previous = NULL;
    if (manager && ManagerAccess::Contains(manager, sunWorkspace)) manager->removeWorkspace(sunWorkspace);
    sunWorkspace=NULL;
    if (!sunMaterial.isNull())
    {
        Ogre::MaterialManager::getSingleton().remove(SunMaterialName);sunMaterial.setNull();
    }
    if (!sunTarget.isNull())
    {
        Ogre::TextureManager::getSingleton().remove(SunTargetName);sunTarget.setNull();
    }
    if (!sunFragment.isNull())
    {
        if(!programCache.Owns(2,sunFragment.get()))Ogre::HighLevelGpuProgramManager::getSingleton().remove(SunFragmentName);
        sunFragment.setNull();
    }
    if (manager && ManagerAccess::Contains(manager, positionWorkspace))
        manager->removeWorkspace(positionWorkspace);
    positionWorkspace = NULL;
    if (!positionMaterial.isNull())
    {
        Ogre::MaterialManager::getSingleton().remove(PositionMaterialName);
        positionMaterial.setNull();
    }
    if (!positionTarget.isNull())
    {
        Ogre::TextureManager::getSingleton().remove(PositionTargetName);
        positionTarget.setNull();
    }
    if (!positionVertex.isNull())
    {
        if(!programCache.Owns(0,positionVertex.get()))Ogre::HighLevelGpuProgramManager::getSingleton().remove(PositionVertexName);
        positionVertex.setNull();
    }
    if (!positionFragment.isNull())
    {
        if(!programCache.Owns(1,positionFragment.get()))Ogre::HighLevelGpuProgramManager::getSingleton().remove(PositionFragmentName);
        positionFragment.setNull();
    }
    if (manager && ManagerAccess::Contains(manager, shot))
        manager->removeWorkspace(shot);
    shot = NULL;
    if(terrainExperiment.root)
    {
        try{terrainExperiment.Destroy(current?current->root:NULL);}
        catch(...)
        {
            restorationSafe=false;chainSafe=false;pending=false;cleanup=false;
            ErrorLog("SatelliteCaptureProbe: private Terrain cleanup refused; retained until process exit");return;
        }
    }
    if (current && current->scene == shotScene)
    {
        if (own && shotScene->findCameraNoThrow("LEOPARD_PrivateCaptureCamera") == own)
            shotScene->destroyCamera(own);
        if (ownNode)
        {
            try
            {
                if (shotScene->getSceneNode(ownNodeId) == ownNode)
                    shotScene->destroySceneNode(ownNode);
            }
            catch (...)
            {
                Log("probe node already removed by scene lifecycle");
            }
        }
    }
    own = NULL;
    ownNode = NULL;
    shotScene = NULL;
    if (!target.isNull())
    {
        std::string n = target->getName();
        target.setNull();
        Ogre::TextureManager::getSingleton().remove(n);
    }
    renderer = NULL;
    handles.clear();
    cleanup = false;
    if(rtvDebt.Active())
    {
        const unsigned debts=rtvDebt.Count();PrivateViewCleanup ownerProof;
        const bool gone=!shot&&!positionWorkspace&&!sunWorkspace&&!terrainExperiment.workspace &&
            !terrainExperiment.root&&target.isNull()&&positionTarget.isNull()&&sunTarget.isNull() &&
            terrainExperiment.output.isNull()&&terrainExperiment.material.isNull()&&
            positionMaterial.isNull()&&sunMaterial.isNull();
        if(!rtvDebt.Clean(ownerProof,gone))
        {
            chainSafe=false;pending=false;
            ErrorLog("SatelliteCaptureProbe: owned RTV lifetime proof failed; one set quarantined, further captures disabled");
        }
        else
        {
            std::ostringstream released;released<<"owned RTV lifetime proof complete; creation debts="<<debts;Log(released.str());
        }
    }
    if(privateRun)LogResourceTelemetry("after-private-terrain");
}
bool FixedViewerOff()
{
    return fixedGsmStartupOff&&!fixedGsmIni.empty()&&
        GetPrivateProfileIntA("GlobalSatelliteMap","Enabled",1,fixedGsmIni.c_str())==0;
}
bool FixedNoViewerTextures()
{
    Ogre::TextureManager*tm=Ogre::TextureManager::getSingletonPtr();if(!tm)return false;
    Ogre::ResourceManager::ResourceMapIterator it=tm->getResourceIterator();unsigned count=0;
    while(it.hasMoreElements())
    {
        if(++count>32768)return false;
        Ogre::ResourcePtr r=it.getNext();if(r.isNull())return false;
        if(r->getGroup()=="LEOPARD_GlobalSatelliteMap_Development"||
           r->getName().find("GlobalSatelliteMap_")==0)return false;
    }
    return true;
}
bool FixedContext(Ogre::CompositorWorkspace*w,bool visible)
{
    if(!profileOK||!OnMain()||!LoadedGuard()||!RenderAbiMatches()||!ou||ou!=requestedWorld||
       !au||!renderer||au->render!=renderer||!restorationSafe||!chainSafe||!w||w!=gameWorkspace)return false;
    if(renderer->workspace!=w)return false;
    SaveFileSystem*fs=SaveFileSystem::getSingleton();
    if(!fs||fs->busy()||!visible||!fixedGsmStartupOff||!FixedViewerOff())return false;
    return FixedTerrainControl::Guard(true,true,FixedNoViewerTextures(),true,true,true,true,true);
}
void FixedDevice(Ogre::CompositorWorkspace*w,ID3D11Device**out)
{
    const Ogre::CompositorChannel&gb=w->getGlobalTexture("global_gbuffer");
    if(gb.textures.size()!=3||gb.textures[0].isNull()||!IsPrivateArrayTexture(gb.textures[0].get()))
        throw std::runtime_error("fixed control main GBuffer/device profile unavailable");
    typedef ID3D11ShaderResourceView*(__fastcall*Getter)(Ogre::Texture*);
    ID3D11ShaderResourceView*v=reinterpret_cast<Getter>(reinterpret_cast<BYTE*>(nativeApi.d3d11)+PrivateArraySrvGetterRva)(gb.textures[0].get());
    if(!v)throw std::runtime_error("fixed control main GBuffer SRV unavailable");v->GetDevice(out);
    if(!*out)throw std::runtime_error("fixed control main D3D device unavailable");
}
SourceReceipts::Scope WorldFrameNow()
{
    SourceReceipts::Scope now=worldFrameJob.scope;
    now.thread=GetCurrentThreadId();now.world=reinterpret_cast<SourceReceipts::U64>(ou);
    if(InterlockedCompareExchange(&worldFrameLoadGeneration,0,0)!=worldFrameCapturedLoad)now.sourceEpoch^=1ULL<<63;
    return now;
}
void WorldFrameFinished(int result)
{
    if(!result)return;
    std::ostringstream report;report<<"world frame token="<<worldFrameToken<<" result="<<result<<" hr=0x"<<std::hex<<unsigned(worldFrame->Error())<<std::dec<<" quarantined="<<worldFrame->Quarantined()<<" directory="<<worldFrame->Output()<<"; owned original source frame, no atlas publication";
    if(result<0)ErrorLog("SatelliteCaptureProbe: "+report.str());else Log(report.str());
    cleanup=true;
}
void PrepareWorldFrame(unsigned token)
{
    const unsigned windowCase=GetPrivateProfileIntA("Probe","WorldFrameCase",0,requestIni.c_str());
    if(windowCase>1)throw std::runtime_error("world frame case outside explicit0/1 control");
    worldFrameCapturedLoad=InterlockedCompareExchange(&worldFrameLoadGeneration,0,0);
    char executable[MAX_PATH]={0};DWORD n=GetModuleFileNameA(NULL,executable,MAX_PATH);
    if(!n||n>=MAX_PATH)throw std::runtime_error("world frame executable path unavailable");
    std::string root(executable,n);root=root.substr(0,root.find_last_of("/\\"));
    if(root.size()>=9&&!_stricmp(root.substr(root.size()-9).c_str(),"RE_Kenshi"))root=root.substr(0,root.find_last_of("/\\"));
    PrivateGeography geography;geography.selectedRegion=&SatelliteRegion::Data.regions[27];geography.ParametersRegion(root,false);
    WorldStream::Job&j=worldFrameJob;std::memset(&j,0,sizeof(j));if(++worldFrameEpoch==0)++worldFrameEpoch;
    j.scope.thread=GetCurrentThreadId();j.scope.world=reinterpret_cast<SourceReceipts::U64>(ou);j.scope.sourceEpoch=(SourceReceipts::U64(GetCurrentProcessId())<<32)|worldFrameEpoch;j.scope.job=(3ULL<<32)|token;
    j.frame.source.value=j.scope.sourceEpoch;j.frame.source.profile=WorldBank::PinnedBaseSnapshot;j.frame.generation=j.scope.job;j.frame.materialGeneration=(2ULL<<32)|token;
    j.frame.region=27;j.frame.cell=10;j.frame.side=512;j.frame.x=j.frame.z=windowCase?-13568.0f:-13824.0f;j.frame.spacing=1;j.frame.cameraY=10000;j.frame.farClip=20000;j.frame.waterHeight=-10000;
    j.grid.x=j.grid.z=7360;j.grid.columns=j.grid.rows=2;j.grid.sourceEpoch=j.scope.sourceEpoch;
    j.material.source=j.frame.source;j.material.generation=j.frame.materialGeneration;j.material.region=27;
    std::memcpy(j.material.parameters,geography.parameters,sizeof(j.material.parameters));std::memcpy(j.material.distortion,geography.distortion,sizeof(j.material.distortion));
    j.shaders.vertex.data=CompactMaterial::VertexShader;j.shaders.vertex.size=sizeof(CompactMaterial::VertexShader);j.shaders.partial.data=StagedMaterial::PartialShader;j.shaders.partial.size=sizeof(StagedMaterial::PartialShader);j.shaders.tail.data=StagedMaterial::TailShader;j.shaders.tail.size=sizeof(StagedMaterial::TailShader);
    std::string path=root+"/data/newland/land/fullmap.tif";if(path.size()>=259)throw std::runtime_error("world frame height path too long");
    for(size_t i=0;i<path.size();++i){if(static_cast<unsigned char>(path[i])>=128)throw std::runtime_error("world frame non-ASCII path unsupported");j.heightPath[i]=static_cast<unsigned char>(path[i]);}
    WorldMaterial::Planner planner;if(!planner.Init(j.scope.sourceEpoch)||!planner.Build(27,10,j.scope.sourceEpoch,0,j.plan))throw std::runtime_error("world frame plan refused");
    if(!worldFrame)worldFrame=new WorldFrameControl::Controller<WorldNative::Provider>;
    char suffix[100];std::sprintf(suffix,"world-frame-%u-%u-%u",GetCurrentProcessId(),token,GetTickCount());
    HRESULT hr=worldFrame->Begin(j,Folder()+suffix);if(FAILED(hr)){std::ostringstream e;e<<"world frame native input admission hr=0x"<<std::hex<<unsigned(hr);throw std::runtime_error(e.str());}
    worldFrameToken=token;worldFrameStop=false;
}
#include "WorldBuildMain.inl"
struct ProbeListener : Ogre::CompositorWorkspaceListener
{
    virtual void passPreExecute(Ogre::CompositorPass *p)
    {
        if (previous)
            previous->passPreExecute(p);
    }
    virtual void workspacePreUpdate(Ogre::CompositorWorkspace *w)
    {
        Ogre::CompositorWorkspaceListener *next = previous;
        if(worldBuildEnabled&&OnMain()&&worldBuild&&worldBuild->Active()&&!worldBuild->Quarantined()&&!inside)
        {
            inside=true;
            try{bool admitted=!worldBuildStop&&FixedContext(w,true)&&worldFrameMapRoot.get()&&worldFrameMapImage.get()&&worldFrameMapRoot.get()->getInheritedVisible()&&worldFrameMapImage.get()->getInheritedVisible();AtlasView::Com<ID3D11Device>d;if(admitted)FixedDevice(w,&d.p);WorldBuildFinished(worldBuild->Tick(admitted,d.p,WorldBuildNow()));}
            catch(const std::exception&e){worldBuildStop=true;worldBuild->Cancel();ErrorLog(std::string("SatelliteCaptureProbe: world build callback: ")+e.what());}
            catch(...){worldBuildStop=true;worldBuild->Cancel();ErrorLog("SatelliteCaptureProbe: world build callback exception; consumer cancellation first");}
            inside=false;
        }
        if(worldFrameEnabled&&OnMain()&&worldFrame&&worldFrame->Active()&&!worldFrame->Quarantined()&&!inside)
        {
            inside=true;
            try{bool admitted=!worldFrameStop&&FixedContext(w,true)&&worldFrameMapRoot.get()&&worldFrameMapImage.get()&&worldFrameMapRoot.get()->getInheritedVisible()&&worldFrameMapImage.get()->getInheritedVisible();AtlasView::Com<ID3D11Device>d;if(admitted)FixedDevice(w,&d.p);WorldFrameFinished(worldFrame->Tick(admitted,d.p,worldFrameToken,WorldFrameNow(),worldFrameJob.frame));}
            catch(const std::exception&e){worldFrameStop=true;worldFrame->Cancel();ErrorLog(std::string("SatelliteCaptureProbe: world frame callback: ")+e.what());}
            catch(...){worldFrameStop=true;worldFrame->Cancel();ErrorLog("SatelliteCaptureProbe: world frame callback exception; cancelling owned request");}
            inside=false;
        }
        if(fixedControl.Active()&&!fixedControl.Stopping()&&!inside)
        {
            inside=true;
            try{if(!FixedContext(w,true))throw std::runtime_error("fixed control world/save/viewer/context changed");AtlasView::Com<ID3D11Device>d;FixedDevice(w,&d.p);if(!fixedControl.SameDevice(d.p))throw std::runtime_error("fixed control retained D3D device changed");fixedControl.Step(true);}
            catch(const std::exception&e){fixedControl.Cancel();ErrorLog(std::string("SatelliteCaptureProbe: ")+e.what());}
            catch(...){fixedControl.Cancel();ErrorLog("SatelliteCaptureProbe: fixed control exception; GPU state scopes restored");}
            inside=false;
        }
        if(terrainExperiment.active && !inside)
        {
            inside=true;
            try
            {
                SaveFileSystem *fs=SaveFileSystem::getSingleton();
                if(!OnMain()||!restorationSafe||!chainSafe||!LoadedGuard()||!RenderAbiMatches()||
                   !au||!au->render||au->render!=renderer||ou!=requestedWorld||renderer->workspace!=w||
                   renderer->root!=terrainExperiment.root||!fs||fs->busy())
                    throw std::runtime_error("isolated Terrain context changed");
                if(terrainExperiment.Step())
                {
                    const Ogre::CompositorChannel &gb=terrainExperiment.workspace->getGlobalTexture("global_gbuffer");
                    if(gb.textures.size()!=3)throw std::runtime_error("private Terrain GBuffer structure differs");
                    std::ostringstream prefix;prefix<<"isolated_terrain_"<<GetCurrentProcessId()<<'_'<<GetTickCount()<<'_'<<++serial;
                    if(terrainExperiment.atlas)atlasRun.Launch(gb,terrainExperiment.camera);
                    else Save(gb.textures[0],prefix.str()+"_gbuffer_raw");
                    if(!terrainExperiment.geometryOnly&&!terrainExperiment.atlas)
                    {
                        std::vector<float> depth(Side*Side);
                        Ogre::PixelBox box(Side,Side,1,Ogre::PF_FLOAT32_R,&depth[0]);
                        gb.textures[2]->getBuffer()->blitToMemory(box);
                        InspectPositionPass(terrainExperiment.camera,depth);
                        InspectSunPass(terrainExperiment.camera,prefix.str(),true,&isolatedLighting);
                    }
                    Log(terrainExperiment.atlas?"atlas block readback copied; bounded file worker launched":"isolated primary-cell diagnostic saved; private Terrain material experiment, not complete atlas");
                    if(!terrainExperiment.AdvancePrimaryCell()){terrainExperiment.active=false;cleanup=true;}
                }
            }
            catch(const std::exception &e)
            {if(atlasRun.active)atlasRun.Cancel();terrainExperiment.active=false;cleanup=true;ErrorLog(std::string("SatelliteCaptureProbe: ")+e.what());}
            catch(...)
            {if(atlasRun.active)atlasRun.Cancel();terrainExperiment.active=false;cleanup=true;ErrorLog("SatelliteCaptureProbe: isolated Terrain experiment failed");}
            inside=false;
        }
        if (pending && !inside)
        {
            pending = false;
            cleanup = true;
            inside = true;
            try
            {
                if (!restorationSafe || !chainSafe || !OnMain() || !LoadedGuard() || !RenderAbiMatches() || ou != requestedWorld || !renderer ||
                    renderer->workspace != w || !Supported(renderer->camera))
                    throw std::runtime_error("capture context changed; refused");
                CameraScope camera(renderer);
                TerrainCameraScope terrainCache;
                ExposureScope exposure;
                ProxyScope proxy;
                exposure.Apply();
                proxy.Apply();
                Ogre::Camera *cam = (mode == 0 || mode == 4) ? renderer->camera : own;
                CameraPairSample("before-aim", renderer->camera, own);
                cam->setAutoAspectRatio(false);
                if (mode >= 2)
                    Aim(cam);
                if (mode == 3)
                    renderer->camera = own;
                CameraPairSample("pre-shot", camera.binding, own);
                if (!shot)
                    throw std::runtime_error("probe workspace unavailable");
                std::ostringstream terrainBefore;
                NativeTerrainCache::State cacheBefore;
                if(!ReadTerrainCache(au->terrain,cacheBefore))throw std::runtime_error("Terrain pre-shot cache unavailable");
                terrainBefore << "Terrain camera before shot matches requested LOD="
                              << (cacheBefore.camera == cam->getLodCamera()) << " matches main LOD="
                              << (cacheBefore.camera == camera.binding->getLodCamera());
                Log(terrainBefore.str());
                const Ogre::CompositorChannel &a = w->getGlobalTexture("global_gbuffer");
                const Ogre::CompositorChannel &b = shot->getGlobalTexture("global_gbuffer");
                if (a.textures.size() != 3 || b.textures.size() != 3)
                    throw std::runtime_error("unexpected GBuffer layout");
                for (int i = 0; i < 3; ++i)
                    if (a.textures[i] == b.textures[i])
                        throw std::runtime_error("main/probe GBuffer aliasing refused");
                const Ogre::CompositorNodeVec &nodes = shot->getNodeSequence();
                if (nodes.size() > 32)
                    throw std::runtime_error("oversized compositor graph");
                bool gui = false;
                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    if (nodes[i]->getDefinition()->getName() == Ogre::IdString("GUI"))
                    {
                        nodes[i]->setEnabled(false);
                        gui = true;
                    }
                    const Ogre::CompositorPassVec &passes = nodes[i]->_getPasses();
                    if (passes.size() > 128)
                        throw std::runtime_error("oversized compositor node");
                    for (size_t k = 0; k < passes.size(); ++k)
                        if (passes[k]->getType() == Ogre::PASS_SCENE)
                        {
                            Ogre::Camera *pc =
                                static_cast<Ogre::CompositorPassScene *>(passes[k])->getCamera();
                            std::ostringstream s;
                            s << "scene pass node=" << nodes[i]->getDefinition()->getNameStr()
                              << " uses requested camera=" << (pc == cam);
                            Log(s.str());
                        }
                }
                if (!gui)
                    throw std::runtime_error("no isolated GUI node to disable");
                shot->_update();
                std::ostringstream terrainAfter;
                NativeTerrainCache::State cacheAfter;
                if(!ReadTerrainCache(au->terrain,cacheAfter))throw std::runtime_error("Terrain post-shot cache unavailable");
                terrainAfter << "Terrain camera after shot matches requested LOD="
                             << (cacheAfter.camera == cam->getLodCamera());
                Log(terrainAfter.str());
                std::ostringstream prefix;
                prefix << "probe_" << GetCurrentProcessId() << "_" << GetTickCount() << "_"
                       << ++serial << "_mode" << mode;
                Save(target, prefix.str() + "_final");
                Save(b.textures[0], prefix.str() + "_gbuffer0_raw");
                std::vector<float> depth(Side * Side);
                Ogre::PixelBox depthBox(Side, Side, 1, Ogre::PF_FLOAT32_R, &depth[0]);
                b.textures[2]->getBuffer()->blitToMemory(depthBox);
                unsigned positive = 0;
                float minDepth = 1e30f, maxDepth = 0;
                for (size_t i = 0; i < depth.size(); ++i)
                    if (depth[i] > 0 && depth[i] < 1e20f)
                    {
                        ++positive;
                        if (depth[i] < minDepth)
                            minDepth = depth[i];
                        if (depth[i] > maxDepth)
                            maxDepth = depth[i];
                    }
                std::ostringstream ds;
                ds << "GBuffer radial depth pixels=" << positive << " min=" << minDepth
                   << " max=" << maxDepth;
                Log(ds.str());
                if (mode >= 2)
                {
                    InspectPositionPass(cam, depth);
                    InspectSunPass(cam,prefix.str());
                }
                Log("one-shot diagnostic saved; GBuffer0 is encoded YCoCg, not player artwork");
            }
            catch (const std::exception &e)
            {
                ErrorLog(std::string("SatelliteCaptureProbe: ") + e.what());
            }
            catch (...)
            {
                ErrorLog("SatelliteCaptureProbe: C++ capture exception; scopes restored");
            }
            inside = false;
        }
        if (next)
            next->workspacePreUpdate(w);
    }
} listener;
void Update(MapScreen *map)
{
    originalUpdate(map);
    try
    {
        if (!mainThread)
            EnumWindows(FindWindowThread, 0);
        if (!OnMain())
            return;
        if(worldBuildEnabled)
        {
            if(worldBuild&&worldBuild->Quarantined()){if(cleanup)Drop();return;}
            if(worldBuild&&worldBuild->Active()&&(worldBuildStop||!map||!map->getVisible()||!FixedContext(gameWorkspace,true)||!SourceReceipts::SameScope(WorldBuildNow(),worldBuildScope)))
            {worldBuildStop=true;worldBuild->Cancel();WorldBuildFinished(worldBuild->Tick(false,NULL,WorldBuildNow()));}
            if(cleanup&&(!worldBuild||!worldBuild->Active())){Drop();return;}
            if(ProbePolicy::PollDue(worldBuildPollAt,GetTickCount()))
            {
                worldBuildPollAt=GetTickCount();unsigned token=GetPrivateProfileIntA("Probe","WorldBuildRequest",0,requestIni.c_str());
                const bool changed=worldBuildLatch.Changed(token),cancel=GetPrivateProfileIntA("Probe","WorldBuildCancel",0,requestIni.c_str())!=0;
                if(cancel&&worldBuild){worldBuildStop=true;worldBuild->Cancel();}
                if(changed)
                {
                    if(!token||cancel||(worldBuild&&worldBuild->Active())||inside||pending||terrainExperiment.root||atlasRun.active||atlasRun.thread||atlasRun.pixels||!map||!map->getVisible()||!chainSafe)throw std::runtime_error("world build request consumed outside idle map context");
                    if(!profileOK||!LoadedGuard()||!au||!au->render||!ou)throw std::runtime_error("world build native world unavailable");EnsureRenderAbi();
                    Renderer*candidate=au->render;Ogre::CompositorWorkspace*workspace=candidate->workspace;SaveFileSystem*fs=SaveFileSystem::getSingleton();
                    if(!workspace||!candidate->root||!candidate->scene||!restorationSafe||!fs||fs->busy()||!RenderAbiMatches()||!FixedViewerOff()||!FixedNoViewerTextures())throw std::runtime_error("world build requires guarded context and viewer OFF at startup/current with no viewer textures");
                    if(!map->mainWidget||!map->mapImage)throw std::runtime_error("world build map widgets unavailable");worldFrameMapRoot.set(map->mainWidget);worldFrameMapImage.set(map->mapImage);
                    if(!worldFrameMapRoot.get()||!worldFrameMapImage.get())throw std::runtime_error("world build map unlink registration unavailable");
                    AtlasView::Com<ID3D11Device>d;FixedDevice(workspace,&d.p);PrepareWorldBuild(token);
                    renderer=candidate;gameWorkspace=workspace;requestedWorld=ou;previous=gameWorkspace->getListener();ourListener=&listener;gameWorkspace->setListener(&listener);
                    Log("world build armed:512@9m four-view cell cohorts; new incomplete GST2 directory only; working atlas unchanged");
                }
            }
            if(worldBuild&&worldBuild->Ready()&&!worldBuildStop&&worldBuildDone<worldBuildCount&&!inside)
            {PrepareWorldBuildJob();if(!worldBuild->Queue(worldBuildJob))throw std::runtime_error("world build native cohort/source admission refused");}
            return;
        }
        if(worldFrameEnabled)
        {
            if(worldFrame&&worldFrame->Quarantined()){if(cleanup)Drop();return;}
            if(worldFrame&&worldFrame->Active()&&(worldFrameStop||!map||!map->getVisible()||!FixedContext(gameWorkspace,true)))
            {worldFrameStop=true;worldFrame->Cancel();WorldFrameFinished(worldFrame->Tick(false,NULL,worldFrameToken,WorldFrameNow(),worldFrameJob.frame));}
            if(cleanup&&(!worldFrame||!worldFrame->Active())){Drop();return;}
            if(ProbePolicy::PollDue(worldFramePollAt,GetTickCount()))
            {
                worldFramePollAt=GetTickCount();unsigned token=GetPrivateProfileIntA("Probe","WorldFrameRequest",0,requestIni.c_str());
                bool changed=worldFrameLatch.Changed(token);bool cancel=GetPrivateProfileIntA("Probe","WorldFrameCancel",0,requestIni.c_str())!=0;
                if(cancel&&worldFrame){worldFrameStop=true;worldFrame->Cancel();}
                if(changed)
                {
                    if(!token||cancel||(worldFrame&&worldFrame->Active())||inside||pending||terrainExperiment.root||atlasRun.active||atlasRun.thread||atlasRun.pixels||!map||!map->getVisible()||!chainSafe)throw std::runtime_error("world frame token consumed outside idle map context");
                    if(!profileOK||!LoadedGuard()||!au||!au->render||!ou)throw std::runtime_error("world frame native world unavailable");EnsureRenderAbi();
                    Renderer*candidate=au->render;Ogre::CompositorWorkspace*workspace=candidate->workspace;SaveFileSystem*fs=SaveFileSystem::getSingleton();
                    if(!workspace||!candidate->root||!candidate->scene||!restorationSafe||!fs||fs->busy()||!RenderAbiMatches()||!FixedViewerOff()||!FixedNoViewerTextures())throw std::runtime_error("world frame requires guarded context and viewer OFF at startup/current with no viewer textures");
                    if(!map->mainWidget||!map->mapImage)throw std::runtime_error("world frame map widgets unavailable");worldFrameMapRoot.set(map->mainWidget);worldFrameMapImage.set(map->mapImage);
                    if(!worldFrameMapRoot.get()||!worldFrameMapImage.get())throw std::runtime_error("world frame map unlink registration unavailable");
                    AtlasView::Com<ID3D11Device>d;FixedDevice(workspace,&d.p);PrepareWorldFrame(token);
                    renderer=candidate;gameWorkspace=workspace;requestedWorld=ou;previous=gameWorkspace->getListener();ourListener=&listener;gameWorkspace->setListener(&listener);
                    Log("world frame armed512 at1m; native-indexed original file inputs, worker validation/export; no atlas mutation");
                }
            }
            return;
        }
        if(nativeReceiptEnabled)
        {
            if(!ProbePolicy::PollDue(nativeReceiptPollAt,GetTickCount()))return;
            nativeReceiptPollAt=GetTickCount();const unsigned token=GetPrivateProfileIntA("Probe","NativeReceiptRequest",0,requestIni.c_str());
            if(!nativeReceiptLatch.Changed(token))return;
            if(!token||!profileOK||!LoadedGuard()||!ou||!map||!map->getVisible()||inside||pending||fixedControl.Active()||atlasRun.active)
            {ErrorLog("SatelliteCaptureProbe: native receipt token consumed outside checked idle world/map frame");return;}
            SaveFileSystem*fs=SaveFileSystem::getSingleton();
            if(!fs||fs->busy()){ErrorLog("SatelliteCaptureProbe: native receipt token consumed while save filesystem unavailable/busy");return;}
            SourceReceipts::Scope scope={GetCurrentThreadId(),reinterpret_cast<SourceReceipts::U64>(ou),(1ULL<<32)|token,token};
            NativeReceiptControl::Report report;HRESULT hr=NativeReceiptControl::Run(scope,report);
            std::ostringstream summary;summary<<"native receipt metadata audit token="<<token<<" pid="<<GetCurrentProcessId()<<" thread="<<scope.thread<<" hr=0x"<<std::hex<<unsigned(hr)<<" profile=0x"<<unsigned(report.profile)<<" catalog=0x"<<unsigned(report.catalog)<<std::dec<<" groups="<<report.groups<<" attempts="<<report.attempts<<" accepted="<<report.accepted<<"; raw indexed original-file metadata only, not native listener stream; no source-file read/GPU/image change";
            if(FAILED(hr))ErrorLog("SatelliteCaptureProbe: "+summary.str());else Log(summary.str());
            for(unsigned i=0;i<report.attempts;++i){const NativeReceiptControl::Attempt&a=report.item[i];std::ostringstream row;row<<"native receipt group="<<a.group<<" resource="<<a.resource<<" hr=0x"<<std::hex<<unsigned(a.status)<<" path="<<a.path;Log(row.str());}
            return;
        }
        if(fixedControlEnabled)
        {
            if(fixedControl.Quarantined()){if(cleanup)Drop();return;}
            if(fixedControl.Active())
            {
                if(!map||!map->getVisible()||!FixedContext(gameWorkspace,true))fixedControl.Cancel();
                DWORD error=0;std::string output;
                if(fixedControl.Poll(error,output)){if(fixedControl.Quarantined())ErrorLog("SatelliteCaptureProbe: fixed worker wait failed; one bounded CPU job/unknown handles retained, GPU cleared, restart required; directory="+output);else if(error)ErrorLog("SatelliteCaptureProbe: fixed control refused/cancelled; incomplete private directory="+output);else Log("fixed base-fixture control complete; not active winner proof or final map; directory="+output);cleanup=true;}
            }
            if(fixedControl.Quarantined()){if(cleanup)Drop();return;}
            if(cleanup&&!fixedControl.Active()){Drop();return;}
            if(ProbePolicy::PollDue(fixedPollAt,GetTickCount()))
            {
                fixedPollAt=GetTickCount();const unsigned token=GetPrivateProfileIntA("Probe","FixedTerrainRequest",0,requestIni.c_str());
                const bool changed=fixedRequestLatch.Changed(token); // Consume even while busy: never replay later.
                const bool cancelled=GetPrivateProfileIntA("Probe","FixedTerrainCancel",0,requestIni.c_str())!=0;
                if(cancelled)fixedControl.Cancel();
                if(changed)
                {
                    if(cancelled||fixedControl.Active()||inside||pending||terrainExperiment.root||atlasRun.active||atlasRun.thread||atlasRun.pixels||!map||!map->getVisible()||!chainSafe)
                        ErrorLog("SatelliteCaptureProbe: fixed control token consumed while unavailable; issue a new token when idle/map visible");
                    else
                    {
                        if(!profileOK||!LoadedGuard()||!au||!au->render||!ou)throw std::runtime_error("fixed control native world unavailable");EnsureRenderAbi();
                        Renderer*candidate=au->render;Ogre::CompositorWorkspace*workspace=candidate->workspace;SaveFileSystem*fs=SaveFileSystem::getSingleton();
                        if(!workspace||!candidate->root||!candidate->scene||!restorationSafe||!fs||fs->busy()||!RenderAbiMatches())throw std::runtime_error("fixed control guarded context/save admission refused");
                        if(!FixedViewerOff()||!FixedNoViewerTextures())throw std::runtime_error("fixed control requires restart with GlobalSatelliteMap.ini [GlobalSatelliteMap] Enabled=0 and no GSM texture resources; viewer and producer budgets cannot coexist");
                        AtlasView::Com<ID3D11Device>d;FixedDevice(workspace,&d.p);if(++fixedEpoch==0)++fixedEpoch;
                        fixedControl.Begin(d.p,Folder(),token,fixedEpoch,true);
                        renderer=candidate;gameWorkspace=workspace;requestedWorld=ou;
                        previous=gameWorkspace->getListener();ourListener=&listener;gameWorkspace->setListener(&listener);
                        Log("fixed256 pinned base-fixture control armed; one bounded CPU slot, no native Terrain/camera, no lighting");
                    }
                }
            }
            return; // Fixed opt-in uses only its own request token, never the legacy keyboard/atlas path.
        }
        if(atlasRun.active&&(!ou||ou!=atlasRun.world||!map||!map->getVisible()||!chainSafe||!restorationSafe))atlasRun.Cancel();
        if(atlasRun.active&&static_cast<DWORD>(GetTickCount()-lastAtlasPoll)>=250){
            lastAtlasPoll=GetTickCount();
            if(GetPrivateProfileIntA("Probe","AtlasCancel",0,requestIni.c_str()))atlasRun.Cancel();
        }
        if(atlasRun.cancelled&&terrainExperiment.atlas){terrainExperiment.active=false;cleanup=true;}
        if(atlasRun.active||atlasRun.thread||atlasRun.pixels)atlasRun.Poll();
        if ((pending || terrainExperiment.active) && (!au || au->render != renderer || !renderer ||
                        renderer->workspace != gameWorkspace || ou != requestedWorld))
        {
            pending = false;
            cleanup = true;
        }
        if(terrainExperiment.active && (!map||!map->getVisible()))
        {terrainExperiment.active=false;cleanup=true;}
        if (cleanup)
        {
            Drop();
            return;
        }
        if(terrainExperiment.recycle.pending)
        {
            SaveFileSystem *fs=SaveFileSystem::getSingleton();
            if(inside||!terrainExperiment.active||!restorationSafe||!chainSafe||!LoadedGuard()||!RenderAbiMatches()||
               !map||!map->getVisible()||!au||au->render!=renderer||!renderer||renderer->workspace!=gameWorkspace||
               ou!=requestedWorld||!renderer->root||terrainExperiment.root!=renderer->root||!fs||fs->busy())
                throw std::runtime_error("deferred private Terrain recycle context changed");
            terrainExperiment.RecycleNativeTerrain(renderer->root);
            return;
        }
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        bool down = pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                    (GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState('Y') & 0x8000);
        bool press = down && !keyHeld;
        keyHeld = down;
        if(atlasRun.active&&!atlasRun.thread&&!terrainExperiment.root)press=true;
        if(ProbePolicy::FileRequestAllowed(fileRequests,pid==GetCurrentProcessId(),backgroundFileRequests,
           map&&map->getVisible(),pending||terrainExperiment.root||inside||atlasRun.active,chainSafe) &&
           ProbePolicy::PollDue(lastRequestPoll,GetTickCount()))
        {
            lastRequestPoll=GetTickCount();
            const bool wasArmed=requestLatch.armed;
            DWORD token=GetPrivateProfileIntA("Probe","Request",0,requestIni.c_str());
            if(requestLatch.Changed(token))
            {
                press=true;std::ostringstream s;s<<"explicit request token="<<token;Log(s.str());
            }
            if(!wasArmed)Log("opt-in request file armed; existing token latched without replay");
        }
        if (!press || pending || terrainExperiment.root || atlasRun.thread || atlasRun.pixels || atlasRun.quarantined || !map || !map->getVisible() || !chainSafe)
            return;
        if (!profileOK || !LoadedGuard() || !au || !au->render || !ou)
            throw std::runtime_error("guard/native world unavailable");
        EnsureRenderAbi();
        SaveFileSystem *fs = SaveFileSystem::getSingleton();
        if (!fs || fs->busy())
            throw std::runtime_error("save/load is busy");
        renderer = au->render;
        if (!renderer->root || !renderer->scene || !renderer->workspace ||
            !Supported(renderer->camera))
            throw std::runtime_error("unsupported main camera state");
        std::string ini = Folder() + "SatelliteCaptureProbe.ini";
        if(GetPrivateProfileIntA("Probe","IsolatedTerrain",0,ini.c_str())==1)
        {
            char executable[MAX_PATH]={0};DWORD n=GetModuleFileNameA(NULL,executable,MAX_PATH);
            if(!n||n>=MAX_PATH)throw std::runtime_error("terrain input path unavailable");
            std::string base(executable,n);base=base.substr(0,base.find_last_of("/\\"));
            if(base.size()>=9 && _stricmp(base.substr(base.size()-9).c_str(),"RE_Kenshi")==0)
                base=base.substr(0,base.find_last_of("/\\"));
            LogResourceTelemetry("before-private-terrain");
            const bool geometryOnlyRequest=GetPrivateProfileIntA("Probe","TerrainGeometryOnly",0,ini.c_str())!=0;
            const bool overlapRequest=GetPrivateProfileIntA("Probe","OverlapControl",0,ini.c_str())==1;
            const bool atlasRequest=atlasRun.active||overlapRequest||GetPrivateProfileIntA("Probe","AtlasCapture",0,ini.c_str())==1;
            if(!geometryOnlyRequest&&!atlasRequest)
                isolatedLighting=ReadLightingSnapshot(GetPrivateProfileIntA("Probe","NativeExposurePreview",0,ini.c_str())!=0,renderer->workspace);
            if(!rtvDebt.Begin())throw std::runtime_error("owned RTV request still active/quarantined");
            char biomeSid[256]={0};
            if(GetPrivateProfileIntA("Probe","BiomeMaterialReference",0,ini.c_str())==1)
                GetPrivateProfileStringA("Probe","ReferenceBiomeSID","5611-Newwworld.mod",biomeSid,sizeof(biomeSid),ini.c_str());
            if(atlasRequest){
                if(!atlasRun.active){
                    unsigned token=GetPrivateProfileIntA("Probe","Request",0,requestIni.c_str());
                    const unsigned controlCase=overlapRequest?GetPrivateProfileIntA("Probe","OverlapCase",0,requestIni.c_str()):0;
                    atlasRun.Begin(Folder(),token?token:GetTickCount(),ou,GetPrivateProfileIntA("Probe","AtlasBlocks",256,requestIni.c_str()),overlapRequest,controlCase);
                }
                terrainExperiment.BeginAtlas(renderer->root,atlasRun.index,base+"/data/newland/land/fullmap.tif",atlasRun.control,atlasRun.controlCase);
            }else
            terrainExperiment.Begin(renderer->root,
                GetPrivateProfileIntA("Probe","PrimaryX",1,ini.c_str()),
                GetPrivateProfileIntA("Probe","PrimaryZ",1,ini.c_str()),
                GetPrivateProfileIntA("Probe","Altitude",12000,ini.c_str()),base+"/data/newland/land/fullmap.tif",
                geometryOnlyRequest,
                GetPrivateProfileIntA("Probe","PrimaryGridBatch",0,ini.c_str())!=0,
                GetPrivateProfileIntA("Probe","RecycleTerrainBetweenCells",0,ini.c_str())!=0,biomeSid,
                GetPrivateProfileIntA("Probe","PaintedMapsReference",0,ini.c_str())==1,
                GetPrivateProfileIntA("Probe","GeographicBiomes",0,ini.c_str())==1,
                GetPrivateProfileIntA("Probe","GeographicNativeFade",0,ini.c_str())==1);
            gameWorkspace=renderer->workspace;previous=gameWorkspace->getListener();ourListener=&listener;
            requestedWorld=ou;gameWorkspace->setListener(&listener);return;
        }
        if(diagnosticRaster.resolutionOnly)
            throw std::runtime_error("high raster control requires the isolated Terrain path");
        mode = GetPrivateProfileIntA("Probe", "Mode", 0, ini.c_str());
        cx = GetPrivateProfileIntA("Probe", "CellX", 0, ini.c_str());
        cz = GetPrivateProfileIntA("Probe", "CellZ", 0, ini.c_str());
        if (GetPrivateProfileIntA("Probe", "CurrentCell", 1, ini.c_str()))
        {
            Ogre::Vector3 c = ou->getCameraCenter();
            cx = ProbePolicy::Cell(c.x);
            cz = ProbePolicy::Cell(c.z);
        }
        if (!ProbePolicy::ValidMode(mode) || !ProbePolicy::SafeCell(cx) ||
            !ProbePolicy::SafeCell(cz))
            throw std::runtime_error("invalid bounded request");
        proxies = GetPrivateProfileIntA("Probe", "ExistingProxies", 0, ini.c_str()) != 0;
        handles.clear();
        if (proxies)
        {
            if (map->mapMarkersTowns.size() > 4096)
                throw std::runtime_error("town marker cap exceeded");
            for (ogre_unordered_map<hand, MapScreen::MapMarkerTown *>::type::const_iterator i =
                     map->mapMarkersTowns.begin();
                 i != map->mapMarkersTowns.end() && handles.size() < 8; ++i)
            {
                TownBase *town = i->first.getTown();
                if (!town)
                    continue;
                const Ogre::Vector3 pos = town->getPosition();
                const double dx = pos.x - ProbePolicy::Centre(cx),
                             dz = pos.z - ProbePolicy::Centre(cz);
                if (std::fabs(dx) <= 1536 && std::fabs(dz) <= 1536)
                    handles.push_back(i->first);
            }
        }
        Ogre::Camera *c = renderer->camera;
        shotScene = renderer->scene;
        own = renderer->scene->createCamera("LEOPARD_PrivateCaptureCamera");
        ownNode = renderer->scene->getRootSceneNode()->createChildSceneNode();
        ownNodeId = ownNode->getId();
        if(!ProbeOwnership::AttachOwnedCamera(own,c,ownNode))
            throw std::runtime_error("owned camera attachment refused");
        NodeSample("before-clone", ownNode);
        if (!ProbeOwnership::CopyOwnedCameraPose(own,c,ownNode,renderer->scene->getRootSceneNode()))
            throw std::runtime_error("owned camera pose ancestry refused");
        own->setOrthoWindow(c->getOrthoWindowWidth(), c->getOrthoWindowHeight());
        own->setProjectionType(c->getProjectionType());
        own->setFOVy(c->getFOVy());
        own->setAspectRatio(c->getAspectRatio());
        own->setNearClipDistance(c->getNearClipDistance());
        own->setFarClipDistance(c->getFarClipDistance());
        own->setFocalLength(c->getFocalLength());
        own->setFrustumOffset(c->getFrustumOffset());
        own->setUseRenderingDistance(c->getUseRenderingDistance());
        nativeApi.setMin(own,nativeApi.getMin(c));
        own->setAutoAspectRatio(false);
        CameraPairSample("after-clone", c, own);
        target = Ogre::TextureManager::getSingleton().createManual(
            "LEOPARD_PrivateCaptureTarget", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, Side, Side, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);
        {
            // Build resources outside compositor iteration and restore any temporary binding
            // immediately, including failures during the custom MainLight pass construction.
            CameraScope camera(renderer);
            TerrainCameraScope terrainCache;
            Ogre::Camera *cam = (mode == 0 || mode == 4) ? renderer->camera : own;
            cam->setAutoAspectRatio(false);
            if (mode >= 2)
                Aim(cam);
            if (mode == 3)
                renderer->camera = own;
            const Ogre::CompositorWorkspaceDef *def = WorkspaceAccess::Def(renderer->workspace);
            if (!def)
                throw std::runtime_error("native workspace definition missing");
            shot = renderer->root->getCompositorManager2()->addWorkspace(
                renderer->scene, target->getBuffer()->getRenderTarget(), cam,
                DefinitionAccess::Name(def), false);
            if (mode >= 2)
            {
                const Ogre::CompositorChannel &gb = shot->getGlobalTexture("global_gbuffer");
                if (gb.textures.size() != 3)
                    throw std::runtime_error("position pass requires three GBuffer surfaces");
                PreparePositionPass(renderer->root->getCompositorManager2(), renderer->scene,
                                    cam, gb.textures[2]);
                PrepareSunPass(renderer->root->getCompositorManager2(),renderer->scene,cam,gb);
            }
        }
        if(!restorationSafe || !chainSafe)
            throw std::runtime_error("construction restoration failed; capture not armed");
        gameWorkspace = renderer->workspace;
        previous = gameWorkspace->getListener();
        ourListener = &listener;
        gameWorkspace->setListener(&listener);
        requestedWorld = ou;
        pending = true;
        std::ostringstream s;
        s << "manual one-shot armed: mode=" << mode << " fixed cell=" << cx << "," << cz
          << " HWND thread=" << mainThread;
        Log(s.str());
    }
    catch (const std::exception &e)
    {
        if(worldBuild&&worldBuild->Active()){worldBuildStop=true;worldBuild->Cancel();}
        if(worldFrame&&worldFrame->Active()){worldFrameStop=true;worldFrame->Cancel();}
        if(fixedControl.Active())fixedControl.Cancel();
        if(atlasRun.active)atlasRun.Cancel();
        ErrorLog(std::string("SatelliteCaptureProbe: ") + e.what());
        cleanup = true;
    }
    catch (...)
    {
        if(worldBuild&&worldBuild->Active()){worldBuildStop=true;worldBuild->Cancel();}
        if(worldFrame&&worldFrame->Active()){worldFrameStop=true;worldFrame->Cancel();}
        if(fixedControl.Active())fixedControl.Cancel();
        if(atlasRun.active)atlasRun.Cancel();
        ErrorLog("SatelliteCaptureProbe: request failed");
        cleanup = true;
    }
}
void WorldFrameLoadHook(SaveFileSystem*self,const std::string&path){InterlockedIncrement(&worldFrameLoadGeneration);worldFrameOriginalLoad(self,path);}
void WorldFrameNewHook(SaveFileSystem*self){InterlockedIncrement(&worldFrameLoadGeneration);worldFrameOriginalNew(self);}
} // namespace
// RE_Kenshi resolves the decorated C++ name ?startPlugin@@YAXXZ.
__declspec(dllexport) void startPlugin()
{
    char exe[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    profileOK = n && n < MAX_PATH && FileHash(exe) == ExpectedExeHash && LoadedGuard();
    if (!profileOK)
    {
        ErrorLog("SatelliteCaptureProbe: exact504b RE/loaded-code guard failed; inert");
        return;
    }
    try
    {
        requestIni=Folder()+"SatelliteCaptureProbe.ini";
        nativeReceiptEnabled=GetPrivateProfileIntA("Probe","NativeReceiptControl",0,requestIni.c_str())==1;
        nativeReceiptLatch.Changed(GetPrivateProfileIntA("Probe","NativeReceiptRequest",0,requestIni.c_str()));
        fixedControlEnabled=GetPrivateProfileIntA("Probe","FixedTerrainControl",0,requestIni.c_str())==1;
        worldFrameEnabled=GetPrivateProfileIntA("Probe","WorldFrameControl",0,requestIni.c_str())==1;
        worldBuildEnabled=GetPrivateProfileIntA("Probe","WorldBuildControl",0,requestIni.c_str())==1;
        worldBuildLatch.Changed(GetPrivateProfileIntA("Probe","WorldBuildRequest",0,requestIni.c_str()));
        worldFrameLatch.Changed(GetPrivateProfileIntA("Probe","WorldFrameRequest",0,requestIni.c_str()));
        if(worldFrameEnabled&&(fixedControlEnabled||nativeReceiptEnabled))throw std::runtime_error("world frame diagnostic modes are mutually exclusive");
        if(worldBuildEnabled&&(worldFrameEnabled||fixedControlEnabled||nativeReceiptEnabled))throw std::runtime_error("world build diagnostic modes are mutually exclusive");
        if(fixedControlEnabled||worldFrameEnabled||worldBuildEnabled)
        {
            std::string folder=Folder();if(!folder.empty())folder.erase(folder.size()-1);size_t slash=folder.find_last_of("/\\");
            if(slash==std::string::npos)throw std::runtime_error("fixed control sibling viewer INI path unavailable");
            fixedGsmIni=folder.substr(0,slash+1)+"GlobalSatelliteMap/GlobalSatelliteMap.ini";
            fixedGsmStartupOff=GetPrivateProfileIntA("Probe","FixedTerrainGsmStartupDisabled",0,requestIni.c_str())==1&&GetPrivateProfileIntA("GlobalSatelliteMap","Enabled",1,fixedGsmIni.c_str())==0;
            fixedRequestLatch.Changed(GetPrivateProfileIntA("Probe","FixedTerrainRequest",0,requestIni.c_str()));
            if(!fixedGsmStartupOff)ErrorLog("SatelliteCaptureProbe: fixed control inert until restart with viewer Enabled=0 and FixedTerrainGsmStartupDisabled=1; existing request latched without replay");
        }
        RasterPolicy::Layout raster;
        if(!RasterPolicy::Select(int(GetPrivateProfileIntA("Probe","DiagnosticSide",512,requestIni.c_str())),raster,
            GetPrivateProfileIntA("Probe","ResolutionOnlyControl",0,requestIni.c_str())==1))
        {ErrorLog("SatelliteCaptureProbe: unsupported diagnostic raster; inert");return;}
        Side = raster.side;
        diagnosticRaster=raster;
        std::ostringstream rasterLog;
        rasterLog<<"diagnostic raster latched side="<<Side<<" named surface bytes="<<raster.targetBytes
                 <<" peak readback/staging bytes="<<raster.readbackBytes<<"; excludes native geometry, depth/driver/main-scene allocations";
        Log(rasterLog.str());
        fileRequests=GetPrivateProfileIntA("Probe","EnableRequestFile",0,requestIni.c_str())==1;
        backgroundFileRequests=fileRequests && GetPrivateProfileIntA("Probe","AllowBackgroundFileRequests",0,requestIni.c_str())==1;
        reusePrivateShaders=GetPrivateProfileIntA("Probe","ReusePrivateShaders",0,requestIni.c_str())==1;
        if(raster.resolutionOnly)
        {
            if(!fileRequests||!reusePrivateShaders)
            {ErrorLog("SatelliteCaptureProbe: high raster control requires opt-in request file and shader reuse; inert");return;}
            std::ostringstream control;
            control<<"resolution-only allocation control:2048-equivalent geometry detail="<<raster.detail
                   <<"; unchanged1GiB/16M native limits; final terrain quality unproven";Log(control.str());
        }
    }
    catch(...){ErrorLog("SatelliteCaptureProbe: request path unavailable; inert");return;}
    if((worldFrameEnabled||worldBuildEnabled)&&(KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),WorldFrameLoadHook,&worldFrameOriginalLoad)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),WorldFrameNewHook,&worldFrameOriginalNew)))
    {ErrorLog("SatelliteCaptureProbe: world frame lifecycle hooks unavailable; inert");return;}
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update), Update, &originalUpdate))
        ErrorLog("SatelliteCaptureProbe: cannot attach manual map observer; inert");
    else
        Log("guard passed; private probe ready for manual request only (not an atlas renderer)");
}
