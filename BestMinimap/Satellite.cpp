// Satellite - снимок мира сверху (проба «вида со спутника»).
//
// КАК. Игра рисует мир через набор компоновщиков Ogre 2.0
// (CompositorWorkspace): тени, отложенное освещение, туман, постобработка.
// Свой «спутник» - это тот же набор, но со своей камерой (высоко над
// местом, взглядом вниз, север вверху) и своей текстурой вместо окна. Набор
// игры берём по имени у её же рабочего набора (Renderer::workspace), так что
// картинка - как в игре, с модами на графику.
//
// История проб (20.10.2026):
//   1-2. своя камера (ортогональная, потом перспективная) - чёрный мир, на
//        снимке только интерфейс и курсор;
//   3.   камера игры, перенесённая на два кадра наверх, - мир сверху есть
//        (на экране мигает вид сверху). Своя камера, видимо, не была
//        прикреплена к узлу сцены (у камеры игры узел есть) - Ogre не знал,
//        где она.
//   4.   своя камера на своём узле сцены: вид игры и вид сверху; для
//        сравнения - снова камера игры. Интерфейс не прячем, а выключаем
//        оверлеи у областей вывода нашей текстуры: MyGUI рисуется только в
//        области с оверлеями. Сработает - экран не мигает совсем.
//
// ПРОБА. Пока это опыт: по клавише (SatelliteShotKey), снимки - в папку
// мода картинками BMP. Пока набор включён, игра рисует мир дважды - пара
// десятков кадров с просадкой, только по клавише.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <Debug.h>

#include <OgreRoot.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTexture.h>
#include <OgreResourceGroupManager.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <Compositor/OgreCompositorWorkspaceDef.h>

#include <kenshi/Globals.h>
#include <kenshi/KingOfRenderThread.h>
#include <kenshi/Renderer.h>

#include "Satellite.h"

namespace
{
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

    const int kSide = 1024;             // снимок, пикселей
    const int kSettleFrames = 12;       // кадров на снимок: свет и адаптация яркости

    Ogre::TexturePtr g_target;
    Ogre::CompositorWorkspace* g_workspace = NULL;
    Ogre::IdString g_defName;           // набор компоновщиков игры
    Ogre::Camera* g_own = NULL;         // своя камера
    Ogre::SceneNode* g_ownNode = NULL;  // и её узел

    // Шаги пробы, ещё не сделанные (с конца): fov 0 - камера игры как есть,
    // иначе - перенесена наверх с этим углом обзора.
    enum Mode { OWN_GAME_VIEW, OWN_TOP, GAME_TOP };
    struct Step { const char* name; Mode mode; float fov; };
    std::vector<Step> g_steps;

    // Камера игры до переноса.
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
    Ogre::Vector3 g_center;             // мировые координаты
    float g_size = 0.0f;
    std::string g_folder;
    int g_wait = 0;
    DWORD g_started = 0;

    Renderer* GameRenderer()
    {
        return ::au == NULL ? NULL : ::au->render;
    }

    void Log(const std::string& text)
    {
        DebugLog("BestMinimap: satellite: " + text);
    }

    // Интерфейс MyGUI - не в нашу текстуру: оверлеи выключены у всех её
    // областей вывода (их создаёт набор компоновщиков).
    int NoOverlays()
    {
        Ogre::RenderTarget* const rt = g_target->getBuffer()->getRenderTarget();
        const unsigned short n = rt->getNumViewports();
        for (unsigned short i = 0; i < n; ++i)
            if (Ogre::Viewport* const vp = rt->getViewport(i))
                vp->setOverlaysEnabled(false);
        return n;
    }

    void StopWorkspace()
    {
        Renderer* const r = GameRenderer();
        if (g_workspace != NULL && r != NULL && r->root != NULL)
            r->root->getCompositorManager2()->removeWorkspace(g_workspace);
        g_workspace = NULL;
    }

    // BMP 32 бита, снизу вверх. PF_A8R8G8B8 в памяти - B, G, R, A: так BMP
    // и хранит.
    bool SaveBmp(const std::string& file, const std::vector<unsigned char>& bgra, int w, int h)
    {
        FILE* f = NULL;
        if (fopen_s(&f, file.c_str(), "wb") != 0 || f == NULL)
            return false;
        const unsigned rowBytes = static_cast<unsigned>(w) * 4;
        const unsigned dataBytes = rowBytes * h;
        unsigned char header[54] = {};
        header[0] = 'B';
        header[1] = 'M';
        const unsigned fileBytes = 54 + dataBytes;
        memcpy(header + 2, &fileBytes, 4);
        const unsigned offset = 54, infoBytes = 40;
        memcpy(header + 10, &offset, 4);
        memcpy(header + 14, &infoBytes, 4);
        memcpy(header + 18, &w, 4);
        memcpy(header + 22, &h, 4);
        const unsigned short planes = 1, bits = 32;
        memcpy(header + 26, &planes, 2);
        memcpy(header + 28, &bits, 2);
        memcpy(header + 34, &dataBytes, 4);
        fwrite(header, 1, sizeof(header), f);
        for (int y = h - 1; y >= 0; --y)
            fwrite(&bgra[static_cast<size_t>(y) * rowBytes], 1, rowBytes, f);
        fclose(f);
        return true;
    }

