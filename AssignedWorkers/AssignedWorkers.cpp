// AssignedWorkers - кто из отряда закреплён за постройкой.
//
// ЧТО НЕ ТАК В ИГРЕ
//
// У выделенной машины, сундука, залежи игра пишет только «Рабочие: 5/5» -
// сколько человек у неё сейчас. Кто именно за ней закреплён, видно лишь с
// другой стороны: открывать список работ у каждого персонажа по очереди.
// На большой базе в этом легко запутаться.
//
// ЧТО ДЕЛАЕТ ПЛАГИН
//
// Пока зажат HoldKey (ALT), рядом с плавающим окном выделенной постройки
// (то, где «ПРОДУКТ» или содержимое сундука) стоит панель: кто из
// персонажей игрока закреплён за постройкой, каким номером эта работа
// стоит в его списке работ и работает ли он там прямо сейчас.
//
// КАКАЯ ПОСТРОЙКА ВЫДЕЛЕНА. Её данные игра выписывает в нижнюю панель
// через виртуальную getGUIData - раз в ~60 мс, пока постройка выделена.
// У построек с работниками (все - UseableStuff) её переопределяют семь
// классов; перехватываем все семь и запоминаем постройку. Вызовы для
// всплывающей подсказки (панель без scrollView) не считаем - это
// постройка под курсором, а не выделенная. Вызовов нет дольше полсекунды
// - значит, выделено что-то другое.
//
// ОКНО. Плавающее окно постройки - MyGUI::Window с её именем в
// заголовке (Kenshi_GenericBuildingWindow у машин и залежей,
// Kenshi_InventoryGenericWindow у сундуков). Ищем его по заголовку, а не
// по разметке, - так панель встаёт к окну при любом моде на интерфейс.
//
// КТО ЗАКРЕПЛЁН. У постоянной работы (Tasker в getPermajobData) цель -
// в subject или в currentSubTarget; сравниваем объекты, а не hand: первая
// версия сравнивала hand и не нашла у залежи никого, хотя работали
// пятеро. И на всякий случай добавляем всех, кто у постройки прямо сейчас
// (UseableStuff::currentOperators): эти работают здесь в любом случае.
//
// СВОБОДНЫЕ. По клавише SelectIdleKey (CTRL+SHIFT+I) выделяются все
// свои персонажи без цели - в окне персонажа «Цель: Нет цели» (текущая
// цель OrdersReceiver пуста) и без текущего действия, в сознании, не в
// клетке; животные - нет.
// Две галочки: не приписанные (SelectIdleNoJobs) и приписанные, но всё
// равно без цели (SelectIdleWithJobs) - например, машина занята или
// нечего делать. Приписан - есть постоянная работа у конкретной постройки
// (станок, машина: IsFixedTarget); «Медик», «Роботехника», «Найти и
// спасти» - не приписка (вторая версия считала любую работу и
// не-приписанным не находила никого). Сверху экрана на 2 с - сколько выделено (или что
// свободных нет: иначе нажатие выглядело бы сломанным).
//
// Первая версия считала свободным того, у кого нет приказов игрока и кто
// не в бою, - и у игрока с «Нет цели» в окне писала «Свободных нет»
// (20.10.2026). Смотрим ту же цель, что показывает игра.
//
// ИСТОРИЯ. Первая версия дописывала строки в саму нижнюю панель
// (setLine). Не годится: getGUIData зовётся для каждой из пяти её частей
// - строки дублировались, - и для подсказки без scrollView, где новая
// строка валит игру (DataPanelLine::createMe читает parent->scrollView).
// Владелец и хотел панель у плавающего окна.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <IniDefaults.h>

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Window.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_InputManager.h>
#include <WidgetRef.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/Tasker.h>
#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/util/hand.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/InventoryGUI.h>
#include <kenshi/Inventory.h>
#include <kenshi/GameData.h>
// scrollView у панели закрытый - открываем на время включения.
#define private public
#define protected public
#include <kenshi/gui/DatapanelGUI.h>
#undef private
#undef protected
#include <kenshi/Building/UseableStuff.h>
#include <kenshi/Building/ProductionBuilding.h>
#include <kenshi/Building/CraftingBuilding.h>
#include <kenshi/Building/FarmBuilding.h>
#include <kenshi/Building/FurnaceBuilding.h>
#include <kenshi/Building/ResearchBuilding.h>
#include <kenshi/Building/TurretBuilding.h>

#include <GameTheme.h>
#include <HoldKey.h>

