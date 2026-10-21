// BestMinimap - миникарта в углу экрана.
//
// ЧТО РИСУЕМ. Фон - та же картинка, что на большой карте игры: берём имя
// текстуры у MapScreen::mapImage, значит моды на карту (другая картинка,
// другая раскраска) подхватываются сами, без настройки. Поверх - точки
// персонажей рядом (свои, союзники, нейтралы, враги, животные), метки мода
// Map-markers и цели WantedMap.
//
// КАК РИСУЕМ. Своей текстурой MyGUI, которую заполняем на процессоре: кусок
// карты вокруг центра камеры, с поворотом, маской формы (круг, квадрат,
// прямоугольник), рамкой и точками. Шейдеры и render-to-texture Ogre не
// нужны, поэтому нет зависимости от того, как рендер игры собран (DX11,
// OpenGL), - текстуру MyGUI сам отдаёт в нужный рендер. Перерисовка -
// UpdateFps раз в секунду; пиксели считает отдельный поток (см. «Поток
// рисования»), в потоке игры - только сбор данных, подписи и запись готовой
// картинки в текстуру.
//
// КАРТА В ПАМЯТИ. Картинку карты один раз читаем с диска через Ogre::Image
// (тот же файл, что грузит игра). Если файл не читается - копию с видеокарты
// (Texture::convertToImage). DXT (DDS) раскодируем сами: Ogre сжатое в
// цвета не переводит. Большую картинку уменьшаем до MaxSourceSize по
// большей стороне: 4096 - это 64 МБ, 8192 было бы 256.
//
// КООРДИНАТЫ. Как мировая точка ложится на картинку, спрашиваем у самой игры
// (MapScreen::worldToMapCoords) по трём точкам и строим аффинное
// преобразование мир -> доля картинки (0..1). Так не нужно гадать, что
// лежит в worldBounds, и масштаб большой карты не важен: делим на размер
// mapImage. Метки Map-markers хранятся в тех же долях картинки.
//
// ЦВЕТА. «auto» - цвет, которым игра красит этого персонажа на большой
// карте (MapScreen::getMarkerColor). Иначе - свой цвет на категорию.
// Категорию определяем вопросами самой игры: isPlayerCharacter, isAnimal,
// isEnemy / isAlly относительно нашего персонажа.
//
// УПРАВЛЕНИЕ. Тащить мышью (если не Locked), колесо над картой - масштаб,
// CTRL+колесо - размер. Клавиши из ini / MCM: показать-скрыть, масштаб,
// размер. Всё сохраняется в ini.

#define KLOC_DOMAIN "best_minimap"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <GameTheme.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <IniDefaults.h>
#include <HoldKey.h>
#include <WidgetRef.h>
#include <WantedMapApi.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ITexture.h>

