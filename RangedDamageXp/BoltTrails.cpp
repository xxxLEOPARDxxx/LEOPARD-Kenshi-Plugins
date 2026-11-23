// SPDX-License-Identifier: GPL-3.0-or-later
// Bolt streaks adapted from MeitouClient by VlcekM and contributors. See ORIGIN.md.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "BoltTrails.h"
#include "BoltTrailRibbon.h"

#include <Windows.h>
#include <cstring>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Renderer.h>
#include <ogre/OgreCamera.h>
#include <ogre/OgreHardwarePixelBuffer.h>
#include <ogre/OgreManualObject.h>
#include <ogre/OgreMaterialManager.h>
#include <ogre/OgrePass.h>
#include <ogre/OgreSceneManager.h>
#include <ogre/OgreSceneNode.h>
#include <ogre/OgreTechnique.h>
#include <ogre/OgreTextureManager.h>
#include <ogre/OgreTextureUnitState.h>

namespace bolt_trail {
namespace {

const char MaterialName[] = "LEOPARD/RangedDamageXp/Streak";
const char MaskName[] = "LEOPARD/RangedDamageXp/StreakMask";
const unsigned char ForwardQueue = 84; // After the main particle queue in Lighting_HDR.
const unsigned NodeOffset = 0x58;
const unsigned SpeedOffset = 0x44;
const unsigned AgeOffset = 0x60;
const unsigned FlightStateOffset = 0x70;
const unsigned Flying = 1;
const unsigned HitBody = 2, HitWorld = 3;

History history;
Streak streaks[MaxTrails];
void (*originalAdvance)(void*) = NULL;
const float* simulationDelta = NULL;
double simulationClock = 0;
unsigned frameSerial = 0;
bool stopped = false;

LONGLONG PackAppearance(const Appearance& style)
{
    return LONGLONG(style.colour) | (LONGLONG(style.brightnessPercent) << 24) |
           (LONGLONG(style.lengthPercent) << 32) | (LONGLONG(style.fadePercent) << 40) |
           (LONGLONG(style.outline) << 47) |
           (LONGLONG(ClampTrailLimit(style.trailLimit) - 1) << 48);
}

__declspec(align(8)) volatile LONGLONG appearanceBits = PackAppearance(Appearance());

Appearance ReadAppearance()
{
    const LONGLONG bits = InterlockedCompareExchange64(&appearanceBits, 0, 0);
    Appearance style;
    style.colour = uint32_t(bits & 0xffffff);
    style.brightnessPercent = unsigned((bits >> 24) & 0xff);
    style.lengthPercent = unsigned((bits >> 32) & 0xff);
    style.fadePercent = unsigned((bits >> 40) & 0x7f);
    style.outline = ((bits >> 47) & 1) != 0;
    style.trailLimit = unsigned((bits >> 48) & 0xff) + 1;
    return style;
}

Point CopyPoint(const Ogre::Vector3& v)
{
    return Point(v.x, v.y, v.z);
}

void HalveMask(unsigned char* rgba, unsigned width, unsigned height)
{
    const unsigned xs = width > 1 ? 2 : 1, ys = height > 1 ? 2 : 1;
    const unsigned nextWidth = width/xs, nextHeight = height/ys;
    for (unsigned y = 0; y < nextHeight; ++y)
        for (unsigned x = 0; x < nextWidth; ++x)
            for (unsigned c = 0; c < 4; ++c) {
                unsigned sum = 0;
                for (unsigned dy = 0; dy < ys; ++dy)
                    for (unsigned dx = 0; dx < xs; ++dx)
                        sum += rgba[((y*ys+dy)*width+x*xs+dx)*4+c];
                rgba[(y*nextWidth+x)*4+c] = static_cast<unsigned char>((sum + xs*ys/2)/(xs*ys));
            }
}

class Drawing : public Ogre::SceneManager::Listener,
                public Ogre::MovableObject::Listener, public Ogre::Node::Listener
{
    Ogre::SceneManager* scene;
    Ogre::SceneNode* node;
    Ogre::ManualObject* lines;
    bool section, visible;
    unsigned maskFade;
    bool maskOutline;

    bool EnsureMask(const Appearance& style)
    {
        if (maskFade == style.fadePercent && maskOutline == style.outline) return true;
        Ogre::TextureManager* textures = Ogre::TextureManager::getSingletonPtr();
        if (!textures) return false;
        Ogre::TexturePtr texture = textures->getByName(MaskName);
        if (texture.isNull())
            texture = textures->createManual(MaskName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                             Ogre::TEX_TYPE_2D, MaskWidth, MaskHeight, MaskMipLevels,
                                             Ogre::PF_BYTE_RGBA, Ogre::TU_STATIC_WRITE_ONLY);
        if (texture.isNull() || texture->getNumMipmaps() != MaskMipLevels) return false;

        unsigned char pixels[MaskWidth*MaskHeight*4];
        MakeMask(pixels, style.fadePercent, style.outline);
        unsigned width = MaskWidth, height = MaskHeight;
        for (unsigned mip = 0; mip <= MaskMipLevels; ++mip) {
            Ogre::HardwarePixelBufferSharedPtr buffer = texture->getBuffer(0, mip);
            if (buffer.isNull()) return false;
            buffer->blitFromMemory(Ogre::PixelBox(width, height, 1, Ogre::PF_BYTE_RGBA, pixels));
            if (mip < MaskMipLevels) {
                HalveMask(pixels, width, height);
                width = width > 1 ? width/2 : 1;
                height = height > 1 ? height/2 : 1;
            }
        }
        maskFade = style.fadePercent;
        maskOutline = style.outline;
        return true;
    }

    void Vertex(const Point& point, const Ogre::ColourValue& colour, float u, float v)
    {
        lines->position(point.x, point.y, point.z);
        lines->colour(colour);
        lines->textureCoord(u, v);
    }

    bool EnsureGeometry(Ogre::SceneManager* owner, Ogre::SceneNode* root, unsigned trailLimit)
    {
        if (!scene) {
            owner->addListener(this);
            scene = owner;
        }
        if (!lines) {
            Ogre::MaterialManager* materials = Ogre::MaterialManager::getSingletonPtr();
            if (!materials) return false;
            Ogre::MaterialPtr material = materials->getByName(MaterialName);
            if (material.isNull()) {
                material = materials->create(MaterialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                Ogre::Technique* technique = material->getNumTechniques() ? material->getTechnique(0) : material->createTechnique();
                Ogre::Pass* pass = technique->getNumPasses() ? technique->getPass(0) : technique->createPass();
                pass->setVertexProgram("Basic_Coloured_Texture_VP");
                pass->setFragmentProgram("Basic_Texture_FP");
                pass->setLightingEnabled(false);
                pass->setDepthCheckEnabled(true);
                pass->setDepthWriteEnabled(false);
                pass->setCullingMode(Ogre::CULL_NONE);
                pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
                Ogre::TextureUnitState* sampler = pass->createTextureUnitState(MaskName);
                sampler->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
                sampler->setTextureFiltering(Ogre::TFO_TRILINEAR);
                material->setReceiveShadows(false);
                material->load();
            }
            if (!material->getNumSupportedTechniques()) return false;
            Ogre::ManualObject* object = owner->createManualObject(Ogre::SCENE_DYNAMIC);
            object->setListener(this);
            lines = object;
            lines->setVisible(false);
            lines->setDynamic(true);
            lines->setCastShadows(false);
            lines->setQueryFlags(0);
            lines->setRenderQueueGroup(ForwardQueue);
            lines->estimateVertexCount(trailLimit * 4);
            lines->estimateIndexCount(trailLimit * 6);
        }
        if (!node) {
            node = root->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            node->setListener(this);
        }
        if (!lines->isAttached()) node->attachObject(lines);
        return true;
    }

public:
    Drawing() : scene(NULL), node(NULL), lines(NULL), section(false), visible(false),
                maskFade(0), maskOutline(false) {}
    bool Visible() const { return visible; }

    void Hide()
    {
        if (lines && visible) lines->setVisible(false);
        visible = false;
    }

    void ReleaseScene()
    {
        Hide();
        if (!scene) return;
        if (lines) {
            Ogre::ManualObject* object = lines;
            object->setListener(NULL);
            lines = NULL;
            scene->destroyManualObject(object);
        }
        section = false;
        if (node) {
            Ogre::SceneNode* old = node;
            old->setListener(NULL);
            node = NULL;
            scene->destroySceneNode(old);
        }
        scene->removeListener(this);
        scene = NULL;
        maskFade = 0;
    }

    void sceneManagerDestroyed(Ogre::SceneManager* source) override
    {
        if (source != scene) return;
        // The native owner destroys its objects. No engine calls in this callback.
        scene = NULL;
        node = NULL;
        lines = NULL;
        section = visible = false;
        maskFade = 0;
        history.Clear();
    }

    void objectDestroyed(Ogre::MovableObject* source) override
    {
        if (source != lines) return;
        lines = NULL;
        section = visible = false;
        maskFade = 0;
        history.Clear();
    }

    void nodeDestroyed(const Ogre::Node* source) override
    {
        if (source != node) return;
        node = NULL;
        visible = false;
        history.Clear();
    }

    bool Update(Ogre::SceneManager* owner, Ogre::SceneNode* root, Point eye, Point right,
                const Appearance& style, unsigned used)
    {
        if (scene && scene != owner) ReleaseScene();
        const Ogre::ColourValue core(float((style.colour >> 16) & 0xff) / 255.0f,
                                     float((style.colour >> 8) & 0xff) / 255.0f,
                                     float(style.colour & 0xff) / 255.0f,
                                     BaseOpacity * float(style.brightnessPercent) / 100.0f);
        unsigned vertices = 0;
        for (unsigned i = 0; i < used; ++i) {
            Quad quad;
            if (!MakeQuad(streaks[i], eye, right, quad)) continue;
            if (!vertices) {
                if (!EnsureMask(style) || !EnsureGeometry(owner, root, style.trailLimit)) return false;
                if (section) lines->beginUpdate(0);
                else lines->begin(MaterialName, Ogre::RenderOperation::OT_TRIANGLE_LIST);
            }
            Vertex(quad.headLeft, core, 0, 0);
            Vertex(quad.headRight, core, 1, 0);
            Vertex(quad.tailRight, core, 1, 1);
            Vertex(quad.tailLeft, core, 0, 1);
            lines->quad(vertices, vertices+1, vertices+2, vertices+3);
            vertices += 4;
        }
        if (!vertices) {
            Hide();
            return true;
        }
        if (!lines->end()) return false;
        section = true;
        if (!visible) lines->setVisible(true);
        visible = true;
        return true;
    }
};

Drawing draw;
Ogre::SceneNode* historyRoot = NULL; // compared only; ownership stays with Ogre
Ogre::Vector3 historyOrigin;

void StopVisuals()
{
    stopped = true;
    history.Clear();
    try { draw.ReleaseScene(); } catch (...) {}
    ErrorLog("RangedDamageXp: bolt trail rendering stopped after an Ogre error");
}

struct Frame {
    GameWorld* world;
    Ogre::SceneManager* scene;
    Ogre::SceneNode* root;
    Appearance style;
    unsigned serial;
    bool timeRead, timeAdvanced, resolved, sampled;

    Frame(GameWorld* owner) : world(owner), scene(NULL), root(NULL), style(ReadAppearance()),
                              serial(++frameSerial), timeRead(false), timeAdvanced(false),
                              resolved(false), sampled(false) {}

    void AdvanceClock()
    {
        if (timeRead) return;
        timeRead = true;
        // MainLoop writes the step at 0x787f13, before dispatching the bolted projectiles.
        // Read only after that dispatch begins or MainLoop has returned.
        const float delta = *simulationDelta;
        timeAdvanced = world && !world->paused && !world->gameResetting && delta > 0 && delta <= 1;
        if (timeAdvanced) simulationClock += delta;
    }

    bool Resolve()
    {
        if (resolved) return root != NULL;
        resolved = true;
        if (!world || !world->render || !world->player || world->gameResetting ||
            world->isLoadingFromASaveGame()) return false;
        Renderer* renderer = static_cast<Renderer*>(world->render);
        scene = renderer->getSceneManager();
        if (!scene) return false;
        root = scene->getRootSceneNode(Ogre::SCENE_DYNAMIC);
        if (!root) return false;
        // Trail coordinates and projectile coordinates have the same parent.
        // Discard historical points if the engine rebases the scene.
        const Ogre::Vector3 origin = renderer->getSceneRelativeOrigin();
        if (root != historyRoot || origin != historyOrigin) {
            history.Clear();
            historyRoot = root;
            historyOrigin = origin;
        }
        return true;
    }

    bool SceneStillLive() const
    {
        if (!root || !world->render || !world->player || world->gameResetting) return false;
        const Renderer* renderer = static_cast<const Renderer*>(world->render);
        return renderer->getSceneManager() == scene && scene->getRootSceneNode(Ogre::SCENE_DYNAMIC) == root;
    }
};

__declspec(thread) Frame* activeFrame = NULL;

class CaptureFrame {
    Frame* previous;
public:
    explicit CaptureFrame(Frame& frame) : previous(activeFrame)
    {
        if (!previous) activeFrame = &frame;
    }
    ~CaptureFrame() { activeFrame = previous; }
};

template<class T> T Field(const void* object, unsigned distance)
{
    T value;
    std::memcpy(&value, static_cast<const unsigned char*>(object) + distance, sizeof(value));
    return value;
}

Point Record(const Ogre::SceneNode& node)
{
    const Ogre::Vector3 v = node.getPosition();
    return Point(v.x, v.y, v.z);
}

// Steam 1.0.65. The native manager holds its flight lock across this call.
// The bolt and its node survive the update. We retain only copied points.
void Advance(void* projectile)
{
    Frame* frame = activeFrame;
    Ogre::SceneNode* node = NULL;
    Point before;
    float ageBefore = 0;
    if (frame && !stopped && Field<unsigned>(projectile, FlightStateOffset) == Flying) {
        try {
            Ogre::SceneNode* flightNode = Field<Ogre::SceneNode*>(projectile, NodeOffset);
            if (flightNode && frame->Resolve() && flightNode->getParent() == frame->root) {
                before = Record(*flightNode);
                ageBefore = Field<float>(projectile, AgeOffset);
                node = flightNode;
            }
        } catch (...) { stopped = true; }
    }
    originalAdvance(projectile);
    if (!node || stopped) return;
    if (Field<Ogre::SceneNode*>(projectile, NodeOffset) != node) return;
    try {
        if (node->getParent() != frame->root) return;
        const unsigned state = Field<unsigned>(projectile, FlightStateOffset);
        const float ageAfter = Field<float>(projectile, AgeOffset);
        const float speed = Field<float>(projectile, SpeedOffset);
        frame->AdvanceClock();
        history.Observe(reinterpret_cast<uintptr_t>(projectile), before, Record(*node),
                        CopyPoint(node->getOrientation().zAxis()), speed, ageBefore, ageAfter,
                        state == Flying, state == HitBody || state == HitWorld,
                        frame->serial, simulationClock, frame->style.lengthPercent);
        frame->sampled = true;
    } catch (...) { stopped = true; }
}

}

bool Install()
{
    // The Steam image used by RE_Kenshi. A different executable retains the XP feature.
    unsigned char* base = reinterpret_cast<unsigned char*>(GetModuleHandleW(NULL));
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) return false;
    const IMAGE_NT_HEADERS64* pe = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    const unsigned char entry[] = {0x48,0x8b,0xc4,0x55,0x57,0x48,0x8d,0x68,0xa1,0x48,
                                   0x81,0xec,0xe8,0,0,0,0x83,0x79,0x70,0x01,
                                   0xf3,0x0f,0x10,0x05,0xd4,0xa9,0xcf,0x01};
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.TimeDateStamp != 0x65d604d7 ||
        pe->OptionalHeader.SizeOfImage != 0x232c000 || std::memcmp(base + 0x437d40, entry, sizeof(entry)))
        return false;
    simulationDelta = reinterpret_cast<const float*>(base + 0x2132730);
    return KenshiLib::AddHook(base + 0x437d40, &Advance, &originalAdvance) == KenshiLib::SUCCESS;
}

