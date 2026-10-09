// BuildModeGizmo - стрелочки перемещения для своей стройплощадки.
//
// ОСНОВНОЙ РЕЖИМ (с 04.10.2026, по просьбе пользователя и Asur): в
// обычной игре, без меню разработчика. Здание поставили и подтвердили,
// оно ещё не построено - выделяешь стройплощадку, и на ней появляются
// стрелочки редактора (TransformWindow с газмо): тянешь - здание
// сдвигается, кнопкой окна - поворот. Чтобы поправить место, больше не
// нужно разбирать площадку или лезть в Shift+F12.
//
// Как: TransformWindow::show(корневой узел здания, &позиция, &поворот,
// без масштаба, родительский узел) - то же окно, что открывает редактор;
// каждый кадр зовём его updateGizmo() (в редакторе это делает
// LevelEditor); при изменении - notifyChange / moveMountedBuildings /
// forceValidUsageNodesValidation, как редактор дописывает сдвиг.
// Окно держится на площадке, пока не выделят другой объект, площадка не
// достроится или окно не закроют крестиком.
//
// РЕЖИМ РЕДАКТОРА (ниже, первая версия): стрелочки в обычном режиме
// строительства меню разработчика. Не подтвердился (журнал: result=0),
// оставлен выключенным - настройка EditorArrows.
//
// ЗАДАЧА (просьба Asur). В меню разработчика два режима строительства.
// В режиме интерьеров уже стоящее здание двигается стрелочками прямо во
// время строительства. В обычном - нет: надо выйти из режима, подвинуть,
// войти обратно.
//
// ЧТО ДЕЛАЕМ. Ничего не переписываем. Стрелочки рисует и обслуживает
// сама игра - LevelEditor::updateGizmo(): она решает, есть ли что
// двигать, открывает окно преобразования, ведёт его и применяет
// изменения. Ровно это и работает в режиме интерьеров. Значит задача не
// «сделать газмо», а «дать игровой процедуре отработать и в обычном
// режиме».
//
// Поэтому плагин вешается на две точки:
//
//   LevelEditor::updateGizmo - только отмечает, что игра вызвала её
//                              сама в этом кадре;
//   LevelEditor::update      - после работы оригинала смотрит: если
//                              игра газмо не трогала, а условия
//                              подходят, вызывает её сама.
//
// Никакой своей арифметики и никакого TransformWindow::show с
// угаданными аргументами: вызывается та же функция игры, с тем же
// состоянием, что и в режиме интерьеров.
//
// ПОЧЕМУ БЕЗ ПРЕДВАРИТЕЛЬНОГО РАЗБОРА КОДА. Прочитать, где именно игра
// отсекает газмо, нечем: установленный kenshi_x64.exe не совпадает ни с
// одной из таблиц RE_Kenshi\RVAs (по обеим лишь каждый десятый адрес
// попадает в начало функции, постоянного сдвига между ними нет). Похоже
// на защищённый образ, расшифровывающий себя при запуске. Отсюда
// устройство правки: она покрывает оба правдоподобных случая - и
// «отсекает update», и «отсекает сама updateGizmo», - а при Debug=1
// пишет в журнал, какой путь сработал. Если не поможет, журнал скажет,
// почему, и второго вслепую захода не понадобится.
//
// ЧЕГО ЗДЕСЬ НАРОЧНО НЕТ. Дописывания изменений в здание
// (notifyChange / moveMountedBuildings) по умолчанию нет: если
// updateGizmo делает это сама, наше вмешательство только помешает.
// Включается в настройках, если окажется, что здание двигается на
// экране, но сдвиг не сохраняется.

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "build_mode_gizmo"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>

#include <cstdio>
#include <cmath>
#include <string>

