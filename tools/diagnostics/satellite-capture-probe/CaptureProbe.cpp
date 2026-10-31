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
#include <OgreRenderTexture.h>
#include <OgreGpuProgramManager.h>
#include <OgreGpuProgramParams.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/OgreCompositorWorkspaceListener.h>
#include <Compositor/OgreCompositorNode.h>
#include <Compositor/OgreCompositorNodeDef.h>
#include <Compositor/OgreCompositorChannel.h>
#include <Compositor/Pass/PassScene/OgreCompositorPassScene.h>
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
#include "OwnedCameraAttachment.h"
#pragma comment(lib, "advapi32.lib")
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace
{
const int Side = 512;
DWORD mainThread = 0;
bool keyHeld = false, pending = false, cleanup = false, inside = false, profileOK = false,
     chainSafe = true;
int mode = 0, cx = 0, cz = 0;
bool proxies = false;
unsigned serial = 0;
GameWorld *requestedWorld = NULL;
Ogre::CompositorWorkspace *gameWorkspace = NULL, *shot = NULL;
Renderer *renderer = NULL;
Ogre::Camera *own = NULL;
Ogre::SceneNode *ownNode = NULL;
Ogre::SceneManager *shotScene = NULL;
Ogre::IdType ownNodeId = 0;
Ogre::TexturePtr target;
Ogre::CompositorWorkspaceListener *previous = NULL;
Ogre::CompositorWorkspaceListener *ourListener = NULL;
std::vector<hand> handles;
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
struct CameraAccess : Ogre::Camera
{
    static Ogre::Viewport *Last(const Ogre::Camera *c)
    {
        return c->*(&CameraAccess::mLastViewport);
    }
    static void Last(Ogre::Camera *c, Ogre::Viewport *v)
    {
        c->*(&CameraAccess::mLastViewport) = v;
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
      << " cull=" << c->getCullingFrustum() << " lod=" << c->getLodCamera() << " view=";
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
          useDistance(c->getUseRenderingDistance()), usePixels(c->getUseMinPixelSize()),
          last(CameraAccess::Last(c))
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
        c->setUseMinPixelSize(usePixels);
        CameraAccess::Last(c, last);
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
    const Ogre::Camera *camera;
    Ogre::Vector3 position;
    TerrainCameraScope()
        : terrain(au ? au->terrain : NULL), camera(terrain ? terrain->mCamera : NULL),
          position(terrain ? terrain->mCameraPosition : Ogre::Vector3::ZERO)
    {
    }
    ~TerrainCameraScope()
    {
        // A secondary viewport can leave this pointer referencing its temporary camera.
        // Restore the known camera cache without depending on a subsequent successful main pass.
        if (au && au->terrain == terrain && terrain)
        {
            terrain->mCamera = camera;
            terrain->mCameraPosition = position;
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
    c->setUseMinPixelSize(false);
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
void Drop()
{
    if (!OnMain())
        return;
    Renderer *current = au ? au->render : NULL;
    Ogre::CompositorManager2 *manager =
        current && current->root ? current->root->getCompositorManager2() : NULL;
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
    gameWorkspace = NULL;
    // Preserve forwarding if an external listener wrapped ours in the intervening frame.
    if (detached)
        previous = NULL;
    if (manager && ManagerAccess::Contains(manager, shot))
        manager->removeWorkspace(shot);
    shot = NULL;
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
}
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
        if (pending && !inside)
        {
            pending = false;
            cleanup = true;
            inside = true;
            try
            {
                if (!OnMain() || !LoadedGuard() || ou != requestedWorld || !renderer ||
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
                terrainBefore << "Terrain camera before shot matches requested="
                              << (au->terrain && au->terrain->mCamera == cam) << " matches main="
                              << (au->terrain && au->terrain->mCamera == camera.binding);
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
                terrainAfter << "Terrain camera after shot matches requested="
                             << (au->terrain && au->terrain->mCamera == cam);
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
        if (pending && (!au || au->render != renderer || !renderer ||
                        renderer->workspace != gameWorkspace || ou != requestedWorld))
        {
            pending = false;
            cleanup = true;
        }
        if (cleanup)
        {
            Drop();
            return;
        }
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        bool down = pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                    (GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState('Y') & 0x8000);
        bool press = down && !keyHeld;
        keyHeld = down;
        if (!press || pending || !map || !map->getVisible() || !chainSafe)
            return;
        if (!profileOK || !LoadedGuard() || !au || !au->render || !ou)
            throw std::runtime_error("guard/native world unavailable");
        SaveFileSystem *fs = SaveFileSystem::getSingleton();
        if (!fs || fs->busy())
            throw std::runtime_error("save/load is busy");
        renderer = au->render;
        if (!renderer->root || !renderer->scene || !renderer->workspace ||
            !Supported(renderer->camera))
            throw std::runtime_error("unsupported main camera state");
        std::string ini = Folder() + "SatelliteCaptureProbe.ini";
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
        own->setPosition(ownNode->convertWorldToLocalPosition(c->getDerivedPosition()));
        own->setOrientation(ownNode->convertWorldToLocalOrientation(c->getDerivedOrientation()));
        own->setOrthoWindow(c->getOrthoWindowWidth(), c->getOrthoWindowHeight());
        own->setProjectionType(c->getProjectionType());
        own->setFOVy(c->getFOVy());
        own->setAspectRatio(c->getAspectRatio());
        own->setNearClipDistance(c->getNearClipDistance());
        own->setFarClipDistance(c->getFarClipDistance());
        own->setFocalLength(c->getFocalLength());
        own->setFrustumOffset(c->getFrustumOffset());
        own->setUseRenderingDistance(c->getUseRenderingDistance());
        own->setUseMinPixelSize(c->getUseMinPixelSize());
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
        }
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
        ErrorLog(std::string("SatelliteCaptureProbe: ") + e.what());
        cleanup = true;
    }
    catch (...)
    {
        ErrorLog("SatelliteCaptureProbe: request failed");
        cleanup = true;
    }
}
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
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update), Update, &originalUpdate))
        ErrorLog("SatelliteCaptureProbe: cannot attach manual map observer; inert");
    else
        Log("guard passed; private probe ready for manual request only (not an atlas renderer)");
}
