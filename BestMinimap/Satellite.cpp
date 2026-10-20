// Satellite - снимок мира сверху (проба «вида со спутника»).
//
// КАК. Игра рисует мир через набор компоновщиков Ogre 2.0
// (CompositorWorkspace): тени, отложенное освещение, туман, постобработка.
// Снимок - это второй экземпляр того же набора (определение берём у
// рабочего набора игры, Renderer::workspace), но с нашей текстурой вместо
// окна. Картинка - как в игре, с модами на графику.
//
// КАМЕРА - только камера игры. Свою (ортогональную, перспективную, на своём
// узле сцены) игра не обслуживает: пробы 1, 2 и 4 (20.10.2026) дали мир
// чёрным или пустым - освещение, тени и что видно игра готовит под свою
// главную камеру. Проба 3 (камера игры, перенесённая наверх на два кадра)
// дала настоящий вид сверху, но на экране мигал вид сверху.
//
// БЕЗ МИГАНИЯ. Ogre рисует наборы по очереди в одном кадре: наш - первым,
// окно игры - после. Слушатель нашего набора перед его отрисовкой ставит
// камеру игры наверх (и на кадр снимка прячет интерфейс и курсор: MyGUI
// рисуется в любую цель рендера, выключенные оверлеи не помогли), слушатель
// набора окна перед его отрисовкой всё возвращает. Экран вид сверху не
// видит. У набора окна свой слушатель может уже быть - встаём цепочкой.
//
// ПРОБА. Пока это опыт: по клавише (SatelliteShotKey) - два снимка с углами
// обзора 20 и 60 градусов (чем уже угол, тем выше камера) в папку мода
// картинками BMP. Пока набор включён, игра рисует мир дважды - пара
// десятков кадров с просадкой, только по клавише.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cmath>
#include <cstdio>
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
    Ogre::CompositorWorkspace* g_workspace = NULL;     // наш набор
    Ogre::CompositorWorkspace* g_gameWorkspace = NULL; // набор окна игры, на котором наш слушатель

    struct Step { const char* name; float fov; };
    std::vector<Step> g_steps;          // ещё не сделанные (с конца)
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

    // Над центром снимка, взглядом вниз, север (-Z мира: на картинке карты
    // он вверху) - вверху снимка. Высота - такая, чтобы при угле fov квадрат
    // g_size на уровне центра влез ровно в кадр. Камера висит на узле сцены -
    // место и поворот задаём относительно узла.
    void AimDown(float fov)
    {
        Ogre::Camera* const cam = GameRenderer()->camera;
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
        const float height = g_size * 0.5f / std::tan(fov * 0.5f * 3.14159265f / 180.0f);
        // Сцена сдвинута относительно мира (числа у камеры мелкие): мировая
        // точка в сцене - минус этот сдвиг.
        const Ogre::Vector3 scene = g_center - GameRenderer()->getSceneRelativeOrigin() +
                                    Ogre::Vector3(0.0f, height, 0.0f);
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
        cam->setFOVy(Ogre::Degree(fov));
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

    // ---- слушатели наборов ----
    // Наш набор: перед отрисовкой - камера наверх, на кадр снимка - без
    // интерфейса.
    struct ShotListener : public Ogre::CompositorWorkspaceListener
    {
        virtual void workspacePreUpdate(Ogre::CompositorWorkspace*)
        {
            if (g_steps.empty())
                return;
            AimDown(g_steps.back().fov);
            if (g_wait == 1)
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

    void StopWorkspace()
    {
        Renderer* const r = GameRenderer();
        if (g_gameWorkspace != NULL)
        {
            // Снимаем свой слушатель, только если он всё ещё наш.
            if (g_gameWorkspace->getListener() == &g_restoreListener)
                g_gameWorkspace->setListener(g_restoreListener.next);
            g_gameWorkspace = NULL;
        }
        if (g_workspace != NULL && r != NULL && r->root != NULL)
            r->root->getCompositorManager2()->removeWorkspace(g_workspace);
        g_workspace = NULL;
        RestoreCamera();
        RestoreGui();
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

    void LogStep(const Step& step)
    {
        const Ogre::Vector3 origin = GameRenderer()->getSceneRelativeOrigin();
        char note[300];
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
        const Ogre::IdString defName = WorkspaceDefAccess::Name(def);
        if (g_target.isNull())
        {
            g_target = Ogre::TextureManager::getSingleton().createManual(
                "BestMinimap_Satellite", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, kSide, kSide, 1, 0, Ogre::PF_A8R8G8B8, Ogre::TU_RENDERTARGET);
        }
        Ogre::RenderTarget* const rt = g_target->getBuffer()->getRenderTarget();
        // Наш - первым в кадре (позиция 0), окно игры - после.
        g_workspace = r->root->getCompositorManager2()->addWorkspace(r->scene, rt, r->camera, defName, true, 0);
        if (g_workspace == NULL)
            return false;
        g_workspace->setListener(&g_shotListener);
        g_gameWorkspace = r->workspace;
        g_restoreListener.next = g_gameWorkspace->getListener();
        g_gameWorkspace->setListener(&g_restoreListener);
        char note[200];
        sprintf_s(note, "started: %.0f m square, %dx%d, game workspace definition %08X, game workspace had a listener: %s",
                  g_size, kSide, kSide, defName.mHash, g_restoreListener.next != NULL ? "yes" : "no");
        Log(note);
        return true;
    }

    void Fail(const std::string& why)
    {
        Log("failed: " + why);
        g_steps.clear();
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
            const Step steps[] = { { "top_fov60", 60.0f }, { "top_fov20", 20.0f } };
            g_steps.assign(steps, steps + 2);
            if (!Begin())
            {
                g_steps.clear();
                StopWorkspace();
                return;
            }
            LogStep(g_steps.back());
            g_wait = kSettleFrames;
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

    // Из цикла игры, до отрисовки кадра: здесь только счёт кадров и
    // чтение готового снимка; камеру и интерфейс двигают слушатели.
    void Tick()
    {
        if (g_steps.empty() || g_workspace == NULL)
            return;
        if (--g_wait > 0)
            return;
        try
        {
            SaveShot(g_steps.back().name);
            g_steps.pop_back();
            if (!g_steps.empty())
            {
                LogStep(g_steps.back());
                g_wait = kSettleFrames;
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
            StopWorkspace();
        }
        catch (...)
        {
            g_workspace = NULL;
            g_gameWorkspace = NULL;
        }
    }
}
