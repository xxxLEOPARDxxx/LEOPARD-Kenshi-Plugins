// ItemMarkers - пометки на иконках вещей: цвет сорта, краденое, форма
// фракции, глаз на изученных чертежах (перенесён из DarkUiTweaks). Один плагин вместо LtEast's Rarity Backgrounds + Item Status
// Indicators + Mod Settings Core (09.10.2026).
//
// ЗАЧЕМ СВОЙ. У LtEast каждый из двух плагинов перехватывает
// InventoryGUI::update - его игра зовёт КАЖДЫЙ КАДР для каждого открытого
// окна инвентаря - и оттуда обходит иконки («all-inventory refresh»,
// «pending icons»). В торговле открыто несколько окон с сотнями вещей:
// дикая просадка FPS, а при продаже, когда игра пересоздаёт иконки, -
// статтеры. Плюс отдельное ядро настроек со своими хуками.
//
// КАК СДЕЛАНО ЗДЕСЬ. Пометки ставятся ОДИН РАЗ, когда игра создаёт иконку
// (InventoryIcon::_CONSTRUCTOR), - детьми корня иконки: уходят вместе с
// ней, ничего не утекает, в кадре работы нет. Подложка - позади картинки
// вещи (глубина 1), значки - поверх. Каждый кадр только одно: зажата ли
// клавиша показа (если задана), и то лишь при смене - видимость пометок.
//
// СОРТ - по уровню вещи 0..100 (Gear::level_0_100): до 15 - 0, до 35 - 1,
// до 50 - 2, до 65 - 3, до 80 - 4, выше - 5. Пороги - те же, что у LtEast
// (сняты с его DLL 09.10.2026). Первая версия узнавала сорт из подсказки
// вещи - пользователь: «значки должны показываться без наведения мыши».
// Цвета - ItemGrade0..5 из kenshi_colours.xml с учётом модов на интерфейс
// (GameTheme.h); ItemGrade5 в ванили нет - как у игры, цвет сорта 4.

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "item_markers"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_WidgetManager.h>
#include <mygui/MyGUI_IUnlinkWidget.h>
#include <OgreResourceGroupManager.h>

#include <kenshi/Globals.h>
#include <kenshi/GameData.h>
#include <kenshi/Item.h>
#include <kenshi/Gear.h>
#include <kenshi/Enums.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Research.h>

