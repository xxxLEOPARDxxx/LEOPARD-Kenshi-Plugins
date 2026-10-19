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
#include <IniDefaults.h>

#include <algorithm>
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
#include <mygui/MyGUI_RenderManager.h>

#include <kenshi/PlayerInterface.h>
#include <kenshi/RootObject.h>
#include <kenshi/GameData.h>
#include <WidgetRef.h>

// Нанимаемость по диалогу (как в RecruitMarkers): conversationsMain у
// Dialogue закрытый.
#define private public
#define protected public
#include <kenshi/Dialogue.h>
#undef private
#undef protected


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

    // [RacialXP] Selected=0 - не показывать множители над именем
    // выделенного персонажа в мире (18.10.2026); SelectedKey - клавиша.
    bool g_showSelectedRacial = true;
    HoldKey::Key g_selectedRacialKey;   // ALT; vk 0 - всегда

    const char* COLOUR_PLAIN = g_colourBracket;
    const char* COLOUR_PLUS  = g_colourPlus;
    const char* COLOUR_MINUS = g_colourMinus;
    const char* COLOUR_LABEL = g_colourLabel;


    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    std::string IniFileName()
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


    // Путь к ini игрока; при первом обращении - копия или дописка из
    // <Мод>.defaults.ini (shared/IniDefaults.h, 11.10.2026).
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
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
            "The bracket with the correction - everything that changes the skill now: (+3) or (-4).",
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

        fprintf(f, "\n[StatColours]\n; %s\nEnabled=1\n", Tr("0 turns the whole plugin off (after a restart)."));

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
        fprintf(f, "; %s\n", Tr("Selected: racial XP multipliers above the name of the selected recruit in the world (anyone - with Debug=1), while SelectedKey is held: 1 shows them, 0 hides."));
        fputs("[RacialXP]\nShow=1\nHoldKey=ALT\nShowRate=1\nSelected=1\nSelectedKey=ALT\n", f);

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
        g_showSelectedRacial = GetPrivateProfileIntA("RacialXP", "Selected", 1,
                                                     ini.c_str()) != 0;
        {
            char key[32] = {};
            GetPrivateProfileStringA("RacialXP", "SelectedKey", "ALT", key,
                                     sizeof(key), ini.c_str());
            g_selectedRacialKey = HoldKey::Parse(key);
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
            //
            // И тоже _melee, как у атаки: общий getMeleeDefence у
            // безоружного отдаёт защиту в рукопашной (от боевых искусств).
            // 16.10.2026: голый персонаж с рюкзаком, защита 48, боевые
            // искусства 0 - строка показывала «(-49) 48».
            return stats->getMeleeDefence_melee(true);
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


    // Протезы и надетое у небоевых навыков - в росписи самой игры
    // («Роботехника», «Доспех/снаряжение», процентами). Своих расчётов
    // вклада протезов и снаряжения больше нет (19.10.2026): они учитывали
    // протезы дважды.


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
        int stance;

        bool operator!=(const DescribedKey& o) const
        {
            return character != o.character || stat != o.stat ||
                   delta != o.delta || statePercent != o.statePercent ||
                   indoors != o.indoors || gear != o.gear || stance != o.stance;
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


    // Прибавка боевой стойки («Блок» на панели приказов) к защите:
    // разница между защитой со стойкой и без неё. В скобку она входит
    // (итог защиты берём со стойкой, как боевая панель игры), а в
    // разборе её не было - со включённым «Блоком» строки не
    // складывались в скобку (16.10.2026).
    int StanceBonus(CharStats* stats, StatsEnumerated stat)
    {
        if (stat != STAT_MELEE_DEFENCE)
            return 0;
        const float d = stats->getMeleeDefence_melee(true) - stats->getMeleeDefence_melee(false);
        return static_cast<int>(d >= 0.0f ? d + 0.5f : d - 0.5f);
    }


    // Боевые навыки: их итог считает не getStat, а боевые геттеры
    // (EffectiveValue), и роспись игры (getStatPenaltiesForGUI) о
    // прибавках оружия, брони и стойки не знает - у Атаки она пишет
    // «Всего: 19», когда в бою 26 (дамп 19.10.2026). Для них строки свои.
    bool IsCombatStat(StatsEnumerated stat)
    {
        return stat == STAT_MELEE_ATTACK || stat == STAT_MELEE_DEFENCE ||
               stat == STAT_MARTIALARTS || stat == STAT_DODGE;
    }

    int Rounded(float v)
    {
        if (v > -0.5f && v < 0.5f)
            return 0;
        return static_cast<int>(v < 0.0f ? v - 0.5f : v + 0.5f);
    }

    void AddRow(const char* label, int value)
    {
        if (value == 0)
            return;
        char number[32];
        sprintf_s(number, "%+d", value);
        g_describedRows.push_back(
            std::make_pair(std::string(Tr(label)) + ":", std::string(number)));
    }

    // Строки боевого навыка сверх росписи игры. Сумма их с навыком (уже
    // с ранами и протезами - getStat(false)) всегда равна итогу: всё, что
    // не нашло своей строки, уходит в остаток «прочее» - иначе строки
    // снова не складывались бы в скобку (у игроков «Защита (-85)» при
    // одной строке «-1»).
    void AddCombatBonusRows(CharStats* stats, StatsEnumerated stat)
    {
        const float effective = EffectiveValue(stats, stat);
        float known = stats->getStat(stat, false);

        if (stat == STAT_DODGE)
        {
            // Уворот - мимо getStat: навык + надетое + раны + вес ноши.
            const float gear = stats->getDodgePenalty_gear();
            const float injuries = stats->getDodgePenalty_injuries();
            const float load = stats->getDodgePenalty_encumbrance();
            known = stats->getStat(stat, true) + gear + injuries + load;
            AddRow("Equipment bonus", Rounded(gear));
            AddRow("Injuries", Rounded(injuries));
            AddRow("Load", Rounded(load));
        }
        else
        {
            int flat = 0;
            FlatEquipmentBonus(stats, stat, flat);
            const int indoors = IndoorBonus(stats, stat);
            const int stance = StanceBonus(stats, stat);
            known += static_cast<float>(flat + indoors) +
                     (stat == STAT_MELEE_DEFENCE
                          ? stats->getMeleeDefence_melee(true) - stats->getMeleeDefence_melee(false)
                          : 0.0f);
            AddRow("Equipment bonus", flat);
            AddRow("Indoor weapon bonus", indoors);
            AddRow("Combat stance", stance);
        }

        AddRow("Load, weapon and other", Rounded(effective - known));
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

        // 19.10.2026: всё, что игра знает про протезы и про роспись, -
        // отдельной строкой. По этому дампу и нашлось, что процент росписи
        // игры уже содержит протезы.
        MedicalSystem* const med = stats->medical;
        char robo[320];
        sprintf_s(robo,
            "StatColours DUMP2 %s: roboticsMult=%.3f _getRoboticsStatMult=%.3f "
            "hasRobotics=%d missingArmMult=%.3f weatherMult=%.3f",
            name.c_str(),
            med != NULL ? med->getStatRoboticsMultiplier(stat) : -1.0f,
            med != NULL ? med->_getRoboticsStatMult(stat) : -1.0f,
            med != NULL && med->hasRobotics() ? 1 : 0,
            med != NULL ? med->getMissingArmPenaltyMult() : -1.0f,
            med != NULL ? med->getWeatherStatPenaltyMult() : -1.0f);
        DebugLog(robo);

        static lektor<StringPair> gameRows;     // буфер игры не освобождается - один на всё время
        gameRows.clear();
        const bool got = stats->getStatPenaltiesForGUI(name, stat, gameRows);
        char head[96];
        sprintf_s(head, "StatColours DUMP3 %s: game rows ok=%d count=%u", name.c_str(),
                  got ? 1 : 0, static_cast<unsigned>(gameRows.size()));
        DebugLog(head);
        for (uint32_t i = 0; i < gameRows.size(); ++i)
            DebugLog("StatColours DUMP3   '" + gameRows[i].s1 + "' | '" + gameRows[i].s2 + "'");
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
        key.stance = 0;

        if (known)
        {
            TotalDelta(nameIt->second, stats, delta);
            key.delta = delta;
            key.statePercent = stats->getStatPenaltiesTotalForGUI(
                nameIt->second, window->statProgress);
            key.indoors = IndoorBonus(stats, window->statProgress);
            key.stance = StanceBonus(stats, window->statProgress);
            FlatEquipmentBonus(stats, window->statProgress, key.gear);
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

            // Роспись - игры, слово в слово (19.10.2026): «Навык», раны,
            // протезы («Роботехника +56%»), надетое («Доспех/снаряжение
            // +20%») и «Всего». Числа в ней перемножаются ровно в итог.
            // Прежде плагин считал протезы и снаряжение сам и учитывал
            // протезы дважды: процент игры их уже содержит («Протезы
            // +43 / снаряжение -43» при скобке +36). А саму роспись не
            // показывал вовсе, если у персонажа одни прибавки: функция
            // игры отвечает true, только когда есть ШТРАФ, строки же
            // заполняет всегда - смотрим на их число.
            if (known)
            {
                const StatsEnumerated stat = window->statProgress;
                const bool combat = IsCombatStat(stat);

                std::vector<std::pair<std::string, std::string> > game;
                const std::string names[2] =
                {
                    nameIt->second,
                    CharStats::getStatName(stat)
                };
                for (int n = 0; n < 2 && game.empty(); ++n)
                {
                    if (names[n].empty() || (n == 1 && names[1] == names[0]))
                        continue;
                    // У lektor нет деструктора (буфер игры не освобождается) -
                    // один на всё время, иначе каждое описание теряло память.
                    static lektor<StringPair> rows;
                    rows.clear();
                    stats->getStatPenaltiesForGUI(names[n], stat, rows);
                    for (uint32_t i = 0; i < rows.size(); ++i)
                        game.push_back(std::make_pair(rows[i].s1, rows[i].s2));
                }

                // Есть что сказать: больше, чем «Навык» и «Всего», или скобка.
                const bool rich = game.size() > 2;
                const int room = PENALTY_SLOTS - 1;

                if (combat && (rich || delta != 0))
                {
                    // Свои строки - первыми: место в панели ограничено, и
                    // итог не должен уйти за край из-за длинной росписи.
                    AddCombatBonusRows(stats, stat);
                    std::vector<std::pair<std::string, std::string> > own;
                    own.swap(g_describedRows);

                    // Своё «Всего» игры у боевого навыка - не итог (оружие,
                    // броня и стойка в нём не учтены): его не пишем. У
                    // уворота раны уже в своих строках (getDodgePenalty_*) -
                    // из росписи игры только «Навык», иначе раны дважды.
                    size_t keep = game.empty() ? 0 : game.size() - 1;
                    if (stat == STAT_DODGE && keep > 1)
                        keep = 1;
                    const size_t space = static_cast<size_t>(room) - 1 - own.size();
                    for (size_t i = 0; i < keep && i < space; ++i)
                        g_describedRows.push_back(game[i]);
                    g_describedRows.insert(g_describedRows.end(), own.begin(), own.end());

                    char number[32];
                    sprintf_s(number, "%d", static_cast<int>(floorf(EffectiveValue(stats, stat))));
                    g_describedRows.push_back(
                        std::make_pair(std::string(Tr("Effective value")) + ":", std::string(number)));
                }
                else if (!combat && (rich || delta != 0))
                {
                    // Её «Всего» и есть итог - своего не дописываем (было два).
                    for (size_t i = 0; i < game.size() && static_cast<int>(g_describedRows.size()) < room; ++i)
                        g_describedRows.push_back(game[i]);

                    // Росписи нет совсем - хотя бы саму поправку и итог.
                    if (game.empty())
                    {
                        AddRow("Equipment total", delta);
                        char number[32];
                        sprintf_s(number, "%d", static_cast<int>(floorf(EffectiveValue(stats, stat))));
                        g_describedRows.push_back(
                            std::make_pair(std::string(Tr("Effective value")) + ":", std::string(number)));
                    }
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
                // Знак - зелёный или красный; число без знака («Навык: 39»,
                // «Всего: 43» из росписи игры) - цветом подписи.
                const bool minus = (right.find('-') != std::string::npos);
                const bool plus = (right.find('+') != std::string::npos);
                if (line->w2 != NULL)
                    line->w2->setTextColour(minus ? g_minusColour
                                                  : (plus ? g_plusColour : g_labelColour));
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


    // ---------------------------------------------------------------
    // Множители расы над именем выделенного (18.10.2026)
    //
    // Просьба пользователя: перед наймом выделить персонажа, зажать
    // клавишу - и над плашкой с его именем видны множители опыта его
    // расы, как в окне статов (рас в сборке много, все не упомнишь).
    // Работает для любого выделенного персонажа - своего, чужого, зверя.
    //
    // Плашку игры не трогаем: панель - свой виджет над ней (приписку
    // отношения фракции к той же плашке делает UITweaks). Плашку ищем,
    // как UITweaks (NamePanel): виджет с именем на «NameText» - но с
    // двумя надписями внутри (FindNamePlate); поиск не чаще раза в
    // секунду, найденный - в WidgetRef. Строки пересобираются, только
    // когда сменилась раса; место - каждый кадр от плашки (она
    // выдвигается). Клавиша отпущена - за кадр одна проверка клавиши.
    // ---------------------------------------------------------------

    WidgetRef* g_namePanelRef = new WidgetRef();   // не удаляются: см. WidgetRef.h
    DWORD g_namePanelSearchMs = 0;
    WidgetRef* g_selPanelRef = new WidgetRef();
    std::string g_selSignature;                    // что сейчас в панели

    // Надписей внутри виджета (с вложенными), до limit.
    int CountTexts(MyGUI::Widget* widget, int depth, int limit)
    {
        if (widget == NULL || depth > 5)
            return 0;
        int n = widget->castType<MyGUI::TextBox>(false) != NULL ? 1 : 0;
        for (size_t i = 0; i < widget->getChildCount() && n < limit; ++i)
            n += CountTexts(widget->getChildAt(i), depth + 1, limit - n);
        return n;
    }

    // Плашка выделенного - виджет на «NameText» с именем и фракцией
    // внутри (две надписи и больше). Подпись под портретом в панели
    // отряда тоже кончается на «NameText», но это одна надпись: первая
    // сборка (18.10.2026) нашла её и встала над портретами.
    MyGUI::Widget* FindNamePlate(MyGUI::Widget* widget, int depth)
    {
        if (widget == NULL || depth > 12)
            return NULL;
        if (EndsWith(widget->getName(), "NameText") && CountTexts(widget, 0, 2) >= 2)
            return widget;
        for (size_t i = 0; i < widget->getChildCount(); ++i)
            if (MyGUI::Widget* found = FindNamePlate(widget->getChildAt(i), depth + 1))
                return found;
        return NULL;
    }

    MyGUI::Widget* NamePanel()
    {
        if (MyGUI::Widget* cached = g_namePanelRef->get())
            return cached;
        const DWORD now = GetTickCount();
        if (g_namePanelSearchMs != 0 && now - g_namePanelSearchMs < 1000)
            return NULL;                // ищем не чаще раза в секунду
        g_namePanelSearchMs = now;
        MyGUI::Gui* gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
            return NULL;
        MyGUI::EnumeratorWidgetPtr roots = gui->getEnumerator();
        while (roots.next())
        {
            if (MyGUI::Widget* found = FindNamePlate(roots.current(), 0))
            {
                g_namePanelRef->set(found);
                if (g_debug)
                    DebugLog(("StatColours: name plate found: " + found->getName()).c_str());
                return found;
            }
        }
        return NULL;
    }

    void HideSelectedRacial()
    {
        if (MyGUI::Widget* const panel = g_selPanelRef->get())
            if (panel->getVisible())
                panel->setVisible(false);
    }

    MyGUI::TextBox* PanelText(MyGUI::Widget* panel, MyGUI::Align align, const MyGUI::Colour& colour,
                              const std::string& caption)
    {
        MyGUI::TextBox* const box = panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, 10), MyGUI::Align::Default, "");
        box->setNeedMouseFocus(false);
        box->setNeedKeyFocus(false);
        box->setTextAlign(align | MyGUI::Align::VCenter);
        box->setTextColour(colour);
        box->setCaption(caption);
        return box;
    }

    // Строки панели для расы: пересобираются только при смене расы.
    void BuildSelectedRacial(MyGUI::Widget* panel, RaceData* race)
    {
        while (panel->getChildCount() > 0)
            MyGUI::Gui::getInstance().destroyWidget(panel->getChildAt(0));

        // Строки - те же, что в окне статов: множители, отличные от x1.
        std::vector<RacialRow> rows;
        for (int i = 0; i < RACIAL_COUNT; ++i)
        {
            const float mult = race->getStatMod(RACIAL_STATS[i]);
            if (mult <= 0.001f || std::fabs(mult - 1.0f) < 0.005f)
                continue;
            CharacterStatsWindow::Stat* const entry = CharacterStatsWindow::getStat(RACIAL_STATS[i]);
            RacialRow row;
            row.name = entry != NULL && !entry->name.empty()
                ? entry->name : CharStats::getStatName(RACIAL_STATS[i]);
            row.mult = mult;
            rows.push_back(row);
        }

        const int pad = 10;
        const int gap = 18;
        const std::string raceName = race->data != NULL ? race->data->name : std::string();
        MyGUI::TextBox* const title = PanelText(panel, MyGUI::Align::Left, g_labelColour,
            std::string(Tr("XP MULTIPLIERS")) + (raceName.empty() ? "" : ": " + raceName));
        const int rowHeight = title->getFontHeight() + 3;

        // Ширина - по самому длинному тексту: имя расы и названия навыков
        // целиком (первая сборка резала заголовок по ширине плашки).
        std::vector<MyGUI::TextBox*> names;
        std::vector<MyGUI::TextBox*> values;
        int columnWidth = 0;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            MyGUI::TextBox* const name = PanelText(panel, MyGUI::Align::Left, g_labelColour, rows[i].name);
            MyGUI::TextBox* const value = PanelText(panel, MyGUI::Align::Right,
                rows[i].mult > 1.0f ? g_plusColour : g_minusColour, DecimalText("x%.2f", rows[i].mult));
            names.push_back(name);
            values.push_back(value);
            columnWidth = std::max(columnWidth, name->getTextSize().width + 12 + value->getTextSize().width);
        }
        if (rows.empty())
        {
            MyGUI::TextBox* const none = PanelText(panel, MyGUI::Align::Left, g_labelColour, Tr("all x1"));
            names.push_back(none);
            columnWidth = none->getTextSize().width;
        }

        // Две колонки, если строк много: столбик в три десятка строк над
        // плашкой ушёл бы за верх экрана.
        const int columns = rows.size() > 12 ? 2 : 1;
        const int count = static_cast<int>(names.size());
        const int perColumn = (count + columns - 1) / columns;
        const int inner = std::max(title->getTextSize().width, columnWidth * columns + gap * (columns - 1));
        if (columns == 1)
            columnWidth = inner;
        title->setCoord(pad, pad, inner, rowHeight);

        const int top = pad + rowHeight + 4;
        for (int i = 0; i < count; ++i)
        {
            const int x = pad + (i / perColumn) * (columnWidth + gap);
            const int y = top + (i % perColumn) * rowHeight;
            names[i]->setCoord(x, y, columnWidth, rowHeight);
            if (i < static_cast<int>(values.size()))
                values[i]->setCoord(x, y, columnWidth, rowHeight);
        }
        panel->setSize(inner + pad * 2, top + perColumn * rowHeight + pad);

        if (g_debug)
        {
            char note[128];
            sprintf_s(note, "StatColours: selected racial panel - %u multipliers, %d columns",
                      static_cast<unsigned>(rows.size()), columns);
            DebugLog(std::string(note) + ", race '" + raceName + "'");
        }
    }

    // ---- Нанимаемость: код RecruitMarkers (16.10.2026), перенесён как есть ----
    //
    // Решение пользователя (19.10.2026): панель - только у тех, кого можно
    // нанять; у всех подряд - при включённом подробном журнале ([Debug]).
    // Нанимаемый - у кого в живом диалоге «игрок заговорил со мной» есть
    // открытая реплика с действием «вступить в отряд».

    const bool g_checkConditions = true;
    const int kNodeLimit = 4000;        // реплик за один обход диалога
    const int kDepthLimit = 40;
    const DWORD kCacheMs = 10000;       // ответ по персонажу живёт 10 с

    struct ScanState
    {
        Dialogue* dialogue;
        Character* target;              // собеседник - персонаж игрока
        GameData* me;                   // данные самого персонажа
        int nodes;
    };

    bool IsJoinLine(DialogLineData* line)
    {
        const lektor<DialogLineData::DialogAction*>& actions = line->actions;
        for (uint32_t i = 0; i < actions.size(); ++i)
        {
            const DialogLineData::DialogAction* a = actions[i];
            if (a != NULL && (a->key == DA_JOIN_SQUAD_WITH_EDIT || a->key == DA_JOIN_SQUAD_FAST))
                return true;
        }
        return false;
    }

    bool LineOpen(ScanState& s, DialogLineData* line, bool root)
    {
        if (s.dialogue->isLocked(line))
            return false;
        if (line->onceOnly && line->saidItBefore())
            return false;
        // Разговор для другого конкретного персонажа из общего пакета.
        if (root && s.me != NULL && line->hasSpecificCharacterRequirement() &&
            !line->isForSpecificCharacter(s.me))
            return false;
        if (g_checkConditions && s.target != NULL &&
            !line->checkConditions(s.dialogue, s.target, false))
            return false;
        return true;
    }

    bool FindJoin(ScanState& s, DialogChoiceList* list, int depth, bool root)
    {
        if (list == NULL || depth > kDepthLimit)
            return false;
        const lektor<DialogLineData*>& choices = list->conversationChoices;
        for (uint32_t i = 0; i < choices.size(); ++i)
        {
            DialogLineData* const line = choices[i];
            if (line == NULL || ++s.nodes > kNodeLimit)
                return false;
            if (!LineOpen(s, line, root))
                continue;
            if (IsJoinLine(line))
                return true;
            if (FindJoin(s, line->children, depth + 1, false))
                return true;
        }
        return false;
    }

    // 1 - нанимаемый, 0 - нет, -1 - диалог не прочитался (сбой)
    int ScanDialogueRaw(Character* c, Character* target, int& nodes)
    {
        Dialogue* const dialogue = c->dialogue;
        if (dialogue == NULL)
            return 0;
        ScanState s;
        s.dialogue = dialogue;
        s.target = target;
        s.me = c->data;
        s.nodes = 0;
        int found = 0;
        typedef std::map<EventTriggerEnum, DialogChoiceList*, std::less<EventTriggerEnum>,
                         Ogre::STLAllocator<std::pair<EventTriggerEnum const, DialogChoiceList*>,
                                            Ogre::GeneralAllocPolicy> > Lists;
        const Lists& lists = dialogue->conversationsMain;
        Lists::const_iterator it = lists.find(EV_PLAYER_TALK_TO_ME);
        if (it != lists.end() && FindJoin(s, it->second, 0, true))
            found = 1;
        nodes = s.nodes;
        return found;
    }

    int ScanDialogue(Character* c, Character* target, int& nodes)
    {
        __try
        {
            return ScanDialogueRaw(c, target, nodes);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return -1;
        }
    }

    Character* Talker(Character* npc)
    {
        if (ou == NULL || ou->player == NULL)
            return NULL;
        const Ogre::Vector3 pos = npc->getPosition();
        Character* c = ou->player->getNearestSelectedCharacterTo(pos);
        if (c == NULL)
            c = ou->player->getNearestCharacterTo(pos);
        return c;
    }

    Character* SafeTalker(Character* npc)
    {
        __try
        {
            return Talker(npc);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return NULL;
        }
    }

    // Ответ по выделенному - на 10 с (диалог за это время не меняется, а
    // обход - сотни реплик): пока клавиша зажата, каждый кадр не ходим.
    Character* g_recruitFor = NULL;
    DWORD g_recruitAt = 0;
    bool g_recruitable = false;

    bool SelectedRecruitable(Character* c)
    {
        const DWORD now = GetTickCount();
        if (c == g_recruitFor && now - g_recruitAt < kCacheMs)
            return g_recruitable;
        g_recruitFor = c;
        g_recruitAt = now;
        int nodes = 0;
        g_recruitable = !c->isPlayerCharacter() && ScanDialogue(c, SafeTalker(c), nodes) == 1;
        if (g_debug)
        {
            char note[96];
            sprintf_s(note, "StatColours: recruitable=%d (%d dialogue lines)", g_recruitable ? 1 : 0, nodes);
            DebugLog(std::string(note) + " - " + c->displayName);
        }
        return g_recruitable;
    }

    void UpdateSelectedRacial()
    {
        if (!g_showSelectedRacial || ou == NULL || ou->player == NULL ||
            (g_selectedRacialKey.vk != 0 && !HoldKey::Held(g_selectedRacialKey)))
        {
            HideSelectedRacial();
            return;
        }

        RootObject* const selected = ou->player->selectedObject.getRootObject();
        Character* const who = selected != NULL && selected->getDataType() == CHARACTER
            ? static_cast<Character*>(selected) : NULL;
        // Только нанимаемых; всех - при подробном журнале ([Debug] Debug=1).
        RaceData* const race = who != NULL && (g_debug || SelectedRecruitable(who))
            ? who->getRace() : NULL;
        MyGUI::Widget* const plate = race != NULL ? NamePanel() : NULL;
        if (plate == NULL || !plate->getInheritedVisible())
        {
            HideSelectedRacial();
            return;
        }

        // Плашка на экране - интерфейс давно в работе, палитру спросить можно.
        ResolveThemeColours();

        MyGUI::Widget* panel = g_selPanelRef->get();
        if (panel == NULL)
        {
            panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "Kenshi_FloatingPanelSkin", MyGUI::IntCoord(0, 0, 100, 100),
                MyGUI::Align::Default, "ToolTip", "StatColours_SelectedRacial");
            if (panel == NULL)
                return;
            panel->setNeedMouseFocus(false);
            panel->setNeedKeyFocus(false);
            g_selPanelRef->set(panel);
            g_selSignature.clear();
        }

        char signature[32];
        sprintf_s(signature, "%p", static_cast<void*>(race));
        if (g_selSignature != signature)
        {
            BuildSelectedRacial(panel, race);
            g_selSignature = signature;
        }

        // Место - каждый кадр от плашки: она выдвигается вместе с панелью
        // выделенного (кнопка со стрелкой), панель едет за ней.
        const MyGUI::IntCoord pc = plate->getAbsoluteCoord();
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        // Сдвиг на глаз пользователя (18.10.2026): на 4 px левее и на 14 px
        // выше плашки - так ровно с её рамкой. За левый край экрана пускаем:
        // рамки панелей игры у края тоже заходят за него.
        int left = pc.left - 4;
        if (left + panel->getWidth() > view.width)
            left = view.width - panel->getWidth();
        int top = pc.top - panel->getHeight() - 18;
        if (top < 0)
            top = 0;
        const MyGUI::IntPoint want(left < -8 ? -8 : left, top);
        if (panel->getPosition() != want)
            panel->setPosition(want);
        if (!panel->getVisible())
            panel->setVisible(true);
    }

    // Панель - украшение: сбой не должен ронять игру.
    void SafeSelectedRacial()
    {
        __try
        {
            UpdateSelectedRacial();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static bool told = false;
            if (!told)
            {
                told = true;
                ErrorLog("StatColours: the selected character's racial panel failed once - skipped");
            }
            g_selSignature.clear();
        }
    }


    void (*g_origMainLoop)(GameWorld* thisptr, float time) = NULL;

    void MainLoop_hook(GameWorld* thisptr, float time)
    {
        g_origMainLoop(thisptr, time);

        if (!gui)
            return;

        SafeSelectedRacial();

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
    g_selSignature.clear();     // панель над именем - новыми цветами
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "StatColours", "Stat Colours", ini.c_str(), &McmReload);
    if (api->version >= 2)
        api->info(api, Tr("Colours the skill numbers by level, breaks down what changes them, shows racial XP multipliers and the current XP rate."));
    api->toggle(api, "StatColours", "Enabled", Tr("Enabled"), Tr("Turns the whole plugin on or off."), 1, MCM_RESTART);
    api->section(api, Tr("Racial XP"));
    api->toggle(api, "RacialXP", "Show", Tr("Racial XP multipliers"), Tr("Racial XP multipliers under the attributes in the character window."), 1, 0);
    api->hotkey(api, "RacialXP", "HoldKey", Tr("Show while held"), Tr("The multipliers are shown only while this key is held. Click and press a key; Backspace - no key, always shown."), "ALT", 0);
    api->toggle(api, "RacialXP", "Selected", Tr("Above the selected recruit"), Tr("Racial XP multipliers above the name of the selected character in the world, if they can be recruited - handy before recruiting. With the detailed log on (Diagnostics) - for anyone."), 1, 0);
    api->hotkey(api, "RacialXP", "SelectedKey", Tr("Key above the name"), Tr("The multipliers above the name are shown only while this key is held. Backspace - no key, always shown."), "ALT", 0);
    api->toggle(api, "RacialXP", "ShowRate", Tr("Current XP rate"), Tr("The current XP rate in brackets after the skill name in the skill description."), 1, 0);
    api->section(api, Tr("Colours"));
    api->toggle(api, "Colours", "UseGameTheme", Tr("Colours from the interface theme"), Tr("auto colours below are taken from the game palette, kenshi_colours.xml, and follow an interface mod over it. Off - built-in colours."), 1, 0);
    {
        static const char* const values[] = { "left", "right" };
        const char* const labels[] = { Tr("left"), Tr("right") };
        api->choice(api, "Colours", "BracketSide", Tr("Bracket side"), Tr("Which side of the skill value the brackets with the correction go. The correction is everything that changes the skill right now: equipment, robotic limbs, injuries, hunger, load, indoor combat, combat stance. On the left they can overlap a long skill name."), "left", values, labels, 2, 0);
    }
    api->colour(api, "Colours", "Brackets", Tr("Brackets"), Tr("The brackets around the correction, like (+3). auto - from the palette, or #RRGGBB."), "auto", 0);
    api->colour(api, "Colours", "Bonus", Tr("Bonus"), Tr("The number in the brackets when the correction adds. auto - from the palette, or #RRGGBB."), "auto", 0);
    api->colour(api, "Colours", "Penalty", Tr("Penalty"), Tr("The number in the brackets when the correction takes away. auto - from the palette, or #RRGGBB."), "auto", 0);
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
    // Выключатель всего плагина (15.10.2026): окно характеристик он
    // перекрашивает и растягивает, на ходу это не снять - без хука до
    // перезапуска. Страница MCM остаётся, чтобы включить обратно.
    {
        const std::string ini = IniPath();
        if (!ini.empty() && GetPrivateProfileIntA("StatColours", "Enabled", 1, ini.c_str()) == 0)
        {
            DebugLog("StatColours: turned off in the ini, nothing is coloured");
            return;
        }
    }

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
