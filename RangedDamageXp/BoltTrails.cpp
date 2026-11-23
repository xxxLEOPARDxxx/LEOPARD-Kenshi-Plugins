#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "BoltTrails.h"
#include "BoltTrailHistory.h"

#include <Windows.h>
#include <cstring>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Renderer.h>
#include <ogre/OgreManualObject.h>
#include <ogre/OgreMaterialManager.h>
#include <ogre/OgrePass.h>
#include <ogre/OgreSceneManager.h>
#include <ogre/OgreSceneNode.h>
#include <ogre/OgreTechnique.h>

namespace bolt_trail {
namespace {

const char MaterialName[] = "LEOPARD/RangedDamageXp/BoltTrails";
const unsigned char ForwardQueue = 83; // Kenshi Lighting_HDR particles pass.
const unsigned NodeOffset = 0x58;
const unsigned FlightStateOffset = 0x70;
const unsigned Flying = 1;

History history;
Segment segments[MaxSegments];
void (*originalAdvance)(void*) = NULL;
bool stopped = false;

class Drawing : public Ogre::SceneManager::Listener,
                public Ogre::MovableObject::Listener, public Ogre::Node::Listener
{
    Ogre::SceneManager* scene;
    Ogre::SceneNode* node;
    Ogre::ManualObject* lines;
    bool section, visible;

    void Vertex(const Point& point, float alpha)
    {
        lines->position(point.x, point.y, point.z);
        lines->colour(1.0f, 0.87f, 0.65f, alpha);
    }

    bool EnsureGeometry(Ogre::SceneManager* owner, Ogre::SceneNode* root)
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
                pass->setVertexProgram("Basic_Colour_VP");
                pass->setFragmentProgram("Basic_FP");
                pass->setLightingEnabled(false);
                pass->setDepthCheckEnabled(true);
                pass->setDepthWriteEnabled(false);
                pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
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
            lines->estimateVertexCount(MaxSegments * 2);
            lines->estimateIndexCount(MaxSegments * 2);
        }
        if (!node) {
            node = root->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            node->setListener(this);
        }
        if (!lines->isAttached()) node->attachObject(lines);
        return true;
    }

public:
    Drawing() : scene(NULL), node(NULL), lines(NULL), section(false), visible(false) {}
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
    }

    void sceneManagerDestroyed(Ogre::SceneManager* source) override
    {
        if (source != scene) return;
        // The native owner destroys its objects. No engine calls in this callback.
        scene = NULL;
        node = NULL;
        lines = NULL;
        section = visible = false;
        history.Clear();
    }

    void objectDestroyed(Ogre::MovableObject* source) override
    {
        if (source != lines) return;
        lines = NULL;
        section = visible = false;
        history.Clear();
    }

    void nodeDestroyed(const Ogre::Node* source) override
    {
        if (source != node) return;
        node = NULL;
        visible = false;
        history.Clear();
    }

    bool Update(Ogre::SceneManager* owner, Ogre::SceneNode* root, unsigned used)
    {
        if (scene && scene != owner) ReleaseScene();
        if (!EnsureGeometry(owner, root)) return false;
        if (section) lines->beginUpdate(0);
        else lines->begin(MaterialName, Ogre::RenderOperation::OT_LINE_LIST);
        for (unsigned i = 0; i < used; ++i) {
            Vertex(segments[i].tail, segments[i].tailAlpha);
            Vertex(segments[i].head, segments[i].headAlpha);
        }
        for (unsigned i = 0; i < used * 2; ++i) lines->index(i);
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
    DWORD now;
    bool resolved, sampled;

    Frame(GameWorld* owner) : world(owner), scene(NULL), root(NULL),
                              now(0), resolved(false), sampled(false) {}

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
        now = GetTickCount();
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
    if (frame && !stopped && Field<unsigned>(projectile, FlightStateOffset) == Flying) {
        try {
            Ogre::SceneNode* flightNode = Field<Ogre::SceneNode*>(projectile, NodeOffset);
            if (flightNode && frame->Resolve() && flightNode->getParent() == frame->root) {
                before = Record(*flightNode);
                node = flightNode;
            }
        } catch (...) { stopped = true; }
    }
    originalAdvance(projectile);
    if (!node || stopped) return;
    if (Field<Ogre::SceneNode*>(projectile, NodeOffset) != node) return;
    try {
        if (node->getParent() != frame->root) return;
        history.Sample(reinterpret_cast<uintptr_t>(projectile), before, Record(*node), frame->now);
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
                                   0x81,0xec,0xe8,0,0,0,0x83,0x79,0x70,0x01};
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.TimeDateStamp != 0x65d604d7 ||
        pe->OptionalHeader.SizeOfImage != 0x232c000 || std::memcmp(base + 0x437d40, entry, sizeof(entry)))
        return false;
    return KenshiLib::AddHook(base + 0x437d40, &Advance, &originalAdvance) == KenshiLib::SUCCESS;
}

void RunFrame(GameWorld* world, float time, void (*original)(GameWorld*, float))
{
    if (stopped || activeFrame) {
        original(world, time);
        return;
    }
    Frame frame(world);
    {
        CaptureFrame capture(frame);
        original(world, time);
    }
    if (stopped) {
        StopVisuals();
        return;
    }
    if (!frame.sampled && !draw.Visible()) return;
    try {
        if (!frame.Resolve() || !frame.SceneStillLive()) {
            history.Clear();
            draw.Hide();
            return;
        }
        const unsigned used = history.Build(frame.now, segments);
        if (used) {
            if (!draw.Update(frame.scene, frame.root, used)) StopVisuals();
        } else {
            draw.Hide();
        }
    } catch (...) { StopVisuals(); }
}

}