#include <GameTheme.h>
#include <HoldKey.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    const int GRADES = 6;              // ItemGrade0..5
    // Глубина среди детей иконки: больше - ниже. Картинка вещи стоит глубже
    // 1 - подложка с глубиной 1 легла поверх неё (09.10.2026); значения - как
    // у LtEast.
    const int BACK_DEPTH = 10000;
    const int FRONT_DEPTH = -10000;

    bool g_enabled = true;
    bool g_rarity = true;
    int g_style = 1;                   // 0 заливка, 1 рамка, 2 градиент, 3 звезда
    float g_rarityAlpha = 0.6f;
    int g_starCorner = 2;              // 0 лев. верх, 1 прав. верх, 2 лев. низ, 3 прав. низ
    int g_minGrade = 2;                // красить с этого сорта (0, 1 - серый и бежевый, обычные вещи)
    bool g_stolen = true;
    int g_stolenSize = 16;
    float g_stolenAlpha = 0.85f;
    bool g_faction = true;
    bool g_blueprint = true;           // глаз на изученных чертежах (перенесён из DarkUiTweaks)
    int g_factionSize = 16;
    float g_factionAlpha = 0.85f;
    HoldKey::Key g_holdKey;            // назначена - видно, пока зажата; NONE (vk 0) - всегда
    bool g_debug = false;

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

    std::string IniPath() { return ModulePath(".ini"); }

    float ReadFloat(const std::string& ini, const char* key, float def, float lo, float hi)
    {
        char text[32] = {};
        char fallback[32];
        sprintf_s(fallback, "%.2f", def);
        GetPrivateProfileStringA("ItemMarkers", key, fallback, text, sizeof(text), ini.c_str());
        float v = static_cast<float>(atof(text));
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        return v;
    }

    int ReadInt(const std::string& ini, const char* key, int def, int lo, int hi)
    {
        int v = static_cast<int>(GetPrivateProfileIntA("ItemMarkers", key, def, ini.c_str()));
        if (v < lo) v = lo;
        if (v > hi) v = hi;
        return v;
    }

    void RefreshAll();

    void __cdecl LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;
        g_enabled = ReadInt(ini, "Enabled", 1, 0, 1) != 0;
        g_rarity = ReadInt(ini, "Rarity", 1, 0, 1) != 0;
        g_style = ReadInt(ini, "RarityStyle", 1, 0, 3);
        // Яркость - шагами по 10% (1..10): ползунок с сотыми был неудобен.
        g_rarityAlpha = ReadInt(ini, "RarityStrength", 6, 1, 10) / 10.0f;
        g_minGrade = ReadInt(ini, "MinGrade", 2, 0, 5);
        g_starCorner = ReadInt(ini, "StarCorner", 2, 0, 3);
        g_stolen = ReadInt(ini, "Stolen", 1, 0, 1) != 0;
        g_stolenSize = ReadInt(ini, "StolenSize", 16, 8, 40);
        g_stolenAlpha = ReadInt(ini, "StolenStrength", 9, 1, 10) / 10.0f;
        g_faction = ReadInt(ini, "Faction", 1, 0, 1) != 0;
        g_factionSize = ReadInt(ini, "FactionSize", 16, 8, 40);
        g_factionAlpha = ReadInt(ini, "FactionStrength", 9, 1, 10) / 10.0f;
        g_blueprint = ReadInt(ini, "Blueprint", 1, 0, 1) != 0;
        char key[64] = {};
        GetPrivateProfileStringA("ItemMarkers", "HoldKey", "NONE", key, sizeof(key), ini.c_str());
        g_holdKey = HoldKey::Parse(key);
        g_debug = ReadInt(ini, "Debug", 0, 0, 1) != 0;
        RefreshAll();                   // из MCM - перерисовать открытые иконки
    }


    // ---------------------------------------------------------------
    // Сорт
    // ---------------------------------------------------------------

    bool Graded(const Item* item)
    {
        if (item == NULL || item->data == NULL)
            return false;
        const itemType t = item->data->type;
        return t == ARMOUR || t == WEAPON || t == CROSSBOW || t == LIMB_REPLACEMENT;
    }

    // Броня, оружие, арбалеты, протезы - классы от Gear: уровень и форму
    // фракции читаем из его полей. Виртуальные getLevel/isAFactionUniform
    // через таблицу из заголовка KenshiLib не зовём: ошибись там номер
    // ячейки - вызвалась бы чужая функция.
    int LevelOf(const Item* item)
    {
        return static_cast<const Gear*>(item)->level_0_100;
    }

    // Форма фракции - виртуальный Item::isAFactionUniform, ячейка 0x2F8 (как
    // в заголовке KenshiLib; так же зовёт LtEast). Поле Gear::isUniform
    // игра, похоже, не заполняет: по нему формы не нашлось ни одной
    // (09.10.2026).
    bool IsUniform(Item* item)
    {
        if (item == NULL)
            return false;
        typedef Faction* (*UniformFn)(Item*);
        const UniformFn fn = (*reinterpret_cast<UniformFn* const*>(item))[0x2F8 / sizeof(void*)];
        return fn(item) != NULL;
    }

    int GradeOf(const Item* item)
    {
        if (!Graded(item))
            return -1;
        const int level = LevelOf(item);
        if (level <= 15) return 0;
        if (level <= 35) return 1;
        if (level <= 50) return 2;
        if (level <= 65) return 3;
        if (level <= 80) return 4;
        return 5;
    }

    // Цвета сортов - из палитры игры (kenshi_colours.xml с модами).
    std::string g_gradeHex[GRADES];
    MyGUI::Colour g_gradeColour[GRADES];
    bool g_coloursRead = false;

    void ReadColours()
    {
        if (g_coloursRead)
            return;
        g_coloursRead = true;
        static const char* const fallback[GRADES] = { "#666666", "#d9ccbf", "#4099b3", "#66fc80", "#fceb0d", "" };
        for (int g = 0; g < GRADES; ++g)
        {
            char key[16];
            sprintf_s(key, "ItemGrade%d", g);
            // ItemGrade5 в ванили нет - цвет сорта 4, как у игры
            std::string hex = GameTheme::Hex(key, g == GRADES - 1 ? g_gradeHex[g - 1].c_str() : fallback[g]);
            for (size_t k = 0; k < hex.size(); ++k)
                hex[k] = static_cast<char>(tolower(static_cast<unsigned char>(hex[k])));
            g_gradeHex[g] = hex;
            g_gradeColour[g] = GameTheme::Parse(hex.c_str(), MyGUI::Colour::White);
        }
    }


    // ---------------------------------------------------------------
    // Иконки и их пометки
    // ---------------------------------------------------------------

    struct IconEntry
    {
        Item* item;
        std::vector<MyGUI::Widget*> marks;     // дети корня иконки
    };
    std::map<MyGUI::Widget*, IconEntry> g_icons;   // корень иконки -> вещь
    bool g_shown = true;                            // показаны ли пометки сейчас

    // MyGUI сообщает о каждом удаляемом виджете: корень иконки ушёл -
    // забыть его (пометки - его дети, уходят с ним).
    class Unlinker : public MyGUI::IUnlinkWidget
    {
    public:
        virtual void _unlinkWidget(MyGUI::Widget* widget)
        {
            if (!g_icons.empty())
                g_icons.erase(widget);
        }
    };
    Unlinker* g_unlinker = NULL;       // живёт, пока жива DLL

    void EnsureUnlinker()
    {
        if (g_unlinker != NULL)
            return;
        MyGUI::WidgetManager* const manager = MyGUI::WidgetManager::getInstancePtr();
        if (manager == NULL)
            return;
        g_unlinker = new Unlinker();
        manager->registerUnlinker(g_unlinker);
    }

    const std::string& Texture(const char* file)
    {
        static std::map<std::string, std::string> resolved;
        std::map<std::string, std::string>::iterator it = resolved.find(file);
        if (it != resolved.end())
            return it->second;
        const std::string names[] = { std::string("gui/gfx/") + file, std::string("gui\\gfx\\") + file, file };
        std::string texture;
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]) && texture.empty(); ++i)
        {
            try
            {
                if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(names[i]))
                    texture = names[i];
            }
            catch (...)
            {
            }
        }
        if (texture.empty())
            ErrorLog(std::string("ItemMarkers: gui/gfx/") + file + " not found");
        return resolved[file] = texture;
    }

    MyGUI::ImageBox* MakeImage(MyGUI::Widget* root, const MyGUI::IntCoord& coord, MyGUI::Align align)
    {
        MyGUI::ImageBox* const image = root->createWidget<MyGUI::ImageBox>("ImageBox", coord, align);
        if (image != NULL)
        {
            image->setNeedMouseFocus(false);   // щелчки и перетаскивание - иконке
            image->setVisible(g_shown);
        }
        return image;
    }

    // Глубину - ПОСЛЕ текстуры. setDepth заново выстраивает отрисовку всех
    // детей иконки по глубине (подложка - первой, под картинкой вещи), а
    // назначение текстуры ставит отрисовку виджета в конец очереди - поверх.
    // 09.10.2026 глубина стояла до текстуры, и заливка с градиентом лежали
    // поверх вещей. Так же у LtEast: текстура, потом setDepth(10000).
    void SetDepthLast(MyGUI::Widget* w, int depth)
    {
        if (w != NULL)
            w->setDepth(depth);
    }

    // Чертёж изучен: исследование завершено или чертёж прочитан (игра тогда
    // пишет «Research already known»). Чертёж узнаём по getClassType() - как
    // DarkUiTweaks: в data у предмета-чертежа лежит сама запись исследования
    // (тип RESEARCH), по data->type == BLUEPRINT глаз не находился (09.10.2026).
    bool BlueprintLearned(Item* item)
    {
        if (item == NULL || item->getClassType() != BLUEPRINT)
            return false;
        if (ou == NULL || ou->player == NULL || ou->player->technology == NULL)
            return false;
        GameData* const research = static_cast<BlueprintItem*>(item)->getResearchData();
        if (research == NULL)
            return false;
        Research* const tech = ou->player->technology;
        return tech->isFinished(research) || tech->hasPaidFor(research);
    }

    void Decorate(MyGUI::Widget* root, IconEntry& entry)
    {
        Item* const item = entry.item;
        if (root == NULL || item == NULL || item->data == NULL)
            return;
        const MyGUI::IntSize size = root->getSize();
        if (size.width <= 0 || size.height <= 0)
            return;

        if (g_rarity)
        {
            const int grade = GradeOf(item);
            if (grade >= g_minGrade)
            {
                ReadColours();
                const MyGUI::Colour& colour = g_gradeColour[grade];
                if (g_style == 1)
                {
                    // Рамка - четыре полосы по краям, толщина в пикселях:
                    // одна картинка, натянутая на ячейку, у длинного
                    // оружия давала толстые бока и тонкий верх (09.10.2026).
                    // В полосе - сплошная кромка и свечение внутрь.
                    int d = (size.width < size.height ? size.width : size.height) / 4;
                    if (d < 6) d = 6;
                    if (d > 14) d = 14;
                    const struct { const char* file; MyGUI::IntCoord coord; MyGUI::Align align; } parts[4] = {
                        { "itemmarkers_glow_top.png", MyGUI::IntCoord(0, 0, size.width, d), MyGUI::Align::HStretch | MyGUI::Align::Top },
                        { "itemmarkers_glow_bottom.png", MyGUI::IntCoord(0, size.height - d, size.width, d), MyGUI::Align::HStretch | MyGUI::Align::Bottom },
                        { "itemmarkers_glow_left.png", MyGUI::IntCoord(0, 0, d, size.height), MyGUI::Align::Left | MyGUI::Align::VStretch },
                        { "itemmarkers_glow_right.png", MyGUI::IntCoord(size.width - d, 0, d, size.height), MyGUI::Align::Right | MyGUI::Align::VStretch } };
                    for (int k = 0; k < 4; ++k)
                    {
                        const std::string& texture = Texture(parts[k].file);
                        if (texture.empty())
                            continue;
                        MyGUI::ImageBox* const edge = MakeImage(root, parts[k].coord, parts[k].align);
                        if (edge != NULL)
                        {
                            edge->setImageTexture(texture);
                            edge->setColour(colour);
                            edge->setAlpha(g_rarityAlpha);
                            SetDepthLast(edge, BACK_DEPTH);
                            entry.marks.push_back(edge);
                        }
                    }
                }
                else if (g_style != 3)
                {
                    static const char* const files[4] = { "itemmarkers_fill.png", "", "itemmarkers_gradient.png", "" };
                    const std::string& texture = Texture(files[g_style]);
                    if (!texture.empty())
                    {
                        MyGUI::IntCoord coord(0, 0, size.width, size.height);
                        MyGUI::Align align = MyGUI::Align::Stretch;
                        MyGUI::ImageBox* const back = MakeImage(root, coord, align);
                        if (back != NULL)
                        {
                            back->setImageTexture(texture);
                            back->setColour(colour);
                            back->setAlpha(g_style == 0 ? g_rarityAlpha * 0.5f : g_rarityAlpha);
                            SetDepthLast(back, BACK_DEPTH);
                            entry.marks.push_back(back);
                        }
                    }
                }
            }
        }

        // Звезда - поверх картинки, в выбранном углу.
        if (g_rarity && g_style == 3 && Graded(item) && GradeOf(item) >= g_minGrade)
        {
            const std::string& texture = Texture("itemmarkers_star.png");
            if (!texture.empty())
            {
                ReadColours();
                const int side = size.width < 80 ? 16 : 20;
                const bool right = g_starCorner == 1 || g_starCorner == 3;
                const bool bottom = g_starCorner >= 2;
                MyGUI::ImageBox* const star = MakeImage(root,
                    MyGUI::IntCoord(right ? size.width - side - 2 : 2, bottom ? size.height - side - 2 : 2, side, side),
                    MyGUI::Align(right ? MyGUI::Align::Right : MyGUI::Align::Left) |
                        MyGUI::Align(bottom ? MyGUI::Align::Bottom : MyGUI::Align::Top));
                if (star != NULL)
                {
                    star->setImageTexture(texture);
                    star->setColour(g_gradeColour[GradeOf(item)]);
                    SetDepthLast(star, FRONT_DEPTH);
                    entry.marks.push_back(star);
                }
            }
        }

        // значки - слева сверху, по очереди; звезда там же - значки за ней
        int corner = (g_rarity && g_style == 3 && g_starCorner == 0 && GradeOf(item) >= g_minGrade)
                         ? (size.width < 80 ? 16 : 20) + 4 : 2;
        if (g_stolen && item->isStolen(true))
        {
            MyGUI::ImageBox* const mark = MakeImage(root, MyGUI::IntCoord(corner, 2, g_stolenSize, g_stolenSize),
                                                    MyGUI::Align::Left | MyGUI::Align::Top);
            if (mark != NULL)
            {
                mark->setItemResource("pic_PointerSteal");   // курсор кражи игры - в стиле любого интерфейса
                mark->setAlpha(g_stolenAlpha);
                SetDepthLast(mark, FRONT_DEPTH);
                entry.marks.push_back(mark);
                corner += g_stolenSize + 1;
            }
        }
        if (g_faction && IsUniform(item))
        {
            MyGUI::ImageBox* const mark = MakeImage(root, MyGUI::IntCoord(corner, 2, g_factionSize, g_factionSize),
                                                    MyGUI::Align::Left | MyGUI::Align::Top);
            if (mark != NULL)
            {
                mark->setItemResource("pic_PointerHand");
                mark->setAlpha(g_factionAlpha);
                SetDepthLast(mark, FRONT_DEPTH);
                entry.marks.push_back(mark);
            }
        }

        // Глаз на изученном чертеже - в левом нижнем углу: у торговца сразу
        // видно, что покупать незачем (перенесено из DarkUiTweaks 09.10.2026).
        if (g_blueprint && BlueprintLearned(item))
        {
            const std::string& texture = Texture("itemmarkers_known.png");
            if (!texture.empty())
            {
                int side = (size.width < size.height ? size.width : size.height) * 2 / 5;
                if (side < 14) side = 14;
                if (side > 28) side = 28;
                MyGUI::ImageBox* const eye = MakeImage(root, MyGUI::IntCoord(2, size.height - side - 2, side, side),
                                                       MyGUI::Align::Left | MyGUI::Align::Bottom);
                if (eye != NULL)
                {
                    eye->setImageTexture(texture);
                    SetDepthLast(eye, FRONT_DEPTH);
                    entry.marks.push_back(eye);
                }
            }
        }
    }

    void Undecorate(IconEntry& entry)
    {
        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        for (size_t k = 0; k < entry.marks.size(); ++k)
            if (gui != NULL && entry.marks[k] != NULL)
                gui->destroyWidget(entry.marks[k]);
        entry.marks.clear();
    }

    void RefreshAll()
    {
        g_coloursRead = false;
        for (std::map<MyGUI::Widget*, IconEntry>::iterator it = g_icons.begin(); it != g_icons.end(); ++it)
        {
            Undecorate(it->second);
            if (g_enabled)
                Decorate(it->first, it->second);
        }
    }

    // ---------------------------------------------------------------
    // Хуки
    // ---------------------------------------------------------------

    void OnIcon(InventoryIcon* self, Item* item)
    {
        if (!g_enabled || self == NULL || item == NULL)
            return;
        MyGUI::Widget* const root = self->getWidget();
        if (root == NULL)
            return;
        EnsureUnlinker();
        if (g_unlinker == NULL)
            return;                         // без него корни не забыть - не рискуем
        IconEntry& entry = g_icons[root];
        entry.item = item;
        entry.marks.clear();
        Decorate(root, entry);
        if (g_debug && item->getClassType() == BLUEPRINT)
        {
            static unsigned s_blueprints = 0;
            if (s_blueprints < 50)
            {
                ++s_blueprints;
                char line[256];
                sprintf_s(line, "ItemMarkers: blueprint '%s' data type %d, learned %d, marks %u",
                          item->data->name.c_str(), static_cast<int>(item->data->type), BlueprintLearned(item) ? 1 : 0,
                          static_cast<unsigned>(entry.marks.size()));
                DebugLog(line);
            }
        }
        if (g_debug && Graded(item))
        {
            static unsigned s_logged = 0;
            if (s_logged < 200)
            {
                ++s_logged;
                char line[256];
                sprintf_s(line, "ItemMarkers: '%s' type %d level %d -> grade %d, stolen %d, uniform %d, marks %u",
                          item->data->name.c_str(), static_cast<int>(item->data->type), LevelOf(item), GradeOf(item),
                          item->isStolen(true) ? 1 : 0, IsUniform(item) ? 1 : 0,
                          static_cast<unsigned>(entry.marks.size()));
                DebugLog(line);
            }
        }
    }

    // Замер (Debug): сколько иконок и сколько времени на них ушло.
    unsigned g_statIcons = 0;
    double g_statMs = 0.0, g_statMaxMs = 0.0;

    double NowMs()
    {
        static LARGE_INTEGER freq = { 0 };
        if (freq.QuadPart == 0)
            QueryPerformanceFrequency(&freq);
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return now.QuadPart * 1000.0 / freq.QuadPart;
    }

    void SafeOnIcon(InventoryIcon* self, Item* item)
    {
        const double started = g_debug ? NowMs() : 0.0;
        __try
        {
            OnIcon(self, item);
            if (g_debug)
            {
                const double spent = NowMs() - started;
                ++g_statIcons;
                g_statMs += spent;
                if (spent > g_statMaxMs)
                    g_statMaxMs = spent;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static bool told = false;
            if (!told)
            {
                told = true;
                ErrorLog("ItemMarkers: marking an item icon failed once - skipped");
            }
        }
    }

    InventoryIcon* (*g_origIconCtor)(InventoryIcon*, Item*, const MyGUI::IntPoint&, MyGUI::Widget*) = NULL;

    InventoryIcon* IconCtor_hook(InventoryIcon* self, Item* item, const MyGUI::IntPoint& position,
                                 MyGUI::Widget* parent)
    {
        InventoryIcon* const result = g_origIconCtor(self, item, position, parent);
        SafeOnIcon(self, item);
        return result;
    }

    // Клавиша показа: каждый кадр - только опрос клавиши.
    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;

    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        if (g_debug && g_statIcons > 0)
        {
            static DWORD s_last = 0;
            const DWORD now = GetTickCount();
            if (now - s_last > 2000)
            {
                s_last = now;
                char line[200];
                sprintf_s(line, "ItemMarkers: %u icons marked, %.1f ms in all, slowest %.2f ms, %u icons tracked",
                          g_statIcons, g_statMs, g_statMaxMs, static_cast<unsigned>(g_icons.size()));
                DebugLog(line);
                g_statIcons = 0;
                g_statMs = g_statMaxMs = 0.0;
            }
        }
        const bool show = g_holdKey.vk == 0 || HoldKey::Held(g_holdKey);
        if (show == g_shown)
            return;
        g_shown = show;
        for (std::map<MyGUI::Widget*, IconEntry>::iterator it = g_icons.begin(); it != g_icons.end(); ++it)
            for (size_t k = 0; k < it->second.marks.size(); ++k)
                it->second.marks[k]->setVisible(show);
    }
}


extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "ItemMarkers", "Item Markers", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Marks on item icons: the colour of the item grade, stolen goods, faction uniforms."));
    api->section(api, Tr("General"));
    api->toggle(api, "ItemMarkers", "Enabled", Tr("Enabled"), Tr("All marks on item icons."), 1, 0);
    api->hotkey(api, "ItemMarkers", "HoldKey", Tr("Show key"), Tr("The marks are shown while this key is held. NONE (Backspace when assigning) - always shown."), "NONE", 0);
    api->section(api, Tr("Grade colour"));
    api->toggle(api, "ItemMarkers", "Rarity", Tr("Grade colour"), Tr("Armour, weapons, crossbows and limbs get the colour of their grade, as in the item tooltip."), 1, 0);
    {
        static const char* const values[] = { "0", "1", "2", "3" };
        static const char* labels[4];
        labels[0] = Tr("Fill");
        labels[1] = Tr("Frame");
        labels[2] = Tr("Gradient");
        labels[3] = Tr("Star");
        api->choice(api, "ItemMarkers", "RarityStyle", Tr("Style"), Tr("Fill - the whole cell, frame - the edge with a glow inside, gradient - from the bottom, star - in the corner."), "1", values, labels, 4, 0);
    }
    {
        static const char* const values[] = { "0", "1", "2", "3" };
        static const char* labels[4];
        labels[0] = Tr("Top left");
        labels[1] = Tr("Top right");
        labels[2] = Tr("Bottom left");
        labels[3] = Tr("Bottom right");
        api->choice(api, "ItemMarkers", "StarCorner", Tr("Star corner"), Tr("Where the star of the Star style goes."), "2", values, labels, 4, 0);
    }
    api->integer(api, "ItemMarkers", "RarityStrength", Tr("Strength"), Tr("How bright the grade colour is, 1-10 (10% steps)."), 6, 1, 10, 0);
    api->integer(api, "ItemMarkers", "MinGrade", Tr("From grade"), Tr("Colour items of this grade and better. 0 and 1 are grey and beige - ordinary items; 2 - from blue."), 2, 0, 5, 0);
    api->section(api, Tr("Badges"));
    api->toggle(api, "ItemMarkers", "Stolen", Tr("Stolen"), Tr("The game's steal pointer on stolen items."), 1, 0);
    api->integer(api, "ItemMarkers", "StolenSize", Tr("Stolen badge size"), Tr("In pixels."), 16, 8, 40, 0);
    api->integer(api, "ItemMarkers", "StolenStrength", Tr("Stolen badge strength"), Tr("How visible the badge is, 1-10 (10% steps)."), 9, 1, 10, 0);
    api->toggle(api, "ItemMarkers", "Faction", Tr("Faction uniform"), Tr("The game's hand pointer on armour that is a faction uniform: wearing it you look like one of them."), 1, 0);
    api->integer(api, "ItemMarkers", "FactionSize", Tr("Uniform badge size"), Tr("In pixels."), 16, 8, 40, 0);
    api->integer(api, "ItemMarkers", "FactionStrength", Tr("Uniform badge strength"), Tr("How visible the badge is, 1-10 (10% steps)."), 9, 1, 10, 0);
    api->toggle(api, "ItemMarkers", "Blueprint", Tr("Eye on learned blueprints"), Tr("An eye in the lower left corner of a blueprint icon if that blueprint is already learned - no need to buy it again."), 1, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "ItemMarkers", "Debug", Tr("Detailed log"), Tr("In RE_Kenshi_log.txt: the first 200 marked items (kind, level, grade) and how long marking takes."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    // Хуки - и при Enabled=0: выключатель проверяется на месте, его можно
    // переключать из MCM без перезапуска.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InventoryIcon::_CONSTRUCTOR), IconCtor_hook, &g_origIconCtor))
    {
        ErrorLog("ItemMarkers: could not hook InventoryIcon - no marks");
        return;
    }
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&ForgottenGUI::update), GuiUpdate_hook, &g_origGuiUpdate))
        ErrorLog("ItemMarkers: could not hook the interface update - the show key does not work");
    DebugLog("ItemMarkers: installed");
}
