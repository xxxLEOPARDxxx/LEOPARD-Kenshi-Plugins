// QuickStartSelect - быстрый выбор начала игры
//
// Своя версия мода «Choose Your Beginning Quicker» (QuickerGameStartSelection,
// Redronn, Nexus 1970): исходников у него нет, написано заново.
//
// Окно игры «Выберите начало» листает старты по одному стрелками. Вместо
// него плагин показывает своё окно со списком всех стартов:
//   * две раскладки - широкая (список слева, описание справа) и
//     компактная (узкое окно, описание под списком), кнопка M;
//   * сортировка по алфавиту (A-Z / Z-A) и по сложности (EASY-HARD /
//     HARD-EASY);
//   * поиск по названию;
//   * фильтр по расе отряда;
//   * дополнительные настройки - своей панелью в нашем окне, те же поля
//     игры; подсказка - внизу; правый клик по строке возвращает значение
//     по умолчанию;
//   * подтверждение перед «Начать».
//
// Как устроено. Окно игры (NewGameWindow) остаётся: оно держит список
// стартов (startsData), настройки и само начинает игру (newGameStart).
// Пока открыто наше, окно игры отодвигаем за край экрана; «Начать» -
// currentStart = выбранный, updateCurrentData, newGameStart, то есть ровно
// то, что игра делает своими стрелками и кнопкой.
//
// Всё, что уничтожает виджеты или зовёт окно игры, делаем не в
// обработчике нажатия, а на следующем кадре (MyGUI eventFrameStart):
// иначе кнопка удалялась бы посреди собственного события. Кадровый хук
// ForgottenGUI::update на главном экране не вызывается - на нём кнопки
// первой сборки и стояли мёртвыми.

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "quick_start_select"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ComboBox.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Window.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ILayer.h>
#include <WidgetRef.h>
#include <GameTheme.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/GameplayOptions.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/gui/DataPanelLine.h>

