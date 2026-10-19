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
// OpenGL), - текстуру MyGUI сам отдаёт в нужный рендер. Работы на кадр
// немного: 220x220 = 48 тыс. точек с билинейной выборкой, и не каждый кадр,
// а UpdateFps раз в секунду.
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
#include <sstream>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_ImageBox.h>
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
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>

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
    int g_maxSource = 4096;
    bool g_swapRB = false;
    bool g_debug = false;

    HoldKey::Key g_keyToggle, g_keyZoomIn, g_keyZoomOut, g_keyBigger, g_keySmaller;

    const float kRangeMin = 30.0f;
    const float kRangeMax = 20000.0f;
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

    void WriteInt(const char* key, int value)
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        char text[32];
        sprintf_s(text, "%d", value);
        WritePrivateProfileStringA(kSec, key, text, ini.c_str());
    }

    void Invalidate();

    // Зовётся при запуске и из MCM после изменения настройки.
    void __cdecl LoadSettings()
    {
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

    // Из Ogre::Image - в g_src, с уменьшением шагом step (точечная выборка:
    // для картинки карты хватает, а память на усреднение 8192x8192 не нужна).
    bool FillFromImage(Ogre::Image& img)
    {
        const int w = static_cast<int>(img.getWidth()), h = static_cast<int>(img.getHeight());
        const Ogre::PixelFormat f = img.getFormat();
        if (w <= 0 || h <= 0 || img.getData() == NULL)
            return false;
        const int step = Hi(1, (Hi(w, h) + g_maxSource - 1) / g_maxSource);
        const int ow = (w + step - 1) / step, oh = (h + step - 1) / step;
        std::vector<unsigned> px(static_cast<size_t>(ow) * oh, 0xFF000000u);
        const unsigned char* const data = img.getData();

        if (Ogre::PixelUtil::isCompressed(f))
        {
            const bool dxt = f == Ogre::PF_DXT1 || f == Ogre::PF_DXT2 || f == Ogre::PF_DXT3 ||
                             f == Ogre::PF_DXT4 || f == Ogre::PF_DXT5;
            if (!dxt)
            {
                ErrorLog("BestMinimap: map picture is in an unsupported compressed format " +
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
        g_src.px.swap(px);
        g_src.w = ow;
        g_src.h = oh;
        return true;
    }

    bool LoadSource(const std::string& texture)
    {
        g_src = Source();
        if (texture.empty())
            return false;
        const DWORD started = GetTickCount();
        bool ok = false;
        std::string how;
        try
        {
            Ogre::Image img;
            img.load(texture, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            ok = FillFromImage(img);
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
                    ok = FillFromImage(img);
                    how = "gpu copy";
                }
            }
            catch (...)
            {
                ok = false;
            }
        }
        if (!ok)
        {
            g_src = Source();
            g_srcFailed = texture;
            ErrorLog("BestMinimap: could not read the map picture '" + texture + "', the minimap shows no terrain");
            return false;
        }
        g_src.texture = texture;
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
    };

    // Категория персонажа считается не каждый кадр: isEnemy/isAlly не
    // бесплатны, а отношения меняются редко. Кэш на секунду.
    struct CatCache { int cat; DWORD at; };
    std::map<Character*, CatCache> g_catCache;
    DWORD g_catCacheCleared = 0;

    Character* ReferenceCharacter()
    {
        PlayerInterface* const p = ou->player;
        if (p->playerCharacters.size() > 0)
            return p->playerCharacters[0];
        return NULL;
    }

    int CategoryOf(Character* c, Character* ref, DWORD now)
    {
        std::map<Character*, CatCache>::iterator it = g_catCache.find(c);
        if (it != g_catCache.end() && now - it->second.at < 1000)
            return it->second.cat;
        int cat = CAT_NEUTRAL;
        if (c->isPlayerCharacter())
            cat = CAT_SQUAD;
        else if (c->isAnimal() != NULL && (ref == NULL || !c->isEnemy(ref, true)))
            cat = CAT_ANIMAL;
        else if (ref != NULL && c->isEnemy(ref, true))
            cat = CAT_ENEMY;
        else if (ref != NULL && c->isAlly(ref, true))
            cat = CAT_ALLY;
        CatCache& e = g_catCache[c];
        e.cat = cat;
        e.at = now;
        return cat;
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
        if (now - g_catCacheCleared > 30000)
        {
            g_catCache.clear();         // персонажи приходят и уходят - не копим
            g_catCacheCleared = now;
        }
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
            const int cat = CategoryOf(c, ref, now);
            if (!g_show[cat])
                continue;
            Dot d;
            d.pos = p;
            d.dead = dead;
            d.selected = cat == CAT_SQUAD && Contains(g_selected, c);
            d.colour = g_colourAuto[cat] ? FromColour(MapScreen::getMarkerColor(c)) : g_colour[cat];
            dots.push_back(d);
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

    // Разбор без библиотеки JSON: формат пишет сам Map-markers, он простой -
    // массив объектов с полями x, y, type.
    bool ReadNumber(const std::string& s, size_t from, size_t to, const char* key, double& out)
    {
        const size_t k = s.find(key, from);
        if (k == std::string::npos || k >= to)
            return false;
        const size_t colon = s.find(':', k);
        if (colon == std::string::npos || colon >= to)
            return false;
        out = atof(s.c_str() + colon + 1);
        return true;
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
            const size_t close = s.find('}', open);
            if (close == std::string::npos)
                break;
            Pin p;
            p.world = false;
            p.x = p.z = 0;
            if (ReadNumber(s, open, close, "\"x\"", p.u) && ReadNumber(s, open, close, "\"y\"", p.v))
            {
                std::string type = "note";
                const size_t t = s.find("\"type\"", open);
                if (t != std::string::npos && t < close)
                {
                    const size_t q1 = s.find('"', s.find(':', t));
                    const size_t q2 = q1 == std::string::npos ? q1 : s.find('"', q1 + 1);
                    if (q2 != std::string::npos && q2 < close)
                        type = s.substr(q1 + 1, q2 - q1 - 1);
                }
                p.colour = MarkerTypeColour(type);
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
    int g_texW = 0, g_texH = 0;     // размер текстуры (ширина кратна 64)
    int g_viewW = 0, g_viewH = 0;   // видимая часть = размер карты на экране
    int g_texSerial = 0;
    std::vector<unsigned> g_buf;
    std::vector<unsigned char> g_mask;  // 0..255: внутри формы
    bool g_dirtyMask = true;
    DWORD g_lastRender = 0;
    DWORD g_lastWorldFrame = 0;
    bool g_hidden = true;

    int ViewWidth() { return g_size; }
    // Прямоугольник - всегда 16:10, как экран: ширина - Size, высота из неё.
    int ViewHeight() { return g_shape == SHAPE_RECT ? (g_size * 10 + 8) / 16 : g_size; }
    int GamePad() { return g_frame == FRAME_GAME && g_shape != SHAPE_CIRCLE ? 8 : 0; }

    void Invalidate()
    {
        g_dirtyMask = true;
        g_lastRender = 0;
    }

    // Одна текстура на всё время игры - 1024x1024, больше самой большой
    // миникарты (kSizeMax). Видна только её часть размером с миникарту
    // (setImageCoord), рисуем тоже только в неё.
    //
    // Первая версия пересоздавала текстуру под каждый размер, и после первой
    // же смены размера или формы миникарта пропадала (20.10.2026): картинка
    // MyGUI не переживала подмену текстуры. Одна неизменная текстура это
    // снимает, а 4 МБ видеопамяти - не цена.
    //
    // Ширина кратна 64 пикселям: строка - ровно 4 КБ, шаг строки у
    // видеокарты совпадает с нашим (MyGUI отдаёт указатель без шага строки).
    const int kTexSize = 1024;

    bool EnsureTexture()
    {
        if (g_tex != NULL)
            return true;
        char name[64];
        sprintf_s(name, "BestMinimap_View_%d", ++g_texSerial);
        try
        {
            MyGUI::RenderManager& rm = MyGUI::RenderManager::getInstance();
            g_tex = rm.createTexture(name);
            g_tex->createManual(kTexSize, kTexSize, MyGUI::TextureUsage::Dynamic | MyGUI::TextureUsage::Write,
                                MyGUI::PixelFormat::R8G8B8A8);
        }
        catch (...)
        {
            g_tex = NULL;
        }
        if (g_tex == NULL)
        {
            ErrorLog("BestMinimap: could not create the minimap texture");
            return false;
        }
        g_texName = name;
        g_texW = kTexSize;
        g_texH = kTexSize;
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

    void OnPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left || g_locked)
            return;
        g_dragging = true;
        g_dragMouse = MyGUI::InputManager::getInstance().getMousePosition();
        g_dragOrigin = MyGUI::IntPoint(g_left, g_top);
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
            ClampPosition(w + 2 * pad, h + 2 * pad);
            root->setCoord(g_left, g_top, w + 2 * pad, h + 2 * pad);
            if (back != NULL)
                back->setSize(w + 2 * pad, h + 2 * pad);
        }
        root->setNeedMouseFocus(true);

        if (!EnsureTexture())
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
            g_viewW = w;
            g_viewH = h;
            g_dirtyMask = true;
        }
        return true;
    }

    void Show(bool on)
    {
        if (MyGUI::Widget* const root = g_rootRef.get())
            root->setVisible(on);
        g_hidden = !on;
        if (!on)
            g_dragging = false;
    }


    // ---------------------------------------------------------------
    // Рисование
    // ---------------------------------------------------------------

    // Маска формы с мягким краем (1 пиксель сглаживания).
    void BuildMask(int w, int h)
    {
        g_mask.assign(static_cast<size_t>(w) * h, 0);
        const float cx = w * 0.5f, cy = h * 0.5f;
        const float radius = Lo(w, h) * 0.5f - 0.5f;
        const float corner = g_shape == SHAPE_CIRCLE ? 0.0f : 3.0f;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                float inside;
                const float fx = x + 0.5f, fy = y + 0.5f;
                if (g_shape == SHAPE_CIRCLE)
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
        g_dirtyMask = false;
    }

    // Насколько точка внутри формы с отступом inset (для рамки): 0..1.
    float InsideShape(float fx, float fy, int w, int h, float inset)
    {
        const float cx = w * 0.5f, cy = h * 0.5f;
        if (g_shape == SHAPE_CIRCLE)
        {
            const float radius = Lo(w, h) * 0.5f - inset;
            const float d = std::sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
            return Clamp(radius - d + 0.5f, 0.0f, 1.0f);
        }
        const float dx = cx - inset - std::fabs(fx - cx), dy = cy - inset - std::fabs(fy - cy);
        return Clamp(Lo(dx, dy) + 0.5f, 0.0f, 1.0f);
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
        const int w = g_viewW, h = g_viewH;
        const int x0 = Hi(0, static_cast<int>(cx - r - 2)), x1 = Lo(w - 1, static_cast<int>(cx + r + 2));
        const int y0 = Hi(0, static_cast<int>(cy - r - 2)), y1 = Lo(h - 1, static_cast<int>(cy + r + 2));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = x + 0.5f - cx, fy = y + 0.5f - cy;
                const float d = std::sqrt(fx * fx + fy * fy);
                const float cover = Clamp(r + 0.5f - d, 0.0f, 1.0f);
                if (cover <= 0.0f)
                    continue;
                const size_t i = static_cast<size_t>(y) * w + x;
                const float clip = g_mask[i] / 255.0f;
                const float inner = Clamp(r - 1.0f + 0.5f - d, 0.0f, 1.0f);
                unsigned c = Blend(g_buf[i], outline, cover * clip * alpha);
                c = Blend(c, fill, inner * clip * alpha);
                g_buf[i] = c;
            }
    }

    // Треугольник «куда смотрит камера» в центре.
    void DrawView(float cx, float cy, float dirX, float dirY, unsigned colour)
    {
        const float len = Hi(8.0f, g_dot * 2.2f), half = len * 0.55f;
        const float tipX = cx + dirX * len, tipY = cy + dirY * len;
        const float nx = -dirY, ny = dirX;
        const float bx = cx - dirX * len * 0.35f, by = cy - dirY * len * 0.35f;
        const float ax = bx + nx * half, ay = by + ny * half;
        const float cx2 = bx - nx * half, cy2 = by - ny * half;
        const int x0 = Hi(0, static_cast<int>(Lo(Lo(tipX, ax), cx2)) - 1);
        const int x1 = Lo(g_viewW - 1, static_cast<int>(Hi(Hi(tipX, ax), cx2)) + 1);
        const int y0 = Hi(0, static_cast<int>(Lo(Lo(tipY, ay), cy2)) - 1);
        const int y1 = Lo(g_viewH - 1, static_cast<int>(Hi(Hi(tipY, ay), cy2)) + 1);
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
                const size_t i = static_cast<size_t>(y) * g_viewW + x;
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
        const int x0 = Hi(0, static_cast<int>(cx - outer)), x1 = Lo(g_viewW - 1, static_cast<int>(cx + outer));
        const int y0 = Hi(0, static_cast<int>(cy - outer)), y1 = Lo(g_viewH - 1, static_cast<int>(cy + outer));
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x)
            {
                const float fx = x + 0.5f - cx, fy = y + 0.5f - cy;
                const float d = std::sqrt(fx * fx + fy * fy);
                const float cover = Clamp(thick * 0.5f + 0.5f - std::fabs(d - r), 0.0f, 1.0f);
                if (cover <= 0.0f)
                    continue;
                const size_t i = static_cast<size_t>(y) * g_viewW + x;
                g_buf[i] = Blend(g_buf[i], colour, cover * alpha * (g_mask[i] / 255.0f));
            }
    }

    // Граница области, докуда долетает камера: объединение кругов радиусом
    // CameraRange вокруг своих персонажей - рисуем только те точки круга,
    // что не лежат внутри другого круга. Окружность - точками по углу: так
    // дешевле, чем считать расстояние до всех центров в каждом пикселе.
    void DrawCameraRange(const std::vector<Ogre::Vector3>& centresWorld, float pxPerMetre,
                         void (*toScreen)(const Ogre::Vector3&, float&, float&))
    {
        const float range = CameraRange();
        if (!g_camRange || range < 20.0f || centresWorld.empty())
            return;
        const float r = range * pxPerMetre;
        if (r < 3.0f)
            return;
        // Центры на экране; почти совпадающие (отряд в куче) - один раз.
        std::vector<float> cx, cy;
        for (size_t i = 0; i < centresWorld.size(); ++i)
        {
            float x, y;
            toScreen(centresWorld[i], x, y);
            bool dup = false;
            for (size_t j = 0; j < cx.size() && !dup; ++j)
                dup = std::fabs(cx[j] - x) < 1.5f && std::fabs(cy[j] - y) < 1.5f;
            if (!dup)
            {
                cx.push_back(x);
                cy.push_back(y);
            }
        }
        const float w = static_cast<float>(g_viewW), h = static_cast<float>(g_viewH);
        const float r2in = (r - 0.75f) * (r - 0.75f);
        const int steps = Clamp(static_cast<int>(6.2832f * r * 1.5f), 32, 40000);
        for (size_t i = 0; i < cx.size(); ++i)
        {
            // Круг целиком мимо миникарты - пропускаем.
            if (cx[i] + r < -2 || cy[i] + r < -2 || cx[i] - r > w + 2 || cy[i] - r > h + 2)
                continue;
            for (int k = 0; k < steps; ++k)
            {
                const float a = 6.2832f * k / steps;
                const float x = cx[i] + r * std::cos(a), y = cy[i] + r * std::sin(a);
                if (x < -1 || y < -1 || x > w + 1 || y > h + 1)
                    continue;
                bool inside = false;
                for (size_t j = 0; j < cx.size() && !inside; ++j)
                    if (j != i)
                        inside = (x - cx[j]) * (x - cx[j]) + (y - cy[j]) * (y - cy[j]) < r2in;
                if (inside)
                    continue;
                const int px = static_cast<int>(x), py = static_cast<int>(y);
                if (px < 0 || py < 0 || px >= g_viewW || py >= g_viewH)
                    continue;
                const size_t idx = static_cast<size_t>(py) * g_viewW + px;
                g_buf[idx] = Blend(g_buf[idx], g_camRangeColour, 0.7f * (g_mask[idx] / 255.0f));
            }
        }
    }

    // Для DrawCameraRange: мир -> экран текущего вида (вид живёт на время Render).
    const void* g_renderView = NULL;
    void WorldToScreenCb(const Ogre::Vector3& p, float& x, float& y)
    {
        const View& v = *static_cast<const View*>(g_renderView);
        double u, vv;
        g_map.apply(p.x, p.z, u, vv);
        ToScreen(v, u, vv, x, y);
    }

    std::vector<Dot> g_dots;

    void Render(const Ogre::Vector3& center, DWORD now)
    {
        const int w = g_viewW, h = g_viewH;
        if (w <= 0 || h <= 0 || g_tex == NULL)
            return;
        if (g_dirtyMask || g_mask.size() != static_cast<size_t>(w) * h)
            BuildMask(w, h);
        g_buf.assign(static_cast<size_t>(w) * h, 0);

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

        // --- фон: картинка карты ---
        const float alpha = g_opacity / 10.0f;
        const unsigned back = g_backColour;
        const bool haveSrc = g_src.ok();
        for (int y = 0; y < h; ++y)
        {
            const double dy = (y + 0.5 - v.cy) * v.scale;
            for (int x = 0; x < w; ++x)
            {
                const size_t i = static_cast<size_t>(y) * w + x;
                const int m = g_mask[i];
                if (m == 0)
                    continue;
                unsigned c = back;
                if (haveSrc)
                {
                    const double dx = (x + 0.5 - v.cx) * v.scale;
                    const double sx = v.su + v.c * dx - v.s * dy - 0.5;
                    const double sy = v.sv + v.s * dx + v.c * dy - 0.5;
                    if (sx >= 0 && sy >= 0 && sx < v.sw - 1 && sy < v.sh - 1)
                    {
                        const int ix = static_cast<int>(sx), iy = static_cast<int>(sy);
                        const float tx = static_cast<float>(sx - ix), ty = static_cast<float>(sy - iy);
                        const unsigned* const row = &g_src.px[static_cast<size_t>(iy) * v.sw + ix];
                        const unsigned p00 = row[0], p10 = row[1], p01 = row[v.sw], p11 = row[v.sw + 1];
                        const float w00 = (1 - tx) * (1 - ty), w10 = tx * (1 - ty), w01 = (1 - tx) * ty, w11 = tx * ty;
                        const int r = static_cast<int>(ChR(p00) * w00 + ChR(p10) * w10 + ChR(p01) * w01 + ChR(p11) * w11 + 0.5f);
                        const int g = static_cast<int>(ChG(p00) * w00 + ChG(p10) * w10 + ChG(p01) * w01 + ChG(p11) * w11 + 0.5f);
                        const int b = static_cast<int>(ChB(p00) * w00 + ChB(p10) * w10 + ChB(p01) * w01 + ChB(p11) * w11 + 0.5f);
                        c = Pack(r, g, b, 255);
                    }
                }
                g_buf[i] = (c & 0x00FFFFFFu) | (static_cast<unsigned>(m * alpha + 0.5f) << 24);
            }
        }

        const float frameW = g_frame == FRAME_LINE ? 2.0f : 0.0f;
        const unsigned dark = 0xFF101010u;

        // --- метки Map-markers и WantedMap ---
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
                // Цель WantedMap - ромбом не рисуем, а двойным кольцом: так её
                // не спутать с персонажем и с меткой игрока.
                if (list == 1)
                    DrawDisc(x, y, pinR + 2.0f, dark, dark, 1.0f);
                DrawDisc(x, y, pinR + (list == 1 ? 1.0f : 0.0f), pins[i].colour, dark, 1.0f);
            }
        }

        // --- граница полёта камеры ---
        if (g_camRange)
        {
            std::vector<Ogre::Vector3> mineAlive;
            const lektor<Character*>& mine = ou->player->playerCharacters;
            for (uint32_t i = 0; i < mine.size(); ++i)
                if (mine[i] != NULL && !mine[i]->isDead())
                    mineAlive.push_back(mine[i]->getPosition());
            g_renderView = &v;
            DrawCameraRange(mineAlive, static_cast<float>(pxPerMetre / v.scale), &WorldToScreenCb);
            g_renderView = NULL;
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
                DrawDisc(x, y, r, d.colour, outline, d.dead ? 0.45f : 1.0f);
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
                DrawRing(x, y, r, 2.2f, dark, 0.8f);
                DrawRing(x, y, r, 1.6f, g_alertColour, 1.0f);
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
            DrawView(v.cx, v.cy, dirX, dirY, 0xFFF0EAD8u);
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
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                {
                    const size_t i = static_cast<size_t>(y) * w + x;
                    if (g_mask[i] == 0)
                        continue;
                    const float inner = InsideShape(x + 0.5f, y + 0.5f, w, h, 2.0f);
                    if (inner >= 1.0f)
                        continue;
                    const float a = (1.0f - inner) * (g_mask[i] / 255.0f);
                    g_buf[i] = Blend(g_buf[i], fc, a);
                    g_buf[i] = (g_buf[i] & 0x00FFFFFFu) | (static_cast<unsigned>(g_mask[i]) << 24);
                }
        }

        // --- в текстуру ---
        void* const data = g_tex->lock(MyGUI::TextureUsage::Write);
        if (data == NULL)
            return;
        unsigned* const dst = static_cast<unsigned*>(data);
        for (int y = 0; y < h; ++y)
        {
            unsigned* const row = dst + static_cast<size_t>(y) * g_texW;
            const unsigned* const src = &g_buf[static_cast<size_t>(y) * w];
            if (g_swapRB)
                for (int x = 0; x < w; ++x)
                {
                    const unsigned c = src[x];
                    row[x] = (c & 0xFF00FF00u) | ((c >> 16) & 0xFFu) | ((c & 0xFFu) << 16);
                }
            else
                memcpy(row, src, static_cast<size_t>(w) * sizeof(unsigned));
        }
        g_tex->unlock();
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

    void Tick()
    {
        const DWORD now = GetTickCount();
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

        const std::string texture = map->mapImage->_getTextureName();
        if (texture != g_src.texture && texture != g_srcFailed)
        {
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

        const DWORD period = 1000 / static_cast<DWORD>(g_fps);
        if (g_lastRender != 0 && now - g_lastRender < period)
            return;
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
            g_map = Affine();
            g_catCache.clear();
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

    api->section(api, Tr("Who is shown"));
    static const char* const catLabels[CAT_COUNT] = { "Your squad", "Allies", "Neutrals", "Enemies", "Animals" };
    for (int i = 0; i < CAT_COUNT; ++i)
        api->toggle(api, kSec, (std::string("Show") + CAT_KEY[i]).c_str(), Tr(catLabels[i]), NULL, 1, 0);
    api->toggle(api, kSec, "ShowDead", Tr("The dead"), Tr("Dimmed."), 0, 0);
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
    api->integer(api, kSec, "UpdateFps", Tr("Updates per second"), Tr("Lower - less work for the processor."), 20, 2, 60, 0);
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
