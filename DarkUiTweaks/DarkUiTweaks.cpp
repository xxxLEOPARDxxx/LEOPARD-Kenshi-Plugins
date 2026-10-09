// DarkUiTweaks - доработки интерфейса, которые вёрсткой не делаются.
//
// Отдельный мод mods\DarkUiTweaks. Сначала жил в папке мода
// «Russian Dark UI simplistic», но от разметки не зависит: правит
// размеры виджетов, которые игра строит сама, и работает с любым
// интерфейсом (что где оговорено - README.txt). Правки разметки самого
// Dark UI (скрипты fix_darkui_*.py на стенде) остались в том моде.
//
// ЧТО ДЕЛАЕТ
//
// 1. Высота строк в очереди исследований и ремесла. Строки тесные, текст
//    налезает сам на себя - особенно на русских шрифтах, которые выше
//    английских. Высоту ячейки задаёт игра
//    (ReorderableListItem::getCellDimension), и разметка на неё не
//    влияет вовсе: Kenshi_ItemListItem.layout описывает только начинку
//    строки.
//
//    Лезть хуком в саму getCellDimension не нужно. У MyGUI::ItemBox
//    высота ячейки спрашивается через открытое поле-делегат
//    requestCoordItem - его достаточно подменить своим. Никаких
//    восстановленных адресов и неэкспортированных функций.
//
// 2. Заливка полосы в строке очереди - во всю рамку строки. Игровая
//    полоса до конца не доходит; почему и как это обойдено - в разделе
//    «Полоса заполнения».
//
// 3. Прокрутка в панелях описания. Описание исследования не влезает -
//    у наёмничьих доспехов там полтора десятка строк «Тип доспеха».
//    Панель создаётся вызовом
//    ForgottenGUI::createDatapanel(имя, виджет, scrolls), и у неё есть
//    ШТАТНЫЙ флаг прокрутки; игра передаёт его нулём. Перехватываем
//    вызов и передаём единицу. Вокруг перезаполнения описания
//    прокрутка удерживается на месте.
//
// 4. Высота ячейки отряда на вкладке «Отряды». Портреты в ячейке
//    видны на полтора ряда, второй ряд обрезан. Высоту задаёт игра
//    (SquadCellView::getCellDimension), а та берёт её из статической
//    переменной SquadCellView::Height. Ни подмена делегата, ни перехват
//    функции не сработали (см. раздел), поэтому правим саму переменную.
//    Прибавку к высоте подбираем по месту: меряем,
//    сколько вышло под портреты, и добавляем недостающее. Рядов - сколько
//    нужно самому большому отряду, но не меньше SquadPortraitRows.
//
// 5. Полоса в строке панели описания - «Следующий уровень» у навыка.
//    Строку целиком строит код панели описания, разметки у неё нет.
//    Скин полосы рассчитан на высоту около 20 пикселей (рамка с углами
//    в 5-6), а игра сплющивает его до семи - рамка и заливка наезжают
//    друг на друга, подпись лежит поверх. Каждый кадр находим строку с
//    полосой в панели описания окна навыков: подложку делаем выше,
//    заливку вписываем в её кайму, текст ставим по середине, а всё, что
//    ниже, сдвигаем.
//
// ПОЧЕМУ ХУК ИМЕННО НА ForgottenGUI::update, а не на игровой цикл.
// Обращаться к MyGUI из GameWorld::_NV_mainLoop_GPUSensitiveStuff
// нельзя - проверено трижды, игра падает (см. shared\GameTheme.h).
// ForgottenGUI::update - это код интерфейса, оттуда MyGUI доступен;
// Character Inspector давно работает через него.

#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "dark_ui_tweaks"
#include <Localization.h>
#include <ModConfigMenu.h>

#include <Windows.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_ItemBox.h>
#include <mygui/MyGUI_ProgressBar.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_RenderManager.h>

#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>
#include <WidgetRef.h>
#include <HoldKey.h>
#include <GameTheme.h>

#include <KenshiSlots.h>

#define private public
#define protected public
#include <kenshi/gui/DatapanelGUI.h>
#undef private
#undef protected

