// StatColours - раскраска чисел в окне характеристик персонажа и показ
// прибавки от надетого снаряжения.
//
// ЗАЧЕМ СВОЙ. Есть в мастерской мод StatsColor (ColoredStats) того же
// назначения, но на локализованной игре он не работает вовсе. Внутри его
// DLL лежат ровно пять широких строк - STRENGTH, TOUGHNESS, Katanas,
// Science, Weapon smith - по которым он ищет нужные столбцы, сравнивая их
// с подписями на экране. На русском там «Сила», «Катаны», «Ковка оружия»,
// ни один якорь не совпадает, и мод молча не красит ничего. Для китайской
// аудитории автора он сломан ровно так же.
//
// КАК СДЕЛАНО У НАС. Ни одной строки на экране мы не читаем и ни одной не
// сравниваем с зашитым текстом. Игра держит колонки окна отдельными
// полями: CharacterStatsWindow::attributesDatapanel и skills1..4Datapanel.
// Обходим строки панели и красим правую ячейку (DataPanelLine::w2).
//
// ПОЧЕМУ ХУК НА КАДР, А НЕ НА ОКНО. Строки в панель качеств дописывает и
// чужой мод KillCounter, причём каждый кадр из GameWorld. Если красить
// только по событиям окна, его строки остаются неокрашенными - они
// появляются позже. Поэтому идём тем же путём: каждый кадр обходим все
// открытые окна из gui->characterStatsWindows.
//
// ПРИБАВКА ОТ СНАРЯЖЕНИЯ. `CharStats::getStat(стат, unmodified)` отдаёт
// одно и то же значение с учётом надетого и без него; разница и есть
// вклад брони. Осталось понять, какой строке какой стат соответствует.
// Разведка показала, что `DataPanelLine::keyValue` у игровых строк - это
// отображаемое имя: «Катаны», «Сила». Само по себе оно от языка зависит,
// но сравнивать его с зашитым текстом мы и не будем: таблицу имён строим
// из самой игры - `CharacterStatsWindow::getStat(тип)->name` по всем
// значениям StatsEnumerated. То есть сверяем строку игры с её же строкой,
// и на любом языке совпадёт.
//
// МНОЖИТЕЛИ ОПЫТА РАСЫ. Под «Качествами» - блок с теми навыками, у которых
// расовый множитель опыта не x1 (как в заброшенном моде RaceMetrics).
// Множитель - RaceData::getStatMod(стат), таблица statMods самой расы.
// Первая версия брала CharStats::getStatMultiplier - и это оказался не
// расовый бонус, а общий множитель опыта с текущими условиями: значения
// плыли (x0,51 почти у всего), а расовые - постоянные. Блок строится внутри
// самого окна, детьми его корня, - поэтому двигается вместе с окном и
// уничтожается вместе с ним, указатели на него мы не храним.
//
// СКОРОСТЬ ПРОКАЧКИ. За заголовком навыка в описании - «(Тек. скор.
// прокачки: 0,510)»: расовый множитель, умноженный на то, во сколько
// раз опыт режется на текущем уровне. Вторую часть считает XP_Overhaul
// (его кривая и множители из настроек) - мы спрашиваем у него
// XPO_CurrentXpRate; без него - ванильная кривая (1 - уровень/101)^2.
// CharStats::getStatMultiplier здесь ни при чём: это сила действия
// навыка («Ковка оружия (скорость): 0,51x»), а не опыт.

#define KLOC_DOMAIN "stat_colours"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <GameTheme.h>
#include <HoldKey.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/MedicalSystem.h>
#include <kenshi/Enums.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/CharacterStatsWindow.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/RaceData.h>
#include <kenshi/gui/DataPanelLine.h>
#include <kenshi/util/lektor.h>
#include <kenshi/util/StringPair.h>

#include <mygui/MyGUI_Colour.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>


namespace
{
    // ---------------------------------------------------------------
    // Палитра
    //
    // Двенадцать ступеней по уровню навыка плюс три служебных цвета.
    // Все они настраиваются в StatColours.ini рядом с DLL; файл пишется
    // сам при первом запуске, читается один раз при старте.
    //
    // Значения по умолчанию приглушённые, под палитру самой игры. Взятая
    // изначально из StatsColor шкала была кислотной: чистые #FFFF4D и
    // #4DFF4D на тёмной панели светятся неоном и спорят с остальным
    // интерфейсом, где всё держится на землистых тонах.
    // ---------------------------------------------------------------

    const int PALETTE_SIZE = 12;

    // Ключ в ini -> что он красит. Порядок тот же, что у ступеней.
    const char* const PALETTE_KEYS[PALETTE_SIZE] =
    {
        "Level0", "Level10", "Level20", "Level30",
        "Level40", "Level50", "Level60", "Level70",
        "Level80", "Level90", "Level100", "Above100"
    };

    const char* const PALETTE_DEFAULT[PALETTE_SIZE] =
    {
        "#8C3A3A",   // 0-9     тусклый кирпич
        "#A34A42",   // 10-19
        "#B25E42",   // 20-29
        "#BE7644",   // 30-39
        "#C79149",   // 40-49
        "#CBAA52",   // 50-59   охра
        "#C2B25A",   // 60-69
        "#AEB25E",   // 70-79
        "#96AC5F",   // 80-89
        "#7EA560",   // 90-99
        "#6C9E63",   // ровно 100  приглушённая зелень
        "#9A7BC8"    // выше 100   фиолетовый: бонус от снаряжения
    };

    // Служебные цвета по умолчанию берутся не отсюда, а из палитры самой
    // игры - data/gui/colours/kenshi_colours.xml, с учётом того мода на
    // интерфейс, который её перекрыл. Эти значения нужны лишь на случай,
    // если файла нет или в нём нет нужного ключа.
    const char* const BRACKET_DEFAULT = "#8E8E8E";   // скобки
    const char* const PLUS_DEFAULT    = "#7EA560";   // прибавка
    const char* const MINUS_DEFAULT   = "#B2504A";   // штраф
    const char* const LABEL_DEFAULT   = "#AFA68B";   // подписи в разбивке

    // Ключи из kenshi_colours.xml, откуда берём эти цвета.
    const char* const BRACKET_THEME_KEY = "GreyedBright";
    const char* const PLUS_THEME_KEY    = "GoodBright";
    const char* const MINUS_THEME_KEY   = "BadBright";
    const char* const LABEL_THEME_KEY   = "Main";

    char g_palette[PALETTE_SIZE][8];
    char g_colourBracket[8];
    char g_colourPlus[8];
    char g_colourMinus[8];
    char g_colourLabel[8];

    // По умолчанию скобка слева от числа - так просили. Имей в виду:
    // у длинных названий («Точная стрельба», «Скрытность») текст
    // значения шире своей ячейки, MyGUI его не обрезает, и он уходит
    // поверх соседа. Справа такого не бывает - там пустой промежуток
    // до следующего столбца. Переключается BracketSide в настройках.
    bool g_bracketOnLeft = true;

    // [Debug] Debug=1 - выкладывать в журнал разбор выбранного
    // навыка. По умолчанию выключено: строка на каждый навык.
    bool g_debug = false;

    // [RacialXP] Show=0 - не показывать блок множителей опыта расы.
    bool g_showRacial = true;

    // [RacialXP] ShowRate=0 - не писать скорость прокачки за заголовком.
    bool g_showRate = true;

    // [RacialXP] HoldKey - блок виден, только пока клавиша зажата (как
    // GearCompare и AssignedWorkers). 0 - виден всегда.
    HoldKey::Key g_racialHoldKey;   // ALT; vk 0 - всегда (см. HoldKey.h)

    const char* COLOUR_PLAIN = g_colourBracket;
    const char* COLOUR_PLUS  = g_colourPlus;
    const char* COLOUR_MINUS = g_colourMinus;
    const char* COLOUR_LABEL = g_colourLabel;


    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    std::string IniPath()
    {
        char modulePath[MAX_PATH] = {};

        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), modulePath, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string path(modulePath, length);
        const std::string::size_type slash = path.find_last_of("\\/");
        if (slash == std::string::npos)
            return std::string();