void ConfigureAppearance(const Appearance& appearance)
{
    InterlockedExchange64(&appearanceBits, PackAppearance(appearance));
}

void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float))
{
    if (stopped || activeFrame) {
        original(world, time);
        return;
    }
    Frame frame(world);
    history.SetLimit(frame.style.trailLimit);
    {
        CaptureFrame capture(frame);
        original(world, time);
    }
    if (stopped) {
        StopVisuals();
        return;
    }
    if (!frame.sampled && !history.Active() && !draw.Visible()) return;
    try {
        if (!frame.Resolve() || !frame.SceneStillLive()) {
            history.Clear();
            draw.Hide();
            return;
        }
        frame.AdvanceClock();
        const unsigned used = history.Build(frame.serial, simulationClock, frame.timeAdvanced,
                                            frame.style.lengthPercent, streaks);
        Renderer* renderer = static_cast<Renderer*>(world->render);
        Ogre::Camera* camera = renderer->getCamera();
        if (!used || !camera) {
            draw.Hide();
        } else {
            const Point eye = CopyPoint(frame.root->convertWorldToLocalPosition(camera->getDerivedPosition()));
            const Ogre::Quaternion orientation =
                frame.root->_getDerivedOrientationUpdated().Inverse() * camera->getDerivedOrientation();
            const Point right = CopyPoint(orientation * Ogre::Vector3::UNIT_X);
            if (!draw.Update(frame.scene, frame.root, eye, right, frame.style, used)) StopVisuals();
        }
    } catch (...) { StopVisuals(); }
}

}
