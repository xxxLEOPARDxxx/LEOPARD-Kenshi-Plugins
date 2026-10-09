// LiveXpBars - полосы опыта тех навыков, которые качаются прямо сейчас.
//
// ЗАЧЕМ СВОЙ. Есть мод XpBars (DougTownsend): он тоже рисует полосы, но
// навыки в нём выбираются галочками вручную, и ради этого заведены три
// окна - само окно полос, окно выбора и кнопка в характеристиках. Просьба
// владельца сборки была ровно обратная: никакого выбора и никаких окон,
// просто показывать то, что растёт.
//
// КАК ОПРЕДЕЛЯЕМ РОСТ. Хук на начисление опыта не нужен: уровень навыка в
// Kenshi - это float, его дробная часть и есть прогресс до следующего
// уровня. Достаточно раз в кадр снимать значения и смотреть, какие
// подросли. Способ грубый, зато не зависит ни от версии игры, ни от того,
// каким путём опыт начислен, - хоть тренировкой, хоть бонусом от мода.
//
// ПРО УДЕРЖАНИЕ - ГЛАВНОЕ МЕСТО. В первой версии его не было: я решил,
// что долгое затухание само по себе мост через паузу между порциями
// опыта. Это оказалось неверно, и вот почему. Прибавка видна РОВНО ОДИН
// кадр - в следующем значение уже прежнее. Значит яркость успевала
// подрасти на dt/FadeIn, то есть примерно на 0.016, и тут же начинала
// гаснуть. В журнале это выглядело как строка, мелькающая раз в 60 мс с
// прозрачностью полтора процента: «появляется на долю секунды и исчезает
// насовсем».
//
// Поэтому состояние «навык качается» держится по времени: запоминаем
// момент последней прибавки и считаем навык активным ещё HoldSeconds.
// Затухание при этом всё равно долгое - оно сглаживает конец.
//
// ПРО ОТРЯД. При нескольких выделенных персонажах полос нет вовсе: у
// каждого свои навыки, на экране вышла бы каша.

#define KLOC_DOMAIN "live_xp_bars"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <GameTheme.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/Enums.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/MainBarGUI.h>

