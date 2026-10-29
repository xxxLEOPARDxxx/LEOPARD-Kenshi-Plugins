#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WorldMarkerProjection.h>
#include "MarkerWidgetPool.h"
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Faction.h>
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <algorithm>
#include <vector>
#include "PowerMarkers.h"

namespace
{
    const int kLimit = 128;
    MarkerWidgetPool<kLimit> g_icons;
    int g_shown = 0;
    DWORD g_lastScan = 0, g_lastWorld = 0, g_lastLog = 0;
    bool g_worldHooked = false;
    struct Candidate { hand target; Ogre::Vector3 position; float distance; };
    std::vector<Candidate> g_candidates;
    bool Nearer(const Candidate& a, const Candidate& b) { return a.distance < b.distance; }
    void Hide(int first)
    {
        for (int i = first; i < g_shown; ++i)
            if (MyGUI::Widget* w = g_icons.Get(i)) if (w->getVisible()) w->setVisible(false);
        g_shown = first;
    }
    bool Warning(Building* b, bool includeNoPower)
    {
        if (b == NULL || b->getFaction() == NULL || !b->getFaction()->isThePlayer()) return false;
        Building::ConstructionState* const state = b->getBuildState();
        if (state != NULL && !state->isComplete) return false;
        UseableStuff* const machine = b->getUseableStuff();
        if (machine == NULL) return false;
        if (!machine->isPowerOn()) return true;
        return includeNoPower && machine->howMuchPowerDoYouWantMax() > 0 && !machine->hasPower();
    }
    bool SafeWarning(Building* b, bool noPower)
    {
        __try { return Warning(b, noPower); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
    void Scan(float range, bool noPower)
    {
        g_candidates.clear();
        static lektor<RootObject*> objects;
        objects.clear();
        const Ogre::Vector3 centre = ou->getCameraCenter();
        ou->getObjectsWithinSphere(objects, centre, range, BUILDING, 8192, NULL);
        for (unsigned i = 0; i < objects.size(); ++i)
        {
            Building* const b = objects[i] == NULL ? NULL : objects[i]->getHandle().getBuilding();
            if (!SafeWarning(b, noPower)) continue;
            Candidate c;
            c.target = b->getHandle(); c.position = b->getPosition();
            c.distance = c.position.squaredDistance(centre);
            if (g_candidates.size() == kLimit) {
                if (c.distance >= g_candidates.front().distance) continue;
                std::pop_heap(g_candidates.begin(), g_candidates.end(), Nearer);
                g_candidates.pop_back();
            }
            g_candidates.push_back(c);
            std::push_heap(g_candidates.begin(), g_candidates.end(), Nearer);
        }
        objects.clear();
        std::sort(g_candidates.begin(), g_candidates.end(), Nearer);
        if (g_candidates.size() > kLimit) g_candidates.resize(kLimit);
    }
    MyGUI::ImageBox* Icon(int i)
    {
        if (MyGUI::Widget* w = g_icons.Get(i)) return static_cast<MyGUI::ImageBox*>(w);
        MyGUI::Gui* const mg = MyGUI::Gui::getInstancePtr();
        if (mg == NULL) return NULL;
        char name[64]; sprintf_s(name, "AssignedWorkers_PowerWarning_%d", i);
        MyGUI::ImageBox* image = mg->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0,0,32,32), MyGUI::Align::Default, "Back", name);
        image->setNeedMouseFocus(false); image->setNeedKeyFocus(false);
        image->setImageTexture("assignedworkers_power.png");
        image->setColour(MyGUI::Colour(1.0f, 0.15f, 0.10f));
        image->setVisible(false);
        g_icons.Set(i, image);
        return image;
    }
    void (*g_origWorld)(GameWorld*, float) = NULL;
    void World(GameWorld* world, float time)
    {
        g_origWorld(world, time);
        const DWORD now=GetTickCount();
        if (g_lastWorld != 0 && now-g_lastWorld > 300) {
            g_lastScan=0; g_candidates.clear();
        }
        g_lastWorld = now;
    }
}
namespace PowerMarkers
{
    bool WorldActive() { return !g_worldHooked || (g_lastWorld != 0 && GetTickCount() - g_lastWorld <= 300); }
    void Install()
    {
        g_worldHooked = KenshiLib::AddHook(KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), World, &g_origWorld) == KenshiLib::SUCCESS;
        if (!g_worldHooked)
            ErrorLog("AssignedWorkers: world power markers unavailable");
    }
    void Tick(bool show, bool noPower, int size, float range, float height, bool debug)
    {
        const DWORD now = GetTickCount();
        if (!show || !g_worldHooked || ou == NULL || gui == NULL || !WorldActive())
        {
            Hide(0); g_lastScan = 0; g_candidates.clear(); return;
        }
        if (!g_lastScan || now - g_lastScan >= 500)
        {
            g_lastScan = now;
            Scan(range, noPower);
            if (debug && (!g_lastLog || now-g_lastLog >= 5000)) {
                g_lastLog=now;
                char note[192]; sprintf_s(note,"AssignedWorkers: power warnings=%u includeNoPower=%d range=%.0f size=%d",(unsigned)g_candidates.size(),noPower ? 1 : 0,range,size);
                DebugLog(note);
            }
        }
        MyGUI::RenderManager* const render = MyGUI::RenderManager::getInstancePtr();
        if (render == NULL) { Hide(0); return; }
        const MyGUI::IntSize view = render->getViewSize();
        int drawn = 0;
        for (size_t i = 0; i < g_candidates.size(); ++i)
        {
            Building* const b = g_candidates[i].target.getBuilding();
            if (b == NULL) continue;
            Ogre::Vector3 p = g_candidates[i].position; p.y += height;
            float x, y;
            if (!WorldMarkerProjection::Project(p, x, y) || x < -size || x > view.width + size || y < 0 || y > view.height + size) continue;
            MyGUI::ImageBox* const icon = Icon(drawn);
            if (icon == NULL) break;
            const MyGUI::IntCoord coord((int)x - size/2, (int)y - size, size, size);
            if (icon->getCoord() != coord) icon->setCoord(coord);
            if (!icon->getVisible()) icon->setVisible(true);
            ++drawn;
        }
        const int previous = g_shown;
        if (drawn < previous) Hide(drawn);
        g_shown = drawn;
    }
}
