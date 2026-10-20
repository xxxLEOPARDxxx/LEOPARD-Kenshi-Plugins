// Satellite - вид со спутника: снимки мира сверху плитками.
//
// КАК СНИМАЕМ. Игра рисует мир через набор компоновщиков Ogre 2.0
// (CompositorWorkspace): тени, отложенное освещение, туман, постобработка.
// Снимок - второй экземпляр того же набора (определение берём у рабочего
// набора игры, Renderer::workspace), но с нашей текстурой вместо окна.
// Картинка - как в игре, с модами на графику.
//
// КАМЕРА - только камера игры. Свою (ортогональную, перспективную, на своём
// узле сцены) игра не обслуживает: пробы 1, 2 и 4 (20.10.2026) дали мир
// чёрным или пустым - освещение, тени и что видно игра готовит под свою
// главную камеру. Камера игры, перенесённая наверх, даёт настоящий вид
// сверху (проба 3).
//
// БЕЗ МИГАНИЯ. Ogre рисует наборы по очереди в одном кадре: наш - первым,
// окно игры - после. Слушатель нашего набора перед его отрисовкой ставит
// камеру игры наверх (и на кадр снимка прячет интерфейс и курсор: MyGUI
// рисуется в любую цель рендера, выключенные оверлеи не помогли - проба
// 4), слушатель набора окна перед его отрисовкой всё возвращает. Экран вид
// сверху не видит (проба 5). Пока не снимаем, наш набор выключен.
//
// ПЛИТКИ. Мир - сетка квадратов по 1024 м. Качество (пикселей на сторону
// плитки) - 1024, 2048, 4096 или 8192: больше 1024 снимаем мозаикой из
// снимков 1024x1024 со сдвигом камеры (при 8192 - 8x8), так видеопамять не
// растёт с качеством. Камера - с узким углом (12 градусов) высоко над
// землёй: почти ортогональная, на холмах шов между плитками - метры.
//
// БЛИЖНИЕ И ДАЛЬНИЕ. Постройки и мелочь игра рисует только около камеры
// (до ~1,5 км). Ближние плитки снимаем в выбранном качестве, дальние (до
// 4 км) - быстро, в 1024, с пометкой: подойдёт камера - переснимем.
//
// ДЕНЬ И НОЧЬ. У плитки два варианта: дневной (снимаем с 9 до 16 игровых
// часов) и ночной (с 22 до 4, с огнями фонарей). Миникарта берёт по времени
// суток, на рассвете и закате смешивает.
//
// ХРАНЕНИЕ. Файл на плитку и вариант в папке satellite мода: сжатие DXT1
// (как у текстур игры - полбайта на пиксель) и уменьшенные копии до 4x4 -
// миникарта читает с диска только нужный под масштаб размер. Сжатие и
// запись - в своём потоке (см. «Сжатие снимков»), запись - через временный
// файл: поток рисования не прочтёт недописанное.
//
// ПАМЯТЬ ПОТОКА РИСОВАНИЯ. Прочитанные уровни плиток - в кэше до 192 МБ, по
// давности использования; отданное в текущем кадре не выбрасывается.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include <Debug.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_PointerManager.h>

#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTexture.h>
#include <OgreResourceGroupManager.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>
#include <Compositor/OgreCompositorWorkspaceListener.h>

#include <kenshi/Globals.h>
#include <kenshi/KingOfRenderThread.h>
#include <kenshi/Renderer.h>
#include <kenshi/util/UtilityT.h>

#include "Satellite.h"

namespace
{
    template <class T> T Lo(T a, T b) { return a < b ? a : b; }
    template <class T> T Hi(T a, T b) { return a > b ? a : b; }

    void Log(const std::string& text)
    {
        DebugLog("BestMinimap: satellite: " + text);
    }

    // ---------------------------------------------------------------
    // DXT1: сжатие и разжатие блока 4x4
    // ---------------------------------------------------------------

    inline unsigned To565(int r, int g, int b)
    {
        return static_cast<unsigned>(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | ((b * 31 + 127) / 255));
    }

    inline void From565(unsigned c, int& r, int& g, int& b)
    {
        r = ((c >> 11) & 31) * 255 / 31;
        g = ((c >> 5) & 63) * 255 / 63;
        b = (c & 31) * 255 / 31;
    }

    // Блок 4x4 (ARGB, шаг строки stride) - в 8 байт DXT1. Концы - углы
    // ограничивающего куба цветов, чуть внутрь; индексы - проекцией на
    // отрезок между ними. Для снимков местности хватает.
    void EncodeBlock(const unsigned* px, int stride, unsigned char out[8])
    {
        int mn[3] = { 255, 255, 255 }, mx[3] = { 0, 0, 0 };
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
            {
                const unsigned c = px[y * stride + x];
                const int ch[3] = { static_cast<int>((c >> 16) & 255), static_cast<int>((c >> 8) & 255),
                                    static_cast<int>(c & 255) };
                for (int k = 0; k < 3; ++k)
                {
                    mn[k] = Lo(mn[k], ch[k]);
                    mx[k] = Hi(mx[k], ch[k]);
                }
            }
        for (int k = 0; k < 3; ++k)
        {
            const int inset = (mx[k] - mn[k]) / 16;
            mn[k] += inset;
            mx[k] -= inset;
        }
        unsigned c0 = To565(mx[0], mx[1], mx[2]), c1 = To565(mn[0], mn[1], mn[2]);
        unsigned indices = 0;
        if (c0 != c1)
        {
            if (c0 < c1)
            {
                const unsigned t = c0;
                c0 = c1;
                c1 = t;
            }
            int a[3], b[3];
            From565(c0, a[0], a[1], a[2]);
            From565(c1, b[0], b[1], b[2]);
            const int d[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
            const int len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
            // Доля пути от c1 к c0 -> индекс DXT1: 1 (c1), 3 (1/3), 2 (2/3), 0 (c0).
            static const unsigned kIndexOf[4] = { 1, 3, 2, 0 };
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 4; ++x)
                {
                    const unsigned c = px[y * stride + x];
                    const int p[3] = { static_cast<int>((c >> 16) & 255) - b[0], static_cast<int>((c >> 8) & 255) - b[1],
                                       static_cast<int>(c & 255) - b[2] };
                    const int dot = p[0] * d[0] + p[1] * d[1] + p[2] * d[2];
                    int step = len2 > 0 ? (dot * 3 + len2 / 2) / len2 : 0;
                    step = step < 0 ? 0 : (step > 3 ? 3 : step);
                    indices |= kIndexOf[step] << (2 * (y * 4 + x));
                }
        }
        out[0] = static_cast<unsigned char>(c0 & 255);
        out[1] = static_cast<unsigned char>(c0 >> 8);
        out[2] = static_cast<unsigned char>(c1 & 255);
        out[3] = static_cast<unsigned char>(c1 >> 8);
        out[4] = static_cast<unsigned char>(indices & 255);
        out[5] = static_cast<unsigned char>((indices >> 8) & 255);
        out[6] = static_cast<unsigned char>((indices >> 16) & 255);
        out[7] = static_cast<unsigned char>(indices >> 24);
    }