#define private public
#include <kenshi/gui/CharacterStatsWindow.h>
#undef private

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Палитра
    //
    // Те же ступени, что в StatColours: 0 - тусклый кирпич, 100 -
    // приглушённая зелень, выше сотни - фиолетовый. Названия ключей
    // совпадают нарочно, чтобы можно было перенести подобранные цвета
    // из одного файла в другой копированием.
    // ---------------------------------------------------------------

    const int PALETTE_SIZE = 12;

    const char* const PALETTE_KEYS[PALETTE_SIZE] =
    {
        "Level0", "Level10", "Level20", "Level30", "Level40", "Level50",
        "Level60", "Level70", "Level80", "Level90", "Level100", "Above100"
    };

    const char* const PALETTE_DEFAULT[PALETTE_SIZE] =
    {
        "#8C3A3A", "#A34A42", "#B25E42", "#BE7644", "#C79149", "#CBAA52",
        "#C2B25A", "#AEB25E", "#96AC5F", "#7EA560", "#6C9E63", "#9A7BC8"
    };

    char g_palette[PALETTE_SIZE][8];
    bool g_smoothColours = true;   // SmoothColours: шкала без ступеней

    // Палитра уже разобранными цветами: ScaleColour зовётся на каждую
    // смену числа в строке, и разбирать «#RRGGBB» через sscanf каждый раз
    // незачем. Заполняется в LoadSettings, после чтения ini.
    MyGUI::Colour g_paletteColour[PALETTE_SIZE];

    // Служебные цвета: подпись навыка, «неокрашенное» число и подсказка
    // в режиме настройки. По умолчанию auto - из палитры игры.
    const char* const NAME_THEME_KEY = "Main";
    const char* const PLAIN_THEME_KEY = "Main";
    const char* const HINT_THEME_KEY = "Greyed";

    const char* const NAME_DEFAULT = "#EBE5D1";
    const char* const PLAIN_DEFAULT = "#EBE5D1";
    const char* const HINT_DEFAULT = "#B3B3AD";

    // «skin» - не трогать цвет вовсе, пусть его даёт скин виджета. Так
    // подписи совпадают с окном навыков, потому что скин у них один.
    const char* const KEEP_SKIN = "skin";

    char g_colourName[8];
    char g_colourPlain[8];
    char g_colourHint[8];

    // Цвет заливки: value - по шкале значений, skin - как у игры,
    // либо свой #RRGGBB.
    char g_colourBar[8];


    bool KeepSkin(const char* value)
    {
        return _stricmp(value, KEEP_SKIN) == 0;
    }


    int PaletteIndex(float value)
    {
        if (value < 0.0f)
            return 0;
        if (value >= 100.5f)
            return 11;
        if (value >= 99.5f)
            return 10;

        const int step = static_cast<int>(value) / 10;
        return step < 0 ? 0 : (step > 9 ? 9 : step);
    }


    MyGUI::Colour FromHex(const char* hex)
    {
        unsigned r = 0, g = 0, b = 0;
        if (hex == NULL || sscanf_s(hex + 1, "%2x%2x%2x", &r, &g, &b) != 3)
            return MyGUI::Colour(1.0f, 1.0f, 1.0f);

        return MyGUI::Colour(r / 255.0f, g / 255.0f, b / 255.0f);
    }


    // Цвет по шкале. Ступенями - как раньше: цвет меняется раз в десять
    // единиц. Плавно (SmoothColours, просьба игрока с Nexus) - ступени
    // становятся опорными точками: LevelN стоит ровно на N, между соседними
    // цвет смешивается линейно. Уровень 31 - почти Level30 с небольшой
    // примесью Level40. Сто и выше - как раньше, без смешивания.
    MyGUI::Colour ScaleColour(float value)
    {
        if (!g_smoothColours || value < 0.0f || value >= 99.5f)
            return g_paletteColour[PaletteIndex(value)];

        int step = static_cast<int>(value / 10.0f);
        if (step > 9)
            step = 9;
        const float t = (value - step * 10.0f) / 10.0f;

        const MyGUI::Colour& a = g_paletteColour[step];
        const MyGUI::Colour& b = g_paletteColour[step + 1];
        return MyGUI::Colour(a.red + (b.red - a.red) * t,
                             a.green + (b.green - a.green) * t,
                             a.blue + (b.blue - a.blue) * t);
    }


    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_hideForSquad = true;
    bool g_colourLevel = true;
    bool g_colourPercent = true;
    bool g_debug = false;
    float g_fadeIn = 1.0f;
    float g_fadeOut = 5.0f;
    float g_hold = 3.0f;
    bool g_alwaysShow = false;
    bool g_hideWithGui = true;
    char g_background[16] = "none";   // none или #RRGGBB
    float g_backgroundOpacity = 0.6f;
    int g_left = 40;
    int g_top = 260;
    int g_width = 460;
    int g_rowGap = 0;               // расстояние между строками, px (08.10.2026)


    std::string IniPath()
    {
        char path[MAX_PATH] = {};

        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string file(path, length);

        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string()
                                        : file.substr(0, dot) + ".ini";
    }


    // По-английски: правило стенда - в бинарнике кириллицы нет. Русский
    // .ini кладётся рядом с DLL при установке, эти строки пишутся только
    // если файла не оказалось.
    void WriteDefaultIni(const std::string& ini)
    {
        FILE* f = NULL;
        if (fopen_s(&f, ini.c_str(), "w") != 0 || f == NULL)
            return;

        fputs("; LiveXpBars - bars for the skills training right now.\n", f);
        fputs(";\n", f);
        fputs("; HoldSeconds is what keeps a bar up: a gain is visible for\n", f);
        fputs("; one frame only, so without it a bar would just flicker.\n", f);
        fputs("; SHIFT+B shows a frame you can drag; the place is saved\n", f);
        fputs("; here by itself.\n", f);
        fputs("\n[Bars]\n", f);
        fputs("Enabled=1\n", f);
        fputs("HoldSeconds=3\n", f);
        fputs("FadeInSeconds=1\n", f);
        fputs("FadeOutSeconds=5\n", f);
        fputs("HideForSquad=1\n", f);
        fputs("; AlwaysShow=1 - a skill that has trained once stays on\n", f);
        fputs("; screen until another character is selected.\n", f);
        fputs("AlwaysShow=0\n", f);
        fputs("; HideWithGUI=1 - hide the bars when the game interface\n", f);
        fputs("; is hidden (F7 by default).\n", f);
        fputs("HideWithGUI=1\n", f);
        fputs("ColourLevel=1\n", f);
        fputs("ColourPercent=1\n", f);
        fputs("Left=40\n", f);
        fputs("Top=260\n", f);
        fputs("Width=460\n", f);
        fputs("Debug=0\n", f);

        fputs("\n[Colours]\n", f);
        fputs("; skin - leave the colour the widget skin gives it.\n", f);
        fputs("; That is what makes the rows match the skills window.\n", f);
        fputs("; auto - take it from the game palette instead\n", f);
        fputs("; (data/gui/colours/kenshi_colours.xml, and from the\n", f);
        fputs("; interface mod if one overrides it). Or write #RRGGBB.\n", f);
        fputs("Name=skin\n", f);
        fputs("Plain=skin\n", f);
        fputs("Hint=skin\n", f);
        fputs("; value - colour the bar fill by progress,\n", f);
        fputs("; skin - leave it as the game draws it,\n", f);
        fputs("; or write #RRGGBB.\n", f);
        fputs("BarColour=value\n", f);
        fputs("; SmoothColours=1 - blend between the LevelN steps below\n", f);
        fputs("; instead of jumping every ten points.\n", f);
        fputs("SmoothColours=1\n", f);
        fputs("; Background under the bars: none, #RRGGBB (flat fill)\n", f);
        fputs("; or game (the game's framed panel), and its\n", f);
        fputs("; opacity from 0 to 1. Helps where the ground is bright.\n", f);
        fputs("Background=none\n", f);
        fputs("BackgroundOpacity=0.6\n", f);
        fputs("\n", f);
        for (int i = 0; i < PALETTE_SIZE; ++i)
            fprintf(f, "%s=%s\n", PALETTE_KEYS[i], PALETTE_DEFAULT[i]);

        fclose(f);
    }


    float ReadFloat(const std::string& ini, const char* key, float fallback)
    {
        char text[32] = {};
        char preset[32] = {};
        sprintf_s(preset, "%.3f", fallback);

        GetPrivateProfileStringA("Bars", key, preset,
                                 text, sizeof(text), ini.c_str());

        const float value = static_cast<float>(atof(text));
        return value >= 0.0f ? value : fallback;
    }


    bool ValidColour(const char* text)
    {
        if (text == NULL || strlen(text) != 7 || text[0] != '#')
            return false;

        for (int i = 1; i < 7; ++i)
        {
            const char c = text[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
                                         || (c >= 'A' && c <= 'F')))
                return false;
        }

        return true;
    }



    // ---------------------------------------------------------------
    // Отложенный опрос палитры
    //
    // При загрузке плагина MyGUI ещё не поднят, и спрашивать у него
    // цвета нельзя - игра падает. Поэтому auto на этом этапе только
    // отмечается, а разрешается в первом же кадре.
    // ---------------------------------------------------------------

    struct PendingColour
    {
        char* slot;
        const char* themeKey;
        const char* fallback;
    };

    std::vector<PendingColour> g_pendingTheme;
    bool g_themeResolved = false;
    // Опрос палитры можно выключить целиком: он обращается к MyGUI, и
    // если падение окажется там, это первое, что стоит проверить.
    bool g_useGameTheme = true;



    void ResolveThemeColours()
    {
        if (g_themeResolved)
            return;

        g_themeResolved = true;

        if (!g_useGameTheme)
        {
            DebugLog("LiveXpBars: game palette is off, keeping the defaults");
            g_pendingTheme.clear();
            return;
        }

        DebugLog("LiveXpBars: asking MyGUI for the palette");

        for (size_t i = 0; i < g_pendingTheme.size(); ++i)
        {
            const PendingColour& item = g_pendingTheme[i];
            const char* const themed =
                GameTheme::Hex(item.themeKey, item.fallback);

            if (themed != NULL)
                strcpy_s(item.slot, 8, themed);
        }

        DebugLog("LiveXpBars: palette taken");
        g_pendingTheme.clear();
    }


    // «auto» - взять из палитры игры. Так служебные цвета едут вместе
    // с темой интерфейса и её модом, а не спорят с ними.
    void ReadColour(const std::string& ini, const char* key,
                    const char* themeKey, const char* fallback, char* out)
    {
        char text[32] = {};
        GetPrivateProfileStringA("Colours", key, KEEP_SKIN,
                                 text, sizeof(text), ini.c_str());

        if (KeepSkin(text))
        {
            strcpy_s(out, 8, KEEP_SKIN);
            return;
        }

        if (_stricmp(text, "auto") == 0)
        {
            strcpy_s(out, 8, fallback);

            PendingColour pending;
            pending.slot = out;
            pending.themeKey = themeKey;
            pending.fallback = fallback;
            g_pendingTheme.push_back(pending);
            return;
        }

        if (ValidColour(text))
        {
            strcpy_s(out, 8, text);
            return;
        }

        char note[160];
        sprintf_s(note, "LiveXpBars: %s='%s' is not skin, auto or #RRGGBB, "
                        "keeping the game palette", key, text);
        ErrorLog(note);
        strcpy_s(out, 8, fallback);

        PendingColour pending;
        pending.slot = out;
        pending.themeKey = themeKey;
        pending.fallback = fallback;
        g_pendingTheme.push_back(pending);
    }


    void SavePosition()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        char text[32];
        sprintf_s(text, "%d", g_left);
        WritePrivateProfileStringA("Bars", "Left", text, ini.c_str());
        sprintf_s(text, "%d", g_top);
        WritePrivateProfileStringA("Bars", "Top", text, ini.c_str());
    }


    // Подложка «game» - игровая панель с рамкой (скин всплывающих
    // описаний). Отдельной рамки без заливки в скинах игры нет: у всех
    // рамочных скинов середина непрозрачная (проверено по атласу), так
    // что это не рамка поверх своего цвета, а панель целиком.
    bool IsGamePanel()
    {
        return _stricmp(g_background, "game") == 0;
    }


    bool HasBackground()
    {
        return g_background[0] == '#' || IsGamePanel();
    }


    void LoadSettings()
    {
        for (int i = 0; i < PALETTE_SIZE; ++i)
        {
            strcpy_s(g_palette[i], sizeof(g_palette[i]), PALETTE_DEFAULT[i]);
            g_paletteColour[i] = FromHex(g_palette[i]);
        }

        strcpy_s(g_colourName, sizeof(g_colourName), KEEP_SKIN);
        strcpy_s(g_colourPlain, sizeof(g_colourPlain), KEEP_SKIN);
        strcpy_s(g_colourHint, sizeof(g_colourHint), KEEP_SKIN);
        strcpy_s(g_colourBar, sizeof(g_colourBar), "value");

        const std::string ini = IniPath();
        if (ini.empty())
            return;

        if (GetFileAttributesA(ini.c_str()) == INVALID_FILE_ATTRIBUTES)
            WriteDefaultIni(ini);

        g_enabled = GetPrivateProfileIntA("Bars", "Enabled", 1, ini.c_str()) != 0;
        g_hideForSquad =
            GetPrivateProfileIntA("Bars", "HideForSquad", 1, ini.c_str()) != 0;
        g_colourLevel =
            GetPrivateProfileIntA("Bars", "ColourLevel", 1, ini.c_str()) != 0;
        g_colourPercent =
            GetPrivateProfileIntA("Bars", "ColourPercent", 1, ini.c_str()) != 0;
        g_debug = GetPrivateProfileIntA("Bars", "Debug", 0, ini.c_str()) != 0;
        g_useGameTheme = GetPrivateProfileIntA("Colours", "UseGameTheme", 1, ini.c_str()) != 0;

        g_hold = ReadFloat(ini, "HoldSeconds", 3.0f);
        g_alwaysShow =
            GetPrivateProfileIntA("Bars", "AlwaysShow", 0, ini.c_str()) != 0;
        g_hideWithGui =
            GetPrivateProfileIntA("Bars", "HideWithGUI", 1, ini.c_str()) != 0;
        g_smoothColours =
            GetPrivateProfileIntA("Colours", "SmoothColours", 1, ini.c_str()) != 0;
        g_fadeIn = ReadFloat(ini, "FadeInSeconds", 1.0f);
        g_fadeOut = ReadFloat(ini, "FadeOutSeconds", 5.0f);

        g_left = GetPrivateProfileIntA("Bars", "Left", 40, ini.c_str());
        g_top = GetPrivateProfileIntA("Bars", "Top", 260, ini.c_str());
        g_width = GetPrivateProfileIntA("Bars", "Width", 460, ini.c_str());
        g_rowGap = static_cast<int>(GetPrivateProfileIntA("Bars", "RowSpacing", 0, ini.c_str()));
        if (g_rowGap < -8)
            g_rowGap = -8;
        if (g_rowGap > 40)
            g_rowGap = 40;

        if (g_width < 240)
            g_width = 240;

        for (int i = 0; i < PALETTE_SIZE; ++i)
        {
            char text[32] = {};
            GetPrivateProfileStringA("Colours", PALETTE_KEYS[i],
                                     PALETTE_DEFAULT[i], text, sizeof(text),
                                     ini.c_str());

            if (ValidColour(text))
            {
                strcpy_s(g_palette[i], sizeof(g_palette[i]), text);
                continue;
            }

            char note[160];
            sprintf_s(note, "LiveXpBars: %s='%s' is not #RRGGBB, using %s",
                      PALETTE_KEYS[i], text, PALETTE_DEFAULT[i]);
            ErrorLog(note);
        }

        for (int i = 0; i < PALETTE_SIZE; ++i)
            g_paletteColour[i] = FromHex(g_palette[i]);

        ReadColour(ini, "Name", NAME_THEME_KEY, NAME_DEFAULT, g_colourName);
        ReadColour(ini, "Plain", PLAIN_THEME_KEY, PLAIN_DEFAULT, g_colourPlain);
        ReadColour(ini, "Hint", HINT_THEME_KEY, HINT_DEFAULT, g_colourHint);

        // «skin» - цвет шаблона Kenshi_TextboxStandardText. У ванили он почти
        // чёрный (шаблон - для пергамента), и над картой подписи не было
        // видно (08.10.2026). На ванильном интерфейсе - свой светлый; моды
        // интерфейса (Dark UI) красят шаблон под тёмный фон сами.
        if (!GameTheme::PaletteFromMod())
        {
            if (KeepSkin(g_colourName))
                strcpy_s(g_colourName, sizeof(g_colourName), NAME_DEFAULT);
            if (KeepSkin(g_colourPlain))
                strcpy_s(g_colourPlain, sizeof(g_colourPlain), PLAIN_DEFAULT);
        }

        // У заливки своё значение по умолчанию - value, поэтому читаем
        // её отдельно от служебных цветов.
        {
            char text[32] = {};
            GetPrivateProfileStringA("Colours", "BarColour", "value",
                                     text, sizeof(text), ini.c_str());

            if (_stricmp(text, "value") == 0 || KeepSkin(text))
                strcpy_s(g_colourBar, sizeof(g_colourBar), text);
            else if (ValidColour(text))
                strcpy_s(g_colourBar, sizeof(g_colourBar), text);
            else
                strcpy_s(g_colourBar, sizeof(g_colourBar), "value");
        }

        // Подложка: none - её нет вовсе, #RRGGBB - цвет. Прозрачность
        // отдельно: цвет с альфой ini-файл не выразит понятно.
        {
            // Разбор терпимый: игрок первый раз подложку не увидел, и
            // строже всего тут было бы молча выкинуть «333333» без решётки
            // или «#333333 ; тёмная» с пометкой в той же строке.
            char raw[64] = {};
            GetPrivateProfileStringA("Colours", "Background", "none",
                                     raw, sizeof(raw), ini.c_str());

            std::string text(raw);
            const std::string::size_type cut = text.find_first_of(" \t;");
            if (cut != std::string::npos)
                text = text.substr(0, cut);
            if (text.size() == 6)
                text = "#" + text;

            if (ValidColour(text.c_str()) || _stricmp(text.c_str(), "game") == 0)
            {
                strcpy_s(g_background, sizeof(g_background), text.c_str());
            }
            else
            {
                strcpy_s(g_background, sizeof(g_background), "none");
                if (_stricmp(text.c_str(), "none") != 0 && !text.empty())
                {
                    ErrorLog(std::string("LiveXpBars: Background='") + raw +
                             "' is not none, game or #RRGGBB, no background");
                }
            }

            char opacity[32] = {};
            GetPrivateProfileStringA("Colours", "BackgroundOpacity", "0.6",
                                     opacity, sizeof(opacity), ini.c_str());
            g_backgroundOpacity = static_cast<float>(atof(opacity));
            if (g_backgroundOpacity < 0.0f) g_backgroundOpacity = 0.0f;
            if (g_backgroundOpacity > 1.0f) g_backgroundOpacity = 1.0f;

            if (HasBackground())
            {
                char note[96];
                sprintf_s(note, "LiveXpBars: background %s, opacity %.2f",
                          g_background, g_backgroundOpacity);
                DebugLog(note);
            }
        }
    }


    // ---------------------------------------------------------------
    // Разметка
    // ---------------------------------------------------------------

    const int kRowHeight = 24;
    const int kNameWidth = 150;
    const int kLevelWidth = 40;
    const int kPercentWidth = 60;
    const int kGap = 8;
    const int kBarHeight = 12;
    const int kSetupRows = 4;

    // Поле между краем рамки настройки и содержимым. У скина рамки
    // своя обводка, и без отступа она налезает на текст.
    const int kSetupPad = 12;

    // По бокам рамке настройки чуть больше: у скина
    // Kenshi_GenericTextBoxFlatSkin кайма справа шире, и при равных полях
    // строки казались сдвинутыми вправо (подобрано по снимку).
    const int kSetupPadLeft = kSetupPad + 1;
    const int kSetupPadRight = kSetupPad + 3;

    // Поле подложки вокруг строк: без него текст и полосы лежат впритык
    // к краю цветного прямоугольника.
    const int kBackgroundPad = 6;

    // У игровой панели рамка в восемь пикселей - строки отодвигаем
    // дальше, чтобы текст на неё не ложился.
    const int kGamePanelPad = 10;

    // Снизу поле меньше, чем сверху: у строки под текстом и так пусто
    // (текст по середине строки в 24 пикселя), и с полным полем снизу
    // подложка выглядела толще, чем надо. Просьба владельца - минус 4.
    const int kBottomTrim = 4;

    // У панели с рамкой срез меньше: рамка сама съедает край, и с полными
    // четырьмя последняя строка к ней прижималась.
    const int kGamePanelBottomTrim = 2;


    // Поля вокруг строк - по сторонам отдельно: подбирались по снимкам,
    // и симметрично выглядело не при равных числах. У панели с рамкой по
    // бокам на 2 больше, чем сверху; про рамку настройки - у kSetupPadLeft.
    struct Margins
    {
        int left, right, top, bottom;
    };


    struct Row
    {
        MyGUI::TextBox* name;
        MyGUI::TextBox* level;
        MyGUI::TextBox* percent;
        MyGUI::Widget* groove;      // подложка полосы
        MyGUI::Widget* fill;        // заливка, её ширина и есть прогресс
        float fraction;             // 0..100
        float alpha;
        unsigned long long lastGain;

        // Что уже стоит на экране. Кадр зовёт FillRow и PlaceRow для
        // каждой видимой строки, а меняется число раз в несколько секунд:
        // без этих отметок каждый кадр заново собирались строки «31» и
        // «30.9%», переставлялись надписи и пять виджетов. С AlwaysShow
        // видимых строк бывает до тридцати трёх.
        std::string label;          // подпись навыка
        int shownKey;               // значение в тысячных: уровень и 0.1%
        int placedTop, placedLeft, placedBar, placedFill;

        Row() : name(0), level(0), percent(0), groove(0), fill(0),
                fraction(0.0f), alpha(0.0f), lastGain(0), shownKey(-1),
                placedTop(-1), placedLeft(-1), placedBar(-1), placedFill(-1) {}
    };


    MyGUI::Widget* g_panel = 0;
    MyGUI::Widget* g_backdrop = 0;
    MyGUI::TextBox* g_hint = 0;
    std::map<int, Row> g_rows;
    std::map<int, float> g_seen;
    Character* g_watched = 0;
    unsigned long long g_lastTick = 0;

    bool g_firstTickLogged = false;
    bool g_setup = false;
    bool g_keyWasDown = false;
    bool g_dragging = false;
    MyGUI::IntPoint g_dragMouse;
    MyGUI::IntPoint g_dragOrigin;


    // Состав списка задаём явно: перебор всего StatsEnumerated тащит и
    // служебные величины, которым в полосах делать нечего. Это те же
    // характеристики, что показывает окно навыков.
    const StatsEnumerated SHOWN_STATS[] =
    {
        STAT_STRENGTH, STAT_TOUGHNESS, STAT_DEXTERITY, STAT_PERCEPTION,
        STAT_KATANAS, STAT_SABRES, STAT_HACKERS, STAT_HEAVYWEAPONS,
        STAT_BLUNT, STAT_POLEARMS,
        STAT_MELEE_ATTACK, STAT_MELEE_DEFENCE, STAT_DODGE, STAT_MARTIALARTS,
        STAT_TURRETS, STAT_CROSSBOWS, STAT_FRIENDLY_FIRE,
        STAT_STEALTH, STAT_LOCKPICKING, STAT_THIEVING, STAT_ASSASSINATION,
        STAT_ATHLETICS, STAT_SWIMMING,
        STAT_MEDIC, STAT_ENGINEERING, STAT_ROBOTICS, STAT_SCIENCE,
        STAT_SMITHING_WEAPON, STAT_SMITHING_ARMOUR, STAT_SMITHING_BOW,
        STAT_LABOURING, STAT_FARMING, STAT_COOKING
    };

    const int SHOWN_COUNT = sizeof(SHOWN_STATS) / sizeof(SHOWN_STATS[0]);


    // Названия берём у игры, поэтому они сразу переведены.
    //
    // Но таблицу окна характеристик игра строит лениво, при первом его
    // открытии, а звать BuildStats самим НЕЛЬЗЯ - это приватная статика,
    // и вызов посреди загрузки мира валил игру. Поэтому: есть таблица -
    // берём перевод, нет - внутреннее английское имя из CharStats.
    // Полосы работают сразу, а подписи сами станут русскими, как только
    // игрок заглянет в характеристики.
    std::vector<std::pair<int, std::string> > g_stats;
    bool g_statsLocalised = false;
    unsigned long long g_statsChecked = 0;


    const std::vector<std::pair<int, std::string> >& StatList(
        unsigned long long now)
    {
        // Повторяем не чаще раза в секунду: пока перевода нет, спрашивать
        // каждый кадр незачем.
        if (g_statsLocalised || (g_statsChecked && now - g_statsChecked < 1000))
            return g_stats;

        g_statsChecked = now;

        std::vector<std::pair<int, std::string> > fresh;
        bool localised = true;

        for (int i = 0; i < SHOWN_COUNT; ++i)
        {
            const StatsEnumerated type = SHOWN_STATS[i];

            std::string name;

            CharacterStatsWindow::Stat* const entry =
                CharacterStatsWindow::getStat(type);
            if (entry != NULL && !entry->name.empty())
                name = entry->name;
            else
                localised = false;

            if (name.empty())
                name = CharStats::getStatName(type);

            if (!name.empty())
                fresh.push_back(std::make_pair(static_cast<int>(type), name));
        }

        if (!fresh.empty())
        {
            g_stats.swap(fresh);
            g_statsLocalised = localised;
        }

        return g_stats;
    }


    std::string Whole(float value)
    {
        char text[16];
        sprintf_s(text, "%d", static_cast<int>(std::floor(value)));
        return text;
    }


    std::string Fraction(float value)
    {
        char text[16];
        // Меньше 99.95 - иначе на 30.9996 выходило бы «30» и «100.0%».
        float percent = (value - std::floor(value)) * 100.0f;
        if (percent > 99.9f)
            percent = 99.9f;
        sprintf_s(text, "%.1f%%", percent);
        return text;
    }


    // ---------------------------------------------------------------
    // Перетаскивание
    // ---------------------------------------------------------------

    // Панель не должна уезжать за край: оттуда её уже не достать мышью,
    // а строки обрезаются по половине - именно так и вышло при первой
    // пробе.
    void ClampPosition(int height)
    {
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();

        const int maxLeft = view.width - 80;

        // Нижний край тоже держим на экране, иначе строки обрезаются, а
        // мышью панель уже не поймать.
        const int tall = height > kRowHeight ? height : kRowHeight;
        const int maxTop = view.height - tall;

        if (g_left < 0) g_left = 0;
        if (g_top < 0) g_top = 0;
        if (g_left > maxLeft) g_left = maxLeft > 0 ? maxLeft : 0;
        if (g_top > maxTop) g_top = maxTop > 0 ? maxTop : 0;
    }


    void OnPressed(MyGUI::Widget*, int, int, MyGUI::MouseButton button)
    {
        if (button != MyGUI::MouseButton::Left)
            return;

        g_dragging = true;
        g_dragMouse = MyGUI::InputManager::getInstance().getMousePosition();

        // Запоминаем координаты СОДЕРЖИМОГО, а не панели: в настройке
        // панель смещена на отступ, и иначе полосы прыгали бы при
        // выходе из неё.
        g_dragOrigin = MyGUI::IntPoint(g_left, g_top);
    }


    void OnDragged(MyGUI::Widget*, int, int, MyGUI::MouseButton)
    {
        if (!g_dragging || g_panel == NULL)
            return;

        const MyGUI::IntPoint mouse =
            MyGUI::InputManager::getInstance().getMousePosition();

        g_left = g_dragOrigin.left + (mouse.left - g_dragMouse.left);
        g_top = g_dragOrigin.top + (mouse.top - g_dragMouse.top);

        ClampPosition(g_panel->getHeight());
        g_panel->setPosition(g_left - kSetupPadLeft, g_top - kSetupPad);
    }


    void OnReleased(MyGUI::Widget*, int, int, MyGUI::MouseButton)
    {
        if (!g_dragging)
            return;

        g_dragging = false;
        SavePosition();
    }


    void EnsurePanel()
    {
        if (g_panel != NULL)
            return;

        ClampPosition(kRowHeight);

        g_panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(g_left, g_top, g_width, kRowHeight),
            MyGUI::Align::Default, "Window", "LiveXpBars_Panel");

        // Полосы - индикатор, а не орган управления: сквозь них должно
        // кликаться по миру. Мышь панель ловит только в настройке.
        g_panel->setNeedMouseFocus(false);

        g_panel->eventMouseButtonPressed += MyGUI::newDelegate(&OnPressed);
        g_panel->eventMouseDrag += MyGUI::newDelegate(&OnDragged);
        g_panel->eventMouseButtonReleased += MyGUI::newDelegate(&OnReleased);

        // Подложка - первым ребёнком, чтобы строки рисовались поверх неё.
        // WhiteSkin, как у заливки полос: setColour умножает цвет на
        // текстуру, и только на белой выходит ровно заданный цвет.
        if (HasBackground())
        {
            g_backdrop = g_panel->createWidget<MyGUI::Widget>(
                IsGamePanel() ? "Kenshi_FloatingPanelSkin" : "WhiteSkin",
                MyGUI::IntCoord(0, 0, g_width, kRowHeight),
                MyGUI::Align::Default, "");
            if (!IsGamePanel())
                g_backdrop->setColour(FromHex(g_background));
            g_backdrop->setNeedMouseFocus(false);
            g_backdrop->setVisible(false);
        }

        g_hint = g_panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText",
            MyGUI::IntCoord(0, 0, g_width, kRowHeight),
            MyGUI::Align::Default, "");
        if (!KeepSkin(g_colourHint))
            g_hint->setTextColour(FromHex(g_colourHint));
        g_hint->setNeedMouseFocus(false);
        g_hint->setVisible(false);
    }


    void SetSetup(bool on)
    {
        if (g_setup == on || g_panel == NULL)
            return;

        g_setup = on;

        // Рамка нужна только на время выбора места.
        g_panel->changeWidgetSkin(on ? "Kenshi_GenericTextBoxFlatSkin"
                                     : "PanelEmpty");
        g_panel->setNeedMouseFocus(on);

        if (!on)
        {
            g_dragging = false;
            SavePosition();

            // Образцы гасим сразу. Иначе после выхода из настройки они
            // ещё несколько секунд висят на экране как настоящие полосы.
            for (std::map<int, Row>::iterator it = g_rows.begin();
                 it != g_rows.end(); ++it)
            {
                if (it->second.lastGain == 0)
                    it->second.alpha = 0.0f;
            }
        }
    }


    // SHIFT+B опрашиваем сами: одна клавиша не стоит того, чтобы лезть
    // хуком в обработчик ввода игры. Печатает игрок в поле - не мешаем.
    // Окно игры ли сейчас впереди. GetAsyncKeyState видит клавиши
    // независимо от фокуса, и без этой проверки SHIFT+B в браузере
    // переключал бы режим настройки в свёрнутой игре.
    bool GameInForeground()
    {
        const HWND front = GetForegroundWindow();
        if (front == NULL)
            return false;

        DWORD pid = 0;
        GetWindowThreadProcessId(front, &pid);
        return pid == GetCurrentProcessId();
    }


    void PollHotkey()
    {
        // Сначала клавиши, потом окно: GetAsyncKeyState дешевле пары
        // вызовов про окно, а клавиши почти всегда отпущены.
        const bool down = (GetAsyncKeyState('B') & 0x8000) != 0
                       && (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0
                       && GameInForeground();

        if (down && !g_keyWasDown &&
            MyGUI::InputManager::getInstance().getKeyFocusWidget() == NULL)
        {
            SetSetup(!g_setup);
        }

        g_keyWasDown = down;
    }


    // ---------------------------------------------------------------
    // Строки
    // ---------------------------------------------------------------

    Row& EnsureRow(int stat, const std::string& label)
    {
        const std::map<int, Row>::iterator found = g_rows.find(stat);
        if (found != g_rows.end())
            return found->second;

        // Палитру спрашиваем здесь, при создании первой строки, а не в
        // первом кадре после загрузки: там тот же вызов валил игру.
        ResolveThemeColours();

        Row row;

        row.name = g_panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, kNameWidth, kRowHeight),
            MyGUI::Align::Default, "");
        row.name->setCaption(label);
        if (!KeepSkin(g_colourName))
            row.name->setTextColour(FromHex(g_colourName));

        row.level = g_panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, kLevelWidth, kRowHeight),
            MyGUI::Align::Default, "");

        row.percent = g_panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, kPercentWidth, kRowHeight),
            MyGUI::Align::Default, "");

        // Те же два скина, которыми полосу рисует игра, только собраны
        // вручную: у готового ProgressBar заливка недоступна из кода.
        row.groove = g_panel->createWidget<MyGUI::Widget>(
            "Kenshi_ProgressBarEmptySkin",
            MyGUI::IntCoord(0, 0, 100, kBarHeight),
            MyGUI::Align::Default, "");

        // Заливка на WhiteSkin, и это принципиально.
        //
        // setColour УМНОЖАЕТ цвет на текстуру скина. Все игровые скины
        // полос в тёмной теме сами тёмные: у «Russian Dark UI»
        // Kenshi_ProgressBarFilledSkin и Kenshi_LifeBarSkin - это
        // RGB(0,59,2) и RGB(0,67,3), почти чёрно-зелёные. Умножай на
        // что угодно - светлее не станет, и заданный цвет не получается
        // вовсе. Проверено измерением атласа, а не на глаз.
        //
        // WhiteSkin из common_skins.xml - белый квадрат 2x2 в углу
        // атласа, белый и в ванили, и под модом. Умножение на белое
        // даёт ровно тот цвет, который просили.
        row.fill = row.groove->createWidget<MyGUI::Widget>(
            "WhiteSkin",
            MyGUI::IntCoord(0, 0, 0, kBarHeight),
            MyGUI::Align::Default, "");

        row.name->setNeedMouseFocus(false);
        row.level->setNeedMouseFocus(false);
        row.percent->setNeedMouseFocus(false);
        row.groove->setNeedMouseFocus(false);
        row.fill->setNeedMouseFocus(false);

        g_rows[stat] = row;
        return g_rows[stat];
    }


    void FillRow(Row& row, const std::string& label, float value)
    {
        // Подпись переназначаем: пока окно характеристик не открывали,
        // имя было английским, а потом становится переведённым.
        // Сравниваем со своей копией: getCaption() - UString, и сравнение
        // с std::string каждый кадр перекодировало бы строку.
        if (row.label != label)
        {
            row.label = label;
            row.name->setCaption(label);
        }

        // Число не изменилось (с точностью до показанного) - ни надписи,
        // ни цвета трогать незачем.
        const int key = static_cast<int>(value * 1000.0f);
        if (key == row.shownKey)
            return;
        row.shownKey = key;

        row.level->setCaption(Whole(value));
        row.percent->setCaption(Fraction(value));

        const float fraction = (value - std::floor(value)) * 100.0f;
        row.fraction = fraction;

        if (_stricmp(g_colourBar, "value") == 0)
            row.fill->setColour(ScaleColour(fraction));
        else if (!KeepSkin(g_colourBar))
            row.fill->setColour(FromHex(g_colourBar));

        // Когда окраска по величине выключена, цвет тоже лучше отдать
        // скину - тогда число выглядит как в окне навыков.
        if (g_colourLevel)
            row.level->setTextColour(ScaleColour(value));
        else if (!KeepSkin(g_colourPlain))
            row.level->setTextColour(FromHex(g_colourPlain));

        // Проценты красим по самому прогрессу: только начал - тускло,
        // близко к уровню - ярко. Шкала та же, что у значения, чтобы
        // глазу не приходилось держать в голове две.
        if (g_colourPercent)
            row.percent->setTextColour(ScaleColour(fraction));
        else if (!KeepSkin(g_colourPlain))
            row.percent->setTextColour(FromHex(g_colourPlain));
    }


    void PlaceRow(Row& row, int index, int leftOffset, int topOffset)
    {
        const int top = topOffset + index * (kRowHeight + g_rowGap);
        const int levelLeft = leftOffset + kNameWidth + kGap;
        const int percentLeft = levelLeft + kLevelWidth + kGap;
        const int barLeft = percentLeft + kPercentWidth + kGap;

        // Полоса кончается там же, где строка: leftOffset + g_width. Было
        // «g_width - barLeft - kGap» - без левого поля и с лишним зазором,
        // и справа от полосы оставалось поле вдвое больше левого плюс 8
        // пикселей (на подложке это бросалось в глаза: слева 10, справа 26).
        int barWidth = leftOffset + g_width - barLeft;
        if (barWidth < 60)
            barWidth = 60;

        int fillWidth = static_cast<int>(barWidth * row.fraction / 100.0f + 0.5f);
        if (fillWidth < 0) fillWidth = 0;
        if (fillWidth > barWidth) fillWidth = barWidth;

        // Всё на своих местах - переставлять нечего.
        if (row.placedTop == top && row.placedLeft == leftOffset &&
            row.placedBar == barWidth && row.placedFill == fillWidth)
        {
            return;
        }
        row.placedTop = top;
        row.placedLeft = leftOffset;
        row.placedBar = barWidth;
        row.placedFill = fillWidth;

        row.name->setCoord(leftOffset, top, kNameWidth, kRowHeight);
        row.level->setCoord(levelLeft, top, kLevelWidth, kRowHeight);
        row.percent->setCoord(percentLeft, top, kPercentWidth, kRowHeight);
        row.groove->setCoord(barLeft, top + (kRowHeight - kBarHeight) / 2,
                             barWidth, kBarHeight);
        row.fill->setCoord(0, 0, fillWidth, kBarHeight);
    }


    void ShowRow(Row& row, bool visible)
    {
        row.name->setVisible(visible);
        row.level->setVisible(visible);
        row.percent->setVisible(visible);
        row.groove->setVisible(visible);

        if (!visible)
            return;

        row.name->setAlpha(row.alpha);
        row.level->setAlpha(row.alpha);
        row.percent->setAlpha(row.alpha);
        row.groove->setAlpha(row.alpha);
    }


    // Спрятан ли интерфейс игры (F7, «Показать основной интерфейс»).
    //
    // Признаков два, и какой из них переключает F7, по заголовкам не
    // видно: флаг ForgottenGUI::visible и видимость корня главной панели
    // (MainBarGUI). Спрятан любой - прячемся и мы. В журнале (Debug)
    // видно, какой именно сменился.
    bool GameGuiHidden()
    {
        if (gui == NULL)
            return false;

        const bool flag = gui->visible;
        MyGUI::Widget* const bar = gui->mainbar != NULL
            ? gui->mainbar->mMainWidget : NULL;
        const bool barShown = bar == NULL || bar->getVisible();

        static int lastFlag = -1;
        static int lastBar = -1;
        if (g_debug && (lastFlag != (flag ? 1 : 0) || lastBar != (barShown ? 1 : 0)))
        {
            char note[96];
            sprintf_s(note, "LiveXpBars: game gui visible=%d, main bar shown=%d",
                      flag ? 1 : 0, barShown ? 1 : 0);
            DebugLog(note);
        }
        lastFlag = flag ? 1 : 0;
        lastBar = barShown ? 1 : 0;

        return !flag || !barShown;
    }


    bool SquadSelected()
    {
        if (!g_hideForSquad || ou == NULL || ou->player == NULL)
            return false;

        return ou->player->selectedCharacters.size() > 1;
    }


    void Report(Character* character, int shown)
    {
        if (!g_debug)
            return;

        static std::string last;

        char line[256];
        sprintf_s(line,
            "LiveXpBars: character=%s selected=%d setup=%d rows=%d shown=%d",
            character ? "yes" : "no",
            (ou && ou->player)
                ? static_cast<int>(ou->player->selectedCharacters.size()) : -1,
            g_setup ? 1 : 0,
            static_cast<int>(g_rows.size()),
            shown);

        if (line == last)
            return;

        last = line;
        DebugLog(line);
    }


    // ---------------------------------------------------------------
    // Кадр
    // ---------------------------------------------------------------

    Margins PanelMargins()
    {
        Margins m = { 0, 0, 0, 0 };

        if (g_setup)
        {
            m.left = kSetupPadLeft;
            m.right = kSetupPadRight;
            m.top = m.bottom = kSetupPad;
        }
        else if (IsGamePanel())
        {
            m.left = m.right = kGamePanelPad + 2;
            m.top = kGamePanelPad;
            m.bottom = kGamePanelPad - kGamePanelBottomTrim;
        }
        else if (HasBackground())
        {
            m.left = m.right = m.top = kBackgroundPad;
            m.bottom = kBackgroundPad - kBottomTrim;
        }

        return m;
    }


    void Tick()
    {
        const unsigned long long now = GetTickCount64();
        const float dt = g_lastTick
            ? static_cast<float>(now - g_lastTick) / 1000.0f : 0.0f;
        g_lastTick = now;

        if (gui == NULL)
            return;

        // Пока игра не построила таблицу названий, показывать нечего -
        // и в интерфейс лезть незачем.
        const std::vector<std::pair<int, std::string> >& list = StatList(now);
        if (list.empty())
            return;

        if (!g_firstTickLogged)
        {
            g_firstTickLogged = true;
            if (g_debug)
                DebugLog("LiveXpBars: first tick, stat names ready");
        }

        EnsurePanel();
        PollHotkey();

        Character* const character =
            gui->getSelectedPlayerCharacter().getCharacter();

        // После долгой паузы кадра (загрузка сейва, переход в меню)
        // прежние значения ничего не значат: новый персонаж мог оказаться
        // по тому же адресу, что прежний, и тогда «выросли» бы сразу все
        // навыки, которые у него выше. Пауза в две секунды - заведомо не
        // обычный кадр.
        if (dt > 2.0f)
            g_seen.clear();

        if (character != g_watched)
        {
            // Прежние значения ни о чём не говорят: у нового персонажа
            // свои. Без этого при переключении вспыхнули бы все навыки.
            g_watched = character;
            g_seen.clear();

            // И строки прежнего гасим: иначе они ещё HoldSeconds держались
            // бы с числами уже нового персонажа (строка идёт за значением,
            // пока видна), а с AlwaysShow не гасли бы вовсе.
            for (std::map<int, Row>::iterator it = g_rows.begin();
                 it != g_rows.end(); ++it)
            {
                it->second.lastGain = 0;
            }
        }

        CharStats* const stats = character ? character->getStats() : NULL;
        const bool blocked = (stats == NULL) || SquadSelected();

        const unsigned long long hold =
            static_cast<unsigned long long>(g_hold * 1000.0f);

        // 1. Что подросло. Прибавку видно ровно один кадр, поэтому
        //    отмечаем её время, а не флаг «растёт».
        if (!blocked && !g_setup)
        {
            for (size_t i = 0; i < list.size(); ++i)
            {
                const int id = list[i].first;
                const float value =
                    stats->getStat(static_cast<StatsEnumerated>(id), true);

                const std::map<int, float>::iterator seen = g_seen.find(id);
                const bool grew = seen != g_seen.end()
                               && value > seen->second + 1e-5f;
                g_seen[id] = value;

                if (grew)
                {
                    Row& row = EnsureRow(id, list[i].second);
                    row.lastGain = now;

                    // Заполняем сразу: иначе первый кадр строка выходит
                    // с пустыми числами и нулевой полосой.
                    FillRow(row, list[i].second, value);
                    continue;
                }

                // Пока навык в удержании, число и полоса должны идти за
                // ним, даже если в этом кадре прибавки не было.
                const std::map<int, Row>::iterator known = g_rows.find(id);
                if (known != g_rows.end() && known->second.alpha > 0.0f)
                    FillRow(known->second, list[i].second, value);
            }
        }

        // 2. Образцы в настройке: в пустой рамке не видно, что двигаешь.
        if (g_setup)
        {
            for (size_t i = 0; i < list.size()
                            && static_cast<int>(i) < kSetupRows; ++i)
            {
                const int id = list[i].first;
                const float value = stats
                    ? stats->getStat(static_cast<StatsEnumerated>(id), true)
                    : 0.0f;

                Row& row = EnsureRow(id, list[i].second);
                row.alpha = 1.0f;
                FillRow(row, list[i].second, value);
            }
        }

        // 3. Яркость.
        const float up = g_fadeIn > 0.01f ? dt / g_fadeIn : 1.0f;
        const float down = g_fadeOut > 0.01f ? dt / g_fadeOut : 1.0f;

        if (!g_setup)
        {
            for (std::map<int, Row>::iterator it = g_rows.begin();
                 it != g_rows.end(); ++it)
            {
                Row& row = it->second;
                // AlwaysShow: качнувшийся хоть раз навык не гаснет, пока
                // выбран тот же персонаж.
                const bool active = !blocked
                                 && row.lastGain != 0
                                 && (g_alwaysShow || (now - row.lastGain) < hold);

                row.alpha += active ? up : -down;

                if (row.alpha > 1.0f) row.alpha = 1.0f;
                if (row.alpha < 0.0f) row.alpha = 0.0f;
            }
        }

        // 4. Раскладка: только видимые, сверху вниз, в порядке навыков.
        const Margins pad = PanelMargins();
        const int leftOffset = pad.left;
        const int topOffset = pad.top + (g_setup ? kRowHeight : 0);

        int index = 0;
        float brightest = 0.0f;   // подложка гаснет вместе со строками
        for (size_t i = 0; i < list.size(); ++i)
        {
            const std::map<int, Row>::iterator found =
                g_rows.find(list[i].first);
            if (found == g_rows.end())
                continue;

            Row& row = found->second;

            // В настройке показываем ИМЕННО образцы - первые строки
            // списка, - а не первые попавшиеся уже созданные. Иначе в
            // рамку попадали строки от недавно качавшихся навыков, с
            // прозрачностью около нуля, и рамка снова выглядела пустой.
            const bool visible = g_setup
                ? (static_cast<int>(i) < kSetupRows)
                : (row.alpha > 0.004f);

            if (visible)
            {
                PlaceRow(row, index++, leftOffset, topOffset);
                if (row.alpha > brightest)
                    brightest = row.alpha;
            }

            ShowRow(row, visible);
        }

        // 5. Панель. В настройке высота постоянная - иначе рамка прыгала
        //    бы вслед за появлением строк.
        g_hint->setVisible(g_setup);
        if (g_setup)
        {
            g_hint->setCaption(Tr("Drag to place. SHIFT+B when done."));
            g_hint->setCoord(pad.left, pad.top, g_width, kRowHeight);
        }

        const int rows = g_setup ? kSetupRows : index;
        const int content = topOffset + rows * kRowHeight + (rows > 1 ? (rows - 1) * g_rowGap : 0);

        const int height = content + pad.bottom;
        const int width = pad.left + g_width + pad.right;

        ClampPosition(height);
        g_panel->setCoord(g_left - pad.left, g_top - pad.top, width,
                          height > 0 ? height : kRowHeight);
        // F7 прячет интерфейс игры - и нас вместе с ним (HideWithGUI).
        // Настройку места не прячем: её включили руками.
        const bool guiHidden = g_hideWithGui && !g_setup && GameGuiHidden();
        g_panel->setVisible((index > 0 || g_setup) && !guiHidden);

        // В настройке подложку прячем: там своя рамка.
        if (g_backdrop != NULL)
        {
            const bool shown = !g_setup && index > 0;
            g_backdrop->setVisible(shown);
            if (shown)
            {
                g_backdrop->setCoord(0, 0, width, height);
                g_backdrop->setAlpha(g_backgroundOpacity * brightest);
            }
        }

        Report(character, index);
    }


    void (*g_origMainLoop)(GameWorld* thisptr, float time) = NULL;

    void MainLoop_hook(GameWorld* thisptr, float time)
    {
        g_origMainLoop(thisptr, time);

        if (g_enabled)
            Tick();
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.

// Перечитывание: панель с полосами собрана с прежними цветами и
// подложкой - убираем её, следующий кадр соберёт заново.
static void __cdecl McmReload()
{
    g_pendingTheme.clear();
    LoadSettings();
    g_themeResolved = false;
    if (g_panel != NULL)
        MyGUI::Gui::getInstance().destroyWidget(g_panel);
    g_panel = 0;
    g_backdrop = 0;
    g_hint = 0;
    g_rows.clear();
    g_setup = false;
    g_dragging = false;
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "LiveXpBars", "Live Xp Bars", ini.c_str(), &McmReload);
    if (api->version >= 2)
        api->info(api, Tr("On-screen experience bars of the skills being trained right now."));
    api->section(api, Tr("Bars"));
    api->toggle(api, "Bars", "Enabled", Tr("Enabled"), Tr("Bars of the skills being trained right now. SHIFT+B - move them."), 1, MCM_RESTART);
    api->number(api, "Bars", "HoldSeconds", Tr("Hold, seconds"), Tr("How long a bar stays after experience is gained. Without it a bar would only blink."), 3.0f, 0.0f, 30.0f, 1, 0);
    api->number(api, "Bars", "FadeInSeconds", Tr("Fade in, seconds"), Tr("How long a bar takes to appear."), 1.0f, 0.0f, 10.0f, 1, 0);
    api->number(api, "Bars", "FadeOutSeconds", Tr("Fade out, seconds"), Tr("How long a bar takes to fade after the hold time is over."), 5.0f, 0.0f, 30.0f, 1, 0);
    api->toggle(api, "Bars", "AlwaysShow", Tr("Keep trained skills on screen"), Tr("A skill that gained experience stays on screen until another character is selected. Off - it fades after the hold time."), 0, 0);
    api->toggle(api, "Bars", "HideForSquad", Tr("Hide when several are selected"), Tr("Each character trains their own skills - with several selected the bars would be a mess."), 1, 0);
    api->toggle(api, "Bars", "HideWithGUI", Tr("Hide with the interface"), Tr("Hide the bars together with the game interface, F7. Off - the bars stay when the interface is hidden."), 1, 0);
    api->integer(api, "Bars", "RowSpacing", Tr("Space between rows, px"), Tr("Extra space between the rows of the bars. Below zero - the rows move closer."), 0, -8, 40, 0);
    api->integer(api, "Bars", "Width", Tr("Row width, px"), Tr("Width of a row: the skill name, level and percentage come first, the rest goes to the bar. The position is set with SHIFT+B."), 460, 240, 1200, 0);
    api->toggle(api, "Bars", "ColourLevel", Tr("Colour the level"), Tr("Colour the skill level by its value, with the level colours below."), 1, 0);
    api->toggle(api, "Bars", "ColourPercent", Tr("Colour the percentage"), Tr("Colour the percentage by progress to the next level: dim at the start, bright near the level."), 1, 0);
    api->section(api, Tr("Colours"));
    api->toggle(api, "Colours", "UseGameTheme", Tr("Colours from the interface theme"), Tr("0 - fallback colours only. Turn off first if the game crashes after loading a save."), 1, 0);
    api->toggle(api, "Colours", "SmoothColours", Tr("Smooth colours"), Tr("Colours of the level, percentage and fill blend smoothly between the level steps below. Off - they change once every ten."), 1, 0);
    api->colour(api, "Colours", "Name", Tr("Skill name"), Tr("skin - as the game draws it, auto - interface theme, or #RRGGBB."), "skin", 0);
    api->colour(api, "Colours", "Plain", Tr("Level and percentage"), Tr("When their colouring is off. skin - as the game draws it, auto - interface theme, or #RRGGBB."), "skin", 0);
    api->colour(api, "Colours", "Hint", Tr("Hint"), Tr("The hint in the setup mode, SHIFT+B. skin - as the game draws it, auto - interface theme, or #RRGGBB."), "skin", 0);
    api->colour(api, "Colours", "BarColour", Tr("Bar fill"), Tr("value - by progress, like the percentage, skin - as the game draws it, or #RRGGBB."), "value", 0);
    api->text(api, "Colours", "Background", Tr("Background"), Tr("Under the bars, where the ground is light. none - no background, game - the game's panel with a frame, or #RRGGBB."), "none", 0);
    api->number(api, "Colours", "BackgroundOpacity", Tr("Background opacity"), Tr("0 - transparent, 1 - solid. 0.5 to 0.7 - the text reads well and the world still shows through."), 0.6f, 0.0f, 1.0f, 2, 0);
    api->section(api, Tr("Skill level colours"));
    {
        static const char* const keys[] = { "Level0", "Level10", "Level20", "Level30", "Level40",
            "Level50", "Level60", "Level70", "Level80", "Level90", "Level100", "Above100" };
        static const char* const defs[] = { "#8C3A3A", "#A34A42", "#B25E42", "#BE7644", "#C79149",
            "#CBAA52", "#C2B25A", "#AEB25E", "#96AC5F", "#7EA560", "#6C9E63", "#9A7BC8" };
        static const char* const names[] = { "0-9", "10-19", "20-29", "30-39", "40-49", "50-59",
            "60-69", "70-79", "80-89", "90-99", "100", NULL };
        for (int i = 0; i < 12; ++i)
            api->colour(api, "Colours", keys[i], names[i] ? names[i] : Tr("Above 100"), Tr("Colour of skill values in this range, #RRGGBB. Above 100 happens only with an equipment bonus."), defs[i], 0);
    }
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "Bars", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: who is selected, how many characters are selected and how many skills are growing."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("LiveXpBars: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(
            KenshiLib::GetRealAddress(
                &GameWorld::_NV_mainLoop_GPUSensitiveStuff),
            MainLoop_hook,
            &g_origMainLoop))
    {
        ErrorLog("LiveXpBars: could not hook the game loop, bars are off");
        return;
    }

    DebugLog("LiveXpBars: installed");
}