        return path.substr(0, slash + 1) + "StatColours.ini";
    }


    // Палитра игры живёт в общем shared/GameTheme.h: её читает и
    // Character Inspector. Ключи «Main», «Greyed», «Good», «Bad» - это
    // тема самой Kenshi, и моды на интерфейс её перекрывают.

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

    void CacheColours();

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
            DebugLog("StatColours: game palette is off, keeping the defaults");
            g_pendingTheme.clear();
            CacheColours();
            return;
        }

        DebugLog("StatColours: asking MyGUI for the palette");

        for (size_t i = 0; i < g_pendingTheme.size(); ++i)
        {
            const PendingColour& item = g_pendingTheme[i];
            const char* const themed =
                GameTheme::Hex(item.themeKey, item.fallback);

            if (themed != NULL)
                strcpy_s(item.slot, 8, themed);
        }

        DebugLog("StatColours: palette taken");
        g_pendingTheme.clear();
        CacheColours();
    }



    // Цвет принимаем только в виде #RRGGBB. Кривое значение молча не
    // проглатываем: строка в журнале дешевле, чем потом гадать, почему
    // одна ступень осталась старого цвета.
    bool ValidColour(const char* text)
    {
        if (text == NULL || strlen(text) != 7 || text[0] != '#')
            return false;

        for (int i = 1; i < 7; ++i)
        {
            const char c = text[i];
            const bool hex = (c >= '0' && c <= '9')
                          || (c >= 'a' && c <= 'f')
                          || (c >= 'A' && c <= 'F');
            if (!hex)
                return false;
        }

        return true;
    }


    void ReadColour(const std::string& ini, const char* key,
                    const char* fallback, char* out,
                    const char* themeKey = NULL)
    {
        char buffer[64] = {};

        GetPrivateProfileStringA("Colours", key,
                                 themeKey ? "auto" : fallback,
                                 buffer, sizeof(buffer), ini.c_str());

        // «auto» - взять из палитры игры. Так служебные цвета едут
        // вместе с темой интерфейса, а не спорят с ней.
        if (themeKey != NULL && _stricmp(buffer, "auto") == 0)
        {
            // Сейчас - запасной, чтобы плагин был работоспособен с
            // первого кадра; настоящий подставится, когда поднимется
            // MyGUI.
            strcpy_s(out, 8, fallback);

            PendingColour pending;
            pending.slot = out;
            pending.themeKey = themeKey;
            pending.fallback = fallback;
            g_pendingTheme.push_back(pending);
            return;
        }

        if (!ValidColour(buffer))
        {
            char note[160];
            sprintf_s(note, "StatColours: %s='%s' is not #RRGGBB, using %s",
                      key, buffer, fallback);
            ErrorLog(note);
            strcpy_s(out, 8, fallback);
            return;
        }

        strcpy_s(out, 8, buffer);
    }


    // Файл пишем сами, а не через WritePrivateProfileString: так в нём
    // остаются пояснения. Игрок открывает его и сразу видит, что менять.
    void WriteDefaultIni(const std::string& ini)
    {
        FILE* f = NULL;
        if (fopen_s(&f, ini.c_str(), "w") != 0 || f == NULL)
            return;

        // Пояснения идут через каталог: в самом бинарнике русского
        // быть не должно, и у англоязычного игрока файл выйдет
        // по-английски.
        static const char* const HEAD[] = {
            "StatColours - colours for the numbers in the stats window.",
            "Read once, when the game starts.",
            "",
            "A colour is #RRGGBB, as in HTML. A value that makes no sense",
            "is ignored, and a line about it goes to the RE_Kenshi log.",
            "",
            "LevelN is the colour for skills from N to N+9. Level100 is",
            "exactly a hundred, Above100 is over it - which only happens",
            "with a bonus from equipment.",
            NULL
        };

        static const char* const TAIL[] = {
            "The bracket with the equipment correction: (+3) or (-4).",
            "Brackets is the brackets themselves, Bonus and Penalty the",
            "number inside. BracketSide is which side of the value it",
            "goes on: left or right. On the left it can overlap a long",
            "skill name - then put right, where it is in nobody's way.",
            NULL
        };

        static const char* const AUTO[] = {
            "auto - take it from the game palette (kenshi_colours.xml),",
            "that is, follow the interface theme and any mod over it.",
            NULL
        };

        static const char* const LABELS[] = {
            "Labels - the captions in the breakdown under the skill.",
            NULL
        };

        // Пустую строку через Tr не гоняем: по пустому msgid
        // gettext отдаёт заголовок каталога.
        for (int i = 0; HEAD[i] != NULL; ++i)
        {
            if (*HEAD[i] == '\0')
                fputs(";\n", f);
            else
                fprintf(f, "; %s\n", Tr(HEAD[i]));
        }

        fputs("\n[Colours]\n", f);

        for (int i = 0; i < PALETTE_SIZE; ++i)
            fprintf(f, "%s=%s\n", PALETTE_KEYS[i], PALETTE_DEFAULT[i]);

        fputc('\n', f);
        for (int i = 0; TAIL[i] != NULL; ++i)
            fprintf(f, "; %s\n", Tr(TAIL[i]));
        fputs("BracketSide=left\n", f);

        for (int i = 0; AUTO[i] != NULL; ++i)
            fprintf(f, "; %s\n", Tr(AUTO[i]));
        fputs("Brackets=auto\n", f);
        fputs("Bonus=auto\n", f);
        fputs("Penalty=auto\n", f);

        for (int i = 0; LABELS[i] != NULL; ++i)
            fprintf(f, "; %s\n", Tr(LABELS[i]));
        fputs("Labels=auto\n", f);

        fprintf(f, "\n; %s\n", Tr("Racial XP multipliers under the attributes: 1 shows them, 0 hides."));
        fprintf(f, "; %s\n", Tr("HoldKey: any key (ALT, CTRL, SHIFT, F, CTRL+B...) - shown only while held; NONE - always."));
        fprintf(f, "; %s\n", Tr("ShowRate: current XP rate after the skill name in the description: 1 shows it, 0 hides."));
        fputs("[RacialXP]\nShow=1\nHoldKey=ALT\nShowRate=1\n", f);

        fclose(f);
    }


    void LoadColours()
    {
        // Сначала значения по умолчанию: если файла нет и создать его не
        // вышло, плагин всё равно красит.
        for (int i = 0; i < PALETTE_SIZE; ++i)
            strcpy_s(g_palette[i], sizeof(g_palette[i]), PALETTE_DEFAULT[i]);
        strcpy_s(g_colourBracket, sizeof(g_colourBracket), BRACKET_DEFAULT);
        strcpy_s(g_colourPlus, sizeof(g_colourPlus), PLUS_DEFAULT);
        strcpy_s(g_colourMinus, sizeof(g_colourMinus), MINUS_DEFAULT);
        strcpy_s(g_colourLabel, sizeof(g_colourLabel), LABEL_DEFAULT);

        const std::string ini = IniPath();
        if (ini.empty())
        {
            ErrorLog("StatColours: cannot resolve the ini path, using defaults");
            CacheColours();
            return;
        }

        if (GetFileAttributesA(ini.c_str()) == INVALID_FILE_ATTRIBUTES)
            WriteDefaultIni(ini);

        for (int i = 0; i < PALETTE_SIZE; ++i)
            ReadColour(ini, PALETTE_KEYS[i], PALETTE_DEFAULT[i], g_palette[i]);

        char side[32] = {};
        GetPrivateProfileStringA("Colours", "BracketSide", "left",
                                 side, sizeof(side), ini.c_str());
        g_bracketOnLeft = (_stricmp(side, "right") != 0);

        ReadColour(ini, "Brackets", BRACKET_DEFAULT, g_colourBracket,
                   BRACKET_THEME_KEY);
        ReadColour(ini, "Bonus", PLUS_DEFAULT, g_colourPlus,
                   PLUS_THEME_KEY);
        ReadColour(ini, "Penalty", MINUS_DEFAULT, g_colourMinus,
                   MINUS_THEME_KEY);
        ReadColour(ini, "Labels", LABEL_DEFAULT, g_colourLabel,
                   LABEL_THEME_KEY);

        g_showRacial = GetPrivateProfileIntA("RacialXP", "Show", 1,
                                             ini.c_str()) != 0;
        g_showRate = GetPrivateProfileIntA("RacialXP", "ShowRate", 1,
                                           ini.c_str()) != 0;
        {
            char key[32] = {};
            GetPrivateProfileStringA("RacialXP", "HoldKey", "ALT", key,
                                     sizeof(key), ini.c_str());
            g_racialHoldKey = HoldKey::Parse(key);   // любая клавиша, «NONE» - всегда
        }
        g_debug = (GetPrivateProfileIntA("Debug", "Debug", 0,
                                         ini.c_str()) != 0);
        CacheColours();
        g_useGameTheme = GetPrivateProfileIntA("Colours", "UseGameTheme", 1, ini.c_str()) != 0;
    }


    int PaletteIndex(long value)
    {
        int index;

        if (value < 0)
            index = 0;
        else if (value < 100)
            index = static_cast<int>(value / 10);
        else if (value == 100)
            index = 10;
        else
            index = 11;

        if (index < 0)
            index = 0;
        if (index >= PALETTE_SIZE)
            index = PALETTE_SIZE - 1;

        return index;
    }


    MyGUI::Colour ColourFromHex(const char* hex)
    {
        unsigned r = 0, g = 0, b = 0;
        sscanf_s(hex + 1, "%2x%2x%2x", &r, &g, &b);
        return MyGUI::Colour(r / 255.0f, g / 255.0f, b / 255.0f);
    }


    // Те же цвета уже разобранными. Красим каждый кадр, а sscanf на
    // каждую строку окна ради одного и того же числа ни к чему;
    // пересчитываются, когда меняется палитра: при чтении ini и когда
    // подставилась тема игры.
    MyGUI::Colour g_paletteColour[PALETTE_SIZE];
    MyGUI::Colour g_plusColour;
    MyGUI::Colour g_minusColour;
    MyGUI::Colour g_labelColour;
    MyGUI::Colour g_bracketColour;

    void CacheColours()
    {
        for (int i = 0; i < PALETTE_SIZE; ++i)
            g_paletteColour[i] = ColourFromHex(g_palette[i]);
        g_plusColour = ColourFromHex(g_colourPlus);
        g_minusColour = ColourFromHex(g_colourMinus);
        g_labelColour = ColourFromHex(g_colourLabel);
        g_bracketColour = ColourFromHex(g_colourBracket);
    }


    // ---------------------------------------------------------------
    // Разбор подписи
    // ---------------------------------------------------------------

    // Убирает цветовые теги MyGUI. Они здесь не наша выдумка: игра сама
    // пишет их в подписи - заголовки групп приходят как «#b7a074 -Оружие- ».
    // Нам это нужно, чтобы разобрать число в строке, которую мы же в
    // прошлом кадре и раскрасили.
    std::string StripTags(const std::string& text)
    {
        std::string out;
        out.reserve(text.size());

        for (std::string::size_type i = 0; i < text.size(); )
        {
            if (text[i] == '#')
            {
                if (i + 1 < text.size() && text[i + 1] == '#')
                {
                    out += '#';           // «##» - экранированная решётка
                    i += 2;
                    continue;
                }
                if (i + 7 <= text.size())
                {
                    i += 7;               // «#RRGGBB»
                    continue;
                }
            }
            out += text[i];
            ++i;
        }

        return out;
    }


    // Число - и ничего кроме числа. Если в строке уже стоят наши скобки,
    // берём то, что после них.
    bool ParseValue(const std::string& raw, long& value)
    {
        std::string text = StripTags(raw);

        const std::string::size_type bracket = text.rfind(')');
        std::string::size_type i =
            (bracket == std::string::npos) ? 0 : bracket + 1;

        std::string::size_type end = text.size();

        while (i < end && (text[i] == ' ' || text[i] == '\t'))
            ++i;
        while (end > i && (text[end - 1] == ' ' || text[end - 1] == '\t'))
            --end;

        if (i >= end)
            return false;

        bool negative = false;
        if (text[i] == '-')
        {
            negative = true;
            ++i;
        }

        if (i >= end)
            return false;

        long result = 0;
        for (std::string::size_type k = i; k < end; ++k)
        {
            const char c = text[k];
            if (c < '0' || c > '9')
                return false;
            result = result * 10 + (c - '0');
            if (result > 1000000)
                return false;
        }

        value = negative ? -result : result;
        return true;
    }


    // ---------------------------------------------------------------
    // Имя строки -> характеристика
    // ---------------------------------------------------------------

    std::map<std::string, StatsEnumerated> g_byName;
    std::map<int, std::string> g_nameOf;      // обратная: тип -> имя
    bool g_mapBuilt = false;

    void BuildNameMap()
    {
        if (g_mapBuilt)
            return;

        // Таблица окна собирается лениво. Флаг StatsBuilt из KenshiLib
        // не экспортируется, но он и не нужен: пока таблицы нет, getStat
        // отдаёт NULL, карта остаётся пустой, и мы просто попробуем ещё
        // раз на следующем кадре.
        for (int t = STAT_NONE + 1; t < STAT_END; ++t)
        {
            const StatsEnumerated type = static_cast<StatsEnumerated>(t);
            CharacterStatsWindow::Stat* stat =
                CharacterStatsWindow::getStat(type);
            if (stat == NULL || stat->name.empty())
                continue;
            g_byName[stat->name] = type;
            g_nameOf[static_cast<int>(type)] = stat->name;

            // Второе написание того же стата. У части строк подпись окна
            // и имя из CharStats расходятся; кладём оба ключа, чтобы
            // совпало в любом случае. Обе строки дают нам сами игровые
            // данные, так что от языка это по-прежнему не зависит.
            const std::string alias = CharStats::getStatName(type);
            if (!alias.empty() && g_byName.find(alias) == g_byName.end())
                g_byName[alias] = type;
        }

        if (!g_byName.empty())
        {
            g_mapBuilt = true;
            char note[96];
            sprintf_s(note, "StatColours: stat name map built, %d entries",
                      static_cast<int>(g_byName.size()));
            DebugLog(note);
        }
    }


    // Вклад надетого: разница между значением с учётом снаряжения и без.
    // Возвращает false, если строке не соответствует ни одна
    // характеристика (заголовок группы, строки чужих модов).
    // Действующее значение стата - то, которым игра пользуется прямо
    // сейчас, со всем надетым, перегрузом, ранами и режимом боя.
    //
    // Сначала я считал поправку как разность getStat(стат, false) и
    // getStat(стат, true). Она ловит только множители - «+10% к атаке», -
    // а ровные прибавки вроде «+4 к атаке» проходят мимо. Это заметил
    // Asur и показал, как сделано в моде ShowEffectiveStats
    // (DougTownsend): у четырёх боевых статов есть отдельные геттеры,
    // и ситуативные поправки игра применяет именно в них, а общий
    // getStat их не знает. Здесь так же.
    //
    // Своей арифметики нет нигде: каждое число отдаёт сама игра.
    float EffectiveValue(CharStats* stats, StatsEnumerated stat)
    {
        switch (stat)
        {
        case STAT_MELEE_ATTACK:
            // Именно _melee. Общий getMeleeAttack у безоружного отдаёт
            // рукопашный рейтинг, и строка «Атака» показывала бы тогда
            // боевые искусства.
            return stats->getMeleeAttack_melee();
        case STAT_MELEE_DEFENCE:
            // true - с учётом боевой стойки. Именно так считает боевая
            // панель игры: при включённом «Блоке» она показывает
            // «Защита: 27 (+11)», а без стойки вышло бы 7 и «-9».
            // Спорить с соседним окном игры мы не хотим.
            return stats->getMeleeDefence(true);
        case STAT_MARTIALARTS:
            return stats->getMeleeAttack_unarmed(true);
        case STAT_DODGE:
            return stats->getDodge(true);
        default:
            return stats->getStat(stat, false);
        }
    }


    // Ровный бонус снаряжения - там, где игра его хранит отдельно.
    // Возвращает false, если у стата такого поля нет.
    //
    // factorEnvironment ОБЯЗАТЕЛЬНО false. С true геттеры подмешивают
    // skillBonusIndoors - прибавку за бой в помещении, к надетому
    // отношения не имеющую. На голом новичке в доме журнал показал
    // «fields: A=0 D=0 U=0 indoors=8», и все три навыка получали
    // одинаковую скобку «(+8)» на пустом месте. Помещение уходит
    // отдельной строкой в разбор, там ему и место.
    bool FlatEquipmentBonus(CharStats* stats, StatsEnumerated stat, int& bonus)
    {
        switch (stat)
        {
        case STAT_MELEE_ATTACK:
            bonus = stats->skillBonusAttack_melee(false);
            return true;
        case STAT_MELEE_DEFENCE:
            bonus = stats->skillBonusDefence(false);
            return true;
        case STAT_MARTIALARTS:
            bonus = stats->skillBonusUnarmed_forGUI(false);
            return true;
        case STAT_PERCEPTION:
            // Геттера у него нет, но поле открытое - игра складывает
            // сюда прибавку к восприятию от надетого.
            bonus = stats->skillBonusPerception;
            return true;
        default:
            return false;
        }
    }


    // Уворот считается мимо getStat: база берётся из навыка, а поправки
    // складываются поверх тремя отдельными слагаемыми. Журнал:
    // dodge 2.263 -> 1.358 при gear=+0.226, inj=-1.131, enc=-0.000.
    // Нам нужно только первое.
    bool DodgeGearBonus(CharStats* stats, StatsEnumerated stat, float& bonus)
    {
        if (stat != STAT_DODGE)
            return false;

        bonus = stats->getDodgePenalty_gear();
        return true;
    }


    // Поправка от снаряжения.
    //
    // Два разных случая, и путать их нельзя - на этом я обжёгся дважды.
    //
    // Атака, защита и боевые искусства: игра держит их прибавку
    // отдельным целым полем, и есть геттеры, которые его отдают. Именно
    // это число она показывает в своей подсказке как «Общий бонус
    // снаряжения». А getMeleeAttack и getMeleeDefence возвращают совсем
    // другое - боевой рейтинг, он и отрицательным бывает, и сравнивать
    // его с уровнем навыка бессмысленно: отсюда брались «Защита (-7) 1»
    // у голого новичка.
    //
    // Все прочие статы: разница между значением с модификаторами и без
    // - НО из неё сперва надо вынуть состояние тела. getStat(false)
    // складывает всё вместе: и множители от надетого, и увечья с
    // голодом. У Силы это видно в упор - base 1.000, mod 0.559, и вся
    // разница от ран, снаряжение силу не трогает вовсе.
    //
    // Долю состояния игра отдаёт отдельно и процентом:
    // getStatPenaltiesTotalForGUI. Проверено по журналу на восьми
    // статах - base*(1 - p/100) сходится с mod до сотых. Значит вклад
    // снаряжения = mod - base*(1 - p/100).
    //
    // Считаем во float и показываем, только если набралось хотя бы
    // полбалла. Иначе навык 1,0 при множителе 0,97 давал «(-1)», и
    // такой минус висел почти на каждой строке.
    // Протезы (роботизированные конечности) правят навыки СВОИМ
    // множителем - MedicalSystem::getStatRoboticsMultiplier. 07.10.2026:
    // у Архонта (руки-протезы) катаны 100 -> 110, а разбор называл эти +10
    // «снаряжением», хотя надето было ни при чём. Теперь это отдельная
    // строка, а снаряжению остаётся остаток.
    float RoboticsMultiplier(CharStats* stats, StatsEnumerated stat)
    {
        if (stats == NULL || stats->medical == NULL)
            return 1.0f;
        const float m = stats->medical->getStatRoboticsMultiplier(stat);
        return (m > 0.0f && fabsf(m - 1.0f) > 0.001f) ? m : 1.0f;
    }

    // Вклад протезов в целых пунктах: основа с учётом состояния тела,
    // умноженная на (множитель - 1).
    int RoboticsDelta(CharStats* stats, StatsEnumerated stat,
                      const std::string& name)
    {
        const float m = RoboticsMultiplier(stats, stat);
        if (m == 1.0f)
            return 0;
        const int statePercent = stats->getStatPenaltiesTotalForGUI(name, stat);
        const float diff = stats->getStat(stat, true)
            * (1.0f - statePercent / 100.0f) * (m - 1.0f);
        if (diff > -0.5f && diff < 0.5f)
            return 0;
        return static_cast<int>(diff < 0.0f ? diff - 0.5f : diff + 0.5f);
    }

    bool EquipmentDelta(const std::string& key, CharStats* stats,
                        int& delta)
    {
        if (stats == NULL || key.empty())
            return false;

        std::map<std::string, StatsEnumerated>::const_iterator it =
            g_byName.find(StripTags(key));
        if (it == g_byName.end())
            return false;

        int flat = 0;
        if (FlatEquipmentBonus(stats, it->second, flat))
        {
            delta = flat;
            return true;
        }

        float dodge = 0.0f;
        if (DodgeGearBonus(stats, it->second, dodge))
        {
            delta = static_cast<int>(dodge < 0.0f ? dodge - 0.5f
                                                  : dodge + 0.5f);
            if (dodge > -0.5f && dodge < 0.5f)
                delta = 0;
            return true;
        }

        const float base = stats->getStat(it->second, true);
        const int statePercent =
            stats->getStatPenaltiesTotalForGUI(StripTags(key), it->second);

        // Протезы - не снаряжение: их множитель входит в «без надетого»
        // (см. RoboticsMultiplier), иначе их прибавка числилась бы здесь.
        const float withoutGear = base * (1.0f - statePercent / 100.0f)
            * RoboticsMultiplier(stats, it->second);
        const float diff = stats->getStat(it->second, false) - withoutGear;

        if (diff > -0.5f && diff < 0.5f)
        {
            delta = 0;
            return true;
        }

        delta = static_cast<int>(diff < 0.0f ? diff - 0.5f : diff + 0.5f);
        return true;
    }


    // Полная поправка - всё, что игра прибавила к навыку: снаряжение,
    // состояние тела, обстановка. Ровно её показывает боевая панель.
    //
    // Своей арифметики нет: итог отдаёт EffectiveValue тем геттером,
    // который для этого стата у игры и предназначен, а основание -
    // getStat без модификаторов. Разность - и есть поправка.
    bool TotalDelta(const std::string& key, CharStats* stats, int& delta)
    {
        if (stats == NULL || key.empty())
            return false;

        std::map<std::string, StatsEnumerated>::const_iterator it =
            g_byName.find(StripTags(key));
        if (it == g_byName.end())
            return false;

        const float diff = EffectiveValue(stats, it->second)
                         - stats->getStat(it->second, true);

        // Порог в полбалла. Без него навык 1,0 при множителе 0,96
        // роняет целое с 1 до 0, и «(-1)» висит почти на каждой строке
        // израненного персонажа - при том, что изменение там на
        // четыре десятых. Что именно отняло эти доли, написано строкой
        // в разборе под описанием.
        if (diff > -0.5f && diff < 0.5f)
            delta = 0;
        else
            delta = static_cast<int>(diff < 0.0f ? diff - 0.5f
                                                 : diff + 0.5f);

        return true;
    }


    // ---------------------------------------------------------------
    // Раскраска
    // ---------------------------------------------------------------

    void ColourPanel(DatapanelGUI* panel, CharStats* stats)
    {
        if (!panel)
            return;

        const int category = panel->getCurrentCategory();
        const int count = panel->getNumLines(category);

        for (int i = 0; i < count; ++i)
        {
            DataPanelLine* line = panel->getLineByNum(category, i);
            if (!line || !line->w2)
                continue;

            const std::string caption = line->w2->getCaption().asUTF8();

            // Число берём из s2 - это то, что игра положила в строку.
            // Разбирать собственную подпись, уже расцвеченную в прошлом
            // кадре, незачем; подсказал тот же ShowEffectiveStats.
            long value = 0;
            if (!ParseValue(line->s2, value) && !ParseValue(caption, value))
                continue;

            const int level = PaletteIndex(value);
            const char* const valueColour = g_palette[level];

            int delta = 0;
            const bool known = TotalDelta(line->keyValue, stats, delta);

            if (!known || delta == 0)
            {
                // Без поправки подпись не трогаем вовсе: цвета виджета
                // достаточно, а лишняя перезапись каждый кадр ни к чему.
                if (caption.find('#') != std::string::npos)
                {
                    const std::string plain = StripTags(caption);
                    if (plain != caption)
                        line->w2->setCaption(plain);
                }
                line->w2->setTextColour(g_paletteColour[level]);
                continue;
            }

            char text[96];
            if (g_bracketOnLeft)
            {
                sprintf_s(text, "%s(%s%+d%s) %s%ld",
                          COLOUR_PLAIN,
                          delta > 0 ? COLOUR_PLUS : COLOUR_MINUS,
                          delta,
                          COLOUR_PLAIN,
                          valueColour,
                          value);
            }
            else
            {
                sprintf_s(text, "%s%ld %s(%s%+d%s)",
                          valueColour,
                          value,
                          COLOUR_PLAIN,
                          delta > 0 ? COLOUR_PLUS : COLOUR_MINUS,
                          delta,
                          COLOUR_PLAIN);
            }

            if (caption != text)
                line->w2->setCaption(text);
        }
    }


    // ---------------------------------------------------------------
    // Разбивка в панели описания
    //
    // Сначала пробовали всплывающую подсказку - ту же, что у игры в
    // боевом окне: ToolTip::setup на виджет строки. Данные приходили
    // (в журнале «ok=1 rows=4»), но подсказка не всплывала ни над
    // числом, ни над названием, даже с включённым мышиным фокусом.
    // Вероятно, общий объект подсказки из ForgottenGUI просто не
    // обслуживает окно характеристик. Разбираться дальше дороже, чем
    // обойти: те же строки дописываем прямо в левую колонку описания,
    // под «Как тренировать» - способом, которым KillCounter добавляет
    // свои строки в панель качеств. Место надёжное, это обычный
    // Datapanel, и видно всё без наведения.
    //
    // Строк держим ровно PENALTY_SLOTS штук, лишние остаются пустыми.
    // Так не нужно ничего удалять при переключении навыка: пустая строка
    // невидима, а стирать чужие строки по ключу опаснее.
    // ---------------------------------------------------------------

    const int PENALTY_SLOTS = 9;

    // По чему роспись собрана. Персонаж в ключе обязателен: без него
    // при переходе к другому персонажу с тем же выбранным навыком и той
    // же скобкой оставалась роспись прежнего - чужие раны, чужая броня.
    // Доля состояния и прибавка в помещении - тоже: они меняются, а
    // целая скобка при этом может остаться прежней.
    struct DescribedKey
    {
        Character* character;
        int stat;
        int delta;
        int statePercent;
        int indoors;
        int gear;

        bool operator!=(const DescribedKey& o) const
        {
            return character != o.character || stat != o.stat ||
                   delta != o.delta || statePercent != o.statePercent ||
                   indoors != o.indoors || gear != o.gear;
        }
    };

    struct Described
    {
        DescribedKey key;
        std::vector<std::pair<std::string, std::string> > rows;
    };

    // По окну: если открыто два окна, общий кэш перебивался бы ими
    // поочерёдно, и роспись (а с ней getStatPenaltiesForGUI, который
    // нечем освободить) пересобиралась бы каждый кадр. Указатель окна -
    // только ключ, по нему ничего не разыменовываем; закрытые окна
    // вычищает MainLoop_hook.
    std::map<CharacterStatsWindow*, Described> g_described;
    std::vector<std::pair<std::string, std::string> > g_describedRows;

    // Прибавка за бой в помещении: разница между вызовом с учётом
    // обстановки и без неё. Отдельным числом игра её не отдаёт, и
    // касается она только трёх боевых статов.
    int IndoorBonus(CharStats* stats, StatsEnumerated stat)
    {
        switch (stat)
        {
        case STAT_MELEE_ATTACK:
            return stats->skillBonusAttack_melee(true)
                 - stats->skillBonusAttack_melee(false);
        case STAT_MELEE_DEFENCE:
            return stats->skillBonusDefence(true)
                 - stats->skillBonusDefence(false);
        case STAT_MARTIALARTS:
            return stats->skillBonusUnarmed_forGUI(true)
                 - stats->skillBonusUnarmed_forGUI(false);
        default:
            return 0;
        }
    }


    // Слагаемые, которых нет в росписи игры: она знает только про
    // состояние тела. Подписи взяты из её собственной подсказки боевой
    // панели - «Общий бонус снаряжения» и «Штраф оружия в здании», - но
    // со второй мы от игры отступили: в помещении безоружный получает
    // ПЛЮС, и «Штраф: +8» читалось противоречиво. У нас «Бонус оружия
    // в здании», знак числа и так на месте.
    //
    // Снаряжение считаем для любого стата, а не только для трёх боевых.
    void AddCombatBonusRows(CharStats* stats, StatsEnumerated stat,
                            const std::string& name)
    {
        char number[32];

        const int robotics = RoboticsDelta(stats, stat, name);
        if (robotics != 0)
        {
            sprintf_s(number, "%+d", robotics);
            g_describedRows.push_back(
                std::make_pair(std::string(Tr("Robotic limbs")) + ":",
                               std::string(number)));
        }

        int gear = 0;
        if (EquipmentDelta(name, stats, gear) && gear != 0)
        {
            sprintf_s(number, "%+d", gear);
            g_describedRows.push_back(
                std::make_pair(std::string(Tr("Equipment bonus")) + ":",
                               std::string(number)));
        }

        const int indoors = IndoorBonus(stats, stat);
        if (indoors != 0)
        {
            sprintf_s(number, "%+d", indoors);
            g_describedRows.push_back(
                std::make_pair(std::string(Tr("Indoor weapon bonus")) + ":",
                               std::string(number)));
        }
    }


    // Все числа, какими игра описывает один навык, одной строкой в
    // журнал. Включается [Debug] Debug=1. Сравнивать с тем, что
    // показывает боевая панель.
    void DumpStat(CharStats* stats, StatsEnumerated stat,
                  const std::string& name)
    {
        char line[640];

        sprintf_s(line,
            "StatColours DUMP %s (%d): base=%.3f mod=%.3f effective=%.3f "
            "penaltyTotal=%d | attack_melee=%d/%d attack_unarmed=%d/%d "
            "defence=%d/%d unarmed=%d/%d perception=%d "
            "fields: A=%d D=%d U=%d indoors=%d encumb=%d | "
            "meleeAttack=%.3f meleeAttack_melee=%.3f unarmedAtk=%.3f/%.3f "
            "meleeDefence=%.3f dodge=%.3f/%.3f "
            "dodgePen gear=%.3f inj=%.3f enc=%.3f",
            name.c_str(), static_cast<int>(stat),
            stats->getStat(stat, true),
            stats->getStat(stat, false),
            EffectiveValue(stats, stat),
            stats->getStatPenaltiesTotalForGUI(name, stat),
            stats->skillBonusAttack_melee(false),
            stats->skillBonusAttack_melee(true),
            stats->skillBonusAttack_unarmed(false),
            stats->skillBonusAttack_unarmed(true),
            stats->skillBonusDefence(false),
            stats->skillBonusDefence(true),
            stats->skillBonusUnarmed_forGUI(false),
            stats->skillBonusUnarmed_forGUI(true),
            stats->skillBonusPerception,
            stats->_skillBonusAttack,
            stats->_skillBonusDefence,
            stats->skillBonusUnarmed,
            stats->skillBonusIndoors,
            stats->unarmedEncumbrancePenalty(),
            stats->getMeleeAttack(),
            stats->getMeleeAttack_melee(),
            stats->getMeleeAttack_unarmed(false),
            stats->getMeleeAttack_unarmed(true),
            stats->getMeleeDefence(false),
            stats->getDodge(false),
            stats->getDodge(true),
            stats->getDodgePenalty_gear(),
            stats->getDodgePenalty_injuries(),
            stats->getDodgePenalty_encumbrance());

        DebugLog(line);
    }


    void DescribePenalties(CharacterStatsWindow* window, CharStats* stats)
    {
        if (window == NULL || stats == NULL)
            return;

        // Правая колонка, а не левая. В левой уже лежит «Влияет на» и
        // «Как тренировать»; у навыков с длинным описанием наши строки
        // уезжали ниже видимой части, и панель просто обзаводилась
        // полосой прокрутки. Справа под текстом описания места вдоволь.
        DatapanelGUI* panel = window->description2Datapanel;
        if (panel == NULL)
            return;

        const int type = static_cast<int>(window->statProgress);

        int delta = 0;
        std::map<int, std::string>::const_iterator nameIt = g_nameOf.find(type);
        const bool known = (nameIt != g_nameOf.end());

        DescribedKey key;
        key.character = window->character;
        key.stat = type;
        key.delta = 0;
        key.statePercent = 0;
        key.indoors = 0;
        key.gear = 0;

        if (known)
        {
            TotalDelta(nameIt->second, stats, delta);
            key.delta = delta;
            key.statePercent = stats->getStatPenaltiesTotalForGUI(
                nameIt->second, window->statProgress);
            key.indoors = IndoorBonus(stats, window->statProgress);
            EquipmentDelta(nameIt->second, stats, key.gear);
        }

        // Пересчитываем, только когда поменялось что-то из ключа:
        // lektor, который наполняет игра, освободить нам нечем, а каждый
        // кадр звать его нельзя - это была бы утечка на ровном месте.
        std::map<CharacterStatsWindow*, Described>::iterator cached =
            g_described.find(window);
        const bool fresh = (cached == g_described.end());
        if (fresh)
            cached = g_described.insert(
                std::make_pair(window, Described())).first;

        if (fresh || cached->second.key != key)
        {
            cached->second.key = key;
            g_describedRows.clear();

            if (g_debug && known)
                DumpStat(stats, window->statProgress, nameIt->second);

            // Условие - «есть что сказать», а не «есть скобка».
            // Состояние тела и бой в помещении к надетому не относятся,
            // но объяснить их надо: на скриншоте у Силы 1 боевая панель
            // писала 0 (-1), и без этой строки непонятно, откуда минус.
            const int statePercent = key.statePercent;
            const int indoors = key.indoors;

            if (known && (delta != 0 || statePercent != 0 || indoors != 0))
            {
                // Имя стата у окна и у CharStats иногда расходится, а
                // разбивку игра отдаёт только по «своему» написанию.
                // Пробуем оба, прежде чем решить, что её нет.
                const std::string names[2] =
                {
                    nameIt->second,
                    CharStats::getStatName(window->statProgress)
                };

                for (int n = 0; n < 2 && g_describedRows.empty(); ++n)
                {
                    if (names[n].empty())
                        continue;
                    if (n == 1 && names[1] == names[0])
                        continue;

                    lektor<StringPair> rows;
                    if (!stats->getStatPenaltiesForGUI(
                            names[n], window->statProgress, rows))
                        continue;

                    for (uint32_t i = 0; i < rows.size() &&
                         static_cast<int>(g_describedRows.size()) < PENALTY_SLOTS - 1; ++i)
                    {
                        g_describedRows.push_back(
                            std::make_pair(rows[i].s1, rows[i].s2));
                    }
                }

                // Снаряжение дописываем ВСЕГДА, а не только когда у
                // игры разбивки нет.
                //
                // Её «Всего» подводит итог лишь тому, что перечислено
                // выше него, - у атаки это навык и травмы. Прибавку от
                // надетого игра считает отдельным механизмом и в эту
                // роспись не кладёт: она в её же подсказке боевой панели
                // как «Общий бонус снаряжения». Поэтому дописываем её
                // сами, иначе «Всего: 12» спорит со скобкой «(-3)» и с
                // боевой панелью, где стоит «9 (-3)».
                const int before = static_cast<int>(g_describedRows.size());
                AddCombatBonusRows(stats, window->statProgress,
                                   nameIt->second);

                // Если слагаемых не нашлось ни одного, называем хотя
                // бы саму поправку - иначе скобка есть, а под ней пусто,
                // и непонятно, посчитали мы что-то или промолчали.
                if (static_cast<int>(g_describedRows.size()) == before &&
                    delta != 0)
                {
                    char number[32];
                    sprintf_s(number, "%+d", delta);
                    g_describedRows.push_back(
                        std::make_pair(std::string(Tr("Equipment total")) + ":",
                                       std::string(number)));
                }

                // И само итоговое значение - то, которым игра
                // пользуется. Скобка говорит, насколько изменилось, эта
                // строка - до чего именно.
                if (delta != 0)
                {
                    char number[32];
                    sprintf_s(number, "%d", static_cast<int>(floorf(
                        EffectiveValue(stats, window->statProgress))));
                    g_describedRows.push_back(
                        std::make_pair(std::string(Tr("Effective value")) + ":",
                                       std::string(number)));
                }
            }

            cached->second.rows.swap(g_describedRows);
            g_describedRows.clear();
        }

        const std::vector<std::pair<std::string, std::string> >& described =
            cached->second.rows;
        const int category = panel->getCurrentCategory();

        for (int i = 0; i < PENALTY_SLOTS; ++i)
        {
            char key[32];
            sprintf_s(key, "SCPenalty%d", i);

            std::string left, right;
            if (!described.empty())
            {
                if (i == 0)
                {
                    left = " ";          // пустая строка-отбивка
                }
                else if (i - 1 < static_cast<int>(described.size()))
                {
                    left = described[i - 1].first;
                    right = described[i - 1].second;
                }
            }

            // Строка уже стоит с тем же текстом - setLine не зовём: он
            // каждый кадр переписывал бы подписи девяти строк и
            // перекладывал панель. Если игра панель пересобрала (смена
            // навыка), строки по ключу не будет, и она встанет заново.
            DataPanelLine* line = panel->getLine(key, category);
            if (line == NULL || line->s1 != left || line->s2 != right)
                line = panel->setLine(key, left, right, category, false, true);

            // Число в разбивке красим так же, как в скобке: зелёное на
            // прибавку, красное на штраф. Подпись - цветом скобок.
            if (line != NULL && !right.empty())
            {
                const bool minus = (right.find('-') != std::string::npos);
                if (line->w2 != NULL)
                    line->w2->setTextColour(minus ? g_minusColour : g_plusColour);
                if (line->w1 != NULL)
                    line->w1->setTextColour(g_labelColour);
            }
        }
    }


    // ---------------------------------------------------------------
    // Множители опыта расы
    // ---------------------------------------------------------------

    // Те же характеристики, что в окне навыков, в его порядке.
    const StatsEnumerated RACIAL_STATS[] =
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

    const int RACIAL_COUNT = sizeof(RACIAL_STATS) / sizeof(RACIAL_STATS[0]);

    // Ключи user string - готовыми std::string: getUserString берёт
    // const std::string&, и из длинного const char* каждый вызов
    // заводил бы временную строку в куче.
    const std::string TAG_TITLE = "StatColoursRacialTitle";
    const std::string TAG_FRAME = "StatColoursRacialFrame";
    const std::string KEY_SIGNATURE = "StatColoursRacialSig";
    // На корне окна: блок сейчас показан (и окно, возможно, растянуто).
    const std::string KEY_SHOWN = "StatColoursRacialShown";


    bool EndsWith(const std::string& text, const char* suffix)
    {
        const size_t n = strlen(suffix);
        return text.size() >= n &&
               text.compare(text.size() - n, n, suffix) == 0;
    }


    // Виджет разметки окна по окончанию имени (BaseLayout приписывает
    // к именам префикс). У MyGUI::Window дети разметки лежат в его
    // клиентской области, поэтому ищем и в ней.
    MyGUI::Widget* FindBySuffix(MyGUI::Widget* widget, const char* suffix,
                                int depth)
    {
        if (widget == NULL || depth > 4)
            return NULL;
        if (EndsWith(widget->getName(), suffix))
            return widget;

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const found =
                FindBySuffix(widget->getChildAt(i), suffix, depth + 1);
            if (found != NULL)
                return found;
        }

        MyGUI::Widget* const client = widget->getClientWidget();
        if (client != NULL && client != widget)
            return FindBySuffix(client, suffix, depth + 1);

        return NULL;
    }


    MyGUI::Widget* FindTagged(MyGUI::Widget* parent, const std::string& tag)
    {
        const size_t count = parent->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = parent->getChildAt(i);
            if (!child->getUserString(tag).empty())
                return child;
        }
        return NULL;
    }


    // Число с десятичным знаком языка игры (по-русски запятая).
    std::string DecimalText(const char* format, float value)
    {
        char text[32];
        sprintf_s(text, format, value);
        std::string out(text);
        const char* const point = Tr("decimal point");
        if (point != NULL && strlen(point) == 1 && point[0] != '.')
        {
            for (size_t i = 0; i < out.size(); ++i)
                if (out[i] == '.')
                    out[i] = point[0];
        }
        return out;
    }


    struct RacialRow
    {
        std::string name;
        float mult;
    };


    // Клавиша считается зажатой, только когда окно игры впереди: иначе
    // ALT+TAB в другое окно показывал бы блок.
    bool RacialKeyDown()
    {
        if (g_racialHoldKey.vk == 0)
            return true;
        if (!HoldKey::Held(g_racialHoldKey))
            return false;

        const HWND front = GetForegroundWindow();
        if (front == NULL)
            return false;
        DWORD pid = 0;
        GetWindowThreadProcessId(front, &pid);
        return pid == GetCurrentProcessId();
    }


    void HideRacial(MyGUI::Widget* title, MyGUI::Widget* frame)
    {
        if (title != NULL && title->getVisible())
            title->setVisible(false);
        if (frame != NULL && frame->getVisible())
            frame->setVisible(false);
    }


    // Название, которое не помещается рядом с числом, укорачиваем с
    // точкой на конце: «Точная стрельба» -> «Точная стре.».
    void FitName(MyGUI::TextBox* box, const std::string& name, int width)
    {
        box->setCaption(name);
        if (box->getTextSize().width <= width)
            return;

        MyGUI::UString text(name);
        while (text.size() > 3)
        {
            text = text.substr(0, text.size() - 1);
            box->setCaption(text + ".");
            if (box->getTextSize().width <= width)
                return;
        }
    }


    // Исходная геометрия окна - до нашего вмешательства. Запоминаем один
    // раз на окно (в его user string), чтобы растягивать от неё, а не
    // накапливать: персонаж в окне меняется, а с ним и число строк.
    struct WindowBase
    {
        int windowHeight;
        int descTitleTop;
        int descFrameTop;
        int statsFrameHeight;
    };

    const std::string KEY_BASE = "StatColoursRacialBase";
    const std::string KEY_GROW = "StatColoursRacialGrow";   // на сколько растянуть


    bool ReadBase(MyGUI::Widget* root, WindowBase& base)
    {
        const std::string text = root->getUserString(KEY_BASE);
        return !text.empty() &&
               sscanf_s(text.c_str(), "%d %d %d %d", &base.windowHeight,
                        &base.descTitleTop, &base.descFrameTop,
                        &base.statsFrameHeight) == 4;
    }


    void WriteBase(MyGUI::Widget* root, const WindowBase& base)
    {
        char text[64];
        sprintf_s(text, "%d %d %d %d", base.windowHeight, base.descTitleTop,
                  base.descFrameTop, base.statsFrameHeight);
        root->setUserString(KEY_BASE, text);
    }


    void SetTop(MyGUI::Widget* widget, int top)
    {
        if (widget != NULL && widget->getTop() != top)
            widget->setPosition(widget->getLeft(), top);
    }


    void SetHeight(MyGUI::Widget* widget, int height)
    {
        if (widget != NULL && widget->getHeight() != height)
            widget->setSize(widget->getWidth(), height);
    }


    // Окно выше исходного на grow: «Описание» с рамкой - ниже, правая
    // колонка производных характеристик - длиннее, чтобы низ остался
    // ровным. grow = 0 возвращает всё как было.
    void GrowWindow(MyGUI::Widget* root, const WindowBase& base, int grow,
                    MyGUI::Widget* descTitle, MyGUI::Widget* descFrame,
                    MyGUI::Widget* statsFrame)
    {
        SetHeight(root, base.windowHeight + grow);
        SetTop(descTitle, base.descTitleTop + grow);
        SetTop(descFrame, base.descFrameTop + grow);
        if (statsFrame != NULL)
            SetHeight(statsFrame, base.statsFrameHeight + grow);
    }


    void ShowRacial(CharacterStatsWindow* window, CharStats* stats)
    {
        MyGUI::Widget* const root = window->mMainWidget;
        if (root == NULL)
            return;

        RaceData* const race = window->character != NULL
            ? window->character->getRace() : NULL;
        const bool want =
            g_showRacial && stats != NULL && race != NULL && RacialKeyDown();

        // Почти всё время клавиша отпущена, а блок спрятан. Тогда делать
        // нечего вовсе - без пяти обходов дерева окна в каждом кадре.
        if (!want && root->getUserString(KEY_SHOWN).empty())
            return;

        MyGUI::Widget* const attrPanel = FindBySuffix(root, "AttributesPanel", 0);
        MyGUI::Widget* const attrTitle = FindBySuffix(root, "lbAttributes", 0);
        MyGUI::Widget* const descTitle = FindBySuffix(root, "lbDescription", 0);
        MyGUI::Widget* const descPanel = FindBySuffix(root, "Description1Panel", 0);
        MyGUI::Widget* const statsPanel = FindBySuffix(root, "StatisticsPanel", 0);
        if (attrPanel == NULL || attrTitle == NULL || descTitle == NULL ||
            descPanel == NULL || attrPanel->getParent() == NULL ||
            descPanel->getParent() == NULL)
        {
            static bool told = false;
            if (!told)
            {
                told = true;
                ErrorLog("StatColours: stats window layout not recognised, "
                         "no racial XP block");
            }
            return;
        }

        MyGUI::Widget* const attrFrame = attrPanel->getParent();
        MyGUI::Widget* const descFrame = descPanel->getParent();
        MyGUI::Widget* const statsFrame =
            statsPanel != NULL ? statsPanel->getParent() : NULL;
        MyGUI::Widget* const parent = attrFrame->getParent();
        if (parent == NULL || attrTitle->getParent() != parent ||
            descTitle->getParent() != parent || descFrame->getParent() != parent)
        {
            return;
        }

        WindowBase base;
        if (!ReadBase(root, base))
        {
            base.windowHeight = root->getHeight();
            base.descTitleTop = descTitle->getTop();
            base.descFrameTop = descFrame->getTop();
            base.statsFrameHeight = statsFrame != NULL ? statsFrame->getHeight() : 0;
            WriteBase(root, base);
        }

        MyGUI::Widget* const tagged = FindTagged(parent, TAG_TITLE);
        MyGUI::TextBox* title = tagged != NULL
            ? tagged->castType<MyGUI::TextBox>(false) : NULL;
        MyGUI::Widget* frame = FindTagged(parent, TAG_FRAME);

        // Что показывать: множители, отличные от x1.
        std::vector<RacialRow> rows;
        std::string signature;
        if (want)
        {
            for (int i = 0; i < RACIAL_COUNT; ++i)
            {
                // Нет записи о стате - множитель x1 (или 0 - тоже «нет»).
                const float mult = race->getStatMod(RACIAL_STATS[i]);
                if (mult <= 0.001f || std::fabs(mult - 1.0f) < 0.005f)
                    continue;

                CharacterStatsWindow::Stat* const entry =
                    CharacterStatsWindow::getStat(RACIAL_STATS[i]);
                RacialRow row;
                row.name = entry != NULL && !entry->name.empty()
                    ? entry->name : CharStats::getStatName(RACIAL_STATS[i]);
                row.mult = mult;
                rows.push_back(row);

                char part[48];
                sprintf_s(part, "%d:%.3f;", static_cast<int>(RACIAL_STATS[i]), mult);
                signature += part;
            }
        }

        if (rows.empty())
        {
            HideRacial(title, frame);
            GrowWindow(root, base, 0, descTitle, descFrame, statsFrame);
            root->clearUserString(KEY_SHOWN);
            return;
        }

        const MyGUI::IntCoord af = attrFrame->getCoord();
        const MyGUI::IntCoord ap = attrPanel->getCoord();
        const MyGUI::IntCoord lt = attrTitle->getCoord();

        if (title == NULL)
        {
            MyGUI::TextBox* const ref = attrTitle->castType<MyGUI::TextBox>(false);
            title = parent->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxPaintedText", lt, MyGUI::Align::Default, "");
            title->setUserString(TAG_TITLE, "1");
            title->setNeedMouseFocus(false);
            if (ref != NULL)
            {
                title->setTextAlign(ref->getTextAlign());
                title->setFontName(ref->getFontName());
                title->setTextColour(ref->getTextColour());
            }
        }

        if (frame == NULL)
        {
            frame = parent->createWidget<MyGUI::Widget>(
                "Kenshi_FrameSkin", af, MyGUI::Align::Default, "");
            frame->setUserString(TAG_FRAME, "1");
            frame->setNeedMouseFocus(false);
        }

        // Строки пересобираем, только когда поменялся состав (другой
        // персонаж, другая раса) или место.
        char geometry[64];
        sprintf_s(geometry, "|%d,%d,%d", af.left, af.top + af.height, af.width);
        signature += geometry;

        if (frame->getUserString(KEY_SIGNATURE) != signature)
        {
            frame->setUserString(KEY_SIGNATURE, signature);

            while (frame->getChildCount() > 0)
                MyGUI::Gui::getInstance().destroyWidget(frame->getChildAt(0));

            // Отступы внутри рамки - как у панели «Качеств» в её рамке.
            const int insetLeft = ap.left;
            const int insetTop = ap.top;
            const int insetRight = af.width - (ap.left + ap.width);
            const int insetBottom = af.height - (ap.top + ap.height);
            const int innerWidth = af.width - insetLeft - insetRight;

            int rowHeight = 0;
            std::string rowFont;
            for (size_t i = 0; i < rows.size(); ++i)
            {
                MyGUI::TextBox* const value = frame->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, 10),
                    MyGUI::Align::Default, "");
                if (rowHeight == 0)
                {
                    rowHeight = value->getFontHeight() + 3;
                    rowFont = value->getFontName();
                }
                const int y = insetTop + static_cast<int>(i) * rowHeight;

                value->setCoord(insetLeft, y, innerWidth, rowHeight);
                value->setNeedMouseFocus(false);
                value->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
                value->setCaption(DecimalText("x%.2f", rows[i].mult));
                value->setTextColour(
                    rows[i].mult > 1.0f ? g_plusColour : g_minusColour);

                MyGUI::TextBox* const name = frame->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText",
                    MyGUI::IntCoord(insetLeft, y, innerWidth, rowHeight),
                    MyGUI::Align::Default, "");
                name->setNeedMouseFocus(false);
                name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
                name->setTextColour(g_labelColour);
                FitName(name, rows[i].name,
                        innerWidth - value->getTextSize().width - 6);
            }

            // Заголовок шире колонки - шрифтом строк, он мельче.
            title->setCaption(Tr("XP MULTIPLIERS"));
            if (title->getTextSize().width > lt.width && !rowFont.empty())
                title->setFontName(rowFont);

            // Под заголовком - как у «Качеств»: тот же зазор до рамки.
            const int titleGap = af.top - (lt.top + lt.height);
            const int titleTop = af.top + af.height + 4;
            const int frameTop = titleTop + lt.height + (titleGap > 0 ? titleGap : 0);
            const int frameHeight = insetTop +
                static_cast<int>(rows.size()) * rowHeight + insetBottom;

            title->setCoord(lt.left, titleTop, lt.width, lt.height);
            frame->setCoord(af.left, frameTop, af.width, frameHeight);

            // Не влезает до «Описания» - окно вытягивается вниз на
            // нехватку, как в RaceMetrics.
            const int need = frameTop + frameHeight + 4;
            const int grow = need > base.descTitleTop ? need - base.descTitleTop : 0;
            char growText[16];
            sprintf_s(growText, "%d", grow);
            frame->setUserString(KEY_GROW, growText);

            if (g_debug)
            {
                char note[160];
                sprintf_s(note, "StatColours: racial XP block - %u multipliers, "
                          "row %d px, window grows by %d px",
                          static_cast<unsigned>(rows.size()), rowHeight, grow);
                DebugLog(note);
            }
        }

        // Блок прячется по отпусканию клавиши, а окно при этом сжимается
        // обратно - значит, растяжение надо возвращать при каждом показе,
        // а не только при пересборке строк.
        GrowWindow(root, base, atoi(frame->getUserString(KEY_GROW).c_str()),
                   descTitle, descFrame, statsFrame);

        if (!title->getVisible())
            title->setVisible(true);
        if (!frame->getVisible())
            frame->setVisible(true);
        if (root->getUserString(KEY_SHOWN).empty())
            root->setUserString(KEY_SHOWN, "1");
    }


    // ---------------------------------------------------------------
    // Скорость прокачки за заголовком навыка
    // ---------------------------------------------------------------

    const std::string TAG_RATE = "StatColoursXpRate";

    typedef float (*XpoRateFn)(int stat, float level);
    XpoRateFn g_xpoRate = NULL;
    bool g_xpoLooked = false;

    // XP_Overhaul грузится не обязательно раньше нас, поэтому ищем его в
    // первом кадре с открытым окном - к этому времени загружены все.
    XpoRateFn XpoRate()
    {
        if (!g_xpoLooked)
        {
            g_xpoLooked = true;
            const HMODULE xpo = GetModuleHandleA("XP_Overhaul.dll");
            if (xpo != NULL)
                g_xpoRate = reinterpret_cast<XpoRateFn>(
                    GetProcAddress(xpo, "XPO_CurrentXpRate"));
            DebugLog(g_xpoRate != NULL
                ? "StatColours: XP rate taken from XP_Overhaul"
                : "StatColours: no XP_Overhaul export, vanilla XP curve");
        }
        return g_xpoRate;
    }


    // Во сколько раз опыт навыка сейчас умножается: раса * уровень.
    float CurrentXpRate(CharStats* stats, RaceData* race, StatsEnumerated stat)
    {
        float raceMult = race != NULL ? race->getStatMod(stat) : 1.0f;
        if (raceMult <= 0.001f)
            raceMult = 1.0f;

        const float level = stats->getStat(stat, true);

        float curve = -1.0f;
        if (XpoRate() != NULL)
            curve = g_xpoRate(static_cast<int>(stat), level);
        if (curve < 0.0f)
        {
            // Ваниль: (1 - уровень/101)^2, на сотне - ноль.
            const float left = level < 100.0f ? 1.0f - level / 101.0f : 0.0f;
            curve = left > 0.0f ? left * left : 0.0f;
        }

        return raceMult * curve;
    }


    // Заголовок навыка и наша подпись - одним обходом панели описания.
    // Заголовок узнаём по тексту, но текст этот - имя навыка из самой
    // игры (как и вся карта имён), так что от языка ничего не зависит.
    void FindHeading(MyGUI::Widget* widget, const std::string& name,
                     const std::string& alias, int depth,
                     MyGUI::TextBox*& heading, MyGUI::Widget*& label)
    {
        if (widget == NULL || depth > 8)
            return;

        if (label == NULL && !widget->getUserString(TAG_RATE).empty())
        {
            label = widget;
            return;
        }

        if (heading == NULL)
        {
            MyGUI::TextBox* const text = widget->castType<MyGUI::TextBox>(false);
            if (text != NULL)
            {
                std::string caption = text->getCaption().asUTF8();
                if (caption.find('#') != std::string::npos)
                    caption = StripTags(caption);
                if (!caption.empty() && (caption == name ||
                    (!alias.empty() && caption == alias)))
                {
                    heading = text;
                }
            }
        }

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
            FindHeading(widget->getChildAt(i), name, alias, depth + 1,
                        heading, label);

        MyGUI::Widget* const client = widget->getClientWidget();
        if (client != NULL && client != widget)
            FindHeading(client, name, alias, depth + 1, heading, label);
    }


    // Разовый дамп дерева панели описания в журнал: тип, имя, место,
    // подпись. Нужен, если заголовок навыка не нашёлся, - чтобы по
    // журналу было видно, во что игра его положила.
    void DumpTree(MyGUI::Widget* widget, int depth)
    {
        if (widget == NULL || depth > 12)
            return;

        std::string caption;
        MyGUI::TextBox* const text = widget->castType<MyGUI::TextBox>(false);
        if (text != NULL)
            caption = text->getCaption().asUTF8();

        char line[512];
        sprintf_s(line, "StatColours tree %*s%s '%s' %d,%d %dx%d vis=%d '%.120s'",
                  depth * 2, "", widget->getTypeName().c_str(),
                  widget->getName().c_str(), widget->getLeft(), widget->getTop(),
                  widget->getWidth(), widget->getHeight(),
                  widget->getVisible() ? 1 : 0, caption.c_str());
        DebugLog(line);

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
            DumpTree(widget->getChildAt(i), depth + 1);

        MyGUI::Widget* const client = widget->getClientWidget();
        if (client != NULL && client != widget)
            DumpTree(client, depth + 1);
    }

    bool g_dumpedMissing = false;
    bool g_loggedFound = false;


    void ShowXpRate(CharacterStatsWindow* window, CharStats* stats)
    {
        MyGUI::Widget* const root = window->mMainWidget;
        if (root == NULL)
            return;

        MyGUI::Widget* const descPanel =
            FindBySuffix(root, "Description1Panel", 0);
        if (descPanel == NULL)
            return;

        const StatsEnumerated stat = window->statProgress;
        std::map<int, std::string>::const_iterator nameIt =
            g_nameOf.find(static_cast<int>(stat));

        MyGUI::TextBox* heading = NULL;
        MyGUI::Widget* label = NULL;
        if (g_showRate && stats != NULL && nameIt != g_nameOf.end())
        {
            FindHeading(descPanel, nameIt->second, CharStats::getStatName(stat),
                        0, heading, label);
        }
        else
        {
            FindHeading(descPanel, std::string(), std::string(), 0,
                        heading, label);
            heading = NULL;
        }

        if (heading == NULL)
        {
            if (!g_dumpedMissing && g_showRate && stats != NULL &&
                nameIt != g_nameOf.end())
            {
                g_dumpedMissing = true;
                DebugLog("StatColours: skill heading '" + nameIt->second +
                         "' not found in the description, panel tree:");
                DumpTree(descPanel, 0);
            }
            if (label != NULL && label->getVisible())
                label->setVisible(false);
            return;
        }

        // Подпись - на самой панели описания, а не соседкой заголовка:
        // у строки панели свой контейнер по размеру текста, и всё, что
        // правее, он обрезал. Место считаем от заголовка в абсолютных
        // координатах.
        MyGUI::Widget* const parent = descPanel;
        if (label != NULL && label->getParent() != parent)
        {
            MyGUI::Gui::getInstance().destroyWidget(label);
            label = NULL;
        }

        MyGUI::TextBox* box = label != NULL
            ? label->castType<MyGUI::TextBox>(false) : NULL;
        if (box == NULL)
        {
            box = parent->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, 10),
                MyGUI::Align::Default, "");
            box->setUserString(TAG_RATE, "1");
            box->setNeedMouseFocus(false);
            box->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Bottom);
        }

        RaceData* const race = window->character != NULL
            ? window->character->getRace() : NULL;
        const std::string caption = std::string("(") + Tr("Current XP rate") +
            ": " + DecimalText("%.3f", CurrentXpRate(stats, race, stat)) + ")";
        if (box->getCaption().asUTF8() != caption)
            box->setCaption(caption);
        box->setTextColour(g_bracketColour);

        // Сразу за текстом заголовка, по его нижнему краю: шрифт у нас
        // мельче, по низу строки читаются как одна.
        // Ширина - самого текста, а не области: заголовок у игры - EditBox
        // на всю колонку (466 px), и область у него вся эта ширина.
        const MyGUI::IntCoord text = heading->getTextRegion();
        const int textWidth = heading->getTextSize().width;
        const int left = heading->getAbsoluteLeft() - parent->getAbsoluteLeft()
                       + text.left + textWidth + 8;
        const int top = heading->getAbsoluteTop() - parent->getAbsoluteTop()
                      + text.top;
        const int width = parent->getWidth() - left;
        const MyGUI::IntCoord want(left, top, width > 10 ? width : 10,
                                   text.height > 4 ? text.height
                                                   : heading->getHeight());
        if (box->getCoord() != want)
            box->setCoord(want);

        if (!g_loggedFound)
        {
            g_loggedFound = true;
            char note[256];
            sprintf_s(note, "StatColours: skill heading %s at %d,%d %dx%d, "
                      "text %d,%d %dx%d; rate label at %d,%d %dx%d",
                      heading->getTypeName().c_str(),
                      heading->getAbsoluteLeft() - parent->getAbsoluteLeft(),
                      heading->getAbsoluteTop() - parent->getAbsoluteTop(),
                      heading->getWidth(), heading->getHeight(),
                      text.left, text.top, text.width, text.height,
                      want.left, want.top, want.width, want.height);
            DebugLog(note);
        }

        if (!box->getVisible())
            box->setVisible(true);
    }


    void ColourWindow(CharacterStatsWindow* window)
    {
        if (!window)
            return;

        CharStats* stats =
            window->character ? window->character->getStats() : NULL;

        // Панель описания и правый столбец производных характеристик не
        // трогаем: там не уровни навыков, а множители и проценты, и шкала
        // «ноль красный, сто зелёный» к ним не применима.
        ColourPanel(window->attributesDatapanel, stats);
        ColourPanel(window->skills1Datapanel, stats);
        ColourPanel(window->skills2Datapanel, stats);
        ColourPanel(window->skills3Datapanel, stats);
        ColourPanel(window->skills4Datapanel, stats);

        DescribePenalties(window, stats);
        ShowRacial(window, stats);
        ShowXpRate(window, stats);
    }


    void (*g_origMainLoop)(GameWorld* thisptr, float time) = NULL;

    void MainLoop_hook(GameWorld* thisptr, float time)
    {
        g_origMainLoop(thisptr, time);

        if (!gui)
            return;

        Ogre::vector<CharacterStatsWindow*>::type& windows =
            gui->characterStatsWindows;

        if (windows.empty())
            return;

        BuildNameMap();

        // Палитру спрашиваем здесь, а не в начале кадра: окно
        // характеристик открыто, значит интерфейс давно в работе. В
        // первом кадре после загрузки сейва тот же вызов валил игру.
        ResolveThemeColours();

        for (size_t i = 0; i < windows.size(); ++i)
            ColourWindow(windows[i]);

        if (g_described.size() > windows.size())
        {
            std::map<CharacterStatsWindow*, Described>::iterator it =
                g_described.begin();
            while (it != g_described.end())
            {
                bool open = false;
                for (size_t i = 0; i < windows.size() && !open; ++i)
                    open = (windows[i] == it->first);
                if (open)
                    ++it;
                else
                    g_described.erase(it++);
            }
        }
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.

// Перечитывание: палитру темы спросить заново, роспись под навыком
// собрать заново (в кэше только строки).
static void __cdecl McmReload()
{
    g_pendingTheme.clear();
    LoadColours();
    g_themeResolved = false;
    g_described.clear();
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "StatColours", "Stat Colours", ini.c_str(), &McmReload);
    if (api->version >= 2)
        api->info(api, Tr("Colours the skill numbers by level, breaks down equipment bonuses, shows racial XP multipliers and the current XP rate."));
    api->section(api, Tr("Racial XP"));
    api->toggle(api, "RacialXP", "Show", Tr("Racial XP multipliers"), Tr("Racial XP multipliers under the attributes in the character window."), 1, 0);
    api->hotkey(api, "RacialXP", "HoldKey", Tr("Show while held"), Tr("The multipliers are shown only while this key is held. Click and press a key; Backspace - no key, always shown."), "ALT", 0);
    api->toggle(api, "RacialXP", "ShowRate", Tr("Current XP rate"), Tr("The current XP rate in brackets after the skill name in the skill description."), 1, 0);
    api->section(api, Tr("Colours"));
    api->toggle(api, "Colours", "UseGameTheme", Tr("Colours from the interface theme"), Tr("auto colours below are taken from the game palette, kenshi_colours.xml, and follow an interface mod over it. Off - built-in colours."), 1, 0);
    {
        static const char* const values[] = { "left", "right" };
        const char* const labels[] = { Tr("left"), Tr("right") };
        api->choice(api, "Colours", "BracketSide", Tr("Bracket side"), Tr("Which side of the skill value the brackets with the equipment correction go. On the left they can overlap a long skill name."), "left", values, labels, 2, 0);
    }
    api->colour(api, "Colours", "Brackets", Tr("Brackets"), Tr("The brackets around the equipment correction, like (+3). auto - from the palette, or #RRGGBB."), "auto", 0);
    api->colour(api, "Colours", "Bonus", Tr("Bonus"), Tr("The number in the brackets when equipment adds. auto - from the palette, or #RRGGBB."), "auto", 0);
    api->colour(api, "Colours", "Penalty", Tr("Penalty"), Tr("The number in the brackets when equipment takes away. auto - from the palette, or #RRGGBB."), "auto", 0);
    api->colour(api, "Colours", "Labels", Tr("Labels"), Tr("The captions in the breakdown under the skill description. auto - from the palette, or #RRGGBB."), "auto", 0);
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
    api->toggle(api, "Debug", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: the palette, the colours used and the breakdown under the skill."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(
            KenshiLib::GetRealAddress(
                &GameWorld::_NV_mainLoop_GPUSensitiveStuff),
            MainLoop_hook,
            &g_origMainLoop))
    {
        ErrorLog("StatColours: could not hook the game loop, colours are off");
        return;
    }

    LoadColours();

    DebugLog("StatColours: game loop hook installed");
}