    // 8 байт DXT1 -> блок 4x4 ARGB (непрозрачный) в dst с шагом строки stride.
    void DecodeBlock(const unsigned char* p, unsigned* dst, int stride)
    {
        const unsigned c0 = p[0] | (p[1] << 8), c1 = p[2] | (p[3] << 8);
        int r[4], g[4], b[4];
        From565(c0, r[0], g[0], b[0]);
        From565(c1, r[1], g[1], b[1]);
        if (c0 > c1)
        {
            r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
            r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;
        }
        else
        {
            r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2;
            r[3] = g[3] = b[3] = 0;
        }
        unsigned colour[4];
        for (int k = 0; k < 4; ++k)
            colour[k] = 0xFF000000u | (static_cast<unsigned>(r[k]) << 16) | (static_cast<unsigned>(g[k]) << 8) |
                        static_cast<unsigned>(b[k]);
        const unsigned idx = p[4] | (p[5] << 8) | (p[6] << 16) | (static_cast<unsigned>(p[7]) << 24);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                dst[y * stride + x] = colour[(idx >> (2 * (y * 4 + x))) & 3];
    }

    // ---------------------------------------------------------------
    // Файл плитки
    // ---------------------------------------------------------------
    //
    // «BMS2», качество, флаги, число уровней, размеры уровней (int каждый),
    // затем блоки DXT1 уровней подряд, строками блоков. «BMS1» (первая
    // версия) - то же без флагов.

    const int kShot = 1024;             // снимок за раз, пикселей в дело
    const int kMargin = 64;             // и поля вокруг - для подгонки яркости к соседям
    const int kFrame = kShot + 2 * kMargin;
    enum { FLAG_RETAKE = 1 };           // снят наспех (издалека или в сумерки) - переснять
    const int kMinLevel = 4;            // самый мелкий уровень плитки
    std::string g_folder;               // папка плиток (с «\» в конце)

    int BlocksBytes(int size)
    {
        return (size / 4) * (size / 4) * 8;
    }

    void Decode(long long key, int& tx, int& tz, Satellite::Variant& v)
    {
        tx = static_cast<int>((key >> 34) & 0xFFFF) - 32768;
        tz = static_cast<int>((key >> 2) & 0xFFFF) - 32768;
        v = static_cast<Satellite::Variant>(key & 3);
    }

    std::string TileFile(int tx, int tz, Satellite::Variant v)
    {
        char name[96];
        sprintf_s(name, "tile_%d_%d_%s.bms", tx, tz, v == Satellite::DAY ? "day" : "night");
        return g_folder + name;
    }

    struct Header
    {
        int quality;
        int flags;
        std::vector<int> sizes;
        std::vector<long> offsets;
        Header() : quality(0), flags(0) {}
    };

    bool ReadHeader(FILE* f, Header& h)
    {
        char magic[4];
        int levels = 0;
        h.flags = 0;
        if (fread(magic, 1, 4, f) != 4)
            return false;
        const bool v2 = memcmp(magic, "BMS2", 4) == 0;
        if (!v2 && memcmp(magic, "BMS1", 4) != 0)
            return false;
        if (fread(&h.quality, 4, 1, f) != 1 || (v2 && fread(&h.flags, 4, 1, f) != 1) ||
            fread(&levels, 4, 1, f) != 1 || levels <= 0 || levels > 16)
            return false;
        h.sizes.resize(levels);
        if (fread(&h.sizes[0], 4, levels, f) != static_cast<size_t>(levels))
            return false;
        h.offsets.resize(levels);
        long at = (v2 ? 16 : 12) + 4 * levels;
        for (int i = 0; i < levels; ++i)
        {
            if (h.sizes[i] < kMinLevel || h.sizes[i] > 8192 || h.sizes[i] % 4 != 0)
                return false;
            h.offsets[i] = at;
            at += BlocksBytes(h.sizes[i]);
        }
        return true;
    }


    // ---------------------------------------------------------------
    // Поток игры: какие плитки есть
    // ---------------------------------------------------------------

    struct Stored
    {
        int quality;
        bool retake;
        Stored() : quality(0), retake(false) {}
        Stored(int q, bool f) : quality(q), retake(f) {}
    };
    std::map<long long, Stored> g_stored;   // что снято
    std::vector<long long> g_fresh;         // снятые, ещё не отданные потоку рисования
    bool g_capture = true;
    int g_quality = 1024;
    bool g_scanned = false;

    // Имя файла плитки -> ключ; false - не наш файл.
    bool KeyOfFile(const char* name, long long& key)
    {
        int tx = 0, tz = 0;
        char variant[16] = {};
        if (sscanf_s(name, "tile_%d_%d_%15[a-z].bms", &tx, &tz, variant, static_cast<unsigned>(sizeof(variant))) != 3)
            return false;
        key = Satellite::Key(tx, tz, strcmp(variant, "night") == 0 ? Satellite::NIGHT : Satellite::DAY);
        return true;
    }

