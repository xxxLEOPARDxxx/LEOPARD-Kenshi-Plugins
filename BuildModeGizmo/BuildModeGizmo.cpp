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
// ЛЮБОЕ ЗДАНИЕ В РЕДАКТОРЕ (11.10.2026, пояснение Asur): в обычном
// режиме строительства меню разработчика (Shift+F12) поставили здание - и
// ни его, ни другие здания выделить и сразу подвинуть нельзя; в режиме
// интерьеров - можно, это делает сама игра. Игра в обычном режиме
// стрелочек не открывает (первая версия ниже, журнал: result=0), поэтому
// там работают те же стрелочки, что на стройплощадке, - только на любом
// выделенном здании, своём и чужом, достроенном или нет. В режиме
// интерьеров плагин молчит. Настройка EditorAnyBuilding.
//
// РЕЖИМ РЕДАКТОРА (ниже, первая версия): стрелочки в обычном режиме
// строительства меню разработчика через LevelEditor::updateGizmo. Не
// подтвердился (журнал: result=0), оставлен выключенным - EditorArrows.
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
#include <IniDefaults.h>

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <set>
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
#include <kenshi/InputHandler.h>
#include <ois/OISKeyboard.h>
#include <kenshi/Faction.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/UtilityT.h>
#include <ogre/OgreSceneNode.h>
#include <ogre/OgreMovableObject.h>
#include <ogre/OgreInstancedEntity.h>
#include <mygui/MyGUI_InputManager.h>

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

    // Стрелочки на любом здании в обычном режиме строительства редактора.
    bool g_editorAnyBuilding = true;


    std::string IniFileName()
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


    // Путь к ini игрока; при первом обращении - копия или дописка из
    // <Мод>.defaults.ini (shared/IniDefaults.h, 11.10.2026).
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
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
        g_editorAnyBuilding =
            GetPrivateProfileIntA(section, "EditorAnyBuilding", 1, ini.c_str()) != 0;
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
    LevelEditor* g_editor = NULL;   // он сам - его выделение
    bool g_editorInterior = false;  // и был ли он в режиме интерьеров

    void Update_hook(LevelEditor* thisptr)
    {
        g_gameRanGizmo = false;
        g_weRanGizmo = false;

        g_origUpdate(thisptr);

        // В режиме интерьеров стрелочками ведает редактор; в обычном его
        // режиме - наши стрелочки на любом здании (EditorAnyBuilding).
        if (thisptr != NULL && thisptr->levelEditModeOn)
        {
            g_editorSeenMs = GetTickCount();
            g_editor = thisptr;
            g_editorInterior = thisptr->isInteriorEditMode();
        }

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
    // Узел здания в прошлом кадре: в редакторе флаг изменения окна может
    // не дойти до нас (11.10.2026: стрелочку хватали, «site moved» - ни разу),
    // поэтому сдвиг замечаем и по самому узлу.
    Ogre::Vector3 g_lastNodePos;
    Ogre::Quaternion g_lastNodeRot;
    bool g_haveLastNode = false;
    DWORD g_lastMoveLogMs = 0;
    bool g_toldNoTransform = false;

    bool g_inEditor = false;        // этот кадр - обычный режим редактора

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

    // В редакторе - любое здание с узлом сцены; в игре - своя площадка.
    Building* Target(Building* b)
    {
        if (!g_inEditor)
            return OwnSite(b);
        return b != NULL && b->getRootNode() != NULL ? b : NULL;
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

    // Заголовок окна стрелочек - название здания. show() его не ставит:
    // игра пишет его сама, когда выделяет здание своим путём, а наши
    // стрелочки (обычный режим стройки, выделение нашим щелчком) оставляли
    // окно с пустым заголовком (14.10.2026).
    bool SafeCaption(TransformWindow* t, const std::string& caption)
    {
        __try
        {
            t->setCaption(caption);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
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
        const std::string caption = b->getName();
        SafeCaption(t, caption);

        g_siteOpen = true;
        g_siteTarget = h;
        g_siteBuilding = b;
        g_haveLastNode = false;
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

    // Для журнала: что выделено - тип, здание, чьё, достроено ли.
    std::string DescribeSelection(const char* who, const hand& h)
    {
        char text[200];
        Building* const b = h.getBuilding();
        if (b == NULL)
        {
            sprintf_s(text, " %s(type=%d obj=%s)", who, static_cast<int>(h.type),
                      h.getRootObject() != NULL ? "yes" : "no");
            return text;
        }
        Faction* const f = b->getFaction();
        Building::ConstructionState* const s = b->getBuildState();
        sprintf_s(text, " %s(building '%s' player=%d complete=%d node=%s)", who,
                  b->getName().c_str(), f != NULL && f->isThePlayer() ? 1 : 0,
                  s == NULL ? -1 : (s->isComplete ? 1 : 0),
                  b->getRootNode() != NULL ? "yes" : "no");
        return text;
    }


    // Debug=1: состояние раз в смену - видит ли плагин редактор, его режим,
    // ведут ли постройку и что выделено (11.10.2026: в редакторе стрелочек
    // нет - найти, на каком шаге отказ).
    void ReportSite()
    {
        if (!g_debug || ou == NULL || ou->player == NULL)
            return;

        const DWORD now = GetTickCount();
        const bool editorOn = g_editorSeenMs != 0 && now - g_editorSeenMs < 500;
        char head[200];
        sprintf_s(head, "BuildModeGizmo: state editor=%d interior=%d anyBuilding=%d placement=%d build=%d open=%d",
                  editorOn ? 1 : 0, g_editorInterior ? 1 : 0, g_editorAnyBuilding ? 1 : 0,
                  ou->player->isObjectPlacementMode() ? 1 : 0, ou->player->isBuildMode() ? 1 : 0,
                  g_siteOpen ? 1 : 0);
        std::string line = head + DescribeSelection("player", ou->player->selectedObject);
        if (g_editor != NULL)
            line += DescribeSelection("editor", g_editor->selectedObject);

        static std::string last;
        if (line == last)
            return;
        last = line;
        DebugLog(line);
    }


    // ---------------------------------------------------------------
    // Выделение щелчком в обычном режиме строительства редактора
    //
    // 11.10.2026 (журнал): в этом режиме щелчок по зданию его не выделяет -
    // поставили два дома, подтвердили, и выделение редактора так и осталось
    // на том, что было при входе. Поэтому щелчок (без перетаскивания, не по
    // окну интерфейса и не по стрелочке) выделяет здание под курсором сам:
    // луч из-под мыши (UtilityT::getMouseRay), здания у луча
    // (GameWorld::getObjectsWithinSphere), точное попадание в модель
    // (LevelEditor::hitOgreMesh - им редактор выбирает мебель), ближайшее -
    // в выделение редактора (LevelEditor::setSelectedObject). Дальше - как
    // с любым выделенным зданием: стрелочки на нём.
    // ---------------------------------------------------------------

    // Попадание в здание - по модели части (как редактор выбирает мебель).
    // 11.10.2026: попадало только по двери. Корпус здания игра рисует
    // инстансингом: его Entity стоит невидимой, и такие мы пропускали.
    // Теперь проверяем и невидимые Entity; для InstancedEntity - по её
    // габаритному ящику (луч переводится в её локальные координаты).
    std::set<std::string> g_pickTypes;  // для журнала: из чего сделаны здания
    std::string g_pickBoxNote;          // и габариты первого инстанса

    void HitNode(const Ogre::Ray& ray, Ogre::SceneNode* node, float* mesh, float* box, int depth)
    {
        if (node == NULL || depth > 8)
            return;
        Ogre::SceneNode::ObjectIterator objects = node->getAttachedObjectIterator();
        while (objects.hasMoreElements())
        {
            Ogre::MovableObject* const mo = objects.getNext();
            if (mo == NULL)
                continue;
            const std::string& type = mo->getMovableType();
            if (g_debug)
                g_pickTypes.insert(type + (mo->isVisible() ? "" : "(hidden)"));
            if (type == "Entity")
            {
                const std::pair<bool, float> r = LevelEditor::hitOgreMesh(ray, *mesh, mo);
                if (r.first && r.second < *mesh)
                    *mesh = r.second;
            }
            else if (type == "InstancedEntity" && mo->getParentNode() != NULL)
            {
                Ogre::Node* const pn = mo->getParentNode();
                const Ogre::Quaternion inv = pn->_getDerivedOrientation().Inverse();
                const Ogre::Vector3 scale = pn->_getDerivedScale();
                const Ogre::Ray local((inv * (ray.getOrigin() - pn->_getDerivedPosition())) / scale,
                                      (inv * ray.getDirection()) / scale);
                const Ogre::AxisAlignedBox& aabb = static_cast<Ogre::InstancedEntity*>(mo)->getBoundingBox();
                const std::pair<bool, Ogre::Real> b = Ogre::Math::intersects(local, aabb);
                if (b.first && b.second < *box)
                    *box = b.second;
                if (g_debug && g_pickBoxNote.empty())
                {
                    const Ogre::Vector3 lo = aabb.getMinimum();
                    const Ogre::Vector3 hi = aabb.getMaximum();
                    const Ogre::Vector3 at = pn->_getDerivedPosition();
                    char note[300];
                    sprintf_s(note, " instance box %.1f %.1f %.1f .. %.1f %.1f %.1f (null %d) node %.1f %.1f %.1f scale %.2f",
                              lo.x, lo.y, lo.z, hi.x, hi.y, hi.z, aabb.isNull() ? 1 : 0,
                              at.x, at.y, at.z, scale.x);
                    g_pickBoxNote = note;
                }
            }
        }
        Ogre::Node::NodeVecIterator children = node->getChildIterator();
        while (children.hasMoreElements())
            HitNode(ray, static_cast<Ogre::SceneNode*>(children.getNext()), mesh, box, depth + 1);
    }


    // Расстояние от луча до вертикального отрезка «основание здания -
    // AXIS_HEIGHT выше»: щелчок по крыше или верху стены проходит высоко над
    // основанием, и расстояние до точки основания выходило за порог.
    const float AXIS_HEIGHT = 40.0f;

    float RayToAxis(const Ogre::Ray& ray, const Ogre::Vector3& base)
    {
        const Ogre::Vector3 o = ray.getOrigin();
        const Ogre::Vector3 d = ray.getDirection();
        const Ogre::Vector3 u = Ogre::Vector3::UNIT_Y;
        const Ogre::Vector3 w0 = o - base;
        const float b = d.dotProduct(u);
        const float dd = d.dotProduct(w0);
        const float e = u.dotProduct(w0);
        const float denom = d.dotProduct(d) - b * b;
        float s = denom > 1.0e-6f ? (d.dotProduct(d) * e - b * dd) / denom : e;
        if (s < 0.0f)
            s = 0.0f;
        if (s > AXIS_HEIGHT)
            s = AXIS_HEIGHT;
        const Ogre::Vector3 point = base + u * s;
        float along = (point - o).dotProduct(d);
        if (along < 0.0f)
            along = 0.0f;
        return (ray.getPoint(along) - point).length();
    }


    Building* PickBuilding()
    {
        const Ogre::Ray ray = UtilityT::getMouseRay();
        std::set<Building*> candidates;
        for (float d = 0.0f; d <= 6000.0f; d += 150.0f)
        {
            // У lektor нет деструктора: буфер, выделенный игрой, не
            // освобождается. Один на всё время - память не растёт.
            static lektor<RootObject*> found;
            found.clear();
            ou->getObjectsWithinSphere(found, ray.getPoint(d), 200.0f, BUILDING, 64, NULL);
            for (uint32_t i = 0; i < found.size(); ++i)
                if (found[i] != NULL && found[i]->getDataType() == BUILDING)
                    candidates.insert(static_cast<Building*>(found[i]));
        }

        // Ближайшее точное попадание; нет ни одного - ближайший ящик.
        const float NONE = 1.0e9f;
        Building* byMesh = NULL;
        Building* byBox = NULL;
        float bestMesh = NONE;
        float bestBox = NONE;
        g_pickTypes.clear();
        g_pickBoxNote.clear();
        for (std::set<Building*>::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
        {
            float mesh = NONE;
            float box = NONE;
            HitNode(ray, (*it)->getRootNode(), &mesh, &box, 0);
            if (mesh < bestMesh)
            {
                bestMesh = mesh;
                byMesh = *it;
            }
            if (box < bestBox)
            {
                bestBox = box;
                byBox = *it;
            }
        }
        // Ни модель, ни ящик не сработали (11.10.2026: корпус дома -
        // InstancedEntity, её ящик в журнале - не в мировых координатах, и
        // луч его не пересекал) - здание, чья ось ближе всех к лучу, если не
        // дальше CENTER_PICK.
        // 40 - щелчок по краю дома не засчитывался (11.10.2026), 90 - ось
        // большого дома ещё в пределах; из нескольких берём ближайшую ось.
        const float CENTER_PICK = 90.0f;
        Building* byCentre = NULL;
        float bestCentre = CENTER_PICK;
        float nearestAxis = NONE;           // для журнала - и при промахе
        if (byMesh == NULL && byBox == NULL)
        {
            for (std::set<Building*>::const_iterator it = candidates.begin(); it != candidates.end(); ++it)
            {
                const Ogre::Vector3 pos = (*it)->getPosition();
                if ((pos - ray.getOrigin()).dotProduct(ray.getDirection()) <= 0.0f)
                    continue;               // позади камеры
                const float off = RayToAxis(ray, pos);
                if (off < nearestAxis)
                    nearestAxis = off;
                if (off < bestCentre)
                {
                    bestCentre = off;
                    byCentre = *it;
                }
            }
        }
        Building* const picked = byMesh != NULL ? byMesh : (byBox != NULL ? byBox : byCentre);
        if (g_debug)
        {
            std::string types;
            for (std::set<std::string>::const_iterator k = g_pickTypes.begin(); k != g_pickTypes.end(); ++k)
                types += (types.empty() ? "" : ",") + *k;
            char note[640];
            sprintf_s(note, "BuildModeGizmo: click pick - %u buildings near the ray, mesh hit %s %.1f, box hit %s %.1f, centre %s %.1f, parts [%s]",
                      static_cast<unsigned>(candidates.size()),
                      byMesh != NULL ? "yes" : "no", byMesh != NULL ? bestMesh : 0.0f,
                      byBox != NULL ? "yes" : "no", byBox != NULL ? bestBox : 0.0f,
                      byCentre != NULL ? "yes" : "no", nearestAxis < NONE ? nearestAxis : 0.0f,
                      types.substr(0, 300).c_str());
            DebugLog(std::string(note) + g_pickBoxNote +
                     (picked != NULL ? " -> '" + picked->getName() + "'" : std::string()));
        }
        return picked;
    }


    Building* SafePick()
    {
        __try
        {
            return PickBuilding();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return NULL;
        }
    }


    bool SafeSelect(LevelEditor* editor, Building* b)
    {
        __try
        {
            editor->setSelectedObject(b);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }


    bool g_clickPrevDown = false;
    POINT g_clickAt;
    bool g_clickOverGui = false;
    bool g_clickGizmo = false;      // за время нажатия стрелочка была в руке
    bool g_clickPickBroken = false;

    void EditorClickPick()
    {
        const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        TransformWindow* const t = TransformWindow::getSingleton();
        if (down && !g_clickPrevDown)
        {
            GetCursorPos(&g_clickAt);
            MyGUI::InputManager* const input = MyGUI::InputManager::getInstancePtr();
            MyGUI::Widget* const focus = input != NULL ? input->getMouseFocusWidget() : NULL;
            g_clickOverGui = focus != NULL;
            g_clickGizmo = false;
            if (g_debug && focus != NULL)
                DebugLog("BuildModeGizmo: click over the interface: '" + focus->getName() + "'");
        }
        if (down && (g_gizmoDragging || (t != NULL && t->isVisible() && t->isActive())))
            g_clickGizmo = true;

        if (!down && g_clickPrevDown && !g_clickOverGui && !g_clickGizmo && !g_clickPickBroken &&
            g_editor != NULL)
        {
            POINT now;
            GetCursorPos(&now);
            if (abs(now.x - g_clickAt.x) + abs(now.y - g_clickAt.y) < 6)
            {
                Building* const b = SafePick();
                if (b != NULL && b != g_editor->selectedObject.getBuilding())
                {
                    if (!SafeSelect(g_editor, b))
                    {
                        g_clickPickBroken = true;
                        ErrorLog("BuildModeGizmo: selecting a building by click failed, click selection is off until restart");
                    }
                }
            }
        }
        g_clickPrevDown = down;
    }


    void TickSite()
    {
        ReportSite();

        if (!g_enabled || g_siteBroken || ou == NULL || ou->player == NULL)
        {
            if (g_siteOpen)
                CloseSite("plugin off");
            return;
        }

        // Редактор включён. В режиме интерьеров стрелочки его - мы не
        // вмешиваемся и окно не закрываем (оно теперь редактора). В обычном
        // режиме строительства - наши, на любом здании (EditorAnyBuilding).
        const bool editorOn = g_editorSeenMs != 0 && GetTickCount() - g_editorSeenMs < 500;
        if (editorOn && (g_editorInterior || !g_editorAnyBuilding))
        {
            g_siteOpen = false;
            g_siteTarget = hand();
            g_siteBuilding = NULL;
            g_inEditor = false;
            return;
        }
        if (!editorOn && !g_siteArrows)
        {
            // Стрелочки на стройплощадке выключены, а в редакторе - нет.
            if (g_siteOpen)
                CloseSite("site arrows off");
            g_inEditor = false;
            return;
        }
        if (editorOn != g_inEditor)
        {
            // Вошли в редактор или вышли: цель меняется (любое здание или
            // только своя площадка) - начинаем заново.
            if (g_siteOpen)
                CloseSite(editorOn ? "developer build mode" : "left the developer menu");
            g_siteDismissed = NULL;
            g_inEditor = editorOn;
            if (g_debug)
                DebugLog(editorOn ? "BuildModeGizmo: arrows on any building (developer build mode)"
                                  : "BuildModeGizmo: arrows on own construction sites");
        }

        // Ведут новую постройку - щелчок ставит здание. В редакторе - по
        // настройке SkipDuringPlacement.
        if (ou->player->isObjectPlacementMode() && (!g_inEditor || g_skipDuringPlacement))
        {
            if (g_siteOpen)
                CloseSite("placing a new building");
            if (g_inEditor && !g_toldAboutPlacement)
            {
                // Один раз и без Debug: если стрелочек в редакторе нет из-за
                // этой проверки, это видно в журнале.
                g_toldAboutPlacement = true;
                DebugLog("BuildModeGizmo: object placement is active, no arrows "
                         "(SkipDuringPlacement=0 to override)");
            }
            return;
        }

        // Щелчок по зданию выделяет его в редакторе (сам редактор в этом
        // режиме не выделяет).
        if (g_inEditor)
            EditorClickPick();
        else
            g_clickPrevDown = false;

        // Окно закрыли (крестиком или Esc) - снова откроется по щелчку по
        // миру: по той же постройке тоже (15.10.2026: после Esc ту же
        // постройку было не выбрать). Щелчок по интерфейсу не в счёт.
        {
            static bool s_down = false;
            const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (down && !s_down && g_siteDismissed != NULL)
            {
                MyGUI::InputManager* const input = MyGUI::InputManager::getInstancePtr();
                if (input == NULL || input->getMouseFocusWidget() == NULL)
                    g_siteDismissed = NULL;
            }
            s_down = down;
        }

        // Выделение: в редакторе - его (если там что-то есть), иначе игрока.
        const hand selection = g_inEditor && g_editor != NULL &&
                               g_editor->selectedObject.getRootObject() != NULL
            ? g_editor->selectedObject : ou->player->selectedObject;
        Building* const selectedBuilding = selection.getBuilding();
        Building* const selectedSite = Target(selectedBuilding);
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
            site = byHand == g_siteBuilding ? Target(byHand) : NULL;
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

        // Узел сдвинулся с прошлого кадра (только пока стрелочка в руке:
        // в покое узел и здание могут расходиться, и это не сдвиг).
        Ogre::SceneNode* const node = site->getRootNode();
        bool nodeMoved = false;
        if (node != NULL)
        {
            const Ogre::Vector3 np = node->getPosition();
            const Ogre::Quaternion nr = node->getOrientation();
            if (g_haveLastNode && (active || leftDown) &&
                (np.squaredDistance(g_lastNodePos) > 0.0001f || 1.0f - fabsf(nr.Dot(g_lastNodeRot)) > 1.0e-6f))
            {
                nodeMoved = true;
            }
            if (g_debug && (active || leftDown) && GetTickCount() - g_lastMoveLogMs > 200)
            {
                g_lastMoveLogMs = GetTickCount();
                char note[360];
                sprintf_s(note, "BuildModeGizmo: drag node %.2f %.2f %.2f | window pos %.2f %.2f %.2f | building %.2f %.2f %.2f | changed=%d active=%d lmb=%d windowNode=%s",
                          np.x, np.y, np.z, g_sitePos.x, g_sitePos.y, g_sitePos.z,
                          site->pos.x, site->pos.y, site->pos.z,
                          t->hasChanged() ? 1 : 0, active ? 1 : 0, leftDown ? 1 : 0,
                          t->node == node ? "same" : (t->node == NULL ? "none" : "other"));
                DebugLog(note);
            }
            g_lastNodePos = np;
            g_lastNodeRot = nr;
            g_haveLastNode = true;
        }

        if (t->hasChanged() || (g_inEditor && nodeMoved))
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

    // Esc закрывает окно стрелочек (15.10.2026: в обычном режиме стройки
    // оно по Esc не закрывалось). Нажатие забираем до игры - иначе Esc
    // открыл бы ещё и меню игры. Закрытое окно - как закрытое крестиком:
    // до выделения другого здания не открывается (TickSite).
    // Флага игры мало: в обычном режиме Esc игра сама забирает раньше
    // (снимает выделение), а без выделения окно стрелочек нарочно не
    // закрывается. Поэтому само нажатие - с клавиатуры, пока окно игры
    // активно (проверено в игре 15.10.2026: по одному флагу не закрывалось).
    bool EscPressedNow()
    {
        static bool s_down = false;
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        const bool down = pid == GetCurrentProcessId() && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
        const bool pressed = down && !s_down;
        s_down = down;
        return pressed;
    }

    void SafeEscCloseSite()
    {
        __try
        {
            const bool pressed = EscPressedNow();
            if (!g_siteOpen)
                return;
            const bool flag = key != NULL && (key->escape || key->escape_msg);
            if (!pressed && !flag)
                return;
            TransformWindow* const t = TransformWindow::getSingleton();
            if (t == NULL || !t->isVisible())
                return;
            if (key != NULL)
            {
                key->escape = false;
                key->escape_msg = false;
            }
            t->close();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // Esc на уровне события клавиши: окно стрелочек открыто - закрываем его,
    // а игре нажатие не отдаём (иначе открывалось меню паузы и снималось
    // выделение; 15.10.2026). Так же Wall-B-Gone гасит Esc своего окна.
    bool g_keyHooked = false;
    void (*g_origKeyDown)(InputHandler*, OIS::KeyCode) = NULL;

    bool SafeEscClosesSite()
    {
        __try
        {
            if (!g_siteOpen)
                return false;
            TransformWindow* const t = TransformWindow::getSingleton();
            if (t == NULL || !t->isVisible())
                return false;
            t->close();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Меню паузы игра открывает по своему флагу Esc, а не по этому событию
    // (проверено: без события Esc меню всё равно открывалось). Флаг гасим
    // сразу и ещё несколько кадров - перед обновлением интерфейса игры.
    int g_swallowEscFrames = 0;

    void ClearEscFlags()
    {
        if (key != NULL)
        {
            key->escape = false;
            key->escape_msg = false;
        }
    }

    void KeyDown_hook(InputHandler* self, OIS::KeyCode code)
    {
        if (code == OIS::KC_ESCAPE && SafeEscClosesSite())
        {
            ClearEscFlags();
            g_swallowEscFrames = 3;
            return;
        }
        g_origKeyDown(self, code);
    }

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        if (g_swallowEscFrames > 0)
        {
            --g_swallowEscFrames;
            ClearEscFlags();
        }
        if (!g_keyHooked)
            SafeEscCloseSite();     // запасной путь, если перехват клавиш не встал
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
    api->toggle(api, "BuildModeGizmo", "EditorAnyBuilding", Tr("Arrows on any building in the developer build mode"), Tr("Developer menu (Shift+F12), normal build mode: select any building - yours or not, built or not - and move it with the arrows right away. In the interior mode the game does this itself."), 1, 0);
    api->toggle(api, "BuildModeGizmo", "SkipDuringPlacement", Tr("Not while placing a new building"), Tr("A click then places the building instead of grabbing an arrow. Developer menu only."), 1, 0);
    api->toggle(api, "BuildModeGizmo", "EditorArrows", Tr("Arrows in the developer build mode"), Tr("The first version: arrows in the normal build mode of the developer menu (Shift+F12). Experimental, off by default."), 0, 0);
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

    g_keyHooked = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&InputHandler::keyDownEvent), KeyDown_hook, &g_origKeyDown);
    if (!g_keyHooked)
        ErrorLog("BuildModeGizmo: could not hook the key event - Esc may also open the game menu");

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