#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/SquadManagementScreen.h>
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/Platoon.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Faction.h>
#include <kenshi/FactionRelations.h>
#include <kenshi/Research.h>
#include <kenshi/Item.h>
#include <kenshi/RootObject.h>
#include <kenshi/util/hand.h>
#include <kenshi/Building/CraftingBuilding.h>
#include <kenshi/Inventory.h>
#include <kenshi/gui/ManagementScreen.h>
#include <kenshi/RootObjectFactory.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/util/iVector2.h>
#include <kenshi/KingOfRenderThread.h>
#include <kenshi/Renderer.h>
#include <kenshi/RenderToTextureotron.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_scrollDescription = true;
    int g_rowHeight = 30;
    int g_barInset = 5;
    int g_squadRows = 2;
    bool g_squadAutoGrow = true;
    int g_progressPad = 5;
    int g_progressDrop = 2;
    int g_progressGrowUp = 1;
    bool g_factionRelation = true;   // отношение фракции у имени выбранного
    bool g_craftIcon = true;         // иконка вещи у мыши в списке рецептов
    // Клавиша иконки (09.10.2026, просьба тестеров; задержку убрали):
    // иконка - пока зажата; NONE - просто по наведению.
    HoldKey::Key g_craftIconKey;
    bool g_debug = false;


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
    // .ini кладётся рядом с DLL при установке.
    void WriteDefaultIni(const std::string& ini)
    {
        FILE* f = NULL;
        if (fopen_s(&f, ini.c_str(), "w") != 0 || f == NULL)
            return;

        fputs("; DarkUiTweaks - what the layout files cannot do.\n", f);
        fputs(";\n", f);
        fputs("; QueueRowHeight - height of a row in the research and\n", f);
        fputs(";   crafting queues, in pixels. The game gives them about\n", f);
        fputs(";   16; on Russian fonts the text does not fit.\n", f);
        fputs(";   0 leaves the game's own height.\n", f);
        fputs("; QueueBarInset - how far the progress bar stays away\n", f);
        fputs(";   from the edge of the row, in pixels. The frame of\n", f);
        fputs(";   the row skin is six pixels wide, but five sits\n", f);
        fputs(";   flush against the black line. 0 leaves the bar\n", f);
        fputs(";   to the game.\n", f);
        fputs("; SquadPortraitRows - how many rows of portraits a squad\n", f);
        fputs(";   cell shows without clipping. 0 leaves the game's own\n", f);
        fputs(";   height.\n", f);
        fputs("; SquadAutoGrow - grow the squad cells when the biggest\n", f);
        fputs(";   squad needs more rows than SquadPortraitRows. All\n", f);
        fputs(";   cells of the list share one height.\n", f);
        fputs("; ProgressBarPad - progress bars inside description\n", f);
        fputs(";   panels (next skill level): the bar is as tall as the\n", f);
        fputs(";   text line plus this many pixels above and below, and\n", f);
        fputs(";   the text sits in its middle. 0 leaves the game's bar.\n", f);
        fputs("; ProgressBarDrop - move that bar down by this many\n", f);
        fputs(";   pixels, the text stays: the font sits high in its\n", f);
        fputs(";   box, so the text looks centred only this way.\n", f);
        fputs("; ProgressBarGrowUp - raise the top edge of that bar by\n", f);
        fputs(";   this many pixels; the bottom edge stays.\n", f);
        fputs("; ScrollDescription - turn the scrollbar on for the\n", f);
        fputs(";   description panels.\n", f);
        fputs("; FactionRelation - after the faction name of the selected\n", f);
        fputs(";   character: that faction's relation to you, (+15) green,\n", f);
        fputs(";   (-40) red from -10 down.\n", f);
        fputs("; CraftIcon - on the Crafting tab, the icon of the item\n", f);
        fputs(";   next to the mouse over a line of the Items list.\n", f);
        fputs("; CraftIconKey - the icon shows while this key is held (ALT,\n", f);
        fputs(";   CTRL+B, ...); NONE - on hover alone.\n", f);
        fputs("; Debug - log every datapanel the game creates, every\n", f);
        fputs(";   queue box found and the sizes of every bar fill.\n", f);
        fputs("\n[Tweaks]\n", f);
        fputs("Enabled=1\n", f);
        fputs("QueueRowHeight=30\n", f);
        fputs("QueueBarInset=5\n", f);
        fputs("SquadPortraitRows=2\n", f);
        fputs("SquadAutoGrow=1\n", f);
        fputs("ProgressBarPad=5\n", f);
        fputs("ProgressBarDrop=2\n", f);
        fputs("ProgressBarGrowUp=1\n", f);
        fputs("ScrollDescription=1\n", f);
        fputs("FactionRelation=1\n", f);
        fputs("CraftIcon=1\n", f);
        fputs("CraftIconKey=ALT\n", f);
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

        g_enabled =
            GetPrivateProfileIntA("Tweaks", "Enabled", 1, ini.c_str()) != 0;
        g_rowHeight =
            GetPrivateProfileIntA("Tweaks", "QueueRowHeight", 30, ini.c_str());
        g_barInset =
            GetPrivateProfileIntA("Tweaks", "QueueBarInset", 5, ini.c_str());
        g_squadRows =
            GetPrivateProfileIntA("Tweaks", "SquadPortraitRows", 2, ini.c_str());
        g_squadAutoGrow =
            GetPrivateProfileIntA("Tweaks", "SquadAutoGrow", 1,
                                  ini.c_str()) != 0;
        g_progressPad =
            GetPrivateProfileIntA("Tweaks", "ProgressBarPad", 5, ini.c_str());
        g_progressDrop =
            GetPrivateProfileIntA("Tweaks", "ProgressBarDrop", 2, ini.c_str());
        g_progressGrowUp =
            GetPrivateProfileIntA("Tweaks", "ProgressBarGrowUp", 1, ini.c_str());
        g_scrollDescription =
            GetPrivateProfileIntA("Tweaks", "ScrollDescription", 1,
                                  ini.c_str()) != 0;
        g_factionRelation =
            GetPrivateProfileIntA("Tweaks", "FactionRelation", 1, ini.c_str()) != 0;
        g_craftIcon =
            GetPrivateProfileIntA("Tweaks", "CraftIcon", 1, ini.c_str()) != 0;
        {
            char key[64] = {};
            GetPrivateProfileStringA("Tweaks", "CraftIconKey", "ALT", key, sizeof(key), ini.c_str());
            g_craftIconKey = HoldKey::Parse(key);
        }
        g_debug =
            GetPrivateProfileIntA("Tweaks", "Debug", 0, ini.c_str()) != 0;

        if (g_rowHeight < 0)
            g_rowHeight = 0;
        if (g_rowHeight > 80)
            g_rowHeight = 80;

        if (g_barInset < 0)
            g_barInset = 0;
        if (g_barInset > 16)
            g_barInset = 16;

        if (g_progressPad < 0)
            g_progressPad = 0;
        if (g_progressPad > 12)
            g_progressPad = 12;

        if (g_progressDrop < -g_progressPad)
            g_progressDrop = -g_progressPad;
        if (g_progressDrop > g_progressPad)
            g_progressDrop = g_progressPad;

        if (g_progressGrowUp < 0)
            g_progressGrowUp = 0;
        if (g_progressGrowUp > 8)
            g_progressGrowUp = 8;

        if (g_squadRows < 0)
            g_squadRows = 0;
        if (g_squadRows > 6)
            g_squadRows = 6;
    }


    // ---------------------------------------------------------------
    // Высота строки очереди
    // ---------------------------------------------------------------

    // Ящики очередей ищем обходом дерева, но отбираем по имени.
    //
    // Поиск через Gui::findWidgetT("ResearchQueueList") не находил
    // ничего: BaseLayout приписывает именам префикс, и в дереве ящик
    // зовётся «0 000 000 0BC 939 DC0_ResearchQueueList». Поэтому обход,
    // а сравнение - по окончанию имени.
    //
    // Отбор обязателен. Первая версия брала ВСЕ ItemBox подряд, и
    // журнал показал восемь штук вместо двух: JobsPanel, SquadsPanel,
    // DismissCharPanel и три сетки инвентаря. Сеткам инвентаря наша
    // высота строки ломает квадратную ячейку.
    const char* const QUEUE_SUFFIX[] =
    {
        "_ResearchQueueList",
        "_CraftQueue"
    };

    const int QUEUE_COUNT = sizeof(QUEUE_SUFFIX) / sizeof(QUEUE_SUFFIX[0]);

    // Метки, которые плагин вешает на виджеты игры пользовательской
    // строкой. Метка живёт и умирает вместе с виджетом, поэтому, в
    // отличие от множества указателей, не врёт после загрузки сохранения:
    // новый ящик на месте старого по адресу метки не несёт.
    const char* const TAG_ROWS = "DarkUiTweaksRows";   // ящику задана высота
    const char* const TAG_FILL = "DarkUiTweaksFill";   // наша заливка
    const char* const TAG_WIDE = "DarkUiTweaksWide";   // уже жаловались в журнал


    bool Tagged(MyGUI::Widget* widget, const char* tag)
    {
        return !widget->getUserString(tag).empty();
    }


    void Tag(MyGUI::Widget* widget, const char* tag)
    {
        widget->setUserString(tag, "1");
    }


    bool EndsWith(const std::string& name, const std::string& suffix)
    {
        return name.size() >= suffix.size() &&
               name.compare(name.size() - suffix.size(),
                            suffix.size(), suffix) == 0;
    }


    bool IsQueueBox(const std::string& name)
    {
        for (int i = 0; i < QUEUE_COUNT; ++i)
        {
            if (EndsWith(name, QUEUE_SUFFIX[i]))
                return true;
        }

        return false;
    }


    void CoordItem(MyGUI::ItemBox* sender, MyGUI::IntCoord& coord, bool drop)
    {
        // Одна строка на всю ширину: ItemBox раскладывает ячейки сеткой,
        // и если ячейка уже клиентской области, он поставит их в ряд.
        int width = sender->getClientCoord().width;
        if (width < 32)
            width = 32;

        coord.left = 0;
        coord.top = 0;
        coord.width = width;
        coord.height = g_rowHeight;

        (void)drop;
    }


    void SetRowHeight(MyGUI::ItemBox* box)
    {
        if (Tagged(box, TAG_ROWS))
            return;

        box->requestCoordItem = MyGUI::newDelegate(&CoordItem);
        Tag(box, TAG_ROWS);

        // Одной подмены делегата мало: ItemBox держит размер ячейки в
        // своих метриках и пересчитывает их только при изменении
        // размера или выравнивания. Дёрганье размера на пиксель не
        // помогло - высота строк осталась прежней. setVerticalAlignment
        // пересчитывает метрики безусловно, даже если значение то же.
        box->setVerticalAlignment(box->getVerticalAlignment());

        const MyGUI::IntSize size = box->getSize();

        if (g_debug)
        {
            char note[224];
            sprintf_s(note,
                      "DarkUiTweaks: itembox '%s' rows set to %d "
                      "(size %dx%d)",
                      box->getName().c_str(), g_rowHeight,
                      size.width, size.height);
            DebugLog(note);
        }
    }


    // ---------------------------------------------------------------
    // Полоса заполнения
    //
    // Две беды с игровой полосой.
    //
    // Сто процентов игра меряет не по самой полосе, а по надписи со
    // значением (ItemValue, «2 hrs») - по её ширине. Проверено на
    // завершении исследования: последняя ширина полосы 663.5, ширина
    // надписи 663.7. А надпись кончается раньше рамки строки, вот
    // заливка и не доходила до конца.
    //
    // И писать в игровую полосу бесполезно: игра переписывает ей и
    // положение, и размер уже после нашего хука - первая версия так и не
    // дошла до экрана.
    //
    // Поэтому игровую полосу делаем прозрачной, а рядом ставим свою
    // заливку, в которую игра не пишет вовсе, и переносим на неё долю:
    // ширина игровой полосы / ширина надписи * ширина рамки.
    // ---------------------------------------------------------------

    // Скин игровой полосы из разметки. Взять его у виджета нельзя - в
    // этой версии MyGUI у виджета нет getSkinName.
    const char* const FILL_SKIN = "Kenshi_LifeBarSkin";

    // Толщина рамки скина подложки Kenshi_Button1Skin: угловые подскины
    // 6x6. По вертикали заливка отступает ровно на неё - так было при
    // строке в 26 пикселей и так выглядит правильно. По горизонтали
    // отступ настраивается (QueueBarInset): пятый пиксель встаёт
    // вплотную к чёрной линии.
    const int FRAME_PX = 6;


    MyGUI::Widget* FindChild(MyGUI::Widget* row, const char* suffix)
    {
        const size_t count = row->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = row->getChildAt(i);
            if (EndsWith(child->getName(), suffix))
                return child;
        }

        return NULL;
    }


    MyGUI::Widget* FindTagged(MyGUI::Widget* row, const char* tag)
    {
        const size_t count = row->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = row->getChildAt(i);
            if (Tagged(child, tag))
                return child;
        }

        return NULL;
    }


    // Своя заливка - последним ребёнком строки, как и игровая полоса в
    // разметке; значит, и рисуется в том же порядке относительно надписей.
    // Указатель на неё нигде не храним: строки создаются и уничтожаются,
    // и каждый кадр заливка находится заново по метке.
    MyGUI::Widget* CreateFill(MyGUI::Widget* row, MyGUI::Widget* bar)
    {
        MyGUI::Widget* const fill = row->createWidget<MyGUI::Widget>(
            FILL_SKIN, bar->getCoord(), MyGUI::Align::Default);

        Tag(fill, TAG_FILL);
        fill->setNeedMouseFocus(false);
        fill->setNeedKeyFocus(false);

        return fill;
    }


    // Доля заполнения по игровой полосе. Сто процентов по мнению игры -
    // ширина надписи со значением: на завершении исследования последняя
    // ширина полосы вышла 663.5 при ширине надписи 663.7 (правый край
    // надписи 675 - не он). В ванильной разметке полоса и надпись одной
    // ширины (0.86), так что игра, видимо, так и задумана.
    float BarShare(MyGUI::Widget* bar, float whole, MyGUI::Widget* fill)
    {
        const float share = bar->getWidth() / whole;

        if (share <= 0.0f)
            return 0.0f;

        if (share <= 1.0f)
            return share;

        // Игровая полоса шире принятых ста процентов - значит, догадка
        // про надпись неполна, и заливка упрётся в край раньше времени.
        // Пишем один раз на заливку.
        if (g_debug && !Tagged(fill, TAG_WIDE))
        {
            Tag(fill, TAG_WIDE);

            char note[200];
            sprintf_s(note,
                "DarkUiTweaks: game bar %d is wider than the assumed "
                "100%% %.0f", bar->getWidth(), whole);
            DebugLog(note);
        }

        return 1.0f;
    }


    void AlignBar(MyGUI::Widget* row, MyGUI::Widget* bar)
    {
        // Подложка - первый ребёнок строки, безымянный виджет со скином
        // Kenshi_Button1. Рамку рисует он, и по ней меряем. Нет надписи
        // со значением - разметка не та, какую мы знаем; не трогаем.
        MyGUI::Widget* const value = FindChild(row, "_ItemValue");
        if (value == NULL || row->getChildCount() == 0)
            return;

        MyGUI::Widget* const plate = row->getChildAt(0);

        const float whole = static_cast<float>(value->getWidth());
        const int room = plate->getWidth() - 2 * g_barInset;

        if (whole < 1.0f || room < 1)
            return;

        MyGUI::Widget* fill = FindTagged(row, TAG_FILL);

        if (fill == NULL)
        {
            fill = CreateFill(row, bar);

            if (g_debug)
            {
                char note[320];
                sprintf_s(note,
                    "DarkUiTweaks: bar fill created - row %d, plate %d at %d, "
                    "game 100%% %.0f (value text width), frame %d, "
                    "game bar %d at %d",
                    row->getWidth(), plate->getWidth(), plate->getLeft(),
                    whole, room, bar->getWidth(), bar->getLeft());
                DebugLog(note);
            }
        }

        // Игра пишет в свою полосу - пусть пишет, но видно её не будет.
        if (bar->getAlpha() != 0.0f)
            bar->setAlpha(0.0f);

        const int width =
            static_cast<int>(BarShare(bar, whole, fill) * room + 0.5f);
        const bool shown = bar->getVisible() && width > 0;

        if (fill->getVisible() != shown)
            fill->setVisible(shown);

        // По высоте - от рамки, а не от доли в разметке: доля растёт
        // вместе со строкой, а рамка скина всегда одной толщины. Строка
        // ниже двух рамок - берём высоту игровой полосы как есть.
        int top = plate->getTop() + FRAME_PX;
        int height = plate->getHeight() - 2 * FRAME_PX;

        if (height < 2)
        {
            top = bar->getTop();
            height = bar->getHeight();
        }

        const MyGUI::IntCoord want(plate->getLeft() + g_barInset,
                                   top, width, height);

        if (fill->getCoord() != want)
            fill->setCoord(want);
    }


    void AlignBars(MyGUI::Widget* widget, int depth)
    {
        if (depth > 8)
            return;

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = widget->getChildAt(i);

            if (EndsWith(child->getName(), "_Progress"))
                AlignBar(widget, child);
            else
                AlignBars(child, depth + 1);
        }
    }


    // ---------------------------------------------------------------
    // Высота ячейки отряда
    //
    // Ячейка отряда - Kenshi_SquadBox.layout: имя отряда и кнопки
    // сверху, под ними рамка с ящиком портретов PortraitsPanel, и рамка,
    // и ящик растягиваются вместе с ячейкой (align Stretch). Значит,
    // каждый пиксель высоты ячейки - пиксель ящику портретов.
    //
    // Сколько именно прибавить, заранее не посчитать: доли разметки
    // считаются от размера при загрузке, а растяжение потом прибавляет
    // разницу. Поэтому меряем по месту: высота ящика портретов против
    // нужных рядов (ряд - высота живого портрета), и недостающее
    // добавляем к прибавке. В игре сошлось за один шаг: ячейка 252 -> 303,
    // ящик портретов 161 -> 212, два ряда по 106.
    // ---------------------------------------------------------------

    const char* const TAG_SQUADS = "DarkUiTweaksSquads";

    int g_squadExtra = 0;         // прибавка к высоте ячейки отряда
    int g_squadCellHeight = 0;    // высота, которую мы отдали последней
    int* g_squadHeightVar = NULL; // SquadCellView::Height в памяти игры
    bool g_squadHeightTried = false;
    int g_squadBase = 0;          // высота, которую задала сама игра
    int g_squadLastHave = -1;     // ящик портретов до последней прибавки
    bool g_squadStuck = false;    // прибавка не доходит до портретов

    // Портретов в ряду - наименьшее, что видели при этой ширине ящика.
    // Полоса прокрутки портретов то появляется, то пропадает и отнимает
    // у ряда место; если бы ряд считался по текущей ширине, на границе
    // ячейка росла бы (полоса пропала, в ряд влез лишний портрет),
    // потом сжималась (полоса вернулась) - и так по кругу.
    int g_perRowWidth = 0;
    int g_perRow = 0;

    const int MAX_SQUAD_ROWS = 6;


    void SquadNote(const std::string& note);


    // Пересчитать ячейки ящика. setVerticalAlignment с тем же значением
    // здесь ничего не делает - журнал: переменную высоты переписали на
    // 303, а ячейки остались 252. Переключаем туда и обратно: каждое
    // переключение пересчитывает размер ячеек, а кадр между ними не
    // рисуется.
    void RefreshCells(MyGUI::ItemBox* box)
    {
        const bool vertical = box->getVerticalAlignment();
        box->setVerticalAlignment(!vertical);
        box->setVerticalAlignment(vertical);
    }


    bool IsSquadBox(const std::string& name)
    {
        return EndsWith(name, "SquadsPanel");
    }


    // Откуда игра берёт высоту ячейки.
    //
    // Попытка 1 - подменить делегат requestCoordItem, как у очередей.
    // Журнал: делегат не вызывался ни разу («asked for 0»).
    // Попытка 2 - перехватить SquadCellView::getCellDimension (она
    // экспортирована). Перехват встал, но тоже ни одного вызова: функция
    // в четыре инструкции, и в посредника BaseItemBox::requestCoordWidgetItem
    // компилятор её встроил. Отдельная копия в exe есть, но её никто не зовёт.
    //
    // Зато обе копии читают одну и ту же статическую переменную
    // SquadCellView::Height. Её адрес KenshiLib не отдаёт, но он есть в
    // коде самой getCellDimension: «mov eax, [rip+X]; mov [rdx+0Ch], eax» -
    // взять Height и положить в coord.height (четвёртое поле IntCoord).
    // Код читаем в памяти, по настоящему адресу функции, - файл exe
    // тут не помощник (он сдвинут относительно памяти).
    int* FindSquadHeightVar()
    {
        typedef void (*CellDimensionFn)(MyGUI::Widget*, MyGUI::IntCoord&,
                                        bool);
        const CellDimensionFn function =
            &SquadManagementScreen::SquadCellView::getCellDimension;

        const unsigned char* const code =
            reinterpret_cast<const unsigned char*>(
                KenshiLib::GetRealAddress(function));
        if (code == NULL)
            return NULL;

        const int SPAN = 64;

        if (g_debug)
        {
            std::string bytes;
            char hex[4];
            for (int i = 0; i < SPAN; ++i)
            {
                sprintf_s(hex, "%02X ", code[i]);
                bytes += hex;
            }
            DebugLog("DarkUiTweaks: squads - getCellDimension code: " + bytes);
        }

        for (int i = 0; i + 9 <= SPAN; ++i)
        {
            // mov r32, [rip+rel32]: 8B, modrm с mod=00 и rm=101.
            if (code[i] != 0x8B || (code[i + 1] & 0xC7) != 0x05)
                continue;

            const int reg = (code[i + 1] >> 3) & 7;
            const int rel = *reinterpret_cast<const int*>(code + i + 2);
            const unsigned char* const next = code + i + 6;

            // Следом mov [rdx+0Ch], r32: 89, modrm = 01 reg 010, disp8 0C.
            if (next[0] == 0x89 && next[1] == (0x42 | (reg << 3)) &&
                next[2] == 0x0C)
            {
                return reinterpret_cast<int*>(
                    const_cast<unsigned char*>(next + rel));
            }
        }

        return NULL;
    }


    // Держим в переменной «игровая высота + прибавка». Если игра сама
    // переписала её (setCellSize при смене разрешения), это её новая
    // высота - к ней и прибавляем. Возвращает true, если высоту поменяли
    // и ящику надо пересчитать ячейки.
    bool ApplySquadHeight()
    {
        int& height = *g_squadHeightVar;

        if (height != g_squadCellHeight)
            g_squadBase = height;

        const int wanted = g_squadBase + g_squadExtra;
        g_squadCellHeight = wanted;

        if (height == wanted)
            return false;

        height = wanted;
        return true;
    }


    // Журнал подбора: пишем, только когда что-то изменилось, - иначе
    // покадровая проверка засыпала бы его одной и той же строкой.
    void SquadNote(const std::string& note)
    {
        static std::string last;
        if (!g_debug || note == last)
            return;

        last = note;
        DebugLog("DarkUiTweaks: squads - " + note);
    }


    // Ящик портретов внутри ячейки отряда.
    MyGUI::ItemBox* FindPortraits(MyGUI::Widget* widget, int depth)
    {
        if (depth > 8)
            return NULL;

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = widget->getChildAt(i);

            MyGUI::ItemBox* const box = child->castType<MyGUI::ItemBox>(false);
            if (box != NULL && EndsWith(box->getName(), "PortraitsPanel"))
                return box;

            MyGUI::ItemBox* const found = FindPortraits(child, depth + 1);
            if (found != NULL)
                return found;
        }

        return NULL;
    }


    // Высота ряда портретов: по живому портрету, если он есть, иначе -
    // что скажет игра.
    int PortraitPitch(MyGUI::ItemBox* portraits)
    {
        if (portraits->getItemCount() > 0)
        {
            MyGUI::Widget* const first = portraits->getWidgetByIndex(0);
            if (first != NULL && first->getHeight() > 0)
                return first->getHeight();
        }

        MyGUI::IntCoord coord;
        SquadManagementScreen::PortraitSquadCellView::getCellDimension(
            portraits, coord, false);
        return coord.height;
    }


    // Бойцов в отряде по номеру строки списка - из данных отряда, а не
    // из виджетов: ячейки строятся только для видимых отрядов, а высота
    // нужна под все, включая прокрученные за край.
    int SquadMembers(MyGUI::ItemBox* squads, size_t index)
    {
        SquadManagementScreen::SquadData** const data =
            squads->getItemDataAt<SquadManagementScreen::SquadData*>(index,
                                                                     false);
        if (data == NULL || *data == NULL || (*data)->platoon == NULL)
            return 0;

        return static_cast<int>((*data)->platoon->things.size());
    }


    int PortraitsPerRow(MyGUI::ItemBox* portraits)
    {
        int width = 0;
        if (portraits->getItemCount() > 0)
        {
            MyGUI::Widget* const first = portraits->getWidgetByIndex(0);
            if (first != NULL)
                width = first->getWidth();
        }

        if (width <= 0)
        {
            MyGUI::IntCoord coord;
            SquadManagementScreen::PortraitSquadCellView::getCellDimension(
                portraits, coord, false);
            width = coord.width;
        }

        if (width <= 0)
            return 0;

        int perRow = portraits->getClientCoord().width / width;
        if (perRow < 1)
            perRow = 1;

        if (portraits->getWidth() != g_perRowWidth)
        {
            g_perRowWidth = portraits->getWidth();
            g_perRow = perRow;
        }
        else if (perRow < g_perRow)
        {
            g_perRow = perRow;
        }

        return g_perRow;
    }


    // Сколько рядов держать; biggest и perRow - для журнала.
    int WantedRows(MyGUI::ItemBox* squads, MyGUI::ItemBox* portraits,
                   int* biggestOut, int* perRowOut)
    {
        int rows = g_squadRows;
        if (!g_squadAutoGrow)
            return rows;

        const int perRow = PortraitsPerRow(portraits);
        if (perRow <= 0)
            return rows;

        int biggest = 0;
        const size_t count = squads->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            const int members = SquadMembers(squads, i);
            if (members > biggest)
                biggest = members;
        }

        const int needed = (biggest + perRow - 1) / perRow;
        if (needed > rows)
            rows = needed;
        if (rows > MAX_SQUAD_ROWS)
            rows = MAX_SQUAD_ROWS;

        *biggestOut = biggest;
        *perRowOut = perRow;
        return rows;
    }


    // Корень разметки ячейки (Root в Kenshi_SquadBox.layout) за ячейкой
    // не тянется: у него нет выравнивания, и размер он берёт один раз,
    // при создании. Журнал: ячейка 303, а ящик портретов внутри всё те
    // же 162. Поэтому корень каждой видимой ячейки подгоняем сами -
    // рамка и ящик портретов внутри него растянутые и пойдут следом.
    // Возвращает true, если хоть один корень поменялся.
    bool FitCellRoots(MyGUI::ItemBox* squads)
    {
        bool changed = false;

        const size_t count = squads->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const cell = squads->getWidgetByIndex(i);
            if (cell == NULL)
                continue;

            bool found = false;
            const size_t children = cell->getChildCount();
            for (size_t c = 0; c < children && !found; ++c)
            {
                MyGUI::Widget* const root = cell->getChildAt(c);
                if (!EndsWith(root->getName(), "Root"))
                    continue;

                found = true;
                if (root->getHeight() != cell->getHeight())
                {
                    root->setSize(root->getWidth(), cell->getHeight());
                    changed = true;
                }
            }

            if (!found && g_debug)
            {
                std::string names;
                for (size_t c = 0; c < children; ++c)
                    names += " '" + cell->getChildAt(c)->getName() + "'";
                SquadNote("no Root in a cell, children:" + names);
            }
        }

        return changed;
    }


    void FitSquadCells(MyGUI::ItemBox* squads)
    {
        // Новый ящик (загрузка сохранения) - пересчитать ему ячейки,
        // чтобы спросил размер уже через перехват.
        if (!Tagged(squads, TAG_SQUADS))
        {
            Tag(squads, TAG_SQUADS);
            RefreshCells(squads);

            SquadNote("found '" + squads->getName() + "'");
            return;   // мерить - со следующего кадра, по новым ячейкам
        }

        if (!squads->getVisible() || g_squadStuck)
            return;

        char note[256];

        if (!g_squadHeightTried)
        {
            g_squadHeightTried = true;
            g_squadHeightVar = FindSquadHeightVar();
            if (g_squadHeightVar == NULL)
            {
                ErrorLog("DarkUiTweaks: squad cell height not found in "
                         "the game code, squad cells stay as they are");
            }
        }

        if (g_squadHeightVar == NULL)
            return;

        // Виджет ячейки - тот, которому ящик ставит размер.
        if (squads->getItemCount() == 0)
        {
            SquadNote("no squads");
            return;
        }

        // Ячейки есть только у видимых отрядов: список мог быть
        // прокручен, и первый отряд - за краем.
        MyGUI::Widget* cell = NULL;
        const size_t count = squads->getItemCount();
        for (size_t i = 0; i < count && cell == NULL; ++i)
            cell = squads->getWidgetByIndex(i);

        if (cell == NULL)
        {
            SquadNote("no squad cell shown");
            return;
        }

        if (g_squadBase == 0)
        {
            const int value = *g_squadHeightVar;
            if (value != cell->getHeight())
            {
                sprintf_s(note, "found height %d, but the cell is %d high - "
                          "wrong variable, leaving it alone",
                          value, cell->getHeight());
                SquadNote(note);
                ErrorLog(std::string("DarkUiTweaks: ") + note);
                g_squadHeightVar = NULL;
                return;
            }

            g_squadBase = value;
            g_squadCellHeight = value;
        }

        if (ApplySquadHeight())
        {
            RefreshCells(squads);
            return;
        }

        if (cell->getHeight() != g_squadCellHeight)
        {
            sprintf_s(note, "cell %d high, asked for %d - waiting",
                      cell->getHeight(), g_squadCellHeight);
            SquadNote(note);
            return;
        }

        // Ячейки создаются при прокрутке и пересоздаются игрой - корни
        // проверяем каждый кадр; поменяли - мерить со следующего.
        if (FitCellRoots(squads))
        {
            SquadNote("cell roots resized");
            return;
        }

        MyGUI::ItemBox* const portraits = FindPortraits(cell, 0);
        if (portraits == NULL)
        {
            SquadNote("no PortraitsPanel in the cell");
            return;
        }

        const int pitch = PortraitPitch(portraits);
        const int have = portraits->getClientCoord().height;
        int biggest = 0;
        int perRow = 0;
        const int rows = WantedRows(squads, portraits, &biggest, &perRow);
        const int need = rows * pitch;

        // Одной строкой: SquadNote гасит только повтор подряд, и две
        // строки через раз забили бы журнал (6000 строк за полминуты).
        sprintf_s(note,
                  "biggest squad %d, %d per row - %d rows; cell %d high "
                  "(extra %d), portraits box %d high (client %d), row %d, "
                  "need %d",
                  biggest, perRow, rows, cell->getHeight(), g_squadExtra,
                  portraits->getHeight(), have, pitch, need);
        SquadNote(note);

        if (pitch <= 0)
            return;

        if (have == need)
        {
            g_squadLastHave = -1;   // сошлось - следующий подбор с нуля
            return;
        }

        // Прибавили, ячейка выросла, а ящик портретов нет - значит,
        // растяжение до него не доходит, и подбирать дальше бессмысленно:
        // только раздули бы ячейку. Откатываемся к игровой высоте.
        if (g_squadLastHave >= 0 && have == g_squadLastHave)
        {
            ErrorLog("DarkUiTweaks: squad portraits do not grow with the "
                     "cell, leaving the game's height");
            g_squadStuck = true;
            g_squadExtra = 0;
            if (ApplySquadHeight())
                RefreshCells(squads);
            return;
        }

        int extra = g_squadExtra + need - have;
        if (extra < 0)
            extra = 0;
        if (extra > 600)
            extra = 600;
        if (extra == g_squadExtra)
            return;

        g_squadExtra = extra;
        g_squadLastHave = have;
        if (ApplySquadHeight())
            RefreshCells(squads);
    }


    // Полоса в описании навыка - раздел «Полоса в строке панели
    // описания» ниже.
    //
    // Панель описания окна навыков (Kenshi_StatsWindow.layout).
    // Окончание с подчёркиванием: у исследований есть
    // «ResearchDescription2Panel», её не трогаем. Без префикса BaseLayout
    // (окно могло грузить разметку напрямую) - имя целиком, без «_».
    const char* const STATS_DESCRIPTION = "_Description2Panel";

    void StyleDescriptionBars(MyGUI::Widget* panel);


    void WalkWidgets(MyGUI::Widget* widget, int depth)
    {
        if (widget == NULL || depth > 24)
            return;

        // Скрытое не рисуется - и поправлять его незачем. Без этого обход
        // каждый кадр шёл по всем окнам игры, в том числе закрытым, а это
        // тысячи виджетов. Окно, которое покажут, поправим в том же кадре
        // или в следующем.
        if (!widget->getVisible())
            return;

        MyGUI::ItemBox* const box = widget->castType<MyGUI::ItemBox>(false);
        if (box != NULL && IsQueueBox(box->getName()))
        {
            if (g_rowHeight > 0)
                SetRowHeight(box);

            // Строки создаются и пропадают, а долю игра переписывает -
            // поэтому не один раз, а каждый кадр.
            if (g_barInset > 0)
                AlignBars(box, 0);

            // Внутри ящика очереди других очередей нет.
            return;
        }

        if (g_progressPad > 0 &&
            (EndsWith(widget->getName(), STATS_DESCRIPTION) ||
             widget->getName() == STATS_DESCRIPTION + 1))
        {
            StyleDescriptionBars(widget);
            return;
        }

        if (box != NULL && g_squadRows > 0 && IsSquadBox(box->getName()))
        {
            FitSquadCells(box);
            return;
        }

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
            WalkWidgets(widget->getChildAt(i), depth + 1);
    }


    // Высота строк, заливка и ячейки отрядов включаются каждая своей
    // настройкой; обход нужен, пока включена хоть одна.
    void PatchQueueBoxes()
    {
        if (g_rowHeight <= 0 && g_barInset <= 0 && g_squadRows <= 0 &&
            g_progressPad <= 0)
        {
            return;
        }

        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
            return;

        MyGUI::EnumeratorWidgetPtr roots = gui->getEnumerator();
        while (roots.next())
            WalkWidgets(roots.current(), 0);
    }


    // ---------------------------------------------------------------
    // Полоса в строке панели описания
    //
    // «Следующий уровень» - строка того же вида, что в списке
    // исследований (DataPanelLine_Research). Журнал с деревом панели:
    //
    //   Button   734x16  - подложка с чёрной каймой
    //   Widget   143x8   - заливка игры: слева +7, сверху +3,
    //                      ширина = доля * (подложка - 2*7)
    //   TextBox  704x16  - «Следующий уровень:»
    //   TextBox  704x16  - «20%»
    //   EditBox  ...     - строки ниже, с шагом 16-22
    //
    // ProgressBar в ней нет, поэтому первые попытки (перехват createMe у
    // трёх классов строк и поиск ProgressBar) ничего не находили.
    //
    // Что делаем, каждый кадр, со всеми детьми холста панели:
    //   подложка - выше на 2*pad;
    //   подпись и процент - ниже на pad, по середине подложки;
    //   заливка - внутрь каймы, на всю новую высоту;
    //   подложка с заливкой - ещё ниже на drop: шрифт сидит в своей
    //   рамке высоко, и по середине текст выглядит только так;
    //   верх подложки с заливкой - выше ещё на up, низ на месте;
    //   всё, что ниже строки, - ниже на 2*pad + drop.
    //
    // Заливку двигаем игровую, а не ставим свою: у неё и цвет, и место в
    // порядке отрисовки (под надписями). Игра переписывает эти виджеты -
    // при смене процента, при смене навыка, - поэтому у каждого храним,
    // что поставила игра (база) и что поставили мы. Не совпало с нашим -
    // значит, писала игра, и это новая база. Так и доля заливки всегда
    // берётся из ширины, которую поставила игра.
    // ---------------------------------------------------------------

    const char* const TAG_DUMPED = "DarkUiTweaksDumped";
    const char* const KEY_BASE = "DarkUiTweaksBase";   // координаты от игры
    const char* const KEY_SET = "DarkUiTweaksSet";     // координаты от нас

    // Отступы игровой заливки от подложки - из журнала (слева +7).
    const int GAME_FILL_X = 7;

    // Кайма подложки - та же, что у строки очереди исследований
    // (FRAME_PX сверху и снизу, QueueBarInset по бокам). Первая версия
    // брала отступы игры (7 слева, 3 сверху) - по снимку заливка легла
    // поверх верхней и нижней чёрной линии, а слева между линией и
    // заливкой остался серый кусок подложки.


    std::string CoordText(const MyGUI::IntCoord& c)
    {
        char text[64];
        sprintf_s(text, "%d %d %d %d", c.left, c.top, c.width, c.height);
        return text;
    }


    MyGUI::IntCoord ParseCoord(const std::string& text)
    {
        MyGUI::IntCoord c;
        sscanf_s(text.c_str(), "%d %d %d %d", &c.left, &c.top, &c.width,
                 &c.height);
        return c;
    }


    // Координаты, которые поставила игра: если сейчас стоят наши - база
    // из памяти, иначе текущие (игра их переписала) и они новая база.
    MyGUI::IntCoord BaseCoord(MyGUI::Widget* widget)
    {
        const std::string set = widget->getUserString(KEY_SET);
        if (!set.empty() && set == CoordText(widget->getCoord()))
            return ParseCoord(widget->getUserString(KEY_BASE));

        const MyGUI::IntCoord base = widget->getCoord();
        widget->setUserString(KEY_BASE, CoordText(base));
        widget->setUserString(KEY_SET, "");
        return base;
    }


    void PlaceWidget(MyGUI::Widget* widget, const MyGUI::IntCoord& want)
    {
        if (widget->getCoord() != want)
            widget->setCoord(want);
        widget->setUserString(KEY_SET, CoordText(want));
    }


    bool IsText(MyGUI::Widget* widget)
    {
        return widget->castType<MyGUI::TextBox>(false) != NULL;
    }


    bool IsPlate(MyGUI::Widget* widget)
    {
        return widget->castType<MyGUI::Button>(false) != NULL;
    }


    bool Inside(const MyGUI::IntCoord& inner, const MyGUI::IntCoord& outer)
    {
        return inner.left >= outer.left && inner.top >= outer.top &&
               inner.left + inner.width <= outer.left + outer.width &&
               inner.top + inner.height <= outer.top + outer.height;
    }


    struct BarRow
    {
        MyGUI::Widget* plate;
        MyGUI::IntCoord plateBase;
        MyGUI::Widget* fill;
    };


    std::string DescribeWidget(const char* what, MyGUI::Widget* widget)
    {
        if (widget == NULL)
            return std::string(" ") + what + " none";

        char text[160];
        sprintf_s(text, " %s %d,%d %dx%d (parent %p)", what,
                  widget->getAbsoluteLeft(), widget->getAbsoluteTop(),
                  widget->getWidth(), widget->getHeight(),
                  static_cast<void*>(widget->getParent()));
        return text;
    }


    void BarNote(const std::string& note)
    {
        static std::string last;
        if (!g_debug || note == last)
            return;

        last = note;
        DebugLog("DarkUiTweaks: bar - " + note);
    }


    // Холст панели: виджет, среди детей которого есть подложка с
    // заливкой внутри.
    void StyleCanvas(MyGUI::Widget* canvas)
    {
        const size_t count = canvas->getChildCount();

        std::vector<MyGUI::Widget*> children(count);
        std::vector<MyGUI::IntCoord> bases(count);
        for (size_t i = 0; i < count; ++i)
        {
            children[i] = canvas->getChildAt(i);
            bases[i] = BaseCoord(children[i]);
        }

        // Строки с полосой: подложка и заливка внутри неё (по базе).
        std::vector<BarRow> rows;
        for (size_t i = 0; i < count; ++i)
        {
            if (!IsPlate(children[i]) || !children[i]->getVisible())
                continue;

            for (size_t j = 0; j < count; ++j)
            {
                if (j == i || IsText(children[j]) || IsPlate(children[j]))
                    continue;

                if (Inside(bases[j], bases[i]))
                {
                    BarRow row = { children[i], bases[i], children[j] };
                    rows.push_back(row);
                    break;
                }
            }
        }

        if (rows.empty())
            return;

        const int grow = 2 * g_progressPad;
        const int drop = g_progressDrop;
        const int up = g_progressGrowUp;
        int lowest = 0;

        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::Widget* const child = children[i];
            const MyGUI::IntCoord& base = bases[i];

            // Сколько строк с полосой выше - на столько и вниз.
            int shift = 0;
            const BarRow* own = NULL;
            for (size_t r = 0; r < rows.size(); ++r)
            {
                const MyGUI::IntCoord& plate = rows[r].plateBase;
                if (plate.top + plate.height <= base.top)
                    shift += grow + (drop > 0 ? drop : 0);
                else if (base.top >= plate.top &&
                         base.top < plate.top + plate.height)
                {
                    own = &rows[r];
                }
            }

            MyGUI::IntCoord want = base;
            want.top += shift;

            if (own != NULL && child == own->plate)
            {
                want.top += drop - up;
                want.height += grow + up;
            }
            else if (own != NULL && child == own->fill)
            {
                const MyGUI::IntCoord& plate = own->plateBase;
                const int gameRoom = plate.width - 2 * GAME_FILL_X;
                float share = gameRoom > 0
                    ? static_cast<float>(base.width) / gameRoom : 0.0f;
                if (share < 0.0f)
                    share = 0.0f;
                if (share > 1.0f)
                    share = 1.0f;

                want.left = plate.left + g_barInset;
                want.top = plate.top + shift + drop - up + FRAME_PX;
                want.width = static_cast<int>(
                    share * (plate.width - 2 * g_barInset) + 0.5f);
                want.height = plate.height + grow + up - 2 * FRAME_PX;
            }
            else if (own != NULL && IsText(child))
            {
                want.top += g_progressPad;
            }

            PlaceWidget(child, want);

            if (child->getVisible() && want.top + want.height > lowest)
                lowest = want.top + want.height;
        }

        // Строки ушли вниз - холст прокрутки должен их вместить.
        MyGUI::ScrollView* const view = canvas->getParent() != NULL
            ? canvas->getParent()->castType<MyGUI::ScrollView>(false)
            : NULL;
        if (view != NULL)
        {
            const MyGUI::IntSize size = view->getCanvasSize();
            if (lowest > size.height)
                view->setCanvasSize(size.width, lowest);
        }

        if (g_debug)
        {
            const BarRow& row = rows[0];
            BarNote("plate" + DescribeWidget("", row.plate) + ", fill" +
                    DescribeWidget("", row.fill) + ", game plate " +
                    CoordText(row.plateBase));
        }
    }


    bool FindCanvases(MyGUI::Widget* widget, int depth)
    {
        if (depth > 6)
            return false;

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (IsPlate(widget->getChildAt(i)))
            {
                StyleCanvas(widget);
                return true;
            }
        }

        bool found = false;
        for (size_t i = 0; i < count; ++i)
            found |= FindCanvases(widget->getChildAt(i), depth + 1);
        return found;
    }


    // Для журнала: всё дерево панели один раз - тип, имя, место, текст.
    void DumpTree(MyGUI::Widget* widget, int depth, int& budget)
    {
        if (budget <= 0 || depth > 12)
            return;
        --budget;

        std::string line(static_cast<size_t>(depth) * 2, ' ');
        line += widget->getTypeName() + " '" + widget->getName() + "'";
        line += DescribeWidget("at", widget);

        MyGUI::TextBox* const text = widget->castType<MyGUI::TextBox>(false);
        if (text != NULL)
            line += " text '" + text->getCaption().asUTF8() + "'";
        if (!widget->getVisible())
            line += " hidden";

        DebugLog("DarkUiTweaks: tree " + line);

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
            DumpTree(widget->getChildAt(i), depth + 1, budget);
    }


    void StyleDescriptionBars(MyGUI::Widget* panel)
    {
        const bool found = FindCanvases(panel, 0);

        if (g_debug && found && !Tagged(panel, TAG_DUMPED))
        {
            Tag(panel, TAG_DUMPED);
            int budget = 150;
            DumpTree(panel, 0, budget);
        }
    }


    // ---------------------------------------------------------------
    // Прокрутка в описании
    // ---------------------------------------------------------------

    // Панели описания по имени. Вокруг их перезаполнения удерживается
    // прокрутка, а для этого нужен указатель на саму панель - и он
    // опасен: панели пересоздаются (в журнале 19 созданий за сессию), и
    // старые при этом уничтожаются. Раньше тут было множество, которое
    // только росло, и перестройка описания обходила в нём уже удалённые
    // панели. Теперь на каждое имя одна запись, и новое создание её
    // перезаписывает - в карте всегда последняя, живая панель.
    typedef std::map<std::string, DatapanelGUI*> PanelMap;
    PanelMap g_descriptionPanels;

    DatapanelGUI* (*g_origCreateDatapanel)(
        ForgottenGUI* thisptr, const std::string& name,
        MyGUI::Widget* win, bool scrolls) = NULL;


    bool WantsScroll(const std::string& name)
    {
        // Панели описания на вкладках «Изучение» и «Ремесло».
        return name.find("DescriptionPanel") != std::string::npos;
    }


    DatapanelGUI* CreateDatapanel_hook(ForgottenGUI* thisptr,
                                       const std::string& name,
                                       MyGUI::Widget* win, bool scrolls)
    {
        bool wanted = scrolls;

        if (g_scrollDescription && !scrolls && WantsScroll(name))
            wanted = true;

        if (g_debug)
        {
            char note[256];
            sprintf_s(note, "DarkUiTweaks: datapanel '%s' scrolls %d -> %d",
                      name.c_str(), scrolls ? 1 : 0, wanted ? 1 : 0);
            DebugLog(note);
        }

        DatapanelGUI* const panel =
            g_origCreateDatapanel(thisptr, name, win, wanted);

        if (WantsScroll(name))
        {
            if (panel != NULL)
                g_descriptionPanels[name] = panel;
            else
                g_descriptionPanels.erase(name);
        }

        return panel;
    }


    // ---------------------------------------------------------------
    // Удержание прокрутки
    //
    // Игра перезаполняет описание, и прокрутка каждый раз прыгает
    // наверх - видно как «взял ползунок, а его сбросило». Панель при
    // этом НЕ пересоздаётся (в журнале её создание встретилось 19 раз
    // за сессию, а не каждый кадр), поэтому достаточно запомнить
    // смещение до перезаполнения и вернуть после.
    // ---------------------------------------------------------------

    void (*g_origRefreshDescription)(void* thisptr) = NULL;


    void RefreshDescription_hook(void* thisptr)
    {
        // Пока мышь захвачена - что-то тянут, и очень может быть, что
        // как раз ползунок описания. Перестройка уничтожает виджеты
        // заново, захват при этом теряется, и перетаскивание срывается:
        // колесом прокручивалось, а ползунком нет. Поэтому на время
        // перетаскивания перестройку пропускаем целиком.
        MyGUI::InputManager* const input =
            MyGUI::InputManager::getInstancePtr();

        if (input != NULL && input->isCaptureMouse())
            return;

        // Смещения всех панелей описания до перестройки.
        std::vector<std::pair<MyGUI::ScrollView*, MyGUI::IntPoint> > saved;

        for (PanelMap::iterator it = g_descriptionPanels.begin();
             it != g_descriptionPanels.end(); ++it)
        {
            MyGUI::ScrollView* const view = it->second->scrollView;
            if (view != NULL)
                saved.push_back(std::make_pair(view, view->getViewOffset()));
        }

        g_origRefreshDescription(thisptr);

        for (size_t i = 0; i < saved.size(); ++i)
        {
            // Содержимое могло стать короче - MyGUI сам прижмёт
            // смещение к допустимому.
            if (saved[i].second.top != 0 || saved[i].second.left != 0)
                saved[i].first->setViewOffset(saved[i].second);
        }
    }


    // ---------------------------------------------------------------
    // Отношение фракции у имени выбранного персонажа
    // ---------------------------------------------------------------
    //
    // Плашка над панелью выбранного (NamePanel/NameText): слева имя,
    // справа фракция. Дописываем к фракции её отношение к игроку - как в
    // окне фракций игры: «Кочевники (+15)». Скобки серые, число зелёным -
    // выше нуля, красным - от -10 и ниже (там уже нападают); между -10 и 0
    // - цветом надписи, по таким городам ещё можно ходить.
    //
    // Игра переписывает надпись сама (при смене выбранного и т.п.), мы -
    // после её кадра: запоминаем её текст (base) и ставим base + приписку,
    // только если надпись отличается от нужной.

    WidgetRef* g_namePanelRef = new WidgetRef();    // не удаляется: см. WidgetRef.h
    WidgetRef* g_factionTextRef = new WidgetRef();
    DWORD g_namePanelSearchMs = 0;
    std::string g_factionBase;       // текст игры без нашей приписки
    std::string g_factionShown;      // что мы поставили последним

    MyGUI::Widget* FindByNameSuffix(MyGUI::Widget* widget, const std::string& suffix, int depth)
    {
        if (widget == NULL || depth > 12)
            return NULL;
        if (EndsWith(widget->getName(), suffix))
            return widget;
        for (size_t i = 0; i < widget->getChildCount(); ++i)
            if (MyGUI::Widget* found = FindByNameSuffix(widget->getChildAt(i), suffix, depth + 1))
                return found;
        return NULL;
    }

    // Самая правая непустая надпись внутри плашки - фракция.
    void CollectTexts(MyGUI::Widget* widget, std::vector<MyGUI::TextBox*>& out, int depth)
    {
        if (widget == NULL || depth > 5)
            return;
        if (MyGUI::TextBox* text = widget->castType<MyGUI::TextBox>(false))
            out.push_back(text);
        for (size_t i = 0; i < widget->getChildCount(); ++i)
            CollectTexts(widget->getChildAt(i), out, depth + 1);
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
            if (MyGUI::Widget* found = FindByNameSuffix(roots.current(), "NameText", 0))
            {
                g_namePanelRef->set(found);
                if (g_debug)
                    DebugLog(("DarkUiTweaks: name plate found: " + found->getName()).c_str());
                return found;
            }
        }
        return NULL;
    }

    // Отношение фракции выбранного к игроку; false - не показывать
    // (никто не выбран, свой отряд, нет записи об отношениях).
    bool SelectedRelation(int& relation)
    {
        if (ou == NULL || ou->player == NULL)
            return false;
        RootObject* selected = ou->player->selectedObject.getRootObject();
        if (selected == NULL)
            return false;
        Faction* faction = selected->getFaction();
        Faction* mine = ou->player->getFaction();
        if (faction == NULL || mine == NULL || faction == mine || faction->relations == NULL)
            return false;
        FactionRelations::RelationData* data = faction->relations->getRelationData(mine);
        if (data == NULL)
            return false;
        const float value = data->relation;
        relation = static_cast<int>(value < 0.0f ? value - 0.5f : value + 0.5f);
        return true;
    }

    void RestoreFactionText()
    {
        MyGUI::Widget* widget = g_factionTextRef->get();
        MyGUI::TextBox* text = widget == NULL ? NULL : widget->castType<MyGUI::TextBox>(false);
        if (text != NULL && !g_factionShown.empty() && text->getCaption().asUTF8() == g_factionShown)
            text->setCaption(g_factionBase);
        g_factionShown.clear();
    }

    MyGUI::UString g_factionShownU;     // то же, что g_factionShown, - сравнивать без выделения памяти
    DWORD g_factionFullAt = 0;

    void UpdateFactionRelation()
    {
        MyGUI::Widget* panel = g_factionRelation ? NamePanel() : NULL;
        if (panel == NULL || !panel->getInheritedVisible())
        {
            if (!g_factionRelation)
                RestoreFactionText();
            return;
        }

        // Быстрый путь (ревью 08.10.2026): наша надпись на месте и игра её
        // не переписала - полный разбор (обход панели, перевод строк, поиск
        // отношения) не чаще двух раз в секунду, а не каждый кадр.
        const DWORD now = GetTickCount();
        if (MyGUI::Widget* const known = g_factionTextRef->get())
        {
            MyGUI::TextBox* const text = known->castType<MyGUI::TextBox>(false);
            if (text != NULL && !g_factionShownU.empty() && text->getCaption() == g_factionShownU &&
                now - g_factionFullAt < 500)
                return;
        }
        g_factionFullAt = now;

        std::vector<MyGUI::TextBox*> texts;
        CollectTexts(panel, texts, 0);
        MyGUI::TextBox* faction = NULL;
        int factionLeft = -1;
        for (size_t i = 0; i < texts.size(); ++i)
        {
            if (texts[i]->getCaption().empty())
                continue;
            const int left = texts[i]->getAbsoluteLeft();
            if (left > factionLeft)
            {
                factionLeft = left;
                faction = texts[i];
            }
        }
        if (faction == NULL || texts.size() < 2)
            return;                     // одна надпись - фракции нет
        if (g_factionTextRef->get() != faction)
        {
            g_factionTextRef->set(faction);
            g_factionShown.clear();
        }

        const std::string current = faction->getCaption().asUTF8();
        if (current != g_factionShown)
            g_factionBase = current;    // игра переписала - это её текст

        int relation = 0;
        std::string want = g_factionBase;
        if (SelectedRelation(relation))
        {
            // Скобки серые, цветное - только число (просьба 07.10.2026).
            // Нейтральное число - цветом обычного текста из палитры игры:
            // getTextColour() у этой надписи чёрный (цвет даёт скин), и
            // число в скобках выходило чёрным.
            char own[8];
            const MyGUI::Colour main = GameTheme::ReadableColour("Main", "#AFA68B");
            sprintf_s(own, "#%02X%02X%02X", static_cast<int>(main.red * 255.0f + 0.5f),
                      static_cast<int>(main.green * 255.0f + 0.5f), static_cast<int>(main.blue * 255.0f + 0.5f));
            const char* colour = relation > 0 ? "#54DB8C" : (relation <= -10 ? "#F25447" : own);
            char tail[64];
            sprintf_s(tail, " #8C8C8C(%s%+d#8C8C8C)", colour, relation);
            want += tail;
        }
        if (current != want)
        {
            faction->setCaption(want);
            g_factionShown = want;
        }
        g_factionShownU = faction->getCaption();
    }


    // Глаз на изученных чертежах 09.10.2026 переехал в ItemMarkers (там все
    // пометки на иконках вещей) - перехвата InventoryIcon здесь больше нет.

    // ---------------------------------------------------------------
    // Иконка изготавливаемой вещи (08.10.2026, просьба пользователя)
    //
    // Во вкладке «Ремесло» в списке «Предметы» одни названия - при наведении
    // на строку показываем у мыши иконку вещи. (Сначала была бледная иконка
    // в ячейке выхода станка - игра там её не показывала.) Иконки игра рисует
    // заранее (RenderToTextureotron::buildAllItemsIcons) в data/icons под
    // именем «<ID вещи>.<ID производителя/материала>[.…].png»; у вещей с
    // готовой картинкой она в поле «icon». Обе папки - группа ресурсов Icons.
    // ---------------------------------------------------------------

    // ID вещи -> имена иконок в data/icons (читается один раз).
    std::map<std::string, std::vector<std::string> >& IconIndex()
    {
        static std::map<std::string, std::vector<std::string> > index;
        static bool built = false;
        if (built)
            return index;
        built = true;
        WIN32_FIND_DATAA found;
        HANDLE h = FindFirstFileA("data\\icons\\*.png", &found);
        if (h == INVALID_HANDLE_VALUE)
            return index;
        do
        {
            const std::string name(found.cFileName);
            // Первая часть - ID вида «1012-gamedata.base» / «10-Мод.mod».
            std::string::size_type cut = std::string::npos;
            const char* ends[] = { ".base.", ".mod.", ".base.png", ".mod.png" };
            for (size_t i = 0; i < 4; ++i)
            {
                const std::string::size_type at = name.find(ends[i]);
                if (at != std::string::npos)
                {
                    const std::string::size_type end = at + (ends[i][1] == 'b' ? 5 : 4);
                    if (cut == std::string::npos || end < cut)
                        cut = end;
                }
            }
            if (cut != std::string::npos)
                index[name.substr(0, cut)].push_back(name);
        } while (FindNextFileA(h, &found));
        FindClose(h);
        if (g_debug)
        {
            char line[96];
            sprintf_s(line, "DarkUiTweaks: %u items with rendered icons", static_cast<unsigned>(index.size()));
            DebugLog(line);
        }
        return index;
    }

    bool ResourceKnown(const std::string& name)
    {
        try
        {
            return Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(name);
        }
        catch (...)
        {
            return false;
        }
    }

    // Имя текстуры иконки вещи или пусто.
    std::map<GameData*, std::string> g_itemIconCache;      // вещь -> иконка (и неудачи)

    std::string ItemIconTexture(GameData* data)
    {
        if (data == NULL)
            return std::string();
        std::map<GameData*, std::string>& cache = g_itemIconCache;
        std::map<GameData*, std::string>::const_iterator known = cache.find(data);
        if (known != cache.end())
            return known->second;

        std::string texture;
        // Готовая картинка из поля «icon» (еда, наборы, …).
        ogre_unordered_map<std::string, std::string>::type::const_iterator file = data->filesdata.find("icon");
        if (file != data->filesdata.end() && !file->second.empty())
        {
            std::string base = file->second;
            const std::string::size_type slash = base.find_last_of("\\/");
            if (slash != std::string::npos)
                base = base.substr(slash + 1);
            if (!base.empty() && ResourceKnown(base))
                texture = base;
        }
        // Нарисованная игрой: сначала с производителем игрока (оружие),
        // потом любая из вариантов этой вещи.
        if (texture.empty())
        {
            const std::map<std::string, std::vector<std::string> >& index = IconIndex();
            std::map<std::string, std::vector<std::string> >::const_iterator it = index.find(data->stringID);
            if (it != index.end() && !it->second.empty())
            {
                GameData* const maker = CraftingBuilding::playerManufacturerData();
                if (maker != NULL)
                {
                    const std::string wanted = data->stringID + "." + maker->stringID + ".png";
                    for (size_t i = 0; i < it->second.size() && texture.empty(); ++i)
                        if (it->second[i] == wanted && ResourceKnown(wanted))
                            texture = wanted;
                }
                for (size_t i = 0; i < it->second.size() && texture.empty(); ++i)
                    if (ResourceKnown(it->second[i]))
                        texture = it->second[i];
            }
        }
        if (g_debug)
            DebugLog(("DarkUiTweaks: craft icon for '" + data->name + "' = '" + texture + "'").c_str());
        cache[data] = texture;
        return texture;
    }

    // Размер картинки - чтобы вписать её без искажения.
    MyGUI::IntSize TextureSize(const std::string& name)
    {
        try
        {
            Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().load(
                name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            if (!tex.isNull())
                return MyGUI::IntSize(static_cast<int>(tex->getWidth()), static_cast<int>(tex->getHeight()));
        }
        catch (...)
        {
        }
        return MyGUI::IntSize(0, 0);
    }

    // Станок вкладки «Ремесло» - его задаёт игра (setCraftingBench).
    hand g_benchHandle;
    bool g_benchKnown = false;
    void (*g_origSetCraftingBench)(ManagementScreen* self, const hand& building) = NULL;

    void SetCraftingBench_hook(ManagementScreen* self, const hand& building)
    {
        g_origSetCraftingBench(self, building);
        g_benchHandle = building;
        g_benchKnown = true;
    }

    // Подпись строки списка «Предметы» -> вещь и материал рецепта. Игра
    // пишет «Вещь», «Вещь - Цвет» (броня) или «Вещь, Производитель» (оружие).
    struct Recipe
    {
        GameData* item;
        GameData* material;
    };
    // Подпись -> рецепт. Копится за всю игру и не чистится: указатели на
    // записи данных живут всю сессию (08.10.2026: пересборка раз в 2 с
    // сбивала задержку - иконка мигала). Неудачи - отдельно, чтобы не искать
    // заново каждый кадр.
    std::map<std::string, Recipe> g_recipes;
    std::set<std::string> g_recipeMisses;
    std::set<Building*> g_recipesFrom;      // станки, чьи рецепты уже взяты

    // Пробелы схлопнуть и обрезать: у игры бывает «юбка -  черного цвета».
    std::string Squeeze(const std::string& text)
    {
        std::string out;
        for (size_t k = 0; k < text.size(); ++k)
        {
            const char c = text[k];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
                if (!out.empty() && out[out.size() - 1] != ' ')
                    out += ' ';
            }
            else
                out += c;
        }
        while (!out.empty() && out[out.size() - 1] == ' ')
            out.erase(out.size() - 1);
        return out;
    }

    void RefreshRecipes(Building* bench)
    {
        if (bench == NULL || !g_recipesFrom.insert(bench).second)
            return;
        g_recipeMisses.clear();             // у нового станка могут быть они
        static lektor<GameDataGroup> crafts;   // один на всё время: память у игры
        crafts.clear();
        static_cast<CraftingBuilding*>(bench)->getAvailableCrafts(crafts);
        for (uint32_t i = 0; i < crafts.size(); ++i)
        {
            GameData* const item = crafts[i].g1;
            GameData* const material = crafts[i].g2;
            if (item == NULL)
                continue;
            Recipe r = { item, material };
            g_recipes[Squeeze(item->name)] = r;
            if (material != NULL)
            {
                // Броня: «Вещь - Цвет», оружие: «Вещь, Производитель».
                g_recipes[Squeeze(item->name + " - " + material->name)] = r;
                g_recipes[Squeeze(item->name + ", " + material->name)] = r;
            }
        }
        if (g_debug)
        {
            char line[96];
            sprintf_s(line, "DarkUiTweaks: crafting bench has %u recipes", crafts.size());
            DebugLog(line);
        }
    }

    // Запись по имени - поиском самой игры (getDataByName) среди типов.
    GameData* DataByName(const std::string& name, const itemType* types, size_t count)
    {
        if (ou == NULL || name.empty())
            return NULL;
        for (size_t i = 0; i < count; ++i)
            if (GameData* const d = ou->gamedata.getDataByName(name, types[i]))
                return d;
        return NULL;
    }

    // Рецепт по подписи строки. Сначала - рецепты станков, иначе разбор
    // подписи «Вещь», «Вещь - Цвет», «Вещь, Производитель» по именам: так
    // работает и станок, выбранный в «Пунктах» самой вкладки (его игра
    // задаёт мимо setCraftingBench).
    const Recipe* FindRecipe(const std::string& caption, const std::string& raw)
    {
        std::map<std::string, Recipe>::const_iterator it = g_recipes.find(caption);
        if (it != g_recipes.end())
            return &it->second;
        if (g_recipeMisses.count(caption) != 0)
            return NULL;
        static const itemType itemTypes[] = { WEAPON, ARMOUR, ITEM, CROSSBOW, LIMB_REPLACEMENT, CONTAINER };
        static const itemType materialTypes[] = { MATERIAL_SPECS_WEAPON, MATERIAL_SPECS_CLOTHING,
                                                  WEAPON_MANUFACTURER, MATERIAL_SPEC, COLOR_DATA };
        const size_t nItems = sizeof(itemTypes) / sizeof(itemTypes[0]);
        const size_t nMaterials = sizeof(materialTypes) / sizeof(materialTypes[0]);
        // Сначала - текст как есть: двойной пробел бывает частью имени вещи
        // («Бронированная тряпичная юбка -  черного цвета» из Mediocre All
        // Black Armor - одна вещь, не «вещь - цвет»; 09.10.2026).
        Recipe r = { DataByName(raw, itemTypes, nItems), NULL };
        if (r.item == NULL)
            r.item = DataByName(caption, itemTypes, nItems);
        if (r.item == NULL)
        {
            const char* const seps[] = { ", ", " - " };
            for (size_t k = 0; k < 2 && r.item == NULL; ++k)
            {
                const std::string::size_type at = caption.rfind(seps[k]);
                if (at == std::string::npos)
                    continue;
                r.item = DataByName(Squeeze(caption.substr(0, at)), itemTypes, nItems);
                if (r.item == NULL)
                    continue;
                const std::string tail = caption.substr(at + strlen(seps[k]));
                r.material = DataByName(Squeeze(tail), materialTypes, nMaterials);
                if (r.material == NULL)
                    r.material = DataByName(" " + Squeeze(tail), materialTypes, nMaterials);
            }
        }
        if (r.item == NULL)
        {
            g_recipeMisses.insert(caption);
            return NULL;
        }
        if (g_debug)
            DebugLog(("DarkUiTweaks: recipe '" + caption + "' found by name").c_str());
        return &(g_recipes[caption] = r);
    }

    // Иконки нет в data/icons: игра рисует её, только когда вещь впервые
    // попадает в инвентарь (у вещей, которых ещё не было, - пусто). Тогда
    // просим нарисовать сами: временная вещь -> InventoryIcon::createIconImage
    // (рисует и кладёт в data/icons) -> вещь удаляем. Раз на рецепт.
    float FloatField(GameData* d, const char* key, float def)
    {
        ogre_unordered_map<std::string, float>::type::const_iterator it = d->fdata.find(key);
        return it == d->fdata.end() ? def : it->second;
    }

    int NextPow2(int v)
    {
        int p = 1;
        while (p < v && p < 4096)
            p <<= 1;
        return p;
    }

    std::map<std::pair<GameData*, GameData*>, std::string> g_recipeIconCache;

    // Последняя попытка - вещь из записи-состояния, как при загрузке
    // сохранения (так же делает Emkejs-Test-Kit). Фабрика по аргументам
    // создаёт оружие, только если его выпускает какой-то производитель;
    // тренировочное оружие из мода открывает лишь исследование - ни одно
    // сочетание createItem его не давало (08.10.2026).
    Item* CreateFromState(const Recipe& r)
    {
        static GameDataContainer* container = NULL;     // не удаляется: живёт всю игру
        if (container == NULL)
            container = new GameDataContainer();
        GameData* const state = container->createNewData(INVENTORY_ITEM_STATE, "", "dut_icon_state");
        if (state == NULL)
            return NULL;
        const bool weapon = r.item->type == WEAPON;
        GameData* const model = weapon ? r.material : NULL;
        GameData* const company = weapon ? CraftingBuilding::playerManufacturerData() : NULL;
        GameData* const material = weapon ? NULL : r.material;
        const hand none;
        ogre_unordered_map<std::string, int>::type::const_iterator fn = r.item->idata.find("item function");
        state->addString("uniform", "", "", true);
        state->addString("color sid", "", "", true);
        state->addString("material sid", model != NULL ? model->stringID
                                         : (material != NULL ? material->stringID : std::string()), "", true);
        state->addString("company sid", company != NULL ? company->stringID : std::string(), "", true);
        state->addString("section", "back", "", true);
        state->addString("base data sid", r.item->stringID, "", true);
        state->add("item function", fn != r.item->idata.end() ? fn->second : static_cast<int>(ITEM_WEAPON), "", true);
        state->add("inventory x", 0, "", true);
        state->add("inventory y", 0, "", true);
        state->add("level", 0, "", true);
        state->add("quantity", 1, "", true);
        state->add("insideBuildingI", none.index, "", true);
        state->add("insideBuildingC", none.container, "", true);
        state->add("insideBuildingS", none.serial, "", true);
        state->add("insideBuildingCS", none.containerSerial, "", true);
        state->add("insideBuildingTYPE", static_cast<int>(none.type), "", true);
        state->add("ownedbyI", none.index, "", true);
        state->add("ownedbyC", none.container, "", true);
        state->add("ownedbyS", none.serial, "", true);
        state->add("ownedbyCS", none.containerSerial, "", true);
        state->add("ownedbyTYPE", static_cast<int>(none.type), "", true);
        Item* const item = ou->theFactory->createItem(state);
        container->destroyData(state);
        if (g_debug)
            DebugLog(("DarkUiTweaks: item from a state for '" + r.item->name + "': " + (item != NULL ? "ok" : "failed")).c_str());
        return item;
    }

    std::string RenderRecipeIcon(const Recipe& r)
    {
        if (ou == NULL || ou->theFactory == NULL || r.item == NULL)
            return std::string();
        // Сочетания аргументов - как перебирает Emkejs-Test-Kit: у оружия
        // фабрика ждёт модель и производителя (порядок и уровень бывают
        // разными), у брони - материал (цвет). 08.10.2026: у тренировочного
        // оружия первая попытка (модель, производитель игрока, 0) не вышла.
        const bool weapon = r.material != NULL && r.material->type == MATERIAL_SPECS_WEAPON;
        GameData* const maker = CraftingBuilding::playerManufacturerData();
        struct Attempt
        {
            GameData* a;
            GameData* b;
            int level;
        };
        const Attempt weaponTries[] = {
            { r.material, maker, 0 }, { r.material, maker, 1 }, { maker, r.material, 1 },
            { r.material, NULL, 1 }, { NULL, maker, 1 }, { NULL, r.material, 1 }, { NULL, NULL, 1 },
            { NULL, NULL, 0 } };
        const Attempt otherTries[] = {
            { NULL, r.material, 0 }, { NULL, r.material, 1 }, { NULL, NULL, 0 } };
        const Attempt* const tries = weapon ? weaponTries : otherTries;
        const size_t count = weapon ? sizeof(weaponTries) / sizeof(weaponTries[0])
                                    : sizeof(otherTries) / sizeof(otherTries[0]);
        Item* item = NULL;
        for (size_t t = 0; t < count && item == NULL; ++t)
        {
            item = ou->theFactory->createItem(r.item, hand(), tries[t].a, tries[t].b, tries[t].level, NULL);
            if (item == NULL && g_debug)
            {
                char line[200];
                sprintf_s(line, "DarkUiTweaks: createItem try %u failed for ", static_cast<unsigned>(t));
                DebugLog((line + r.item->name).c_str());
            }
        }
        if (item == NULL)
            item = CreateFromState(r);
        if (item == NULL)
            return std::string();
        std::string name;
        iVector2 size;
        InventoryIcon::createIconImage(item, name, size);
        ou->destroy(item, false, "DarkUiTweaks craft icon");
        if (g_debug)
            DebugLog(("DarkUiTweaks: rendered icon for '" + r.item->name + "' = '" + name + "'").c_str());
        return name;
    }

    // Иконка рецепта: сначала «вещь.материал», потом общая для вещи.
    std::string RecipeIconTexture(const Recipe& r)
    {
        std::map<std::pair<GameData*, GameData*>, std::string>& rendered = g_recipeIconCache;
        const std::pair<GameData*, GameData*> key(r.item, r.material);
        std::map<std::pair<GameData*, GameData*>, std::string>::const_iterator done = rendered.find(key);
        if (done != rendered.end())
            return done->second;
        if (r.material != NULL)
        {
            const std::map<std::string, std::vector<std::string> >& index = IconIndex();
            std::map<std::string, std::vector<std::string> >::const_iterator it = index.find(r.item->stringID);
            if (it != index.end())
            {
                const std::string prefix = r.item->stringID + "." + r.material->stringID + ".";
                for (size_t i = 0; i < it->second.size(); ++i)
                    if (it->second[i].compare(0, prefix.size(), prefix) == 0 && ResourceKnown(it->second[i]))
                        return it->second[i];
            }
        }
        std::string texture = ItemIconTexture(r.item);
        if (texture.empty())
        {
            // Неудачу помним ДО попытки: если рисование упадёт, иначе оно
            // повторялось бы каждый кадр (08.10.2026 - отсюда и просадка FPS).
            rendered[key] = std::string();
            texture = RenderRecipeIcon(r);
            rendered[key] = texture;
        }
        return texture;
    }

    WidgetRef* g_hoverPanel = new WidgetRef();   // не удаляется: см. WidgetRef.h
    WidgetRef* g_hoverImage = new WidgetRef();

    void HideHoverIcon()
    {
        if (MyGUI::Widget* const panel = g_hoverPanel->get())
            if (panel->getVisible())
                panel->setVisible(false);
    }

    // Текст надписи без цветовых тегов MyGUI («#RRGGBB»; «##» - это «#»).
    std::string PlainText(MyGUI::Widget* w)
    {
        MyGUI::TextBox* const text = w != NULL ? w->castType<MyGUI::TextBox>(false) : NULL;
        if (text == NULL)
            return std::string();
        const std::string raw = text->getCaption().asUTF8();
        std::string out;
        for (size_t k = 0; k < raw.size(); ++k)
        {
            if (raw[k] == '#' && k + 1 < raw.size() && raw[k + 1] == '#')
            {
                out += '#';
                ++k;
            }
            else if (raw[k] == '#' && k + 6 < raw.size())
                k += 6;
            else
                out += raw[k];
        }
        return out;
    }

    bool InManagementScreen(ManagementScreen* screen, MyGUI::Widget* w)
    {
        MyGUI::Widget* root = w;
        while (root != NULL && root->getParent() != NULL)
            root = root->getParent();
        MyGUI::Widget* tabRoot = screen->getTab(0);
        while (tabRoot != NULL && tabRoot->getParent() != NULL)
            tabRoot = tabRoot->getParent();
        return tabRoot != NULL && root == tabRoot;
    }

    // Рецепт по надписи на одной высоте со строкой под мышью: надписи
    // строк лежат рядом с их кнопками в общем контейнере списка, и первая
    // попавшаяся надпись была бы чужой строкой.
    const Recipe* RecipeInRow(MyGUI::Widget* w, int depth, int rowTop, int rowBottom, std::string* texts,
                              MyGUI::Widget* skip = NULL)
    {
        if (w == NULL || w == skip || !w->getVisible())
            return NULL;
        // Сначала дешёвое - высота, текст - только у надписи своей строки
        // (08.10.2026: при быстром ведении мышью по списку FPS 280 -> 200).
        const MyGUI::IntCoord c = w->getAbsoluteCoord();
        // Ветки по прямоугольнику НЕ отсекаем: контейнер строк бывает размером
        // с видимую часть, а строки ниже лежат за его пределами - после
        // прокрутки нижние строки отсекались вместе с веткой (09.10.2026,
        // тестеры: «в очереди иконка есть, в списке Предметы - нет»).
        const int middle = c.top + c.height / 2;
        if (middle >= rowTop && middle <= rowBottom && w->castType<MyGUI::TextBox>(false) != NULL)
        {
            const std::string rawText = PlainText(w);
            const std::string text = Squeeze(rawText);
            if (!text.empty())
            {
                if (const Recipe* found = FindRecipe(text, rawText))
                    return found;
                if (texts != NULL && texts->size() < 400)
                    *texts += "[" + text + "]";
            }
        }
        if (depth <= 0)
            return NULL;
        const size_t children = w->getChildCount();
        // Надписи всех строк - дети одного контейнера: строк с кнопками и
        // надписями бывает сотня и больше (64 обрезало нижние строки). Предела
        // нет: 1024 не хватило, когда изучено всё (09.10.2026, тестеры: ниже
        // «Открытого шлема самурая» иконок не было). Проверка высоты дешёвая.
        for (size_t c = 0; c < children; ++c)
            if (const Recipe* found = RecipeInRow(w->getChildAt(c), depth - 1, rowTop, rowBottom, texts, skip))
                return found;
        return NULL;
    }

    // Рецепт под мышью. Строки «Предметы» - панель данных игры
    // (QueueItemsPanel): под мышью кнопка без подписи, надпись - рядом.
    const Recipe* HoveredRecipe(std::string* chain)
    {
        ManagementScreen* const screen = ManagementScreen::getSingleton();
        if (screen == NULL || !screen->getVisible())
            return NULL;
        MyGUI::InputManager* const input = MyGUI::InputManager::getInstancePtr();
        MyGUI::Widget* w = input != NULL ? input->getMouseFocusWidget() : NULL;
        if (w == NULL)
            return NULL;
        static MyGUI::Widget* s_lastWidget = NULL;
        static MyGUI::IntCoord s_lastCoord;
        static const Recipe* s_lastFound = NULL;
        static DWORD s_lastAt = 0;
        const MyGUI::IntCoord row = w->getAbsoluteCoord();
        const DWORD now = GetTickCount();
        // Тот же виджет на том же месте - прежний ответ; раз в 250 мс всё же
        // заново: поиск списка подставляет в те же строки другие надписи.
        if (w == s_lastWidget && row == s_lastCoord && now - s_lastAt < 250)
            return s_lastFound;
        s_lastWidget = w;
        s_lastCoord = row;
        s_lastAt = now;
        s_lastFound = NULL;
        if (!InManagementScreen(screen, w))
            return NULL;
        if (g_benchKnown)
            RefreshRecipes(g_benchHandle.getBuilding());
        if (row.height <= 0 || row.height > 80)
            return NULL;                    // не строка, а целая панель
        MyGUI::Widget* done = NULL;         // эту ветку уже прошли уровнем ниже
        for (int depth = 0; w != NULL && depth < 4; ++depth, done = w, w = w->getParent())
        {
            if (w->castType<MyGUI::ScrollView>(false) != NULL)
                break;
            std::string texts;
            if (const Recipe* found = RecipeInRow(w, 3, row.top, row.top + row.height,
                                                  chain != NULL ? &texts : NULL, done))
                return s_lastFound = found;
            if (chain != NULL)
                *chain += w->getTypeName() + " '" + w->getName() + "' " + texts + " < ";
        }
        return NULL;
    }

    void ShowHoverIcon(const std::string& texture, GameData* item)
    {
        MyGUI::Widget* panel = g_hoverPanel->get();
        MyGUI::ImageBox* image = g_hoverImage->get() != NULL
            ? g_hoverImage->get()->castType<MyGUI::ImageBox>(false) : NULL;
        if (panel == NULL || image == NULL)
        {
            panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("Kenshi_FloatingPanelSkin",
                MyGUI::IntCoord(0, 0, 64, 64), MyGUI::Align::Default, "ToolTip", "DUT_CraftHoverIcon");
            if (panel == NULL)
                return;
            panel->setNeedMouseFocus(false);
            image = panel->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 1, 1),
                                                         MyGUI::Align::Default);
            if (image == NULL)
                return;
            image->setNeedMouseFocus(false);
            g_hoverPanel->set(panel);
            g_hoverImage->set(image);
        }
        if (image->getUserString("dut_tex") != texture)
        {
            // Сначала загрузить картинку (TextureSize грузит её в Ogre), потом
            // отдать MyGUI: наоборот в первый раз была пустая рамка.
            MyGUI::IntSize img = TextureSize(texture);
            if (img.width <= 0 || img.height <= 0)
            {
                HideHoverIcon();
                return;
            }
            image->setImageTexture(texture);
            image->setUserString("dut_tex", texture);
            // Пропорции - по месту вещи в инвентаре (клетки или пиксели):
            // текстура иконки растянута до удобного видеокарте размера, по
            // ней броня выходила вытянутой. Крупно: 40 px на клетку.
            MyGUI::IntSize grid = item != NULL ? InventoryIcon::getItemSize(item) : MyGUI::IntSize(0, 0);
            int w = 0;
            int h = 0;
            if (grid.width > 0 && grid.height > 0 && grid.width <= 24 && grid.height <= 24)
            {
                w = grid.width * 40;
                h = grid.height * 40;
            }
            else if (grid.width > 0 && grid.height > 0)
            {
                w = grid.width * 3 / 2;
                h = grid.height * 3 / 2;
            }
            else
            {
                w = img.width;
                h = img.height;
            }
            const float cap = std::min(1.0f, std::min(420.0f / w, 300.0f / h));
            w = std::max(32, static_cast<int>(w * cap));
            h = std::max(32, static_cast<int>(h * cap));
            if (g_debug)
            {
                char line[256];
                sprintf_s(line, "DarkUiTweaks: hover icon %s - grid %dx%d, texture %dx%d, shown %dx%d",
                          texture.c_str(), grid.width, grid.height, img.width, img.height, w, h);
                DebugLog(line);
            }
            const int pad = 8;
            panel->setSize(w + pad * 2, h + pad * 2);
            image->setCoord(pad, pad, w, h);
        }
        // Рядом с мышью, не за краем экрана.
        const MyGUI::IntPoint mouse = MyGUI::InputManager::getInstance().getMousePosition();
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        int x = mouse.left + 24;
        int y = mouse.top + 24;
        if (x + panel->getWidth() > view.width)
            x = mouse.left - 12 - panel->getWidth();
        if (y + panel->getHeight() > view.height)
            y = view.height - panel->getHeight();
        if (panel->getPosition() != MyGUI::IntPoint(x, y))
            panel->setPosition(x, y);
        if (!panel->getVisible())
            panel->setVisible(true);
    }

    void UpdateCraftHover()
    {
        if (!g_craftIcon)
        {
            HideHoverIcon();
            return;
        }
        // Клавиша задана - иконка только пока она зажата (vk 0 - NONE). До
        // поиска строки: без клавиши список под мышью не обходим вовсе.
        if (g_craftIconKey.vk != 0 && !HoldKey::Held(g_craftIconKey))
        {
            HideHoverIcon();
            return;
        }
        std::string chain;
        const Recipe* const recipe = HoveredRecipe(g_debug ? &chain : NULL);
        if (recipe == NULL)
        {
            // Подробный журнал: что под мышью, если рецепт не узнан.
            static std::string lastChain;
            if (g_debug && !chain.empty() && chain != lastChain)
            {
                lastChain = chain;
                DebugLog(("DarkUiTweaks: no recipe under the mouse: " + chain).c_str());
            }
            HideHoverIcon();
            return;
        }
        const std::string texture = RecipeIconTexture(*recipe);
        if (texture.empty())
        {
            HideHoverIcon();
            return;
        }
        ShowHoverIcon(texture, recipe->item);
    }

    // Иконка - украшение: любой сбой не должен ронять игру.
    void SafeCraftHover()
    {
        __try
        {
            UpdateCraftHover();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static bool told = false;
            if (!told)
            {
                told = true;
                ErrorLog("DarkUiTweaks: the craft icon failed once - skipped");
            }
        }
    }

    // ---------------------------------------------------------------
    // Прорисовка иконок всех вещей (кнопка в MCM, 08.10.2026)
    //
    // Игра рисует иконку вещи, только когда та впервые попадает в
    // инвентарь. Здесь - заранее, для всех вещей без иконки: по одной за
    // кадр (временная вещь -> InventoryIcon::createIconImage -> удалить),
    // чтобы игра не замирала. Рисунки ложатся в data/icons и остаются.
    // ---------------------------------------------------------------

    std::vector<GameData*> g_iconJob;
    size_t g_iconJobPos = 0;
    bool g_iconJobRunning = false;
    unsigned g_iconJobMade = 0;
    unsigned g_iconJobFailed = 0;
    DWORD g_iconJobStarted = 0;
    DWORD g_iconJobFinished = 0;
    GameData* g_defaultWeaponModel = NULL;
    std::string g_iconJobReport;
    WidgetRef* g_iconJobLabel = new WidgetRef();   // не удаляется: см. WidgetRef.h

    void SetIconJobText(const std::string& text)
    {
        MyGUI::TextBox* label = g_iconJobLabel->get() != NULL
            ? g_iconJobLabel->get()->castType<MyGUI::TextBox>(false) : NULL;
        if (label == NULL)
        {
            const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
            label = MyGUI::Gui::getInstance().createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",
                MyGUI::IntCoord(view.width / 2 - 300, 40, 600, 30), MyGUI::Align::Default, "ToolTip",
                "DUT_IconJobLabel");
            if (label == NULL)
                return;
            label->setNeedMouseFocus(false);
            label->setTextAlign(MyGUI::Align::Center);
            label->setTextShadow(true);
            label->setTextColour(MyGUI::Colour(0.93f, 0.86f, 0.56f));
            g_iconJobLabel->set(label);
        }
        label->setCaption(text);
        label->setVisible(!text.empty());
    }

    bool HasAnyIcon(GameData* d)
    {
        const std::map<std::string, std::vector<std::string> >& index = IconIndex();
        if (index.find(d->stringID) != index.end())
            return true;
        ogre_unordered_map<std::string, std::string>::type::const_iterator file = d->filesdata.find("icon");
        return file != d->filesdata.end() && !file->second.empty();
    }

    int __cdecl StartIconJob(void*, char* err, unsigned errSize)
    {
        if (ou == NULL || ou->theFactory == NULL || ManagementScreen::getSingleton() == NULL)
        {
            strcpy_s(err, errSize, Tr("Load a game first - icons are drawn in the game world."));
            return 1;
        }
        if (g_iconJobRunning)
            return 0;
        g_iconJob.clear();
        static lektor<GameData*> list;      // память у игры - один на всё время
        list.clear();
        ou->gamedata.getDataOfType(list, MATERIAL_SPECS_WEAPON);
        g_defaultWeaponModel = list.size() > 0 ? list[0] : NULL;
        const itemType types[] = { WEAPON, ARMOUR, ITEM, CROSSBOW, LIMB_REPLACEMENT, CONTAINER };
        for (size_t t = 0; t < sizeof(types) / sizeof(types[0]); ++t)
        {
            list.clear();
            ou->gamedata.getDataOfType(list, types[t]);
            for (uint32_t k = 0; k < list.size(); ++k)
                if (list[k] != NULL && !HasAnyIcon(list[k]))
                    g_iconJob.push_back(list[k]);
        }
        g_iconJobPos = 0;
        g_iconJobMade = 0;
        g_iconJobFailed = 0;
        // Отчёт - в файл рядом с модом: журнал игра переписывает при запуске.
        {
            std::string report = IniPath();
            const std::string::size_type slash = report.find_last_of("\\/");
            report = (slash == std::string::npos ? std::string() : report.substr(0, slash + 1)) + "icons_failed.txt";
            g_iconJobReport = report;
            FILE* f = NULL;
            if (fopen_s(&f, report.c_str(), "wb") == 0 && f != NULL)
            {
                fputs("DarkUiTweaks: items whose icon could not be drawn (name | string ID | type)\r\n", f);
                fclose(f);
            }
        }
        g_iconJobStarted = GetTickCount();
        g_iconJobRunning = true;
        char line[128];
        sprintf_s(line, "DarkUiTweaks: drawing icons for %u items without one", static_cast<unsigned>(g_iconJob.size()));
        DebugLog(line);
        return 0;
    }

    void DrawOneIconInner(GameData* d, char* out, size_t outSize)
    {
        Recipe r = { d, d->type == WEAPON ? g_defaultWeaponModel : NULL };
        const std::string name = RenderRecipeIcon(r);
        strncpy_s(out, outSize, name.c_str(), _TRUNCATE);
    }

    // Одна вещь. Сбой - не повод останавливать всё. Имя - в буфер: в
    // функции с __try не может быть объектов с деструктором.
    bool DrawOneIcon(GameData* d, char* out, size_t outSize)
    {
        out[0] = 0;
        __try
        {
            DrawOneIconInner(d, out, outSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            out[0] = 0;
        }
        return out[0] != 0;
    }

    void TickIconJob()
    {
        if (g_iconJobRunning && (ou == NULL || ou->theFactory == NULL))
        {
            // Вышли из игры посреди работы - записи вещей уже не те.
            g_iconJobRunning = false;
            std::vector<GameData*>().swap(g_iconJob);
            SetIconJobText(std::string());
            DebugLog("DarkUiTweaks: drawing all icons stopped - the game was unloaded");
            return;
        }
        if (!g_iconJobRunning)
        {
            if (g_iconJobFinished != 0 && GetTickCount() - g_iconJobFinished > 8000)
            {
                g_iconJobFinished = 0;
                SetIconJobText(std::string());
            }
            return;
        }
        if (g_iconJobPos < g_iconJob.size())
        {
            GameData* const d = g_iconJob[g_iconJobPos++];
            char name[512];
            if (DrawOneIcon(d, name, sizeof(name)))
            {
                ++g_iconJobMade;
                // в оглавление - иначе до перезапуска игры её не найти
                IconIndex()[d->stringID].push_back(name);
            }
            else
            {
                ++g_iconJobFailed;
                FILE* f = NULL;
                if (!g_iconJobReport.empty() && fopen_s(&f, g_iconJobReport.c_str(), "ab") == 0 && f != NULL)
                {
                    fprintf(f, "%s | %s | %d\r\n", d->name.c_str(), d->stringID.c_str(), static_cast<int>(d->type));
                    fclose(f);
                }
            }
            char text[160];
            sprintf_s(text, TrFmt("Drawing item icons: %u / %u"), static_cast<unsigned>(g_iconJobPos),
                      static_cast<unsigned>(g_iconJob.size()));
            SetIconJobText(text);
            if (g_iconJobPos % 100 == 0)
            {
                char line[160];
                sprintf_s(line, "DarkUiTweaks: icons %u / %u (drawn %u, failed %u)", static_cast<unsigned>(g_iconJobPos),
                          static_cast<unsigned>(g_iconJob.size()), g_iconJobMade, g_iconJobFailed);
                DebugLog(line);
            }
            return;
        }
        g_iconJobRunning = false;
        g_iconJobFinished = GetTickCount();
        std::vector<GameData*>().swap(g_iconJob);      // список больше не нужен
        // Забыть прежние неудачи: теперь иконки могут найтись.
        for (std::map<GameData*, std::string>::iterator it = g_itemIconCache.begin(); it != g_itemIconCache.end();)
            if (it->second.empty())
                g_itemIconCache.erase(it++);
            else
                ++it;
        char text[200];
        sprintf_s(text, TrFmt("Item icons done: %u drawn, %u could not be drawn"), g_iconJobMade, g_iconJobFailed);
        SetIconJobText(text);
        char line[200];
        sprintf_s(line, "DarkUiTweaks: icons done in %u s - drawn %u, failed %u",
                  static_cast<unsigned>((GetTickCount() - g_iconJobStarted) / 1000), g_iconJobMade, g_iconJobFailed);
        DebugLog(line);
    }

    void SafeTickIconJob()
    {
        __try
        {
            TickIconJob();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_iconJobRunning = false;
            ErrorLog("DarkUiTweaks: drawing all icons failed - stopped");
        }
    }


    // ---------------------------------------------------------------
    // Кадр интерфейса
    // ---------------------------------------------------------------

    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);

        if (g_enabled)
        {
            PatchQueueBoxes();
            UpdateFactionRelation();
            SafeCraftHover();
            SafeTickIconJob();
        }
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "DarkUiTweaks", "Dark Ui Tweaks", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Interface fixes that layouts cannot do: queues, bars, squad cells, scrolling descriptions."));
    api->section(api, Tr("General"));
    api->toggle(api, "Tweaks", "Enabled", Tr("Enabled"), Tr("Interface fixes that layouts cannot do. Each one can be turned off below."), 1, MCM_RESTART);
    api->section(api, Tr("Research and crafting queues"));
    api->integer(api, "Tweaks", "QueueRowHeight", Tr("Row height, px"), Tr("Row height in the research and crafting queues. The game gives about 16 and Russian text does not fit. 0 - the game's height."), 30, 0, 80, 0);
    api->integer(api, "Tweaks", "QueueBarInset", Tr("Bar inset, px"), Tr("Gap between the progress fill and the row frame. The plugin draws its own fill across the whole frame. 0 - the bar is left to the game."), 5, 0, 16, 0);
    api->section(api, Tr("Squads tab"));
    api->integer(api, "Tweaks", "SquadPortraitRows", Tr("Rows of portraits"), Tr("How many rows of portraits fit in a squad cell on the Squads tab without cutting. 0 - the game's height."), 2, 0, 6, 0);
    api->toggle(api, "Tweaks", "SquadAutoGrow", Tr("Grow with the largest squad"), Tr("When the largest squad needs more rows, all cells grow - the list has one height for all. Off - extra fighters go under the scroll."), 1, 0);
    api->section(api, Tr("Next level bar"));
    api->integer(api, "Tweaks", "ProgressBarPad", Tr("Frame padding, px"), Tr("How much taller the frame of the Next level bar in the skill description is, at the top and the bottom. 0 - the bar is left to the game."), 5, 0, 12, 0);
    api->integer(api, "Tweaks", "ProgressBarDrop", Tr("Lower by, px"), Tr("Moves the bar with its frame down without moving the text, so the text looks centred. Lines below move by the same amount."), 2, -12, 12, 0);
    api->integer(api, "Tweaks", "ProgressBarGrowUp", Tr("Raise the top edge by, px"), Tr("Raises only the top edge of the bar frame. The bottom edge and the text stay where they are."), 1, 0, 8, 0);
    api->section(api, Tr("Description panels"));
    api->toggle(api, "Tweaks", "ScrollDescription", Tr("Scroll bar"), Tr("A scroll bar in the description panels of research and items, so long text is not cut off."), 1, MCM_RESTART);
    api->section(api, Tr("Hints"));
    api->toggle(api, "Tweaks", "FactionRelation", Tr("Faction relation by the name"), Tr("After the faction name of the selected character - that faction's relation to you, like in the factions window: green above zero, red from -10 down."), 1, 0);
    api->toggle(api, "Tweaks", "CraftIcon", Tr("Item icon in the crafting list"), Tr("On the Crafting tab, the icon of the item appears next to the mouse over a line of the Items list."), 1, 0);
    if (api->version >= 3)
        api->action(api, Tr("Draw icons of all items"), Tr("The game draws an item icon only when the item first gets into an inventory. This draws them now for every item without one, a few per frame, in a loaded game. The pictures stay in data/icons."), &StartIconJob, NULL, 0);
    api->hotkey(api, "Tweaks", "CraftIconKey", Tr("Icon key"), Tr("The icon shows while this key is held. NONE (Backspace when assigning) - on hover alone."), "ALT", 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "Tweaks", "Debug", Tr("Detailed log"), Tr("Lists every description panel created and its flags in RE_Kenshi_log.txt - useful if the scroll bar did not appear."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("DarkUiTweaks: disabled in the ini");
        return;
    }

    // Перехват создания панелей ставим первым: описание строится один
    // раз, и пропустить этот момент нельзя.
    if (g_scrollDescription)
    {
        typedef DatapanelGUI* (ForgottenGUI::*CreateFn)(
            const std::string&, MyGUI::Widget*, bool);

        const CreateFn target = &ForgottenGUI::createDatapanel;

        if (KenshiLib::SUCCESS !=
            KenshiLib::AddHook(KenshiLib::GetRealAddress(target),
                               CreateDatapanel_hook,
                               &g_origCreateDatapanel))
        {
            ErrorLog("DarkUiTweaks: could not hook createDatapanel, "
                     "description panels stay as they are");
        }
    }

    // Удержание прокрутки. Функция перезаполнения описания не
    // экспортируется - адрес берём из таблицы KenshiLib по номеру
    // ячейки, с её же самопроверкой.
    if (g_scrollDescription && KenshiSlots::Ready())
    {
        void* const target = KenshiSlots::At(
            KenshiSlots::ManagementScreen_refreshResearchListDescription);

        if (target == NULL ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(
                target, RefreshDescription_hook,
                reinterpret_cast<void**>(&g_origRefreshDescription)))
        {
            ErrorLog("DarkUiTweaks: could not hook the description refresh, "
                     "the scrollbar will jump back to the top");
        }
    }
    else if (g_scrollDescription)
    {
        ErrorLog("DarkUiTweaks: slot table not recognised, "
                 "the scrollbar will jump back to the top");
    }

    // Иконка вещи у мыши во вкладке «Ремесло» - нужно знать её станок.
    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ManagementScreen::setCraftingBench),
                           SetCraftingBench_hook, &g_origSetCraftingBench))
    {
        ErrorLog("DarkUiTweaks: could not hook the crafting tab, no item icons there");
    }

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                           GuiUpdate_hook,
                           &g_origGuiUpdate))
    {
        ErrorLog("DarkUiTweaks: could not hook the interface update, "
                 "queue rows stay as they are");
        return;
    }

    DebugLog("DarkUiTweaks: installed");
}