#include <OgreImage.h>
#include <OgrePixelFormat.h>
#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreCamera.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/CameraClass.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/gui/MapScreen.h>
// ManagementScreen.h наследует шаблон от wraps::BaseItemBox, но сам его не
// подключает - подключаем раньше.
#include <mygui/common/itembox/BaseItemBox.h>
#include <kenshi/gui/ManagementScreen.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/MainBarGUI.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>
#include <kenshi/Platoon.h>
#include <kenshi/Town.h>
#include <kenshi/SharedKing.h>
#include <kenshi/Faction.h>
#include <kenshi/FactionRelations.h>
#include <kenshi/util/UtilityT.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Мелочи
    // ---------------------------------------------------------------

    // min/max из Windows.h мешают std::min/max - свои.
    template <class T> T Lo(T a, T b) { return a < b ? a : b; }
    template <class T> T Hi(T a, T b) { return a > b ? a : b; }
    template <class T> T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

    // Цвет в буфере - 0xAARRGGBB: так MyGUI раскладывает R8G8B8A8 в
    // текстуре Ogre (PF_A8R8G8B8). Если на экране красное и синее
    // поменялись местами - SwapRedBlue=1 в ini.
    inline unsigned Pack(int r, int g, int b, int a)
    {
        return (static_cast<unsigned>(a) << 24) | (static_cast<unsigned>(r) << 16) |
               (static_cast<unsigned>(g) << 8) | static_cast<unsigned>(b);
    }
    inline int ChA(unsigned c) { return (c >> 24) & 0xFF; }
    inline int ChR(unsigned c) { return (c >> 16) & 0xFF; }
    inline int ChG(unsigned c) { return (c >> 8) & 0xFF; }
    inline int ChB(unsigned c) { return c & 0xFF; }

    unsigned FromColour(const MyGUI::Colour& c, int alpha = 255)
    {
        return Pack(Clamp(static_cast<int>(c.red * 255.0f + 0.5f), 0, 255),
                    Clamp(static_cast<int>(c.green * 255.0f + 0.5f), 0, 255),
                    Clamp(static_cast<int>(c.blue * 255.0f + 0.5f), 0, 255), alpha);
    }

    bool ParseHex(const char* text, unsigned& out)
    {
        while (*text == ' ' || *text == '#')
            ++text;
        unsigned rgb = 0;
        if (strlen(text) < 6 || sscanf_s(text, "%6x", &rgb) != 1)
            return false;
        out = 0xFF000000u | (rgb & 0xFFFFFFu);
        return true;
    }

    std::string ModulePath(const char* ext)
    {
        char path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();
        std::string file(path, length);
        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string() : file.substr(0, dot) + ext;
    }

    // Путь к ini игрока - см. shared/IniDefaults.h.
    std::string IniPath() { return IniDefaults::Prepare(ModulePath(".ini")); }

    const char* const kSec = "BestMinimap";


    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    enum Shape { SHAPE_CIRCLE, SHAPE_SQUARE, SHAPE_RECT };
    enum Frame { FRAME_NONE, FRAME_LINE, FRAME_GAME };
    enum Category { CAT_SQUAD, CAT_ALLY, CAT_NEUTRAL, CAT_ENEMY, CAT_ANIMAL, CAT_COUNT };

    const char* const CAT_KEY[CAT_COUNT] = { "Squad", "Allies", "Neutrals", "Enemies", "Animals" };
    const char* const CAT_DEFAULT[CAT_COUNT] = { "auto", "auto", "auto", "auto", "#C8B48C" };

    bool g_enabled = true;
    bool g_visible = true;              // переключается клавишей, хранится в ini
    Shape g_shape = SHAPE_CIRCLE;
    int g_size = 220;                   // круг и квадрат: сторона; прямоугольник: ширина
    int g_left = -1, g_top = -1;        // -1 - правый верхний угол
    bool g_placed = false;              // место выбрано (см. ClampPosition)
    bool g_locked = false;
    float g_range = 2000.0f;            // метров от центра до края
    bool g_rotate = false;
    bool g_centerOnSelected = false;
    int g_opacity = 10;
    int g_fps = 20;
    int g_dot = 5;
    Frame g_frame = FRAME_LINE;
    unsigned g_frameColour = 0xFFAFA68Bu;
    bool g_frameAuto = true;
    unsigned g_backColour = 0xFF1E1C19u;
    bool g_show[CAT_COUNT] = { true, true, true, true, true };
    bool g_colourAuto[CAT_COUNT] = { true, true, true, true, false };
    unsigned g_colour[CAT_COUNT] = { 0xFF4FB4FFu, 0xFF5AD25Au, 0xFFD2D2D2u, 0xFFE63C32u, 0xFFC8B48Cu };
    bool g_showDead = false;
    bool g_showView = true;
    bool g_mapMarkers = true;
    bool g_wanted = true;
    bool g_pinToEdge = true;
    bool g_alert = true;                // оповещение: нашего персонажа атакуют
    unsigned g_alertColour = 0xFFFF3B30u;
    int g_alertSeconds = 8;             // сколько держится метка после последней атаки
    bool g_alertMessage = true;         // ещё и сообщение игры
    bool g_alertFlash = true;           // рамка мигает цветом тревоги
    bool g_camRange = true;             // круг: докуда долетает камера
    int g_camRangeSet = 0;              // метров; 0 - узнаём сами (см. LearnCameraRange)
    float g_camRangeLearned = 0.0f;
    unsigned g_camRangeColour = 0xFFE8E2D0u;
    unsigned g_viewColour = 0xFFF0EAD8u;    // стрелка камеры в центре
    bool g_camRangeDots = true;         // пунктиром (dots) или сплошной (line)
    bool g_showFacing = true;           // «носик» у точки: куда смотрит персонаж
    bool g_facingFlip = false;          // если носики смотрят назад
    int g_squadLabels = 1;              // подписи отрядов: 0 нет, 1 чужие, 2 все
    unsigned g_squadLabelColour = 0xFFE6C84Au;
    bool g_showTowns = true;            // поселения и постройки на карте
    bool g_townNames = true;
    bool g_townRadius = true;           // круг радиуса поселения
    bool g_townGameIcons = true;        // значки как на большой карте игры
    int g_townIconSize = 22;            // их высота, пикселей
    int g_townFontSize = 0;             // высота названий поселений, пикс. (0 - как прочие подписи)
    unsigned g_townColour = 0xFFF0E6C8u;
    bool g_rightClickMove = true;       // ПКМ по миникарте - идти туда
    bool g_dblClickCamera = true;       // двойной щелчок по миникарте - камера туда
    bool g_showBounty = true;           // кольцо и сумма награды за голову
    unsigned g_bountyColour = 0xFFF2C230u;
    bool g_tooltips = true;             // подсказка при наведении мыши
    bool g_charNames = false;           // имена персонажей рядом с точками
    int g_townNamesRange = 8000;        // имена поселений - при обзоре не больше, м
    bool g_townAutoColour = true;       // цвет поселения по отношениям фракций
    int g_maxSource = 4096;
    bool g_swapRB = false;
    bool g_debug = false;

    HoldKey::Key g_keyToggle, g_keyZoomIn, g_keyZoomOut, g_keyBigger, g_keySmaller;

    const float kRangeMin = 30.0f;
    const float kRangeMax = 150000.0f;     // половина мира (worldBounds +-147 км) - вся карта
    const int kSizeMin = 80;
    const int kSizeMax = 900;
    const float kZoomStep = 1.25f;
    const int kSizeStep = 20;

    bool ReadBool(const std::string& ini, const char* key, bool def)
    {
        return GetPrivateProfileIntA(kSec, key, def ? 1 : 0, ini.c_str()) != 0;
    }

    int ReadInt(const std::string& ini, const char* key, int def, int lo, int hi)
    {
        return Clamp(static_cast<int>(GetPrivateProfileIntA(kSec, key, def, ini.c_str())), lo, hi);
    }

    std::string ReadText(const std::string& ini, const char* key, const char* def)
    {
        char text[128] = {};
        GetPrivateProfileStringA(kSec, key, def, text, sizeof(text), ini.c_str());
        return text;
    }

    // Цвет «auto» или #RRGGBB. Плохое значение - как «auto».
    void ReadColour(const std::string& ini, const char* key, const char* def, bool& isAuto, unsigned& colour)
    {
        const std::string t = ReadText(ini, key, def);
        unsigned c = 0;
        if (t == "auto" || !ParseHex(t.c_str(), c))
        {
            isAuto = true;
            unsigned fallback = 0;
            if (ParseHex(def, fallback))
                colour = fallback;
            return;
        }
        isAuto = false;
        colour = c;
    }

    // Запись в ini - не сразу, а через полторы секунды тишины: каждая
    // WritePrivateProfileString - это запись файла на диск в потоке игры, а
    // зум и размер с клавиш при удержании повторяются 8 раз в секунду.
    std::map<std::string, int> g_pendingWrites;
    DWORD g_pendingAt = 0;

    void WriteInt(const char* key, int value)
    {
        g_pendingWrites[key] = value;
        g_pendingAt = GetTickCount();
    }

    void FlushWrites()
    {
        if (g_pendingWrites.empty())
            return;
        const std::string ini = IniPath();
        if (!ini.empty())
            for (std::map<std::string, int>::const_iterator it = g_pendingWrites.begin(); it != g_pendingWrites.end(); ++it)
            {
                char text[32];
                sprintf_s(text, "%d", it->second);
                WritePrivateProfileStringA(kSec, it->first.c_str(), text, ini.c_str());
            }
        g_pendingWrites.clear();
    }

    void FlushWritesIfQuiet(DWORD now)
    {
        if (!g_pendingWrites.empty() && now - g_pendingAt >= 1500)
            FlushWrites();
    }

    void Invalidate();

    // Зовётся при запуске и из MCM после изменения настройки.
    void __cdecl LoadSettings()
    {
        // Отложенное - сначала на диск, иначе перечитали бы старое.
        FlushWrites();
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        g_enabled = ReadBool(ini, "Enabled", true);
        g_visible = ReadBool(ini, "Visible", true);

        const std::string shape = ReadText(ini, "Shape", "circle");
        g_shape = shape == "square" ? SHAPE_SQUARE : (shape == "rect" ? SHAPE_RECT : SHAPE_CIRCLE);
        g_size = ReadInt(ini, "Size", 220, kSizeMin, kSizeMax);
        g_left = ReadInt(ini, "Left", -1, -1, 10000);
        g_top = ReadInt(ini, "Top", -1, -1, 10000);
        g_placed = g_left >= 0 && g_top >= 0;
        g_locked = ReadBool(ini, "Locked", false);
        g_range = static_cast<float>(ReadInt(ini, "Range", 2000, static_cast<int>(kRangeMin), static_cast<int>(kRangeMax)));
        g_rotate = ReadBool(ini, "RotateWithCamera", false);
        g_centerOnSelected = ReadBool(ini, "CenterOnSelected", false);
        g_opacity = ReadInt(ini, "Opacity", 10, 1, 10);
        g_fps = ReadInt(ini, "UpdateFps", 20, 2, 60);
        g_dot = ReadInt(ini, "DotSize", 5, 2, 16);

        const std::string frame = ReadText(ini, "Frame", "line");
        g_frame = frame == "none" ? FRAME_NONE : (frame == "game" ? FRAME_GAME : FRAME_LINE);
        ReadColour(ini, "FrameColour", "auto", g_frameAuto, g_frameColour);
        bool backAuto = false;
        ReadColour(ini, "BackgroundColour", "#1E1C19", backAuto, g_backColour);
        if (backAuto)
            g_backColour = 0xFF1E1C19u;

        for (int i = 0; i < CAT_COUNT; ++i)
        {
            g_show[i] = ReadBool(ini, (std::string("Show") + CAT_KEY[i]).c_str(), true);
            ReadColour(ini, (std::string("Colour") + CAT_KEY[i]).c_str(), CAT_DEFAULT[i],
                       g_colourAuto[i], g_colour[i]);
        }
        g_showDead = ReadBool(ini, "ShowDead", false);
        g_showView = ReadBool(ini, "ShowView", true);
        {
            bool unused = false;
            ReadColour(ini, "ViewColour", "#F0EAD8", unused, g_viewColour);
        }
        g_mapMarkers = ReadBool(ini, "MapMarkers", true);
        g_wanted = ReadBool(ini, "WantedMarkers", true);
        g_pinToEdge = ReadBool(ini, "PinToEdge", true);

        bool unused = false;
        g_alert = ReadBool(ini, "AttackAlert", true);
        ReadColour(ini, "AlertColour", "#FF3B30", unused, g_alertColour);
        g_alertSeconds = ReadInt(ini, "AlertSeconds", 8, 2, 120);
        g_alertMessage = ReadBool(ini, "AlertMessage", true);
        g_alertFlash = ReadBool(ini, "AlertFlashFrame", true);
        g_camRange = ReadBool(ini, "ShowCameraRange", true);
        g_camRangeSet = ReadInt(ini, "CameraRange", 0, 0, 50000);
        g_camRangeLearned = static_cast<float>(ReadInt(ini, "CameraRangeLearned", 0, 0, 50000));
        ReadColour(ini, "CameraRangeColour", "#E8E2D0", unused, g_camRangeColour);
        g_camRangeDots = ReadText(ini, "CameraRangeStyle", "dots") != "line";
        g_showFacing = ReadBool(ini, "ShowFacing", true);
        g_facingFlip = ReadBool(ini, "FacingFlip", false);
        const std::string labels = ReadText(ini, "SquadLabels", "others");
        g_squadLabels = labels == "none" ? 0 : (labels == "all" ? 2 : 1);
        ReadColour(ini, "SquadLabelColour", "#E6C84A", unused, g_squadLabelColour);
        g_showTowns = ReadBool(ini, "ShowTowns", true);
        g_townNames = ReadBool(ini, "TownNames", true);
        g_townRadius = ReadBool(ini, "TownRadius", true);
        g_townGameIcons = ReadText(ini, "TownIcons", "game") != "diamond";
        g_townIconSize = ReadInt(ini, "TownIconSize", 22, 10, 64);
        g_townFontSize = ReadInt(ini, "TownNameSize", 0, 0, 40);
        g_rightClickMove = ReadBool(ini, "RightClickMove", true);
        g_dblClickCamera = ReadBool(ini, "DoubleClickCamera", true);
        g_showBounty = ReadBool(ini, "ShowBounty", true);
        ReadColour(ini, "BountyColour", "#F2C230", unused, g_bountyColour);
        g_tooltips = ReadBool(ini, "Tooltips", true);
        g_charNames = ReadBool(ini, "CharacterNames", false);
        g_townNamesRange = ReadInt(ini, "TownNamesRange", 8000, 0, 150000);
        ReadColour(ini, "TownColour", "auto", g_townAutoColour, g_townColour);
        if (g_townAutoColour)
            g_townColour = 0xFFF0E6C8u;

        g_keyToggle = HoldKey::Parse(ReadText(ini, "KeyToggle", "CTRL+M"));
        g_keyZoomIn = HoldKey::Parse(ReadText(ini, "KeyZoomIn", "="));
        g_keyZoomOut = HoldKey::Parse(ReadText(ini, "KeyZoomOut", "-"));
        g_keyBigger = HoldKey::Parse(ReadText(ini, "KeyBigger", "CTRL+="));
        g_keySmaller = HoldKey::Parse(ReadText(ini, "KeySmaller", "CTRL+-"));

        g_maxSource = ReadInt(ini, "MaxSourceSize", 4096, 512, 8192);
        g_swapRB = ReadBool(ini, "SwapRedBlue", false);
        g_debug = ReadBool(ini, "Debug", false);
        Invalidate();
    }


    // ---------------------------------------------------------------
    // Картинка карты в памяти
    // ---------------------------------------------------------------

    struct Source
    {
        std::string texture;    // имя текстуры, с которой читали
        int w, h;               // размер в памяти (уже уменьшенный)
        std::vector<unsigned> px;
        // Уменьшенные вдвое копии (мип-уровни): при сильном отдалении на
        // пиксель миникарты приходятся десятки пикселей картинки, и выборка
        // из полной рябила бы. Берём уровень под масштаб.
        struct Level { int w, h; std::vector<unsigned> px; };
        std::vector<Level> mips;
        Source() : w(0), h(0) {}
        bool ok() const { return w > 0 && h > 0; }
    };

    Source g_src;
    std::string g_srcFailed;    // имя, которое прочитать не вышло: не пробовать каждый кадр

    void Rgb565(unsigned c, int& r, int& g, int& b)
    {
        r = ((c >> 11) & 31) * 255 / 31;
        g = ((c >> 5) & 63) * 255 / 63;
        b = (c & 31) * 255 / 31;
    }

    // Раскодировать один блок 4x4 DXT1/DXT3/DXT5 в out[16] (0xAARRGGBB).
    void DecodeBlock(const unsigned char* p, Ogre::PixelFormat f, unsigned out[16])
    {
        int alpha[16];
        for (int i = 0; i < 16; ++i)
            alpha[i] = 255;
        const bool dxt1 = f == Ogre::PF_DXT1;
        if (f == Ogre::PF_DXT2 || f == Ogre::PF_DXT3)
        {
            for (int i = 0; i < 16; ++i)
                alpha[i] = ((p[i / 2] >> ((i & 1) * 4)) & 15) * 17;
            p += 8;
        }
        else if (f == Ogre::PF_DXT4 || f == Ogre::PF_DXT5)
        {
            int a[8];
            a[0] = p[0];
            a[1] = p[1];
            if (a[0] > a[1])
                for (int i = 1; i < 7; ++i)
                    a[i + 1] = ((7 - i) * a[0] + i * a[1]) / 7;
            else
            {
                for (int i = 1; i < 5; ++i)
                    a[i + 1] = ((5 - i) * a[0] + i * a[1]) / 5;
                a[6] = 0;
                a[7] = 255;
            }
            unsigned long long bits = 0;
            for (int i = 0; i < 6; ++i)
                bits |= static_cast<unsigned long long>(p[2 + i]) << (8 * i);
            for (int i = 0; i < 16; ++i)
                alpha[i] = a[(bits >> (3 * i)) & 7];
            p += 8;
        }
        const unsigned c0 = p[0] | (p[1] << 8), c1 = p[2] | (p[3] << 8);
        int r[4], g[4], b[4], a4[4] = { 255, 255, 255, 255 };
        Rgb565(c0, r[0], g[0], b[0]);
        Rgb565(c1, r[1], g[1], b[1]);
        if (c0 > c1 || !dxt1)
        {
            r[2] = (2 * r[0] + r[1]) / 3; g[2] = (2 * g[0] + g[1]) / 3; b[2] = (2 * b[0] + b[1]) / 3;
            r[3] = (r[0] + 2 * r[1]) / 3; g[3] = (g[0] + 2 * g[1]) / 3; b[3] = (b[0] + 2 * b[1]) / 3;
        }
        else
        {
            r[2] = (r[0] + r[1]) / 2; g[2] = (g[0] + g[1]) / 2; b[2] = (b[0] + b[1]) / 2;
            r[3] = g[3] = b[3] = 0;
            a4[3] = 0;
        }
        const unsigned idx = p[4] | (p[5] << 8) | (p[6] << 16) | (static_cast<unsigned>(p[7]) << 24);
        for (int i = 0; i < 16; ++i)
        {
            const int k = (idx >> (2 * i)) & 3;
            out[i] = Pack(r[k], g[k], b[k], dxt1 ? a4[k] : alpha[i]);
        }
    }

    // Из Ogre::Image - в out, с уменьшением шагом step до maxSize (точечная
    // выборка: для картинки карты хватает, а память на усреднение 8192x8192
    // не нужна). mips - строить ли уменьшенные копии.
    bool FillFromImage(Ogre::Image& img, Source& out, int maxSize, bool mips)
    {
        const int w = static_cast<int>(img.getWidth()), h = static_cast<int>(img.getHeight());
        const Ogre::PixelFormat f = img.getFormat();
        if (w <= 0 || h <= 0 || img.getData() == NULL)
            return false;
        const int step = Hi(1, (Hi(w, h) + maxSize - 1) / maxSize);
        const int ow = (w + step - 1) / step, oh = (h + step - 1) / step;
        std::vector<unsigned> px(static_cast<size_t>(ow) * oh, 0xFF000000u);
        const unsigned char* const data = img.getData();

        if (Ogre::PixelUtil::isCompressed(f))
        {
            const bool dxt = f == Ogre::PF_DXT1 || f == Ogre::PF_DXT2 || f == Ogre::PF_DXT3 ||
                             f == Ogre::PF_DXT4 || f == Ogre::PF_DXT5;
            if (!dxt)
            {
                ErrorLog("BestMinimap: picture is in an unsupported compressed format " +
                         Ogre::PixelUtil::getFormatName(f));
                return false;
            }
            const int blockBytes = f == Ogre::PF_DXT1 ? 8 : 16;
            const int bw = (w + 3) / 4, bh = (h + 3) / 4;
            unsigned block[16];
            for (int by = 0; by < bh; ++by)
            {
                // Строка блоков нужна, если в ней есть хоть одна выбираемая строка пикселей.
                bool rowNeeded = false;
                for (int y = by * 4; y < by * 4 + 4 && y < h; ++y)
                    if (y % step == 0)
                        rowNeeded = true;
                if (!rowNeeded)
                    continue;
                for (int bx = 0; bx < bw; ++bx)
                {
                    bool colNeeded = false;
                    for (int x = bx * 4; x < bx * 4 + 4 && x < w; ++x)
                        if (x % step == 0)
                            colNeeded = true;
                    if (!colNeeded)
                        continue;
                    DecodeBlock(data + (static_cast<size_t>(by) * bw + bx) * blockBytes, f, block);
                    for (int j = 0; j < 4; ++j)
                    {
                        const int y = by * 4 + j;
                        if (y >= h || y % step != 0)
                            continue;
                        for (int i = 0; i < 4; ++i)
                        {
                            const int x = bx * 4 + i;
                            if (x >= w || x % step != 0)
                                continue;
                            px[static_cast<size_t>(y / step) * ow + x / step] = block[j * 4 + i];
                        }
                    }
                }
            }
        }
        else
        {
            const size_t bpp = Ogre::PixelUtil::getNumElemBytes(f);
            if (bpp == 0)
                return false;
            for (int y = 0; y < oh; ++y)
                for (int x = 0; x < ow; ++x)
                {
                    const unsigned char* p = data + (static_cast<size_t>(y) * step * w + static_cast<size_t>(x) * step) * bpp;
                    Ogre::uint8 r = 0, g = 0, b = 0, a = 255;
                    Ogre::PixelUtil::unpackColour(&r, &g, &b, &a, f, p);
                    px[static_cast<size_t>(y) * ow + x] = Pack(r, g, b, a);
                }
        }
        out.px.swap(px);
        // Мип-уровни: среднее 2x2, пока сторона не меньше 8.
        out.mips.clear();
        if (mips)
        {
            int lw = ow, lh = oh;
            const std::vector<unsigned>* prev = &out.px;
            while (lw >= 16 && lh >= 16)
            {
                Source::Level lv;
                lv.w = lw / 2;
                lv.h = lh / 2;
                lv.px.resize(static_cast<size_t>(lv.w) * lv.h);
                for (int y = 0; y < lv.h; ++y)
                    for (int x = 0; x < lv.w; ++x)
                    {
                        const unsigned* const a = &(*prev)[static_cast<size_t>(2 * y) * lw + 2 * x];
                        const unsigned* const b = a + lw;
                        lv.px[static_cast<size_t>(y) * lv.w + x] = Pack(
                            (ChR(a[0]) + ChR(a[1]) + ChR(b[0]) + ChR(b[1]) + 2) / 4,
                            (ChG(a[0]) + ChG(a[1]) + ChG(b[0]) + ChG(b[1]) + 2) / 4,
                            (ChB(a[0]) + ChB(a[1]) + ChB(b[0]) + ChB(b[1]) + 2) / 4, 255);
                    }
                out.mips.push_back(lv);
                prev = &out.mips.back().px;
                lw = lv.w;
                lh = lv.h;
            }
        }
        out.w = ow;
        out.h = oh;
        return true;
    }

    // Текстуру игры - в память: файлом через Ogre, а не вышло - копией с
    // видеокарты.
    bool ReadTexture(const std::string& texture, Source& out, int maxSize, bool mips, std::string& how)
    {
        out = Source();
        if (texture.empty())
            return false;
        bool ok = false;
        try
        {
            Ogre::Image img;
            img.load(texture, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            ok = FillFromImage(img, out, maxSize, mips);
            how = "file";
        }
        catch (...)
        {
            ok = false;
        }
        if (!ok)
        {
            try
            {
                Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName(texture);
                if (!tex.isNull())
                {
                    Ogre::Image img;
                    tex->convertToImage(img);
                    ok = FillFromImage(img, out, maxSize, mips);
                    how = "gpu copy";
                }
            }
            catch (...)
            {
                ok = false;
            }
        }
        if (!ok)
            out = Source();
        else
            out.texture = texture;
        return ok;
    }

    bool LoadSource(const std::string& texture)
    {
        const DWORD started = GetTickCount();
        std::string how;
        const bool ok = ReadTexture(texture, g_src, g_maxSource, true, how);
        if (texture.empty())
            return false;
        if (!ok)
        {
            g_src = Source();
            g_srcFailed = texture;
            ErrorLog("BestMinimap: could not read the map picture '" + texture + "', the minimap shows no terrain");
            return false;
        }
        char note[256];
        sprintf_s(note, "BestMinimap: map picture '%s' read (%s), %dx%d in memory, %u ms",
                  texture.c_str(), how.c_str(), g_src.w, g_src.h, GetTickCount() - started);
        DebugLog(note);
        return true;
    }


    // ---------------------------------------------------------------
    // Мир -> доля картинки карты
    // ---------------------------------------------------------------

    struct Affine
    {
        double au, bu, cu;      // u = au*x + bu*z + cu
        double av, bv, cv;      // v = av*x + bv*z + cv
        bool ok;
        Affine() : au(0), bu(0), cu(0), av(0), bv(0), cv(0), ok(false) {}
        void apply(double x, double z, double& u, double& v) const
        {
            u = au * x + bu * z + cu;
            v = av * x + bv * z + cv;
        }
    };

    Affine g_map;

    MapScreen* GetMapScreen()
    {
        ManagementScreen* const ms = ManagementScreen::getSingleton();
        return ms == NULL ? NULL : ms->mapScreen;
    }

    // Три точки через саму игру. Шаг большой (20 км): worldToMapCoords
    // отдаёт целые пиксели, на малом шаге округление дало бы ошибку в
    // проценты. Мировые координаты за краем мира не мешают - отображение
    // линейное.
    bool Calibrate(MapScreen* map, const Ogre::Vector3& c)
    {
        MyGUI::ImageBox* const img = map->mapImage;
        if (img == NULL)
            return false;
        const double w = img->getWidth(), h = img->getHeight();
        if (w <= 0 || h <= 0)
            return false;
        const double d = 20000.0;
        const MyGUI::IntPoint p0 = map->worldToMapCoords(c);
        const MyGUI::IntPoint px = map->worldToMapCoords(c + Ogre::Vector3(static_cast<float>(d), 0, 0));
        const MyGUI::IntPoint pz = map->worldToMapCoords(c + Ogre::Vector3(0, 0, static_cast<float>(d)));
        Affine a;
        a.au = (px.left - p0.left) / (d * w);
        a.bu = (pz.left - p0.left) / (d * w);
        a.av = (px.top - p0.top) / (d * h);
        a.bv = (pz.top - p0.top) / (d * h);
        if (std::fabs(a.au * a.bv - a.bu * a.av) < 1e-14)
            return false;           // вырожденное - карта ещё не настроена
        a.cu = p0.left / w - a.au * c.x - a.bu * c.z;
        a.cv = p0.top / h - a.av * c.x - a.bv * c.z;
        a.ok = true;
        g_map = a;
        char note[320];
        sprintf_s(note, "BestMinimap: map calibrated, image %dx%d, u=%.3g*x%+.3g*z%+.4f v=%.3g*x%+.3g*z%+.4f",
                  static_cast<int>(w), static_cast<int>(h), a.au, a.bu, a.cu, a.av, a.bv, a.cv);
        DebugLog(note);
        return true;
    }

    bool g_mapApprox = false;       // сейчас примерная калибровка
    DWORD g_lastCalibration = 0;

    // Примерно, пока точная калибровка не вышла: считаем, что worldBounds -
    // это (minX, minZ, maxX, maxZ) мира, а картинка покрывает его целиком,
    // север сверху. Как только игра выставит размер большой карты, это
    // заменит точная калибровка. Неверное предположение видно сразу в
    // журнале: строка «map calibrated» после «approximate» покажет разницу.
    bool ApproximateFromBounds(MapScreen* map)
    {
        const Ogre::Vector4& b = map->worldBounds;
        const double wx = b.z - b.x, wz = b.w - b.y;
        if (wx < 1.0 || wz < 1.0)
            return false;
        Affine a;
        a.au = 1.0 / wx;
        a.cu = -b.x / wx;
        a.bv = 1.0 / wz;
        a.cv = -b.y / wz;
        a.ok = true;
        g_map = a;
        char note[200];
        sprintf_s(note, "BestMinimap: approximate map placement from worldBounds (%.0f %.0f %.0f %.0f)",
                  b.x, b.y, b.z, b.w);
        DebugLog(note);
        return true;
    }


    // ---------------------------------------------------------------
    // Кто на карте
    // ---------------------------------------------------------------

    struct Dot
    {
        Ogre::Vector3 pos;
        unsigned colour;
        bool selected;
        bool dead;
        int cat;
        float fx, fz;           // куда смотрит, мировые x/z (единичный)
        ActivePlatoon* platoon;
        bool leader;            // лидер своего отряда - на нём подпись
        int bounty;             // награда за голову, 0 - нет
        const std::string* name;    // из кэша персонажа (g_catCache) - не копия
    };

    // Категория персонажа считается не каждый кадр: isEnemy/isAlly не
    // бесплатны, а отношения меняются редко. Кэш на секунду.
    struct CatCache { int cat; int bounty; unsigned colour; std::string name; DWORD at; DWORD seen; };
    std::map<Character*, CatCache> g_catCache;
    DWORD g_catCacheSwept = 0;

    Character* ReferenceCharacter()
    {
        PlayerInterface* const p = ou->player;
        if (p->playerCharacters.size() > 0)
            return p->playerCharacters[0];
        return NULL;
    }

    // Обновление размазано по времени (1-1,5 с, по адресу персонажа): иначе
    // раз в секунду все сотни персонажей пересчитывались бы в одном кадре.
    const CatCache& CategoryOf(Character* c, Character* ref, DWORD now)
    {
        std::map<Character*, CatCache>::iterator it = g_catCache.find(c);
        const DWORD life = 1000 + static_cast<DWORD>((reinterpret_cast<size_t>(c) >> 4) % 500);
        if (it != g_catCache.end())
            it->second.seen = now;
        if (it != g_catCache.end() && now - it->second.at < life)
            return it->second;
        int cat = CAT_NEUTRAL;
        if (c->isPlayerCharacter())
            cat = CAT_SQUAD;
        else if (c->isAnimal() != NULL && (ref == NULL || !c->isEnemy(ref, true)))
            cat = CAT_ANIMAL;
        else if (ref != NULL && c->isEnemy(ref, true))
            cat = CAT_ENEMY;
        else if (ref != NULL && c->isAlly(ref, true))
            cat = CAT_ALLY;
        // Награда за голову - вся (getTotalBounty), как «$» над головой.
        // «Как видит наш персонаж» (getPercievedBounty) почти всегда 0:
        // игра отдаёт её, только когда наш персонаж разыскиваемого узнал
        // (20.10.2026).
        // Так же берёт её и Vital-Sense для своего «$».
        int bounty = 0;
        if (g_showBounty && cat != CAT_SQUAD)
            bounty = Hi(0, c->crimes.getTotalBounty());
        CatCache& e = g_catCache[c];
        e.seen = now;
        e.bounty = bounty;
        e.cat = cat;
        // Цвет игры - только для категорий с цветом «auto».
        e.colour = g_colourAuto[cat] ? FromColour(MapScreen::getMarkerColor(c)) : g_colour[cat];
        if (g_tooltips || g_charNames)
            e.name = c->getName();
        else
            e.name.clear();
        e.at = now;
        return e;
    }

    // Выделенные персонажи - указателями. Набор selectedCharacters игры
    // хранит hand, а искать в нём по hand нельзя: хэш для hand в заголовках
    // KenshiLib не объявлен. Выделенных обычно единицы - перебор дешёвый.
    void SelectedCharacters(std::vector<Character*>& out)
    {
        out.clear();
        const ogre_unordered_set<hand>::type& selected = ou->player->selectedCharacters;
        for (ogre_unordered_set<hand>::type::const_iterator it = selected.begin(); it != selected.end(); ++it)
            if (Character* const c = it->getCharacter())
                out.push_back(c);
    }

    bool Contains(const std::vector<Character*>& list, Character* c)
    {
        for (size_t i = 0; i < list.size(); ++i)
            if (list[i] == c)
                return true;
        return false;
    }

    std::vector<Character*> g_selected;

    void CollectDots(std::vector<Dot>& dots, const Ogre::Vector3& center, float reach, DWORD now)
    {
        dots.clear();
        // Персонажи приходят и уходят - не копим: раз в 10 с выбрасываем
        // тех, кого 10 с не было рядом. Раньше кэш раз в 30 с очищался
        // целиком, и следующая перерисовка пересчитывала всех разом.
        if (now - g_catCacheSwept > 10000)
        {
            g_catCacheSwept = now;
            for (std::map<Character*, CatCache>::iterator it = g_catCache.begin(); it != g_catCache.end();)
            {
                if (now - it->second.seen > 10000)
                    g_catCache.erase(it++);
                else
                    ++it;
            }
        }
        // Отряд персонажа - для подписей отрядов и подсказки; не нужны -
        // не спрашиваем.
        const bool wantPlatoon = g_squadLabels != 0 || g_tooltips;
        Character* const ref = ReferenceCharacter();
        SelectedCharacters(g_selected);
        const ogre_unordered_set<Character*>::type& all = ou->getCharacterUpdateList();
        const float reach2 = reach * reach;
        for (ogre_unordered_set<Character*>::type::const_iterator it = all.begin(); it != all.end(); ++it)
        {
            Character* const c = *it;
            if (c == NULL)
                continue;
            const Ogre::Vector3 p = c->getPosition();
            const float dx = p.x - center.x, dz = p.z - center.z;
            if (dx * dx + dz * dz > reach2)
                continue;
            const bool dead = c->isDead();
            if (dead && !g_showDead)
                continue;
            const CatCache& info = CategoryOf(c, ref, now);
            const int cat = info.cat;
            if (!g_show[cat])
                continue;
            Dot d;
            d.pos = p;
            d.dead = dead;
            d.selected = cat == CAT_SQUAD && Contains(g_selected, c);
            d.colour = g_colourAuto[cat] ? info.colour : g_colour[cat];
            d.cat = cat;
            d.bounty = info.bounty;
            d.name = &info.name;
            d.fx = d.fz = 0.0f;
            if (g_showFacing && !dead)
            {
                // Вперёд у персонажа - ось +Z его ориентации (FacingFlip=1 -
                // если на деле наоборот).
                const Ogre::Vector3 f = c->getOrientation() * Ogre::Vector3::UNIT_Z;
                const float fl = std::sqrt(f.x * f.x + f.z * f.z);
                d.fx = fl > 1e-4f ? f.x / fl * (g_facingFlip ? -1.0f : 1.0f) : 0.0f;
                d.fz = fl > 1e-4f ? f.z / fl * (g_facingFlip ? -1.0f : 1.0f) : 0.0f;
            }
            d.platoon = wantPlatoon ? c->getPlatoon() : NULL;
            d.leader = d.platoon != NULL && d.platoon->squadleader == c;
            dots.push_back(d);
        }
        if (g_debug)
        {
            static int lastWanted = -1;
            int wanted = 0;
            for (size_t i = 0; i < dots.size(); ++i)
                wanted += dots[i].bounty > 0 ? 1 : 0;
            if (wanted != lastWanted)
            {
                lastWanted = wanted;
                char note[96];
                sprintf_s(note, "BestMinimap: %d characters with a bounty in range", wanted);
                DebugLog(note);
            }
        }
    }


    // ---------------------------------------------------------------
    // Метки Map-markers (файл Map-markers.json в папке сейва)
    // ---------------------------------------------------------------

    struct Pin
    {
        double u, v;            // доля картинки карты
        bool world;             // true - x/z ниже, а не u/v
        double x, z;
        unsigned colour;
        std::string tip;        // подсказка при наведении
    };

    std::vector<Pin> g_mapPins;
    std::string g_mapPinsFile;
    FILETIME g_mapPinsTime = {};
    DWORD g_mapPinsChecked = 0;

    unsigned MarkerTypeColour(const std::string& type)
    {
        // Цвета - как у самого Map-markers (BuildMarkerColour, не выделенная).
        struct T { const char* name; float r, g, b; };
        static const T table[] = {
            { "danger", 0.88f, 0.18f, 0.18f }, { "stash", 0.18f, 0.42f, 0.95f },
            { "ruin", 0.92f, 0.82f, 0.18f },   { "mine", 0.95f, 0.54f, 0.16f },
            { "base", 0.84f, 0.84f, 0.84f },   { "trader", 0.18f, 0.84f, 0.84f },
            { "safe_spot", 0.52f, 0.90f, 0.40f }, { "quest", 0.60f, 0.30f, 0.88f },
            { "todo", 0.54f, 0.54f, 0.54f },   { "note", 0.18f, 0.78f, 0.20f },
        };
        for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
            if (type == table[i].name)
                return FromColour(MyGUI::Colour(table[i].r, table[i].g, table[i].b));
        return FromColour(MyGUI::Colour(0.18f, 0.78f, 0.20f));
    }

    // Название типа метки Map-markers - для подсказки.
    const char* MarkerTypeName(const std::string& type)
    {
        if (type == "danger") return Tr("Danger");
        if (type == "stash") return Tr("Stash");
        if (type == "ruin") return Tr("Ruin");
        if (type == "mine") return Tr("Mine / Resource");
        if (type == "base") return Tr("Base / Outpost");
        if (type == "trader") return Tr("Trader / Shop");
        if (type == "safe_spot") return Tr("Safe Spot / Bed / Recovery");
        if (type == "quest") return Tr("Quest");
        if (type == "todo") return Tr("Todo");
        return Tr("Note");
    }

    // Разбор без библиотеки JSON: формат пишет сам Map-markers, он простой -
    // массив объектов с полями id, x, y, type, label. Строки - с
    // экранированием \" \\ \n \r \t (как пишет его JsonEscapeString), поэтому
    // скобки и кавычки внутри текста заметки разбору не мешают.

    // Конец строки JSON, начатой кавычкой в from (позиция закрывающей).
    size_t JsonStringEnd(const std::string& s, size_t from)
    {
        for (size_t i = from + 1; i < s.size(); ++i)
        {
            if (s[i] == '\\')
                ++i;
            else if (s[i] == '"')
                return i;
        }
        return std::string::npos;
    }

    // Объект { ... } с позиции open: позиция его закрывающей скобки.
    size_t JsonObjectEnd(const std::string& s, size_t open)
    {
        int depth = 0;
        for (size_t i = open; i < s.size(); ++i)
        {
            if (s[i] == '"')
            {
                i = JsonStringEnd(s, i);
                if (i == std::string::npos)
                    return i;
            }
            else if (s[i] == '{')
                ++depth;
            else if (s[i] == '}' && --depth == 0)
                return i;
        }
        return std::string::npos;
    }

    // Значение поля key в объекте obj: число или строка (без кавычек,
    // раскрытая).
    bool JsonField(const std::string& obj, const char* key, std::string& out, bool& isString)
    {
        const std::string quoted = std::string("\"") + key + "\"";
        size_t at = 0;
        while ((at = obj.find('"', at)) != std::string::npos)
        {
            const size_t end = JsonStringEnd(obj, at);
            if (end == std::string::npos)
                return false;
            const bool match = obj.compare(at, end - at + 1, quoted) == 0;
            size_t p = end + 1;
            while (p < obj.size() && (obj[p] == ' ' || obj[p] == '\t' || obj[p] == '\r' || obj[p] == '\n'))
                ++p;
            if (p >= obj.size() || obj[p] != ':')
            {
                at = end + 1;       // это было значение, а не имя поля
                continue;
            }
            ++p;
            while (p < obj.size() && (obj[p] == ' ' || obj[p] == '\t' || obj[p] == '\r' || obj[p] == '\n'))
                ++p;
            if (p >= obj.size())
                return false;
            if (obj[p] == '"')
            {
                const size_t close = JsonStringEnd(obj, p);
                if (close == std::string::npos)
                    return false;
                if (match)
                {
                    out.clear();
                    for (size_t i = p + 1; i < close; ++i)
                    {
                        if (obj[i] != '\\' || i + 1 >= close)
                        {
                            out += obj[i];
                            continue;
                        }
                        const char e = obj[++i];
                        if (e == 'n')
                            out += '\n';
                        else if (e == 't')
                            out += ' ';
                        else if (e != 'r')
                            out += e;       // \" и \\ - сам знак
                    }
                    isString = true;
                    return true;
                }
                at = close + 1;
                continue;
            }
            size_t q = p;
            while (q < obj.size() && obj[q] != ',' && obj[q] != '}')
                ++q;
            if (match)
            {
                out = obj.substr(p, q - p);
                isString = false;
                return true;
            }
            at = q;
        }
        return false;
    }

    void ParseMapMarkers(const std::string& s)
    {
        g_mapPins.clear();
        size_t at = s.find("\"markers\"");
        if (at == std::string::npos)
            return;
        while (true)
        {
            const size_t open = s.find('{', at);
            if (open == std::string::npos)
                break;
            const size_t close = JsonObjectEnd(s, open);
            if (close == std::string::npos)
                break;
            const std::string obj = s.substr(open, close - open + 1);
            std::string x, y, type, label;
            bool str = false;
            if (JsonField(obj, "x", x, str) && JsonField(obj, "y", y, str))
            {
                Pin p;
                p.world = false;
                p.x = p.z = 0;
                p.u = atof(x.c_str());
                p.v = atof(y.c_str());
                if (!JsonField(obj, "type", type, str) || !str)
                    type = "note";
                p.colour = MarkerTypeColour(type);
                if (!JsonField(obj, "label", label, str) || !str)
                    label.clear();
                p.tip = label.empty() ? std::string(MarkerTypeName(type))
                                      : label + "\n" + MarkerTypeName(type);
                g_mapPins.push_back(p);
            }
            at = close + 1;
        }
    }

    void RefreshMapMarkers(DWORD now)
    {
        if (!g_mapMarkers || now - g_mapPinsChecked < 2000)
            return;
        g_mapPinsChecked = now;
        // Файл меток остаётся в сейве и без мода: мода нет - и меток нет.
        if (GetModuleHandleA("Map-markers.dll") == NULL)
        {
            g_mapPins.clear();
            g_mapPinsFile.clear();
            return;
        }
        SaveFileSystem* const fs = SaveFileSystem::getSingleton();
        const std::string dir = fs == NULL ? std::string() : fs->getActiveSave();
        if (dir.empty())
        {
            g_mapPins.clear();
            return;
        }
        const std::string file = dir + ((dir[dir.size() - 1] == '\\' || dir[dir.size() - 1] == '/') ? "" : "\\") +
                                 "Map-markers.json";
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (!GetFileAttributesExA(file.c_str(), GetFileExInfoStandard, &info))
        {
            g_mapPins.clear();
            g_mapPinsFile.clear();
            return;
        }
        if (file == g_mapPinsFile && CompareFileTime(&info.ftLastWriteTime, &g_mapPinsTime) == 0)
            return;
        g_mapPinsFile = file;
        g_mapPinsTime = info.ftLastWriteTime;
        std::ifstream in(file.c_str(), std::ios::binary);
        std::stringstream text;
        text << in.rdbuf();
        ParseMapMarkers(text.str());
        if (g_debug)
        {
            char note[96];
            sprintf_s(note, "BestMinimap: %u Map-markers markers read", static_cast<unsigned>(g_mapPins.size()));
            DebugLog(note);
        }
    }


    // ---------------------------------------------------------------
    // Цели WantedMap
    // ---------------------------------------------------------------

    std::vector<Pin> g_wantedPins;
    DWORD g_wantedChecked = 0;

    void RefreshWanted(DWORD now)
    {
        if (!g_wanted || now - g_wantedChecked < 500)
            return;
        g_wantedChecked = now;
        g_wantedPins.clear();
        // Ищем каждый раз заново: порядок загрузки плагинов не важен, а
        // GetModuleHandle дешёвый.
        HMODULE const m = GetModuleHandleA("WantedMap.dll");
        WantedMap_GetMarkersFn const get = m == NULL ? NULL :
            reinterpret_cast<WantedMap_GetMarkersFn>(GetProcAddress(m, "WantedMap_GetMarkers"));
        if (get == NULL)
            return;
        WantedMap_Marker buf[64];
        const int n = get(buf, 64);
        for (int i = 0; i < n; ++i)
        {
            Pin p;
            p.world = true;
            p.x = buf[i].x;
            p.z = buf[i].z;
            p.u = p.v = 0;
            p.colour = FromColour(MyGUI::Colour(buf[i].r, buf[i].g, buf[i].b));
            buf[i].label[sizeof(buf[i].label) - 1] = 0;
            p.tip = buf[i].label;
            g_wantedPins.push_back(p);
        }
    }


    // ---------------------------------------------------------------
    // Тревога: нашего персонажа атакуют
    // ---------------------------------------------------------------

    // Атакуют - если у персонажа есть нападающие (getAllAttackersCount, как
    // в Formations). Опрос 4 раза в секунду по своим персонажам, без хуков:
    // своих - десятки, а хук на удар пришлось бы ставить на ближний бой,
    // стрельбу и прочее по отдельности. Метка держится AlertSeconds после
    // последней атаки, чтобы не мигала между ударами.
    //
    // Указатель на персонажа - только ключ: разыменовываем лишь тех, кто
    // сейчас в списке игры; позиция запоминается, и ушедший из загрузки
    // персонаж остаётся меткой там, где его атаковали.
    struct Alert
    {
        Ogre::Vector3 pos;
        DWORD last;
    };

    bool g_hidden = true;               // миникарта сейчас скрыта (см. Show)

    std::map<Character*, Alert> g_alerts;
    DWORD g_alertScan = 0;
    DWORD g_lastMessage = 0;
    std::vector<std::string> g_alertNames;  // ещё не объявленные в сообщении

    void ScanAttacks(DWORD now)
    {
        if (!g_alert)
        {
            g_alerts.clear();
            g_alertNames.clear();
            return;
        }
        // Миникарта скрыта, а сообщения выключены - тревогу никто не увидит.
        if (!g_alertMessage && g_hidden)
            return;
        if (now - g_alertScan < 250)
            return;
        g_alertScan = now;
        const DWORD hold = static_cast<DWORD>(g_alertSeconds) * 1000;
        const lektor<Character*>& mine = ou->player->playerCharacters;
        for (uint32_t i = 0; i < mine.size(); ++i)
        {
            Character* const c = mine[i];
            if (c == NULL || c->isDead() || c->getAllAttackersCount() <= 0)
                continue;
            std::map<Character*, Alert>::iterator it = g_alerts.find(c);
            if (it == g_alerts.end() || now - it->second.last > hold)
                g_alertNames.push_back(c->getName());   // новая тревога
            Alert& a = g_alerts[c];
            a.pos = c->getPosition();
            a.last = now;
        }
        for (std::map<Character*, Alert>::iterator it = g_alerts.begin(); it != g_alerts.end();)
        {
            if (now - it->second.last > hold)
                g_alerts.erase(it++);
            else
                ++it;
        }
        // Сообщение игры - не чаще раза в 3 с, всех новых разом: в большой
        // драке иначе выйдет по строке на каждого.
        if (!g_alertNames.empty() && now - g_lastMessage >= 3000)
        {
            if (g_alertMessage)
            {
                std::string text = Tr("Under attack: ");
                for (size_t i = 0; i < g_alertNames.size() && i < 4; ++i)
                    text += (i > 0 ? ", " : "") + g_alertNames[i];
                if (g_alertNames.size() > 4)
                    text += ", ...";
                ou->showPlayerAMessage(text, false);
            }
            g_alertNames.clear();
            g_lastMessage = now;
        }
    }


    // ---------------------------------------------------------------
    // Докуда долетает камера
    // ---------------------------------------------------------------

    // Игра держит камеру рядом со своими персонажами (CameraClass::
    // restrictPosition), но сам предел в заголовках KenshiLib не виден.
    // Поэтому CameraRange=0 - узнаём его сами: самое большое расстояние от
    // центра камеры до ближайшего своего персонажа, какое удалось увидеть.
    // Отвели камеру до упора - предел записан (CameraRangeLearned в ini).
    // В свободной камере не учимся: там предела нет.
    DWORD g_camLearnSaved = 0;
    float g_camLearnSavedValue = 0.0f;

    float CameraRange()
    {
        return g_camRangeSet > 0 ? static_cast<float>(g_camRangeSet) : g_camRangeLearned;
    }

    DWORD g_camLearnAt = 0;

    void LearnCameraRange(DWORD now)
    {
        if (g_camRangeSet > 0 || !g_camRange || now - g_camLearnAt < 250)
            return;
        g_camLearnAt = now;
        CameraClass* const cam = ou->player->camera;
        if (cam == NULL || cam->isFreeCameraMode())
            return;
        const Ogre::Vector3 c = ou->getCameraCenter();
        const lektor<Character*>& mine = ou->player->playerCharacters;
        float best = -1.0f;
        for (uint32_t i = 0; i < mine.size(); ++i)
        {
            Character* const ch = mine[i];
            if (ch == NULL || ch->isDead())
                continue;
            const Ogre::Vector3 p = ch->getPosition();
            const float d = std::sqrt((p.x - c.x) * (p.x - c.x) + (p.z - c.z) * (p.z - c.z));
            if (best < 0.0f || d < best)
                best = d;
        }
        if (best > g_camRangeLearned && best < 50000.0f)
            g_camRangeLearned = best;
        // В ini - когда выросло хотя бы на 10 м и не чаще раза в 5 с.
        if (g_camRangeLearned >= g_camLearnSavedValue + 10.0f && now - g_camLearnSaved >= 5000)
        {
            g_camLearnSaved = now;
            g_camLearnSavedValue = g_camRangeLearned;
            WriteInt("CameraRangeLearned", static_cast<int>(g_camRangeLearned + 0.5f));
        }
    }


    // ---------------------------------------------------------------
    // Виджеты и текстура
    // ---------------------------------------------------------------

    WidgetRef g_rootRef;            // панель: ловит мышь
    WidgetRef g_backRef;            // подложка-скин игры (Frame=game)
    WidgetRef g_imageRef;           // картинка с нашей текстурой
    MyGUI::ITexture* g_tex = NULL;
    std::string g_texName;
    int g_texW = 0, g_texH = 0;     // размер текстуры (степень двойки от 256)
    int g_viewW = 0, g_viewH = 0;   // видимая часть = размер карты на экране
    int g_texSerial = 0;
    // Рисует отдельный поток (см. «Поток рисования»): буфер, маска и их
    // размер - его; основной поток трогает их, только пока поток свободен.
    std::vector<unsigned> g_buf;
    std::vector<unsigned char> g_mask;  // 0..255: внутри формы
    int g_rw = 0, g_rh = 0;             // размер, под который рисует поток
    int g_rshape = 0;                   // форма, под которую построена маска
    int g_frameW = 0;                   // ширина, под которую построена маска
    bool g_dirtyMask = true;
    DWORD g_lastRender = 0;
    DWORD g_lastWorldFrame = 0;

    int ViewWidth() { return g_size; }
    // Прямоугольник - всегда 16:10, как экран: ширина - Size, высота из неё.
    int ViewHeight() { return g_shape == SHAPE_RECT ? (g_size * 10 + 8) / 16 : g_size; }
    int GamePad() { return g_frame == FRAME_GAME && g_shape != SHAPE_CIRCLE ? 8 : 0; }

    std::vector<Dot> g_dots;
    DWORD g_townsAt = 0;

    void Invalidate()
    {
        g_dirtyMask = true;
        g_lastRender = 0;
        // Цвета, имена, награды в кэше персонажей и поселения - по старым
        // настройкам: пересобрать сразу.
        g_dots.clear();
        g_catCache.clear();
        g_townsAt = 0;
    }

    // Видна только часть текстуры размером с миникарту (setImageInfo), рисуем
    // тоже только в неё. Ширина - степень двойки от 256: строка - не меньше
    // 1 КБ, и шаг строки у видеокарты совпадает с нашим (MyGUI отдаёт
    // указатель без шага строки).
    //
    // Текстура - степень двойки не меньше миникарты (от 256): чем меньше,
    // тем дешевле её запись каждую перерисовку. Растёт при увеличении
    // миникарты; прежняя удаляется кадром позже, когда картинка уже
    // переключена на новую.
    MyGUI::ITexture* g_oldTex = NULL;

    int TexSide(int n)
    {
        int side = 256;
        while (side < n)
            side <<= 1;
        return side;
    }

    bool EnsureTexture(int w, int h)
    {
        if (g_oldTex != NULL)
        {
            MyGUI::RenderManager::getInstance().destroyTexture(g_oldTex);
            g_oldTex = NULL;
        }
        if (g_tex != NULL && g_texW >= w && g_texH >= h)
            return true;
        const int tw = TexSide(Hi(w, g_texW)), th = TexSide(Hi(h, g_texH));
        MyGUI::ITexture* const was = g_tex;
        char name[64];
        sprintf_s(name, "BestMinimap_View_%d", ++g_texSerial);
        try
        {
            MyGUI::RenderManager& rm = MyGUI::RenderManager::getInstance();
            g_tex = rm.createTexture(name);
            g_tex->createManual(tw, th, MyGUI::TextureUsage::Dynamic | MyGUI::TextureUsage::Write,
                                MyGUI::PixelFormat::R8G8B8A8);
        }
        catch (...)
        {
            g_tex = NULL;
        }
        if (g_tex == NULL)
        {
            g_tex = was;
            ErrorLog("BestMinimap: could not create the minimap texture");
            return was != NULL && g_texW >= w && g_texH >= h;
        }
        g_oldTex = was;
        g_texName = name;
        g_texW = tw;
        g_texH = th;
        return true;
    }

    // Место ещё не выбрано (Left/Top = -1 в ini) - правый верхний угол.
    // Отдельным флагом, а не «g_left < 0»: при перетаскивании за край экрана
    // координата становится отрицательной, и раньше это принималось за «места
    // нет» - миникарта отскакивала из угла на место по умолчанию (20.10.2026).

    void ClampPosition(int w, int h)
    {
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        if (!g_placed)
        {
            g_left = view.width - w - 24;
            g_top = 24;
        }
        g_left = Clamp(g_left, 0, Hi(0, view.width - w));
        g_top = Clamp(g_top, 0, Hi(0, view.height - h));
    }

    bool g_dragging = false;
    MyGUI::IntPoint g_dragMouse, g_dragOrigin;

    void MoveSelectedTo();          // ниже, после Render: нужен последний вид
    void CameraTo();

    void OnPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button == MyGUI::MouseButton::Right)
        {
            MoveSelectedTo();
            return;
        }
        if (button != MyGUI::MouseButton::Left || g_locked)
            return;
        g_dragging = true;
        g_dragMouse = MyGUI::InputManager::getInstance().getMousePosition();
        g_dragOrigin = MyGUI::IntPoint(g_left, g_top);
    }

    // Двойной щелчок ЛКМ - камера туда. Сначала была СКМ, но её игра
    // забирает себе (поворот камеры), до миникарты она не доходит
    // (21.10.2026, проверено пользователем).
    void OnDoubleClick(MyGUI::Widget*)
    {
        CameraTo();
    }

    void OnDragged(MyGUI::Widget*, int, int, MyGUI::MouseButton)
    {
        MyGUI::Widget* const root = g_rootRef.get();
        if (!g_dragging || root == NULL)
            return;
        const MyGUI::IntPoint mouse = MyGUI::InputManager::getInstance().getMousePosition();
        g_placed = true;
        g_left = g_dragOrigin.left + (mouse.left - g_dragMouse.left);
        g_top = g_dragOrigin.top + (mouse.top - g_dragMouse.top);
        ClampPosition(root->getWidth(), root->getHeight());
        root->setPosition(g_left, g_top);
    }

    void OnReleased(MyGUI::Widget*, int, int, MyGUI::MouseButton)
    {
        if (!g_dragging)
            return;
        g_dragging = false;
        WriteInt("Left", g_left);
        WriteInt("Top", g_top);
    }

    void Zoom(float factor)
    {
        g_range = Clamp(g_range * factor, kRangeMin, kRangeMax);
        WriteInt("Range", static_cast<int>(g_range + 0.5f));
        g_lastRender = 0;
    }

    void Resize(int delta)
    {
        g_size = Clamp(g_size + delta, kSizeMin, kSizeMax);
        WriteInt("Size", g_size);
        Invalidate();
    }

    void OnWheel(MyGUI::Widget*, int rel)
    {
        const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        if (ctrl)
            Resize(rel > 0 ? kSizeStep : -kSizeStep);
        else
            Zoom(rel > 0 ? 1.0f / kZoomStep : kZoomStep);
    }

    bool EnsureWidgets()
    {
        const int w = ViewWidth(), h = ViewHeight(), pad = GamePad();
        MyGUI::Widget* root = g_rootRef.get();
        const bool created = root == NULL;
        if (root == NULL)
        {
            ClampPosition(w + 2 * pad, h + 2 * pad);
            root = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "PanelEmpty", MyGUI::IntCoord(g_left, g_top, w + 2 * pad, h + 2 * pad),
                MyGUI::Align::Default, "Window", "BestMinimap_Root");
            g_rootRef.set(root);
            root->eventMouseButtonPressed += MyGUI::newDelegate(&OnPressed);
            root->eventMouseDrag += MyGUI::newDelegate(&OnDragged);
            root->eventMouseButtonReleased += MyGUI::newDelegate(&OnReleased);
            root->eventMouseWheel += MyGUI::newDelegate(&OnWheel);
            root->eventMouseButtonDoubleClick += MyGUI::newDelegate(&OnDoubleClick);
            root->setNeedMouseFocus(true);
            g_backRef.reset();
            g_imageRef.reset();
            g_dirtyMask = true;
        }

        // Подложка-скин игры: при FRAME_GAME у квадрата и прямоугольника.
        MyGUI::Widget* back = g_backRef.get();
        if (pad > 0 && back == NULL)
        {
            back = root->createWidget<MyGUI::Widget>("Kenshi_FloatingPanelSkin",
                MyGUI::IntCoord(0, 0, w + 2 * pad, h + 2 * pad), MyGUI::Align::Default, "");
            back->setNeedMouseFocus(false);
            g_backRef.set(back);
            // Подложка должна быть под картинкой: картинку пересоздаём сверху.
            if (MyGUI::Widget* const old = g_imageRef.get())
                MyGUI::Gui::getInstance().destroyWidget(old);
            g_imageRef.reset();
        }
        else if (pad == 0 && back != NULL)
        {
            MyGUI::Gui::getInstance().destroyWidget(back);
            g_backRef.reset();
        }

        MyGUI::Widget* image = g_imageRef.get();
        if (image == NULL)
        {
            image = root->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(pad, pad, w, h),
                                                        MyGUI::Align::Default, "");
            image->setNeedMouseFocus(false);
            g_imageRef.set(image);
            g_viewW = g_viewH = 0;
        }

        if (root->getWidth() != w + 2 * pad || root->getHeight() != h + 2 * pad)
        {
            // Размер меняется - держим тот край, к которому миникарта ближе:
            // стоит справа - растёт влево, снизу - вверх. Раньше всегда рос
            // вправо-вниз от левого верхнего угла, и миникарта в правом углу
            // уезжала (20.10.2026).
            const int wasLeft = g_left, wasTop = g_top;
            if (!created && g_placed)
            {
                const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
                const int oldW = root->getWidth(), oldH = root->getHeight();
                if (g_left + oldW / 2 > view.width / 2)
                    g_left += oldW - (w + 2 * pad);
                if (g_top + oldH / 2 > view.height / 2)
                    g_top += oldH - (h + 2 * pad);
            }
            ClampPosition(w + 2 * pad, h + 2 * pad);
            // Новое место - в ini: иначе перечитка настроек (MCM) вернула бы
            // старое.
            if (g_placed && (g_left != wasLeft || g_top != wasTop))
            {
                WriteInt("Left", g_left);
                WriteInt("Top", g_top);
            }
            root->setCoord(g_left, g_top, w + 2 * pad, h + 2 * pad);
            if (back != NULL)
                back->setSize(w + 2 * pad, h + 2 * pad);
        }

        if (!EnsureTexture(w, h))
            return false;
        MyGUI::ImageBox* const box = image->castType<MyGUI::ImageBox>();
        if (g_viewW != w || g_viewH != h || box->_getTextureName() != g_texName)
        {
            box->setCoord(pad, pad, w, h);
            // Текстура, часть её и «плитка» - одним вызовом. По отдельности
            // нельзя: размер плитки ImageBox запоминает при первой установке
            // и сам не меняет, и после смены размера миникарты показывался
            // кусок старого размера, растянутый на новый, - центр уезжал
            // (20.10.2026).
            box->setImageInfo(g_texName, MyGUI::IntCoord(0, 0, w, h), MyGUI::IntSize(w, h));
            // И кадр выбрать: setImageInfo кадр не выбирает (в отличие от
            // setImageCoord), а без кадра ImageBox не рисует ничего - карта
            // пропадала совсем, оставались одни подписи (20.10.2026).
            box->setImageIndex(0);
            g_viewW = w;
            g_viewH = h;
            g_dirtyMask = true;
        }
        return true;
    }

    void HideTooltip();

    void Show(bool on)
    {
        // Скрыто - и так скрыто: Hide зовётся каждый кадр, пока причина есть.
        if (!on && g_hidden)
            return;
        if (MyGUI::Widget* const root = g_rootRef.get())
            root->setVisible(on);
        g_hidden = !on;
        if (!on)
        {
            g_dragging = false;
            HideTooltip();
        }
    }


    // ---------------------------------------------------------------
    // Рисование
    // ---------------------------------------------------------------

    float InsideShapeOf(int shape, float fx, float fy, int w, int h, float inset);

    struct FrameCov { unsigned index; float alpha; };
    std::vector<FrameCov> g_frameCov;

    // Маска формы с мягким краем (1 пиксель сглаживания).
    void BuildMask(int w, int h)
    {
        g_mask.assign(static_cast<size_t>(w) * h, 0);
        const float cx = w * 0.5f, cy = h * 0.5f;
        const float radius = Lo(w, h) * 0.5f - 0.5f;
        const float corner = g_rshape == SHAPE_CIRCLE ? 0.0f : 3.0f;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                float inside;
                const float fx = x + 0.5f, fy = y + 0.5f;
                if (g_rshape == SHAPE_CIRCLE)
                {
                    const float d = std::sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
                    inside = radius - d + 0.5f;
                }
                else
                {
                    // Прямоугольник со скруглением угла corner.
                    const float qx = std::fabs(fx - cx) - (cx - corner);
                    const float qy = std::fabs(fy - cy) - (cy - corner);
                    const float ox = Hi(qx, 0.0f), oy = Hi(qy, 0.0f);
                    const float outside = std::sqrt(ox * ox + oy * oy) + Lo(Hi(qx, qy), 0.0f) - corner;
                    inside = -outside + 0.5f;
                }
                g_mask[static_cast<size_t>(y) * w + x] =
                    static_cast<unsigned char>(Clamp(inside, 0.0f, 1.0f) * 255.0f + 0.5f);
            }
        // Рамка-линия: где и насколько её рисовать - тоже один раз, а не
        // InsideShape в каждом пикселе каждой перерисовки.
        g_frameCov.clear();
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const size_t i = static_cast<size_t>(y) * w + x;
                if (g_mask[i] == 0)
                    continue;
                const float inner = InsideShapeOf(g_rshape, x + 0.5f, y + 0.5f, w, h, 2.0f);
                if (inner >= 1.0f)
                    continue;
                FrameCov c;
                c.index = static_cast<unsigned>(i);
                c.alpha = (1.0f - inner) * (g_mask[i] / 255.0f);
                g_frameCov.push_back(c);
            }
    }

    // Насколько точка внутри формы с отступом inset (для рамки): 0..1.
    float InsideShapeOf(int shape, float fx, float fy, int w, int h, float inset)
    {
        const float cx = w * 0.5f, cy = h * 0.5f;
        if (shape == SHAPE_CIRCLE)
        {
            const float radius = Lo(w, h) * 0.5f - inset;
            const float d = std::sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
            return Clamp(radius - d + 0.5f, 0.0f, 1.0f);
        }
        const float dx = cx - inset - std::fabs(fx - cx), dy = cy - inset - std::fabs(fy - cy);
        return Clamp(Lo(dx, dy) + 0.5f, 0.0f, 1.0f);
    }

    // То же для текущей формы - в основном потоке (метки, подписи, мышь).
    float InsideShape(float fx, float fy, int w, int h, float inset)
    {
        return InsideShapeOf(g_shape, fx, fy, w, h, inset);
    }

    inline unsigned Blend(unsigned dst, unsigned src, float a)
    {
        if (a <= 0.0f)
            return dst;
        if (a >= 1.0f)
            return (src & 0x00FFFFFFu) | (static_cast<unsigned>(Hi(ChA(dst), ChA(src))) << 24);
        const float b = 1.0f - a;
        const int r = static_cast<int>(ChR(dst) * b + ChR(src) * a + 0.5f);
        const int g = static_cast<int>(ChG(dst) * b + ChG(src) * a + 0.5f);
        const int bl = static_cast<int>(ChB(dst) * b + ChB(src) * a + 0.5f);
        const int al = static_cast<int>(Hi(static_cast<float>(ChA(dst)), ChA(src) * a) + 0.5f);
        return Pack(r, g, bl, al);
    }

    // Диск с тёмной обводкой; clip - обрезать по форме карты.
    void DrawDisc(float cx, float cy, float r, unsigned fill, unsigned outline, float alpha)
    {
        const int w = g_rw, h = g_rh;
        const int x0 = Hi(0, static_cast<int>(cx - r - 2)), x1 = Lo(w - 1, static_cast<int>(cx + r + 2));
        const int y0 = Hi(0, static_cast<int>(cy - r - 2)), y1 = Lo(h - 1, static_cast<int>(cy + r + 2));
        // Корень - только на краю: внутри (без сглаживания) и снаружи
        // хватает квадрата расстояния.
        const float out2 = (r + 0.5f) * (r + 0.5f);
        const float solid = Hi(0.0f, r - 1.5f), solid2 = solid * solid;
        const bool opaque = alpha >= 1.0f;
        const unsigned fillA = (fill & 0x00FFFFFFu) | 0xFF000000u;
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = x + 0.5f - cx, fy = y + 0.5f - cy;
                const float d2 = fx * fx + fy * fy;
                if (d2 >= out2)
                    continue;
                const size_t i = static_cast<size_t>(y) * w + x;
                if (opaque && d2 < solid2 && g_mask[i] == 255)
                {
                    g_buf[i] = fillA;
                    continue;
                }
                const float d = std::sqrt(d2);
                const float cover = Clamp(r + 0.5f - d, 0.0f, 1.0f);
                if (cover <= 0.0f)
                    continue;
                const float clip = g_mask[i] / 255.0f;
                const float inner = Clamp(r - 1.0f + 0.5f - d, 0.0f, 1.0f);
                unsigned c = Blend(g_buf[i], outline, cover * clip * alpha);
                c = Blend(c, fill, inner * clip * alpha);
                g_buf[i] = c;
            }
    }

    // Треугольник «куда смотрит камера» в центре.
    void DrawView(float cx, float cy, float dirX, float dirY, float len, unsigned colour)
    {
        const float half = len * 0.55f;
        const float tipX = cx + dirX * len, tipY = cy + dirY * len;
        const float nx = -dirY, ny = dirX;
        const float bx = cx - dirX * len * 0.35f, by = cy - dirY * len * 0.35f;
        const float ax = bx + nx * half, ay = by + ny * half;
        const float cx2 = bx - nx * half, cy2 = by - ny * half;
        const int x0 = Hi(0, static_cast<int>(Lo(Lo(tipX, ax), cx2)) - 1);
        const int x1 = Lo(g_rw - 1, static_cast<int>(Hi(Hi(tipX, ax), cx2)) + 1);
        const int y0 = Hi(0, static_cast<int>(Lo(Lo(tipY, ay), cy2)) - 1);
        const int y1 = Lo(g_rh - 1, static_cast<int>(Hi(Hi(tipY, ay), cy2)) + 1);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                // По 4 выборки на пиксель - мягкий край без лишних формул.
                int hits = 0;
                for (int s = 0; s < 4; ++s)
                {
                    const float px = x + 0.25f + 0.5f * (s & 1), py = y + 0.25f + 0.5f * (s >> 1);
                    const float e1 = (ax - tipX) * (py - tipY) - (ay - tipY) * (px - tipX);
                    const float e2 = (cx2 - ax) * (py - ay) - (cy2 - ay) * (px - ax);
                    const float e3 = (tipX - cx2) * (py - cy2) - (tipY - cy2) * (px - cx2);
                    if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0))
                        ++hits;
                }
                if (hits == 0)
                    continue;
                const size_t i = static_cast<size_t>(y) * g_rw + x;
                g_buf[i] = Blend(g_buf[i], colour, hits / 4.0f * (g_mask[i] / 255.0f) * 0.85f);
            }
    }

    // Мир / доля картинки -> пиксель миникарты. Возвращает расстояние от
    // центра в долях «радиуса» (больше 1 - за краем).
    struct View
    {
        double su, sv;          // центр в пикселях картинки
        double scale;           // пикселей картинки на пиксель миникарты
        double c, s;            // поворот экрана в картинку
        float cx, cy;           // центр миникарты
        int sw, sh;             // размер картинки
    };

    void ToScreen(const View& v, double u, double vv, float& x, float& y)
    {
        const double dx = (u * v.sw - v.su) / v.scale, dy = (vv * v.sh - v.sv) / v.scale;
        // Обратный поворот (транспонированная матрица).
        x = static_cast<float>(v.cx + v.c * dx + v.s * dy);
        y = static_cast<float>(v.cy - v.s * dx + v.c * dy);
    }

    // Метку за краем прижимаем к краю (PinToEdge) или пропускаем.
    bool PlacePin(float& x, float& y, float inset, bool force = false)
    {
        if (InsideShape(x, y, g_viewW, g_viewH, inset) > 0.5f)
            return true;
        if (!g_pinToEdge && !force)
            return false;
        const float cx = g_viewW * 0.5f, cy = g_viewH * 0.5f;
        float dx = x - cx, dy = y - cy;
        if (g_shape == SHAPE_CIRCLE)
        {
            const float r = Lo(g_viewW, g_viewH) * 0.5f - inset - 1.0f;
            const float d = std::sqrt(dx * dx + dy * dy);
            if (d < 1e-3f)
                return true;
            x = cx + dx / d * r;
            y = cy + dy / d * r;
        }
        else
        {
            const float hx = cx - inset - 1.0f, hy = cy - inset - 1.0f;
            const float k = Lo(std::fabs(dx) > 1e-3f ? hx / std::fabs(dx) : 1e9f,
                               std::fabs(dy) > 1e-3f ? hy / std::fabs(dy) : 1e9f);
            x = cx + dx * k;
            y = cy + dy * k;
        }
        return true;
    }

    // Кольцо толщиной thick (со сглаживанием), обрезанное по форме карты.
    void DrawRing(float cx, float cy, float r, float thick, unsigned colour, float alpha)
    {
        const float outer = r + thick * 0.5f + 1.0f;
        const int x0 = Hi(0, static_cast<int>(cx - outer)), x1 = Lo(g_rw - 1, static_cast<int>(cx + outer));
        const int y0 = Hi(0, static_cast<int>(cy - outer)), y1 = Lo(g_rh - 1, static_cast<int>(cy + outer));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = x + 0.5f - cx, fy = y + 0.5f - cy;
                const float d = std::sqrt(fx * fx + fy * fy);
                const float cover = Clamp(thick * 0.5f + 0.5f - std::fabs(d - r), 0.0f, 1.0f);
                if (cover <= 0.0f)
                    continue;
                const size_t i = static_cast<size_t>(y) * g_rw + x;
                g_buf[i] = Blend(g_buf[i], colour, cover * alpha * (g_mask[i] / 255.0f));
            }
    }

    // Граница области, докуда долетает камера: объединение кругов радиусом
    // CameraRange вокруг своих персонажей - рисуем только те точки круга,
    // что не лежат внутри другого круга. Окружность - точками по углу: так
    // дешевле, чем считать расстояние до всех центров в каждом пикселе.
    //
    // Своих бывает под сотню (аванпосты), поэтому: берём только круги,
    // задевающие миникарту; центры ближе пары пикселей (отряд в куче)
    // склеиваем; каждую точку проверяем лишь по соседним кругам; пунктир -
    // сразу точками через 7 пикселей, без промежуточных выборок.
    void DrawCameraRange(const std::vector<float>& sx, const std::vector<float>& sy, float r, bool dotted,
                         unsigned colour)
    {
        if (r < 3.0f || sx.empty())
            return;
        const float w = static_cast<float>(g_rw), h = static_cast<float>(g_rh);
        const float r2 = r * r;
        const float merge = Hi(1.5f, r * 0.02f);
        std::vector<float> cx, cy;
        for (size_t i = 0; i < sx.size(); ++i)
        {
            const float x = sx[i], y = sy[i];
            // Круг не задевает миникарту - не нужен ни для линии, ни для
            // проверки «внутри».
            const float nx = Clamp(x, 0.0f, w), ny = Clamp(y, 0.0f, h);
            if ((nx - x) * (nx - x) + (ny - y) * (ny - y) > (r + 2.0f) * (r + 2.0f))
                continue;
            // Круг накрывает всю миникарту - линии в ней нет вовсе.
            const float fx = Hi(std::fabs(x), std::fabs(x - w)), fy = Hi(std::fabs(y), std::fabs(y - h));
            if (fx * fx + fy * fy < r2)
                return;
            bool dup = false;
            for (size_t j = 0; j < cx.size() && !dup; ++j)
                dup = std::fabs(cx[j] - x) < merge && std::fabs(cy[j] - y) < merge;
            if (!dup)
            {
                cx.push_back(x);
                cy.push_back(y);
            }
        }
        const float r2in = (r - 0.75f) * (r - 0.75f);
        const float circ = 6.2832f * r;
        const int steps = dotted ? Hi(8, static_cast<int>(circ / 7.0f))
                                         : Clamp(static_cast<int>(circ * 1.5f), 32, 20000);
        std::vector<size_t> nearby;
        for (size_t i = 0; i < cx.size(); ++i)
        {
            // Соседи: только их круги могут закрыть точки этого.
            nearby.clear();
            for (size_t j = 0; j < cx.size(); ++j)
                if (j != i && (cx[j] - cx[i]) * (cx[j] - cx[i]) + (cy[j] - cy[i]) * (cy[j] - cy[i]) < 4.0f * r2)
                    nearby.push_back(j);
            for (int k = 0; k < steps; ++k)
            {
                const float a = 6.2832f * k / steps;
                const float x = cx[i] + r * std::cos(a), y = cy[i] + r * std::sin(a);
                if (x < -1 || y < -1 || x > w + 1 || y > h + 1)
                    continue;
                bool inside = false;
                for (size_t n = 0; n < nearby.size() && !inside; ++n)
                {
                    const size_t j = nearby[n];
                    inside = (x - cx[j]) * (x - cx[j]) + (y - cy[j]) * (y - cy[j]) < r2in;
                }
                if (inside)
                    continue;
                if (dotted)
                {
                    DrawDisc(x, y, 1.1f, colour, colour, 0.85f);
                    continue;
                }
                const int px = static_cast<int>(x), py = static_cast<int>(y);
                if (px < 0 || py < 0 || px >= g_rw || py >= g_rh)
                    continue;
                const size_t idx = static_cast<size_t>(py) * g_rw + px;
                g_buf[idx] = Blend(g_buf[idx], colour, 0.7f * (g_mask[idx] / 255.0f));
            }
        }
    }

    // Последний нарисованный вид: по нему ПКМ переводит точку на миникарте в
    // мир.
    View g_lastView;
    bool g_haveView = false;


    // Закрашенный треугольник (4 выборки на пиксель), обрезанный по форме.
    void DrawTri(float ax, float ay, float bx, float by, float cx, float cy, unsigned colour, float alpha)
    {
        const int x0 = Hi(0, static_cast<int>(Lo(Lo(ax, bx), cx)) - 1);
        const int x1 = Lo(g_rw - 1, static_cast<int>(Hi(Hi(ax, bx), cx)) + 1);
        const int y0 = Hi(0, static_cast<int>(Lo(Lo(ay, by), cy)) - 1);
        const int y1 = Lo(g_rh - 1, static_cast<int>(Hi(Hi(ay, by), cy)) + 1);
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                int hits = 0;
                for (int k = 0; k < 4; ++k)
                {
                    const float px = x + 0.25f + 0.5f * (k & 1), py = y + 0.25f + 0.5f * (k >> 1);
                    const float e1 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
                    const float e2 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
                    const float e3 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
                    if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0))
                        ++hits;
                }
                if (hits == 0)
                    continue;
                const size_t i = static_cast<size_t>(y) * g_rw + x;
                g_buf[i] = Blend(g_buf[i], colour, hits / 4.0f * alpha * (g_mask[i] / 255.0f));
            }
    }

    // ---- подписи: пул TextBox поверх картинки ----
    // Текст рисует MyGUI (шрифт игры), а не наш буфер: так буквы чёткие и
    // на любом языке. Пул постоянный: виджеты создаются по мере нужды и
    // прячутся, когда не нужны; ссылки - WidgetRef, обнулятся сами, если
    // игра пересоздаст интерфейс.
    const int kLabelPool = 64;
    WidgetRef g_labelRefs[kLabelPool];
    int g_labelsUsed = 0;
    // Уже поставленные в этой перерисовке: новая подпись, налезающая на
    // них, не ставится - при дальнем обзоре иначе каша. Кто раньше, тот и
    // виден: поэтому сначала важное (свои, поселения), потом прочее.
    std::vector<MyGUI::IntCoord> g_labelRects;
    // Размер текста по строке: мерить можно только виджетом, а менять ему
    // подпись ради замера каждый кадр - лишняя перекладка текста.
    std::map<std::string, MyGUI::IntSize> g_textSizes;

    MyGUI::TextBox* LabelBox(int slot)
    {
        MyGUI::Widget* const root = g_rootRef.get();
        if (root == NULL)
            return NULL;
        WidgetRef& ref = g_labelRefs[slot];
        MyGUI::TextBox* box = ref.get() ? ref.get()->castType<MyGUI::TextBox>(false) : NULL;
        if (box == NULL)
        {
            box = root->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(0, 0, 200, 20), MyGUI::Align::Default, "");
            box->setNeedMouseFocus(false);
            box->setTextShadow(true);
            box->setVisible(false);
            ref.set(box);
        }
        return box;
    }

    void SetLabelText(MyGUI::TextBox* box, const std::string& text)
    {
        if (box->getUserString("bm_text") != text)
        {
            box->setUserString("bm_text", text);
            box->setCaption(MyGUI::UString(text));
        }
    }

    // Подпись: по умолчанию по центру над точкой x, y (низ подписи - y);
    // rightOf - слева от неё x, по высоте по центру y. font - шрифт (пусто -
    // шрифт оформления).
    // Шрифт игры под высоту px: самый мелкий не ниже её - его и ужимаем до
    // px (уменьшенный остаётся чётким, растянутый расплылся бы). Выше
    // самого крупного - крупный, растянутый.
    std::string FontFor(int px)
    {
        static int lastPx = -1;
        static std::string lastFont;
        if (px == lastPx)
            return lastFont;
        static const char* const fonts[] = { "Kenshi_StandardFont_Small", "Kenshi_StandardFont_MediumSmall",
                                             "Kenshi_StandardFont_Medium", "Kenshi_StandardFont_Large",
                                             "Kenshi_PaintedTextFont_Large" };
        std::string pick;
        for (size_t i = 0; i < sizeof(fonts) / sizeof(fonts[0]); ++i)
        {
            MyGUI::IFont* const f = MyGUI::FontManager::getInstance().getByName(fonts[i]);
            if (f == NULL)
                continue;
            pick = fonts[i];
            if (f->getDefaultHeight() >= px)
                break;
        }
        lastPx = px;
        lastFont = pick;
        return pick;
    }

    void PlaceLabel(const std::string& text, float x, float y, unsigned colour, int fontPx = 0, bool rightOf = false)
    {
        if (text.empty() || g_labelsUsed >= kLabelPool)
            return;
        // Подпись - только если её точка внутри формы карты.
        if (InsideShape(x, y, g_viewW, g_viewH, 2.0f) < 0.5f)
            return;
        MyGUI::TextBox* const box = LabelBox(g_labelsUsed);
        if (box == NULL)
            return;
        // Шрифт: под высоту fontPx или тот, с которым виджет создан.
        if (box->getUserString("bm_font0").empty())
        {
            box->setUserString("bm_font0", box->getFontName());
            char h[16];
            sprintf_s(h, "%d", box->getFontHeight());
            box->setUserString("bm_height0", h);
        }
        const std::string wantFont = fontPx > 0 ? FontFor(fontPx) : box->getUserString("bm_font0");
        const int wantHeight = fontPx > 0 ? fontPx : atoi(box->getUserString("bm_height0").c_str());
        if (!wantFont.empty() && box->getFontName() != wantFont)
            box->setFontName(wantFont);
        if (wantHeight > 0 && box->getFontHeight() != wantHeight)
            box->setFontHeight(wantHeight);
        char heightText[16];
        sprintf_s(heightText, "%d|", wantHeight);
        const std::string sizeKey = heightText + wantFont + "|" + text;
        std::map<std::string, MyGUI::IntSize>::const_iterator known = g_textSizes.find(sizeKey);
        MyGUI::IntSize ts;
        if (known != g_textSizes.end())
            ts = known->second;
        else
        {
            SetLabelText(box, text);
            ts = box->getTextSize();
            if (g_textSizes.size() > 1024)
                g_textSizes.clear();
            g_textSizes[sizeKey] = ts;
        }
        const int pad = GamePad();
        int left = pad + static_cast<int>(x) - (rightOf ? 0 : ts.width / 2);
        int top = pad + static_cast<int>(y) - (rightOf ? ts.height / 2 : ts.height);
        left = Clamp(left, pad, Hi(pad, pad + g_viewW - ts.width));
        top = Clamp(top, pad, Hi(pad, pad + g_viewH - ts.height));
        const MyGUI::IntCoord rc(left, top, ts.width + 2, ts.height);
        for (size_t i = 0; i < g_labelRects.size(); ++i)
        {
            const MyGUI::IntCoord& o = g_labelRects[i];
            // По высоте строки шрифта есть пустые поля - касание в 4
            // пикселя за наложение не считаем.
            if (rc.left < o.left + o.width && o.left < rc.left + rc.width &&
                rc.top < o.top + o.height - 4 && o.top < rc.top + rc.height - 4)
                return;
        }
        g_labelRects.push_back(rc);
        ++g_labelsUsed;
        SetLabelText(box, text);
        const MyGUI::Colour tc(ChR(colour) / 255.0f, ChG(colour) / 255.0f, ChB(colour) / 255.0f);
        const MyGUI::Colour& was = box->getTextColour();
        if (was.red != tc.red || was.green != tc.green || was.blue != tc.blue)
            box->setTextColour(tc);
        if (box->getCoord() != rc)
            box->setCoord(rc);
        if (!box->getVisible())
            box->setVisible(true);
    }

    void HideUnusedLabels()
    {
        for (int i = g_labelsUsed; i < kLabelPool; ++i)
            if (MyGUI::Widget* const w = g_labelRefs[i].get())
                if (w->getVisible())
                    w->setVisible(false);
    }

    // ---- подсказка при наведении ----
    // Отдельное окошко в слое подсказок (поверх всего, не обрезается краем
    // миникарты): фон - панель игры, текст - над курсором, чтобы курсор его
    // не закрывал; сверху места нет - под курсором.
    WidgetRef g_tipRef;         // фон
    WidgetRef g_tipTextRef;     // текст

    void ShowTooltip(const std::string& text)
    {
        MyGUI::Widget* back = g_tipRef.get();
        MyGUI::TextBox* box = g_tipTextRef.get() ? g_tipTextRef.get()->castType<MyGUI::TextBox>(false) : NULL;
        if (back == NULL || box == NULL)
        {
            if (back != NULL)
                MyGUI::Gui::getInstance().destroyWidget(back);
            back = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("Kenshi_FloatingPanelSkin",
                MyGUI::IntCoord(0, 0, 300, 40), MyGUI::Align::Default, "ToolTip");
            back->setNeedMouseFocus(false);
            box = back->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(8, 5, 284, 30), MyGUI::Align::Default, "");
            box->setNeedMouseFocus(false);
            box->setTextShadow(true);
            g_tipRef.set(back);
            g_tipTextRef.set(box);
        }
        if (box->getUserString("bm_text") != text)
        {
            box->setUserString("bm_text", text);
            box->setCaption(MyGUI::UString(text));
        }
        const MyGUI::IntSize ts = box->getTextSize();
        const int w = ts.width + 18, h = ts.height + 10;
        const MyGUI::IntPoint m = MyGUI::InputManager::getInstance().getMousePosition();
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        int left = m.left - w / 2, top = m.top - h - 14;
        if (top < 0)
            top = m.top + 30;
        left = Clamp(left, 0, Hi(0, view.width - w));
        top = Clamp(top, 0, Hi(0, view.height - h));
        box->setCoord(9, 5, ts.width + 2, ts.height);
        back->setCoord(left, top, w, h);
        back->setVisible(true);
    }

    void HideTooltip()
    {
        if (MyGUI::Widget* const w = g_tipRef.get())
            w->setVisible(false);
    }

    // ---- поселения и прочие места большой карты ----
    // Список - у игры (shou->townList, см. CollectTowns); открыто ли -
    // isDiscovered.
    struct TownInfo
    {
        Ogre::Vector3 pos;
        float radius;
        std::string name;
        unsigned colour;
        std::string tip;
        // Значок большой карты, готовый к рисованию (NULL - ромб).
        const struct Icon* icon;
    };
    std::vector<TownInfo> g_towns;

    std::string RelationText(Faction* f, Faction* player, unsigned& colour)
    {
        colour = g_townColour;
        if (f == NULL || player == NULL)
            return std::string();
        if (f == player)
        {
            colour = g_colour[CAT_SQUAD];
            return Tr("your faction");
        }
        if (f->relations == NULL)
            return std::string();
        const int value = static_cast<int>(f->relations->getFactionRelation(player));
        char num[24];
        sprintf_s(num, " (%d)", value);
        if (f->relations->isEnemy(player))
        {
            colour = 0xFFE63C32u;
            return std::string(Tr("hostile")) + num;
        }
        if (f->relations->isAlly(player))
        {
            colour = 0xFF5AD25Au;
            return std::string(Tr("ally")) + num;
        }
        return std::string(Tr("neutral")) + num;
    }

    // ---- значки поселений как на большой карте ----
    // Значок большой карты - ImageBox метки поселения (MapMarkerTown::image).
    // Текстура у него открыта (_getTextureName), а кусок атласа - в закрытых
    // полях ImageBox. Читаем их через класс с той же раскладкой полей
    // (MyGUI 3.2 из заголовков игры) и проверяем раскладку по размеру
    // текстуры: не сошёлся - значка нет, будет ромб.
    class ImageBoxPeek : public MyGUI::Widget, public MyGUI::MemberObsolete<MyGUI::ImageBox>
    {
    public:
        MyGUI::IntRect mRectImage;
        MyGUI::IntSize mSizeTile;
        MyGUI::IntSize mSizeTexture;
        size_t mIndexSelect;
        MyGUI::VectorImages mItems;
    };

    // Значок, вырезанный из атласа, уменьшенный до TownIconSize (и
    // тонированный tint). Атлас читается в память один раз (как
    // картинка карты); значки копятся в кэше по ключу и не удаляются - поток
    // рисования берёт их по указателю.
    struct Icon
    {
        int w, h;
        std::vector<unsigned> px;   // ARGB, не умноженный на альфу
    };

    std::map<std::string, Source> g_atlases;
    std::map<std::string, Icon> g_icons;

    const Icon* IconFor(const std::string& texture, const MyGUI::IntCoord& c, int texW, int texH,
                        const MyGUI::Colour& tint, int height)
    {
        char key[320];
        sprintf_s(key, "%s|%d,%d,%d,%d|%d|%.3f,%.3f,%.3f,%.3f", texture.c_str(), c.left, c.top, c.width, c.height,
                  height, tint.red, tint.green, tint.blue, tint.alpha);
        std::map<std::string, Icon>::iterator it = g_icons.find(key);
        if (it != g_icons.end())
            return it->second.px.empty() ? NULL : &it->second;
        if (g_icons.size() > 2000)
            return NULL;
        Icon& icon = g_icons[key];
        icon.w = icon.h = 0;
        std::map<std::string, Source>::iterator a = g_atlases.find(texture);
        if (a == g_atlases.end())
        {
            const DWORD started = GetTickCount();
            std::string how;
            a = g_atlases.insert(std::make_pair(texture, Source())).first;
            const bool ok = ReadTexture(texture, a->second, 8192, false, how);
            char note[300];
            sprintf_s(note, "BestMinimap: map icons '%s' %s (%s), %dx%d, %u ms", texture.c_str(),
                      ok ? "read" : "could not be read", how.c_str(), a->second.w, a->second.h,
                      GetTickCount() - started);
            DebugLog(note);
        }
        const Source& atlas = a->second;
        if (!atlas.ok() || texW <= 0 || texH <= 0)
            return NULL;
        // Атлас в памяти может быть другого размера, чем текстура.
        const float sx = static_cast<float>(atlas.w) / texW, sy = static_cast<float>(atlas.h) / texH;
        const int left = static_cast<int>(c.left * sx), top = static_cast<int>(c.top * sy);
        const int cw = Hi(1, static_cast<int>(c.width * sx)), ch = Hi(1, static_cast<int>(c.height * sy));
        if (left < 0 || top < 0 || left + cw > atlas.w || top + ch > atlas.h)
            return NULL;
        icon.h = height;
        icon.w = Hi(1, cw * height / ch);
        icon.px.assign(static_cast<size_t>(icon.w) * icon.h, 0);
        // Уменьшение средним по площади, с весом альфы (края не темнеют).
        for (int y = 0; y < icon.h; ++y)
        {
            const int y0 = top + y * ch / icon.h, y1 = Hi(y0 + 1, top + (y + 1) * ch / icon.h);
            for (int x = 0; x < icon.w; ++x)
            {
                const int x0 = left + x * cw / icon.w, x1 = Hi(x0 + 1, left + (x + 1) * cw / icon.w);
                double sa = 0, sr = 0, sg = 0, sb = 0;
                int n = 0;
                for (int yy = y0; yy < y1; ++yy)
                    for (int xx = x0; xx < x1; ++xx)
                    {
                        const unsigned p = atlas.px[static_cast<size_t>(yy) * atlas.w + xx];
                        const double al = ChA(p);
                        sa += al;
                        sr += ChR(p) * al;
                        sg += ChG(p) * al;
                        sb += ChB(p) * al;
                        ++n;
                    }
                if (sa <= 0.0)
                    continue;
                const int r = static_cast<int>(sr / sa * tint.red + 0.5);
                const int g = static_cast<int>(sg / sa * tint.green + 0.5);
                const int b = static_cast<int>(sb / sa * tint.blue + 0.5);
                const int al = static_cast<int>(sa / n * tint.alpha + 0.5);
                icon.px[static_cast<size_t>(y) * icon.w + x] = Pack(Clamp(r, 0, 255), Clamp(g, 0, 255),
                                                                    Clamp(b, 0, 255), Clamp(al, 0, 255));
            }
        }
        return &icon;
    }

    bool IconOf(MyGUI::ImageBox* img, std::string& texture, MyGUI::IntCoord& coord, int& texW, int& texH)
    {
        if (img == NULL)
            return false;
        texture = img->_getTextureName();
        if (texture.empty())
            return false;
        MyGUI::ITexture* const tex = MyGUI::RenderManager::getInstance().getTexture(texture);
        if (tex == NULL || tex->getWidth() <= 0 || tex->getHeight() <= 0)
            return false;
        const ImageBoxPeek* const peek = reinterpret_cast<const ImageBoxPeek*>(img);
        if (peek->mSizeTexture.width != tex->getWidth() || peek->mSizeTexture.height != tex->getHeight())
            return false;       // раскладка не та - не читаем дальше
        const size_t count = peek->mItems.size();
        if (peek->mIndexSelect >= count || count > 4096 || peek->mItems[peek->mIndexSelect].images.empty())
            return false;
        const MyGUI::FloatRect& uv = peek->mItems[peek->mIndexSelect].images[0];
        texW = tex->getWidth();
        texH = tex->getHeight();
        const float tw = static_cast<float>(texW), th = static_cast<float>(texH);
        coord = MyGUI::IntCoord(static_cast<int>(uv.left * tw + 0.5f), static_cast<int>(uv.top * th + 0.5f),
                                static_cast<int>((uv.right - uv.left) * tw + 0.5f),
                                static_cast<int>((uv.bottom - uv.top) * th + 0.5f));
        return coord.width > 0 && coord.height > 0 && coord.left >= 0 && coord.top >= 0 &&
               coord.right() <= tex->getWidth() && coord.bottom() <= tex->getHeight();
    }

    // Метки большой карты по поселению (по ссылке игры, а если её не
    // вышло получить - по подписи метки).
    std::map<TownBase*, MapScreen::MapMarkerTown*> g_markerByTown;
    std::map<std::string, MapScreen::MapMarkerTown*> g_markerByName;

    // Метки (и значки) большая карта строит при первом открытии. Не
    // открывали - просим её построить их сами, один раз на карту:
    // refreshTowns игры (защищённый - вызываем через наследника).
    struct MapScreenAccess : public MapScreen
    {
        static void RefreshTowns(MapScreen* map) { (map->*(&MapScreenAccess::refreshTowns))(); }
    };
    MapScreen* g_markersBuiltFor = NULL;
    size_t g_markersIndexed = 0;        // сколько меток было при последнем разборе
    DWORD g_markersIndexedAt = 0;

    // Разбор меток карты - когда их число сменилось, и не чаще раза в 10 с
    // без этого: раньше - каждую секунду, с копией подписи каждой метки.
    void IndexMarkers(MapScreen* map, DWORD now)
    {
        if (map == NULL || !g_townGameIcons)
        {
            g_markerByTown.clear();
            g_markerByName.clear();
            g_markersIndexed = 0;
            return;
        }
        if (map->mapMarkersTowns.empty() && g_markersBuiltFor != map)
        {
            g_markersBuiltFor = map;
            MapScreenAccess::RefreshTowns(map);
            char note[120];
            sprintf_s(note, "BestMinimap: asked the big map to build its town markers: %u",
                      static_cast<unsigned>(map->mapMarkersTowns.size()));
            DebugLog(note);
        }
        if (map->mapMarkersTowns.size() == g_markersIndexed && now - g_markersIndexedAt < 10000 &&
            g_markersIndexedAt != 0)
            return;
        g_markersIndexed = map->mapMarkersTowns.size();
        g_markersIndexedAt = now;
        g_markerByTown.clear();
        g_markerByName.clear();
        typedef ogre_unordered_map<hand, MapScreen::MapMarkerTown*>::type Towns;
        const Towns& towns = map->mapMarkersTowns;
        for (Towns::const_iterator it = towns.begin(); it != towns.end(); ++it)
        {
            MapScreen::MapMarkerTown* const m = it->second;
            if (m == NULL || m->image == NULL)
                continue;
            TownBase* t = it->first.getTown();
            if (t == NULL)
                t = m->handle.getTown();
            if (t != NULL)
                g_markerByTown[t] = m;
            if (m->label != NULL)
                g_markerByName[m->label->getCaption().asUTF8()] = m;
        }
    }

    // Что у поселения меняется редко (имя, значок) - на 10 с; отношения -
    // каждый раз (война начинается сразу).
    struct TownStatic
    {
        std::string name;
        const Icon* icon;
        bool iconTried;
        int iconSize;           // под какой TownIconSize подбирали
        DWORD at;
        TownStatic() : icon(NULL), iconTried(false), iconSize(0), at(0) {}
    };
    std::map<TownBase*, TownStatic> g_townStatic;

    void AddTown(TownBase* t, Faction* player, const Ogre::Vector3& center, float reach, DWORD now)
    {
        TownInfo info;
        info.icon = NULL;
        info.pos = t->getPosition();
        info.radius = t->getRadius();
        const float dx = info.pos.x - center.x, dz = info.pos.z - center.z;
        const float lim = reach + info.radius;
        if (dx * dx + dz * dz > lim * lim)
            return;
        if (!t->isDiscovered())
            return;
        TownStatic& st = g_townStatic[t];
        if (st.at == 0 || now - st.at > 10000 || st.iconSize != g_townIconSize)
        {
            st.at = now;
            st.name = t->getKnownName();
            st.icon = NULL;
            st.iconTried = false;
            st.iconSize = g_townIconSize;
        }
        info.name = st.name;
        Faction* const f = t->getFaction();
        unsigned relColour = g_townColour;
        const std::string rel = RelationText(f, player, relColour);
        info.colour = g_townAutoColour ? relColour : g_townColour;
        info.tip = info.name;
        if (f != NULL)
            info.tip += "\n" + f->getName() + (rel.empty() ? std::string() : " - " + rel);
        if (g_townGameIcons && !st.iconTried && !g_markerByTown.empty())
        {
            st.iconTried = true;
            std::map<TownBase*, MapScreen::MapMarkerTown*>::const_iterator m = g_markerByTown.find(t);
            MapScreen::MapMarkerTown* marker = m != g_markerByTown.end() ? m->second : NULL;
            if (marker == NULL)
            {
                std::map<std::string, MapScreen::MapMarkerTown*>::const_iterator n = g_markerByName.find(info.name);
                marker = n != g_markerByName.end() ? n->second : NULL;
            }
            std::string texture;
            MyGUI::IntCoord coord;
            int texW = 0, texH = 0;
            if (marker != NULL && IconOf(marker->image, texture, coord, texW, texH))
                st.icon = IconFor(texture, coord, texW, texH, MyGUI::Colour::White, g_townIconSize);
        }
        info.icon = g_townGameIcons ? st.icon : NULL;
        g_towns.push_back(info);
    }

    // Все поселения и гнёзда мира - из списка игры (shou->townList): метки
    // большой карты (MapScreen::mapMarkersTowns) заполняются не всегда, и
    // по ним выходило 0 поселений (20.10.2026). Метки карты - запасной путь.
    void CollectTowns(MapScreen* map, const Ogre::Vector3& center, float reach, DWORD now)
    {
        g_towns.clear();
        if (!g_showTowns)
            return;
        Faction* const player = ou->player->getFaction();
        IndexMarkers(map, now);
        std::set<TownBase*> seen;
        if (shou != NULL && shou->townList != NULL)
        {
            const lektor<RootObject*>* const lists[2] = { &shou->townList->things, &shou->townList->nests };
            for (int l = 0; l < 2; ++l)
                for (uint32_t i = 0; i < lists[l]->size(); ++i)
                {
                    RootObject* const o = (*lists[l])[i];
                    if (o == NULL)
                        continue;
                    TownBase* const t = static_cast<TownBase*>(o);
                    if (seen.insert(t).second)
                        AddTown(t, player, center, reach, now);
                }
        }
        if (seen.empty() && map != NULL)
        {
            typedef ogre_unordered_map<hand, MapScreen::MapMarkerTown*>::type Towns;
            const Towns& towns = map->mapMarkersTowns;
            for (Towns::const_iterator it = towns.begin(); it != towns.end(); ++it)
                if (TownBase* const t = it->first.getTown())
                    if (seen.insert(t).second)
                        AddTown(t, player, center, reach, now);
        }
        // В журнал - когда что-то из этого меняется (а не каждую секунду).
        static unsigned lastIcons = ~0u, lastMarkers = ~0u, lastMatched = ~0u;
        if (g_debug)
        {
            unsigned withIcon = 0;
            for (size_t i = 0; i < g_towns.size(); ++i)
                withIcon += g_towns[i].icon != NULL ? 1 : 0;
            const unsigned markers = map == NULL ? 0u : static_cast<unsigned>(map->mapMarkersTowns.size());
            const unsigned matched = static_cast<unsigned>(g_markerByTown.size());
            if ((withIcon > 0) != (lastIcons > 0 && lastIcons != ~0u) || lastIcons == ~0u ||
                markers != lastMarkers || matched != lastMatched)
            {
                char note[300];
                sprintf_s(note, "BestMinimap: %u towns and nests known to the game, %u near the view (%u with a map icon); "
                                "big map markers %u, matched by reference %u",
                          static_cast<unsigned>(seen.size()), static_cast<unsigned>(g_towns.size()), withIcon,
                          markers, matched);
                DebugLog(note);
            }
            lastIcons = withIcon;
            lastMarkers = markers;
            lastMatched = matched;
        }
    }

    // ---- флажок «идти сюда» после ПКМ ----
    Ogre::Vector3 g_movePos;
    DWORD g_moveAt = 0;
    const DWORD kMoveFlagMs = 3000;

    // Метки на экране этой перерисовки - для подсказки при наведении.
    struct ScreenTip { float x, y; const std::string* text; };
    std::vector<ScreenTip> g_pinTips;

    // Поселения стоят на месте: список - раз в секунду, или сразу, если
    // сменился обзор или центр ушёл на четверть обзора (g_townsAt - выше).
    double g_townsRange = -1.0;
    Ogre::Vector3 g_townsCenter;

    // ---- замер времени перерисовки (Debug=1) ----
    // Раз в 5 с в журнал: сколько в среднем занимает каждая часть - чтобы
    // искать тормоза по цифрам.
    enum { ST_TOWNS, ST_CHARS, ST_LABELS, ST_UPLOAD, ST_DRAW, ST_COUNT };
    double g_stageMs[ST_COUNT] = {};
    int g_perfRenders = 0, g_perfDots = 0, g_perfTowns = 0, g_perfLabels = 0;
    DWORD g_perfLogAt = 0;

    struct Stopwatch
    {
        LARGE_INTEGER freq, at;
        Stopwatch() { QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&at); }
        void lap(int stage)
        {
            LARGE_INTEGER t;
            QueryPerformanceCounter(&t);
            g_stageMs[stage] += (t.QuadPart - at.QuadPart) * 1000.0 / freq.QuadPart;
            at = t;
        }
    };

    void LogPerf(DWORD now)
    {
        ++g_perfRenders;
        if (!g_debug)
        {
            g_perfRenders = 0;
            for (int i = 0; i < ST_COUNT; ++i)
                g_stageMs[i] = 0.0;
            return;
        }
        if (g_perfLogAt == 0)
            g_perfLogAt = now;
        if (now - g_perfLogAt < 5000)
            return;
        const double n = g_perfRenders;
        const double game = g_stageMs[ST_TOWNS] + g_stageMs[ST_CHARS] + g_stageMs[ST_LABELS] + g_stageMs[ST_UPLOAD];
        char note[400];
        sprintf_s(note, "BestMinimap: render %.1f/s; game thread %.2f ms avg (towns %.2f [%d], characters %.2f [%d], "
                        "labels %.2f [%d], upload %.2f); drawing thread %.2f ms",
                  n * 1000.0 / (now - g_perfLogAt), game / n, g_stageMs[ST_TOWNS] / n, g_perfTowns,
                  g_stageMs[ST_CHARS] / n, g_perfDots, g_stageMs[ST_LABELS] / n, g_perfLabels,
                  g_stageMs[ST_UPLOAD] / n, g_stageMs[ST_DRAW] / n);
        DebugLog(note);
        g_perfRenders = 0;
        for (int i = 0; i < ST_COUNT; ++i)
            g_stageMs[i] = 0.0;
        g_perfLogAt = now;
    }

    // ---------------------------------------------------------------
    // Поток рисования
    // ---------------------------------------------------------------

    // Пиксели миникарты считает отдельный поток: картинка карты, точки,
    // кольца, носики - миллисекунды работы, и в основном потоке игры они
    // ложились на каждый третий-четвёртый кадр рывком (20.10.2026). Основной
    // поток только собирает кадр (Render: что и где рисовать - списком
    // команд) и потом пишет готовые пиксели в текстуру (UploadJob):
    // текстуру и MyGUI трогать можно только из него.
    //
    // Обмен - через одно задание g_job и его состояние: пока поток рисует,
    // основной поток задание не трогает и нового не собирает.
    enum { OP_DISC, OP_RING, OP_TRI, OP_VIEW, OP_CAMRANGE, OP_ICON };

    struct Op
    {
        int kind;
        float f[6];
        unsigned colour, outline;
        float alpha;
        const Icon* icon;
    };

    struct Job
    {
        int w, h, shape;
        bool rebuildMask;
        View v;
        float alpha;            // непрозрачность карты
        unsigned back;          // цвет за краем картинки
        std::vector<Op> ops;
        std::vector<float> camX, camY;
        float camR;
        bool camDots;
        unsigned camColour;
        bool frame;
        unsigned frameColour;
        double drawMs;          // сколько рисовал поток
    };

    enum { JOB_IDLE, JOB_DRAWING, JOB_DONE };
    Job g_job;
    volatile LONG g_jobState = JOB_IDLE;
    HANDLE g_jobEvent = NULL;
    bool g_threadFailed = false;

    void QDisc(float x, float y, float r, unsigned fill, unsigned outline, float alpha)
    {
        Op op = { OP_DISC, { x, y, r, 0, 0, 0 }, fill, outline, alpha, NULL };
        g_job.ops.push_back(op);
    }

    void QRing(float x, float y, float r, float thick, unsigned colour, float alpha)
    {
        Op op = { OP_RING, { x, y, r, thick, 0, 0 }, colour, 0, alpha, NULL };
        g_job.ops.push_back(op);
    }

    void QTri(float ax, float ay, float bx, float by, float cx, float cy, unsigned colour, float alpha)
    {
        Op op = { OP_TRI, { ax, ay, bx, by, cx, cy }, colour, 0, alpha, NULL };
        g_job.ops.push_back(op);
    }

    void QView(float x, float y, float dirX, float dirY, float len, unsigned colour)
    {
        Op op = { OP_VIEW, { x, y, dirX, dirY, len, 0 }, colour, 0, 1.0f, NULL };
        g_job.ops.push_back(op);
    }

    void QIcon(float x, float y, const Icon* icon)
    {
        Op op = { OP_ICON, { x, y, 0, 0, 0, 0 }, 0, 0, 1.0f, icon };
        g_job.ops.push_back(op);
    }

    // Значок по центру x, y, обрезанный по форме карты.
    void DrawIcon(float cx, float cy, const Icon& icon)
    {
        const int left = static_cast<int>(cx - icon.w * 0.5f + 0.5f), top = static_cast<int>(cy - icon.h * 0.5f + 0.5f);
        for (int y = Hi(0, -top); y < icon.h && top + y < g_rh; ++y)
            for (int x = Hi(0, -left); x < icon.w && left + x < g_rw; ++x)
            {
                const unsigned p = icon.px[static_cast<size_t>(y) * icon.w + x];
                if (ChA(p) == 0)
                    continue;
                const size_t i = static_cast<size_t>(top + y) * g_rw + left + x;
                g_buf[i] = Blend(g_buf[i], p | 0xFF000000u, ChA(p) / 255.0f * (g_mask[i] / 255.0f));
            }
    }

    // Картинка карты под вид v. Точка картинки линейна по x строки,
    // поэтому идём шагом в фиксированной точке 16.16, а смесь 4 соседей -
    // целыми, по два канала за раз.
    void DrawBackground(const Job& j)
    {
        const View& v = j.v;
        const int w = j.w, h = j.h;
        const bool haveSrc = g_src.ok();
        // Уровень картинки под масштаб: 2^level пикселей на пиксель миникарты.
        int level = 0;
        while (level < static_cast<int>(g_src.mips.size()) && v.scale >= 2.0 * (1 << level))
            ++level;
        const unsigned* const lpx = level == 0 ? (g_src.px.empty() ? NULL : &g_src.px[0]) : &g_src.mips[level - 1].px[0];
        const int lw = level == 0 ? v.sw : g_src.mips[level - 1].w;
        const int lh = level == 0 ? v.sh : g_src.mips[level - 1].h;
        const double lk = 1.0 / (1 << level);
        unsigned alphaOf[256];
        for (int m = 0; m < 256; ++m)
            alphaOf[m] = static_cast<unsigned>(m * j.alpha + 0.5f) << 24;
        const unsigned backRgb = j.back & 0x00FFFFFFu;
        const double kx = v.scale * lk * 65536.0;
        const long long stepX = static_cast<long long>(v.c * kx), stepY = static_cast<long long>(v.s * kx);
        const long long maxX = static_cast<long long>(lw - 1) << 16, maxY = static_cast<long long>(lh - 1) << 16;
        const bool sample = haveSrc && lpx != NULL;
        for (int y = 0; y < h; ++y)
        {
            const double dy = (y + 0.5 - v.cy) * v.scale;
            const double dx0 = (0.5 - v.cx) * v.scale;
            long long fx = static_cast<long long>(((v.su + v.c * dx0 - v.s * dy) * lk - 0.5) * 65536.0);
            long long fy = static_cast<long long>(((v.sv + v.s * dx0 + v.c * dy) * lk - 0.5) * 65536.0);
            const unsigned char* const mrow = &g_mask[static_cast<size_t>(y) * w];
            unsigned* const brow = &g_buf[static_cast<size_t>(y) * w];
            for (int x = 0; x < w; ++x, fx += stepX, fy += stepY)
            {
                const unsigned m = mrow[x];
                if (m == 0)
                    continue;
                unsigned c = backRgb;
                if (sample && fx >= 0 && fy >= 0 && fx < maxX && fy < maxY)
                {
                    const int ix = static_cast<int>(fx >> 16), iy = static_cast<int>(fy >> 16);
                    const unsigned tx = static_cast<unsigned>(fx >> 8) & 0xFFu, ty = static_cast<unsigned>(fy >> 8) & 0xFFu;
                    const unsigned* const p = &lpx[static_cast<size_t>(iy) * lw + ix];
                    const unsigned p00 = p[0], p10 = p[1], p01 = p[lw], p11 = p[lw + 1];
                    // Веса в сумме ровно 256.
                    const unsigned w11 = (tx * ty) >> 8, w10 = tx - w11, w01 = ty - w11, w00 = 256 - tx - ty + w11;
                    const unsigned rb = ((p00 & 0x00FF00FFu) * w00 + (p10 & 0x00FF00FFu) * w10 +
                                         (p01 & 0x00FF00FFu) * w01 + (p11 & 0x00FF00FFu) * w11) >> 8;
                    const unsigned g = ((p00 & 0x0000FF00u) * w00 + (p10 & 0x0000FF00u) * w10 +
                                        (p01 & 0x0000FF00u) * w01 + (p11 & 0x0000FF00u) * w11) >> 8;
                    c = (rb & 0x00FF00FFu) | (g & 0x0000FF00u);
                }
                brow[x] = c | alphaOf[m];
            }
        }
    }

    void DrawJob(Job& j)
    {
        LARGE_INTEGER freq, t0, t1;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&t0);
        g_rw = j.w;
        g_rh = j.h;
        const size_t n = static_cast<size_t>(j.w) * j.h;
        if (j.rebuildMask || g_rshape != j.shape || g_mask.size() != n || g_frameW != j.w)
        {
            g_rshape = j.shape;
            g_frameW = j.w;
            BuildMask(j.w, j.h);
        }
        g_buf.assign(n, 0);
        DrawBackground(j);
        for (size_t i = 0; i < j.ops.size(); ++i)
        {
            const Op& o = j.ops[i];
            switch (o.kind)
            {
            case OP_DISC: DrawDisc(o.f[0], o.f[1], o.f[2], o.colour, o.outline, o.alpha); break;
            case OP_RING: DrawRing(o.f[0], o.f[1], o.f[2], o.f[3], o.colour, o.alpha); break;
            case OP_TRI: DrawTri(o.f[0], o.f[1], o.f[2], o.f[3], o.f[4], o.f[5], o.colour, o.alpha); break;
            case OP_VIEW: DrawView(o.f[0], o.f[1], o.f[2], o.f[3], o.f[4], o.colour); break;
            case OP_CAMRANGE: DrawCameraRange(j.camX, j.camY, j.camR, j.camDots, j.camColour); break;
            case OP_ICON: DrawIcon(o.f[0], o.f[1], *o.icon); break;
            }
        }
        if (j.frame)
            for (size_t k = 0; k < g_frameCov.size(); ++k)
            {
                const unsigned i = g_frameCov[k].index;
                const unsigned c = Blend(g_buf[i], j.frameColour, g_frameCov[k].alpha);
                g_buf[i] = (c & 0x00FFFFFFu) | (static_cast<unsigned>(g_mask[i]) << 24);
            }
        QueryPerformanceCounter(&t1);
        j.drawMs = (t1.QuadPart - t0.QuadPart) * 1000.0 / freq.QuadPart;
    }

    DWORD WINAPI DrawThread(LPVOID)
    {
        for (;;)
        {
            WaitForSingleObject(g_jobEvent, INFINITE);
            if (g_jobState != JOB_DRAWING)
                continue;
            try
            {
                DrawJob(g_job);
            }
            catch (...)
            {
                g_job.w = 0;    // не вышло (нет памяти) - кадр пропустим
            }
            InterlockedExchange(&g_jobState, JOB_DONE);
        }
    }

    // Отдать задание потоку. Поток не запустился - рисуем сами, как раньше.
    void SubmitJob()
    {
        if (g_jobEvent == NULL && !g_threadFailed)
        {
            g_jobEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
            HANDLE const thread = g_jobEvent == NULL ? NULL : CreateThread(NULL, 0, &DrawThread, NULL, 0, NULL);
            if (thread == NULL)
            {
                g_threadFailed = true;
                ErrorLog("BestMinimap: could not start the drawing thread - drawing in the game thread");
            }
            else
                CloseHandle(thread);
        }
        if (g_threadFailed)
        {
            DrawJob(g_job);
            InterlockedExchange(&g_jobState, JOB_DONE);
            return;
        }
        InterlockedExchange(&g_jobState, JOB_DRAWING);
        SetEvent(g_jobEvent);
    }

    // Готовый кадр - в текстуру (основной поток, каждый кадр игры).
    void UploadJob()
    {
        if (g_jobState != JOB_DONE)
            return;
        Stopwatch watch;
        const Job& j = g_job;
        g_stageMs[ST_DRAW] += j.drawMs;
        // Миникарту за это время переделали (размер, форма) - кадр устарел.
        if (j.w > 0 && j.w == g_viewW && j.h == g_viewH && j.shape == g_shape && g_tex != NULL &&
            j.w <= g_texW && j.h <= g_texH)
        {
            void* const data = g_tex->lock(MyGUI::TextureUsage::Write);
            if (data != NULL)
            {
                unsigned* const dst = static_cast<unsigned*>(data);
                for (int y = 0; y < j.h; ++y)
                {
                    unsigned* const row = dst + static_cast<size_t>(y) * g_texW;
                    const unsigned* const src = &g_buf[static_cast<size_t>(y) * j.w];
                    if (g_swapRB)
                        for (int x = 0; x < j.w; ++x)
                        {
                            const unsigned c = src[x];
                            row[x] = (c & 0xFF00FF00u) | ((c >> 16) & 0xFFu) | ((c & 0xFFu) << 16);
                        }
                    else
                        memcpy(row, src, static_cast<size_t>(j.w) * sizeof(unsigned));
                }
                g_tex->unlock();
            }
        }
        watch.lap(ST_UPLOAD);
        InterlockedExchange(&g_jobState, JOB_IDLE);
    }

    // Собрать кадр для потока рисования: вид, что и где рисовать (списком
    // команд), подписи и подсказку - сразу. Здесь только то, что требует
    // игры и MyGUI; пиксели считает поток. false - поток ещё занят.
    bool Render(const Ogre::Vector3& center, DWORD now)
    {
        Stopwatch watch;
        const int w = g_viewW, h = g_viewH;
        if (w <= 0 || h <= 0 || g_tex == NULL)
            return true;
        if (g_jobState != JOB_IDLE)
            return false;
        Job& job = g_job;
        job.w = w;
        job.h = h;
        job.shape = g_shape;
        job.rebuildMask = g_dirtyMask;
        g_dirtyMask = false;
        job.ops.clear();
        job.camX.clear();
        job.camY.clear();
        job.frame = false;

        // --- вид: центр, масштаб, поворот ---
        View v;
        v.sw = Hi(1, g_src.w);
        v.sh = Hi(1, g_src.h);
        double cu = 0.5, cv = 0.5;
        g_map.apply(center.x, center.z, cu, cv);
        v.su = cu * v.sw;
        v.sv = cv * v.sh;
        // Пикселей картинки на метр - по оси x мира.
        const double pxPerMetre = std::sqrt((g_map.au * v.sw) * (g_map.au * v.sw) + (g_map.av * v.sh) * (g_map.av * v.sh));
        v.scale = Hi(1e-6, pxPerMetre * g_range / (Lo(w, h) * 0.5));
        v.c = 1.0;
        v.s = 0.0;
        // Направление камеры в пикселях картинки.
        double fdx = 0.0, fdy = -1.0;
        if (CameraClass* const cam = ou->player->camera)
            if (cam->camera != NULL)
            {
                const Ogre::Vector3 dir = cam->camera->getDerivedDirection();
                const double ex = g_map.au * v.sw * dir.x + g_map.bu * v.sw * dir.z;
                const double ey = g_map.av * v.sh * dir.x + g_map.bv * v.sh * dir.z;
                const double len = std::sqrt(ex * ex + ey * ey);
                if (len > 1e-9)
                {
                    fdx = ex / len;
                    fdy = ey / len;
                }
            }
        if (g_rotate)
        {
            // Экран «вверх» (0,-1) должен стать направлением камеры:
            // R*(0,-1) = (s, -c) = (fdx, fdy).
            v.s = fdx;
            v.c = -fdy;
        }
        v.cx = w * 0.5f;
        v.cy = h * 0.5f;

        // --- фон: картинку карты рисует поток (DrawBackground) ---
        job.v = v;
        job.alpha = g_opacity / 10.0f;
        job.back = g_backColour;

        const float frameW = g_frame == FRAME_LINE ? 2.0f : 0.0f;
        const unsigned dark = 0xFF101010u;

        g_labelsUsed = 0;
        g_labelRects.clear();
        const float pxPerMetreScreen = static_cast<float>(pxPerMetre / v.scale);

        // --- поселения: круг радиуса и значок ---
        const float townsMoved = std::sqrt((center.x - g_townsCenter.x) * (center.x - g_townsCenter.x) +
                                           (center.z - g_townsCenter.z) * (center.z - g_townsCenter.z));
        if (now - g_townsAt >= 1000 || g_townsRange != g_range || townsMoved > g_range * 0.25)
        {
            g_townsAt = now;
            g_townsRange = g_range;
            g_townsCenter = center;
            CollectTowns(GetMapScreen(), center, static_cast<float>(g_range * 2.1), now);
        }
        for (size_t i = 0; i < g_towns.size(); ++i)
        {
            const TownInfo& t = g_towns[i];
            double u, vv;
            g_map.apply(t.pos.x, t.pos.z, u, vv);
            float x, y;
            ToScreen(v, u, vv, x, y);
            const float rr = t.radius * pxPerMetreScreen;
            if (g_townRadius && rr >= 6.0f)
                QRing(x, y, rr, 1.2f, 0xFFFFFFFFu, 0.55f);
            // Значок большой карты - виджетом поверх; нет его - ромб с
            // тёмной обводкой.
            if (t.icon != NULL)
            {
                QIcon(x, y, t.icon);
                continue;
            }
            const float k = Hi(3.5f, g_dot * 0.8f);
            QTri(x, y - k - 1.2f, x + k + 1.2f, y, x, y + k + 1.2f, 0xFF101010u, 0.9f);
            QTri(x, y - k - 1.2f, x - k - 1.2f, y, x, y + k + 1.2f, 0xFF101010u, 0.9f);
            QTri(x, y - k, x + k, y, x, y + k, t.colour, 1.0f);
            QTri(x, y - k, x - k, y, x, y + k, t.colour, 1.0f);
        }
        watch.lap(ST_TOWNS);

        // --- метки Map-markers и WantedMap ---
        g_pinTips.clear();
        const float pinR = Hi(2.5f, g_dot * 0.7f);
        for (int list = 0; list < 2; ++list)
        {
            const std::vector<Pin>& pins = list == 0 ? g_mapPins : g_wantedPins;
            if ((list == 0 && !g_mapMarkers) || (list == 1 && !g_wanted))
                continue;
            for (size_t i = 0; i < pins.size(); ++i)
            {
                double u = pins[i].u, vv = pins[i].v;
                if (pins[i].world)
                    g_map.apply(pins[i].x, pins[i].z, u, vv);
                float x, y;
                ToScreen(v, u, vv, x, y);
                if (!PlacePin(x, y, frameW + pinR + 1.0f))
                    continue;
                if (!pins[i].tip.empty())
                {
                    ScreenTip t = { x, y, &pins[i].tip };
                    g_pinTips.push_back(t);
                }
                // Цель WantedMap - ромбом не рисуем, а двойным кольцом: так её
                // не спутать с персонажем и с меткой игрока.
                if (list == 1)
                    QDisc(x, y, pinR + 2.0f, dark, dark, 1.0f);
                QDisc(x, y, pinR + (list == 1 ? 1.0f : 0.0f), pins[i].colour, dark, 1.0f);
            }
        }

        // --- граница полёта камеры ---
        g_lastView = v;
        g_haveView = true;
        const float camR = CameraRange() * pxPerMetreScreen;
        if (g_camRange && CameraRange() >= 20.0f && camR >= 3.0f)
        {
            const lektor<Character*>& mine = ou->player->playerCharacters;
            for (uint32_t i = 0; i < mine.size(); ++i)
                if (mine[i] != NULL && !mine[i]->isDead())
                {
                    const Ogre::Vector3 p = mine[i]->getPosition();
                    double u, vv;
                    g_map.apply(p.x, p.z, u, vv);
                    float x, y;
                    ToScreen(v, u, vv, x, y);
                    job.camX.push_back(x);
                    job.camY.push_back(y);
                }
            job.camR = camR;
            job.camDots = g_camRangeDots;
            job.camColour = g_camRangeColour;
            Op op = {};
            op.kind = OP_CAMRANGE;
            job.ops.push_back(op);
        }

        // --- персонажи ---
        const float reach = static_cast<float>(g_range * (g_shape == SHAPE_RECT ? 1.8 : 1.5));
        CollectDots(g_dots, center, reach, now);
        // Сначала все остальные, последними - выделенные: они не должны
        // теряться в толпе.
        for (int pass = 0; pass < 2; ++pass)
            for (size_t i = 0; i < g_dots.size(); ++i)
            {
                const Dot& d = g_dots[i];
                if (d.selected != (pass == 1))
                    continue;
                double u, vv;
                g_map.apply(d.pos.x, d.pos.z, u, vv);
                float x, y;
                ToScreen(v, u, vv, x, y);
                if (x < -10 || y < -10 || x > w + 10 || y > h + 10)
                    continue;
                const float r = g_dot * 0.5f + (d.selected ? 1.0f : 0.0f);
                const unsigned outline = d.selected ? 0xFFFFFFFFu : dark;
                // Носик: куда смотрит. Направление мира -> экран тем же
                // поворотом, что и карта.
                if (g_showFacing && !d.dead && (d.fx != 0.0f || d.fz != 0.0f))
                {
                    const double ex = g_map.au * v.sw * d.fx + g_map.bu * v.sw * d.fz;
                    const double ey = g_map.av * v.sh * d.fx + g_map.bv * v.sh * d.fz;
                    float nx = static_cast<float>(v.c * ex + v.s * ey);
                    float ny = static_cast<float>(-v.s * ex + v.c * ey);
                    const float nl = std::sqrt(nx * nx + ny * ny);
                    if (nl > 1e-6f)
                    {
                        nx /= nl;
                        ny /= nl;
                        const float tip = r + 3.5f, half = r * 0.75f + 0.5f;
                        QTri(x + nx * (tip + 1.0f), y + ny * (tip + 1.0f), x - ny * (half + 1.0f), y + nx * (half + 1.0f),
                                x + ny * (half + 1.0f), y - nx * (half + 1.0f), dark, 0.9f);
                        QTri(x + nx * tip, y + ny * tip, x - ny * half, y + nx * half, x + ny * half, y - nx * half, d.colour, 1.0f);
                    }
                }
                QDisc(x, y, r, d.colour, outline, d.dead ? 0.45f : 1.0f);
                if (g_showBounty && d.bounty > 0)
                    QRing(x, y, r + 2.5f, 1.5f, g_bountyColour, 1.0f);
            }

        // --- тревога: наших атакуют ---
        // Пульсирующее кольцо на месте атаки; за краем - у края, всегда
        // (это важнее настройки PinToEdge).
        const float pulse = 0.5f + 0.5f * std::sin(now * 0.008f);
        if (g_alert)
            for (std::map<Character*, Alert>::const_iterator it = g_alerts.begin(); it != g_alerts.end(); ++it)
            {
                double u, vv;
                g_map.apply(it->second.pos.x, it->second.pos.z, u, vv);
                float x, y;
                ToScreen(v, u, vv, x, y);
                const float r = g_dot * 0.8f + 4.0f + 3.0f * pulse;
                PlacePin(x, y, frameW + r + 2.0f, true);
                QRing(x, y, r, 2.2f, dark, 0.8f);
                QRing(x, y, r, 1.6f, g_alertColour, 1.0f);
            }

        // --- флажок «идти сюда» ---
        if (g_moveAt != 0 && now - g_moveAt < kMoveFlagMs)
        {
            double u, vv;
            g_map.apply(g_movePos.x, g_movePos.z, u, vv);
            float x, y;
            ToScreen(v, u, vv, x, y);
            const float fade = 1.0f - static_cast<float>(now - g_moveAt) / kMoveFlagMs;
            QRing(x, y, 5.0f + 3.0f * (1.0f - fade), 1.6f, 0xFF3CC85Au, fade);
            QDisc(x, y, 3.0f, 0xFF3CC85Au, dark, Hi(0.3f, fade));
        }

        // --- куда смотрит камера ---
        if (g_showView)
        {
            float dirX = static_cast<float>(fdx), dirY = static_cast<float>(fdy);
            if (g_rotate)
            {
                dirX = 0.0f;
                dirY = -1.0f;
            }
            QView(v.cx, v.cy, dirX, dirY, Hi(8.0f, g_dot * 2.2f), g_viewColour);
        }

        // --- рамка ---
        // Пока идёт тревога - рамка мигает её цветом (и без рамки тоже).
        const bool flash = g_alert && g_alertFlash && !g_alerts.empty();
        if (flash || g_frame == FRAME_LINE || (g_frame == FRAME_GAME && g_shape == SHAPE_CIRCLE))
        {
            unsigned fc = g_frameColour;
            if (g_frameAuto)
                fc = FromColour(GameTheme::Colour("Main", "#AFA68B"));
            if (flash)
                fc = Blend(fc | 0xFF000000u, g_alertColour, 0.35f + 0.65f * pulse);
            job.frame = true;
            job.frameColour = fc;
        }
        watch.lap(ST_CHARS);

        // --- подписи (виджеты MyGUI поверх картинки) ---
        // Поселения - пока обзор не шире TownNamesRange: на всей карте их
        // сотни, и подписи слились бы в кашу.
        // Налезающая подпись не ставится, поэтому порядок - по важности:
        // награды (раньше их съедали названия отрядов над теми же
        // бандитами), поселения, отряды, имена.
        static std::vector<float> dotX, dotY;      // без выделения памяти каждую перерисовку
        dotX.resize(g_dots.size());
        dotY.resize(g_dots.size());
        for (size_t i = 0; i < g_dots.size(); ++i)
        {
            double u, vv;
            g_map.apply(g_dots[i].pos.x, g_dots[i].pos.z, u, vv);
            ToScreen(v, u, vv, dotX[i], dotY[i]);
        }
        const float labelR = g_dot * 0.5f + 2.0f;
        if (g_showBounty)
            for (size_t i = 0; i < g_dots.size(); ++i)
                if (g_dots[i].bounty > 0)
                {
                    char text[32];
                    sprintf_s(text, "%d c", g_dots[i].bounty);
                    PlaceLabel(text, dotX[i] + labelR + 4.0f, dotY[i] + 2.0f, g_bountyColour, 0, true);
                }
        if (g_showTowns && g_townNames && g_range <= g_townNamesRange)
            for (size_t i = 0; i < g_towns.size(); ++i)
            {
                double u, vv;
                g_map.apply(g_towns[i].pos.x, g_towns[i].pos.z, u, vv);
                float x, y;
                ToScreen(v, u, vv, x, y);
                // Над значком: у значка игры - его половина высоты.
                const float above = g_towns[i].icon != NULL ? g_towns[i].icon->h * 0.5f + 1.0f : 5.0f;
                PlaceLabel(g_towns[i].name, x, y - above, g_towns[i].colour, g_townFontSize);
            }
        for (size_t i = 0; i < g_dots.size(); ++i)
        {
            const Dot& d = g_dots[i];
            // Подпись отряда - над его лидером.
            const bool squadOk = g_squadLabels == 2 || (g_squadLabels == 1 && d.cat != CAT_SQUAD);
            if (squadOk && d.leader && d.platoon != NULL && !d.dead)
                PlaceLabel(d.platoon->getName(), dotX[i], dotY[i] - labelR, g_squadLabelColour);
        }
        if (g_charNames)
            for (size_t i = 0; i < g_dots.size(); ++i)
                if (!g_dots[i].dead)
                    PlaceLabel(*g_dots[i].name, dotX[i], dotY[i] + labelR + 14.0f, g_dots[i].colour);
        HideUnusedLabels();

        // Подсказка: ближайшее к мыши поселение или персонаж.
        bool tip = false;
        if (g_tooltips)
            if (MyGUI::Widget* const img = g_imageRef.get())
            {
                const MyGUI::IntPoint m = MyGUI::InputManager::getInstance().getMousePosition();
                const MyGUI::IntPoint a = img->getAbsolutePosition();
                const float mx = static_cast<float>(m.left - a.left), my = static_cast<float>(m.top - a.top);
                if (mx >= 0 && my >= 0 && mx < w && my < h && InsideShape(mx, my, w, h, 0.0f) > 0.5f)
                {
                    float best = 81.0f;
                    std::string text;
                    for (size_t i = 0; i < g_towns.size(); ++i)
                    {
                        double u, vv;
                        g_map.apply(g_towns[i].pos.x, g_towns[i].pos.z, u, vv);
                        float x, y;
                        ToScreen(v, u, vv, x, y);
                        const float d2 = (x - mx) * (x - mx) + (y - my) * (y - my);
                        if (d2 < best)
                        {
                            best = d2;
                            text = g_towns[i].tip;
                        }
                    }
                    // Метки Map-markers и WantedMap - там, где нарисованы
                    // (у края - тоже).
                    for (size_t i = 0; i < g_pinTips.size(); ++i)
                    {
                        const float x = g_pinTips[i].x, y = g_pinTips[i].y;
                        const float d2 = (x - mx) * (x - mx) + (y - my) * (y - my);
                        if (d2 < best && d2 < 64.0f)
                        {
                            best = d2;
                            text = *g_pinTips[i].text;
                        }
                    }
                    for (size_t i = 0; i < g_dots.size(); ++i)
                    {
                        double u, vv;
                        g_map.apply(g_dots[i].pos.x, g_dots[i].pos.z, u, vv);
                        float x, y;
                        ToScreen(v, u, vv, x, y);
                        const float d2 = (x - mx) * (x - mx) + (y - my) * (y - my);
                        if (d2 < best && d2 < 49.0f)
                        {
                            best = d2;
                            text = *g_dots[i].name;
                            if (g_dots[i].platoon != NULL && !g_dots[i].platoon->getName().empty())
                                text += "\n" + g_dots[i].platoon->getName();
                            if (g_dots[i].bounty > 0)
                            {
                                char b[32];
                                sprintf_s(b, "\n%d c", g_dots[i].bounty);
                                text += b;
                            }
                        }
                    }
                    if (!text.empty())
                    {
                        ShowTooltip(text);
                        tip = true;
                    }
                }
            }
        if (!tip)
            HideTooltip();
        watch.lap(ST_LABELS);
        g_perfDots = static_cast<int>(g_dots.size());
        g_perfTowns = static_cast<int>(g_towns.size());
        g_perfLabels = g_labelsUsed;

        // --- отдать потоку ---
        SubmitJob();
        LogPerf(now);
        return true;
    }

    // ПКМ по миникарте - выделенные идут в эту точку, как по ПКМ по земле
    // (PlayerInterface::playerMove). Точку мира считаем обратным нашим же
    // преобразованием: mapCoordsToWorld игры принимает целые пиксели большой
    // карты, а на её мелком масштабе пиксель - это больше сотни метров.
    // Высота - по земле (UtilityT::getTerrainHeight), если там она есть.
    // Точка мира под мышью на миникарте; false - вида ещё нет.
    bool MouseWorldPoint(Ogre::Vector3& pos)
    {
        if (!g_haveView || !g_map.ok || ou == NULL || ou->player == NULL)
            return false;
        MyGUI::Widget* const img = g_imageRef.get();
        if (img == NULL)
            return false;
        const MyGUI::IntPoint m = MyGUI::InputManager::getInstance().getMousePosition();
        const MyGUI::IntPoint a = img->getAbsolutePosition();
        const View& v = g_lastView;
        const double dx = (m.left - a.left + 0.5 - v.cx) * v.scale;
        const double dy = (m.top - a.top + 0.5 - v.cy) * v.scale;
        const double u = (v.su + v.c * dx - v.s * dy) / v.sw;
        const double vv = (v.sv + v.s * dx + v.c * dy) / v.sh;
        const Affine& f = g_map;
        const double det = f.au * f.bv - f.bu * f.av;
        if (std::fabs(det) < 1e-18)
            return false;
        pos.x = static_cast<float>((f.bv * (u - f.cu) - f.bu * (vv - f.cv)) / det);
        pos.z = static_cast<float>((-f.av * (u - f.cu) + f.au * (vv - f.cv)) / det);
        pos.y = ou->getCameraCenter().y;
        const float ground = UtilityT::getTerrainHeight(pos.x, pos.z);
        if (ground == ground && ground > -100000.0f && ground < 100000.0f)
            pos.y = ground;
        return true;
    }

    void MoveSelectedTo()
    {
        Ogre::Vector3 pos;
        if (!g_rightClickMove || !MouseWorldPoint(pos))
            return;
        ou->player->playerMove(pos, NULL);
        g_movePos = pos;
        g_moveAt = GetTickCount();
        g_lastRender = 0;
        if (g_debug)
        {
            char note[128];
            sprintf_s(note, "BestMinimap: move order to %.0f %.0f %.0f", pos.x, pos.y, pos.z);
            DebugLog(note);
        }
    }

    // Двойной щелчок по миникарте (21.10.2026, просьба пользователя) - камера в точку
    // под мышью, если до неё дотягивается: игра держит камеру не дальше
    // предела от своих персонажей (CameraRange, тот же, что линия на
    // миникарте). Дальше - камеру не трогаем: игра всё равно вернула бы её
    // к краю. Предел ещё не известен (0) или свободная камера - без
    // проверки. Камера, следящая за персонажем, перестаёт следить - иначе
    // она тут же вернулась бы к нему.
    void CameraTo()
    {
        Ogre::Vector3 pos;
        if (!g_dblClickCamera || !MouseWorldPoint(pos))
            return;
        CameraClass* const cam = ou->player->camera;
        if (cam == NULL)
            return;
        const float range = CameraRange();
        if (!cam->isFreeCameraMode() && range >= 20.0f)
        {
            const lektor<Character*>& mine = ou->player->playerCharacters;
            float best = -1.0f;
            for (uint32_t i = 0; i < mine.size(); ++i)
            {
                Character* const ch = mine[i];
                if (ch == NULL || ch->isDead())
                    continue;
                const Ogre::Vector3 p = ch->getPosition();
                const float d = std::sqrt((p.x - pos.x) * (p.x - pos.x) + (p.z - pos.z) * (p.z - pos.z));
                if (best < 0.0f || d < best)
                    best = d;
            }
            if (best > range)
            {
                if (g_debug)
                {
                    char note[128];
                    sprintf_s(note, "BestMinimap: camera spot %.0f m from own people, range %.0f - not moved", best, range);
                    DebugLog(note);
                }
                return;
            }
        }
        if (!cam->getFollowObject().isNull())
            cam->stopFollowing();
        cam->teleport(pos);
        g_lastRender = 0;
        if (g_debug)
        {
            char note[128];
            sprintf_s(note, "BestMinimap: camera to %.0f %.0f %.0f", pos.x, pos.y, pos.z);
            DebugLog(note);
        }
    }


    // ---------------------------------------------------------------
    // Клавиши
    // ---------------------------------------------------------------

    bool Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

    // Ровно это сочетание: «=» не срабатывает, когда зажат CTRL (это уже
    // «CTRL+=» - другая клавиша миникарты).
    bool Exact(const HoldKey::Key& k)
    {
        if (k.vk == 0 || !HoldKey::Held(k))
            return false;
        if (!k.ctrl && k.vk != VK_CONTROL && Down(VK_CONTROL))
            return false;
        if (!k.shift && k.vk != VK_SHIFT && Down(VK_SHIFT))
            return false;
        if (!k.alt && k.vk != VK_MENU && Down(VK_MENU))
            return false;
        return true;
    }

    struct Edge
    {
        bool was;
        DWORD repeatAt;
        Edge() : was(false), repeatAt(0) {}
        // Нажатие; при удержании - повтор каждые 120 мс после 400 мс.
        bool fire(bool down, DWORD now, bool repeat)
        {
            bool result = false;
            if (down && !was)
            {
                result = true;
                repeatAt = now + 400;
            }
            else if (down && repeat && now >= repeatAt)
            {
                result = true;
                repeatAt = now + 120;
            }
            was = down;
            return result;
        }
    };

    Edge g_eToggle, g_eZoomIn, g_eZoomOut, g_eBigger, g_eSmaller;

    void PollKeys(DWORD now)
    {
        // Печатает игрок в поле - клавиши не наши.
        const bool typing = MyGUI::InputManager::getInstance().getKeyFocusWidget() != NULL;
        if (g_eToggle.fire(!typing && Exact(g_keyToggle), now, false))
        {
            g_visible = !g_visible;
            WriteInt("Visible", g_visible ? 1 : 0);
        }
        if (!g_visible)
            return;
        if (g_eZoomIn.fire(!typing && Exact(g_keyZoomIn), now, true))
            Zoom(1.0f / kZoomStep);
        if (g_eZoomOut.fire(!typing && Exact(g_keyZoomOut), now, true))
            Zoom(kZoomStep);
        if (g_eBigger.fire(!typing && Exact(g_keyBigger), now, true))
            Resize(kSizeStep);
        if (g_eSmaller.fire(!typing && Exact(g_keySmaller), now, true))
            Resize(-kSizeStep);
    }


    // ---------------------------------------------------------------
    // Кадр
    // ---------------------------------------------------------------

    Ogre::Vector3 CenterPoint()
    {
        if (g_centerOnSelected)
        {
            // Центр выделенных.
            std::vector<Character*> selected;
            SelectedCharacters(selected);
            Ogre::Vector3 sum(0, 0, 0);
            for (size_t i = 0; i < selected.size(); ++i)
                sum += selected[i]->getPosition();
            if (!selected.empty())
                return sum / static_cast<float>(selected.size());
        }
        return ou->getCameraCenter();
    }

    // Почему миникарта сейчас скрыта - в журнал при каждой смене причины
    // (не каждый кадр). Без этого «не появилась» нечем разбирать: первая
    // версия молча пряталась, потому что MapScreen::getVisible() отвечает
    // «видна» и при закрытом окне карты (20.10.2026).
    std::string g_reason = "start";

    void Reason(const std::string& why)
    {
        if (why == g_reason)
            return;
        g_reason = why;
        DebugLog(why.empty() ? std::string("BestMinimap: shown") : "BestMinimap: hidden - " + why);
    }

    void Hide(const std::string& why)
    {
        Show(false);
        Reason(why);
    }

    // Открыта ли большая карта. MapScreen::getVisible() не годится: виджет
    // вкладки карты остаётся «видимым» и в закрытом окне управления -
    // смотрим видимость с учётом всех родителей.
    bool BigMapOpen(MapScreen* map)
    {
        return map->mainWidget != NULL && map->mainWidget->getInheritedVisible();
    }

    // Спрятан ли интерфейс игры (F7). Как в LiveXpBars: признаков два -
    // флаг ForgottenGUI::visible и видимость главной панели, - какой из них
    // переключает F7, по заголовкам не видно. Спрятан любой - прячемся.
    bool GameGuiHidden()
    {
        if (gui == NULL)
            return false;
        MyGUI::Widget* const bar = gui->mainbar != NULL ? gui->mainbar->mMainWidget : NULL;
        return !gui->visible || (bar != NULL && !bar->getVisible());
    }

    void Tick()
    {
        const DWORD now = GetTickCount();
        FlushWritesIfQuiet(now);
        if (!g_enabled)
            return Hide("disabled (Enabled=0)");
        if (ou == NULL || ou->player == NULL)
            return Hide("no game world");
        PollKeys(now);
        // Тревога и учёт предела камеры - и при скрытой миникарте: сообщение
        // игры об атаке нужно и без неё.
        ScanAttacks(now);
        LearnCameraRange(now);
        if (!g_visible)
            return Hide("switched off by the key (Visible=0)");
        MapScreen* const map = GetMapScreen();
        if (map == NULL || map->mapImage == NULL)
            return Hide("the game has no map screen yet");
        if (BigMapOpen(map))
            return Hide("the big map is open");
        if (GameGuiHidden())
            return Hide("the game interface is hidden (F7)");

        const std::string& texture = map->mapImage->_getTextureName();
        if (texture != g_src.texture && texture != g_srcFailed)
        {
            // Картинку карты читает поток рисования - меняем, только когда
            // он свободен.
            if (g_jobState == JOB_DRAWING)
                return;
            LoadSource(texture);
            g_map = Affine();
        }

        const Ogre::Vector3 center = CenterPoint();
        if (!g_map.ok || g_mapApprox)
        {
            // Точная калибровка - когда игра выставит размер большой карты
            // (у некоторых - только после первого её открытия). До тех пор -
            // примерная, по worldBounds; пробуем точную раз в секунду.
            if (now - g_lastCalibration >= 1000)
            {
                g_lastCalibration = now;
                if (Calibrate(map, center))
                    g_mapApprox = false;
                else if (!g_map.ok)
                    g_mapApprox = ApproximateFromBounds(map);
            }
            if (!g_map.ok)
            {
                char why[200];
                sprintf_s(why, "map placement unknown yet (map image %dx%d, worldBounds %.0f %.0f %.0f %.0f)",
                          map->mapImage->getWidth(), map->mapImage->getHeight(), map->worldBounds.x,
                          map->worldBounds.y, map->worldBounds.z, map->worldBounds.w);
                return Hide(why);
            }
        }

        if (!EnsureWidgets())
            return Hide("could not create the minimap widgets or texture");
        if (g_hidden)
            Show(true);
        Reason("");

        UploadJob();
        const DWORD period = 1000 / static_cast<DWORD>(g_fps);
        if (g_lastRender != 0 && now - g_lastRender < period)
            return;
        if (g_jobState != JOB_IDLE)
            return;             // поток ещё рисует прошлый кадр
        g_lastRender = now;
        RefreshMapMarkers(now);
        RefreshWanted(now);
        Render(center, now);
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void (*g_origMainLoop)(GameWorld*, float) = NULL;
    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;

    void MainLoop_hook(GameWorld* self, float time)
    {
        g_origMainLoop(self, time);
        g_lastWorldFrame = GetTickCount();
        Tick();
    }

    // В главном меню и на загрузке цикл мира не идёт - миникарту прячем,
    // а калибровку забываем: в другом сейве (или после смены модов) карта
    // может быть другой.
    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        if (!g_hidden && GetTickCount() - g_lastWorldFrame > 300)
        {
            Hide("the game loop stopped (main menu or loading)");
            FlushWrites();
            // Другой сейв - другие персонажи, поселения и метки большой
            // карты: всё, что держит их указатели, - забыть. Иначе после
            // загрузки значки поселений брались бы из уже удалённых меток.
            g_map = Affine();
            g_dots.clear();
            g_catCache.clear();
            g_alerts.clear();
            g_alertNames.clear();
            g_towns.clear();
            g_townStatic.clear();
            g_markerByTown.clear();
            g_markerByName.clear();
            g_markersBuiltFor = NULL;
            g_markersIndexed = 0;
            g_markersIndexedAt = 0;
            g_townsAt = 0;
            g_mapPinsFile.clear();
        }
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "BestMinimap", "Best Minimap", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("A minimap in the corner of the screen: the game's map picture, characters nearby, Map-markers and WantedMap markers."));

    api->section(api, Tr("General"));
    api->toggle(api, kSec, "Enabled", Tr("Enabled"), Tr("Off - no minimap at all."), 1, 0);
    api->hotkey(api, kSec, "KeyToggle", Tr("Show / hide key"), Tr("Shows or hides the minimap."), "CTRL+M", 0);
    api->toggle(api, kSec, "Visible", Tr("Shown"), Tr("The same switch as the show / hide key."), 1, 0);

    api->section(api, Tr("Look"));
    static const char* const shapes[] = { "circle", "square", "rect" };
    static const char* shapeLabels[3];
    shapeLabels[0] = Tr("Circle");
    shapeLabels[1] = Tr("Square");
    shapeLabels[2] = Tr("Rectangle");
    api->choice(api, kSec, "Shape", Tr("Shape"), Tr("The rectangle is 16:10, its width is the Size."), "circle", shapes, shapeLabels, 3, 0);
    api->integer(api, kSec, "Size", Tr("Size, px"), Tr("Diameter of the circle, side of the square, width of the rectangle. CTRL+wheel over the minimap changes it too."), 220, kSizeMin, kSizeMax, 0);
    static const char* const frames[] = { "none", "line", "game" };
    static const char* frameLabels[3];
    frameLabels[0] = Tr("None");
    frameLabels[1] = Tr("Line");
    frameLabels[2] = Tr("Game panel");
    api->choice(api, kSec, "Frame", Tr("Frame"), Tr("Game panel - the skin of the game's own panels (and of interface mods); the circle gets a line in the colour of the game palette."), "line", frames, frameLabels, 3, 0);
    api->colour(api, kSec, "FrameColour", Tr("Frame colour"), Tr("auto - from the palette of the game (and of interface mods)."), "auto", 0);
    api->colour(api, kSec, "BackgroundColour", Tr("Background"), Tr("Colour where the map picture ends."), "#1E1C19", 0);
    api->integer(api, kSec, "Opacity", Tr("Opacity"), Tr("1-10 (10% steps). Only the map: the markers stay opaque."), 10, 1, 10, 0);
    api->integer(api, kSec, "DotSize", Tr("Marker size, px"), NULL, 5, 2, 16, 0);
    api->toggle(api, kSec, "Locked", Tr("Lock position"), Tr("Off - drag the minimap with the left mouse button."), 0, 0);

    api->section(api, Tr("Map"));
    api->integer(api, kSec, "Range", Tr("Range, m"), Tr("From the centre to the edge. The wheel over the minimap and the zoom keys change it too. The map picture of the game has about 70 m in a pixel, so below about 1500 m it gets blurry."), 2000, static_cast<int>(kRangeMin), static_cast<int>(kRangeMax), 0);
    api->hotkey(api, kSec, "KeyZoomIn", Tr("Zoom in key"), NULL, "=", 0);
    api->hotkey(api, kSec, "KeyZoomOut", Tr("Zoom out key"), NULL, "-", 0);
    api->hotkey(api, kSec, "KeyBigger", Tr("Bigger key"), Tr("Makes the minimap itself bigger on the screen."), "CTRL+=", 0);
    api->hotkey(api, kSec, "KeySmaller", Tr("Smaller key"), NULL, "CTRL+-", 0);
    api->toggle(api, kSec, "RotateWithCamera", Tr("Rotate with the camera"), Tr("Off - north is always up."), 0, 0);
    api->toggle(api, kSec, "CenterOnSelected", Tr("Centre on the selected"), Tr("Off - on the centre of the camera."), 0, 0);
    api->toggle(api, kSec, "ShowView", Tr("Camera direction"), Tr("A small arrow in the centre."), 1, 0);
    api->colour(api, kSec, "ViewColour", Tr("Camera arrow colour"), Tr("The white arrow is hard to see on light sand - pick a brighter or darker one."), "#F0EAD8", 0);
    api->toggle(api, kSec, "RightClickMove", Tr("Right click - go there"), Tr("Right click on the minimap sends the selected characters to that spot, as a right click on the ground does."), 1, 0);
    api->toggle(api, kSec, "DoubleClickCamera", Tr("Double click - camera there"), Tr("A double left click on the minimap moves the camera to that spot, if the camera can go that far from your characters (the camera range line). Farther - nothing happens."), 1, 0);
    api->toggle(api, kSec, "Tooltips", Tr("Tooltips"), Tr("Hover a town or a character on the minimap: name, faction and relations, squad, bounty."), 1, 0);

    api->section(api, Tr("Who is shown"));
    static const char* const catLabels[CAT_COUNT] = { "Your squad", "Allies", "Neutrals", "Enemies", "Animals" };
    for (int i = 0; i < CAT_COUNT; ++i)
        api->toggle(api, kSec, (std::string("Show") + CAT_KEY[i]).c_str(), Tr(catLabels[i]), NULL, 1, 0);
    api->toggle(api, kSec, "ShowDead", Tr("The dead"), Tr("Dimmed."), 0, 0);
    api->toggle(api, kSec, "ShowFacing", Tr("Facing"), Tr("A small nose on each dot: where the character looks."), 1, 0);
    api->toggle(api, kSec, "FacingFlip", Tr("Flip the facing"), Tr("If the noses point backwards."), 0, 0);
    api->toggle(api, kSec, "CharacterNames", Tr("Character names"), Tr("A name under every dot. Many dots - many names."), 0, 0);
    api->toggle(api, kSec, "ShowBounty", Tr("Bounties"), Tr("A gold ring and the sum on characters with a price on their head."), 1, 0);
    api->colour(api, kSec, "BountyColour", Tr("Bounty colour"), NULL, "#F2C230", 0);
    static const char* const squadValues[] = { "none", "others", "all" };
    static const char* squadLabels[3];
    squadLabels[0] = Tr("None");
    squadLabels[1] = Tr("Other squads");
    squadLabels[2] = Tr("All squads");
    api->choice(api, kSec, "SquadLabels", Tr("Squad names"), Tr("The name of a squad over its leader."), "others", squadValues, squadLabels, 3, 0);
    api->colour(api, kSec, "SquadLabelColour", Tr("Squad name colour"), NULL, "#E6C84A", 0);

    api->section(api, Tr("Towns"));
    api->toggle(api, kSec, "ShowTowns", Tr("Towns and places"), Tr("Everything the big map marks: towns, outposts, camps, nests, ruins - as the game knows them."), 1, 0);
    api->toggle(api, kSec, "TownNames", Tr("Town names"), NULL, 1, 0);
    api->integer(api, kSec, "TownNamesRange", Tr("Names up to range, m"), Tr("Town names are shown while the range is not wider than this: on the whole map they would merge."), 8000, 0, 150000, 0);
    api->toggle(api, kSec, "TownRadius", Tr("Town radius"), Tr("A thin white circle: how far the town reaches."), 1, 0);
    static const char* const iconValues[] = { "game", "diamond" };
    static const char* iconLabels[2];
    iconLabels[0] = Tr("As on the game map");
    iconLabels[1] = Tr("Diamonds");
    api->choice(api, kSec, "TownIcons", Tr("Town icons"), Tr("As on the game map - the icons of the big map, from the game's interface and interface mods. Places the big map has no icon for get a diamond."), "game", iconValues, iconLabels, 2, 0);
    api->integer(api, kSec, "TownIconSize", Tr("Town icon size, px"), NULL, 22, 10, 64, 0);
    api->integer(api, kSec, "TownNameSize", Tr("Town name size, px"), Tr("0 - like the other labels."), 0, 0, 40, 0);
    api->colour(api, kSec, "TownColour", Tr("Town colour"), Tr("auto - by the relations of the town's faction to yours: red hostile, green ally."), "auto", 0);
    api->toggle(api, kSec, "MapMarkers", Tr("Map-markers markers"), Tr("Markers of the Map-markers mod, from the current save."), 1, 0);
    api->toggle(api, kSec, "WantedMarkers", Tr("WantedMap targets"), Tr("Targets of the WantedMap mod, as of the last look at the big map."), 1, 0);
    api->toggle(api, kSec, "PinToEdge", Tr("Pin to the edge"), Tr("Markers of Map-markers and WantedMap that are out of range stay at the edge, pointing the way."), 1, 0);

    api->section(api, Tr("Attack alert"));
    api->toggle(api, kSec, "AttackAlert", Tr("Alert when your people are attacked"), Tr("A pulsing ring on the minimap where a character of your faction is attacked; out of range - at the edge, pointing the way."), 1, 0);
    api->colour(api, kSec, "AlertColour", Tr("Alert colour"), NULL, "#FF3B30", 0);
    api->integer(api, kSec, "AlertSeconds", Tr("Alert lasts, s"), Tr("How long the ring stays after the last attack."), 8, 2, 120, 0);
    api->toggle(api, kSec, "AlertMessage", Tr("Message in the game"), Tr("Under attack: and the names - in the game's message line, at most once in 3 seconds."), 1, 0);
    api->toggle(api, kSec, "AlertFlashFrame", Tr("Flash the frame"), Tr("While someone is attacked the frame of the minimap blinks in the alert colour."), 1, 0);

    api->section(api, Tr("Camera range"));
    api->toggle(api, kSec, "ShowCameraRange", Tr("Show how far the camera can go"), Tr("A thin line around your characters: the camera cannot be moved farther."), 1, 0);
    api->integer(api, kSec, "CameraRange", Tr("Camera range, m"), Tr("0 - found out by itself: move the camera away from your people as far as it goes once, and the line takes that distance."), 0, 0, 50000, 0);
    api->colour(api, kSec, "CameraRangeColour", Tr("Camera line colour"), NULL, "#E8E2D0", 0);
    static const char* const rangeStyles[] = { "dots", "line" };
    static const char* rangeStyleLabels[2];
    rangeStyleLabels[0] = Tr("Dotted");
    rangeStyleLabels[1] = Tr("Solid");
    api->choice(api, kSec, "CameraRangeStyle", Tr("Camera line style"), NULL, "dots", rangeStyles, rangeStyleLabels, 2, 0);

    // Подписи цветов - свои, не те же, что у переключателей выше: панель
    // настроек игры различает строки по подписи, и одноимённые строки
    // сливались - раздел «Цвета» был пустым (20.10.2026).
    api->section(api, Tr("Colours"));
    static const char* const colourLabels[CAT_COUNT] = {
        "Your squad colour", "Allies colour", "Neutrals colour", "Enemies colour", "Animals colour" };
    for (int i = 0; i < CAT_COUNT; ++i)
        api->colour(api, kSec, (std::string("Colour") + CAT_KEY[i]).c_str(), Tr(colourLabels[i]),
                    Tr("auto - the colour the game gives this character on the big map."), CAT_DEFAULT[i], 0);

    api->section(api, Tr("Diagnostics"));
    api->integer(api, kSec, "UpdateFps", Tr("Updates per second"), Tr("How often the whole minimap is redrawn: the map, dots, names, the camera arrow. Higher - smoother when the camera moves, a little more work for the processor; the game FPS does not depend on it."), 20, 2, 60, 0);
    api->integer(api, kSec, "MaxSourceSize", Tr("Map picture limit, px"), Tr("A bigger map picture is scaled down in memory to this size."), 4096, 512, 8192, MCM_RESTART);
    api->toggle(api, kSec, "SwapRedBlue", Tr("Swap red and blue"), Tr("If the colours of the minimap look wrong (blue sand)."), 0, 0);
    api->toggle(api, kSec, "Debug", Tr("Detailed log"), Tr("In RE_Kenshi_log.txt."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    // Хуки - и при Enabled=0: выключатель проверяется в каждом кадре.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&GameWorld::_NV_mainLoop_GPUSensitiveStuff), MainLoop_hook, &g_origMainLoop))
    {
        ErrorLog("BestMinimap: could not hook the game loop, no minimap");
        return;
    }
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update), GuiUpdate_hook, &g_origGuiUpdate))
        ErrorLog("BestMinimap: could not hook the interface update - the minimap may stay in the main menu");
    DebugLog("BestMinimap: installed");
}
