// Development-only global-map capture viewer. Does not capture/render the world.
// All owned UI is a child overlay; native mapImage texture/input remain untouched.
#define NOMINMAX
#define KLOC_DOMAIN "global_satellite_map"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <WidgetRef.h>
#include <IniDefaults.h>
#include "CaptureValidation.h"
#include <Debug.h>
#include <core/Functions.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ScrollView.h>
#include <OgreTextureManager.h>
#include <OgreImage.h>
#include <OgreResourceGroupManager.h>
#include <kenshi/Globals.h>
#include <kenshi/SaveFileSystem.h>
#define protected public
#define private public
#include <kenshi/gui/MapScreen.h>
#undef private
#undef protected
#include <cmath>
#include <sstream>
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace
{
const char *const ID = "GlobalSatelliteMap";
const char *const Group = "LEOPARD_GlobalSatelliteMap_Development";
bool enabled = false, debug = false, attempted = false, ready = false, failed = false,
     groupCreated = false, hookFailure = false;
LONG observedGeneration = -1;
DWORD nextTick = 0, statusUntil = 0;
GSM::Session session;
GSM::Manifest manifest;
GSM::ValidationJob *job = NULL;
WidgetRef layer, status, tiles[16];
struct Entry
{
    Ogre::TexturePtr texture;
    std::string name;
    int tile, level;
    DWORD use;
    Entry() : tile(-1), level(-1), use(0) {}
};
Entry cache[16];
unsigned cacheBytes = 0, serial = 0;
void (*originalUpdate)(MapScreen *) = NULL;
void (*originalLoad)(SaveFileSystem *, const std::string &) = NULL;
void (*originalNew)(SaveFileSystem *) = NULL;
std::string Folder()
{
    char p[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), p, MAX_PATH);
    if (!n || n >= MAX_PATH)
        return std::string();
    std::string s(p, n);
    return s.substr(0, s.find_last_of("/\\") + 1);
}
std::string Exe()
{
    char p[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameA(NULL, p, MAX_PATH);
    return !n || n >= MAX_PATH ? std::string() : std::string(p, n);
}
std::string Game()
{
    std::string p = Exe();
    size_t at = p.find_last_of("/\\");
    if (at == p.npos)
        return std::string();
    p.erase(at + 1);
    if (GetFileAttributesA((p + "data/mods.cfg").c_str()) != INVALID_FILE_ATTRIBUTES)
        return p;
    at = p.find_last_of("/\\", p.size() - 2);
    if (at == p.npos)
        return std::string();
    p.erase(at + 1);
    return GetFileAttributesA((p + "data/mods.cfg").c_str()) != INVALID_FILE_ATTRIBUTES
               ? p
               : std::string();
}
std::string Ini()
{
    return IniDefaults::Prepare(Folder() + "GlobalSatelliteMap.ini");
}
void Settings()
{
    std::string ini = Ini();
    enabled = !hookFailure && GetPrivateProfileIntA(ID, "Enabled", 0, ini.c_str()) != 0;
    debug = GetPrivateProfileIntA(ID, "Debug", 0, ini.c_str()) != 0;
    KLocLog::SetEnabled(debug);
    InterlockedIncrement(&GSM::generation);
}
void Diagnostic(const std::string &s)
{
    if (debug)
        DebugLog("GlobalSatelliteMap: " + s);
}
void DropEntry(int i)
{
    Entry &e = cache[i];
    if (e.tile < 0)
        return;
    for (int k = 0; k < 16; ++k)
        if (tiles[k].get())
        {
            MyGUI::ImageBox *w = static_cast<MyGUI::ImageBox *>(tiles[k].get());
            if (w->_getTextureName() == e.name)
            {
                w->setImageTexture("");
                w->setVisible(false);
            }
        }
    MyGUI::RenderManager *rm = MyGUI::RenderManager::getInstancePtr();
    if (rm)
    {
        MyGUI::ITexture *t = rm->getTexture(e.name);
        if (t)
            rm->destroyTexture(t);
    }
    e.texture.setNull();
    Ogre::TextureManager *tm = Ogre::TextureManager::getSingletonPtr();
    if (tm && !tm->getByName(e.name).isNull())
        tm->remove(e.name);
    cacheBytes -= GSM::Bytes(8192 >> e.level);
    e = Entry();
}
void Clear()
{
    ready = false;
    attempted = false;
    failed = false;
    manifest = GSM::Manifest();
    if (layer.get())
        MyGUI::Gui::getInstance().destroyWidget(layer.get());
    layer.reset();
    for (int i = 0; i < 16; ++i)
    {
        tiles[i].reset();
        DropEntry(i);
    }
    if (status.get())
        MyGUI::Gui::getInstance().destroyWidget(status.get());
    status.reset();
    statusUntil = 0;
    cacheBytes = 0;
}
void Notice(MapScreen *map)
{
    if (status.get())
        return;
    MyGUI::Widget *p = map->mainWidget;
    if (!p)
        return;
    MyGUI::TextBox *w = p->createWidget<MyGUI::TextBox>(
        "Kenshi_TextboxStandardText",
        MyGUI::IntCoord(12, 12, p->getWidth() > 24 ? p->getWidth() - 24 : 300, 56),
        MyGUI::Align::Default, "");
    status.set(w);
    w->setNeedMouseFocus(false);
    w->setNeedKeyFocus(false);
    w->setCaption(
        MyGUI::UString(Tr("Satellite view is unavailable: genuine captures for this saved world "
                          "are missing or invalid. The normal map remains visible.")));
    statusUntil = GetTickCount() + 12000;
}
void Fail(MapScreen *map, const std::string &why)
{
    Clear();
    attempted = true;
    failed = true;
    ErrorLog("GlobalSatelliteMap: " + why);
    Notice(map);
}
void StartValidation(MapScreen *map, const std::string &save)
{
    attempted = true;
    std::string text, err;
    GSM::Manifest check;
    if (!GSM::ReadSmall(Folder() + "captures/manifest.gsm", text, 65536) ||
        !GSM::Parse(text, check, err))
    {
        Fail(map, "genuine capture manifest missing or invalid; satellite overlay disabled");
        return;
    }
    job = new GSM::ValidationJob();
    job->folder = Folder() + "captures/";
    job->savePath = save;
    job->game = Game();
    job->engine = Exe();
    job->generation = InterlockedCompareExchange(&GSM::generation, 0, 0);
    if (job->game.empty() || job->engine.empty())
    {
        delete job;
        job = NULL;
        Fail(map, "cannot identify game input directory");
        return;
    }
    job->thread = CreateThread(NULL, 0, &GSM::ValidateCapture, job, 0, NULL);
    if (!job->thread)
    {
        delete job;
        job = NULL;
        Fail(map, "could not start bounded capture validation");
    }
    else
        Diagnostic("validating capture and exact native save in file worker");
}
bool Poll(MapScreen *map)
{
    if (!job)
        return true;
    if (WaitForSingleObject(job->thread, 0) != WAIT_OBJECT_0)
        return false;
    CloseHandle(job->thread);
    bool current = job->generation == InterlockedCompareExchange(&GSM::generation, 0, 0);
    bool ok = job->ok;
    std::string error = job->error;
    if (current && ok)
    {
        manifest = job->manifest;
        ready = true;
        Diagnostic("validated static capture preview; native dynamic lighting remains pending");
    }
    delete job;
    job = NULL;
    if (current && !ok)
        Fail(map, error);
    return true;
}
bool Geometry(MapScreen *map, GSM::Rect out[16])
{
    if (!map->mapImage || !map->mapScrollView)
        return false;
    MyGUI::ImageBox *image = map->mapImage;
    int w = image->getWidth(), h = image->getHeight();
    if (w <= 0 || h <= 0 || w > 1000000 || h > 1000000)
        return false;
    const double d = 20000;
    MyGUI::IntPoint a = map->worldToMapCoords(Ogre::Vector3(0, 0, 0));
    MyGUI::IntPoint x = map->worldToMapCoords(Ogre::Vector3(float(d), 0, 0));
    MyGUI::IntPoint z = map->worldToMapCoords(Ogre::Vector3(0, 0, float(d)));
    double ax = (x.left - a.left) / d, bx = (z.left - a.left) / d, az = (x.top - a.top) / d,
           bz = (z.top - a.top) / d;
    if (std::fabs(bx) > 1e-8 || std::fabs(az) > 1e-8 || ax <= 0 || bz <= 0)
        return false;
    for (int tz = 0; tz < 4; ++tz)
        for (int tx = 0; tx < 4; ++tx)
        {
            double x0 = manifest.minX + (manifest.maxX - manifest.minX) * tx / 4,
                   x1 = manifest.minX + (manifest.maxX - manifest.minX) * (tx + 1) / 4;
            double z0 = manifest.minZ + (manifest.maxZ - manifest.minZ) * tz / 4,
                   z1 = manifest.minZ + (manifest.maxZ - manifest.minZ) * (tz + 1) / 4;
            double L = a.left + ax * x0, T = a.top + bz * z0, R = a.left + ax * x1,
                   B = a.top + bz * z1;
            if (!GSM::Finite(L) || !GSM::Finite(T) || !GSM::Finite(R) || !GSM::Finite(B) ||
                L < -1e6 || T < -1e6 || R > 1e6 || B > 1e6)
                return false;
            int l = int(std::floor(L + 0.5)), t = int(std::floor(T + 0.5)),
                r = int(std::floor(R + 0.5)), b = int(std::floor(B + 0.5));
            out[tz * 4 + tx] = GSM::Rect(l, t, r - l, b - t);
        }
    return true;
}
void EnsureLayer(MapScreen *map)
{
    MyGUI::Widget *image = map->mapImage;
    if (layer.get() && layer.get()->getParent() != image)
    {
        MyGUI::Gui::getInstance().destroyWidget(layer.get());
        layer.reset();
    }
    if (!layer.get())
    {
        MyGUI::Widget *w = image->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(0, 0, image->getWidth(), image->getHeight()),
            MyGUI::Align::Default, "");
        layer.set(w);
        w->setDepth(1000000);
        w->setNeedMouseFocus(false);
        w->setNeedKeyFocus(false);
        for (int i = 0; i < 16; ++i)
        {
            MyGUI::ImageBox *t = w->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(),
                                                                  MyGUI::Align::Default, "");
            tiles[i].set(t);
            t->setNeedMouseFocus(false);
            t->setNeedKeyFocus(false);
            t->setVisible(false);
        }
    }
    else if (layer.get()->getSize() != image->getSize())
        layer.get()->setSize(image->getSize());
}
int Find(int tile, int level)
{
    for (int i = 0; i < 16; ++i)
        if (cache[i].tile == tile && cache[i].level == level)
            return i;
    return -1;
}
int Upload(int tile, int level, const bool visible[16])
{
    const unsigned bytes = GSM::Bytes(8192 >> level);
    int slot = -1;
    for (;;)
    {
        for (int i = 0; i < 16; ++i)
            if (cache[i].tile < 0)
            {
                slot = i;
                break;
            }
        if (slot >= 0 && cacheBytes <= GSM::CacheBudget - bytes)
            break;
        int victim = -1;
        for (int i = 0; i < 16; ++i)
            if (cache[i].tile >= 0 && (!visible[cache[i].tile] || cache[i].level != level) &&
                (victim < 0 || cache[i].use < cache[victim].use))
                victim = i;
        if (victim < 0)
            return -1;
        DropEntry(victim);
    }
    const GSM::Asset &a = manifest.At(tile, level);
    if (!groupCreated)
    {
        Ogre::ResourceGroupManager &r = Ogre::ResourceGroupManager::getSingleton();
        r.createResourceGroup(Group);
        r.addResourceLocation(Folder() + "captures/", "FileSystem", Group);
        groupCreated = true;
    }
    WIN32_FILE_ATTRIBUTE_DATA before, after;
    const std::string file = Folder() + "captures/" + a.file;
    if (!GetFileAttributesExA(file.c_str(), GetFileExInfoStandard, &before) ||
        ((static_cast<unsigned long long>(before.ftLastWriteTime.dwHighDateTime) << 32) |
         before.ftLastWriteTime.dwLowDateTime) != a.stamp)
        throw std::runtime_error("capture texture changed after validation");
    Ogre::Image image;
    image.load(a.file, Group);
    if (!GetFileAttributesExA(file.c_str(), GetFileExInfoStandard, &after) ||
        CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime) ||
        before.nFileSizeLow != after.nFileSizeLow || before.nFileSizeHigh != after.nFileSizeHigh)
        throw std::runtime_error("capture texture changed during loading");
    if (image.getWidth() != unsigned(a.size) || image.getHeight() != unsigned(a.size) ||
        image.getFormat() != Ogre::PF_DXT1 || image.getNumMipmaps() != 0)
        throw std::runtime_error("capture texture changed after validation");
    std::ostringstream name;
    name << "GlobalSatelliteMap_" << observedGeneration << "_" << ++serial;
    Entry &e = cache[slot];
    Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().loadImage(
        name.str(), Group, image, Ogre::TEX_TYPE_2D, 0);
    if (texture.isNull() || texture->getFormat() != Ogre::PF_DXT1 || texture->getNumMipmaps() != 0)
    {
        if (!texture.isNull())
            Ogre::TextureManager::getSingleton().remove(texture->getName());
        throw std::runtime_error("native renderer did not retain BC1 capture format");
    }
    e.texture = texture;
    e.name = name.str();
    e.tile = tile;
    e.level = level;
    e.use = GetTickCount();
    cacheBytes += bytes;
    return slot;
}
void Draw(MapScreen *map)
{
    GSM::Rect coords[16];
    if (!Geometry(map, coords))
        throw std::runtime_error("native map projection is not ready or unsupported");
    EnsureLayer(map);
    MyGUI::IntCoord view = map->mapScrollView->getAbsoluteCoord();
    MyGUI::IntPoint origin = map->mapImage->getAbsolutePosition();
    GSM::Rect clip(view.left - origin.left, view.top - origin.top, view.width, view.height);
    bool visible[16] = {false};
    int count = 0, display = 1;
    for (int i = 0; i < 16; ++i)
    {
        visible[i] = GSM::Intersects(coords[i], clip);
        if (visible[i])
        {
            ++count;
            if (coords[i].w > display)
                display = coords[i].w;
            if (coords[i].h > display)
                display = coords[i].h;
        }
    }
    if (!count)
        return;
    int level = GSM::ChooseLevel(count, display, GSM::CacheBudget);
    bool uploaded = false;
    for (int i = 0; i < 16; ++i)
    {
        MyGUI::ImageBox *w = static_cast<MyGUI::ImageBox *>(tiles[i].get());
        if (!w)
            continue;
        if (!visible[i])
        {
            if (w->getVisible())
                w->setVisible(false);
            continue;
        }
        int c = Find(i, level);
        if (c < 0 && !uploaded)
        {
            uploaded = true;
            c = Upload(i, level, visible);
        }
        if (c < 0)
        {
            if (w->getVisible())
                w->setVisible(false);
            continue;
        }
        cache[c].use = GetTickCount();
        MyGUI::IntCoord rect(coords[i].x, coords[i].y, coords[i].w, coords[i].h);
        if (w->getCoord() != rect)
            w->setCoord(rect);
        if (w->_getTextureName() != cache[c].name)
            w->setImageTexture(cache[c].name);
        if (!w->getVisible())
            w->setVisible(true);
    }
}
void Tick(MapScreen *map)
{
    LONG gen = InterlockedCompareExchange(&GSM::generation, 0, 0);
    if (gen != observedGeneration)
    {
        Clear();
        session.Reset();
        observedGeneration = gen;
    }
    if (!map || !map->getVisible() || !enabled)
    {
        if (layer.get())
            layer.get()->setVisible(false);
        if (status.get())
            status.get()->setVisible(false);
        return;
    }
    DWORD now = GetTickCount();
    if (status.get() && statusUntil && static_cast<LONG>(now - statusUntil) >= 0)
    {
        MyGUI::Gui::getInstance().destroyWidget(status.get());
        status.reset();
        statusUntil = 0;
    }
    if (static_cast<LONG>(now - nextTick) < 0)
        return;
    nextTick = now + 200;
    SaveFileSystem *fs = SaveFileSystem::getSingleton();
    if (!fs || fs->busy() || !ou)
        return;
    std::string save = fs->getActiveSave();
    if (save.empty())
    {
        if (!attempted)
            Fail(map, "satellite preview requires a saved world");
        return;
    }
    char full[MAX_PATH] = {0};
    DWORD n = GetFullPathNameA(save.c_str(), MAX_PATH, full, NULL);
    if (!n || n >= MAX_PATH)
        return;
    save.assign(full, n);
    for (size_t i = 0; i < save.size(); ++i)
    {
        if (save[i] == '\\')
            save[i] = '/';
        else if (save[i] >= 'A' && save[i] <= 'Z')
            save[i] = save[i] - 'A' + 'a';
    }
    if (save[save.size() - 1] != '/')
        save += '/';
    if (session.Change(save, reinterpret_cast<unsigned long long>(ou)))
    {
        Clear();
        InterlockedIncrement(&GSM::generation);
        observedGeneration = InterlockedCompareExchange(&GSM::generation, 0, 0);
    }
    if (!Poll(map))
        return;
    if (!attempted && !job)
        StartValidation(map, save);
    if (!ready)
        return;
    Draw(map);
    if (layer.get() && !layer.get()->getVisible())
        layer.get()->setVisible(true);
}
void UpdateHook(MapScreen *map)
{
    originalUpdate(map);
    try
    {
        Tick(map);
    }
    catch (const std::exception &e)
    {
        Fail(map, e.what());
    }
    catch (...)
    {
        Fail(map, "map overlay failed; normal map retained");
    }
}
void LoadHook(SaveFileSystem *self, const std::string &path)
{
    InterlockedIncrement(&GSM::generation);
    originalLoad(self, path);
}
void NewHook(SaveFileSystem *self)
{
    InterlockedIncrement(&GSM::generation);
    originalNew(self);
}
} // namespace
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api *api)
{
    static std::string ini = Ini();
    api->beginMod(api, ID, "Global Satellite Map (development)", ini.c_str(), &Settings);
    if (api->version >= 2)
        api->info(
            api, Tr("Development prototype: displays verified static captures on the global map. "
                    "Native dynamic lighting and unloaded buildings are still under development."));
    api->section(api, Tr("Global map"));
    api->toggle(api, ID, "Enabled", Tr("Satellite view"),
                Tr("Optional global-map layer. Requires genuine captures for this saved world; the "
                   "minimap keeps its existing map source."),
                0, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, ID, "Debug", Tr("Detailed log"),
                Tr("Write detailed diagnostics to RE_Kenshi_log.txt. Errors are always logged."), 0,
                0);
}
// RE_Kenshi resolves the decorated C++ name ?startPlugin@@YAXXZ.
__declspec(dllexport) void startPlugin()
{
    Settings();
    if (KenshiLib::SUCCESS !=
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame), LoadHook,
                               &originalLoad) ||
        KenshiLib::SUCCESS !=
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame), NewHook,
                               &originalNew) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update),
                                                 UpdateHook, &originalUpdate))
    {
        enabled = false;
        hookFailure = true;
        ErrorLog("GlobalSatelliteMap: required hooks unavailable; development overlay disabled");
    }
}