#define KLOC_DOMAIN "assigned_workers"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <PluginSettings.h>
#include "PowerMarkers.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_powerWarnings = true, g_includeNoPower = true;
    int g_powerSize = 32;
    float g_powerRange = 150, g_powerHeight = 4;
    bool g_showNobody = true;
    bool g_debug = false;
    HoldKey::Key g_holdKey;    // ALT; vk 0 - показывать всегда (см. HoldKey.h)
    HoldKey::Key g_idleKey;    // выделить свободных; vk 0 - нет клавиши
    bool g_idleNoJobs = true;      // выделять не приписанных к постройке
    bool g_idleWithJobs = false;   // выделять приписанных, но без цели


    std::string IniFileName()
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


    // Путь к ini игрока; при первом обращении - копия или дописка из
    // <Мод>.defaults.ini (shared/IniDefaults.h, 11.10.2026).
    std::string IniPath()
    {
        return IniDefaults::Prepare(IniFileName());
    }


    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        const char* const section = "AssignedWorkers";
        g_powerWarnings = PluginSettings::Bool(section, "PowerWarnings", true, ini);
        g_includeNoPower = PluginSettings::Bool(section, "IncludeNoPower", true, ini);
        g_powerSize = (int)PluginSettings::Number(section, "PowerIconSize", 32, 12, 96, ini);
        g_powerRange = PluginSettings::Number(section, "PowerMarkerRange", 150, 20, 5000, ini);
        g_powerHeight = PluginSettings::Number(section, "PowerMarkerHeight", 4, 0, 50, ini);

        g_enabled =
            GetPrivateProfileIntA(section, "Enabled", 1, ini.c_str()) != 0;
        g_showNobody =
            GetPrivateProfileIntA(section, "ShowNobody", 1, ini.c_str()) != 0;
        g_debug =
            GetPrivateProfileIntA(section, "Debug", 0, ini.c_str()) != 0;
        KLocLog::SetEnabled(g_debug);

        char key[32] = {};
        GetPrivateProfileStringA(section, "HoldKey", "ALT", key, sizeof(key),
                                 ini.c_str());
        g_holdKey = HoldKey::Parse(key);   // любая клавиша, «NONE» - всегда

        char idle[32] = {};
        GetPrivateProfileStringA(section, "SelectIdleKey", "CTRL+SHIFT+I", idle,
                                 sizeof(idle), ini.c_str());
        g_idleKey = HoldKey::Parse(idle);  // «NONE» - vk 0, без клавиши
        g_idleNoJobs =
            GetPrivateProfileIntA(section, "SelectIdleNoJobs", 1, ini.c_str()) != 0;
        g_idleWithJobs =
            GetPrivateProfileIntA(section, "SelectIdleWithJobs", 0, ini.c_str()) != 0;
    }


    // Клавиша считается зажатой, только когда окно игры впереди: иначе
    // ALT+TAB в другое окно показывал бы панель.
    bool HoldKeyDown()
    {
        return HoldKey::Held(g_holdKey);
    }


    // ---------------------------------------------------------------
    // Выделенная постройка
    // ---------------------------------------------------------------

    UseableStuff* g_selected = NULL;   // сверяется с g_selectedHandle
    DWORD g_selectedAt = 0;

    hand& SelectedHandle()
    {
        static hand h;
        return h;
    }

    // Нет вызовов getGUIData дольше этого - постройка уже не выделена.
    const DWORD SELECTION_TIMEOUT_MS = 500;


    // Постройка, если она всё ещё выделена и жива. Указатель годится,
    // только пока его hand ведёт на тот же объект: постройку могли
    // снести, и тогда hand ведёт в никуда.
    UseableStuff* SelectedBuilding()
    {
        if (g_selected == NULL ||
            GetTickCount() - g_selectedAt > SELECTION_TIMEOUT_MS)
        {
            return NULL;
        }

        RootObjectBase* const alive = SelectedHandle().getRootObjectBase();
        if (alive != static_cast<RootObjectBase*>(g_selected))
            return NULL;

        return g_selected;
    }


    // ---------------------------------------------------------------
    // Кто закреплён
    // ---------------------------------------------------------------

    struct Worker
    {
        std::string name;
        std::vector<int> slots;   // номера работ в его списке, с единицы
        bool working;
    };


    bool IsOperating(UseableStuff* building, Character* who)
    {
        typedef std::set<hand, std::less<hand>,
                         Ogre::STLAllocator<hand, Ogre::GeneralAllocPolicy> >
            HandSet;

        const HandSet& operators = building->currentOperators;
        for (HandSet::const_iterator it = operators.begin();
             it != operators.end(); ++it)
        {
            if (it->getRootObjectBase() == static_cast<RootObjectBase*>(who))
                return true;
        }

        return false;
    }


    // Закреплена ли работа за постройкой.
    //
    // Смотреть только на цель мало: у «Инженера» в subject лежит
    // последняя постройка, которую он чинил или строил, - и первая версия
    // показывала его «закреплённым» за стендом дубления или сундуком, к
    // которым его никто не прикреплял (30.09.2026). Закрепление у игры -
    // отдельный признак самой работы: постоянная работа с фиксированной
    // целью (TaskData::permaJob_FixedTarget). У «Работы с машиной» он
    // есть, у «Инженера» - нет.
    bool IsFixedTarget(const Tasker* job)
    {
        return job->taskData != NULL && job->taskData->permaJob_FixedTarget;
    }


    bool Targets(const Tasker* job, UseableStuff* building)
    {
        if (!IsFixedTarget(job))
            return false;

        RootObjectBase* const target = static_cast<RootObjectBase*>(building);
        return job->subject.getRootObjectBase() == target ||
               job->currentSubTarget.getRootObjectBase() == target;
    }


    void CollectWorkers(UseableStuff* building, std::vector<Worker>& out)
    {
        if (ou == NULL || ou->player == NULL)
            return;

        const lektor<Character*>& squad = ou->player->playerCharacters;

        for (uint32_t i = 0; i < squad.size(); ++i)
        {
            Character* const c = squad[i];
            if (c == NULL)
                continue;

            Worker worker;
            const int count = c->getPermajobCount();
            for (int slot = 0; slot < count; ++slot)
            {
                const Tasker* const job = c->getPermajobData(slot);
                if (job != NULL && Targets(job, building))
                    worker.slots.push_back(slot + 1);
            }

            worker.working = IsOperating(building, c);

            if (worker.slots.empty() && !worker.working)
                continue;

            worker.name = c->getName();
            out.push_back(worker);
        }
    }


    std::string NameOf(const hand& h)
    {
        RootObjectBase* const o = h.getRootObjectBase();
        return o != NULL ? "'" + o->getName() + "'" : std::string("none");
    }


    // Для журнала, раз на постройку: у каждой работы персонажей - куда
    // она смотрит. По нему видно, где игра на самом деле держит цель.
    void DumpJobs(UseableStuff* building)
    {
        if (ou == NULL || ou->player == NULL)
            return;

        DebugLog("AssignedWorkers: jobs around '" + building->getName() + "'");

        const lektor<Character*>& squad = ou->player->playerCharacters;
        int written = 0;

        for (uint32_t i = 0; i < squad.size() && written < 40; ++i)
        {
            Character* const c = squad[i];
            if (c == NULL)
                continue;

            const int count = c->getPermajobCount();
            for (int slot = 0; slot < count && written < 40; ++slot)
            {
                const Tasker* const job = c->getPermajobData(slot);
                if (job == NULL)
                    continue;

                char head[64];
                sprintf_s(head, " #%d '", slot + 1);
                char kind[64];
                sprintf_s(kind, " task %d%s",
                          job->taskData != NULL
                              ? static_cast<int>(job->taskData->key) : -1,
                          IsFixedTarget(job) ? " fixed" : " loose");
                DebugLog("AssignedWorkers:   " + c->getName() + head +
                         c->getPermajobName(slot) + "'" + kind + " subject " +
                         NameOf(job->subject) + ", subtarget " +
                         NameOf(job->currentSubTarget) +
                         (Targets(job, building) ? " - MATCH" : ""));
                ++written;
            }
        }
    }


    // ---------------------------------------------------------------
    // Окно постройки
    // ---------------------------------------------------------------

    // Заголовок без цветовых тегов «#RRGGBB» («##» - сама решётка).
    std::string StripTags(const std::string& text)
    {
        std::string out;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '#')
            {
                if (i + 1 < text.size() && text[i + 1] == '#')
                {
                    out += '#';
                    ++i;
                    continue;
                }

                bool tag = i + 7 <= text.size();
                for (size_t k = 1; tag && k < 7; ++k)
                    tag = isxdigit(static_cast<unsigned char>(text[i + k])) != 0;
                if (tag)
                {
                    i += 6;
                    continue;
                }
            }
            out += text[i];
        }
        return out;
    }


    // Окно постройки ищем по заголовку. Имён два: самой постройки и её
    // записи в данных. Перевод игры грузится как мод (de.translation и
    // т.п.) и переименовывает записи, а у постройки из сохранения имя
    // может остаться прежним - 07.10.2026 у игрока на немецком окно не
    // находилось вовсе: заголовок переведён, getName() - нет.
    bool SameText(const std::string& a, const std::string& b)
    {
        return !a.empty() && a.size() == b.size() && _stricmp(a.c_str(), b.c_str()) == 0;
    }

    MyGUI::Widget* FindBuildingWindowNow(UseableStuff* building);

    // Найденное окно помним (ревью 08.10.2026): поиск перебирает все окна и
    // переводит их заголовки - каждый кадр, пока зажата клавиша, незачем.
    // Пока окно живо и видно - то же, перепроверка раз в полсекунды.
    WidgetRef* g_windowRef = new WidgetRef();      // не удаляется: см. WidgetRef.h
    UseableStuff* g_windowFor = NULL;
    DWORD g_windowCheckedAt = 0;

    MyGUI::Widget* FindBuildingWindow(UseableStuff* building)
    {
        const DWORD now = GetTickCount();
        MyGUI::Widget* const known = g_windowRef->get();
        if (g_windowFor == building && now - g_windowCheckedAt < 500)
            return known != NULL && known->getInheritedVisible() ? known : NULL;
        g_windowCheckedAt = now;
        g_windowFor = building;
        MyGUI::Widget* const found = FindBuildingWindowNow(building);
        g_windowRef->set(found);
        return found;
    }

    MyGUI::Widget* FindBuildingWindowNow(UseableStuff* building)
    {
        const std::string own = building->getName();
        const std::string data = building->data != NULL ? building->data->name : std::string();

        MyGUI::EnumeratorWidgetPtr roots =
            MyGUI::Gui::getInstance().getEnumerator();
        std::string seen;
        while (roots.next())
        {
            MyGUI::Widget* const root = roots.current();
            if (!root->getVisible())
                continue;

            MyGUI::Window* const window = root->castType<MyGUI::Window>(false);
            if (window == NULL)
                continue;
            const std::string caption = StripTags(window->getCaption().asUTF8());
            if (SameText(caption, own) || SameText(caption, data))
                return window;
            if (g_debug)
                seen += " '" + caption + "'";
        }

        // Запасной путь - окно инвентаря самой постройки (хранилища,
        // станки): заголовок тогда не нужен вовсе.
        Inventory* const inventory = building->getInventory();
        InventoryGUI* const gui = inventory != NULL ? inventory->getInventoryGUI() : NULL;
        if (gui != NULL && gui->win != NULL && gui->win->getInheritedVisible())
            return gui->win;

        static UseableStuff* s_loggedFor = NULL;
        if (g_debug && s_loggedFor != building)
        {
            s_loggedFor = building;
            DebugLog("AssignedWorkers: no window for '" + own + "' (data '" + data +
                     "'), visible windows:" + seen);
        }
        return NULL;
    }


    // ---------------------------------------------------------------
    // Панель
    // ---------------------------------------------------------------

    const int kPadX = 14;
    const int kPadY = 10;
    const int kGap = 18;

    WidgetRef g_panelRef;
    MyGUI::Widget* g_panel = NULL;
    std::vector<MyGUI::TextBox*> g_cells;   // по две на строку
    int g_rowHeight = 20;
    int g_panelWidth = 0;
    int g_panelHeight = 0;

    MyGUI::Colour g_title;
    MyGUI::Colour g_label;
    MyGUI::Colour g_working;
    bool g_coloursReady = false;


    void EnsureColours()
    {
        if (g_coloursReady)
            return;
        g_coloursReady = true;

        g_title = GameTheme::ReadableColour("Title", "#B7A074");
        g_label = GameTheme::ReadableColour("Main", "#AFA68B");
        g_working = GameTheme::Parse("#A8B774", g_label);
    }


    void EnsurePanel()
    {
        g_panel = g_panelRef.get();
        if (g_panel != NULL)
            return;
        g_cells.clear();

        g_panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
            "Kenshi_FloatingPanelSkin", MyGUI::IntCoord(0, 0, 100, 100),
            MyGUI::Align::Default, "ToolTip", "AssignedWorkers_Panel");
        g_panelRef.set(g_panel);
        g_panel->setNeedMouseFocus(false);
        g_panel->setNeedKeyFocus(false);
        g_panel->setVisible(false);
    }


    MyGUI::TextBox* Cell(size_t index)
    {
        while (g_cells.size() <= index)
        {
            MyGUI::TextBox* const box = g_panel->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, 10),
                MyGUI::Align::Default, "");
            box->setNeedMouseFocus(false);
            box->setNeedKeyFocus(false);
            g_cells.push_back(box);

            if (g_cells.size() == 1)
                g_rowHeight = box->getFontHeight() + 4;
        }
        return g_cells[index];
    }


    struct RowSpec
    {
        std::string left;
        std::string right;     // пусто - строка во всю ширину
        MyGUI::Colour colour;
    };


    int TextWidth(MyGUI::TextBox* box, const std::string& text)
    {
        box->setCaption(text);
        return box->getTextSize().width;
    }


    // «#» в имени персонажа MyGUI принял бы за начало цвета.
    std::string Escape(const std::string& text)
    {
        std::string out;
        for (size_t i = 0; i < text.size(); ++i)
        {
            out += text[i];
            if (text[i] == '#')
                out += '#';
        }
        return out;
    }


    std::string Hex(const MyGUI::Colour& c)
    {
        char text[16];
        sprintf_s(text, "#%02X%02X%02X",
                  static_cast<int>(c.red * 255.0f + 0.5f),
                  static_cast<int>(c.green * 255.0f + 0.5f),
                  static_cast<int>(c.blue * 255.0f + 0.5f));
        return text;
    }


    std::string Slots(const std::vector<int>& slots)
    {
        std::string text;
        for (size_t i = 0; i < slots.size(); ++i)
        {
            char number[16];
            sprintf_s(number, "%d", slots[i]);
            if (!text.empty())
                text += ", ";
            text += number;
        }
        return text;
    }


    // Правая колонка: «приоритет 5, работает сейчас» - номер цветом
    // подписи, «работает сейчас» зелёным. Номер - место работы в его
    // списке работ (в игре «5: Работа с машиной: ...»), то есть её
    // приоритет. Сначала было «работа №5» - знака «№» в шрифте игры нет,
    // вышло «работа  5», и владелец не понял, что за цифры.
    std::string WorkerValue(const Worker& w)
    {
        std::string value;
        if (!w.slots.empty())
            value = Hex(g_label) + Tr("priority ") + Slots(w.slots);
        if (w.working)
        {
            if (!value.empty())
                value += Hex(g_label) + ", ";
            value += Hex(g_working) + Tr("working now");
        }
        return value;
    }


    void Layout(const std::vector<RowSpec>& rows)
    {
        int left = 0;
        int right = 0;
        int wide = 0;

        for (size_t i = 0; i < rows.size(); ++i)
        {
            MyGUI::TextBox* const a = Cell(i * 2);
            MyGUI::TextBox* const b = Cell(i * 2 + 1);

            if (rows[i].right.empty())
            {
                const int w = TextWidth(a, rows[i].left);
                if (w > wide) wide = w;
                b->setCaption("");
                continue;
            }

            const int wl = TextWidth(a, rows[i].left);
            const int wr = TextWidth(b, rows[i].right);
            if (wl > left) left = wl;
            if (wr > right) right = wr;
        }

        int inner = left + kGap + right;
        if (right == 0)
            inner = left;
        if (inner < wide)
            inner = wide;

        int y = kPadY;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            MyGUI::TextBox* const a = Cell(i * 2);
            MyGUI::TextBox* const b = Cell(i * 2 + 1);

            a->setTextColour(rows[i].colour);
            a->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
            a->setVisible(true);

            if (rows[i].right.empty())
            {
                a->setCoord(kPadX, y, inner, g_rowHeight);
                b->setVisible(false);
            }
            else
            {
                a->setCoord(kPadX, y, left, g_rowHeight);
                b->setCoord(kPadX + inner - right, y, right, g_rowHeight);
                b->setTextColour(g_label);
                b->setTextAlign(MyGUI::Align::Right | MyGUI::Align::VCenter);
                b->setVisible(true);
            }

            y += g_rowHeight;
        }

        for (size_t i = rows.size() * 2; i < g_cells.size(); ++i)
            g_cells[i]->setVisible(false);

        g_panelWidth = inner + kPadX * 2;
        g_panelHeight = y + kPadY;
    }


    void Fill(const std::vector<Worker>& workers)
    {
        EnsureColours();

        std::vector<RowSpec> rows;

        RowSpec title;
        title.left = Tr("Assigned workers");
        title.colour = g_title;
        rows.push_back(title);

        for (size_t i = 0; i < workers.size(); ++i)
        {
            RowSpec row;
            row.left = Escape(workers[i].name);
            row.right = WorkerValue(workers[i]);
            row.colour = g_label;
            rows.push_back(row);
        }

        if (workers.empty())
        {
            RowSpec none;
            none.left = Tr("nobody");
            none.colour = g_label;
            rows.push_back(none);
        }

        Layout(rows);
    }


    // Справа от окна постройки, а не влезает - слева; по верху окна.
    void Place(MyGUI::Widget* window)
    {
        const MyGUI::IntCoord w = window->getAbsoluteCoord();
        const MyGUI::IntSize view =
            MyGUI::RenderManager::getInstance().getViewSize();

        int x = w.left + w.width + 4;
        if (x + g_panelWidth > view.width)
            x = w.left - 4 - g_panelWidth;
        if (x < 0)
            x = 0;

        int y = w.top;
        if (y + g_panelHeight > view.height)
            y = view.height - g_panelHeight;
        if (y < 0)
            y = 0;

        const MyGUI::IntCoord want(x, y, g_panelWidth, g_panelHeight);
        if (g_panel->getCoord() != want)
            g_panel->setCoord(want);
    }


    void Hide()
    {
        g_panel = g_panelRef.get();
        if (g_panel != NULL && g_panel->getVisible())
            g_panel->setVisible(false);
    }


    // ---------------------------------------------------------------
    // Кадр
    // ---------------------------------------------------------------

    // Список пересчитываем не каждый кадр: перебор всего отряда с его
    // работами - не бесплатный, а меняется он не быстрее, чем раз в
    // несколько секунд.
    const DWORD REFRESH_MS = 300;

    UseableStuff* g_shownFor = NULL;
    UseableStuff* g_dumpedFor = NULL;
    std::string g_shownKey;
    DWORD g_refreshedAt = 0;


    void Update()
    {
        if (!g_enabled || !PowerMarkers::WorldActive() || !HoldKeyDown())
        {
            Hide();
            return;
        }

        UseableStuff* const building = SelectedBuilding();
        if (building == NULL)
        {
            Hide();
            return;
        }

        MyGUI::Widget* const window = FindBuildingWindow(building);
        if (window == NULL)
        {
            Hide();
            return;
        }

        if (g_panelRef.get() == NULL) { g_shownFor = NULL; g_shownKey.clear(); }
        EnsurePanel();

        const DWORD now = GetTickCount();
        if (building != g_shownFor || now - g_refreshedAt > REFRESH_MS)
        {
            g_refreshedAt = now;

            if (g_debug && building != g_dumpedFor)
            {
                g_dumpedFor = building;
                DumpJobs(building);
            }

            std::vector<Worker> workers;
            CollectWorkers(building, workers);

            const bool mine = ou != NULL && ou->player != NULL &&
                building->getFaction() == ou->player->getFaction();

            if (workers.empty() && !(mine && g_showNobody))
            {
                g_shownFor = building;
                g_shownKey = "-";
                Hide();
                return;
            }

            // Перестраиваем, только если список поменялся.
            std::string key;
            for (size_t i = 0; i < workers.size(); ++i)
                key += workers[i].name + "|" + WorkerValue(workers[i]) + "\n";

            if (building != g_shownFor || key != g_shownKey)
            {
                Fill(workers);

                if (g_debug && (building != g_shownFor || key != g_shownKey))
                {
                    char count[32];
                    sprintf_s(count, "%u", static_cast<unsigned>(workers.size()));
                    DebugLog("AssignedWorkers: '" + building->getName() + "' - " + count +
                             " assigned");
                }
            }

            g_shownFor = building;
            g_shownKey = key;
        }
        else if (g_shownKey == "-")
        {
            return;   // своя пустая или чужая без наших - панели нет
        }

        Place(window);
        if (!g_panel->getVisible())
            g_panel->setVisible(true);
    }


    // ---------------------------------------------------------------
    // Свободные персонажи
    // ---------------------------------------------------------------

    // Цель персонажа (как «Цель:» в его окне); NULL_TASK - нет цели.
    TaskType GoalOf(Character* c)
    {
        OrdersReceiver* const orders = c->getOrdersReciever();
        if (orders == NULL)
            return NULL_TASK;
        const TaskMatch& goal = orders->getCurrentGoal();
        return goal.taskData == NULL ? NULL_TASK : goal.key();
    }


    // Приписан к постройке: есть работа у конкретной (станок, машина).
    bool IsAssigned(Character* c)
    {
        const int count = c->getPermajobCount();
        for (int slot = 0; slot < count; ++slot)
        {
            const Tasker* const job = c->getPermajobData(slot);
            if (job != NULL && IsFixedTarget(job))
                return true;
        }
        return false;
    }


    // Действие, которое персонаж выполняет прямо сейчас; NULL_TASK - никакого.
    TaskType ActionOf(Character* c)
    {
        AI* const ai = c->getAI();
        if (ai == NULL || ai->taskSystemAI == NULL)
            return NULL_TASK;
        const Tasker* const task = ai->taskSystemAI->actions.getFirstTask();
        return task != NULL ? task->key() : NULL_TASK;
    }


    // Что игра пишет в «Цель:» окна персонажа - для журнала.
    std::string GoalText(Character* c)
    {
        AI* const ai = c->getAI();
        return ai != NULL && ai->taskSystemAI != NULL ? ai->taskSystemAI->getCurrentGoalString()
                                                     : std::string("?");
    }


    // Без цели; приписан или нет - по настройкам.
    bool IsIdle(Character* c)
    {
        if (c->isDead() || c->isUnconcious() || c->isAnimal() != NULL)
            return false;
        if (c->inSomething == IN_PRISON)
            return false;
        // Ни цели, ни действия. Цель 0 - ровно «Нет цели» окна персонажа
        // (журнал 20.10.2026); действие - на случай, когда цель уже
        // снята, а персонаж ещё доделывает начатое.
        const TaskType goal = GoalOf(c), action = ActionOf(c);
        if ((goal != NULL_TASK && goal != IDLE) || (action != NULL_TASK && action != IDLE))
            return false;
        return IsAssigned(c) ? g_idleWithJobs : g_idleNoJobs;
    }


    // Надпись сверху экрана: сколько выделено. Через WidgetRef: удалит
    // игра виджет при смене интерфейса - создадим заново, а не обратимся к
    // удалённому.
    WidgetRef* g_noticeRef = new WidgetRef();      // не удаляется: см. WidgetRef.h
    MyGUI::TextBox* g_noticeText = NULL;           // дочерний, живёт с надписью
    DWORD g_noticeUntil = 0;

    void ShowNotice(const std::string& text)
    {
        EnsureColours();
        MyGUI::Widget* notice = g_noticeRef->get();
        if (notice == NULL)
        {
            MyGUI::Widget* const created = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
                "Kenshi_FloatingPanelSkin", MyGUI::IntCoord(0, 0, 100, 40),
                MyGUI::Align::Default, "ToolTip", "AssignedWorkers_Notice");
            created->setNeedMouseFocus(false);
            created->setNeedKeyFocus(false);
            g_noticeText = created->createWidget<MyGUI::TextBox>(
                "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, 10),
                MyGUI::Align::Default, "");
            g_noticeText->setNeedMouseFocus(false);
            g_noticeText->setNeedKeyFocus(false);
            g_noticeText->setTextAlign(MyGUI::Align::Center);
            g_noticeRef->set(created);
            notice = created;
        }
        g_noticeText->setTextColour(g_label);
        g_noticeText->setCaption(text);
        const MyGUI::IntSize size = g_noticeText->getTextSize();
        const int w = size.width + kPadX * 2, h = size.height + kPadY * 2;
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        notice->setCoord((view.width - w) / 2, view.height / 8, w, h);
        g_noticeText->setCoord(kPadX, kPadY, size.width, size.height);
        notice->setVisible(true);
        g_noticeUntil = GetTickCount() + 2000;
    }


    void SelectIdle()
    {
        if (ou == NULL || ou->player == NULL)
            return;
        PlayerInterface* const player = ou->player;
        const lektor<Character*>& squad = player->playerCharacters;

        std::vector<Character*> idle;
        for (uint32_t i = 0; i < squad.size(); ++i)
        {
            Character* const c = squad[i];
            if (c == NULL)
                continue;
            const bool isFree = IsIdle(c);
            if (isFree)
                idle.push_back(c);
            if (g_debug)
            {
                char note[160];
                sprintf_s(note, " - goal %d, action %d, jobs %d, assigned %d, unconscious %d, caged %d%s",
                          static_cast<int>(GoalOf(c)), static_cast<int>(ActionOf(c)),
                          c->getPermajobCount(), IsAssigned(c) ? 1 : 0,
                          c->isUnconcious() ? 1 : 0, c->inSomething == IN_PRISON ? 1 : 0,
                          isFree ? " - FREE" : "");
                DebugLog("AssignedWorkers: " + c->getName() + note + ", goal text '" + GoalText(c) + "'");
            }
        }

        if (g_debug)
        {
            char sum[96];
            sprintf_s(sum, "AssignedWorkers: free %u of %u", static_cast<unsigned>(idle.size()),
                      static_cast<unsigned>(squad.size()));
            DebugLog(sum);
        }
        if (idle.empty())
        {
            ShowNotice(Tr("Nobody is free"));
            return;
        }
        // Первый - вместо прежнего выделения, остальные - добавляются.
        for (size_t i = 0; i < idle.size(); ++i)
            player->selectObject(idle[i], i > 0);
        char count[16];
        sprintf_s(count, "%u", static_cast<unsigned>(idle.size()));
        ShowNotice(std::string(Tr("Free characters selected: ")) + count);
    }


    bool g_idleWasDown = false;

    void PollIdleKey()
    {
        MyGUI::Widget* const notice = g_noticeRef->get();
        if (notice != NULL && notice->getVisible() &&
            static_cast<LONG>(GetTickCount() - g_noticeUntil) > 0)
            notice->setVisible(false);
        if (g_idleKey.vk == 0 || (!g_idleNoJobs && !g_idleWithJobs))
            return;
        // Печатает игрок в поле - клавиши не наши (спрашиваем, только
        // когда клавиша нажата: кадр без нажатия ничего не стоит).
        const bool down = HoldKey::Held(g_idleKey) &&
                          MyGUI::InputManager::getInstance().getKeyFocusWidget() == NULL;
        if (down && !g_idleWasDown)
            SelectIdle();
        g_idleWasDown = down;
    }


    // ---------------------------------------------------------------
    // Перехват
    // ---------------------------------------------------------------

    typedef void (*GuiDataFn)(UseableStuff* self, DatapanelGUI* panel,
                              int cat);

    const int HOOKS = 7;
    GuiDataFn g_orig[HOOKS] = {};

    template <int N>
    void GuiData_hook(UseableStuff* self, DatapanelGUI* panel, int cat)
    {
        g_orig[N](self, panel, cat);
        if (!g_enabled) return;

        // Подсказка под курсором - панель без scrollView; это не
        // выделенная постройка.
        if (self != NULL && panel != NULL && panel->scrollView != NULL)
        {
            g_selected = self;
            SelectedHandle() = self->getHandle();
            g_selectedAt = GetTickCount();
        }
    }


    // Хук - типизированным указателем: привести адрес экземпляра шаблона
    // прямо к void* VC10 не даёт (C2440), через переменную - можно.
    void Install(const char* name, intptr_t target, GuiDataFn detour,
                 GuiDataFn* orig)
    {
        if (target == 0 ||
            KenshiLib::SUCCESS != KenshiLib::AddHook(
                target, reinterpret_cast<void*>(detour), orig))
        {
            ErrorLog(std::string("AssignedWorkers: could not hook ") + name);
        }
    }


    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);
        if (g_enabled && PowerMarkers::WorldActive()) PollIdleKey();
        else {
            g_idleWasDown = false;
            if (MyGUI::Widget* notice = g_noticeRef->get())
                if (notice->getVisible()) notice->setVisible(false);
            g_selected = NULL; SelectedHandle() = hand();
        }
        Update();
        PowerMarkers::Tick(g_enabled && g_powerWarnings && HoldKeyDown(),
                           g_includeNoPower, g_powerSize, g_powerRange, g_powerHeight, g_debug);
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "AssignedWorkers", "Assigned Workers", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Shows who of the squad is assigned to the selected building."));
    api->section(api, Tr("Panel"));
    api->toggle(api, "AssignedWorkers", "Enabled", Tr("Enabled"), Tr("Who of the squad is assigned to the selected building - a panel next to its window."), 1, 0);
    api->hotkey(api, "AssignedWorkers", "HoldKey", Tr("Show while held"), Tr("The panel is shown only while this key is held. Click and press a key; Backspace - no key, the panel is always shown."), "ALT", 0);
    api->toggle(api, "AssignedWorkers", "ShowNobody", Tr("Show when nobody is assigned"), Tr("At your own building nobody is assigned to: a panel saying nobody. Off - no panel at all. At other buildings nobody is never shown."), 1, 0);
    api->section(api, Tr("Machine power markers"));
    api->toggle(api, "AssignedWorkers", "PowerWarnings", Tr("Mark switched-off machines"), Tr("A red lightning bolt above completed machines owned by the player, using the same show key as the workers panel."), 1, 0);
    api->toggle(api, "AssignedWorkers", "IncludeNoPower", Tr("Also mark missing electricity"), Tr("Also mark enabled machines that require electricity but have none."), 1, 0);
    api->integer(api, "AssignedWorkers", "PowerIconSize", Tr("Power marker size"), Tr("Lightning bolt size in screen pixels."), 32, 12, 96, 0);
    api->number(api, "AssignedWorkers", "PowerMarkerRange", Tr("Power marker range"), Tr("Only loaded player machines within this distance of the camera are scanned."), 150, 20, 5000, 0, 0);
    api->number(api, "AssignedWorkers", "PowerMarkerHeight", Tr("Power marker height"), Tr("World-space height above the machine's origin."), 4, 0, 50, 1, 0);
    api->section(api, Tr("Free characters"));
    api->hotkey(api, "AssignedWorkers", "SelectIdleKey", Tr("Select free characters"), Tr("Selects all your characters with no goal (the character window says No goal), conscious, not caged. Which ones - the two rows below. Click and press a key; Backspace - no key."), "CTRL+SHIFT+I", 0);
    api->toggle(api, "AssignedWorkers", "SelectIdleNoJobs", Tr("Not assigned"), Tr("Select characters with no goal who are not assigned to a building (a bench, a machine...). Medic, Robotics and similar jobs do not count as an assignment."), 1, 0);
    api->toggle(api, "AssignedWorkers", "SelectIdleWithJobs", Tr("Assigned"), Tr("Select characters with no goal even though they are assigned to a building (a bench, a machine...): it has nothing to do or the place is taken."), 0, 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "AssignedWorkers", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: for each selected building, where every character's jobs point and who was found; on the free characters key, why each character is or is not free."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    PowerMarkers::Install();

    Install("UseableStuff::getGUIData",
            KenshiLib::GetRealAddress(&UseableStuff::_NV_getGUIData),
            &GuiData_hook<0>, &g_orig[0]);
    Install("ProductionBuilding::getGUIData",
            KenshiLib::GetRealAddress(&ProductionBuilding::_NV_getGUIData),
            &GuiData_hook<1>, &g_orig[1]);
    Install("CraftingBuilding::getGUIData",
            KenshiLib::GetRealAddress(&CraftingBuilding::_NV_getGUIData),
            &GuiData_hook<2>, &g_orig[2]);
    Install("FarmBuilding::getGUIData",
            KenshiLib::GetRealAddress(&FarmBuilding::_NV_getGUIData),
            &GuiData_hook<3>, &g_orig[3]);
    Install("FurnaceBuilding::getGUIData",
            KenshiLib::GetRealAddress(&FurnaceBuilding::_NV_getGUIData),
            &GuiData_hook<4>, &g_orig[4]);
    Install("ResearchBuilding::getGUIData",
            KenshiLib::GetRealAddress(&ResearchBuilding::_NV_getGUIData),
            &GuiData_hook<5>, &g_orig[5]);
    Install("TurretBuilding::getGUIData",
            KenshiLib::GetRealAddress(&TurretBuilding::_NV_getGUIData),
            &GuiData_hook<6>, &g_orig[6]);

    if (KenshiLib::SUCCESS !=
        KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update),
                           GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("AssignedWorkers: could not hook the interface update, "
                 "the panel will not show");
        return;
    }

    if (g_debug) DebugLog("AssignedWorkers: installed");
}
