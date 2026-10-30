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
#include <IniDefaults.h>

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
#include <kenshi/Character.h>
#include <kenshi/util/StringPair.h>

#include <GameTheme.h>
#include <HoldKey.h>
#include "StolenPresentation.h"

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
    // Мешочек на вещах этой сделки (15.10.2026): продали - золотой, купили -
    // зелёный; правый верхний угол.
    bool g_tradeBags = true;
    MyGUI::Colour g_soldColour(0.91f, 0.72f, 0.19f);
    MyGUI::Colour g_boughtColour(0.37f, 0.83f, 0.37f);
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

    // Путь к ini игрока - см. shared/IniDefaults.h (11.10.2026).
    std::string IniPath() { return IniDefaults::Prepare(ModulePath(".ini")); }

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
        g_tradeBags = ReadInt(ini, "TradeBags", 1, 0, 1) != 0;
        {
            char hex[32] = {};
            GetPrivateProfileStringA("ItemMarkers", "TradeSoldColour", "#e8b830", hex, sizeof(hex), ini.c_str());
            g_soldColour = GameTheme::Parse(hex, MyGUI::Colour(0.91f, 0.72f, 0.19f));
            GetPrivateProfileStringA("ItemMarkers", "TradeBoughtColour", "#5fd35f", hex, sizeof(hex), ini.c_str());
            g_boughtColour = GameTheme::Parse(hex, MyGUI::Colour(0.37f, 0.83f, 0.37f));
        }
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
        if (t == ARMOUR)
        {
            // Броня, которая ничего не закрывает (деревянные сандалии, часть
            // штанов): сорт у неё в данных есть, но ни на что не влияет, и
            // игра его не показывает. Мод LtEast красил и её - игрок на
            // Nexus (17.10.2026): «vanilla doesn't show any quality for it
            // because it has no armor coverage». Спрашиваем у самой игры.
            Armour* const armour = const_cast<Item*>(item)->isArmour();
            return armour != NULL && armour->hasArmourCoverage();
        }
        return t == WEAPON || t == CROSSBOW || t == LIMB_REPLACEMENT;
    }

    // Броня, оружие, арбалеты, протезы - классы от Gear: уровень читаем из
    // его поля level_0_100 (виртуальный getLevel не нужен). Форма фракции и
    // тип чертежа - виртуальными вызовами игры (ниже): поля для них игра не
    // заполняет или там лежит другое (09.10.2026).
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
    // Мейто - высший класс оружия; у игры он того же сорта и цвета, что
    // мастерский (ItemGrade5). Свой цвет - ключ ItemGradeMeitou в палитре
    // интерфейса (его прописывает, например, наш Dark UI); нет ключа - цвет
    // ItemGrade5, как у игры (15.10.2026, просьба пользователя и Асура).
    MyGUI::Colour g_meitouColour;
    std::string g_meitouHex;            // как в палитре, в нижнем регистре
    bool g_coloursRead = false;

    // Класс оружия «Meitou» (rebirth.mod, тип записи 50) - по строковому
    // ID записи класса: название переводят, ID - нет.
    bool IsMeitou(const Item* item)
    {
        const GameData* const material = item != NULL ? item->materialData : NULL;
        return material != NULL && material->stringID == "52293-rebirth.mod";
    }

    // Вещь перешла из рук в руки в открытом окне торговли: у неё игра
    // пишет «Выкупить» (Item::getBuyBackGUIInfo даёт строку).
    void BuyBackLinesRaw(Item* item, Ogre::vector<StringPair>::type* lines, bool* ok)
    {
        __try
        {
            item->getBuyBackGUIInfo(*lines);
            *ok = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            *ok = false;
        }
    }

    bool TradedNow(Item* item)
    {
        if (item == NULL)
            return false;
        Ogre::vector<StringPair>::type lines;
        bool ok = false;
        BuyBackLinesRaw(item, &lines, &ok);
        return ok && !lines.empty();
    }

    // У нашего персонажа (купили) или у торговца (продали). Вещь может
    // лежать в рюкзаке - тогда хозяин инвентаря сам рюкзак, а персонаж -
    // хозяин рюкзака: поднимаемся по цепочке (15.10.2026: купленное в
    // рюкзаке получало мешочек проданного).
    bool HeldByPlayer(Item* item)
    {
        const Item* current = item;
        for (int depth = 0; depth < 4 && current != NULL; ++depth)
        {
            const hand& holder = current->_whosInventoryWeAreIn;
            Character* const character = holder.getCharacter();
            if (character != NULL)
                return character->isPlayerCharacter();
            current = holder.getItem();
        }
        return false;
    }

    // Native isStolen is contextual: detach clears the current inventory hand.
    // Preserve its source-context answer only for an exact player-owned drag.
    // No owner/theft/quantity fields are written; foreign origins stay native.
    bool g_dragCaptureReady = false;
    ItemMarkersStolen::DragState g_drag = {};
    struct StolenNative
    {
        typedef Item ItemType;
        bool Enabled() const { return g_enabled && g_stolen && g_dragCaptureReady; }
        bool PlayerOwned(void* raw) const
        {
            InventoryGUI* const inventory = static_cast<InventoryGUI*>(raw);
            Character* const character = inventory != NULL ? inventory->getCallbackCharacter() : NULL;
            if (character != NULL)
                return character->isPlayerCharacter();
            // An unequipped/nested backpack can have no callback character yet.
            // Native getCallbackObject then returns the container itself.
            RootObject* const callback = inventory != NULL ? inventory->getCallbackObject() : NULL;
            return callback != NULL && callback->getDataType() == CONTAINER &&
                   HeldByPlayer(static_cast<Item*>(callback));
        }
        Item* Selected(void* raw, const std::string& section) const
        {
            return static_cast<InventoryGUI*>(raw)->getSelectedItem(section);
        }
        bool NativeStolen(Item* item) const { return item->isStolen(true); }
    };

    bool CaptureDragRaw(InventoryGUI* inventory, const std::string* section, ItemMarkersStolen::DragState* state)
    {
        __try
        {
            StolenNative native;
            *state = ItemMarkersStolen::Capture(native,inventory,*section);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }

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
        g_meitouHex = GameTheme::Hex("ItemGradeMeitou", g_gradeHex[GRADES - 1].c_str());
        for (size_t k = 0; k < g_meitouHex.size(); ++k)
            g_meitouHex[k] = static_cast<char>(tolower(static_cast<unsigned char>(g_meitouHex[k])));
        g_meitouColour = GameTheme::Parse(g_meitouHex.c_str(), g_gradeColour[GRADES - 1]);
    }

    // Цвет пометки вещи: сорт, у мейто - свой
    const MyGUI::Colour& GradeColourOf(const Item* item, int grade)
    {
        return IsMeitou(item) ? g_meitouColour : g_gradeColour[grade];
    }


    // ---------------------------------------------------------------
    // Иконки и их пометки
    // ---------------------------------------------------------------

    struct IconEntry
    {
        Item* item;
        std::vector<MyGUI::Widget*> marks;     // дети корня иконки
        hand handle;                           // вещь по ссылке игры: удалена - пусто
        bool bag;                              // нарисован мешочек сделки
        ItemMarkersStolen::IconState stolen;
        IconEntry() : item(NULL), bag(false) { ItemMarkersStolen::Reset(stolen); }
    };
    std::map<MyGUI::Widget*, IconEntry> g_icons;   // корень иконки -> вещь
    // пометка -> корень её иконки. Игра может удалить детей иконки сама
    // (перестройка иконки), и указатель в marks остаётся висеть: потом
    // RefreshAll (смена настроек в MCM) удалял его второй раз, а клавиша
    // показа звала setVisible - порча кучи и падение при отрисовке
    // интерфейса (дампы 13-14.10.2026, «поменял заливку - вылет»).
    std::map<MyGUI::Widget*, MyGUI::Widget*> g_markRoot;
    bool g_shown = true;                            // показаны ли пометки сейчас

    void ForgetMarks(IconEntry& entry)
    {
        for (size_t k = 0; k < entry.marks.size(); ++k)
            g_markRoot.erase(entry.marks[k]);
        entry.marks.clear();
    }

    // MyGUI сообщает о каждом удаляемом виджете: корень иконки ушёл -
    // забыть его вместе с пометками; ушла сама пометка - убрать её из
    // списка иконки.
    class Unlinker : public MyGUI::IUnlinkWidget
    {
    public:
        virtual void _unlinkWidget(MyGUI::Widget* widget)
        {
            if (!g_markRoot.empty())
            {
                std::map<MyGUI::Widget*, MyGUI::Widget*>::iterator mark = g_markRoot.find(widget);
                if (mark != g_markRoot.end())
                {
                    std::map<MyGUI::Widget*, IconEntry>::iterator icon = g_icons.find(mark->second);
                    if (icon != g_icons.end())
                    {
                        std::vector<MyGUI::Widget*>& marks = icon->second.marks;
                        for (size_t k = 0; k < marks.size(); ++k)
                            if (marks[k] == widget)
                            {
                                marks.erase(marks.begin() + k);
                                break;
                            }
                    }
                    g_markRoot.erase(mark);
                    return;
                }
            }
            if (!g_icons.empty())
            {
                std::map<MyGUI::Widget*, IconEntry>::iterator icon = g_icons.find(widget);
                if (icon != g_icons.end())
                {
                    ForgetMarks(icon->second);
                    g_icons.erase(icon);
                }
            }
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
                const MyGUI::Colour& colour = GradeColourOf(item, grade);
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
                            g_markRoot[edge] = root;
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
                            g_markRoot[back] = root;
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
                    star->setColour(GradeColourOf(item, GradeOf(item)));
                    SetDepthLast(star, FRONT_DEPTH);
                    entry.marks.push_back(star);
                    g_markRoot[star] = root;
                }
            }
        }

        // значки - слева сверху, по очереди; звезда там же - значки за ней
        int corner = (g_rarity && g_style == 3 && g_starCorner == 0 && GradeOf(item) >= g_minGrade)
                         ? (size.width < 80 ? 16 : 20) + 4 : 2;
        StolenNative stolenNative;
        if (g_stolen && ItemMarkersStolen::Show(entry.stolen,stolenNative,item))
        {
            MyGUI::ImageBox* const mark = MakeImage(root, MyGUI::IntCoord(corner, 2, g_stolenSize, g_stolenSize),
                                                    MyGUI::Align::Left | MyGUI::Align::Top);
            if (mark != NULL)
            {
                mark->setItemResource("pic_PointerSteal");   // курсор кражи игры - в стиле любого интерфейса
                mark->setAlpha(g_stolenAlpha);
                SetDepthLast(mark, FRONT_DEPTH);
                entry.marks.push_back(mark);
                g_markRoot[mark] = root;
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
                g_markRoot[mark] = root;
            }
        }

        // Мешочек сделки - в правом верхнем углу (звезда там - левее неё).
        // «Выкупить» в подсказке игра пишет у вещей, которые в этом окне
        // торговли перешли из рук в руки; её же функцией и узнаём. У нашего
        // персонажа - куплена, иначе (у торговца) - продана. Новое окно -
        // новые иконки, а игра сделку уже забыла: пометка уходит сама.
        entry.bag = g_tradeBags && TradedNow(item);
        if (entry.bag)
        {
            const int side = g_stolenSize;
            int right = 2;
            if (g_rarity && g_style == 3 && g_starCorner == 1 && GradeOf(item) >= g_minGrade)
                right += (size.width < 80 ? 16 : 20) + 2;
            MyGUI::ImageBox* const bag = MakeImage(root, MyGUI::IntCoord(size.width - side - right, 2, side, side),
                                                   MyGUI::Align::Right | MyGUI::Align::Top);
            if (bag != NULL)
            {
                // Белый мешочек (из значка кражи игры, обесцвечен): красный
                // значок игры при перекраске оставался красным.
                const std::string& texture = Texture("itemmarkers_bag.png");
                if (!texture.empty())
                    bag->setImageTexture(texture);
                else
                    bag->setItemResource("pic_PointerSteal");
                bag->setColour(HeldByPlayer(item) ? g_boughtColour : g_soldColour);
                SetDepthLast(bag, FRONT_DEPTH);
                entry.marks.push_back(bag);
                g_markRoot[bag] = root;
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
                    g_markRoot[eye] = root;
                }
            }
        }
    }

    void Undecorate(IconEntry& entry)
    {
        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        // копия: каждое удаление через Unlinker правит entry.marks
        const std::vector<MyGUI::Widget*> marks = entry.marks;
        for (size_t k = 0; k < marks.size(); ++k)
            if (gui != NULL && marks[k] != NULL)
                gui->destroyWidget(marks[k]);
        ForgetMarks(entry);
    }

    // Перерисовка из MCM идёт по уже открытым иконкам: вещь могла уйти
    // раньше иконки - сбой одной иконки не должен ронять игру (ревью
    // 09.10.2026; при создании иконки та же защита - SafeOnIcon).
    bool SafeDecorate(MyGUI::Widget* root, IconEntry* entry)
    {
        __try
        {
            Decorate(root, *entry);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void RefreshAll()
    {
        g_coloursRead = false;
        unsigned failed = 0;
        for (std::map<MyGUI::Widget*, IconEntry>::iterator it = g_icons.begin(); it != g_icons.end(); ++it)
        {
            Undecorate(it->second);
            if (g_enabled && !SafeDecorate(it->first, &it->second))
                ++failed;
        }
        if (failed > 0)
        {
            char line[96];
            sprintf_s(line, "ItemMarkers: %u icons could not be redrawn - skipped", failed);
            ErrorLog(line);
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
        entry.handle = item->getHandle();
        ItemMarkersStolen::OnIcon(entry.stolen,g_drag,item);
        ForgetMarks(entry);
        Decorate(root, entry);
        if (g_debug && entry.stolen.frozen)
        {
            static unsigned logged = 0;
            if (logged < 64)
            {
                ++logged;
                char line[192];
                sprintf_s(line,"ItemMarkers: player drag stolen badge source=%d current=%d; icon-local snapshot",
                          entry.stolen.stolen ? 1 : 0,item->isStolen(true) ? 1 : 0);
                DebugLog(line);
            }
        }
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
            StolenNative native;
            static unsigned s_logged = 0;
            if (s_logged < 200)
            {
                ++s_logged;
                char line[256];
                sprintf_s(line, "ItemMarkers: '%s' type %d level %d -> grade %d, stolen %d, uniform %d, marks %u",
                          item->data->name.c_str(), static_cast<int>(item->data->type), LevelOf(item), GradeOf(item),
                          ItemMarkersStolen::Show(entry.stolen,native,item) ? 1 : 0, IsUniform(item) ? 1 : 0,
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

    // Строка класса в подсказке оружия («[Модель # Мейто]») игра красит
    // цветом ItemGrade5 - тем же, что у мастерского. У мейто этот цвет в
    // строках подсказки меняется на цвет мейто (15.10.2026). Цвет в строке -
    // тег MyGUI «#rrggbb» (регистр любой) или «#{ItemGrade5}».
    void RecolourMeitouText(std::string& text)
    {
        const std::string& from = g_gradeHex[GRADES - 1];
        if (from.size() != 7 || g_meitouHex.size() != 7 || from == g_meitouHex)
            return;
        for (size_t i = 0; i + 7 <= text.size(); ++i)
        {
            if (text[i] != '#')
                continue;
            bool same = true;
            for (size_t k = 1; k < 7 && same; ++k)
                same = tolower(static_cast<unsigned char>(text[i + k])) == from[k];
            if (same)
                text.replace(i, 7, g_meitouHex);
        }
        const std::string tag = "#{ItemGrade5}";
        for (std::string::size_type at = text.find(tag); at != std::string::npos; at = text.find(tag, at))
            text.replace(at, tag.size(), g_meitouHex);
    }

    void RecolourMeitouLines(Item* item, Ogre::vector<StringPair>::type& lines)
    {
        if (!g_enabled || !g_rarity || !IsMeitou(item))
            return;
        ReadColours();
        for (size_t i = 0; i < lines.size(); ++i)
        {
            RecolourMeitouText(lines[i].s1);
            RecolourMeitouText(lines[i].s2);
        }
        if (g_debug)
        {
            static int logged = 0;
            if (logged < 2)
            {
                ++logged;
                for (size_t i = 0; i < lines.size(); ++i)
                    DebugLog(("ItemMarkers: meitou tooltip line [" + lines[i].s1 + "] [" + lines[i].s2 + "]").c_str());
            }
        }
    }

    void SafeRecolourMeitou(Item* item, Ogre::vector<StringPair>::type* lines)
    {
        __try
        {
            RecolourMeitouLines(item, *lines);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    typedef void (*TooltipFn)(Item*, Ogre::vector<StringPair>::type&);
    TooltipFn g_origWeaponTip1 = NULL;
    TooltipFn g_origWeaponTip2 = NULL;
    TooltipFn g_origSwordTip1 = NULL;
    TooltipFn g_origSwordTip2 = NULL;

    void WeaponTip1_hook(Item* self, Ogre::vector<StringPair>::type& lines)
    {
        g_origWeaponTip1(self, lines);
        SafeRecolourMeitou(self, &lines);
    }
    void WeaponTip2_hook(Item* self, Ogre::vector<StringPair>::type& lines)
    {
        g_origWeaponTip2(self, lines);
        SafeRecolourMeitou(self, &lines);
    }
    void SwordTip1_hook(Item* self, Ogre::vector<StringPair>::type& lines)
    {
        g_origSwordTip1(self, lines);
        SafeRecolourMeitou(self, &lines);
    }
    void SwordTip2_hook(Item* self, Ogre::vector<StringPair>::type& lines)
    {
        g_origSwordTip2(self, lines);
        SafeRecolourMeitou(self, &lines);
    }

    InventoryIcon* (*g_origIconCtor)(InventoryIcon*, Item*, const MyGUI::IntPoint&, MyGUI::Widget*) = NULL;

    InventoryIcon* IconCtor_hook(InventoryIcon* self, Item* item, const MyGUI::IntPoint& position,
                                 MyGUI::Widget* parent)
    {
        InventoryIcon* const result = g_origIconCtor(self, item, position, parent);
        SafeOnIcon(self, item);
        return result;
    }

    bool (*g_origPickup)(InventoryGUI*,std::string) = NULL;
    Item* (*g_origTakeAmount)(InventoryGUI*,Item*,int) = NULL;

    struct PickupCall
    {
        InventoryGUI* inventory;
        const std::string* section;
        bool Original() { return g_origPickup(inventory,*section); }
    };

    bool PickupScopedRaw(InventoryGUI* inventory,const std::string* section,
                         const ItemMarkersStolen::DragState* next)
    {
        const ItemMarkersStolen::DragState previous = g_drag;
        PickupCall call = { inventory,section };
        // POD locals only. Run handles C++ unwinding; finally also handles SEH.
        __try { return ItemMarkersStolen::Run(g_drag,*next,call); }
        __finally { g_drag = previous; }
    }

    bool Pickup_hook(InventoryGUI* self,std::string section)
    {
        ItemMarkersStolen::DragState next = {};
        if (!CaptureDragRaw(self,&section,&next))
        {
            const ItemMarkersStolen::DragState empty = {};
            next = empty;
            static bool logged = false;
            if (!logged)
            {
                logged = true;
                ErrorLog("ItemMarkers: drag source context unavailable; native badge retained");
            }
        }
        return PickupScopedRaw(self,&section,&next);
    }

    Item* TakeAmount_hook(InventoryGUI* self,Item* source,int amount)
    {
        Item* const result = g_origTakeAmount(self,source,amount);
        ItemMarkersStolen::SplitResult(g_drag,self,source,result);
        return result;
    }

    // Продали или купили в уже лежащую стопку - игра новую иконку не
    // создаёт, только число меняет: мешочка не было (15.10.2026). Пока
    // открыты окна, раз в 0,4 с видимые иконки спрашиваются заново; сменилось
    // - пометки иконки рисуются заново. Вещь - по ссылке игры, не по
    // указателю: слитая в стопку вещь удалена, ссылка тогда пустая.
    void RecheckTradeBags(ForgottenGUI* gui)
    {
        if (!g_enabled || !g_tradeBags || !g_shown || g_icons.empty() || gui == NULL)
            return;
        // Только пока идёт торговля (участники окна торговли заданы): вне
        // торговли «вещей сделки» не бывает (ревью 15.10.2026).
        if (gui->tradeA.getRootObject() == NULL && gui->tradeB.getRootObject() == NULL)
            return;
        static DWORD s_last = 0;
        const DWORD now = GetTickCount();
        if (now - s_last < 400)
            return;
        s_last = now;
        for (std::map<MyGUI::Widget*, IconEntry>::iterator it = g_icons.begin(); it != g_icons.end(); ++it)
        {
            IconEntry& entry = it->second;
            if (!it->first->getInheritedVisible())
                continue;
            Item* const live = entry.handle.getItem();
            if (live == NULL || live != entry.item)
                continue;
            if (TradedNow(live) == entry.bag)
                continue;
            Undecorate(entry);
            SafeDecorate(it->first, &entry);
        }
    }

    // Клавиша показа: каждый кадр - только опрос клавиши.
    void (*g_origGuiUpdate)(ForgottenGUI*) = NULL;

    void GuiUpdate_hook(ForgottenGUI* self)
    {
        g_origGuiUpdate(self);
        RecheckTradeBags(self);
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
    api->toggle(api, "ItemMarkers", "TradeBags", Tr("Bought and sold in this trade"), Tr("A bag in the top right corner of items that changed hands in the open trade window: gold - you sold it, green - you bought it. Gone in the next trade window."), 1, 0);
    api->colour(api, "ItemMarkers", "TradeSoldColour", Tr("Sold bag"), Tr("Colour of the bag on items you sold, #RRGGBB."), "#e8b830", 0);
    api->colour(api, "ItemMarkers", "TradeBoughtColour", Tr("Bought bag"), Tr("Colour of the bag on items you bought, #RRGGBB."), "#5fd35f", 0);
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
    const bool takeHook = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&InventoryGUI::takeCertainAmountFrom),TakeAmount_hook,&g_origTakeAmount);
    const bool pickupHook = KenshiLib::SUCCESS == KenshiLib::AddHook(
        KenshiLib::GetRealAddress(&InventoryGUI::pickupItemToMouse),Pickup_hook,&g_origPickup);
    g_dragCaptureReady = takeHook && pickupHook;
    if (!g_dragCaptureReady)
        ErrorLog("ItemMarkers: drag badge hooks unavailable; native badge retained");
    // строка класса мейто в подсказке оружия - цветом мейто
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Weapon::_NV_getTooltipData1), WeaponTip1_hook, &g_origWeaponTip1) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Weapon::_NV_getTooltipData2), WeaponTip2_hook, &g_origWeaponTip2) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Sword::_NV_getTooltipData1), SwordTip1_hook, &g_origSwordTip1) ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Sword::_NV_getTooltipData2), SwordTip2_hook, &g_origSwordTip2))
        ErrorLog("ItemMarkers: could not hook the weapon tooltip - the Meitou line keeps the game's colour");
    DebugLog("ItemMarkers: installed");
}