    // Камеру игры - над центром снимка, взглядом вниз, север (-Z мира: на
    // картинке карты он вверху) - вверху снимка. Высота - такая, чтобы при
    // угле fov квадрат g_size на уровне центра влез ровно в кадр. Камера может
    // висеть на узле сцены - тогда место и поворот задаём относительно узла.
    // Камеру cam - наверх (для своей камеры сохранять нечего).
    void AimDown(Ogre::Camera* cam, float fov)
    {
        Renderer* const r = GameRenderer();
        if (cam == r->camera && !g_saved.saved)
        {
            g_saved.position = cam->getPosition();
            g_saved.orientation = cam->getOrientation();
            g_saved.fov = cam->getFOVy();
            g_saved.nearClip = cam->getNearClipDistance();
            g_saved.farClip = cam->getFarClipDistance();
            g_saved.aspect = cam->getAspectRatio();
            g_saved.saved = true;
        }
        const float height = g_size * 0.5f / std::tan(fov * 0.5f * 3.14159265f / 180.0f);
        // Сцена может быть сдвинута относительно мира (чтобы числа были
        // мелкими у камеры): мировая точка в сцене - минус этот сдвиг.
        const Ogre::Vector3 origin = r->getSceneRelativeOrigin();
        const Ogre::Vector3 scene = g_center - origin + Ogre::Vector3(0.0f, height, 0.0f);
        // Камера смотрит по -Z; поворот на -90 градусов вокруг X - взгляд вниз
        // (-Y), «верх» снимка - -Z, север.
        const Ogre::Quaternion down(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_X);
        Ogre::Vector3 position = scene;
        Ogre::Quaternion orientation = down;
        if (cam == g_own && g_ownNode != NULL)
        {
            // Своя - двигаем её узел, сама камера в нём без сдвига.
            g_ownNode->setPosition(position);
            g_ownNode->setOrientation(orientation);
        }
        else
        {
            if (Ogre::Node* const parent = cam->getParentNode())
            {
                const Ogre::Quaternion inv = parent->_getDerivedOrientation().Inverse();
                position = inv * (scene - parent->_getDerivedPosition());
                orientation = inv * down;
            }
            cam->setPosition(position);
            cam->setOrientation(orientation);
        }
        cam->setFOVy(Ogre::Degree(fov));
        cam->setAspectRatio(1.0f);
        // Глубина - только вокруг земли: так её точности хватает и с
        // десятка километров.
        cam->setNearClipDistance(height > 4005.0f ? height - 4000.0f : 5.0f);
        cam->setFarClipDistance(height + 4000.0f);
    }

    // Свою камеру - туда же, куда смотрит камера игры.
    void AimLikeGame()
    {
        Ogre::Camera* const game = GameRenderer()->camera;
        g_ownNode->setPosition(game->getDerivedPosition());
        g_ownNode->setOrientation(game->getDerivedOrientation());
        g_own->setFOVy(game->getFOVy());
        g_own->setAspectRatio(1.0f);
        g_own->setNearClipDistance(game->getNearClipDistance());
        g_own->setFarClipDistance(game->getFarClipDistance());
    }

    void RestoreGameCamera()
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

    void LogShot(const Step& step)
    {
        Renderer* const r = GameRenderer();
        const Ogre::Vector3 origin = r->getSceneRelativeOrigin();
        Ogre::Camera* const cam = r->camera;
        char note[400];
        if (step.mode == OWN_GAME_VIEW)
            sprintf_s(note, "shot '%s': own camera where the game camera is (%.0f %.0f %.0f scene, fov %.1f)",
                      step.name, cam->getDerivedPosition().x, cam->getDerivedPosition().y, cam->getDerivedPosition().z,
                      cam->getFOVy().valueDegrees());
        else
            sprintf_s(note, "shot '%s' with %.0f deg: %.0f m over world %.0f %.0f %.0f (scene origin %.0f %.0f %.0f)",
                      step.name, step.fov, g_size * 0.5f / std::tan(step.fov * 0.5f * 3.14159265f / 180.0f),
                      g_center.x, g_center.y, g_center.z, origin.x, origin.y, origin.z);
        Log(note);
    }