    void ScanFolder()
    {
        g_scanned = true;
        g_stored.clear();
        CreateDirectoryA(g_folder.c_str(), NULL);
        WIN32_FIND_DATAA fd;
        HANDLE const h = FindFirstFileA((g_folder + "tile_*.bms").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return;
        do
        {
            long long key = 0;
            if (!KeyOfFile(fd.cFileName, key))
                continue;
            FILE* f = NULL;
            if (fopen_s(&f, (g_folder + fd.cFileName).c_str(), "rb") != 0 || f == NULL)
                continue;
            Header hd;
            if (ReadHeader(f, hd))
                g_stored[key] = Stored(hd.quality, (hd.flags & FLAG_RETAKE) != 0);
            fclose(f);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        char note[160];
        sprintf_s(note, "%d day and %d night tiles in %s", Satellite::StoredCount(Satellite::DAY),
                  Satellite::StoredCount(Satellite::NIGHT), g_folder.c_str());
        Log(note);
    }


    // ---------------------------------------------------------------
    // Поток игры: съёмка
    // ---------------------------------------------------------------

    // Имя набора компоновщиков игры - в защищённых полях Ogre; берём через
    // указатель на член, полученный в наследнике (законный путь к protected).
    struct WorkspaceAccess : public Ogre::CompositorWorkspace
    {
        static const Ogre::CompositorWorkspaceDef* Def(const Ogre::CompositorWorkspace* w)
        {
            return w->*(&WorkspaceAccess::mDefinition);
        }
    };

    struct WorkspaceDefAccess : public Ogre::CompositorWorkspaceDef
    {
        static Ogre::IdString Name(const Ogre::CompositorWorkspaceDef* d)
        {
            return d->*(&WorkspaceDefAccess::mName);
        }
    };

    const float kFov = 12.0f;           // угол, под которым с высоты съёмки видна сторона снимка, градусов
    const int kSettleFirst = 8;         // кадров до первого снимка плитки: свет, адаптация яркости
    const int kSettleNext = 2;          // и между снимками мозаики

    Renderer* GameRenderer()
    {
        return ::au == NULL ? NULL : ::au->render;
    }

    Ogre::TexturePtr g_target;
    Ogre::CompositorWorkspace* g_workspace = NULL;     // наш набор (выключен, пока не снимаем)
    Ogre::CompositorWorkspace* g_workspaceFor = NULL;  // набор игры, с которого он сделан
    Ogre::CompositorWorkspace* g_gameWorkspace = NULL; // набор окна игры, на котором наш слушатель

    // Текущая плитка.
    struct Job
    {
        bool active;
        int tx, tz;
        Satellite::Variant variant;
        int quality, n;                 // n x n снимков
        int shot;                       // который снимаем
        int wait;                       // кадров до чтения
        float groundY;                  // высота земли в центре плитки (мир)
        bool retake;                    // наспех (см. FLAG_RETAKE)
        DWORD started;
        Job() : active(false), tx(0), tz(0), variant(Satellite::DAY), quality(0), n(1), shot(0), wait(0),
                groundY(0), retake(false), started(0) {}
    };
    Job g_job;
    DWORD g_nextPlan = 0;
    LONG g_gen = 0;                     // номер съёмки: после загрузки сейва старое не нужно
    bool g_encoding = false;            // поток сжатия ещё дописывает прошлую плитку

    // ---- камера игры: наверх и обратно ----
    struct CameraState
    {
        Ogre::Vector3 position;
        Ogre::Quaternion orientation;
        Ogre::Radian fov;
        float nearClip, farClip, aspect;
        bool saved;
        CameraState() : nearClip(0), farClip(0), aspect(0), saved(false) {}
    };
    CameraState g_saved;

    // Над центром снимка (мир), взглядом вниз, север (-Z мира) - вверху.
    // Высота - такая, чтобы квадрат side на уровне земли влез ровно в кадр.
    // Камера висит на узле сцены - место и поворот задаём относительно узла.
    void AimDown(const Ogre::Vector3& centre, float side)
    {
        Renderer* const r = GameRenderer();
        Ogre::Camera* const cam = r->camera;
        if (!g_saved.saved)
        {
            g_saved.position = cam->getPosition();
            g_saved.orientation = cam->getOrientation();
            g_saved.fov = cam->getFOVy();
            g_saved.nearClip = cam->getNearClipDistance();
            g_saved.farClip = cam->getFarClipDistance();
            g_saved.aspect = cam->getAspectRatio();
            g_saved.saved = true;
        }
        const float height = side * 0.5f / std::tan(kFov * 0.5f * 3.14159265f / 180.0f);
        // Сцена сдвинута относительно мира (числа у камеры мелкие): мировая
        // точка в сцене - минус этот сдвиг.
        const Ogre::Vector3 scene = centre - r->getSceneRelativeOrigin() + Ogre::Vector3(0.0f, height, 0.0f);
        // Камера смотрит по -Z; поворот на -90 градусов вокруг X - взгляд вниз
        // (-Y), «верх» снимка - -Z, север.
        const Ogre::Quaternion down(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_X);
        Ogre::Vector3 position = scene;
        Ogre::Quaternion orientation = down;
        if (Ogre::Node* const parent = cam->getParentNode())
        {
            const Ogre::Quaternion inv = parent->_getDerivedOrientation().Inverse();
            position = inv * (scene - parent->_getDerivedPosition());
            orientation = inv * down;
        }
        cam->setPosition(position);
        cam->setOrientation(orientation);
        // Кадр - со стороной побольше: поля kMargin вокруг снимка.
        const float frame = side * kFrame / kShot;
        cam->setFOVy(Ogre::Radian(2.0f * std::atan(frame * 0.5f / height)));
        cam->setAspectRatio(1.0f);
        // Глубина - только вокруг земли: так её точности хватает и с
        // десятка километров.
        cam->setNearClipDistance(height > 4005.0f ? height - 4000.0f : 5.0f);
        cam->setFarClipDistance(height + 4000.0f);
    }

    void RestoreCamera()
    {
        if (!g_saved.saved)
            return;
        Renderer* const r = GameRenderer();
        if (r != NULL && r->camera != NULL)
        {
            Ogre::Camera* const cam = r->camera;
            cam->setPosition(g_saved.position);
            cam->setOrientation(g_saved.orientation);
            cam->setFOVy(g_saved.fov);
            cam->setNearClipDistance(g_saved.nearClip);
            cam->setFarClipDistance(g_saved.farClip);
            cam->setAspectRatio(g_saved.aspect);
        }
        g_saved.saved = false;
    }

    // ---- интерфейс: спрятать на наш набор, вернуть к окну ----
    std::vector<MyGUI::Widget*> g_hiddenGui;
    bool g_pointerHidden = false;

    void HideGui()
    {
        g_hiddenGui.clear();
        MyGUI::EnumeratorWidgetPtr roots = MyGUI::Gui::getInstance().getEnumerator();
        while (roots.next())
        {
            MyGUI::Widget* const w = roots.current();
            if (w != NULL && w->getVisible())
            {
                w->setVisible(false);
                g_hiddenGui.push_back(w);
            }
        }
        MyGUI::PointerManager& pointer = MyGUI::PointerManager::getInstance();
        if (pointer.isVisible())
        {
            pointer.setVisible(false);
            g_pointerHidden = true;
        }
    }

    // Окна за это время могли удалить - возвращаем только тем, кто ещё
    // среди корневых.
    void RestoreGui()
    {
        if (!g_hiddenGui.empty())
        {
            MyGUI::EnumeratorWidgetPtr roots = MyGUI::Gui::getInstance().getEnumerator();
            while (roots.next())
            {
                MyGUI::Widget* const w = roots.current();
                for (size_t i = 0; i < g_hiddenGui.size(); ++i)
                    if (g_hiddenGui[i] == w)
                    {
                        w->setVisible(true);
                        break;
                    }
            }
            g_hiddenGui.clear();
        }
        if (g_pointerHidden)
        {
            MyGUI::PointerManager::getInstance().setVisible(true);
            g_pointerHidden = false;
        }
    }

    // Центр текущего снимка мозаики и его сторона (мир).
    void ShotArea(Ogre::Vector3& centre, float& side)
    {
        side = Satellite::kTileMetres / g_job.n;
        const int sx = g_job.shot % g_job.n, sz = g_job.shot / g_job.n;
        const float x = g_job.tx * Satellite::kTileMetres + (sx + 0.5f) * side;
        const float z = g_job.tz * Satellite::kTileMetres + (sz + 0.5f) * side;
        // Высота земли - под этим снимком: чем ближе к ней плоскость, на
        // которую камера «наведена», тем меньше сдвиг холмов на краях.
        const float ground = UtilityT::getTerrainHeight(x, z);
        centre = Ogre::Vector3(x, (ground == ground && ground > -100000.0f && ground < 100000.0f) ? ground : g_job.groundY, z);
    }

    // ---- слушатели наборов ----
    // Наш набор: перед отрисовкой - камера наверх, на кадр снимка - без
    // интерфейса.
    struct ShotListener : public Ogre::CompositorWorkspaceListener
    {
        virtual void workspacePreUpdate(Ogre::CompositorWorkspace*)
        {
            if (!g_job.active)
                return;
            Ogre::Vector3 centre;
            float side = 0.0f;
            ShotArea(centre, side);
            AimDown(centre, side);
            if (g_job.wait == 1)
                HideGui();
        }
    };
    ShotListener g_shotListener;

    // Набор окна: перед отрисовкой - всё назад, дальше - прежнему слушателю.
    struct RestoreListener : public Ogre::CompositorWorkspaceListener
    {
        Ogre::CompositorWorkspaceListener* next;
        RestoreListener() : next(NULL) {}
        virtual void workspacePreUpdate(Ogre::CompositorWorkspace* w)
        {
            RestoreCamera();
            RestoreGui();
            if (next != NULL)
                next->workspacePreUpdate(w);
        }
        virtual void passPreExecute(Ogre::CompositorPass* pass)
        {
            if (next != NULL)
                next->passPreExecute(pass);
        }
    };
    RestoreListener g_restoreListener;

    void Unchain()
    {
        if (g_gameWorkspace != NULL)
        {
            // Снимаем свой слушатель, только если он всё ещё наш.
            if (g_gameWorkspace->getListener() == &g_restoreListener)
                g_gameWorkspace->setListener(g_restoreListener.next);
            g_gameWorkspace = NULL;
        }
    }

    // Набор - выключить; он остаётся до смены набора игры или загрузки.
    void StopShooting()
    {
        Unchain();
        if (g_workspace != NULL)
            g_workspace->setEnabled(false);
        RestoreCamera();
        RestoreGui();
        g_job.active = false;
    }

    void DropWorkspace()
    {
        StopShooting();
        Renderer* const r = GameRenderer();
        if (g_workspace != NULL && r != NULL && r->root != NULL)
            r->root->getCompositorManager2()->removeWorkspace(g_workspace);
        g_workspace = NULL;
        g_workspaceFor = NULL;
    }

    // Наш набор - включить (создать, если нет или набор игры сменился) и
    // встать слушателем на набор окна.
    bool StartShooting()
    {
        Renderer* const r = GameRenderer();
        if (r == NULL || r->root == NULL || r->scene == NULL || r->workspace == NULL || r->camera == NULL)
            return false;
        // Набор игры сменился (настройки графики) - наш, сделанный со
        // старого, забываем, не удаляя: игра могла удалить все наборы
        // сама, и указатель уже не годится. Выключенный - безвреден.
        if (g_workspace != NULL && g_workspaceFor != r->workspace)
        {
            g_workspace = NULL;
            g_workspaceFor = NULL;
            g_gameWorkspace = NULL;
        }
        if (g_target.isNull())
            g_target = Ogre::TextureManager::getSingleton().createManual(
                "BestMinimap_Satellite", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, kFrame, kFrame, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);
        if (g_workspace == NULL)
        {
            const Ogre::CompositorWorkspaceDef* const def = WorkspaceAccess::Def(r->workspace);
            if (def == NULL)
                return false;
            Ogre::RenderTarget* const rt = g_target->getBuffer()->getRenderTarget();
            // Наш - первым в кадре (позиция 0), окно игры - после.
            g_workspace = r->root->getCompositorManager2()->addWorkspace(r->scene, rt, r->camera,
                                                                         WorkspaceDefAccess::Name(def), false, 0);
            if (g_workspace == NULL)
                return false;
            g_workspace->setListener(&g_shotListener);
            g_workspaceFor = r->workspace;
        }
        g_gameWorkspace = r->workspace;
        g_restoreListener.next = g_gameWorkspace->getListener() == &g_restoreListener ? g_restoreListener.next
                                                                                      : g_gameWorkspace->getListener();
        g_gameWorkspace->setListener(&g_restoreListener);
        g_workspace->setEnabled(true);
        return true;
    }

    // ---------------------------------------------------------------
    // Сжатие снимков - свой поток
    // ---------------------------------------------------------------
    //
    // DXT1 для снимка 1024x1024 с уменьшенными копиями - миллисекунды; при
    // качестве 8192 их 64 на плитку, и в потоке игры это была просадка FPS
    // (20.10.2026). Поток игры теперь только читает снимок из видеокарты и
    // отдаёт сюда; здесь - подгонка яркости, сжатие и запись файла.
    //
    // ЯРКОСТЬ. Игра подстраивает экспозицию под кадр: снимок светлого песка
    // выходит темнее, тёмных скал - светлее, и плитки шли лоскутами. Поэтому
    // у снимка поля kMargin: они видят края уже снятых соседей (слева и
    // сверху в той же плитке, по краям - соседние плитки с диска), и
    // снимок умножается на отношение их яркостей.

    struct Msg
    {
        enum Kind { BEGIN, SHOT, END };
        Kind kind;
        LONG gen;
        int tx, tz, quality, n, shot;
        Satellite::Variant variant;
        bool retake;
        DWORD started;
        std::vector<unsigned> px;       // снимок с полями, kFrame x kFrame
    };

    struct Result
    {
        LONG gen;
        int tx, tz, quality;
        Satellite::Variant variant;
        bool retake, ok;
        DWORD ms;
        float minGain, maxGain;
    };

    CRITICAL_SECTION g_encLock;
    HANDLE g_encWake = NULL;
    std::deque<Msg*> g_encQueue;
    std::vector<Result> g_encDone;
    volatile LONG g_encGen = 0;         // какой номер съёмки ещё нужен

    // Состояние потока сжатия.
    struct Neighbour
    {
        bool tried, ok;
        int size;
        std::vector<unsigned> px;
    };
    struct EncTile
    {
        bool active;
        LONG gen;
        int tx, tz, quality, n;
        Satellite::Variant variant;
        bool retake;
        DWORD started;
        std::vector<int> sizes;
        std::vector<std::vector<unsigned char> > blocks;
        Neighbour nb[4];                // слева, справа, сверху, снизу
        float minGain, maxGain;
    };
    EncTile g_enc;

    inline int Luma(unsigned c)
    {
        return static_cast<int>(((c >> 16) & 255) + 2 * ((c >> 8) & 255) + (c & 255));
    }

    double MeanLuma(const unsigned* px, int stride, int x0, int y0, int w, int h)
    {
        if (w <= 0 || h <= 0)
            return 0.0;
        double sum = 0.0;
        for (int y = y0; y < y0 + h; ++y)
            for (int x = x0; x < x0 + w; ++x)
                sum += Luma(px[static_cast<size_t>(y) * stride + x]);
        return sum / (static_cast<double>(w) * h);
    }

    // Средняя яркость куска уровня 0 текущей плитки (уже сжатого): x0, y0,
    // w, h кратны 4.
    double MeanLumaBlocks(int x0, int y0, int w, int h)
    {
        const int size = g_enc.sizes[0], bw = size / 4;
        const unsigned char* const blocks = &g_enc.blocks[0][0];
        unsigned tile[16];
        double sum = 0.0;
        for (int by = y0 / 4; by < (y0 + h) / 4; ++by)
            for (int bx = x0 / 4; bx < (x0 + w) / 4; ++bx)
            {
                DecodeBlock(blocks + (static_cast<size_t>(by) * bw + bx) * 8, tile, 4);
                for (int k = 0; k < 16; ++k)
                    sum += Luma(tile[k]);
            }
        return sum / (static_cast<double>(w) * h);
    }

    // Соседняя плитка с диска (тот же вариант): уровень не больше 1024.
    Neighbour& LoadNeighbour(int side)
    {
        Neighbour& nb = g_enc.nb[side];
        if (nb.tried)
            return nb;
        nb.tried = true;
        nb.ok = false;
        static const int dx[4] = { -1, 1, 0, 0 }, dz[4] = { 0, 0, -1, 1 };
        FILE* f = NULL;
        if (fopen_s(&f, TileFile(g_enc.tx + dx[side], g_enc.tz + dz[side], g_enc.variant).c_str(), "rb") != 0 || f == NULL)
            return nb;
        Header h;
        // Снятое наспех (в сумерки) - не образец яркости для настоящего.
        if (ReadHeader(f, h) && (g_enc.retake || (h.flags & FLAG_RETAKE) == 0))
        {
            int level = static_cast<int>(h.sizes.size()) - 1;
            for (size_t i = 0; i < h.sizes.size(); ++i)
                if (h.sizes[i] <= 1024)
                {
                    level = static_cast<int>(i);
                    break;
                }
            nb.size = h.sizes[level];
            std::vector<unsigned char> blocks(BlocksBytes(nb.size));
            if (fseek(f, h.offsets[level], SEEK_SET) == 0 && fread(&blocks[0], 1, blocks.size(), f) == blocks.size())
            {
                nb.px.resize(static_cast<size_t>(nb.size) * nb.size);
                const int bw = nb.size / 4;
                for (int by = 0; by < bw; ++by)
                    for (int bx = 0; bx < bw; ++bx)
                        DecodeBlock(&blocks[(static_cast<size_t>(by) * bw + bx) * 8],
                                    &nb.px[static_cast<size_t>(by * 4) * nb.size + bx * 4], nb.size);
                nb.ok = true;
            }
        }
        fclose(f);
        return nb;
    }

    // Множитель яркости снимка shot (кадр p с полями) по уже снятым соседям.
    float GainFor(int shot, const std::vector<unsigned>& p)
    {
        const int n = g_enc.n, sx = shot % n, sz = shot / n;
        const int X0 = sx * kShot, Z0 = sz * kShot;
        const unsigned* const P = &p[0];
        double ref = 0.0, our = 0.0;
        // Сосед и наши поля над ним; совсем тёмное (край мира) - не в счёт.
        struct Pair { double r, o; };
        Pair pairs[6];
        int count = 0;
        if (sx > 0)
        {
            pairs[count].r = MeanLumaBlocks(X0 - kMargin, Z0, kMargin, kShot);
            pairs[count++].o = MeanLuma(P, kFrame, 0, kMargin, kMargin, kShot);
        }
        if (sz > 0)
        {
            pairs[count].r = MeanLumaBlocks(X0, Z0 - kMargin, kShot, kMargin);
            pairs[count++].o = MeanLuma(P, kFrame, kMargin, 0, kShot, kMargin);
        }
        // Соседние плитки: полоса той же ширины в метрах, в их пикселях.
        const bool edge[4] = { sx == 0, sx == n - 1, sz == 0, sz == n - 1 };
        for (int side = 0; side < 4; ++side)
        {
            if (!edge[side])
                continue;
            Neighbour& nb = LoadNeighbour(side);
            if (!nb.ok)
                continue;
            const int S = nb.size;
            const int w = Hi(1, kMargin * S / (kShot * n));
            const int a = (side < 2 ? sz : sx) * S / n, len = Hi(1, S / n);
            if (side == 0)
            {
                pairs[count].r = MeanLuma(&nb.px[0], S, S - w, a, w, len);
                pairs[count++].o = MeanLuma(P, kFrame, 0, kMargin, kMargin, kShot);
            }
            else if (side == 1)
            {
                pairs[count].r = MeanLuma(&nb.px[0], S, 0, a, w, len);
                pairs[count++].o = MeanLuma(P, kFrame, kMargin + kShot, kMargin, kMargin, kShot);
            }
            else if (side == 2)
            {
                pairs[count].r = MeanLuma(&nb.px[0], S, a, S - w, len, w);
                pairs[count++].o = MeanLuma(P, kFrame, kMargin, 0, kShot, kMargin);
            }
            else
            {
                pairs[count].r = MeanLuma(&nb.px[0], S, a, 0, len, w);
                pairs[count++].o = MeanLuma(P, kFrame, kMargin, kMargin + kShot, kShot, kMargin);
            }
        }
        for (int i = 0; i < count; ++i)
            if (pairs[i].o >= 8.0 && pairs[i].r >= 8.0)
            {
                ref += pairs[i].r;
                our += pairs[i].o;
            }
        if (our <= 0.0)
            return 1.0f;
        const double g = ref / our;
        return static_cast<float>(g < 0.6 ? 0.6 : (g > 1.6 ? 1.6 : g));
    }

    // Снимок мозаики (середина кадра, уже с подогнанной яркостью) - во все
    // уровни плитки: уровень 0 - как есть, дальше - уменьшение вдвое
    // (среднее 2x2), пока снимок не 4x4.
    void StoreShot(int shot, std::vector<unsigned>& px)
    {
        const int sx = shot % g_enc.n, sz = shot / g_enc.n;
        int side = kShot;
        for (size_t level = 0; level < g_enc.sizes.size() && side >= 4; ++level)
        {
            const int tileBlocks = g_enc.sizes[level] / 4;
            const int shotBlocks = side / 4;
            unsigned char* const out = &g_enc.blocks[level][0];
            for (int by = 0; by < shotBlocks; ++by)
                for (int bx = 0; bx < shotBlocks; ++bx)
                {
                    const size_t block = static_cast<size_t>(sz * shotBlocks + by) * tileBlocks + sx * shotBlocks + bx;
                    EncodeBlock(&px[static_cast<size_t>(by * 4) * side + bx * 4], side, out + block * 8);
                }
            // Вдвое меньше - на месте.
            const int half = side / 2;
            for (int y = 0; y < half; ++y)
                for (int x = 0; x < half; ++x)
                {
                    const unsigned a = px[static_cast<size_t>(2 * y) * side + 2 * x], b = px[static_cast<size_t>(2 * y) * side + 2 * x + 1];
                    const unsigned c = px[static_cast<size_t>(2 * y + 1) * side + 2 * x], d = px[static_cast<size_t>(2 * y + 1) * side + 2 * x + 1];
                    const unsigned rb = (((a & 0xFF00FFu) + (b & 0xFF00FFu) + (c & 0xFF00FFu) + (d & 0xFF00FFu) + 0x020002u) >> 2) & 0xFF00FFu;
                    const unsigned g = (((a & 0x00FF00u) + (b & 0x00FF00u) + (c & 0x00FF00u) + (d & 0x00FF00u) + 0x000200u) >> 2) & 0x00FF00u;
                    px[static_cast<size_t>(y) * half + x] = 0xFF000000u | rb | g;
                }
            side = half;
        }
    }

    // Файл - через временный: поток рисования не прочтёт недописанное. Он
    // же может держать старый файл открытым - тогда пробуем ещё раз.
    bool WriteTile()
    {
        const std::string file = TileFile(g_enc.tx, g_enc.tz, g_enc.variant);
        const std::string temp = file + ".tmp";
        FILE* f = NULL;
        if (fopen_s(&f, temp.c_str(), "wb") != 0 || f == NULL)
            return false;
        const int levels = static_cast<int>(g_enc.sizes.size());
        const int flags = g_enc.retake ? FLAG_RETAKE : 0;
        bool ok = fwrite("BMS2", 1, 4, f) == 4 && fwrite(&g_enc.quality, 4, 1, f) == 1 && fwrite(&flags, 4, 1, f) == 1 &&
                  fwrite(&levels, 4, 1, f) == 1 && fwrite(&g_enc.sizes[0], 4, levels, f) == static_cast<size_t>(levels);
        for (int i = 0; ok && i < levels; ++i)
            ok = fwrite(&g_enc.blocks[i][0], 1, g_enc.blocks[i].size(), f) == g_enc.blocks[i].size();
        fclose(f);
        bool moved = false;
        for (int attempt = 0; ok && !moved && attempt < 20; ++attempt)
        {
            moved = MoveFileExA(temp.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
            if (!moved)
                Sleep(10);
        }
        if (!moved)
            DeleteFileA(temp.c_str());
        return moved;
    }

    void Handle(Msg* m)
    {
        if (m->gen != g_encGen)
            return;                     // съёмка из прошлой игры
        if (m->kind == Msg::BEGIN)
        {
            g_enc = EncTile();
            g_enc.active = true;
            g_enc.gen = m->gen;
            g_enc.tx = m->tx;
            g_enc.tz = m->tz;
            g_enc.variant = m->variant;
            g_enc.quality = m->quality;
            g_enc.n = m->n;
            g_enc.retake = m->retake;
            g_enc.started = m->started;
            g_enc.minGain = 10.0f;
            g_enc.maxGain = 0.0f;
            for (int i = 0; i < 4; ++i)
                g_enc.nb[i].tried = g_enc.nb[i].ok = false;
            for (int size = m->quality, shot = kShot; shot >= kMinLevel; size /= 2, shot /= 2)
            {
                g_enc.sizes.push_back(size);
                g_enc.blocks.push_back(std::vector<unsigned char>(BlocksBytes(size)));
            }
            return;
        }
        if (m->kind == Msg::END && !g_enc.active)
        {
            // Плитку бросили (не хватило памяти) - поток игры ждёт ответа.
            Result res;
            res.gen = m->gen;
            res.tx = m->tx;
            res.tz = m->tz;
            res.quality = m->quality;
            res.variant = m->variant;
            res.retake = m->retake;
            res.ok = false;
            res.ms = GetTickCount() - m->started;
            res.minGain = res.maxGain = 1.0f;
            EnterCriticalSection(&g_encLock);
            g_encDone.push_back(res);
            LeaveCriticalSection(&g_encLock);
            return;
        }
        if (!g_enc.active || g_enc.gen != m->gen)
            return;
        if (m->kind == Msg::SHOT)
        {
            const float gain = GainFor(m->shot, m->px);
            g_enc.minGain = Lo(g_enc.minGain, gain);
            g_enc.maxGain = Hi(g_enc.maxGain, gain);
            const unsigned k = static_cast<unsigned>(gain * 256.0f + 0.5f);
            // Середина кадра, с подогнанной яркостью, - подряд.
            std::vector<unsigned> mid(static_cast<size_t>(kShot) * kShot);
            for (int y = 0; y < kShot; ++y)
            {
                const unsigned* const src = &m->px[static_cast<size_t>(y + kMargin) * kFrame + kMargin];
                unsigned* const dst = &mid[static_cast<size_t>(y) * kShot];
                for (int x = 0; x < kShot; ++x)
                {
                    const unsigned c = src[x];
                    const unsigned r = Lo(255u, (((c >> 16) & 255) * k) >> 8);
                    const unsigned g = Lo(255u, (((c >> 8) & 255) * k) >> 8);
                    const unsigned b = Lo(255u, ((c & 255) * k) >> 8);
                    dst[x] = 0xFF000000u | (r << 16) | (g << 8) | b;
                }
            }
            StoreShot(m->shot, mid);
            return;
        }
        // END
        Result res;
        res.gen = g_enc.gen;
        res.tx = g_enc.tx;
        res.tz = g_enc.tz;
        res.quality = g_enc.quality;
        res.variant = g_enc.variant;
        res.retake = g_enc.retake;
        res.ok = WriteTile();
        res.ms = GetTickCount() - g_enc.started;
        res.minGain = g_enc.minGain;
        res.maxGain = g_enc.maxGain;
        g_enc = EncTile();
        EnterCriticalSection(&g_encLock);
        g_encDone.push_back(res);
        LeaveCriticalSection(&g_encLock);
    }

    DWORD WINAPI EncoderThread(LPVOID)
    {
        for (;;)
        {
            WaitForSingleObject(g_encWake, INFINITE);
            for (;;)
            {
                Msg* m = NULL;
                EnterCriticalSection(&g_encLock);
                if (!g_encQueue.empty())
                {
                    m = g_encQueue.front();
                    g_encQueue.pop_front();
                }
                LeaveCriticalSection(&g_encLock);
                if (m == NULL)
                    break;
                try
                {
                    Handle(m);
                }
                catch (...)
                {
                    g_enc = EncTile();          // нет памяти - плитку бросаем
                }
                delete m;
            }
        }
    }

    bool g_encReady = false;

    bool StartEncoder()
    {
        if (g_encReady)
            return true;
        InitializeCriticalSection(&g_encLock);
        g_encWake = CreateEventA(NULL, FALSE, FALSE, NULL);
        HANDLE const thread = g_encWake == NULL ? NULL : CreateThread(NULL, 0, &EncoderThread, NULL, 0, NULL);
        if (thread == NULL)
        {
            Log("could not start the compression thread - no satellite shots");
            return false;
        }
        CloseHandle(thread);
        g_encReady = true;
        return true;
    }

    void Post(Msg* m)
    {
        EnterCriticalSection(&g_encLock);
        g_encQueue.push_back(m);
        LeaveCriticalSection(&g_encLock);
        SetEvent(g_encWake);
    }

    bool Begin(int tx, int tz, Satellite::Variant v, bool retake)
    {
        if (!StartEncoder())
            return false;
        g_job = Job();
        g_job.tx = tx;
        g_job.tz = tz;
        g_job.variant = v;
        g_job.retake = retake;
        // Наспех - быстро, в 1024: его всё равно переснимут.
        g_job.quality = retake ? kShot : g_quality;
        g_job.n = g_job.quality / kShot;
        // Высота земли - в центре плитки; нет её там (край мира) - 0.
        const float cx = (tx + 0.5f) * Satellite::kTileMetres, cz = (tz + 0.5f) * Satellite::kTileMetres;
        const float ground = UtilityT::getTerrainHeight(cx, cz);
        g_job.groundY = (ground == ground && ground > -100000.0f && ground < 100000.0f) ? ground : 0.0f;
        g_job.wait = kSettleFirst;
        g_job.started = GetTickCount();
        g_job.active = true;
        if (!StartShooting())
        {
            g_job.active = false;
            return false;
        }
        Msg* const m = new Msg();
        m->kind = Msg::BEGIN;
        m->gen = g_gen;
        m->tx = tx;
        m->tz = tz;
        m->quality = g_job.quality;
        m->n = g_job.n;
        m->shot = 0;
        m->variant = v;
        m->retake = retake;
        m->started = g_job.started;
        Post(m);
        return true;
    }

    // Готовое из потока сжатия - в список снятого.
    void CollectDone()
    {
        if (!g_encReady)
            return;
        std::vector<Result> done;
        EnterCriticalSection(&g_encLock);
        done.swap(g_encDone);
        LeaveCriticalSection(&g_encLock);
        for (size_t i = 0; i < done.size(); ++i)
        {
            const Result& r = done[i];
            if (r.gen != g_gen)
                continue;
            g_encoding = false;
            char note[220];
            sprintf_s(note, "tile %d %d (%s, %d px%s) %s in %u ms, brightness x%.2f-%.2f", r.tx, r.tz,
                      r.variant == Satellite::DAY ? "day" : "night", r.quality, r.retake ? ", retake later" : "",
                      r.ok ? "taken" : "could not be saved", r.ms, r.minGain, r.maxGain);
            Log(note);
            if (!r.ok)
                continue;
            const long long key = Satellite::Key(r.tx, r.tz, r.variant);
            g_stored[key] = Stored(r.quality, r.retake);
            g_fresh.push_back(key);
        }
    }

    // Какой вариант снимать в это время суток. Настоящее время - день 9-16,
    // ночь 22-4. В сумерки (и утром, и вечером) снимаем только то, чего нет
    // совсем, наспех и с пометкой: придёт день или ночь - переснимем. Иначе
    // полсуток игры мир вокруг не снимался бы вовсе (20.10.2026).
    Satellite::Variant VariantAt(double hour, bool& proper)
    {
        proper = true;
        if (hour >= 9.0 && hour < 16.0)
            return Satellite::DAY;
        if (hour >= 22.0 || hour < 4.0)
            return Satellite::NIGHT;
        proper = false;
        return (hour >= 7.0 && hour < 19.0) ? Satellite::DAY : Satellite::NIGHT;
    }

    // Следующая плитка. Сначала ближние (центр плитки не дальше 1,4 км от
    // центра камеры: там игра держит мир целиком), потом дальние (до 4 км,
    // только рельеф). В настоящее время суток ближние - которых нет, снятые
    // наспех или хуже нынешнего качества; дальние - которых нет (наспех, в
    // 1024: переснимем вблизи). В сумерки - только которых нет, наспех.
    bool PickTile(const Ogre::Vector3& c, Satellite::Variant v, bool proper, int& outX, int& outZ, bool& retake)
    {
        const float nearR = 1400.0f, farR = 4100.0f;
        const int cx = static_cast<int>(std::floor(c.x / Satellite::kTileMetres));
        const int cz = static_cast<int>(std::floor(c.z / Satellite::kTileMetres));
        float bestNear = nearR * nearR, bestFar = farR * farR;
        bool foundNear = false, foundFar = false;
        int nx = 0, nz = 0, fx = 0, fz = 0;
        for (int dz = -5; dz <= 5; ++dz)
            for (int dx = -5; dx <= 5; ++dx)
            {
                const int tx = cx + dx, tz = cz + dz;
                const float mx = (tx + 0.5f) * Satellite::kTileMetres - c.x, mz = (tz + 0.5f) * Satellite::kTileMetres - c.z;
                const float d2 = mx * mx + mz * mz;
                if (d2 >= farR * farR)
                    continue;
                std::map<long long, Stored>::const_iterator it = g_stored.find(Satellite::Key(tx, tz, v));
                const bool have = it != g_stored.end();
                const bool nearby = d2 < nearR * nearR;
                const bool need = !have || (proper && nearby && (it->second.retake || it->second.quality < g_quality));
                if (!need)
                    continue;
                if (nearby && d2 < bestNear)
                {
                    bestNear = d2;
                    nx = tx;
                    nz = tz;
                    foundNear = true;
                }
                else if (!nearby && d2 < bestFar)
                {
                    bestFar = d2;
                    fx = tx;
                    fz = tz;
                    foundFar = true;
                }
            }
        if (foundNear)
        {
            outX = nx;
            outZ = nz;
            retake = !proper;
            return true;
        }
        if (foundFar)
        {
            outX = fx;
            outZ = fz;
            retake = true;
            return true;
        }
        return false;
    }

    void Fail(const std::string& why)
    {
        Log("failed: " + why);
        StopShooting();
    }


    // ---------------------------------------------------------------
    // Поток рисования: плитки в памяти
    // ---------------------------------------------------------------

    struct FileInfo
    {
        bool exists;
        Header header;
        DWORD checked;
    };
    std::map<long long, FileInfo> g_files;

    // Какие плитки есть на диске - свой список потока рисования: папка
    // читается один раз, снятые потом добавляет Forget. Без него при
    // широком обзоре поток раз в 5 с открывал бы файл каждого из тысяч
    // участков в кадре.
    std::map<long long, bool> g_onDisk;
    bool g_onDiskScanned = false;

    void ScanOnDisk()
    {
        g_onDiskScanned = true;
        WIN32_FIND_DATAA fd;
        HANDLE const h = FindFirstFileA((g_folder + "tile_*.bms").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE)
            return;
        do
        {
            long long key = 0;
            if (KeyOfFile(fd.cFileName, key))
                g_onDisk[key] = true;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }

    struct Cached
    {
        std::vector<unsigned> px;
        int size;
        unsigned frame;                 // кадр последнего использования
    };
    typedef std::map<std::pair<long long, int>, Cached> CacheMap;
    CacheMap g_cache;
    size_t g_cacheBytes = 0;
    unsigned g_frame = 0;
    const size_t kCacheBudget = 192u * 1024u * 1024u;

    const FileInfo& Info(long long key)
    {
        static FileInfo none;
        if (!g_onDiskScanned)
            ScanOnDisk();
        if (g_onDisk.find(key) == g_onDisk.end())
        {
            none.exists = false;
            return none;
        }
        const DWORD now = GetTickCount();
        std::map<long long, FileInfo>::iterator it = g_files.find(key);
        // Нет файла - проверяем снова не чаще раза в 5 с.
        if (it != g_files.end() && (it->second.exists || now - it->second.checked < 5000))
            return it->second;
        FileInfo& info = g_files[key];
        info.exists = false;
        info.checked = now;
        int tx, tz;
        Satellite::Variant v;
        Decode(key, tx, tz, v);
        FILE* f = NULL;
        if (fopen_s(&f, TileFile(tx, tz, v).c_str(), "rb") == 0 && f != NULL)
        {
            info.exists = ReadHeader(f, info.header);
            fclose(f);
        }
        return info;
    }

    // Лишнее из кэша - самое давно не нужное, но не из этого кадра.
    void Trim()
    {
        while (g_cacheBytes > kCacheBudget)
        {
            CacheMap::iterator oldest = g_cache.end();
            for (CacheMap::iterator it = g_cache.begin(); it != g_cache.end(); ++it)
                if (it->second.frame != g_frame && (oldest == g_cache.end() || it->second.frame < oldest->second.frame))
                    oldest = it;
            if (oldest == g_cache.end())
                return;
            g_cacheBytes -= oldest->second.px.size() * sizeof(unsigned);
            g_cache.erase(oldest);
        }
    }
}


namespace Satellite
{
    long long Key(int tx, int tz, Variant v)
    {
        return (static_cast<long long>(tx + 32768) << 34) | (static_cast<long long>(tz + 32768) << 2) |
               static_cast<long long>(v);
    }

    int StoredCount(Variant v)
    {
        int n = 0;
        for (std::map<long long, Stored>::const_iterator it = g_stored.begin(); it != g_stored.end(); ++it)
            n += (it->first & 3) == v ? 1 : 0;
        return n;
    }

    void Configure(bool capture, int quality, const std::string& folder)
    {
        g_capture = capture;
        g_quality = quality == 2048 || quality == 4096 || quality == 8192 ? quality : 1024;
        if (folder != g_folder)
        {
            g_folder = folder;
            ScanFolder();
        }
    }

    void Tick(const Ogre::Vector3& cameraCentre, double hourOfDay, bool allowed)
    {
        const DWORD now = GetTickCount();
        CollectDone();
        if (!g_job.active)
        {
            if (!allowed || !g_capture || g_encoding || now < g_nextPlan)
                return;
            g_nextPlan = now + 500;
            bool proper = true;
            const Variant v = VariantAt(hourOfDay, proper);
            int tx = 0, tz = 0;
            bool retake = false;
            if (!PickTile(cameraCentre, v, proper, tx, tz, retake))
                return;
            try
            {
                Begin(tx, tz, v, retake);
            }
            catch (const Ogre::Exception& e)
            {
                Fail(e.getFullDescription());
            }
            catch (...)
            {
                Fail("unknown error");
            }
            return;
        }
        if (--g_job.wait > 0)
            return;
        // Поток сжатия не успевает - ждём: очередь снимков по 5 МБ не копим.
        EnterCriticalSection(&g_encLock);
        const size_t queued = g_encQueue.size();
        LeaveCriticalSection(&g_encLock);
        if (queued >= 3)
        {
            g_job.wait = 1;
            return;
        }
        try
        {
            // Поток игры - только чтение снимка; остальное - поток сжатия.
            Msg* const m = new Msg();
            m->kind = Msg::SHOT;
            m->gen = g_gen;
            m->shot = g_job.shot;
            m->px.resize(static_cast<size_t>(kFrame) * kFrame);
            Ogre::PixelBox box(kFrame, kFrame, 1, Ogre::PF_A8R8G8B8, &m->px[0]);
            g_target->getBuffer()->blitToMemory(box);
            Post(m);
            if (++g_job.shot < g_job.n * g_job.n)
            {
                g_job.wait = kSettleNext;
                return;
            }
            Msg* const end = new Msg();
            end->kind = Msg::END;
            end->gen = g_gen;
            end->tx = g_job.tx;
            end->tz = g_job.tz;
            end->quality = g_job.quality;
            end->n = g_job.n;
            end->shot = 0;
            end->variant = g_job.variant;
            end->retake = g_job.retake;
            end->started = g_job.started;
            Post(end);
            g_encoding = true;
            StopShooting();
            g_nextPlan = GetTickCount() + 500;      // передышка между плитками
        }
        catch (const Ogre::Exception& e)
        {
            Fail(e.getFullDescription());
        }
        catch (...)
        {
            Fail("unknown error");
        }
    }

    void CaptureNow(const Ogre::Vector3& centre, double hourOfDay)
    {
        if (g_job.active || g_encoding)
            return;
        const Variant v = hourOfDay >= 6.0 && hourOfDay < 19.0 ? DAY : NIGHT;
        const int tx = static_cast<int>(std::floor(centre.x / kTileMetres));
        const int tz = static_cast<int>(std::floor(centre.z / kTileMetres));
        try
        {
            Begin(tx, tz, v, false);
        }
        catch (const Ogre::Exception& e)
        {
            Fail(e.getFullDescription());
        }
        catch (...)
        {
            Fail("unknown error");
        }
    }

    void Reset()
    {
        // Начатое потоком сжатия - не нужно: новый номер съёмки.
        g_gen = InterlockedIncrement(&g_encGen);
        g_encoding = false;
        try
        {
            DropWorkspace();
        }
        catch (...)
        {
            g_workspace = NULL;
            g_workspaceFor = NULL;
            g_gameWorkspace = NULL;
            g_job.active = false;
        }
        g_nextPlan = GetTickCount() + 5000;
    }

    void TakeFresh(std::vector<long long>& out)
    {
        out.swap(g_fresh);
        g_fresh.clear();
    }

    void BeginFrame()
    {
        ++g_frame;
        Trim();
    }

    void Forget(const std::vector<long long>& keys)
    {
        for (size_t i = 0; i < keys.size(); ++i)
        {
            g_onDisk[keys[i]] = true;
            g_files.erase(keys[i]);
            for (CacheMap::iterator it = g_cache.begin(); it != g_cache.end();)
            {
                if (it->first.first == keys[i])
                {
                    g_cacheBytes -= it->second.px.size() * sizeof(unsigned);
                    g_cache.erase(it++);
                }
                else
                    ++it;
            }
        }
    }

    bool TileLevel(int tx, int tz, Variant v, int wantSize, Level& out)
    {
        const long long key = Key(tx, tz, v);
        const FileInfo& info = Info(key);
        if (!info.exists)
            return false;
        // Самый мелкий уровень не меньше нужного; нужно больше, чем есть, -
        // самый подробный.
        int level = 0;
        for (size_t i = 0; i < info.header.sizes.size(); ++i)
            if (info.header.sizes[i] >= wantSize)
                level = static_cast<int>(i);
        const int size = info.header.sizes[level];
        CacheMap::iterator it = g_cache.find(std::make_pair(key, size));
        if (it == g_cache.end())
        {
            std::vector<unsigned char> blocks(BlocksBytes(size));
            FILE* f = NULL;
            if (fopen_s(&f, TileFile(tx, tz, v).c_str(), "rb") != 0 || f == NULL)
                return false;
            const bool ok = fseek(f, info.header.offsets[level], SEEK_SET) == 0 &&
                            fread(&blocks[0], 1, blocks.size(), f) == blocks.size();
            fclose(f);
            if (!ok)
            {
                g_files.erase(key);
                return false;
            }
            Cached c;
            c.size = size;
            c.frame = g_frame;
            c.px.resize(static_cast<size_t>(size) * size);
            const int bw = size / 4;
            for (int by = 0; by < bw; ++by)
                for (int bx = 0; bx < bw; ++bx)
                    DecodeBlock(&blocks[(static_cast<size_t>(by) * bw + bx) * 8], &c.px[static_cast<size_t>(by * 4) * size + bx * 4], size);
            g_cacheBytes += c.px.size() * sizeof(unsigned);
            it = g_cache.insert(std::make_pair(std::make_pair(key, size), Cached())).first;
            it->second.px.swap(c.px);
            it->second.size = size;
        }
        it->second.frame = g_frame;
        out.size = it->second.size;
        out.px = &it->second.px[0];
        return true;
    }
}
