// Satellite - снимок мира сверху (проба «вида со спутника»).
//
// КАК. Игра рисует мир через набор компоновщиков Ogre 2.0
// (CompositorWorkspace): тени, отложенное освещение, туман, постобработка.
// Свой «спутник» - это тот же набор, но со своей камерой (сверху,
// ортогональная проекция, север вверху) и своей текстурой вместо окна. Набор
// игры берём по имени у её же рабочего набора (Renderer::workspace), так что
// картинка - ровно как в игре, с модами на графику.
//
// ПРОБА. Пока это опыт: по клавише (SatelliteShotKey) - три снимка с
// высот 400, 1500 и 5000 м, по одному на кадр с паузой (освещению с
// адаптацией яркости нужно несколько кадров), в папку мода картинками BMP.
// По ним видно, что игра рисует сверху: дальность прорисовки, туман, свет.
// Пока набор включён, игра рисует мир дважды - это несколько десятков
// кадров с просадкой, и только по клавише.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include <Debug.h>

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
    Ogre::Camera* g_camera = NULL;
    Ogre::CompositorWorkspace* g_workspace = NULL;

    std::vector<float> g_heights;       // высоты снимков, ещё не сделанных
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

    // Камеру - над центром снимка на высоте height, взглядом вниз, север
    // (-Z мира: на картинке карты он вверху) - вверху снимка.
    void AimCamera(float height)
    {
        Renderer* const r = GameRenderer();
        // Сцена может быть сдвинута относительно мира (чтобы числа были
        // мелкими у камеры): мировая точка в сцене - минус этот сдвиг.
        const Ogre::Vector3 origin = r->getSceneRelativeOrigin();
        const Ogre::Vector3 scene = g_center - origin;
        g_camera->setPosition(scene.x, scene.y + height, scene.z);
        g_camera->setNearClipDistance(1.0f);
        g_camera->setFarClipDistance(height + 20000.0f);
        char note[300];
        const Ogre::Vector3 main = r->camera != NULL ? r->camera->getDerivedPosition() : Ogre::Vector3::ZERO;
        sprintf_s(note, "shot from %.0f m: world centre %.0f %.0f %.0f, scene origin %.0f %.0f %.0f, "
                        "game camera at %.0f %.0f %.0f (scene)",
                  height, g_center.x, g_center.y, g_center.z, origin.x, origin.y, origin.z, main.x, main.y, main.z);
        Log(note);
    }

    bool Begin()
    {
        Renderer* const r = GameRenderer();
        if (r == NULL || r->root == NULL || r->scene == NULL || r->workspace == NULL)
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
        const Ogre::IdString defName = WorkspaceDefAccess::Name(def);
        if (g_target.isNull())
        {
            g_target = Ogre::TextureManager::getSingleton().createManual(
                "BestMinimap_Satellite", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, kSide, kSide, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);
        }
        if (g_camera == NULL)
        {
            g_camera = r->scene->createCamera("BestMinimap_SatelliteCamera");
            g_camera->setFixedYawAxis(false);
            // Камера смотрит по -Z; поворот на -90 градусов вокруг X - взгляд
            // вниз (-Y), а «верх» снимка - -Z, север.
            g_camera->setOrientation(Ogre::Quaternion(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_X));
            g_camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
            g_camera->setAspectRatio(1.0f);
        }
        g_camera->setOrthoWindow(g_size, g_size);
        Ogre::RenderTarget* const rt = g_target->getBuffer()->getRenderTarget();
        g_workspace = r->root->getCompositorManager2()->addWorkspace(r->scene, rt, g_camera, defName, true, 0);
        char note[160];
        sprintf_s(note, "started: %.0f m square, %dx%d, game workspace definition %08X",
                  g_size, kSide, kSide, defName.mHash);
        Log(note);
        return g_workspace != NULL;
    }

    void SaveShot(float height)
    {
        std::vector<unsigned char> pixels(static_cast<size_t>(kSide) * kSide * 4, 0);
        Ogre::PixelBox box(kSide, kSide, 1, Ogre::PF_A8R8G8B8, &pixels[0]);
        g_target->getBuffer()->blitToMemory(box);
        // Альфа из рендера бывает любой - в BMP делаем непрозрачной.
        for (size_t i = 3; i < pixels.size(); i += 4)
            pixels[i] = 255;
        char name[64];
        sprintf_s(name, "satellite_probe_%dm.bmp", static_cast<int>(height));
        const std::string file = g_folder + name;
        Log(std::string(SaveBmp(file, pixels, kSide, kSide) ? "saved " : "could not save ") + file);
    }
}

namespace Satellite
{
    void RequestProbe(const Ogre::Vector3& centerWorld, float sizeMetres, const std::string& folder)
    {
        if (!g_heights.empty())
            return;                 // ещё снимаем прошлый
        g_center = centerWorld;
        g_size = sizeMetres;
        g_folder = folder;
        g_started = GetTickCount();
        try
        {
            if (!Begin())
            {
                StopWorkspace();
                return;
            }
            g_heights.push_back(5000.0f);
            g_heights.push_back(1500.0f);
            g_heights.push_back(400.0f);
            AimCamera(g_heights.back());
            g_wait = kSettleFrames;
        }
        catch (const Ogre::Exception& e)
        {
            Log("failed: " + e.getFullDescription());
            g_heights.clear();
            StopWorkspace();
        }
        catch (...)
        {
            Log("failed with an unknown error");
            g_heights.clear();
            StopWorkspace();
        }
    }

    void Tick()
    {
        if (g_heights.empty() || g_workspace == NULL)
            return;
        if (--g_wait > 0)
            return;
        try
        {
            SaveShot(g_heights.back());
            g_heights.pop_back();
            if (!g_heights.empty())
            {
                AimCamera(g_heights.back());
                g_wait = kSettleFrames;
                return;
            }
            char note[96];
            sprintf_s(note, "done in %u ms", GetTickCount() - g_started);
            Log(note);
        }
        catch (const Ogre::Exception& e)
        {
            Log("failed: " + e.getFullDescription());
            g_heights.clear();
        }
        catch (...)
        {
            Log("failed with an unknown error");
            g_heights.clear();
        }
        StopWorkspace();
    }

    void Reset()
    {
        g_heights.clear();
        try
        {
            StopWorkspace();
        }
        catch (...)
        {
            g_workspace = NULL;
        }
    }
}