    bool Begin()
    {
        Renderer* const r = GameRenderer();
        if (r == NULL || r->root == NULL || r->scene == NULL || r->workspace == NULL || r->camera == NULL)
        {
            Log("the game renderer is not ready");
            return false;
        }
        const Ogre::CompositorWorkspaceDef* const def = WorkspaceAccess::Def(r->workspace);
        if (def == NULL)
        {
            Log("the game workspace has no definition");
            return false;
        }
        g_defName = WorkspaceDefAccess::Name(def);
        if (g_target.isNull())
        {
            g_target = Ogre::TextureManager::getSingleton().createManual(
                "BestMinimap_Satellite", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, kSide, kSide, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);
        }
        if (g_own == NULL)
        {
            g_own = r->scene->createCamera("BestMinimap_SatelliteCamera");
            g_own->setFixedYawAxis(false);
            g_own->setAspectRatio(1.0f);
            const bool attached = g_own->getParentNode() != NULL;
            if (attached)
                g_own->detachFromParent();
            g_ownNode = r->scene->getRootSceneNode()->createChildSceneNode();
            g_ownNode->attachObject(g_own);
            g_own->setPosition(Ogre::Vector3::ZERO);
            g_own->setOrientation(Ogre::Quaternion::IDENTITY);
            Log(std::string("own camera created; Ogre had attached it to a node by itself: ") + (attached ? "yes" : "no"));
        }
        char note[160];
        sprintf_s(note, "started: %.0f m square, %dx%d, game workspace definition %08X", g_size, kSide, kSide,
                  g_defName.mHash);
        Log(note);
        return true;
    }

    // Набор компоновщиков на шаг - со своей камерой или с камерой игры.
    void StartStep(const Step& step)
    {
        StopWorkspace();
        Renderer* const r = GameRenderer();
        Ogre::Camera* const cam = step.mode == GAME_TOP ? r->camera : g_own;
        if (step.mode == OWN_GAME_VIEW)
            AimLikeGame();
        else if (step.mode == OWN_TOP)
            AimDown(g_own, step.fov);
        Ogre::RenderTarget* const rt = g_target->getBuffer()->getRenderTarget();
        g_workspace = r->root->getCompositorManager2()->addWorkspace(r->scene, rt, cam, g_defName, true, 0);
        char note[80];
        sprintf_s(note, "viewports with overlays turned off: %d", NoOverlays());
        Log(note);
        LogShot(step);
        g_wait = kSettleFrames;
    }

    void SaveShot(const char* label)
    {
        std::vector<unsigned char> pixels(static_cast<size_t>(kSide) * kSide * 4, 0);
        Ogre::PixelBox box(kSide, kSide, 1, Ogre::PF_A8R8G8B8, &pixels[0]);
        g_target->getBuffer()->blitToMemory(box);
        // Альфа из рендера бывает любой - в BMP делаем непрозрачной.
        for (size_t i = 3; i < pixels.size(); i += 4)
            pixels[i] = 255;
        char name[96];
        sprintf_s(name, "satellite_probe_%s.bmp", label);
        const std::string file = g_folder + name;
        Log(std::string(SaveBmp(file, pixels, kSide, kSide) ? "saved " : "could not save ") + file);
    }

    void Fail(const std::string& why)
    {
        Log("failed: " + why);
        g_steps.clear();
        RestoreGameCamera();
        StopWorkspace();
    }
}

namespace Satellite
{
    void RequestProbe(const Ogre::Vector3& centerWorld, float sizeMetres, const std::string& folder)
    {
        if (!g_steps.empty())
            return;                 // ещё снимаем прошлый
        g_center = centerWorld;
        // Квадрат - не больше 2 км: проба подробности, а не всей карты.
        g_size = sizeMetres < 2000.0f ? sizeMetres : 2000.0f;
        g_folder = folder;
        g_started = GetTickCount();
        try
        {
            if (!Begin())
                return;
            const Step steps[] = { { "game_top_fov20", GAME_TOP, 20.0f },
                                   { "own_top_fov20", OWN_TOP, 20.0f },
                                   { "own_game_view", OWN_GAME_VIEW, 0.0f } };
            g_steps.assign(steps, steps + 3);
            StartStep(g_steps.back());
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

    void Tick()
    {
        if (g_steps.empty() || g_workspace == NULL)
            return;
        --g_wait;
        try
        {
            const Step step = g_steps.back();
            // Камеру игры - наверх на два последних кадра (ставим каждый
            // кадр: игра может двигать её сама).
            if (g_wait <= 2 && g_wait > 0 && step.mode == GAME_TOP)
                AimDown(GameRenderer()->camera, step.fov);
            NoOverlays();
            if (g_wait > 0)
                return;
            SaveShot(step.name);
            RestoreGameCamera();
            g_steps.pop_back();
            if (!g_steps.empty())
            {
                StartStep(g_steps.back());
                return;
            }
            char note[96];
            sprintf_s(note, "done in %u ms", GetTickCount() - g_started);
            Log(note);
            StopWorkspace();
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
        g_steps.clear();
        try
        {
            RestoreGameCamera();
            StopWorkspace();
        }
        catch (...)
        {
            g_workspace = NULL;
        }
    }
}