#define private public
#define protected public
#include <kenshi/gui/NewGameWindow.h>
#include <kenshi/gui/NewGameOptionsWindow.h>
#undef private
#undef protected

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    const char* const SECTION = "QuickStartSelect";

    enum SortMode { SORT_AZ = 0, SORT_DIFFICULTY = 1 };

    bool g_enabled = true;
    bool g_debug = false;
    bool g_compact = false;         // раскладка: широкая или компактная
    int g_sort = SORT_AZ;
    bool g_sortDesc = false;        // Z-A, HARD-EASY
    bool g_confirm = true;          // спросить перед «Начать»
    bool g_rightClickReset = true;  // правый клик по настройке - по умолчанию
    // Размер окна - доля экрана, % (пожелание Асура 08.10.2026).
    int g_wideWidth = 74;
    int g_compactWidth = 32;
    int g_windowHeight = 86;
    // Свои названия сложностей под каждый из 6 цветов (через запятую) - для
    // стартов из модов с нестандартной сложностью («Бип!!»). Пожелание
    // Асура 08.10.2026: цвета остаются встроенными, задаётся текст.
    std::string g_customDifficulty[6];


    std::string IniPath()
    {
        char path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();
        std::string file(path, length);
        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string() : file.substr(0, dot) + ".ini";
    }


    std::string ReadIniString(const char* key, const char* def)
    {
        const std::string ini = IniPath();
        char buf[64] = {};
        GetPrivateProfileStringA(SECTION, key, def, buf, sizeof(buf), ini.c_str());
        std::string v(buf);
        for (size_t i = 0; i < v.size(); ++i)
            v[i] = static_cast<char>(tolower(static_cast<unsigned char>(v[i])));
        return v;
    }


    extern bool g_needLayout;
    extern bool g_open;

    // Строка как есть (UTF-8), без перевода в нижний регистр.
    std::string ReadIniRaw(const char* key)
    {
        const std::string ini = IniPath();
        char buf[1024] = {};
        GetPrivateProfileStringA(SECTION, key, "", buf, sizeof(buf), ini.c_str());
        return buf;
    }

    void RecomputeRanks();

    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        g_enabled = GetPrivateProfileIntA(SECTION, "Enabled", 1, ini.c_str()) != 0;
        g_debug = GetPrivateProfileIntA(SECTION, "Debug", 0, ini.c_str()) != 0;
        g_compact = ReadIniString("Layout", "wide") == "compact";
        g_sort = ReadIniString("Sort", "az") == "difficulty" ? SORT_DIFFICULTY : SORT_AZ;
        g_sortDesc = GetPrivateProfileIntA(SECTION, "SortDesc", 0, ini.c_str()) != 0;
        g_confirm = GetPrivateProfileIntA(SECTION, "ConfirmBegin", 1, ini.c_str()) != 0;
        g_rightClickReset = GetPrivateProfileIntA(SECTION, "RightClickReset", 1, ini.c_str()) != 0;
        g_wideWidth = std::max(50, std::min(100, static_cast<int>(GetPrivateProfileIntA(SECTION, "WideWidth", 74, ini.c_str()))));
        g_compactWidth = std::max(25, std::min(70, static_cast<int>(GetPrivateProfileIntA(SECTION, "CompactWidth", 32, ini.c_str()))));
        g_windowHeight = std::max(50, std::min(100, static_cast<int>(GetPrivateProfileIntA(SECTION, "WindowHeight", 86, ini.c_str()))));
        static const char* const tierKeys[6] = { "DifficultyVeryEasy", "DifficultyEasy", "DifficultyNormal",
                                                 "DifficultyAboveNormal", "DifficultyHard", "DifficultyVeryHard" };
        for (int i = 0; i < 6; ++i)
            g_customDifficulty[i] = ReadIniRaw(tierKeys[i]);
        RecomputeRanks();
        g_needLayout = g_open;          // открыто - пересобрать с новым размером и цветами
    }


    // Раскладку (M) и сортировку окно запоминает само.
    void SaveViewSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        WritePrivateProfileStringA(SECTION, "Layout", g_compact ? "compact" : "wide", ini.c_str());
        WritePrivateProfileStringA(SECTION, "Sort", g_sort == SORT_DIFFICULTY ? "difficulty" : "az", ini.c_str());
        WritePrivateProfileStringA(SECTION, "SortDesc", g_sortDesc ? "1" : "0", ini.c_str());
    }


    // ---------------------------------------------------------------
    // Текст
    // ---------------------------------------------------------------

    std::wstring Widen(const std::string& s)
    {
        if (s.empty())
            return std::wstring();
        const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), NULL, 0);
        if (n <= 0)
            return std::wstring();
        std::wstring w(static_cast<size_t>(n), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), &w[0], n);
        return w;
    }


    // Нижний регистр и для кириллицы; «ё» = «е», чтобы поиск их не различал.
    std::wstring Lower(const std::string& s)
    {
        std::wstring w = Widen(s);
        if (!w.empty())
            CharLowerBuffW(&w[0], static_cast<DWORD>(w.size()));
        for (size_t i = 0; i < w.size(); ++i)
            if (w[i] == 0x0451)
                w[i] = 0x0435;
        return w;
    }


    // MyGUI принимает «#RRGGBB» в подписи за смену цвета; «##» - это «#».
    std::string EscapeTags(const std::string& v)
    {
        std::string r;
        for (size_t k = 0; k < v.size(); ++k)
        {
            r += v[k];
            if (v[k] == '#')
                r += '#';
        }
        return r;
    }


    std::string Money(int n)
    {
        char raw[32];
        sprintf_s(raw, "%d", n < 0 ? -n : n);
        std::string digits(raw), out;
        for (size_t i = 0; i < digits.size(); ++i)
        {
            if (i > 0 && (digits.size() - i) % 3 == 0)
                out += ',';
            out += digits[i];
        }
        return (n < 0 ? "-c." : "c.") + out;
    }


    // ---------------------------------------------------------------
    // Сложность
    //
    // Сложность у старта - просто строка, переведённая вместе с данными
    // игры («Hard», «Тяжёлая», «Easy/Hard combination»). Числа нет, поэтому
    // ранжируем по словам. Порядок проверки важен: «very hard» раньше
    // «hard», «harder» раньше «hard», смешанные («Easy/Hard») раньше обоих.
    // Кириллица - кодами: в бинарнике строк на русском быть не должно.
    // ---------------------------------------------------------------

    struct Keyword
    {
        const wchar_t* word;
        int rank;
    };

    const Keyword DIFFICULTY_WORDS[] = {
        { L"default", 30 },             // «не задано» - как обычная
        { L"\x043e\x0431\x044b\x0447\x043d", 30 },                              // обычн
        { L"very easy", 10 },
        { L"\x043e\x0447\x0435\x043d\x044c \x043b\x0435\x0433", 10 },           // очень лег(ё)
        { L"easy/hard", 38 },
        { L"normal/hard", 38 },
        { L"\x0441\x043c\x0435\x0448\x0430\x043d", 38 },                        // смешан
        { L"\x0434\x0432\x043e\x0435", 38 },                                    // двое
        { L"hardest", 70 },
        { L"very hard", 60 },
        { L"\x043e\x0447\x0435\x043d\x044c \x0442\x044f\x0436", 60 },           // очень тяж
        { L"\x0432\x044b\x0448\x0435 \x0441\x0440\x0435\x0434\x043d", 40 },   // выше средн(его) - раньше «средн»
        { L"harder", 40 },
        { L"\x043f\x043e\x0441\x043b\x043e\x0436\x043d\x0435\x0435", 40 },      // посложнее
        { L"\x0441\x043b\x043e\x0436\x043d\x0435\x0435", 40 },                  // сложнее
        { L"dodgy", 45 },
        { L"\x0449\x0435\x043a\x043e\x0442\x043b", 45 },                        // щекотл
        { L"slightly", 35 },
        { L"kinda", 35 },
        { L"easier", 15 },
        { L"\x043f\x043e\x043b\x0435\x0433\x0447\x0435", 15 },                  // полегче
        { L"easy", 20 },
        { L"\x043b\x0435\x0433", 20 },                                          // лег(ё)
        { L"hard", 50 },
        { L"\x0442\x044f\x0436", 50 },                                          // тяж
        { L"\x0436\x0435\x0441\x0442\x043a", 50 },                              // жест(ё)к
        { L"normal", 30 },
        { L"doable", 30 },
        { L"\x0443\x043c\x0435\x0440\x0435\x043d", 30 },                        // умерен
        { L"\x0441\x0440\x0435\x0434\x043d", 30 },                              // средн
        { L"\x043d\x043e\x0440\x043c\x0430\x043b", 30 },                        // нормал
    };

    const int RANK_UNKNOWN = 30;

    // Ранг-представитель каждого из 6 цветов (см. RankColour).
    const int TIER_RANK[6] = { 10, 20, 30, 40, 50, 60 };

    // Свои названия из настроек: вхождение без учёта регистра, через
    // запятую или точку с запятой. -1 - не задано.
    int CustomRank(const std::wstring& w)
    {
        for (int t = 0; t < 6; ++t)
        {
            const std::string& list = g_customDifficulty[t];
            size_t from = 0;
            while (from <= list.size())
            {
                size_t to = list.find_first_of(",;", from);
                if (to == std::string::npos)
                    to = list.size();
                std::string item = list.substr(from, to - from);
                while (!item.empty() && (item[0] == ' ' || item[0] == '\t'))
                    item.erase(0, 1);
                while (!item.empty() && (item[item.size() - 1] == ' ' || item[item.size() - 1] == '\t'))
                    item.erase(item.size() - 1);
                if (!item.empty() && w.find(Lower(item)) != std::wstring::npos)
                    return TIER_RANK[t];
                from = to + 1;
            }
        }
        return -1;
    }

    int DifficultyRank(const std::string& text)
    {
        const std::wstring w = Lower(text);    // «ё» уже стала «е»
        if (w.empty())
            return RANK_UNKNOWN;
        const int custom = CustomRank(w);
        if (custom >= 0)
            return custom;
        for (size_t i = 0; i < sizeof(DIFFICULTY_WORDS) / sizeof(DIFFICULTY_WORDS[0]); ++i)
            if (w.find(DIFFICULTY_WORDS[i].word) != std::wstring::npos)
                return DIFFICULTY_WORDS[i].rank;
        return RANK_UNKNOWN;
    }


    // Цвет сложности в списке: от зелёного к красному.
    MyGUI::Colour Ink(const MyGUI::Colour& c);

    MyGUI::Colour RankColour(int rank)
    {
        if (rank <= 15) return Ink(MyGUI::Colour(0.55f, 0.74f, 0.45f));
        if (rank <= 25) return Ink(MyGUI::Colour(0.68f, 0.76f, 0.47f));
        if (rank <= 32) return Ink(MyGUI::Colour(0.80f, 0.74f, 0.50f));
        if (rank <= 45) return Ink(MyGUI::Colour(0.85f, 0.62f, 0.40f));
        if (rank <= 55) return Ink(MyGUI::Colour(0.85f, 0.48f, 0.36f));
        return Ink(MyGUI::Colour(0.86f, 0.34f, 0.32f));
    }


    // ---------------------------------------------------------------
    // Старты
    // ---------------------------------------------------------------

    struct Start
    {
        int index;                      // номер в NewGameWindow::startsData
        std::string name;
        std::wstring nameLower;
        std::string difficulty;
        int rank;
        std::string description;
        std::string style;
        int money;
        std::set<std::string> races;
        // Список «town» старта: один - там и начнёшь, несколько - игра
        // выбирает случайный. chosenTown - выбранный игроком (-1 - случайный).
        std::vector<GameData*> towns;
        int chosenTown;
        int characters;                 // лидеры и члены отрядов - сколько их выйдет
        int charactersMax;              // со случайными добавочными («num random chars max»)
        int animals;                    // список «animals» отрядов
        // «faction relations»: фракция и отношение (value[0]).
        std::vector<std::pair<std::string, int> > relations;
    };

    std::vector<Start> g_starts;

    // Настройки поменялись - ранги заново (цвет и сортировка по сложности).
    void RecomputeRanks()
    {
        for (size_t i = 0; i < g_starts.size(); ++i)
            g_starts[i].rank = DifficultyRank(g_starts[i].difficulty);
    }
    std::vector<std::string> g_raceNames;   // по алфавиту, для выпадающего списка
    std::map<std::string, int> g_raceCount;


    std::string StrField(GameData* d, const char* key)
    {
        ogre_unordered_map<std::string, std::string>::type::iterator it = d->sdata.find(key);
        return it == d->sdata.end() ? std::string() : it->second;
    }

    int IntField(GameData* d, const char* key, int def)
    {
        ogre_unordered_map<std::string, int>::type::iterator it = d->idata.find(key);
        return it == d->idata.end() ? def : it->second;
    }


    void RefDatas(GameData* d, const char* list, std::vector<GameData*>& out)
    {
        out.clear();
        if (d == NULL || ou == NULL)
            return;
        const Ogre::vector<GameDataReference>::type* refs = d->getReferenceListIfExists(list);
        if (refs == NULL)
            return;
        for (size_t i = 0; i < refs->size(); ++i)
        {
            GameData* const p = (*refs)[i].getPtr(&ou->gamedata);
            if (p != NULL)
                out.push_back(p);
        }
    }


    // Сколько их по списку: число в ссылке - количество («settler 1» x4 у
    // Искателей свободы). Раньше считались записи, и одинаковые персонажи
    // шли за одного (08.10.2026, Асур: «персов 2, а в отряде 4»).
    int RefCount(GameData* d, const char* list)
    {
        if (d == NULL || ou == NULL)
            return 0;
        const Ogre::vector<GameDataReference>::type* refs = d->getReferenceListIfExists(list);
        if (refs == NULL)
            return 0;
        int n = 0;
        for (size_t i = 0; i < refs->size(); ++i)
            if ((*refs)[i].getPtr(&ou->gamedata) != NULL)
                n += std::max(1, (*refs)[i].values.value[0]);
        return n;
    }


    // Раса персонажа. Список «race» у CHARACTER: одна запись - эта раса,
    // несколько - одна из них случайно (берём все), пусто - игра выбирает
    // случайного человека.
    void CharacterRaces(GameData* c, std::set<std::string>& out)
    {
        std::vector<GameData*> races;
        RefDatas(c, "race", races);
        if (races.empty())
        {
            out.insert(Tr("Human (random)"));
            return;
        }
        for (size_t i = 0; i < races.size(); ++i)
            if (!races[i]->name.empty())
                out.insert(races[i]->name);
    }


    // Старт -> отряды (squad) -> лидер и состав (leader, squad) -> раса.
    // Животных отряда (animals) не считаем: фильтр - по тем, кем играешь.
    void StartRaces(GameData* start, std::set<std::string>& out)
    {
        std::vector<GameData*> squads, members;
        RefDatas(start, "squad", squads);
        for (size_t s = 0; s < squads.size(); ++s)
        {
            const char* const lists[] = { "leader", "squad" };
            for (size_t l = 0; l < 2; ++l)
            {
                RefDatas(squads[s], lists[l], members);
                for (size_t m = 0; m < members.size(); ++m)
                    CharacterRaces(members[m], out);
            }
        }
    }


    void LoadStarts(NewGameWindow* w)
    {
        g_starts.clear();
        g_raceNames.clear();
        g_raceCount.clear();
        const size_t n = w->startsData.size();
        GameData* const* datas = w->startsData.begin();
        for (size_t i = 0; i < n; ++i)
        {
            GameData* const d = datas[i];
            if (d == NULL)
                continue;
            // Пустышка: в сборке есть только переименование старта, а мода,
            // который его создаёт, нет - ни отряда, ни денег, ни описания.
            // Начать ею нельзя, в списке не показываем.
            const Ogre::vector<GameDataReference>::type* const squads = d->getReferenceListIfExists("squad");
            if (squads == NULL || squads->empty())
            {
                if (g_debug)
                    DebugLog("QuickStartSelect: start '" + d->name + "' has no squad, hidden");
                continue;
            }
            Start s;
            s.index = static_cast<int>(i);
            s.name = d->name;
            s.nameLower = Lower(d->name);
            s.difficulty = StrField(d, "difficulty");
            s.rank = DifficultyRank(s.difficulty);
            s.description = StrField(d, "description");
            s.style = StrField(d, "style");
            s.money = IntField(d, "money", 0);
            StartRaces(d, s.races);
            RefDatas(d, "town", s.towns);
            s.chosenTown = -1;
            s.characters = 0;
            s.charactersMax = 0;
            s.animals = 0;
            {
                std::vector<GameData*> squadList;
                RefDatas(d, "squad", squadList);
                for (size_t q = 0; q < squadList.size(); ++q)
                {
                    const int fixed = RefCount(squadList[q], "leader") + RefCount(squadList[q], "squad");
                    const int randMin = std::max(0, IntField(squadList[q], "num random chars", 0));
                    const int randMax = std::max(randMin, IntField(squadList[q], "num random chars max", 0));
                    s.characters += fixed + randMin;
                    s.charactersMax += fixed + randMax;
                    s.animals += RefCount(squadList[q], "animals");
                }
            }
            if (const Ogre::vector<GameDataReference>::type* rel = d->getReferenceListIfExists("faction relations"))
                for (size_t r = 0; r < rel->size() && ou != NULL; ++r)
                {
                    GameData* const f = (*rel)[r].getPtr(&ou->gamedata);
                    if (f != NULL && !f->name.empty())
                        s.relations.push_back(std::make_pair(f->name, (*rel)[r].values.value[0]));
                }
            for (std::set<std::string>::const_iterator r = s.races.begin(); r != s.races.end(); ++r)
                ++g_raceCount[*r];
            g_starts.push_back(s);

            if (g_debug)
            {
                std::string races;
                for (std::set<std::string>::const_iterator r = s.races.begin(); r != s.races.end(); ++r)
                    races += (races.empty() ? "" : ", ") + *r;
                char extra[64];
                sprintf_s(extra, " | rank %d | money %d | ", s.rank, s.money);
                DebugLog("QuickStartSelect: start '" + s.name + "' | " + s.difficulty + extra + races);
            }
        }
        for (std::map<std::string, int>::const_iterator r = g_raceCount.begin(); r != g_raceCount.end(); ++r)
            g_raceNames.push_back(r->first);
    }


    // ---------------------------------------------------------------
    // Окно
    // ---------------------------------------------------------------

    NewGameWindow* g_ngw = NULL;
    bool g_open = false;   // объявлен выше extern (LoadSettings)
    bool g_openFailed = false;          // не открылось - не пробовать до следующего показа

    WidgetRef* g_winRef = NULL;         // наше окно; обнулится, если игра снесёт GUI
    WidgetRef* g_confirmRef = NULL;     // окно «Начать?»

    MyGUI::Window* g_win = NULL;
    MyGUI::Button* g_btnLayout = NULL;
    MyGUI::Button* g_btnAz = NULL;
    MyGUI::Button* g_btnDiff = NULL;
    MyGUI::ComboBox* g_raceBox = NULL;
    MyGUI::EditBox* g_search = NULL;
    MyGUI::Button* g_btnClear = NULL;
    MyGUI::TextBox* g_count = NULL;
    MyGUI::ScrollView* g_list = NULL;
    MyGUI::EditBox* g_title = NULL;
    MyGUI::TextBox* g_infoKeys[3] = { NULL, NULL, NULL };
    MyGUI::TextBox* g_infoVals[3] = { NULL, NULL, NULL };
    // Вторая колонка: Город, Персонажи, Отношения (как у «Choose Your
    // Beginning Quicker»). По городу и отношениям - щелчок, окно выбора.
    MyGUI::TextBox* g_infoKeys2[3] = { NULL, NULL, NULL };
    MyGUI::TextBox* g_infoVals2[3] = { NULL, NULL, NULL };
    MyGUI::Widget* g_underline[3] = { NULL, NULL, NULL };   // под щёлкаемыми значениями
    WidgetRef* g_pickRef = NULL;        // окно «Города» / «Отношения»
    bool g_needPickClose = false;
    MyGUI::ScrollView* g_descView = NULL;
    MyGUI::EditBox* g_desc = NULL;
    MyGUI::Button* g_btnOptions = NULL;
    MyGUI::Button* g_btnBegin = NULL;
    MyGUI::EditBox* g_tipText = NULL;   // подсказка к настройке

    struct Row
    {
        MyGUI::Button* button;
        MyGUI::TextBox* name;
        MyGUI::TextBox* difficulty;
        int start;                      // номер в g_starts
    };
    std::vector<Row> g_rows;

    int g_selected = -1;                // номер в g_starts
    int g_raceFilter = -1;              // -1 - все, иначе номер в g_raceNames
    std::wstring g_filterText;
    bool g_showOptions = false;

    // Отложенное - на следующий кадр.
    bool g_needLayout = false;          // пересоздать окно целиком
    bool g_needList = false;            // пересобрать строки списка
    bool g_needBegin = false;           // начать игру выбранным стартом
    bool g_needClose = false;           // закрыть окно игры (крестик)
    bool g_needConfirmClose = false;

    MyGUI::IntSize g_viewSize;
    MyGUI::IntPoint g_vanillaPos;       // где стояло окно игры
    bool g_vanillaMoved = false;

    int g_rowH = 28;
    int g_lastClickRow = -1;
    DWORD g_lastClickMs = 0;

    const int PAD = 8;
    const int GAP = 6;
    // setLineSpacing у DatapanelGUI - строк на экран, а не пиксели.
    const float OPT_LINES_PER_SCREEN = 34.0f;
    int OPT_LINE_H = 32;                // по нему - высота панели настроек


    // Ваниль - светлый пергамент, текст тёмный; мод на интерфейс (Russian
    // Dark UI) - тёмные окна, текст светлый (07.10.2026: на ванили светлые
    // надписи на пергаменте не читались, а «Kenshi_GenericTextBox» там -
    // пергаментная плашка; надписи теперь на прозрачном
    // «Kenshi_TextboxStandardText»).
    bool Parchment()
    {
        return !GameTheme::PaletteFromMod();
    }

    // Цвет «для тёмного фона» - на пергаменте темнее.
    MyGUI::Colour Ink(const MyGUI::Colour& c)
    {
        return Parchment() ? MyGUI::Colour(c.red * 0.55f, c.green * 0.55f, c.blue * 0.55f) : c;
    }

    MyGUI::Colour TextColour()
    {
        if (Parchment())
            return MyGUI::Colour(0.20f, 0.15f, 0.11f);
        return GameTheme::ReadableColour("Main", "#AFA68B");
    }

    MyGUI::Colour LabelColour()
    {
        if (Parchment())
            return MyGUI::Colour(0.36f, 0.28f, 0.20f);
        const MyGUI::Colour c = TextColour();
        return MyGUI::Colour(c.red * 0.72f, c.green * 0.72f, c.blue * 0.72f);
    }

    MyGUI::Colour GoldColour()
    {
        if (Parchment())
            return MyGUI::Colour(0.45f, 0.17f, 0.10f);      // как заголовки ванили
        return MyGUI::Colour(0.93f, 0.86f, 0.56f);
    }


    MyGUI::Widget* VanillaMain(NewGameWindow* w)
    {
        return w == NULL ? NULL : static_cast<wraps::BaseLayout*>(w)->mMainWidget;
    }


    MyGUI::Widget* OptionsMain(NewGameWindow* w)
    {
        if (w == NULL || w->newGameOptions == NULL)
            return NULL;
        return static_cast<wraps::BaseLayout*>(w->newGameOptions)->mMainWidget;
    }


    void ForgetWidgets()
    {
        g_win = NULL;
        g_btnLayout = g_btnAz = g_btnDiff = g_btnClear = g_btnOptions = g_btnBegin = NULL;
        g_raceBox = NULL;
        g_search = NULL;
        g_count = NULL;
        g_list = NULL;
        g_title = NULL;
        for (int i = 0; i < 3; ++i)
        {
            g_infoKeys[i] = g_infoVals[i] = g_infoKeys2[i] = g_infoVals2[i] = NULL;
            g_underline[i] = NULL;
        }
        g_descView = NULL;
        g_desc = NULL;
        g_tipText = NULL;
        g_rows.clear();
    }


    void DestroyOptionsPanel();
    void BuildOptionsPanel(MyGUI::Widget* host);


    void DestroyConfirm()
    {
        if (g_confirmRef == NULL || g_confirmRef->get() == NULL)
            return;
        MyGUI::Widget* const w = g_confirmRef->get();
        MyGUI::InputManager::getInstance().removeWidgetModal(w);
        g_confirmRef->reset();
        MyGUI::Gui::getInstance().destroyWidget(w);
    }


    void DestroyPick();

    void DestroyWindow()
    {
        DestroyConfirm();
        DestroyPick();
        DestroyOptionsPanel();          // панель игры - до виджетов, на которых она стоит
        if (g_winRef != NULL && g_winRef->get() != NULL)
        {
            MyGUI::Widget* const w = g_winRef->get();
            g_winRef->reset();
            MyGUI::Gui::getInstance().destroyWidget(w);
        }
        ForgetWidgets();
    }


    // --- что видно в списке ---

    bool Passes(const Start& s)
    {
        if (g_raceFilter >= 0 && g_raceFilter < static_cast<int>(g_raceNames.size()) &&
            s.races.count(g_raceNames[g_raceFilter]) == 0)
            return false;
        if (!g_filterText.empty() && s.nameLower.find(g_filterText) == std::wstring::npos)
            return false;
        return true;
    }


    struct ByOrder
    {
        bool operator()(int a, int b) const
        {
            const Start& x = g_starts[a];
            const Start& y = g_starts[b];
            if (g_sort == SORT_DIFFICULTY && x.rank != y.rank)
                return g_sortDesc ? x.rank > y.rank : x.rank < y.rank;
            const int c = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                         x.nameLower.c_str(), -1, y.nameLower.c_str(), -1);
            if (c != CSTR_EQUAL)
                return (g_sort == SORT_AZ && g_sortDesc) ? c == CSTR_GREATER_THAN : c == CSTR_LESS_THAN;
            return x.index < y.index;
        }
    };


    void UpdateSortCaptions()
    {
        if (g_btnAz != NULL)
        {
            g_btnAz->setCaption(g_sort == SORT_AZ && g_sortDesc ? "Z-A" : "A-Z");
            g_btnAz->setStateSelected(g_sort == SORT_AZ);
        }
        if (g_btnDiff != NULL)
        {
            g_btnDiff->setCaption(g_sort == SORT_DIFFICULTY && g_sortDesc ? Tr("HARD-EASY") : Tr("EASY-HARD"));
            g_btnDiff->setStateSelected(g_sort == SORT_DIFFICULTY);
        }
    }


    // ---- Город, персонажи, отношения --------------------------------

    std::string TownCaption(const Start& s)
    {
        if (s.towns.empty())
            return "-";
        if (s.chosenTown >= 0 && s.chosenTown < static_cast<int>(s.towns.size()))
            return s.towns[s.chosenTown]->name;
        std::string all;
        for (size_t i = 0; i < s.towns.size(); ++i)
            all += (all.empty() ? "" : ", ") + s.towns[i]->name;
        return all;
    }

    void ShowTownAndRelations(const Start* s)
    {
        if (g_infoVals2[0] == NULL)
            return;
        const bool pickTown = s != NULL && s->towns.size() > 1;
        const bool pickRel = s != NULL && !s->relations.empty();
        g_infoVals2[0]->setCaption(s ? EscapeTags(TownCaption(*s)) : std::string());
        g_infoVals2[0]->setTextColour(pickTown ? GoldColour() : TextColour());
        char count[96];
        count[0] = 0;
        if (s != NULL)
        {
            if (s->charactersMax > s->characters)
                sprintf_s(count, "%d-%d", s->characters, s->charactersMax);
            else
                sprintf_s(count, "%d", s->characters);
            if (s->animals > 0)
            {
                char more[64];
                sprintf_s(more, TrFmt(", animals: %d"), s->animals);
                strcat_s(count, more);
            }
        }
        g_infoVals2[1]->setCaption(count);
        std::string rel;
        for (size_t i = 0; s != NULL && i < s->relations.size(); ++i)
            rel += (rel.empty() ? "" : ", ") + s->relations[i].first;
        g_infoVals2[2]->setCaption(s ? EscapeTags(rel.empty() ? std::string("-") : rel) : std::string());
        g_infoVals2[2]->setTextColour(pickRel ? GoldColour() : TextColour());
        const bool pick[3] = { pickTown, false, pickRel };
        for (int i = 0; i < 3; ++i)
        {
            if (g_underline[i] == NULL)
                continue;
            g_underline[i]->setVisible(pick[i]);
            if (pick[i])
            {
                const int w = std::min(g_infoVals2[i]->getTextSize().width, g_infoVals2[i]->getWidth());
                g_underline[i]->setSize(std::max(1, w), 1);
                g_underline[i]->setColour(GoldColour());
            }
        }
    }

    void DestroyPick()
    {
        if (g_pickRef == NULL || g_pickRef->get() == NULL)
            return;
        MyGUI::Widget* const w = g_pickRef->get();
        MyGUI::InputManager::getInstance().removeWidgetModal(w);
        g_pickRef->reset();
        MyGUI::Gui::getInstance().destroyWidget(w);
    }

    void OnPickWindowButton(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
            g_needPickClose = true;
    }

    void OnPickClose(MyGUI::Widget*)
    {
        g_needPickClose = true;
    }

    void OnTownChosen(MyGUI::Widget* sender)
    {
        if (g_selected >= 0 && g_selected < static_cast<int>(g_starts.size()))
        {
            g_starts[g_selected].chosenTown = atoi(sender->getUserString("qss_town").c_str());
            ShowTownAndRelations(&g_starts[g_selected]);
        }
        g_needPickClose = true;
    }

    MyGUI::Button* MakeButton(MyGUI::Widget* parent, const std::string& caption, int x, int y, int w, int h,
                              MyGUI::Align align = MyGUI::Align::Default);

    // Окно «Города» (выбор) или «Отношения» (список) - по центру, модальное.
    void ShowPick(bool town)
    {
        DestroyPick();
        if (g_selected < 0 || g_selected >= static_cast<int>(g_starts.size()) || g_win == NULL)
            return;
        const Start& st = g_starts[g_selected];
        const int rows = town ? static_cast<int>(st.towns.size()) + 1 : static_cast<int>(st.relations.size());
        if ((town && st.towns.size() < 2) || (!town && rows == 0))
            return;
        // Размер - по содержимому (08.10.2026: окно отношений было широким,
        // у городов внизу оставалось пустое место). Сначала строим с
        // запасом, меряем надписи, потом ужимаем окно и строки.
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        const int startW = 600;
        const int startH = 400;
        MyGUI::Window* const box = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("Kenshi_WindowCX",
            MyGUI::IntCoord((view.width - startW) / 2, (view.height - startH) / 2, startW, startH),
            MyGUI::Align::Default, g_win->getLayer() ? g_win->getLayer()->getName() : "Window",
            "QuickStartSelect_Pick");
        box->setCaption(town ? Tr("Towns") : Tr("Relations"));
        box->eventWindowButtonPressed += MyGUI::newDelegate(&OnPickWindowButton);
        if (g_pickRef == NULL)
            g_pickRef = new WidgetRef();        // не удаляется: см. WidgetRef.h
        g_pickRef->set(box);

        const MyGUI::IntCoord client = box->getClientCoord();
        const int frameW = startW - client.width;
        const int frameH = startH - client.height;
        const int captionW = box->getCaptionWidget() != NULL ? box->getCaptionWidget()->getTextSize().width : 0;
        int y = PAD;
        int contentW = std::max(180, captionW + 60);   // заголовок и крестик должны влезть
        std::vector<MyGUI::Widget*> full;               // во всю ширину
        std::vector<std::pair<MyGUI::Widget*, MyGUI::Widget*> > pairs;   // имя - число
        int nameW = 0;
        int valueW = 0;
        if (town)
        {
            for (int i = -1; i < static_cast<int>(st.towns.size()); ++i)
            {
                const std::string caption = i < 0 ? std::string(Tr("Random")) : st.towns[i]->name;
                MyGUI::Button* const b = MakeButton(box, EscapeTags(caption), PAD, y, client.width - PAD * 2, g_rowH,
                                                    MyGUI::Align::Default);
                char idx[16];
                sprintf_s(idx, "%d", i);
                b->setUserString("qss_town", idx);
                b->setStateSelected(i == st.chosenTown);
                b->eventMouseButtonClick += MyGUI::newDelegate(&OnTownChosen);
                contentW = std::max(contentW, b->getTextSize().width + 48);
                full.push_back(b);
                y += g_rowH + 2;
            }
            y += PAD - 2;
        }
        else
        {
            for (size_t i = 0; i < st.relations.size(); ++i)
            {
                MyGUI::TextBox* const name = box->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                    MyGUI::IntCoord(PAD, y, client.width / 2, g_rowH), MyGUI::Align::Default);
                name->setCaption(EscapeTags(st.relations[i].first + ":"));
                name->setTextColour(TextColour());
                name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
                MyGUI::TextBox* const value = box->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                    MyGUI::IntCoord(client.width / 2, y, client.width / 2 - PAD, g_rowH), MyGUI::Align::Default);
                char num[16];
                sprintf_s(num, "%+d", st.relations[i].second);
                value->setCaption(num);
                const int v = st.relations[i].second;
                value->setTextColour(v > 0 ? Ink(MyGUI::Colour(0.33f, 0.86f, 0.55f))
                                           : (v < 0 ? Ink(MyGUI::Colour(0.95f, 0.33f, 0.28f)) : TextColour()));
                value->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
                nameW = std::max(nameW, name->getTextSize().width);
                valueW = std::max(valueW, value->getTextSize().width);
                pairs.push_back(std::make_pair(static_cast<MyGUI::Widget*>(name), static_cast<MyGUI::Widget*>(value)));
                y += g_rowH;
            }
            contentW = std::max(contentW, nameW + 24 + valueW);
            y += GAP;
            MyGUI::Button* const ok = MakeButton(box, Tr("Close"), PAD, y, client.width - PAD * 2, g_rowH,
                                                 MyGUI::Align::Default);
            ok->eventMouseButtonClick += MyGUI::newDelegate(&OnPickClose);
            contentW = std::max(contentW, ok->getTextSize().width + 48);
            full.push_back(ok);
            y += g_rowH + PAD;
        }

        const int clientW = std::min(view.width - 80 - frameW, contentW + PAD * 2);
        const int clientH = std::min(view.height - 80 - frameH, y);
        const int w = clientW + frameW;
        const int h = clientH + frameH;
        box->setCoord((view.width - w) / 2, (view.height - h) / 2, w, h);
        const int innerW = clientW - PAD * 2;
        for (size_t i = 0; i < full.size(); ++i)
            full[i]->setSize(innerW, full[i]->getHeight());
        for (size_t i = 0; i < pairs.size(); ++i)
        {
            const int vw = std::max(valueW + 4, 40);
            pairs[i].first->setSize(innerW - vw - 8, pairs[i].first->getHeight());
            pairs[i].second->setCoord(PAD + innerW - vw, pairs[i].second->getTop(), vw, pairs[i].second->getHeight());
        }
        MyGUI::InputManager::getInstance().addWidgetModal(box);
    }

    void OnPickClicked(MyGUI::Widget* sender)
    {
        ShowPick(sender->getUserString("qss_pick") == "town");
    }

    // Выбранный город - в данные старта: в списке «town» остаётся только
    // он. Прежний список возвращается при следующем показе окна выбора
    // старта (RestoreTowns), чтобы следующая новая игра снова выбирала.
    std::vector<std::pair<GameData*, Ogre::vector<GameDataReference>::type> > g_savedTowns;

    void RestoreTowns()
    {
        for (size_t i = 0; i < g_savedTowns.size(); ++i)
            if (Ogre::vector<GameDataReference>::type* list = g_savedTowns[i].first->_getReferenceList_nonConst("town"))
                *list = g_savedTowns[i].second;
        g_savedTowns.clear();
    }

    void ApplyChosenTown(GameData* d, const Start& st)
    {
        if (d == NULL || st.chosenTown < 0 || st.chosenTown >= static_cast<int>(st.towns.size()))
            return;
        Ogre::vector<GameDataReference>::type* const list = d->_getReferenceList_nonConst("town");
        if (list == NULL)
            return;
        const std::string sid = st.towns[st.chosenTown]->stringID;
        g_savedTowns.push_back(std::make_pair(d, *list));
        for (size_t i = list->size(); i > 0; --i)
            if ((*list)[i - 1].sid != sid)
                list->erase(list->begin() + (i - 1));
        if (g_debug)
            DebugLog("QuickStartSelect: town chosen '" + st.towns[st.chosenTown]->name + "'");
    }


    void ShowDetails()
    {
        if (g_title == NULL)
            return;
        const Start* s = (g_selected >= 0 && g_selected < static_cast<int>(g_starts.size()))
                             ? &g_starts[g_selected] : NULL;
        g_title->setCaption(s ? EscapeTags(s->name) : std::string());
        const std::string values[3] = {
            s ? s->difficulty : std::string(),
            s ? Money(s->money) : std::string(),
            s ? s->style : std::string()
        };
        for (int i = 0; i < 3; ++i)
            if (g_infoVals[i] != NULL)
                g_infoVals[i]->setCaption(EscapeTags(values[i]));
        if (g_infoVals[0] != NULL && s != NULL)
            g_infoVals[0]->setTextColour(RankColour(s->rank));
        ShowTownAndRelations(s);

        if (g_desc != NULL && g_descView != NULL)
        {
            const int w = g_descView->getViewCoord().width;
            g_desc->setSize(w, 10);
            g_desc->setCaption(s ? EscapeTags(s->description) : std::string());
            const int h = std::max(g_desc->getTextSize().height + 8, g_descView->getViewCoord().height);
            g_desc->setSize(w, h);
            g_descView->setCanvasSize(w, h);
            g_descView->setViewOffset(MyGUI::IntPoint(0, 0));
        }
        if (g_btnBegin != NULL)
            g_btnBegin->setEnabled(s != NULL);

        for (size_t r = 0; r < g_rows.size(); ++r)
        {
            const bool on = g_rows[r].start == g_selected;
            g_rows[r].button->setStateSelected(on);
            g_rows[r].name->setTextColour(on ? GoldColour() : TextColour());
        }
    }


    void OnListWheel(MyGUI::Widget*, int rel)
    {
        if (g_list == NULL)
            return;
        MyGUI::IntPoint offset = g_list->getViewOffset();
        offset.top += rel > 0 ? g_rowH * 2 : -g_rowH * 2;
        g_list->setViewOffset(offset);
    }


    void OnDescWheel(MyGUI::Widget*, int rel)
    {
        if (g_descView == NULL)
            return;
        MyGUI::IntPoint offset = g_descView->getViewOffset();
        offset.top += rel > 0 ? g_rowH : -g_rowH;
        g_descView->setViewOffset(offset);
    }


    void RequestBegin();

    void OnRowClick(MyGUI::Widget* sender)
    {
        const int row = atoi(sender->getUserString("qss_row").c_str());
        if (row < 0 || row >= static_cast<int>(g_rows.size()))
            return;
        // Двойной щелчок своим счётом: у кнопки eventMouseButtonDoubleClick
        // приходит не всегда - первый щелчок уже меняет её вид.
        const DWORD now = GetTickCount();
        const bool twice = g_lastClickRow == row && now - g_lastClickMs < 400;
        g_lastClickRow = row;
        g_lastClickMs = now;
        g_selected = g_rows[row].start;
        ShowDetails();
        if (twice)
        {
            g_lastClickRow = -1;
            RequestBegin();
        }
    }


    void ScrollToSelected()
    {
        if (g_list == NULL)
            return;
        for (size_t r = 0; r < g_rows.size(); ++r)
        {
            if (g_rows[r].start != g_selected)
                continue;
            const int y = g_rows[r].button->getTop();
            const int viewH = g_list->getViewCoord().height;
            const int top = -g_list->getViewOffset().top;
            if (y < top || y + g_rowH > top + viewH)
                g_list->setViewOffset(MyGUI::IntPoint(0, -std::max(0, y - viewH / 2)));
            return;
        }
    }


    void BuildList()
    {
        if (g_list == NULL)
            return;
        MyGUI::Gui& mygui = MyGUI::Gui::getInstance();
        for (size_t r = 0; r < g_rows.size(); ++r)
            mygui.destroyWidget(g_rows[r].button);
        g_rows.clear();

        std::vector<int> order;
        for (size_t i = 0; i < g_starts.size(); ++i)
            if (Passes(g_starts[i]))
                order.push_back(static_cast<int>(i));
        std::sort(order.begin(), order.end(), ByOrder());

        // Выбранный старт отфильтрован - выбираем первый видимый.
        if (std::find(order.begin(), order.end(), g_selected) == order.end())
            g_selected = order.empty() ? -1 : order[0];

        // Ширина строки - по клиентской части списка, когда полоса
        // прокрутки уже на месте. Пока холст пустой, полосы нет, и
        // getViewCoord отдаёт ширину вместе с ней: строки уезжали под
        // полосу, и сложность обрезалась. Поэтому сначала холст нужной
        // высоты, потом замер.
        const int contentH = static_cast<int>(order.size()) * (g_rowH + 2) + 4;
        g_list->setCanvasSize(g_list->getViewCoord().width, contentH);
        const int viewW = g_list->getViewCoord().width;
        const int rowW = viewW - 6;
        int y = 2;
        for (size_t k = 0; k < order.size(); ++k)
        {
            const Start& s = g_starts[order[k]];
            Row row;
            row.start = order[k];
            row.button = g_list->createWidget<MyGUI::Button>("Kenshi_Button1",
                MyGUI::IntCoord(2, y, rowW, g_rowH), MyGUI::Align::Default);
            char idx[16];
            sprintf_s(idx, "%u", static_cast<unsigned>(k));
            row.button->setUserString("qss_row", idx);
            row.button->eventMouseButtonClick += MyGUI::newDelegate(&OnRowClick);
            row.button->eventMouseWheel += MyGUI::newDelegate(&OnListWheel);

            // Сложность справа: ширина по тексту, имя - всё остальное.
            row.difficulty = row.button->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(0, 0, rowW, g_rowH), MyGUI::Align::Default);
            row.difficulty->setNeedMouseFocus(false);
            row.difficulty->setCaption(EscapeTags(s.difficulty));
            row.difficulty->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
            row.difficulty->setTextColour(RankColour(s.rank));
            // Сложность - целиком, по правому краю; имя - сколько останется.
            const int diffW = std::min(rowW * 3 / 5, row.difficulty->getTextSize().width + 6);
            row.difficulty->setCoord(rowW - 8 - diffW, 0, diffW, g_rowH);

            // Имя обрезается за 14 пикселей до сложности - не налезает.
            row.name = row.button->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(8, 0, std::max(10, rowW - 8 - diffW - 14 - 8), g_rowH), MyGUI::Align::Default);
            row.name->setNeedMouseFocus(false);
            row.name->setCaption(EscapeTags(s.name));
            row.name->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            row.name->setTextColour(TextColour());

            g_rows.push_back(row);
            y += g_rowH + 2;
        }
        g_list->setCanvasSize(viewW, std::max(y + 2, g_list->getViewCoord().height));
        g_list->setViewOffset(MyGUI::IntPoint(0, 0));

        if (g_count != NULL)
        {
            char text[64];
            sprintf_s(text, "%u / %u", static_cast<unsigned>(order.size()),
                      static_cast<unsigned>(g_starts.size()));
            g_count->setCaption(text);
        }
        UpdateSortCaptions();
        ShowDetails();
        ScrollToSelected();
    }


    // --- кнопки ---

    void OnLayoutButton(MyGUI::Widget*)
    {
        g_compact = !g_compact;
        SaveViewSettings();
        g_needLayout = true;
    }

    void OnSortAz(MyGUI::Widget*)
    {
        if (g_sort == SORT_AZ)
            g_sortDesc = !g_sortDesc;
        else
        {
            g_sort = SORT_AZ;
            g_sortDesc = false;
        }
        SaveViewSettings();
        g_needList = true;
    }

    void OnSortDifficulty(MyGUI::Widget*)
    {
        if (g_sort == SORT_DIFFICULTY)
            g_sortDesc = !g_sortDesc;
        else
        {
            g_sort = SORT_DIFFICULTY;
            g_sortDesc = false;
        }
        SaveViewSettings();
        g_needList = true;
    }

    void OnRaceAccept(MyGUI::ComboBox* box, size_t index)
    {
        g_raceFilter = (index == MyGUI::ITEM_NONE || index == 0) ? -1 : static_cast<int>(index) - 1;
        (void)box;
        g_needList = true;
    }

    void OnSearchChange(MyGUI::EditBox* edit)
    {
        g_filterText = Lower(edit->getOnlyText());
        g_needList = true;
    }

    void OnClearSearch(MyGUI::Widget*)
    {
        if (g_search != NULL)
            g_search->setCaption("");
        g_filterText.clear();
        g_raceFilter = -1;
        if (g_raceBox != NULL)
            g_raceBox->setIndexSelected(0);
        g_needList = true;
    }

    // В компактной раскладке место одно: описание или настройки.
    void OnOptionsButton(MyGUI::Widget*)
    {
        g_showOptions = !g_showOptions;
        g_needLayout = true;
    }

    void OnBeginButton(MyGUI::Widget*)
    {
        RequestBegin();
    }

    void OnWindowButton(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
            g_needClose = true;
    }


    // --- подтверждение ---

    void OnConfirmYes(MyGUI::Widget*)
    {
        g_needConfirmClose = true;
        g_needBegin = true;
    }

    void OnConfirmNo(MyGUI::Widget*)
    {
        g_needConfirmClose = true;
    }

    void OnConfirmWindowButton(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
            g_needConfirmClose = true;
    }


    MyGUI::Button* MakeButton(MyGUI::Widget* parent, const std::string& caption, int x, int y, int w, int h,
                              MyGUI::Align align)
    {
        MyGUI::Button* const b = parent->createWidget<MyGUI::Button>("Kenshi_Button1",
            MyGUI::IntCoord(x, y, w, h), align);
        b->setCaption(caption);
        return b;
    }


    int TextWidth(MyGUI::Button* b)
    {
        return b->getTextSize().width;
    }


    void ShowConfirm()
    {
        if (g_confirmRef != NULL && g_confirmRef->get() != NULL)
            return;
        if (g_selected < 0 || g_selected >= static_cast<int>(g_starts.size()) || g_win == NULL)
            return;

        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        const int w = std::max(420, view.width / 4);
        const int h = g_rowH * 3 + 120;
        MyGUI::Window* const box = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("Kenshi_WindowCX",
            MyGUI::IntCoord((view.width - w) / 2, (view.height - h) / 2, w, h),
            MyGUI::Align::Default, g_win->getLayer() ? g_win->getLayer()->getName() : "Window",
            "QuickStartSelect_Confirm");
        box->setCaption(Tr("BEGIN"));
        box->eventWindowButtonPressed += MyGUI::newDelegate(&OnConfirmWindowButton);
        if (g_confirmRef == NULL)
            g_confirmRef = new WidgetRef();     // не удаляется: см. WidgetRef.h
        g_confirmRef->set(box);

        const MyGUI::IntCoord client = box->getClientCoord();
        MyGUI::EditBox* const text = box->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
            MyGUI::IntCoord(PAD, PAD, client.width - PAD * 2, client.height - g_rowH - PAD * 3),
            MyGUI::Align::Default);
        text->setEditMultiLine(true);
        text->setEditWordWrap(true);
        text->setEditStatic(true);
        text->setNeedMouseFocus(false);
        text->setTextAlign(MyGUI::Align::Center);
        text->setTextColour(TextColour());
        text->setCaption(EscapeTags(std::string(Tr("Begin a new game with this start?")) + "\n\n" +
                                    g_starts[g_selected].name));

        const int bw = (client.width - PAD * 3) / 2;
        const int by = client.height - g_rowH - PAD;
        MyGUI::Button* const yes = MakeButton(box, Tr("Begin"), PAD, by, bw, g_rowH);
        yes->eventMouseButtonClick += MyGUI::newDelegate(&OnConfirmYes);
        MyGUI::Button* const no = MakeButton(box, Tr("Cancel"), PAD * 2 + bw, by, bw, g_rowH);
        no->eventMouseButtonClick += MyGUI::newDelegate(&OnConfirmNo);

        MyGUI::InputManager::getInstance().addWidgetModal(box);
    }


    void RequestBegin()
    {
        if (g_selected < 0)
            return;
        if (g_confirm)
            ShowConfirm();
        else
            g_needBegin = true;
    }


    // ---------------------------------------------------------------
    // Дополнительные настройки - своей панелью в нашем окне
    //
    // Строки панели игры (newGameOptions->optionsPanel) привязаны к полям
    // GameplayOptions через valuePtr. Наша панель - такие же строки
    // DatapanelGUI игры (как в MCM), привязанные к тем же полям: что
    // выставлено у нас, то игра и прочтёт при старте. Окно игры спрятано.
    //
    // Поле строки - по смещению valuePtr от начала структуры. Начало ищем
    // по флажкам (DataPanelLine_CheckBox::valuePtr): смещения всех должны
    // лечь на bool-поля. Ползунок игры тип DPL_SLIDER не выставляет
    // (первая сборка его так и не нашла), поэтому указатель берём по
    // смещению 0x130 - оно у обоих ползунков - и принимаем, только если
    // он попал на float-поле. Пределы: у DataPanelLine_SliderEditable
    // 0x128/0x12C, у DataPanelLine_Slider 0x124/0x128; который из них -
    // сверяем таблицу виртуальных функций с нашей строкой-образцом.
    //
    // По умолчанию - GameplayOptions::reset() на своей копии.
    // ---------------------------------------------------------------

    struct FieldInfo
    {
        size_t off;
        bool isFloat;
        const char* label;              // msgid игры (titlescreen.cpp)
        const char* tip;                // её же подсказка, NULL - нет
    };

    const FieldInfo FIELDS[] = {
        { 0x1C, true, "Hunger time",
          "This sets how long it takes characters to get hungry.  A higher number means it takes longer, therefore they need less food to get by." },
        { 0x00, true, "Chance of death",
          "Determines the overall likelyhood of any character dying (players or NPCs).  Directly affects rates of wound degeneration and blood loss (Higher=more likely to die)" },
        { 0x08, true, "Global damage multiplier",
          "Overall combat damage for everything.  Doesn't necessarily make the game harder or easier, but affects the speed of battles" },
        { 0x18, true, "Production speed",
          "Affects speed of item production such as mines and crafting (more than 1.0 == easier)" },
        { 0x14, true, "Research speed", "Affects speed of research (more than 1.0 == easier)" },
        { 0x0C, true, "Building speed", "Affects speed of building (more than 1.0 == easier)" },
        { 0x10, true, "Number of nests multiplier",
          "Affects number of animal nests, bandit camps etc that appear in the wilds.  Higher number makes life more dangerous." },
        { 0x20, false, "Bandits loot the player", "Makes life unfair" },
        { 0x04, false, "Easy prospecting", NULL },
        { 0x21, false, "Animals eat", NULL },
        { 0x22, false, "Difficult healing", NULL },
    };

    const FieldInfo* FieldAt(size_t off)
    {
        for (size_t i = 0; i < sizeof(FIELDS) / sizeof(FIELDS[0]); ++i)
            if (FIELDS[i].off == off)
                return &FIELDS[i];
        return NULL;
    }

    bool IsFloatField(size_t off)
    {
        const FieldInfo* const f = FieldAt(off);
        return f != NULL && f->isFloat;
    }

    bool IsBoolField(size_t off)
    {
        const FieldInfo* const f = FieldAt(off);
        return f != NULL && !f->isFloat;
    }


    struct OptionLine
    {
        bool isSlider;
        void* ptr;
        size_t off;
        std::string label;              // подпись, как у игры
        float min, max;
        DataPanelLine* ours;            // наша строка
    };

    std::vector<OptionLine> g_options;
    DatapanelGUI* g_optPanel = NULL;
    const int OPT_CAT = 0;
    const int PROBE_CAT = 99;           // строка-образец, никогда не показывается
    float g_probeValue = 0.0f;

    union DefaultsBuffer
    {
        unsigned char bytes[64];
        double align;
    };
    DefaultsBuffer g_defaults;
    bool g_defaultsReady = false;


    size_t FieldOrder(size_t off)
    {
        for (size_t i = 0; i < sizeof(FIELDS) / sizeof(FIELDS[0]); ++i)
            if (FIELDS[i].off == off)
                return i;
        return 100;
    }

    struct ByFieldOrder
    {
        bool operator()(const OptionLine& a, const OptionLine& b) const
        {
            return FieldOrder(a.off) < FieldOrder(b.off);
        }
    };


    std::string LineLabel(DataPanelLine* l)
    {
        return !l->keyValue.empty() ? l->keyValue : l->s1;
    }


    bool SaneRange(float lo, float hi)
    {
        return lo == lo && hi == hi && lo < hi && lo > -1.0e6f && hi < 1.0e6f;
    }


    void CollectOptions(const void* vtableEditable)
    {
        g_options.clear();
        if (g_ngw == NULL || g_ngw->newGameOptions == NULL || g_ngw->newGameOptions->optionsPanel == NULL)
            return;
        DatapanelGUI* const panel = g_ngw->newGameOptions->optionsPanel;
        const int cat = panel->getCurrentCategory();
        const int n = panel->getNumLines(cat);
        std::vector<DataPanelLine*> lines;
        for (int i = 0; i < n; ++i)
            if (DataPanelLine* const l = panel->getLineByNum(cat, i))
                lines.push_back(l);

        // Начало структуры - по флажкам.
        const unsigned char* lowest = NULL;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (lines[i]->getType() != DataPanelLine::DPL_CHECK)
                continue;
            const unsigned char* const p =
                reinterpret_cast<const unsigned char*>(static_cast<DataPanelLine_CheckBox*>(lines[i])->valuePtr);
            if (p != NULL && (lowest == NULL || p < lowest))
                lowest = p;
        }
        const unsigned char* base = NULL;
        for (size_t back = 0; lowest != NULL && back <= 0x22 && base == NULL; ++back)
        {
            const unsigned char* const b = lowest - back;
            bool ok = true;
            for (size_t i = 0; i < lines.size() && ok; ++i)
            {
                if (lines[i]->getType() != DataPanelLine::DPL_CHECK)
                    continue;
                const unsigned char* const p =
                    reinterpret_cast<const unsigned char*>(static_cast<DataPanelLine_CheckBox*>(lines[i])->valuePtr);
                ok = p != NULL && p >= b && IsBoolField(static_cast<size_t>(p - b));
            }
            if (ok)
                base = b;
        }

        for (size_t i = 0; i < lines.size(); ++i)
        {
            DataPanelLine* const l = lines[i];
            const DataPanelLine::LineType type = l->getType();
            const unsigned char* const raw = reinterpret_cast<const unsigned char*>(l);
            OptionLine o;
            o.ours = NULL;
            o.min = o.max = 0.0f;
            if (type == DataPanelLine::DPL_CHECK)
            {
                o.isSlider = false;
                o.ptr = static_cast<DataPanelLine_CheckBox*>(l)->valuePtr;
            }
            else if (type == DataPanelLine::DPL_BUTTON || type == DataPanelLine::DPL_DROPBOX ||
                     type == DataPanelLine::DPL_TEXT || type == DataPanelLine::DPL_TEXT_EDIT ||
                     type == DataPanelLine::DPL_PROGRESS)
                continue;
            else
            {
                o.isSlider = true;
                o.ptr = *reinterpret_cast<void* const*>(raw + 0x130);
                const bool editable = vtableEditable != NULL &&
                                      *reinterpret_cast<void* const*>(l) == vtableEditable;
                o.min = *reinterpret_cast<const float*>(raw + (editable ? 0x128 : 0x124));
                o.max = *reinterpret_cast<const float*>(raw + (editable ? 0x12C : 0x128));
            }

            const unsigned char* const p = static_cast<const unsigned char*>(o.ptr);
            const bool placed = base != NULL && p != NULL && p >= base && p < base + 0x24;
            o.off = placed ? static_cast<size_t>(p - base) : 0;
            const bool fits = placed && (o.isSlider ? IsFloatField(o.off) : IsBoolField(o.off));

            if (g_debug)
            {
                char text[200];
                sprintf_s(text, " | type %d | ptr %s | offset 0x%X | range %g..%g | vtable %s",
                          static_cast<int>(type), fits ? "ok" : "skipped", static_cast<unsigned>(o.off),
                          o.min, o.max,
                          vtableEditable == NULL ? "?" :
                          (*reinterpret_cast<void* const*>(l) == vtableEditable ? "editable" : "other"));
                DebugLog("QuickStartSelect: game option '" + LineLabel(l) + "'" + text);
            }
            if (!fits)
                continue;

            if (o.isSlider && !SaneRange(o.min, o.max))
            {
                const float v = *static_cast<float*>(o.ptr);
                o.min = 0.0f;
                o.max = std::max(2.0f, v * 4.0f);
            }
            o.label = LineLabel(l);
            if (o.label.empty())
                o.label = Tr(FieldAt(o.off)->label);
            g_options.push_back(o);
        }
        if (g_debug && base == NULL)
            DebugLog("QuickStartSelect: game options layout not recognised");

        // Панель игры хранит строки по ключу (map), и getLineByNum отдаёт
        // их по алфавиту. На экране у игры - в порядке добавления, он же
        // порядок FIELDS. Наша панель ставит строки в порядке добавления.
        std::sort(g_options.begin(), g_options.end(), ByFieldOrder());
    }


    void PrepareDefaults()
    {
        if (g_defaultsReady)
            return;
        g_defaultsReady = true;
        memset(&g_defaults, 0, sizeof(g_defaults));
        reinterpret_cast<GameplayOptions*>(g_defaults.bytes)->reset();
    }


    float DefaultFloat(const OptionLine& o)
    {
        return *reinterpret_cast<const float*>(g_defaults.bytes + o.off);
    }

    bool DefaultBool(const OptionLine& o)
    {
        return g_defaults.bytes[o.off] != 0;
    }


    std::string FormatValue(float v)
    {
        char text[32];
        sprintf_s(text, "%.2f", v);
        std::string r(text);
        while (r.size() > 3 && r[r.size() - 1] == '0')      // 1.50 -> 1.5, 1.00 -> 1.0
            r.erase(r.size() - 1);
        return r;
    }


    void ShowOptionTip(int index)
    {
        if (g_tipText == NULL)
            return;
        if (index < 0 || index >= static_cast<int>(g_options.size()))
        {
            g_tipText->setCaption("");
            return;
        }
        const OptionLine& o = g_options[index];
        const FieldInfo* const f = FieldAt(o.off);
        std::string text = o.label;
        if (f != NULL && f->tip != NULL)
            text += std::string("\n") + Tr(f->tip);
        const std::string def = o.isSlider ? FormatValue(DefaultFloat(o))
                                           : std::string(DefaultBool(o) ? Tr("on") : Tr("off"));
        char line[256];
        sprintf_s(line, Tr("Default: %s. Right click resets it."), def.c_str());
        if (g_rightClickReset)
            text += std::string("\n") + line;
        g_tipText->setCaption(EscapeTags(text));
    }


    void ResetOption(int index)
    {
        if (index < 0 || index >= static_cast<int>(g_options.size()))
            return;
        const OptionLine& o = g_options[index];
        if (o.isSlider)
        {
            const float v = std::max(o.min, std::min(o.max, DefaultFloat(o)));
            *static_cast<float*>(o.ptr) = v;
            if (DataPanelLine_SliderEditable* const l = static_cast<DataPanelLine_SliderEditable*>(o.ours))
            {
                l->setValue(v);
                l->refresh();
            }
        }
        else
        {
            const bool v = DefaultBool(o);
            *static_cast<bool*>(o.ptr) = v;
            if (DataPanelLine_CheckBox* const l = static_cast<DataPanelLine_CheckBox*>(o.ours))
            {
                l->setValue(v);
                l->refresh();
            }
        }
        if (g_debug)
            DebugLog("QuickStartSelect: reset option '" + o.label + "'");
    }


    int OptionIndex(MyGUI::Widget* w)
    {
        const std::string key = w->getUserString("qss_opt");
        return key.empty() ? -1 : atoi(key.c_str());
    }

    void OnOptionPressed(MyGUI::Widget* sender, int, int, MyGUI::MouseButton id)
    {
        if (id == MyGUI::MouseButton::Right && g_rightClickReset)
            ResetOption(OptionIndex(sender));
    }

    void OnOptionFocus(MyGUI::Widget* sender, MyGUI::Widget*)
    {
        ShowOptionTip(OptionIndex(sender));
    }


    void HookWidget(MyGUI::Widget* w, int index)
    {
        if (w == NULL || !w->getUserString("qss_opt").empty())
            return;
        char key[16];
        sprintf_s(key, "%d", index);
        w->setUserString("qss_opt", key);
        w->eventMouseButtonPressed += MyGUI::newDelegate(&OnOptionPressed);
        w->eventMouseSetFocus += MyGUI::newDelegate(&OnOptionFocus);
    }


    // Подписка - на подпись, поле числа, ползунок, флажок и всё, что
    // строка создала. Виджеты строка создаёт при показе, поэтому смотрим
    // каждый кадр; повторно не подписываемся (метка в UserString).
    void HookOurLines()
    {
        for (size_t i = 0; i < g_options.size(); ++i)
        {
            DataPanelLine* const l = g_options[i].ours;
            if (l == NULL)
                continue;
            const int index = static_cast<int>(i);
            HookWidget(l->w1, index);
            HookWidget(l->w2, index);
            for (size_t k = 0; k < l->widgets.size(); ++k)
                HookWidget(l->widgets[k], index);
            if (g_options[i].isSlider)
            {
                DataPanelLine_SliderEditable* const sl = static_cast<DataPanelLine_SliderEditable*>(l);
                HookWidget(sl->nameText, index);
                HookWidget(sl->valueEditBox, index);
                HookWidget(sl->sliderBar, index);
            }
            else
            {
                DataPanelLine_CheckBox* const c = static_cast<DataPanelLine_CheckBox*>(l);
                HookWidget(c->text, index);
                HookWidget(c->button, index);
            }
        }
    }


    void DestroyOptionsPanel()
    {
        for (size_t i = 0; i < g_options.size(); ++i)
            g_options[i].ours = NULL;
        if (g_optPanel != NULL && gui != NULL)
        {
            g_optPanel->show(false);
            gui->destroy(g_optPanel);
        }
        g_optPanel = NULL;
    }


    void BuildOptionsPanel(MyGUI::Widget* host)
    {
        DestroyOptionsPanel();
        if (host == NULL || gui == NULL)
            return;
        g_optPanel = gui->createDatapanel("QuickStartSelectOptions", host, true);
        if (g_optPanel == NULL)
        {
            ErrorLog("QuickStartSelect: createDatapanel failed");
            return;
        }
        g_optPanel->setLineSpacing(OPT_LINES_PER_SCREEN);

        DataPanelLine_SliderEditable* const probe =
            g_optPanel->setLineSliderEditable("qss_probe", PROBE_CAT, true, 0.0f, 1.0f, &g_probeValue);
        CollectOptions(probe != NULL ? *reinterpret_cast<void* const*>(probe) : NULL);
        PrepareDefaults();

        for (size_t i = 0; i < g_options.size(); ++i)
        {
            OptionLine& o = g_options[i];
            if (o.isSlider)
            {
                DataPanelLine_SliderEditable* const l = g_optPanel->setLineSliderEditable(
                    o.label, OPT_CAT, true, o.min, o.max, static_cast<float*>(o.ptr));
                if (l != NULL)
                    l->setPrecision(2);
                o.ours = l;
            }
            else
                o.ours = g_optPanel->setLineCheckbox(o.label, static_cast<bool*>(o.ptr), OPT_CAT);
        }
        if (g_options.empty())
            g_optPanel->setLine(Tr("The game's options were not found."), "", OPT_CAT, false, true);
        g_optPanel->changeCategory(OPT_CAT);
        HookOurLines();
    }


    // --- построение окна ---

    void BuildWindow()
    {
        DestroyWindow();
        if (g_ngw == NULL)
            return;

        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        g_viewSize = view;
        OPT_LINE_H = static_cast<int>(view.height / OPT_LINES_PER_SCREEN) + 1;

        // Всё - от размера экрана, чтобы на 1080p и на 4K окно занимало
        // одну и ту же долю. Высота строки - от шрифта.
        // Широкая - от кнопок главного меню (до ~21% ширины) почти до
        // правого края; настройки игры теперь внутри, места им хватает.
        const int winH = std::max(480, view.height * g_windowHeight / 100);
        const int winW = std::min(view.width, g_compact ? std::max(460, view.width * g_compactWidth / 100)
                                                        : std::max(900, view.width * g_wideWidth / 100));
        const int x = std::max(0, view.width - winW - view.width * 2 / 100);
        const int y = std::max(0, (view.height - winH) / 2);

        MyGUI::Widget* const vanilla = VanillaMain(g_ngw);
        std::string layer = "Window";
        if (vanilla != NULL && vanilla->getLayer() != NULL)
            layer = vanilla->getLayer()->getName();

        g_win = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("Kenshi_WindowCX",
            MyGUI::IntCoord(x, y, winW, winH), MyGUI::Align::Default, layer, "QuickStartSelect_Window");
        if (g_win == NULL)
        {
            ErrorLog("QuickStartSelect: could not create the window");
            return;
        }
        if (g_winRef == NULL)
            g_winRef = new WidgetRef();
        g_winRef->set(g_win);
        g_win->setCaption(Tr("Choose your beginning"));
        g_win->eventWindowButtonPressed += MyGUI::newDelegate(&OnWindowButton);

        MyGUI::Widget* const client = g_win;
        const MyGUI::IntCoord cc = g_win->getClientCoord();
        const int cw = cc.width;
        const int ch = cc.height;

        // Высота строки - по шрифту кнопки.
        {
            MyGUI::Button* probe = MakeButton(client, "Ag", 0, 0, 50, 20);
            g_rowH = std::max(26, probe->getFontHeight() + 12);
            MyGUI::Gui::getInstance().destroyWidget(probe);
        }
        const int rh = g_rowH;

        // --- панель: M, A-Z, сложность, раса; ниже поиск ---
        int ty = PAD;
        int tx = PAD;
        // Подпись - во что кнопка переключит.
        g_btnLayout = MakeButton(client, g_compact ? Tr("Wide view") : Tr("Compact view"), tx, ty, 10, rh);
        const int layoutW = TextWidth(g_btnLayout) + 24;
        g_btnLayout->setSize(layoutW, rh);
        g_btnLayout->eventMouseButtonClick += MyGUI::newDelegate(&OnLayoutButton);
        tx += layoutW + GAP;

        g_btnAz = MakeButton(client, "Z-A", tx, ty, 10, rh);
        const int azW = TextWidth(g_btnAz) + 24;
        g_btnAz->setSize(azW, rh);
        g_btnAz->eventMouseButtonClick += MyGUI::newDelegate(&OnSortAz);
        tx += azW + GAP;

        g_btnDiff = MakeButton(client, Tr("HARD-EASY"), tx, ty, 10, rh);
        int diffW = TextWidth(g_btnDiff);
        g_btnDiff->setCaption(Tr("EASY-HARD"));
        diffW = std::max(diffW, TextWidth(g_btnDiff)) + 24;
        g_btnDiff->setSize(diffW, rh);
        g_btnDiff->eventMouseButtonClick += MyGUI::newDelegate(&OnSortDifficulty);
        tx += diffW + GAP;

        g_raceBox = client->createWidget<MyGUI::ComboBox>("Kenshi_ComboBox",
            MyGUI::IntCoord(tx, ty, std::max(120, cw - tx - PAD), rh), MyGUI::Align::Default);
        g_raceBox->setComboModeDrop(true);
        g_raceBox->setSmoothShow(false);
        {
            char text[32];
            sprintf_s(text, " (%u)", static_cast<unsigned>(g_starts.size()));
            g_raceBox->addItem(std::string(Tr("All races")) + text);
            for (size_t r = 0; r < g_raceNames.size(); ++r)
            {
                sprintf_s(text, " (%d)", g_raceCount[g_raceNames[r]]);
                g_raceBox->addItem(EscapeTags(g_raceNames[r]) + text);
            }
        }
        g_raceBox->setIndexSelected(g_raceFilter < 0 ? 0 : static_cast<size_t>(g_raceFilter) + 1);
        g_raceBox->eventComboAccept += MyGUI::newDelegate(&OnRaceAccept);

        ty += rh + GAP;
        MyGUI::TextBox* const searchLabel = client->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
            MyGUI::IntCoord(PAD + 4, ty, 10, rh), MyGUI::Align::Default);
        searchLabel->setCaption(Tr("Search:"));
        searchLabel->setTextColour(LabelColour());
        searchLabel->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        const int labelW = searchLabel->getTextSize().width + 12;
        searchLabel->setSize(labelW, rh);

        g_count = client->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
            MyGUI::IntCoord(0, ty, 10, rh), MyGUI::Align::Default);
        g_count->setCaption("000 / 000");
        g_count->setTextColour(LabelColour());
        g_count->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
        const int countW = g_count->getTextSize().width + 8;

        const int clearW = rh + 6;
        const int searchX = PAD + 4 + labelW;
        const int searchW = std::max(60, cw - searchX - GAP - clearW - GAP - countW - PAD);
        g_search = client->createWidget<MyGUI::EditBox>("Kenshi_EditBox",
            MyGUI::IntCoord(searchX, ty, searchW, rh), MyGUI::Align::Default);
        g_search->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        g_search->eventEditTextChange += MyGUI::newDelegate(&OnSearchChange);
        g_btnClear = MakeButton(client, "X", searchX + searchW + GAP, ty, clearW, rh);
        g_btnClear->eventMouseButtonClick += MyGUI::newDelegate(&OnClearSearch);
        g_count->setCoord(cw - PAD - countW, ty, countW, rh);

        ty += rh + PAD;

        // --- список и описание ---
        const int bottomH = rh + PAD * 2;              // кнопки внизу
        const int areaTop = ty;
        const int areaH = ch - areaTop - bottomH;
        MyGUI::IntCoord listCoord, detailCoord;
        if (g_compact)
        {
            const int listH = areaH * 50 / 100;
            listCoord = MyGUI::IntCoord(PAD, areaTop, cw - PAD * 2, listH);
            detailCoord = MyGUI::IntCoord(PAD, areaTop + listH + PAD, cw - PAD * 2, areaH - listH - PAD);
        }
        else
        {
            const int listW = cw * 48 / 100;
            listCoord = MyGUI::IntCoord(PAD, areaTop, listW, areaH);
            detailCoord = MyGUI::IntCoord(PAD * 2 + listW, areaTop, cw - listW - PAD * 3, areaH);
        }

        g_list = client->createWidget<MyGUI::ScrollView>("Kenshi_ScrollView", listCoord, MyGUI::Align::Default);
        g_list->setVisibleHScroll(false);
        g_list->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        g_list->eventMouseWheel += MyGUI::newDelegate(&OnListWheel);

        MyGUI::Widget* const detail = client->createWidget<MyGUI::Widget>("PanelEmpty", detailCoord,
                                                                          MyGUI::Align::Default);
        const int dw = detailCoord.width;
        int dy = 0;
        // Название - крупным шрифтом заголовков, как имя мода в MCM.
        // Отступ INFO_X - вровень с текстом в рамке описания ниже.
        const int INFO_X = 8;
        g_title = detail->createWidget<MyGUI::EditBox>("Kenshi_TextboxPaintedText",
            MyGUI::IntCoord(INFO_X - 2, dy, dw - INFO_X, rh + 14), MyGUI::Align::Default);
        g_title->setEditStatic(true);
        g_title->setNeedMouseFocus(false);
        g_title->setFontName("Kenshi_PaintedTextFont_Large");
        g_title->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        g_title->setTextColour(GoldColour());
        dy += rh + 14 + GAP;

        const char* const keys[3] = { "Difficulty:", "Cash:", "Play Style:" };
        int keyW = 0;
        for (int i = 0; i < 3; ++i)
        {
            g_infoKeys[i] = detail->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(INFO_X, dy + i * rh, dw / 2, rh), MyGUI::Align::Default);
            g_infoKeys[i]->setCaption(Tr(keys[i]));
            g_infoKeys[i]->setTextColour(LabelColour());
            g_infoKeys[i]->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            keyW = std::max(keyW, g_infoKeys[i]->getTextSize().width);
        }
        keyW += 12;
        // Вторая колонка - с середины ширины.
        const char* const keys2[3] = { "Town:", "Characters:", "Relations:" };
        const int col2 = dw / 2;
        int keyW2 = 0;
        for (int i = 0; i < 3; ++i)
        {
            g_infoKeys2[i] = detail->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(col2, dy + i * rh, dw / 4, rh), MyGUI::Align::Default);
            g_infoKeys2[i]->setCaption(Tr(keys2[i]));
            g_infoKeys2[i]->setTextColour(LabelColour());
            g_infoKeys2[i]->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            keyW2 = std::max(keyW2, g_infoKeys2[i]->getTextSize().width);
        }
        keyW2 += 12;
        for (int i = 0; i < 3; ++i)
        {
            g_infoKeys2[i]->setSize(keyW2, rh);
            g_infoVals2[i] = detail->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(col2 + keyW2, dy + i * rh, std::max(10, dw - col2 - keyW2 - INFO_X), rh),
                MyGUI::Align::Default);
            g_infoVals2[i]->setTextColour(TextColour());
            g_infoVals2[i]->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            if (i != 1)
            {
                // Подчёркивание - как у ссылки: значит, по значению можно
                // щёлкнуть. Видно, только когда выбор есть (ShowTownAndRelations).
                g_underline[i] = detail->createWidget<MyGUI::Widget>("WhiteSkin",
                    MyGUI::IntCoord(col2 + keyW2, dy + i * rh + rh - 4, 10, 1), MyGUI::Align::Default);
                g_underline[i]->setNeedMouseFocus(false);
                g_underline[i]->setVisible(false);
                g_infoVals2[i]->setNeedMouseFocus(true);
                g_infoVals2[i]->setUserString("qss_pick", i == 0 ? "town" : "relations");
                g_infoVals2[i]->eventMouseButtonClick += MyGUI::newDelegate(&OnPickClicked);
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            g_infoKeys[i]->setSize(keyW, rh);
            g_infoVals[i] = detail->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(INFO_X + keyW, dy + i * rh, std::max(10, col2 - keyW - INFO_X - 6), rh),
                MyGUI::Align::Default);
            g_infoVals[i]->setTextColour(TextColour());
            g_infoVals[i]->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
        }
        dy += rh * 3 + GAP;

        // Описание и настройки. Широкая: описание, ниже настройки игры и
        // подсказка к ним. Компактная: одно из двух, кнопка внизу.
        const bool showDesc = !g_compact || !g_showOptions;
        const bool showOpts = !g_compact || g_showOptions;
        if (!showDesc)
            dy = 0;                     // в компактной настройки - на всю высоту
        for (int i = 0; i < 3 && !showDesc; ++i)
        {
            g_infoKeys[i]->setVisible(false);
            g_infoVals[i]->setVisible(false);
            g_infoKeys2[i]->setVisible(false);
            g_infoVals2[i]->setVisible(false);
        }
        if (!showDesc)
            g_title->setVisible(false);

        // Широкая: панель настроек не шире, чем у окна игры (~треть
        // экрана) - на широкой панели у ползунков пропадала полоса; справа
        // от неё - подсказка. Компактная: подсказка под панелью.
        const int restH = detailCoord.height - dy;
        const bool tipBeside = !g_compact;
        const int optW = tipBeside ? std::min(dw * 60 / 100, view.width * 34 / 100) : dw;
        const int tipH = tipBeside ? 0 : rh * 3 + 12;
        const int headH = rh;
        int optH = 0, descH = restH;
        if (showOpts)
        {
            const int want = 9 * OPT_LINE_H + 16;   // у игры 9 строк
            optH = showDesc ? std::min(want, restH - tipH - headH - GAP * 3 - rh * 3)
                            : restH - tipH - headH - GAP * 2;
            optH = std::max(rh * 3, optH);
            descH = showDesc ? restH - optH - tipH - headH - GAP * 2 - (tipBeside ? 0 : GAP) : 0;
        }

        if (showDesc)
        {
            g_descView = detail->createWidget<MyGUI::ScrollView>("Kenshi_ScrollView",
                MyGUI::IntCoord(0, dy, dw, std::max(rh * 2, descH)), MyGUI::Align::Default);
            g_descView->setVisibleHScroll(false);
            g_descView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            g_descView->eventMouseWheel += MyGUI::newDelegate(&OnDescWheel);
            g_desc = g_descView->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
                MyGUI::IntCoord(0, 0, g_descView->getViewCoord().width, 10), MyGUI::Align::Default);
            g_desc->setEditMultiLine(true);
            g_desc->setEditWordWrap(true);
            g_desc->setEditStatic(true);
            g_desc->setNeedMouseFocus(false);
            g_desc->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            // Цвет текста не задаём: рамка Kenshi_ScrollView и шаблон
            // Kenshi_TextboxStandardText идут парой, и интерфейс-мод красит
            // их согласованно (ваниль - тёмный на пергаменте, Dark UI -
            // #9c9c9c). 08.10.2026: вместо тёмной подложки под светлым
            // текстом - так посоветовал Асур.
            dy += std::max(rh * 2, descH) + GAP;
        }

        if (showOpts)
        {
            MyGUI::TextBox* const head = detail->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(4, dy, dw - 4, headH), MyGUI::Align::Default);
            head->setCaption(Tr("Advanced options"));
            head->setTextColour(GoldColour());
            head->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            dy += headH + GAP;

            MyGUI::Widget* const host = detail->createWidget<MyGUI::Widget>("PanelEmpty",
                MyGUI::IntCoord(0, dy, optW, optH), MyGUI::Align::Default);

            // Подсказка - как панель подсказок в окне настроек игры.
            const MyGUI::IntCoord tipCoord = tipBeside
                ? MyGUI::IntCoord(optW + GAP, dy, dw - optW - GAP, optH)
                : MyGUI::IntCoord(0, dy + optH + GAP, dw, tipH);
            MyGUI::ScrollView* const tipFrame = detail->createWidget<MyGUI::ScrollView>("Kenshi_ScrollView",
                tipCoord, MyGUI::Align::Default);
            tipFrame->setVisibleHScroll(false);
            tipFrame->setVisibleVScroll(false);
            const MyGUI::IntCoord tv = tipFrame->getViewCoord();
            tipFrame->setCanvasSize(tv.width, tv.height);
            g_tipText = tipFrame->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
                MyGUI::IntCoord(4, 2, tv.width - 8, tv.height - 4), MyGUI::Align::Default);
            g_tipText->setEditMultiLine(true);
            g_tipText->setEditWordWrap(true);
            g_tipText->setEditStatic(true);
            g_tipText->setNeedMouseFocus(false);
            g_tipText->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
            // Цвет - из шаблона интерфейса, как у описания.

            BuildOptionsPanel(host);
        }

        // --- кнопки внизу ---
        const int by = ch - rh - PAD;
        if (g_compact)
        {
            g_btnOptions = MakeButton(client, g_showOptions ? Tr("Description") : Tr("Advanced options"),
                                      PAD, by, 10, rh);
            g_btnOptions->setSize(TextWidth(g_btnOptions) + 32, rh);
            g_btnOptions->eventMouseButtonClick += MyGUI::newDelegate(&OnOptionsButton);
        }

        g_btnBegin = MakeButton(client, Tr("Begin"), 0, by, 10, rh);
        const int beginW = std::max(cw / 4, TextWidth(g_btnBegin) + 48);
        g_btnBegin->setCoord(cw - PAD - beginW, by, beginW, rh);
        g_btnBegin->eventMouseButtonClick += MyGUI::newDelegate(&OnBeginButton);

        BuildList();
        MyGUI::InputManager::getInstance().setKeyFocusWidget(g_search);
    }


    // ---------------------------------------------------------------
    // Открытие, закрытие, кадр
    // ---------------------------------------------------------------

    void HideVanilla()
    {
        MyGUI::Widget* const main = VanillaMain(g_ngw);
        if (main == NULL)
            return;
        if (!g_vanillaMoved)
        {
            g_vanillaPos = main->getPosition();
            g_vanillaMoved = true;
        }
        if (main->getLeft() != -20000)
            main->setPosition(-20000, -20000);
    }


    void RestoreVanilla()
    {
        MyGUI::Widget* const main = VanillaMain(g_ngw);
        if (main != NULL && g_vanillaMoved)
            main->setPosition(g_vanillaPos);
        g_vanillaMoved = false;
    }


    bool g_frameHooked = false;
    void OnFrame(float);

    void Open(NewGameWindow* w)
    {
        if (!g_enabled || w == NULL)
            return;
        g_ngw = w;
        LoadStarts(w);
        if (g_starts.empty())
        {
            g_openFailed = true;
            DebugLog("QuickStartSelect: the game has no starts, keeping its own window");
            return;
        }
        // Старт, который сейчас у игры, - выбранным.
        g_selected = -1;
        for (size_t i = 0; i < g_starts.size(); ++i)
            if (g_starts[i].index == w->currentStart)
                g_selected = static_cast<int>(i);
        g_needLayout = g_needList = g_needBegin = g_needClose = g_needConfirmClose = false;
        g_open = true;
        if (!g_frameHooked)
        {
            // Подписка навсегда: без открытого окна Tick сразу выходит.
            MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(&OnFrame);
            g_frameHooked = true;
        }
        HideVanilla();
        BuildWindow();
        if (g_win == NULL)
        {
            g_open = false;
            g_openFailed = true;
            RestoreVanilla();
            return;
        }
        if (g_debug)
            DebugLog("QuickStartSelect: opened");
    }


    void Close()
    {
        if (!g_open)
            return;
        g_open = false;
        DestroyWindow();
        RestoreVanilla();
        if (g_debug)
            DebugLog("QuickStartSelect: closed");
    }


    void Begin()
    {
        if (g_ngw == NULL || g_selected < 0 || g_selected >= static_cast<int>(g_starts.size()))
            return;
        const int index = g_starts[g_selected].index;
        if (index < 0 || index >= static_cast<int>(g_ngw->startsData.size()))
            return;
        if (g_debug)
            DebugLog("QuickStartSelect: begin '" + g_starts[g_selected].name + "'");
        ApplyChosenTown(g_ngw->startsData.begin()[index], g_starts[g_selected]);
        g_ngw->currentStart = index;
        g_ngw->updateCurrentData();
        NewGameWindow* const w = g_ngw;
        Close();
        w->newGameStart(NULL);
    }


    void Tick()
    {
        if (!g_open || g_ngw == NULL)
            return;

        // GUI пересоздан игрой - наше окно снесено вместе с ним.
        if (g_winRef == NULL || g_winRef->get() == NULL)
        {
            // Панель настроек стояла на снесённых виджетах: удалять её
            // уже нельзя - только забыть (лучше утечка, чем падение).
            for (size_t i = 0; i < g_options.size(); ++i)
                g_options[i].ours = NULL;
            g_optPanel = NULL;
            ForgetWidgets();
            g_open = false;
            g_vanillaMoved = false;
            return;
        }
        if (!g_ngw->getVisible())
        {
            Close();
            return;
        }

        if (g_needConfirmClose)
        {
            g_needConfirmClose = false;
            DestroyConfirm();
        }
        if (g_needPickClose)
        {
            g_needPickClose = false;
            DestroyPick();
        }
        if (g_needBegin)
        {
            g_needBegin = false;
            Begin();
            return;
        }
        if (g_needClose)
        {
            g_needClose = false;
            NewGameWindow* const w = g_ngw;
            Close();
            if (w->newGameOptions != NULL)
                w->newGameOptions->setVisible(false);
            w->close(NULL);
            return;
        }

        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        if (view.width != g_viewSize.width || view.height != g_viewSize.height)
            g_needLayout = true;
        if (g_needLayout)
        {
            g_needLayout = false;
            g_needList = false;
            const std::wstring keepFilter = g_filterText;
            const std::string keepText = g_search ? g_search->getOnlyText() : std::string();
            BuildWindow();
            if (g_search != NULL && !keepText.empty())
            {
                g_search->setOnlyText(keepText);
                g_filterText = keepFilter;
                BuildList();
            }
        }
        if (g_needList)
        {
            g_needList = false;
            BuildList();
        }

        HideVanilla();
        // Окно настроек игры не нужно: те же поля - в нашей панели.
        if (g_ngw->newGameOptions != NULL && g_ngw->newGameOptions->getVisible())
            g_ngw->newGameOptions->setVisible(false);
        HookOurLines();
    }


    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void (*g_origShow)(NewGameWindow*, bool) = NULL;
    void (*g_origUpdate)(NewGameWindow*) = NULL;


    void Show_hook(NewGameWindow* self, bool on)
    {
        g_origShow(self, on);
        if (on)
            RestoreTowns();             // город прошлой игры - назад, в список старта
        if (!on)
            g_openFailed = false;
        if (on && !g_open && !g_openFailed)
            Open(self);
        else if (!on && g_open)
            Close();
    }


    // После своего кадра окно игры могло вернуть себя и окно настроек на
    // место - отодвигаем снова.
    void Update_hook(NewGameWindow* self)
    {
        g_origUpdate(self);
        if (!g_open && !g_openFailed && g_enabled && self->getVisible())
            Open(self);
        if (g_open && self == g_ngw)
            HideVanilla();
    }


    void OnFrame(float)
    {
        Tick();
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, SECTION, "Quick Start Select", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("The \"Choose your beginning\" window as a list: search, sorting, race filter, advanced options beside it."));

    api->section(api, Tr("Window"));
    api->toggle(api, SECTION, "Enabled", Tr("Enabled"),
                Tr("Replace the game's start selection window with the list."), 1, MCM_RESTART);
    // Вид окна и сортировка - кнопками в самом окне (они же пишут ini);
    // на странице MCM они лишние (07.10.2026, просьба пользователя).
    api->toggle(api, SECTION, "ConfirmBegin", Tr("Confirm before starting"),
                Tr("Ask before starting the game, so a stray double click does not start it."), 1, 0);
    api->toggle(api, SECTION, "RightClickReset", Tr("Right click resets an option"),
                Tr("Right click on a line of the advanced options returns its default value."), 1, 0);

    api->section(api, Tr("Window size"));
    api->integer(api, SECTION, "WideWidth", Tr("Wide view width, %"),
                 Tr("Width of the wide window, in percent of the screen width."), 74, 50, 100, 0);
    api->integer(api, SECTION, "CompactWidth", Tr("Compact view width, %"),
                 Tr("Width of the compact window, in percent of the screen width."), 32, 25, 70, 0);
    api->integer(api, SECTION, "WindowHeight", Tr("Window height, %"),
                 Tr("Height of the window, in percent of the screen height."), 86, 50, 100, 0);

    api->section(api, Tr("Custom difficulties"));
    static const char* const tierKeys[6] = { "DifficultyVeryEasy", "DifficultyEasy", "DifficultyNormal",
                                             "DifficultyAboveNormal", "DifficultyHard", "DifficultyVeryHard" };
    static const char* const tierLabels[6] = { "Very easy (green)", "Easy (light green)", "Normal (yellow)",
                                               "Above normal (orange)", "Hard (red-orange)", "Very hard (red)" };
    for (int i = 0; i < 6; ++i)
        api->text(api, SECTION, tierKeys[i], Tr(tierLabels[i]),
                  Tr("Difficulties of starts from other mods to show in this colour and sort as this level. "
                     "Several - separated by commas; part of the word is enough."), "", 0);

    api->section(api, Tr("Diagnostics"));
    api->toggle(api, SECTION, "Debug", Tr("Detailed log"),
                Tr("Details in RE_Kenshi_log.txt: every start with its difficulty and races, the advanced options and their defaults."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("QuickStartSelect: disabled in the ini");
        return;
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&NewGameWindow::_NV_show),
                                                 Show_hook, &g_origShow))
    {
        ErrorLog("QuickStartSelect: could not hook NewGameWindow::show, the plugin stays off");
        return;
    }
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(KenshiLib::GetRealAddress(&NewGameWindow::_NV_update),
                                                 Update_hook, &g_origUpdate))
        ErrorLog("QuickStartSelect: could not hook NewGameWindow::update");

    DebugLog("QuickStartSelect: installed");
}