#include <KenshiSlots.h>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Building/Building.h>
#include <kenshi/gui/LevelEditor.h>
#include <kenshi/gui/TransformWindow.h>
#include <kenshi/gui/InteriorModeButtonWindow.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/Faction.h>
#include <kenshi/util/hand.h>
#include <ogre/OgreSceneNode.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;

    // Пока игрок ведёт по земле новую постройку, газмо не трогаем:
    // щелчок должен ставить здание, а не хватать стрелочку.
    bool g_skipDuringPlacement = true;

    // Дописывать изменения в здание самим. По умолчанию нет - см. шапку.
    bool g_applyChanges = false;

    bool g_debug = false;

    // Стрелочки на своей стройплощадке в обычной игре (основной режим).
    bool g_siteArrows = true;
    bool g_snapToGround = true;     // после перестройки площадка садится на землю (как раньше)

    // Первая версия: стрелочки в режиме строительства редактора.
    bool g_editorArrows = false;


    std::string IniPath()
    {
        char path[MAX_PATH] = {};

        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string file(path, length);

        const std::string::size_type dot = file.rfind('.');
        if (dot == std::string::npos)
            return std::string();

        return file.substr(0, dot) + ".ini";
    }


    void WriteDefaultIni(const std::string& ini)
    {
        FILE* f = NULL;
        if (fopen_s(&f, ini.c_str(), "w") != 0 || f == NULL)
            return;

        // По-английски: правило стенда - в бинарнике кириллицы нет.
        // Русский .ini кладётся рядом с DLL при установке, эти строки
        // пишутся только если файла не оказалось.
        fputs("; BuildModeGizmo\n", f);
        fputs(";\n", f);
        fputs("; Enabled - 0 turns the plugin off completely.\n", f);
        fputs("; SkipDuringPlacement - leave the gizmo alone while a new\n", f);
        fputs(";   building is being placed. Turn it off only if the\n", f);
        fputs(";   arrows never show up at all.\n", f);
        fputs("; ApplyChanges - write the move back into the building\n", f);
        fputs(";   ourselves. Only needed if a building moves on screen\n", f);
        fputs(";   but the move is not kept.\n", f);
        fputs("; Debug - details in RE_Kenshi_log.txt.\n", f);
        fputs("\n[BuildModeGizmo]\n", f);
        fputs("Enabled=1\n", f);
        fputs("SiteArrows=1\n", f);
        fputs("EditorArrows=0\n", f);
        fputs("SkipDuringPlacement=1\n", f);
        fputs("ApplyChanges=0\n", f);
        fputs("Debug=0\n", f);

        fclose(f);
    }


    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        if (GetFileAttributesA(ini.c_str()) == INVALID_FILE_ATTRIBUTES)
            WriteDefaultIni(ini);

        const char* const section = "BuildModeGizmo";

        g_enabled =
            GetPrivateProfileIntA(section, "Enabled", 1, ini.c_str()) != 0;
        g_skipDuringPlacement =
            GetPrivateProfileIntA(section, "SkipDuringPlacement", 1,
                                  ini.c_str()) != 0;
        g_applyChanges =
            GetPrivateProfileIntA(section, "ApplyChanges", 0, ini.c_str()) != 0;
        g_debug =
            GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
        g_siteArrows =
            GetPrivateProfileIntA(section, "SiteArrows", 1, ini.c_str()) != 0;
        g_snapToGround =
            GetPrivateProfileIntA(section, "SnapToGround", 1, ini.c_str()) != 0;
        g_editorArrows =
            GetPrivateProfileIntA(section, "EditorArrows", 0, ini.c_str()) != 0;
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    bool (*g_origGizmo)(LevelEditor* thisptr) = NULL;
    void (*g_origUpdate)(LevelEditor* thisptr) = NULL;

    bool g_gameRanGizmo = false;    // игра вызвала updateGizmo сама
    bool g_weRanGizmo = false;      // вызвали мы
    bool g_gizmoResult = false;

    std::string g_lastReport;
    bool g_toldAboutPlacement = false;


    bool Gizmo_hook(LevelEditor* thisptr)
    {
        g_gameRanGizmo = true;
        return g_origGizmo(thisptr);
    }


    // Сдвиг сохранится, только если здание об этом узнает. Нужно не
    // всегда - см. шапку, - поэтому по умолчанию выключено.
    void ApplyIfChanged(LevelEditor* editor)
    {
        if (!g_applyChanges)
            return;

        TransformWindow* const transform = TransformWindow::getSingleton();
        if (transform == NULL || !transform->hasChanged())
            return;

        Building* const building = editor->selectedObject.getBuilding();
        if (building != NULL)
        {
            building->notifyChange();
            building->moveMountedBuildings();
            building->forceValidUsageNodesValidation();
        }

        transform->clearChangedFlag();

        if (g_debug)
            DebugLog("BuildModeGizmo: change written back to the building");
    }


    void DriveGizmo(LevelEditor* editor)
    {
        if (!g_enabled || !g_editorArrows || g_gameRanGizmo)
            return;

        if (editor == NULL || !editor->levelEditModeOn)
            return;

        if (ou == NULL || ou->player == NULL)
            return;

        if (g_skipDuringPlacement && ou->player->isObjectPlacementMode())
        {
            // Один раз говорим вслух: если стрелочек нет и виновата
            // именно эта проверка, пусть это будет видно без Debug=1.
            if (!g_toldAboutPlacement)
            {
                g_toldAboutPlacement = true;
                DebugLog("BuildModeGizmo: object placement is active, "
                         "the gizmo is left alone "
                         "(SkipDuringPlacement=0 to override)");
            }
            return;
        }

        // Двигать нечего - и звать нечего.
        if (editor->selectedObject.getRootObject() == NULL)
            return;

        g_weRanGizmo = true;
        g_gizmoResult = g_origGizmo(editor);

        ApplyIfChanged(editor);
    }


    void Report(LevelEditor* editor)
    {
        if (!g_debug || editor == NULL || !editor->levelEditModeOn)
            return;

        PlayerInterface* const player = ou ? ou->player : NULL;
        if (player == NULL)
            return;

        InteriorModeButtonWindow* const interior = editor->interiorModeWindow;
        TransformWindow* const transform = TransformWindow::getSingleton();
        const hand& selected = editor->selectedObject;

        char line[640];
        sprintf_s(line,
            "BuildModeGizmo: gameRanGizmo=%d weRanGizmo=%d result=%d | "
            "placement=%d build=%d interiorEdit=%d | "
            "selected(type=%d index=%u serial=%u) | "
            "transform visible=%d active=%d node=%s mode=%d changed=%d",
            g_gameRanGizmo ? 1 : 0,
            g_weRanGizmo ? 1 : 0,
            g_gizmoResult ? 1 : 0,
            player->isObjectPlacementMode() ? 1 : 0,
            player->isBuildMode() ? 1 : 0,
            editor->isInteriorEditMode() ? 1 : 0,
            static_cast<int>(selected.type), selected.index, selected.serial,
            transform ? (transform->isVisible() ? 1 : 0) : -1,
            transform ? (transform->isActive() ? 1 : 0) : -1,
            (transform && transform->node) ? "yes" : "no",
            transform ? transform->mode : -1,
            transform ? (transform->hasChanged() ? 1 : 0) : -1);

        if (line == g_lastReport)
            return;

        g_lastReport = line;
        DebugLog(line);
    }


    DWORD g_editorSeenMs = 0;       // когда редактор последний раз был включён

    void Update_hook(LevelEditor* thisptr)
    {
        g_gameRanGizmo = false;
        g_weRanGizmo = false;

        g_origUpdate(thisptr);

        // В редакторе стрелочками ведает он сам - режим площадки молчит.
        if (thisptr != NULL && thisptr->levelEditModeOn)
            g_editorSeenMs = GetTickCount();

        DriveGizmo(thisptr);
        Report(thisptr);
    }



    // ---------------------------------------------------------------
    // Стрелочки на своей стройплощадке (основной режим)
    // ---------------------------------------------------------------

    // Площадку узнаём по указателю здания: сравнение hand через его
    // operator== в первой проверке давало «не та» каждый кадр - окно
    // закрывалось и открывалось 10 тысяч раз, мерцало и не давалось в руки.
    // Указатели только сравниваем; живо ли здание - спрашиваем hand.
    hand g_siteTarget;              // площадка под стрелочками
    Building* g_siteBuilding = NULL;
    bool g_siteOpen = false;
    Building* g_siteDismissed = NULL;   // окно закрыли крестиком - не открывать, пока не выделят другое
    bool g_gizmoDragging = false;   // при зажатой кнопке газмо двигал здание - тянут стрелочку
    bool g_lastActive = false;

    // Перестройка «рабочей части» после сдвига. Газмо двигает картинку;
    // точка строителей (шестерёнки, positionMarker), физика и узлы
    // использования остаются на старом месте до загрузки сохранения
    // (пользователь, 04.10). Поэтому, когда кнопку отпустили, здание
    // один раз перестраивается так же, как при загрузке.
    Ogre::Vector3 g_basePos;        // позиция и поворот здания на момент
    Ogre::Quaternion g_baseRot;     // прошлой перестройки (или открытия)
    bool g_pendingRebuild = false;
    DWORD g_lastChangeMs = 0;
    Ogre::Vector3 g_sitePos;        // их правит окно преобразования
    Ogre::Quaternion g_siteRot;
    bool g_siteBroken = false;      // вызов игры упал - режим выключен до перезапуска
    bool g_toldNoTransform = false;

    // Своя, ещё не достроенная постройка с узлом сцены.
    Building* OwnSite(Building* b)
    {
        if (b == NULL)
            return NULL;
        Faction* const faction = b->getFaction();
        if (faction == NULL || !faction->isThePlayer())
            return NULL;
        Building::ConstructionState* const state = b->getBuildState();
        if (state == NULL || state->isComplete)
            return NULL;
        if (b->getRootNode() == NULL)
            return NULL;
        return b;
    }

    // Вызовы игры - под SEH: окно преобразования создавалось для
    // редактора, и если вне его оно чего-то не найдёт, лучше выключить
    // режим, чем уронить игру.
    bool SafeShow(TransformWindow* t, Ogre::SceneNode* node, Ogre::SceneNode* parent)
    {
        __try
        {
            t->show(node, &g_sitePos, &g_siteRot, NULL, parent);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SafeUpdateGizmo(TransformWindow* t)
    {
        __try
        {
            t->updateGizmo();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Газмо двигает только узел сцены. Позиция и поворот самого здания
    // (RootObjectBase::pos, RootObject::rot) - отдельные поля: по ним
    // считаются физика и сохранение. Первый прогон: узел уехал на 4612,
    // а здание осталось на 4626 - после загрузки площадка вернулась бы.
    // Поэтому сначала переписываем поля по узлу, потом notifyChange.
    bool SafeApplyMove(Building* b)
    {
        __try
        {
            Ogre::SceneNode* const node = b->getRootNode();
            if (node != NULL)
            {
                b->pos = node->getPosition();
                b->rot = node->getOrientation();
            }
            b->notifyChange();
            b->moveMountedBuildings();
            b->forceValidUsageNodesValidation();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void SafeClose(TransformWindow* t)
    {
        __try
        {
            if (t->isVisible())
                t->close();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // Всё - под SEH: функции загрузки здания вне загрузки игра не зовёт.
    bool SafeRebuild(Building* b, Ogre::Vector3* markerBefore, Ogre::Vector3* markerAfter)
    {
        __try
        {
            const Ogre::Vector3 newPos = b->pos;
            const Ogre::Quaternion newRot = b->rot;

            // Точку строителей переносим вместе со зданием: тот же сдвиг и
            // тот же поворот вокруг здания. Только если она в мировых
            // координатах (у неё порядок величин как у позиции здания).
            *markerBefore = b->positionMarker;
            if (b->positionMarker.squaredLength() > 1000000.0f)
            {
                const Ogre::Vector3 local = g_baseRot.Inverse() * (b->positionMarker - g_basePos);
                b->positionMarker = newRot * local + newPos;
            }
            *markerAfter = b->positionMarker;

            b->destroyPhysical();
            b->createPhysical();
            b->updateUsageNodes();
            b->forceValidUsageNodesValidation();
            b->moveMountedBuildings();
            b->notifyChange();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void OpenSite(Building* b, const hand& h);

    // Высота после перестройки: игра сажает площадку на землю (08.10.2026:
    // поднятое стрелкой здание после отпускания падало вниз). При
    // SnapToGround=0 возвращаем ту высоту, до которой подняли.
    bool SafeKeepHeight(Building* b, float y)
    {
        __try
        {
            Ogre::SceneNode* const node = b->getRootNode();
            if (node != NULL)
            {
                Ogre::Vector3 at = node->getPosition();
                at.y = y;
                node->setPosition(at);
            }
            b->pos.y = y;
            b->notifyChange();
            b->moveMountedBuildings();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Перестройка: окно стрелочек закрываем (оно держит узел здания,
    // а пересоздание физики может узел заменить), перестраиваем и
    // открываем снова на том узле, что есть теперь.
    bool RebuildSite(Building* b)
    {
        g_pendingRebuild = false;
        TransformWindow* const t = TransformWindow::getSingleton();
        const bool wasOpen = g_siteOpen;
        const hand target = g_siteTarget;
        if (t != NULL)
            SafeClose(t);
        g_siteOpen = false;

        Ogre::SceneNode* const nodeBefore = b->getRootNode();
        Ogre::Vector3 markerBefore = Ogre::Vector3::ZERO;
        Ogre::Vector3 markerAfter = Ogre::Vector3::ZERO;
        const float wantY = b->pos.y;
        if (!SafeRebuild(b, &markerBefore, &markerAfter))
        {
            ErrorLog("BuildModeGizmo: rebuilding the moved site failed; it will be right after a reload");
            g_siteTarget = hand();
            g_siteBuilding = NULL;
            return false;
        }
        const float rebuiltY = b->getRootNode() != NULL ? b->getRootNode()->getPosition().y : b->pos.y;
        if (!g_snapToGround && (fabsf(b->pos.y - wantY) > 0.01f || fabsf(rebuiltY - wantY) > 0.01f))
        {
            if (!SafeKeepHeight(b, wantY))
                ErrorLog("BuildModeGizmo: keeping the raised height failed");
        }
        if (g_debug)
        {
            char note[200];
            sprintf_s(note, "BuildModeGizmo: height before rebuild %.2f, after %.2f (node %.2f), now %.2f",
                      wantY, b->pos.y, rebuiltY, b->pos.y);
            DebugLog(note);
        }
        g_basePos = b->pos;
        g_baseRot = b->rot;

        if (g_debug)
        {
            char note[320];
            sprintf_s(note, "BuildModeGizmo: site rebuilt at %.1f %.1f %.1f, marker %.1f %.1f %.1f -> %.1f %.1f %.1f, node %s",
                      b->pos.x, b->pos.y, b->pos.z,
                      markerBefore.x, markerBefore.y, markerBefore.z,
                      markerAfter.x, markerAfter.y, markerAfter.z,
                      b->getRootNode() == nodeBefore ? "kept" : "replaced");
            DebugLog(note);
        }

        if (wasOpen && b->getRootNode() != NULL)
            OpenSite(b, target);
        return true;
    }

    void CloseSite(const char* why)
    {
        // Сдвиг не перестроен - перестроить, пока здание ещё наше.
        if (g_pendingRebuild && g_siteBuilding != NULL && g_siteTarget.getBuilding() == g_siteBuilding)
        {
            g_siteOpen = false;
            RebuildSite(g_siteBuilding);
        }
        g_pendingRebuild = false;

        if (g_siteOpen)
        {
            TransformWindow* const t = TransformWindow::getSingleton();
            if (t != NULL)
                SafeClose(t);
            if (g_debug)
                DebugLog(std::string("BuildModeGizmo: site arrows closed: ") + why);
        }
        g_siteOpen = false;
        g_siteTarget = hand();
        g_siteBuilding = NULL;
        g_gizmoDragging = false;
    }

    void Broken(const char* what)
    {
        g_siteBroken = true;
        g_siteOpen = false;
        g_siteTarget = hand();
        g_siteBuilding = NULL;
        ErrorLog(std::string("BuildModeGizmo: ") + what +
                 " failed outside the editor, site arrows are off until restart");
    }

    void OpenSite(Building* b, const hand& h)
    {
        TransformWindow* const t = TransformWindow::getSingleton();
        if (t == NULL)
        {
            if (!g_toldNoTransform)
            {
                g_toldNoTransform = true;
                ErrorLog("BuildModeGizmo: no transform window outside the editor, site arrows are off");
            }
            return;
        }

        Ogre::SceneNode* const node = b->getRootNode();
        g_sitePos = node->getPosition();
        g_siteRot = node->getOrientation();
        if (!SafeShow(t, node, node->getParentSceneNode()))
        {
            Broken("TransformWindow::show");
            return;
        }

        g_siteOpen = true;
        g_siteTarget = h;
        g_siteBuilding = b;
        g_basePos = b->pos;
        g_baseRot = b->rot;
        if (g_debug)
        {
            char note[256];
            sprintf_s(note, "BuildModeGizmo: site arrows on '%s' at %.1f %.1f %.1f",
                      b->getName().c_str(), g_sitePos.x, g_sitePos.y, g_sitePos.z);
            DebugLog(note);
        }
    }

    void TickSite()
    {
        if (!g_enabled || !g_siteArrows || g_siteBroken || ou == NULL || ou->player == NULL)
        {
            if (g_siteOpen)
                CloseSite("plugin off");
            return;
        }

        // Редактор включён - стрелочки его, мы не вмешиваемся и окно не
        // закрываем (оно теперь редактора).
        if (g_editorSeenMs != 0 && GetTickCount() - g_editorSeenMs < 500)
        {
            g_siteOpen = false;
            g_siteTarget = hand();
            g_siteBuilding = NULL;
            return;
        }

        // Ведут новую постройку - щелчок ставит здание.
        if (ou->player->isObjectPlacementMode())
        {
            if (g_siteOpen)
                CloseSite("placing a new building");
            return;
        }

        const hand& selection = ou->player->selectedObject;
        Building* const selectedBuilding = selection.getBuilding();
        Building* const selectedSite = OwnSite(selectedBuilding);
        if (selectedSite != NULL)
        {
            const bool same = g_siteOpen && selectedSite == g_siteBuilding;
            if (!same && selectedSite != g_siteDismissed)
            {
                CloseSite("another site selected");
                OpenSite(selectedSite, selection);
            }
        }
        else if (selection.getRootObject() != NULL)
        {
            // Выделили что-то другое - стрелочки убираем.
            if (g_siteOpen)
                CloseSite("something else selected");
            g_siteDismissed = NULL;
        }
        else if (!g_siteOpen)
        {
            g_siteDismissed = NULL;
        }
        // Ничего не выделено, а стрелочки открыты - оставляем: щелчок по
        // земле рядом со стрелочкой не должен их убирать.

        if (!g_siteOpen)
            return;

        // Здание живо и всё ещё площадка? Выделено - берём его, иначе
        // спрашиваем свой hand и сверяем с запомненным указателем.
        Building* site = selectedSite == g_siteBuilding ? selectedSite : NULL;
        if (site == NULL)
        {
            Building* const byHand = g_siteTarget.getBuilding();
            site = byHand == g_siteBuilding ? OwnSite(byHand) : NULL;
        }
        TransformWindow* const t = TransformWindow::getSingleton();
        if (site == NULL || t == NULL)
        {
            CloseSite(site == NULL ? "site built, removed or gone" : "no transform window");
            return;
        }

        if (!t->isVisible())
        {
            // Окно закрыли крестиком - до выделения другого не открываем.
            if (g_debug)
                DebugLog("BuildModeGizmo: transform window closed by the player");
            g_siteDismissed = g_siteBuilding;
            g_siteOpen = false;
            g_siteTarget = hand();
            g_siteBuilding = NULL;
            return;
        }

        if (!SafeUpdateGizmo(t))
        {
            Broken("TransformWindow::updateGizmo");
            return;
        }

        const bool leftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!leftDown)
            g_gizmoDragging = false;

        const bool active = t->isActive();
        if (g_debug && active != g_lastActive)
            DebugLog(active ? "BuildModeGizmo: gizmo active" : "BuildModeGizmo: gizmo idle");
        g_lastActive = active;

        if (t->hasChanged())
        {
            g_pendingRebuild = true;
            g_lastChangeMs = GetTickCount();
            if (leftDown)
                g_gizmoDragging = true;
            if (!SafeApplyMove(site))
            {
                Broken("writing the move into the building");
                return;
            }
            t->clearChangedFlag();

            if (g_debug)
            {
                const Ogre::Vector3 nodePos = site->getRootNode()->getPosition();
                const Ogre::Vector3 objPos = site->getPosition();
                char note[256];
                sprintf_s(note, "BuildModeGizmo: site moved, node %.1f %.1f %.1f, building %.1f %.1f %.1f",
                          nodePos.x, nodePos.y, nodePos.z, objPos.x, objPos.y, objPos.z);
                DebugLog(note);
            }
        }

        // Кнопку отпустили, и сдвиг улёгся - перестраиваем рабочую часть.
        if (g_pendingRebuild && !leftDown && GetTickCount() - g_lastChangeMs > 150)
            RebuildSite(site);
    }

    // Рамка выделения. Стрелочку тянут левой кнопкой - та же кнопка у
    // игры рисует рамку выделения и по отпусканию выделяет, что в рамке
    // (пользователь: «и выделение мышью срабатывает, когда перетаскиваешь»).
    // Пока газмо занят, рамку не начинаем, а начатую гасим.
    // Занят: окно говорит isActive() или в этом кадре при зажатой кнопке
    // газмо уже сдвинул здание (на случай, если isActive значит другое).
    bool GizmoBusy()
    {
        if (!g_siteOpen)
            return false;
        if (g_gizmoDragging)
            return true;
        TransformWindow* const t = TransformWindow::getSingleton();
        return t != NULL && t->isActive();
    }

    void (*g_origBoxStart)(SelectionBox* self, const Ogre::Vector2& m) = NULL;
    void (*g_origBoxUpdate)(SelectionBox* self, const Ogre::Vector2& m) = NULL;

    void BoxStart_hook(SelectionBox* self, const Ogre::Vector2& m)
    {
        if (GizmoBusy())
            return;
        g_origBoxStart(self, m);
    }

    void BoxUpdate_hook(SelectionBox* self, const Ogre::Vector2& m)
    {
        if (GizmoBusy())
        {
            if (self != NULL && self->isActive())
                self->cancel();
            return;
        }
        g_origBoxUpdate(self, m);
    }

    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);
        TickSite();
    }

    // ---------------------------------------------------------------
    // Установка
    // ---------------------------------------------------------------

    bool Hook(int slot, const char* name, void* detour, void** original)
    {
        void* const target = KenshiSlots::At(slot);
        if (target == NULL)
        {
            ErrorLog("BuildModeGizmo: no address for " + std::string(name));
            return false;
        }

        if (KenshiLib::SUCCESS != KenshiLib::AddHook(target, detour, original))
        {
            ErrorLog("BuildModeGizmo: could not hook " + std::string(name));
            return false;
        }

        if (g_debug)
        {
            char note[192];
            sprintf_s(note, "BuildModeGizmo: hooked %s at rva 0x%08llX",
                      name,
                      static_cast<unsigned long long>(
                          reinterpret_cast<uintptr_t>(target)
                          - reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL))));
            DebugLog(note);
        }

        return true;
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "BuildModeGizmo", "Build Mode Gizmo", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Move arrows on your construction site: shift a placed building before it is built, without the developer menu."));
    api->section(api, Tr("Construction sites"));
    api->toggle(api, "BuildModeGizmo", "Enabled", Tr("Enabled"), Tr("Turns the whole plugin on or off."), 1, MCM_RESTART);
    api->toggle(api, "BuildModeGizmo", "SiteArrows", Tr("Arrows on your construction site"), Tr("Select your building that is not built yet: move arrows appear on it. Drag an arrow to shift it, the window button switches to rotation. Close the window to hide them."), 1, 0);
    api->toggle(api, "BuildModeGizmo", "SnapToGround", Tr("Settle on the ground after a move"), Tr("When the arrow is released, the building is rebuilt where it now stands, and the game puts it on the ground. Off - it stays at the height you raised it to."), 1, 0);
    api->section(api, Tr("Developer menu"));
    api->toggle(api, "BuildModeGizmo", "EditorArrows", Tr("Arrows in the developer build mode"), Tr("The first version: arrows in the normal build mode of the developer menu (Shift+F12). Experimental, off by default."), 0, 0);
    api->toggle(api, "BuildModeGizmo", "SkipDuringPlacement", Tr("Not while placing a new building"), Tr("A click then places the building instead of grabbing an arrow. Developer menu only."), 1, 0);
    api->toggle(api, "BuildModeGizmo", "ApplyChanges", Tr("Write the move into the building"), Tr("Developer menu only: if a building moves on screen but the move is not saved."), 0, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "BuildModeGizmo", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: when the arrows open and close and where the building moved."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("BuildModeGizmo: disabled in the ini");
        return;
    }

    // Основной режим: покадровое обновление интерфейса игры.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update),
            GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("BuildModeGizmo: could not hook the interface update, no site arrows");
    }

    // Рамка выделения не мешает тянуть стрелочку.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&SelectionBox::start), BoxStart_hook, &g_origBoxStart)
        || KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&SelectionBox::update), BoxUpdate_hook, &g_origBoxUpdate))
    {
        ErrorLog("BuildModeGizmo: could not hook the selection box, dragging an arrow also selects");
    }

    // Номера ячеек привязаны к сборке KenshiLib. Не сошлось - молча
    // ничего не делаем: вызов по чужому номеру это не ошибка компоновки,
    // а тихий вылет. Хуки редактора нужны и основному режиму: по ним
    // видно, что редактор включён и стрелочки - его.
    if (!KenshiSlots::Ready())
    {
        ErrorLog("BuildModeGizmo: this KenshiLib is not the one the slot "
                 "numbers were taken from, the plugin stays off");
        return;
    }

    if (!Hook(KenshiSlots::LevelEditor_updateGizmo, "LevelEditor::updateGizmo",
              Gizmo_hook, reinterpret_cast<void**>(&g_origGizmo)))
        return;

    if (!Hook(KenshiSlots::LevelEditor_update, "LevelEditor::update",
              Update_hook, reinterpret_cast<void**>(&g_origUpdate)))
        return;

    DebugLog("BuildModeGizmo: installed");
}
