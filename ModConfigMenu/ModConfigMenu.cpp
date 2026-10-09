// ModConfigMenu (MCM) - вкладка «Моды» в окне «Настройки» игры.
//
// Слева - список модов, справа - настройки выбранного, родными строками
// игры (DatapanelGUI: флажок, ползунок с полем, выпадающий список, поле
// ввода). Как в MCM из Fallout, только на интерфейсе Kenshi.
//
// КАК НАХОДИТ МОДЫ. Плагин экспортирует MCM_Describe (shared/ModConfigMenu.h).
// Когда игра создаёт окно настроек (OptionsWindow::create), MCM перебирает
// загруженные DLL и зовёт MCM_Describe у каждой, где он есть. Порядок
// загрузки не важен: к моменту первого открытия настроек все плагины
// RE_Kenshi уже загружены.
//
// ГДЕ ЗНАЧЕНИЯ. В ini самого плагина. MCM читает их при показе страницы и
// пишет при изменении (WritePrivateProfileString - комментарии и прочие
// ключи файла не трогаются), затем зовёт apply плагина. Изменения ловим
// опросом в OptionsWindow::update: строки игры сами пишут значения по нашим
// указателям, мы сравниваем с последним записанным. Ползунок пишем, когда
// он полсекунды не двигался; при закрытии окна (saveOptions) - сразу.
//
// Вкладку добавляем так же, как Emkej (Mod Hub) и Au2942: TabItem в
// OptionsWindow::tabs, панель - ForgottenGUI::createDatapanel, панель
// кладётся в данные вкладки (setItemData) - окно настроек ждёт её там.
//
// Вкладка «MCM» встаёт ПЕРЕД последней (ванильной «МОДЫ»), а не в конец:
// RE_Kenshi и Steel Mods вешают свои кнопки на последнюю вкладку окна
// (RE_Kenshi: getItemAt(getItemCount() - 1)). Стань мы последними - их
// кнопки легли бы поверх наших настроек. Так же делает KEP.

#define KLOC_DOMAIN "mod_config_menu"
#include <Localization.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_ScrollView.h>
#include <mygui/MyGUI_TabControl.h>
#include <mygui/MyGUI_TabItem.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_InputManager.h>
#include <mygui/MyGUI_Window.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_ComboBox.h>
#include <WidgetRef.h>
#include <GameTheme.h>

#include <kenshi/Globals.h>
#include <kenshi/gui/ForgottenGUI.h>
#define private public
#define protected public
// DatapanelGUI - тоже под public: нужен адрес его деструктора (_DESTRUCTOR
// у него protected), см. DatapanelDtor_hook.
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/gui/DataPanelLine.h>
#include <kenshi/gui/ToolTip.h>
#include <kenshi/gui/OptionsWindow.h>
#include <kenshi/InputHandler.h>
#undef private
#undef protected

#include <ModConfigMenu.h>
#include <ois/OISKeyboard.h>
#include <fstream>
#include <map>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
    // ---------------------------------------------------------------
    // Модель: моды и их настройки
    // ---------------------------------------------------------------

    enum Kind { K_SECTION, K_TOGGLE, K_INT, K_FLOAT, K_CHOICE, K_TEXT, K_HOTKEY, K_COLOUR, K_ACTION };

    struct Setting
    {
        Kind kind;
        std::string section, key, label, tooltip, def;
        unsigned flags;
        float fmin, fmax;
        int decimals;
        std::vector<std::string> values, labels;

        // Значения, к которым привязаны строки панели.
        bool b;
        float f;
        int i;
        std::string committed;      // что сейчас в ini
        DataPanelLine* line;
        std::string lastSeen;       // для ползунка: значение и когда сдвинули
        DWORD lastMoveMs;
        // Текстовое поле: что в нём было, когда его впервые увидели. Пишем,
        // только если игрок это изменил: однажды поле открылось пустым, и
        // пустота ушла в ini вместо цветов.
        std::string baseline;
        bool hasBaseline;
        // Игрок заходил в поле (у него был фокус ввода). Без этого поле не
        // пишется: на сбросе однажды ушла пустая строка без всякой правки.
        bool touched;

        // API v3: значение не в ini, а у плагина - через его функции.
        bool fn;
        void* ud;
        void* get;
        void* set;
        bool fnHasDef;              // API v5: значение по умолчанию в def

        // Выпадающий список: пункт, замеченный в самом списке в прошлом
        // кадре и ещё не принятый (см. PollChoiceCombo).
        int comboSeen;

        // Цвет (09.10.2026, как в Mod Hub Emkej): палитра или поле #RRGGBB
        // (кнопка Hex), палитра развёрнута или нет. Живёт всю игру; место
        // поля игры - чтобы в режиме Hex ужать его, не теряя исходного.
        bool colourHex;
        bool colourOpen;
        bool colourHasBase;
        MyGUI::IntCoord colourBase;
        MyGUI::IntCoord colourShrunk;

        Setting() : kind(K_TEXT), flags(0), fmin(0), fmax(1), decimals(0), b(false), f(0), i(0),
                    line(NULL), lastMoveMs(0), hasBaseline(false), touched(false), fn(false), ud(NULL), get(NULL),
                    set(NULL), fnHasDef(false), comboSeen(-1), colourHex(false), colourOpen(false),
                    colourHasBase(false) {}
    };

    struct ModEntry
    {
        std::string id, title, ini, info;
        MCM_ApplyFn apply;
        std::vector<Setting*> settings;
        // API v4: своя область вверху страницы
        MCM_AttachFn attach;
        MCM_DetachFn detach;
        void* areaUd;
        int areaPercent;
        ModEntry() : apply(NULL), attach(NULL), detach(NULL), areaUd(NULL), areaPercent(0) {}
    };

    std::vector<ModEntry*> g_mods;
    ModEntry* g_building = NULL;
    bool g_debug = false;

    std::string ModuleDir()
    {
        char path[MAX_PATH] = {};
        const DWORD n = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        std::string s(path, n);
        const std::string::size_type slash = s.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
    }

    std::string Str(const char* s) { return s ? std::string(s) : std::string(); }

    Setting* AddSetting(Kind kind, const char* sec, const char* key, const char* label,
                        const char* tip, const std::string& def, unsigned flags)
    {
        if (g_building == NULL)
            return NULL;
        Setting* s = new Setting();
        s->kind = kind;
        s->section = Str(sec);
        s->key = Str(key);
        s->label = Str(label);
        s->tooltip = Str(tip);
        s->def = def;
        s->flags = flags;
        g_building->settings.push_back(s);
        return s;
    }

    // --- реализация MCM_Api ---

    void __cdecl Api_beginMod(MCM_Api*, const char* id, const char* title, const char* ini,
                              MCM_ApplyFn apply)
    {
        ModEntry* m = new ModEntry();
        m->id = Str(id);
        m->title = title && *title ? Str(title) : m->id;
        m->ini = Str(ini);
        m->apply = apply;
        g_mods.push_back(m);
        g_building = m;
    }

    void __cdecl Api_section(MCM_Api*, const char* title)
    {
        AddSetting(K_SECTION, NULL, NULL, title, NULL, std::string(), 0);
    }

    void __cdecl Api_toggle(MCM_Api*, const char* sec, const char* key, const char* label,
                            const char* tip, int def, unsigned flags)
    {
        AddSetting(K_TOGGLE, sec, key, label, tip, def ? "1" : "0", flags);
    }

    void __cdecl Api_integer(MCM_Api*, const char* sec, const char* key, const char* label,
                             const char* tip, int def, int mn, int mx, unsigned flags)
    {
        char buf[32];
        sprintf_s(buf, "%d", def);
        Setting* s = AddSetting(K_INT, sec, key, label, tip, buf, flags);
        if (s)
        {
            s->fmin = static_cast<float>(mn);
            s->fmax = static_cast<float>(mx);
        }
    }

    void __cdecl Api_number(MCM_Api*, const char* sec, const char* key, const char* label,
                            const char* tip, float def, float mn, float mx, int decimals,
                            unsigned flags)
    {
        char buf[64];
        sprintf_s(buf, "%.*f", decimals < 0 ? 0 : decimals, def);
        Setting* s = AddSetting(K_FLOAT, sec, key, label, tip, buf, flags);
        if (s)
        {
            s->fmin = mn;
            s->fmax = mx;
            s->decimals = decimals < 0 ? 0 : decimals;
        }
    }

    void __cdecl Api_choice(MCM_Api*, const char* sec, const char* key, const char* label,
                            const char* tip, const char* def, const char* const* values,
                            const char* const* labels, int count, unsigned flags)
    {
        Setting* s = AddSetting(K_CHOICE, sec, key, label, tip, Str(def), flags);
        if (s == NULL)
            return;
        for (int k = 0; k < count && values; ++k)
        {
            s->values.push_back(Str(values[k]));
            s->labels.push_back(labels && labels[k] ? Str(labels[k]) : Str(values[k]));
        }
    }

    void __cdecl Api_text(MCM_Api*, const char* sec, const char* key, const char* label,
                          const char* tip, const char* def, unsigned flags)
    {
        AddSetting(K_TEXT, sec, key, label, tip, Str(def), flags);
    }

    void __cdecl Api_hotkey(MCM_Api*, const char* sec, const char* key, const char* label,
                            const char* tip, const char* def, unsigned flags)
    {
        AddSetting(K_HOTKEY, sec, key, label, tip, Str(def), flags);
    }

    void __cdecl Api_colour(MCM_Api*, const char* sec, const char* key, const char* label,
                            const char* tip, const char* def, unsigned flags)
    {
        AddSetting(K_COLOUR, sec, key, label, tip, Str(def), flags);
    }

    void __cdecl Api_info(MCM_Api*, const char* text)
    {
        if (g_building != NULL)
            g_building->info = Str(text);
    }

    // --- API v3: строки на функциях ---

    Setting* AddFn(Kind kind, const char* label, const char* tip, void* get, void* set, void* ud,
                   unsigned flags)
    {
        Setting* s = AddSetting(kind, NULL, NULL, label, tip, std::string(), flags);
        if (s)
        {
            s->fn = true;
            s->get = get;
            s->set = set;
            s->ud = ud;
        }
        return s;
    }

    void __cdecl Api_toggleFn(MCM_Api*, const char* label, const char* tip, MCM_GetIntFn get,
                              MCM_SetIntFn set, void* ud, unsigned flags)
    {
        AddFn(K_TOGGLE, label, tip, get, set, ud, flags);
    }

    void __cdecl Api_integerFn(MCM_Api*, const char* label, const char* tip, MCM_GetIntFn get,
                               MCM_SetIntFn set, void* ud, int mn, int mx, unsigned flags)
    {
        Setting* s = AddFn(K_INT, label, tip, get, set, ud, flags);
        if (s)
        {
            s->fmin = static_cast<float>(mn);
            s->fmax = static_cast<float>(mx);
        }
    }

    void __cdecl Api_numberFn(MCM_Api*, const char* label, const char* tip, MCM_GetFloatFn get,
                              MCM_SetFloatFn set, void* ud, float mn, float mx, int decimals,
                              unsigned flags)
    {
        Setting* s = AddFn(K_FLOAT, label, tip, get, set, ud, flags);
        if (s)
        {
            s->fmin = mn;
            s->fmax = mx;
            s->decimals = decimals < 0 ? 0 : (decimals > 6 ? 6 : decimals);
        }
    }

    void __cdecl Api_choiceFn(MCM_Api*, const char* label, const char* tip, MCM_GetIntFn get,
                              MCM_SetIntFn set, void* ud, const int* values,
                              const char* const* labels, int count, unsigned flags)
    {
        Setting* s = AddFn(K_CHOICE, label, tip, get, set, ud, flags);
        if (s == NULL)
            return;
        // Числа храним строками - дальше выбор работает как у ini-строк.
        for (int k = 0; k < count && values; ++k)
        {
            char buf[16];
            sprintf_s(buf, "%d", values[k]);
            s->values.push_back(buf);
            s->labels.push_back(labels && labels[k] ? Str(labels[k]) : std::string(buf));
        }
    }

    void __cdecl Api_textFn(MCM_Api*, const char* label, const char* tip, MCM_GetTextFn get,
                            MCM_SetTextFn set, void* ud, unsigned flags)
    {
        AddFn(K_TEXT, label, tip, get, set, ud, flags);
    }

    void __cdecl Api_colourFn(MCM_Api*, const char* label, const char* tip, MCM_GetTextFn get,
                              MCM_SetTextFn set, void* ud, unsigned flags)
    {
        AddFn(K_COLOUR, label, tip, get, set, ud, flags);
    }

    void __cdecl Api_hotkeyFn(MCM_Api*, const char* label, const char* tip, MCM_GetKeyFn get,
                              MCM_SetKeyFn set, void* ud, unsigned flags)
    {
        AddFn(K_HOTKEY, label, tip, get, set, ud, flags);
    }

    void __cdecl Api_action(MCM_Api*, const char* label, const char* tip, MCM_ActionFn run,
                            void* ud, unsigned flags)
    {
        AddFn(K_ACTION, label, tip, run, NULL, ud, flags);
    }

    void __cdecl Api_customArea(MCM_Api*, MCM_AttachFn attach, MCM_DetachFn detach, void* ud,
                                int heightPercent)
    {
        if (g_building == NULL || attach == NULL)
            return;
        g_building->attach = attach;
        g_building->detach = detach;
        g_building->areaUd = ud;
        g_building->areaPercent = heightPercent < 10 ? 10 : (heightPercent > 90 ? 90 : heightPercent);
    }

    void __cdecl Api_defaultValue(MCM_Api*, const char* value)
    {
        if (g_building == NULL || g_building->settings.empty() || value == NULL)
            return;
        Setting* const s = g_building->settings.back();
        if (!s->fn || s->kind == K_ACTION || s->kind == K_SECTION)
            return;
        s->def = value;
        s->fnHasDef = true;
    }

    MCM_Api MakeApi()
    {
        MCM_Api api;
        memset(&api, 0, sizeof(api));
        api.version = MCM_API_VERSION;
        api.beginMod = &Api_beginMod;
        api.section = &Api_section;
        api.toggle = &Api_toggle;
        api.integer = &Api_integer;
        api.number = &Api_number;
        api.choice = &Api_choice;
        api.text = &Api_text;
        api.hotkey = &Api_hotkey;
        api.colour = &Api_colour;
        api.info = &Api_info;
        api.toggleFn = &Api_toggleFn;
        api.integerFn = &Api_integerFn;
        api.numberFn = &Api_numberFn;
        api.choiceFn = &Api_choiceFn;
        api.textFn = &Api_textFn;
        api.colourFn = &Api_colourFn;
        api.hotkeyFn = &Api_hotkeyFn;
        api.action = &Api_action;
        api.customArea = &Api_customArea;
        api.defaultValue = &Api_defaultValue;
        return api;
    }

    // Чужой код - под __try (в функции без объектов с деструкторами).
    bool SafeDescribe(MCM_DescribeFn fn, MCM_Api* api)
    {
        __try
        {
            fn(api);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool SafeApply(MCM_ApplyFn fn)
    {
        __try
        {
            fn();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool TitleLess(const ModEntry* a, const ModEntry* b)
    {
        return _stricmp(Tr(a->title.c_str()), Tr(b->title.c_str())) < 0;
    }

    // Debug=1: все тексты страниц - в strings\<id>.txt рядом с DLL. Из них
    // делаются каталоги перевода плагинов, у которых тексты собираются на
    // ходу (моды Emkej).
    void DumpStrings()
    {
        const std::string dir = ModuleDir() + "strings";
        CreateDirectoryA(dir.c_str(), NULL);
        for (size_t k = 0; k < g_mods.size(); ++k)
        {
            const ModEntry* const m = g_mods[k];
            std::ofstream f((dir + "\\" + m->id + ".txt").c_str(), std::ios::binary | std::ios::trunc);
            f << "title\t" << m->title << "\n";
            if (!m->info.empty())
                f << "info\t" << m->info << "\n";
            for (size_t i = 0; i < m->settings.size(); ++i)
            {
                const Setting* const s = m->settings[i];
                f << (s->kind == K_SECTION ? "section" : "label") << "\t" << s->label << "\n";
                if (!s->tooltip.empty())
                    f << "tip\t" << s->tooltip << "\n";
                if (s->fn && s->kind != K_ACTION && s->kind != K_SECTION)
                    f << "default\t" << (s->fnHasDef ? s->def : std::string("<none>")) << "\n";
                for (size_t v = 0; v < s->labels.size(); ++v)
                    f << "option\t" << s->labels[v] << "\n";
            }
        }
    }

    typedef BOOL (WINAPI *EnumModulesFn)(HANDLE, HMODULE*, DWORD, LPDWORD);

    void Discover()
    {
        if (!g_mods.empty())
            return;
        // K32EnumProcessModules - в kernel32 с Windows 7, psapi.lib не нужна.
        EnumModulesFn enumModules = reinterpret_cast<EnumModulesFn>(
            GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32EnumProcessModules"));
        if (enumModules == NULL)
        {
            ErrorLog("ModConfigMenu: K32EnumProcessModules not found");
            return;
        }
        std::vector<HMODULE> mods(1024);
        DWORD needed = 0;
        if (!enumModules(GetCurrentProcess(), &mods[0], static_cast<DWORD>(mods.size() * sizeof(HMODULE)), &needed))
            return;
        const size_t count = std::min<size_t>(mods.size(), needed / sizeof(HMODULE));
        MCM_Api api = MakeApi();
        for (size_t k = 0; k < count; ++k)
        {
            MCM_DescribeFn fn = reinterpret_cast<MCM_DescribeFn>(GetProcAddress(mods[k], "MCM_Describe"));
            if (fn == NULL)
                continue;
            char name[MAX_PATH] = {};
            GetModuleFileNameA(mods[k], name, MAX_PATH);
            g_building = NULL;
            if (!SafeDescribe(fn, &api))
                ErrorLog(std::string("ModConfigMenu: MCM_Describe crashed in ") + name);
            else if (g_debug)
                DebugLog(std::string("ModConfigMenu: described by ") + name);
        }
        g_building = NULL;
        std::sort(g_mods.begin(), g_mods.end(), TitleLess);
        if (g_debug)
            DumpStrings();
        char note[96];
        sprintf_s(note, "ModConfigMenu: %u mod(s) with settings", static_cast<unsigned>(g_mods.size()));
        DebugLog(note);
    }


    // ---------------------------------------------------------------
    // Значения: ini <-> строки панели
    // ---------------------------------------------------------------

    struct KeyName { int code; const char* name; };
    const KeyName kKeyNames[] = {
        // Одиночные модификаторы - первыми: их и печатаем.
        { OIS::KC_LMENU, "ALT" }, { OIS::KC_LCONTROL, "CTRL" }, { OIS::KC_LSHIFT, "SHIFT" },
        { OIS::KC_A, "A" }, { OIS::KC_B, "B" }, { OIS::KC_C, "C" }, { OIS::KC_D, "D" },
        { OIS::KC_E, "E" }, { OIS::KC_F, "F" }, { OIS::KC_G, "G" }, { OIS::KC_H, "H" },
        { OIS::KC_I, "I" }, { OIS::KC_J, "J" }, { OIS::KC_K, "K" }, { OIS::KC_L, "L" },
        { OIS::KC_M, "M" }, { OIS::KC_N, "N" }, { OIS::KC_O, "O" }, { OIS::KC_P, "P" },
        { OIS::KC_Q, "Q" }, { OIS::KC_R, "R" }, { OIS::KC_S, "S" }, { OIS::KC_T, "T" },
        { OIS::KC_U, "U" }, { OIS::KC_V, "V" }, { OIS::KC_W, "W" }, { OIS::KC_X, "X" },
        { OIS::KC_Y, "Y" }, { OIS::KC_Z, "Z" },
        { OIS::KC_0, "0" }, { OIS::KC_1, "1" }, { OIS::KC_2, "2" }, { OIS::KC_3, "3" },
        { OIS::KC_4, "4" }, { OIS::KC_5, "5" }, { OIS::KC_6, "6" }, { OIS::KC_7, "7" },
        { OIS::KC_8, "8" }, { OIS::KC_9, "9" },
        { OIS::KC_F1, "F1" }, { OIS::KC_F2, "F2" }, { OIS::KC_F3, "F3" }, { OIS::KC_F4, "F4" },
        { OIS::KC_F5, "F5" }, { OIS::KC_F6, "F6" }, { OIS::KC_F7, "F7" }, { OIS::KC_F8, "F8" },
        { OIS::KC_F9, "F9" }, { OIS::KC_F10, "F10" }, { OIS::KC_F11, "F11" }, { OIS::KC_F12, "F12" },
        { OIS::KC_SPACE, "SPACE" }, { OIS::KC_TAB, "TAB" }, { OIS::KC_RETURN, "ENTER" },
        { OIS::KC_ESCAPE, "ESC" }, { OIS::KC_BACK, "BACKSPACE" }, { OIS::KC_INSERT, "INSERT" },
        { OIS::KC_DELETE, "DELETE" }, { OIS::KC_HOME, "HOME" }, { OIS::KC_END, "END" },
        { OIS::KC_PGUP, "PAGEUP" }, { OIS::KC_PGDOWN, "PAGEDOWN" }, { OIS::KC_UP, "UP" },
        { OIS::KC_DOWN, "DOWN" }, { OIS::KC_LEFT, "LEFT" }, { OIS::KC_RIGHT, "RIGHT" },
        { OIS::KC_GRAVE, "`" }, { OIS::KC_MINUS, "-" }, { OIS::KC_EQUALS, "=" },
        { OIS::KC_LBRACKET, "[" }, { OIS::KC_RBRACKET, "]" }, { OIS::KC_SEMICOLON, ";" },
        { OIS::KC_APOSTROPHE, "'" }, { OIS::KC_COMMA, "," }, { OIS::KC_PERIOD, "." },
        { OIS::KC_SLASH, "/" }, { OIS::KC_BACKSLASH, "\\" },
        { OIS::KC_NUMPAD0, "NUM0" }, { OIS::KC_NUMPAD1, "NUM1" }, { OIS::KC_NUMPAD2, "NUM2" },
        { OIS::KC_NUMPAD3, "NUM3" }, { OIS::KC_NUMPAD4, "NUM4" }, { OIS::KC_NUMPAD5, "NUM5" },
        { OIS::KC_NUMPAD6, "NUM6" }, { OIS::KC_NUMPAD7, "NUM7" }, { OIS::KC_NUMPAD8, "NUM8" },
        { OIS::KC_NUMPAD9, "NUM9" }, { OIS::KC_LCONTROL, "LCTRL" }, { OIS::KC_RCONTROL, "RCTRL" },
        { OIS::KC_LSHIFT, "LSHIFT" }, { OIS::KC_RSHIFT, "RSHIFT" }, { OIS::KC_LMENU, "LALT" },
        { OIS::KC_RMENU, "RALT" },
    };

    std::string FormatKey(const MCM_Key& k)
    {
        if (k.keycode < 0)
            return "NONE";
        std::string r;
        if (k.modifiers & MCM_KEY_CTRL) r += "CTRL+";
        if (k.modifiers & MCM_KEY_SHIFT) r += "SHIFT+";
        if (k.modifiers & MCM_KEY_ALT) r += "ALT+";
        for (size_t i = 0; i < sizeof(kKeyNames) / sizeof(kKeyNames[0]); ++i)
            if (kKeyNames[i].code == k.keycode)
                return r + kKeyNames[i].name;
        char buf[16];
        sprintf_s(buf, "KEY%d", k.keycode);
        return r + buf;
    }

    bool ParseKey(const std::string& text, MCM_Key* out)
    {
        std::string t;
        for (size_t i = 0; i < text.size(); ++i)
            if (text[i] != ' ')
                t += static_cast<char>(toupper(static_cast<unsigned char>(text[i])));
        out->keycode = -1;
        out->modifiers = 0;
        if (t.empty() || t == "NONE")
            return true;
        // «+» как клавиша - последний символ после другого «+»
        size_t start = 0;
        while (true)
        {
            size_t plus = t.find('+', start);
            if (plus == std::string::npos || plus + 1 >= t.size())
                break;
            const std::string mod = t.substr(start, plus - start);
            if (mod == "CTRL") out->modifiers |= MCM_KEY_CTRL;
            else if (mod == "SHIFT") out->modifiers |= MCM_KEY_SHIFT;
            else if (mod == "ALT") out->modifiers |= MCM_KEY_ALT;
            else return false;
            start = plus + 1;
        }
        const std::string key = t.substr(start);
        for (size_t i = 0; i < sizeof(kKeyNames) / sizeof(kKeyNames[0]); ++i)
            if (key == kKeyNames[i].name)
            {
                out->keycode = kKeyNames[i].code;
                return true;
            }
        if (key.size() > 3 && key.compare(0, 3, "KEY") == 0)
        {
            out->keycode = atoi(key.c_str() + 3);
            return out->keycode > 0;
        }
        return false;
    }

    // Значение у плагина (API v3) - строкой, как оно было бы в ini.
    std::string ReadFn(const Setting* s)
    {
        char buf[1024] = {};
        switch (s->kind)
        {
        case K_TOGGLE:
        case K_INT:
        case K_CHOICE:
        {
            int v = 0;
            if (reinterpret_cast<MCM_GetIntFn>(s->get)(s->ud, &v) != 0)
                return std::string();
            if (s->kind == K_TOGGLE)
                return v ? "1" : "0";
            sprintf_s(buf, "%d", v);
            return buf;
        }
        case K_FLOAT:
        {
            float v = 0;
            if (reinterpret_cast<MCM_GetFloatFn>(s->get)(s->ud, &v) != 0)
                return std::string();
            sprintf_s(buf, "%.*f", s->decimals, v);
            return buf;
        }
        case K_TEXT:
        case K_COLOUR:
            if (reinterpret_cast<MCM_GetTextFn>(s->get)(s->ud, buf, sizeof(buf) - 1) != 0)
                return std::string();
            return buf;
        case K_HOTKEY:
        {
            MCM_Key k = { -1, 0 };
            if (reinterpret_cast<MCM_GetKeyFn>(s->get)(s->ud, &k) != 0)
                return std::string();
            return FormatKey(k);
        }
        default:
            return std::string();
        }
    }

    // Отдать плагину. false - отказ (текст причины - в журнал).
    bool WriteFn(const std::string& modId, const Setting* s, const std::string& v)
    {
        char err[512] = {};
        int r = 0;
        switch (s->kind)
        {
        case K_TOGGLE:
        case K_INT:
        case K_CHOICE:
            r = reinterpret_cast<MCM_SetIntFn>(s->set)(s->ud, atoi(v.c_str()), err, sizeof(err) - 1);
            break;
        case K_FLOAT:
            r = reinterpret_cast<MCM_SetFloatFn>(s->set)(s->ud, static_cast<float>(atof(v.c_str())),
                                                         err, sizeof(err) - 1);
            break;
        case K_TEXT:
        case K_COLOUR:
            r = reinterpret_cast<MCM_SetTextFn>(s->set)(s->ud, v.c_str(), err, sizeof(err) - 1);
            break;
        case K_HOTKEY:
        {
            MCM_Key k;
            if (!ParseKey(v, &k))
            {
                ErrorLog("ModConfigMenu: " + modId + " '" + s->label + "': not a key: " + v);
                return false;
            }
            r = reinterpret_cast<MCM_SetKeyFn>(s->set)(s->ud, k, err, sizeof(err) - 1);
            break;
        }
        default:
            return false;
        }
        if (r != 0)
        {
            ErrorLog("ModConfigMenu: " + modId + " '" + s->label + "' rejected '" + v + "': " + err);
            return false;
        }
        return true;
    }

    std::string ReadIni(const ModEntry* m, const Setting* s)
    {
        if (s->fn)
            return ReadFn(s);
        char buf[2048] = {};
        GetPrivateProfileStringA(s->section.c_str(), s->key.c_str(), s->def.c_str(), buf,
                                 sizeof(buf), m->ini.c_str());
        // Хвостовой комментарий «; ...» и пробелы - не значение.
        std::string v(buf);
        const std::string::size_type semi = v.find(';');
        if (semi != std::string::npos && s->kind != K_TEXT)
            v = v.substr(0, semi);
        while (!v.empty() && (v[v.size() - 1] == ' ' || v[v.size() - 1] == '\t'))
            v.erase(v.size() - 1);
        return v;
    }

    float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

    // Строка из ini -> значения, к которым привязана строка панели.
    void Load(Setting* s, const std::string& v)
    {
        switch (s->kind)
        {
        case K_TOGGLE:
            s->b = atoi(v.c_str()) != 0 || _stricmp(v.c_str(), "true") == 0;
            break;
        case K_INT:
        case K_FLOAT:
            s->f = Clamp(static_cast<float>(atof(v.c_str())), s->fmin, s->fmax);
            break;
        case K_CHOICE:
        {
            s->i = 0;
            int byDef = -1;
            for (size_t k = 0; k < s->values.size(); ++k)
            {
                if (_stricmp(s->values[k].c_str(), v.c_str()) == 0)
                {
                    s->i = static_cast<int>(k);
                    byDef = -2;
                    break;
                }
                if (_stricmp(s->values[k].c_str(), s->def.c_str()) == 0)
                    byDef = static_cast<int>(k);
            }
            if (byDef >= 0)
                s->i = byDef;
            break;
        }
        default:
            break;
        }
    }

    // Текущее значение строки панели -> как оно пишется в ini.
    std::string Current(const Setting* s)
    {
        char buf[64];
        switch (s->kind)
        {
        case K_TOGGLE:
            return s->b ? "1" : "0";
        case K_INT:
            sprintf_s(buf, "%d", static_cast<int>(floor(Clamp(s->f, s->fmin, s->fmax) + 0.5f)));
            return buf;
        case K_FLOAT:
            sprintf_s(buf, "%.*f", s->decimals, Clamp(s->f, s->fmin, s->fmax));
            return buf;
        case K_CHOICE:
        {
            // s->i ставит наш OnChoicePicked (список игры в valPtr не
            // пишет). Пункты добавлены по порядку: номер = номер значения.
            if (s->i >= 0 && s->i < static_cast<int>(s->values.size()))
                return s->values[s->i];
            return s->def;
        }
        case K_HOTKEY:
            return s->committed;            // пишет сам захват клавиши
        case K_TEXT:
        case K_COLOUR:
        {
            DataPanelLine_TextEditable* const t = static_cast<DataPanelLine_TextEditable*>(s->line);
            if (t != NULL && t->getEditBox() != NULL)
            {
                std::string v = t->getEditBox()->getOnlyText();
                std::string::size_type pos = 0;
                while ((pos = v.find("##", pos)) != std::string::npos)
                    v.erase(pos++, 1);
                return v;
            }
            return s->committed;
        }
        default:
            return std::string();
        }
    }


    // ---------------------------------------------------------------
    // Вкладка
    // ---------------------------------------------------------------

    const int CAT = 0;
    const int LIST_GAP = 20;           // между списком и настройками, посередине - разделитель
    const int BUTTON_H = 30;
    // Шаг списка меньше высоты кнопки: у скина кнопки прозрачные поля сверху
    // и снизу, и при шаге 34 между кнопками были большие пустоты (07.10.2026).
    const int BUTTON_STEP = 24;
    const float SECTION_GAP = 0.5f;     // addSpace - в строках, не в пикселях

    OptionsWindow* g_window = NULL;
    MyGUI::TabItem* g_tab = NULL;
    MyGUI::ScrollView* g_list = NULL;
    DatapanelGUI* g_panel = NULL;
    MyGUI::Widget* g_right = NULL;      // правая часть вкладки
    MyGUI::Widget* g_host = NULL;       // родитель панели строк
    MyGUI::Widget* g_area = NULL;       // своя область мода (API v4)
    ModEntry* g_areaOwner = NULL;       // чья область сейчас подключена
    DWORD g_lastUpdateMs = 0;
    // Смена страницы - в следующем кадре: «Сбросить по умолчанию» живёт в
    // самой панели, и пересоздать панель из её обработчика нельзя.
    ModEntry* g_pendingShow = NULL;
    bool g_hasPending = false;
    // Перестройка той же страницы (палитра, сброс, действие) - вернуть
    // прокрутку. 10 кадров не хватало (09.10.2026: страницу всё равно
    // отбрасывало наверх): холст новой панели дорастает позже, и игра сама
    // ставит прокрутку в начало. Держим до 1,5 с, но поправляем, только если
    // страницу отбросило в самый верх или прокрутка упёрлась в недоросший
    // холст; игрок крутит колесо сам - не мешаем.
    MyGUI::IntPoint g_scrollToKeep;
    MyGUI::IntPoint g_scrollLastSet;
    DWORD g_scrollUntil = 0;

    // Что на самом деле прокручивает страницу. У DatapanelGUI два поля:
    // scrollWin (0x50) и scrollView (0x160); по scrollWin прокрутка
    // читалась нулевой (09.10.2026: возврат ни разу не сработал), колесо
    // панель крутит сама (notifyMouseWheel). Берём scrollView, без него -
    // scrollWin.
    MyGUI::ScrollView* PanelScroll(DatapanelGUI* panel)
    {
        if (panel == NULL)
            return NULL;
        if (panel->scrollView != NULL)
            return panel->scrollView;
        return panel->scrollWin != NULL ? panel->scrollWin->castType<MyGUI::ScrollView>(false) : NULL;
    }
    std::vector<MyGUI::Button*> g_buttons;
    ModEntry* g_current = NULL;
    DataPanelLine* g_resetLine = NULL;

    // Живы ли ещё вкладка и панель строк. ReturnToMainMenu (07.10.2026)
    // при возврате в меню сносит интерфейс игры, а окно настроек
    // продолжает получать update - и мы читали удалённую панель (g_panel)
    // по старому указателю: вылет в UpdateScrollBar. WidgetRef обнуляется
    // сам, когда MyGUI удаляет виджет (см. WidgetRef.h).
    WidgetRef* g_tabRef = new WidgetRef();         // не разрушается
    WidgetRef* g_panelRootRef = new WidgetRef();   // корень панели строк в g_host

    // Подсказка в нижней панели окна настроек. Точку в конце игра красит
    // синим (видимо, разбирает текст по-своему) - срезаем её, и китайскую
    // «。» тоже.
    void Tip(MyGUI::Widget* w, const std::string& text)
    {
        if (w == NULL || text.empty() || g_window == NULL || g_window->tooltip == NULL)
            return;
        std::string t(text);
        while (!t.empty() && (t[t.size() - 1] == '.' || t[t.size() - 1] == ' '))
            t.erase(t.size() - 1);
        static const char kZhStop[] = "\xE3\x80\x82";    // «。» в UTF-8
        while (t.size() >= 3 && t.compare(t.size() - 3, 3, kZhStop) == 0)
            t.erase(t.size() - 3);
        // «#RRGGBB» в подсказке MyGUI принимает за смену цвета: текст
        // пропадал, остаток красился синим. «##» - это «#».
        std::string e;
        for (size_t k = 0; k < t.size(); ++k)
        {
            e += t[k];
            if (t[k] == '#')
                e += '#';
        }
        g_window->tooltip->setup(w, e);
    }

    std::string Upper(const std::string& s)
    {
        // Только латиница: кириллицу в UTF-8 побайтно не поднимешь, а
        // подписи раздела и так короткие - оставляем как есть.
        std::string r(s);
        for (size_t k = 0; k < r.size(); ++k)
            if (r[k] >= 'a' && r[k] <= 'z')
                r[k] = static_cast<char>(r[k] - 'a' + 'A');
        return r;
    }

    void Commit(ModEntry* m, Setting* s, const std::string& v)
    {
        if (s->fn)
        {
            if (!WriteFn(m->id, s, v))
            {
                // Плагин не принял - показать то, что у него на самом деле.
                g_pendingShow = m;
                g_hasPending = true;
            }
            s->committed = ReadFn(s);
            if (g_debug)
                DebugLog("ModConfigMenu: " + m->id + " '" + s->label + "' = " + s->committed);
            return;
        }
        WritePrivateProfileStringA(s->section.c_str(), s->key.c_str(), v.c_str(), m->ini.c_str());
        s->committed = v;
        if (g_debug)
            DebugLog("ModConfigMenu: " + m->id + " [" + s->section + "] " + s->key + "=" + v);
    }

    void ApplyMod(ModEntry* m)
    {
        if (m->apply != NULL && !SafeApply(m->apply))
            ErrorLog("ModConfigMenu: apply crashed in " + m->id);
    }

    void OnResetLine(DataPanelLine*);
    void OnActionLine(DataPanelLine*);
    void RunAction(Setting* s);
    std::string EscapeTags(const std::string& v);

    // Подпись кнопки варианта или клавиши.
    std::string ButtonCaption(const Setting* s)
    {
        if (s->kind == K_CHOICE)
        {
            if (s->i >= 0 && s->i < static_cast<int>(s->labels.size()))
                return EscapeTags(s->labels[s->i]);
            return EscapeTags(s->def);
        }
        const std::string& v = s->committed;
        if (v.empty() || _stricmp(v.c_str(), "NONE") == 0)
            return Tr("none");
        return EscapeTags(v);
    }

    // ---- свои кнопки на строках --------------------------------------
    // Как в Mod Hub Emkej: кнопка - свой виджет MyGUI с обработчиком
    // щелчка. Строки-кнопки окна настроек игры (setLineTextButton,
    // setLineButton) и её выпадающий список в окне настроек В ИГРЕ щелчков
    // не отдают (07.10.2026: в главном меню работало, в игре - ни одна).
    // Строка игры остаётся только подписью; кнопку ставим рядом и каждый
    // кадр сверяем место - игра пересобирает строки.
    enum
    {
        OWN_SETTING = 0, OWN_RESET = 1,
        // Строка цвета: кнопка Hex/Палитра, кнопка «Палитра»/«Скрыть»,
        // образец текущего цвета, клетки палитры (arg - номер цвета, -1 -
        // слово по умолчанию: auto, skin...).
        OWN_COLOUR_MODE = 2, OWN_COLOUR_OPEN = 3, OWN_COLOUR_SWATCH = 4, OWN_COLOUR_CELL = 5
    };
    struct OwnButton
    {
        DataPanelLine* line;
        Setting* s;
        int what;
        int arg;
        WidgetRef* ref;
    };
    std::vector<OwnButton> g_ownButtons;
    std::vector<WidgetRef*> g_freeRefs;     // WidgetRef не удаляется - в запас
    Setting* g_capture = NULL;              // кнопка клавиши ждёт нажатия
    int g_captureMod = 0;                   // модификатор, зажатый в одиночку (OIS)

    void OnOwnButton(MyGUI::Widget* sender);
    void OnPanelWheel(MyGUI::Widget* sender, int rel);
    void PlaceColourPart(size_t index);

    // Палитра - как в Mod Hub Emkej (hub_color.h, GetDefaultPalette).
    const char* const kPalette[] = {
        "#FFFFFF", "#C9C9C9", "#8C8C8C", "#4D4D4D", "#111111",
        "#FF3333", "#FF7A00", "#FFD400", "#DEE85A", "#40FF40",
        "#00B894", "#00D2D3", "#3399FF", "#355CFF", "#7A4CFF",
        "#B84DFF", "#FF4FD8", "#FF6B9D", "#B87333", "#F5E6C8",
        "#8C8CFF", "#56CFE1", "#2EC4B6", "#F4A261", "#E76F51"
    };
    const int kPaletteCount = sizeof(kPalette) / sizeof(kPalette[0]);
    const int kPaletteLines = 2;            // строк панели под палитру

    // «#RRGGBB» или «RRGGBB» -> цвет. Слова (auto, skin, value) - нет.
    bool ParseHexColour(const std::string& text, MyGUI::Colour* out)
    {
        std::string v(text);
        while (!v.empty() && (v[0] == ' ' || v[0] == '#'))
            v.erase(0, 1);
        while (!v.empty() && v[v.size() - 1] == ' ')
            v.erase(v.size() - 1);
        if (v.size() < 6)
            return false;
        unsigned rgb[3];
        for (int k = 0; k < 3; ++k)
        {
            char part[3] = { v[k * 2], v[k * 2 + 1], 0 };
            char* end = NULL;
            rgb[k] = static_cast<unsigned>(strtoul(part, &end, 16));
            if (end != part + 2)
                return false;
        }
        if (out != NULL)
            *out = MyGUI::Colour(rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f, 1.0f);
        return true;
    }

    // Значение по умолчанию - слово (auto, skin...), а не код цвета:
    // тогда в палитре первой идёт кнопка с этим словом.
    bool DefaultIsWord(const Setting* s)
    {
        return !s->def.empty() && !ParseHexColour(s->def, NULL);
    }

    // Кнопка из кода - сразу на скине Kenshi_Button1Skin. «Kenshi_Button1»
    // - не скин, а шаблон (kenshi_templates.xml): внутри него скин сидит со
    // смещением 20,25 и размером 29x26, и у виджета, созданного кодом, от
    // кнопки оставалась тонкая полоска (07.10.2026). Шрифт и выравнивание -
    // как в шаблоне.
    MyGUI::Button* MakeButton(MyGUI::Widget* parent, const MyGUI::IntCoord& coord, MyGUI::Align align)
    {
        MyGUI::Button* const b = parent->createWidget<MyGUI::Button>("Kenshi_Button1Skin", coord, align);
        if (b != NULL)
        {
            b->setFontName("Kenshi_PaintedTextFont_Medium");
            b->setTextAlign(MyGUI::Align::Center);
        }
        return b;
    }

    void Poll(bool force);

    bool InsideOurTab(MyGUI::Widget* w)
    {
        MyGUI::Widget* const tab = g_tabRef->get();
        MyGUI::Widget* const root = g_panelRootRef->get();
        for (MyGUI::Widget* p = w; p != NULL; p = p->getParent())
            if ((tab != NULL && p == tab) || (root != NULL && p == root))
                return true;
        return false;
    }

    // Фокус клавиатуры и мыши - снять со своих полей до того, как их не
    // станет. 08.10.2026: ввод цвета в поле, окно закрыли (фокус остался в
    // поле), открыли снова - игра упала на первом кадре отрисовки.
    void DropFocusInside()
    {
        MyGUI::InputManager* const input = MyGUI::InputManager::getInstancePtr();
        if (input == NULL)
            return;
        MyGUI::Widget* const key = input->getKeyFocusWidget();
        if (key != NULL && InsideOurTab(key))
        {
            if (g_debug)
                DebugLog("ModConfigMenu: key focus taken off our field");
            input->resetKeyFocusWidget();
        }
        MyGUI::Widget* const mouse = input->getMouseFocusWidget();
        if (mouse != NULL && InsideOurTab(mouse))
            input->_resetMouseFocusWidget();
    }

    void ForgetOwnButtons()
    {
        for (size_t k = 0; k < g_ownButtons.size(); ++k)
        {
            g_ownButtons[k].ref->reset();   // виджеты уходят вместе с панелью
            g_freeRefs.push_back(g_ownButtons[k].ref);
        }
        g_ownButtons.clear();
        g_capture = NULL;
        g_captureMod = 0;
    }

    std::string OwnCaption(const OwnButton& b)
    {
        if (b.what == OWN_RESET)
            return Tr("Reset to defaults");
        if (b.s->kind == K_ACTION)
            return EscapeTags(b.s->label);
        if (g_capture == b.s)
            return Tr("Press a key... Backspace - none, click - cancel");
        return ButtonCaption(b.s);
    }

    void PlaceOwnButton(size_t index)
    {
        OwnButton& b = g_ownButtons[index];
        if (b.what >= OWN_COLOUR_MODE)
        {
            PlaceColourPart(index);
            return;
        }
        // Строка - игровая строка-текст (setLineTextEditable): у неё
        // правильные высота и место, в том числе у последней строки внизу
        // страницы. Её поле прячем, кнопку ставим ровно на его место
        // (07.10.2026: кнопки по высоте подписи выходили то тонкими, то
        // обрезанными, то съезжали).
        DataPanelLine_TextEditable* const t = static_cast<DataPanelLine_TextEditable*>(b.line);
        MyGUI::Widget* const field = t == NULL ? NULL : t->getEditBox();
        if (field == NULL || field->getParent() == NULL)
            return;                         // поле строка создаёт при показе
        if (field->getVisible())
            field->setVisible(false);
        MyGUI::Widget* const row = field->getParent();
        MyGUI::Widget* button = b.ref->get();
        if (button != NULL && button->getParent() != row)
        {
            MyGUI::Gui::getInstance().destroyWidget(button);
            button = NULL;
        }
        const MyGUI::IntCoord coord = field->getCoord();
        if (button == NULL)
        {
            MyGUI::Button* const created = MakeButton(row, coord, MyGUI::Align::Left | MyGUI::Align::Top);
            if (created == NULL)
                return;
            char index_text[16];
            sprintf_s(index_text, "%u", static_cast<unsigned>(index));
            created->setUserString("mcm_own", index_text);
            created->setNeedMouseFocus(true);
            created->eventMouseButtonClick += MyGUI::newDelegate(&OnOwnButton);
            if (b.s != NULL)
                Tip(created, b.s->tooltip);
            b.ref->set(created);
            button = created;
            if (g_debug)
            {
                const MyGUI::IntCoord real = created->getAbsoluteCoord();
                char line[200];
                sprintf_s(line, "ModConfigMenu: own button %u at %d,%d %dx%d",
                          static_cast<unsigned>(index), real.left, real.top, real.width, real.height);
                DebugLog(line);
            }
        }
        else if (button->getCoord() != coord)
        {
            button->setCoord(coord);
        }
        MyGUI::Button* const asButton = button->castType<MyGUI::Button>(false);
        const std::string caption = OwnCaption(b);
        if (asButton != NULL && asButton->getCaption().asUTF8() != caption)
            asButton->setCaption(caption);
    }

    void PlaceOwnButtons()
    {
        for (size_t k = 0; k < g_ownButtons.size(); ++k)
            PlaceOwnButton(k);
    }

    void AddOwnButton(DataPanelLine* line, Setting* s, int what, int arg = 0)
    {
        OwnButton b;
        b.line = line;
        b.s = s;
        b.what = what;
        b.arg = arg;
        if (!g_freeRefs.empty())
        {
            b.ref = g_freeRefs.back();
            g_freeRefs.pop_back();
        }
        else
        {
            b.ref = new WidgetRef();        // не удаляется: см. WidgetRef.h
        }
        g_ownButtons.push_back(b);
        PlaceOwnButton(g_ownButtons.size() - 1);
    }

    // ---- строка цвета ---------------------------------------------------
    // Как в Mod Hub Emkej: [Hex | Палитра] [Палитра/Скрыть] [образец], под
    // строкой - клетки палитры. Поле игры с кодом цвета остаётся: в режиме
    // Hex оно видно (ужато до середины строки), в режиме палитры спрятано.
    // Смена режима и выбор цвета перестраивают страницу (место под палитру
    // - пустые строки панели).
    MyGUI::Widget* MakeFill(MyGUI::Widget* parent, const MyGUI::IntCoord& coord, const MyGUI::Colour& colour)
    {
        MyGUI::Widget* const w = parent->createWidget<MyGUI::Widget>("WhiteSkin", coord,
                                                                     MyGUI::Align::Left | MyGUI::Align::Top);
        if (w != NULL)
            w->setColour(colour);
        return w;
    }

    // Клетка цвета: тёмная (или светлая - выбранная) рамка, внутри цвет.
    MyGUI::Widget* MakeColourCell(MyGUI::Widget* parent, const MyGUI::IntCoord& coord, size_t index)
    {
        MyGUI::Widget* const frame = MakeFill(parent, coord, MyGUI::Colour(0.05f, 0.05f, 0.05f));
        if (frame == NULL)
            return NULL;
        const int inset = coord.height >= 24 ? 3 : 2;
        MyGUI::Widget* const fill = MakeFill(frame, MyGUI::IntCoord(inset, inset, coord.width - 2 * inset,
                                                                    coord.height - 2 * inset),
                                             MyGUI::Colour(0.3f, 0.3f, 0.3f));
        if (fill != NULL)
            fill->setNeedMouseFocus(false);
        char index_text[16];
        sprintf_s(index_text, "%u", static_cast<unsigned>(index));
        frame->setUserString("mcm_own", index_text);
        frame->setNeedMouseFocus(true);
        frame->eventMouseButtonClick += MyGUI::newDelegate(&OnOwnButton);
        frame->eventMouseWheel += MyGUI::newDelegate(&OnPanelWheel);
        return frame;
    }

    void SetCellColour(MyGUI::Widget* frame, const MyGUI::Colour& fill, bool selected)
    {
        // Цвет виджета MyGUI назад не отдаёт - помним его строкой.
        char key[64];
        sprintf_s(key, "%d %.3f %.3f %.3f", selected ? 1 : 0, fill.red, fill.green, fill.blue);
        if (frame->getUserString("mcm_fill") == key)
            return;
        frame->setUserString("mcm_fill", key);
        frame->setColour(selected ? MyGUI::Colour(0.93f, 0.86f, 0.56f) : MyGUI::Colour(0.05f, 0.05f, 0.05f));
        if (frame->getChildCount() > 0)
            frame->getChildAt(0)->setColour(fill);
    }

    void PlaceColourPart(size_t index)
    {
        OwnButton& b = g_ownButtons[index];
        Setting* const s = b.s;
        DataPanelLine_TextEditable* const t = static_cast<DataPanelLine_TextEditable*>(b.line);
        MyGUI::Widget* const field = t == NULL ? NULL : t->getEditBox();
        if (s == NULL || field == NULL || field->getParent() == NULL)
            return;                         // поле строка создаёт при показе
        MyGUI::Widget* const row = field->getParent();

        // Место поля, которое дала игра (не наше ужатое).
        const MyGUI::IntCoord fc = field->getCoord();
        if (!s->colourHasBase || fc != s->colourShrunk)
        {
            s->colourBase = fc;
            s->colourShrunk = fc;
            s->colourHasBase = true;
        }
        const MyGUI::IntCoord& F = s->colourBase;
        const int h = F.height;
        const int gap = h / 10 > 4 ? h / 10 : 4;
        const int modeW = F.width / 5 > h * 2 ? F.width / 5 : h * 2;
        const MyGUI::IntCoord modeC(F.left, F.top, modeW, h);
        const MyGUI::IntCoord swatchC(F.left + F.width - h, F.top, h, h);
        const MyGUI::IntCoord midC(F.left + modeW + gap, F.top, F.width - modeW - h - 2 * gap, h);

        MyGUI::Widget* w = b.ref->get();
        if (b.what == OWN_COLOUR_MODE)
        {
            if (s->colourHex)
            {
                if (!field->getVisible())
                    field->setVisible(true);
                if (fc != midC)
                    field->setCoord(midC);
                s->colourShrunk = midC;
            }
            else if (field->getVisible())
            {
                field->setVisible(false);
            }
        }
        if (b.what == OWN_COLOUR_OPEN && s->colourHex)
            return;                         // в режиме Hex на её месте поле

        if (b.what == OWN_COLOUR_MODE || b.what == OWN_COLOUR_OPEN)
        {
            const MyGUI::IntCoord& coord = b.what == OWN_COLOUR_MODE ? modeC : midC;
            if (w != NULL && w->getParent() != row)
            {
                MyGUI::Gui::getInstance().destroyWidget(w);
                w = NULL;
            }
            if (w == NULL)
            {
                MyGUI::Button* const created = MakeButton(row, coord, MyGUI::Align::Left | MyGUI::Align::Top);
                if (created == NULL)
                    return;
                char index_text[16];
                sprintf_s(index_text, "%u", static_cast<unsigned>(index));
                created->setUserString("mcm_own", index_text);
                created->setNeedMouseFocus(true);
                created->eventMouseButtonClick += MyGUI::newDelegate(&OnOwnButton);
                Tip(created, b.what == OWN_COLOUR_MODE
                    ? Tr("Switch between the palette and typing the colour code (#RRGGBB)")
                    : Tr("Show or hide the colours to pick from"));
                b.ref->set(created);
                w = created;
            }
            else if (w->getCoord() != coord)
            {
                w->setCoord(coord);
            }
            std::string caption;
            if (b.what == OWN_COLOUR_MODE)
                caption = s->colourHex ? Tr("Palette") : std::string("Hex");
            else
                caption = s->colourOpen ? Tr("Hide") : Tr("Palette");
            MyGUI::Button* const asButton = w->castType<MyGUI::Button>(false);
            if (asButton != NULL && asButton->getCaption().asUTF8() != caption)
                asButton->setCaption(caption);
            return;
        }

        if (b.what == OWN_COLOUR_SWATCH)
        {
            // Образец: то, что в поле (в режиме Hex меняется при наборе).
            const std::string value = Current(s);
            const int inset = h / 8;
            const MyGUI::IntCoord coord(swatchC.left + inset, swatchC.top + inset, swatchC.width - 2 * inset,
                                        swatchC.height - 2 * inset);
            if (w != NULL && w->getParent() != row)
            {
                MyGUI::Gui::getInstance().destroyWidget(w);
                w = NULL;
            }
            if (w == NULL)
            {
                w = MakeColourCell(row, coord, index);
                if (w == NULL)
                    return;
                b.ref->set(w);
            }
            else if (w->getCoord() != coord)
            {
                w->setCoord(coord);
            }
            if (w->getUserString("mcm_value") != value)
            {
                w->setUserString("mcm_value", value);
                Tip(w, std::string(Tr("Current colour")) + ": " + (value.empty() ? std::string(Tr("none")) : value));
            }
            MyGUI::Colour colour(0.3f, 0.3f, 0.3f);
            ParseHexColour(value, &colour);
            SetCellColour(w, colour, false);
            return;
        }

        // Клетка палитры - под строкой, на холсте прокрутки (в ряду её
        // обрезало бы по высоте строки). Место - от поля, каждый кадр.
        if (g_panel == NULL || g_panel->scrollWin == NULL)
            return;
        const bool word = DefaultIsWord(s);
        const int slots = kPaletteCount + (word ? 2 : 0);   // слово - на две клетки
        const int cols = (slots + kPaletteLines - 1) / kPaletteLines;
        int cellGap = h / 14 > 3 ? h / 14 : 3;
        int cell = (F.width - (cols - 1) * cellGap) / cols;
        if (cell > h * 3 / 4)
            cell = h * 3 / 4;
        const int slot = b.arg < 0 ? 0 : b.arg + (word ? 2 : 0);
        const int col = slot % cols;
        const int line = slot / cols;
        const MyGUI::IntPoint rowAbs = row->getAbsolutePosition();
        MyGUI::IntCoord abs(rowAbs.left + F.left + col * (cell + cellGap),
                            rowAbs.top + F.top + h + line * h + (h - cell) / 2,
                            b.arg < 0 ? cell * 2 + cellGap : cell, cell);
        if (w == NULL)
        {
            if (b.arg < 0)
            {
                MyGUI::Button* const created = g_panel->scrollWin->createWidget<MyGUI::Button>(
                    "Kenshi_Button1Skin", MyGUI::IntCoord(0, 0, abs.width, abs.height),
                    MyGUI::Align::Left | MyGUI::Align::Top);
                if (created == NULL)
                    return;
                created->setFontName("Kenshi_PaintedTextFont_Medium");
                created->setTextAlign(MyGUI::Align::Center);
                created->setCaption(EscapeTags(s->def));
                char index_text[16];
                sprintf_s(index_text, "%u", static_cast<unsigned>(index));
                created->setUserString("mcm_own", index_text);
                created->setNeedMouseFocus(true);
                created->eventMouseButtonClick += MyGUI::newDelegate(&OnOwnButton);
                created->eventMouseWheel += MyGUI::newDelegate(&OnPanelWheel);
                Tip(created, std::string(Tr("Default")) + ": " + s->def);
                w = created;
            }
            else
            {
                w = MakeColourCell(g_panel->scrollWin, MyGUI::IntCoord(0, 0, abs.width, abs.height), index);
                if (w == NULL)
                    return;
                Tip(w, kPalette[b.arg]);
            }
            b.ref->set(w);
        }
        const MyGUI::IntPoint parentAbs = w->getParent() != NULL ? w->getParent()->getAbsolutePosition()
                                                                 : MyGUI::IntPoint();
        const MyGUI::IntCoord coord(abs.left - parentAbs.left, abs.top - parentAbs.top, abs.width, abs.height);
        if (w->getCoord() != coord)
            w->setCoord(coord);
        if (b.arg >= 0)
        {
            MyGUI::Colour colour;
            ParseHexColour(kPalette[b.arg], &colour);
            SetCellColour(w, colour, _stricmp(s->committed.c_str(), kPalette[b.arg]) == 0);
        }
    }

    // Колесо над клетками палитры - прокрутка страницы (сами клетки колесо
    // себе забирают).
    void OnPanelWheel(MyGUI::Widget*, int rel)
    {
        MyGUI::ScrollView* const view = PanelScroll(g_panel);
        if (view == NULL)
            return;
        MyGUI::IntPoint offset = view->getViewOffset();
        offset.top += rel > 0 ? 40 : -40;
        view->setViewOffset(offset);
    }

    // ---- захват клавиши ----------------------------------------------
    // Нажатие ловим в InputHandler::keyDownEvent, как Mod Hub, и в игру не
    // пускаем. Esc - отмена, Backspace - «нет клавиши». Модификатор,
    // отпущенный без другой клавиши, - сам по себе клавиша (ALT, CTRL,
    // SHIFT): клавиши показа у AssignedWorkers / GearCompare такие.
    void FinishCapture(const std::string& text)
    {
        Setting* const s = g_capture;
        g_capture = NULL;
        g_captureMod = 0;
        ModEntry* const m = g_current;
        if (s == NULL || m == NULL)
            return;
        Commit(m, s, text);
        ApplyMod(m);
        PlaceOwnButtons();
    }

    bool IsModifier(OIS::KeyCode k)
    {
        return k == OIS::KC_LCONTROL || k == OIS::KC_RCONTROL || k == OIS::KC_LSHIFT ||
               k == OIS::KC_RSHIFT || k == OIS::KC_LMENU || k == OIS::KC_RMENU;
    }

    const char* ModifierName(int k)
    {
        if (k == OIS::KC_LCONTROL || k == OIS::KC_RCONTROL) return "CTRL";
        if (k == OIS::KC_LSHIFT || k == OIS::KC_RSHIFT) return "SHIFT";
        return "ALT";
    }

    void (*g_origKeyDown)(InputHandler*, OIS::KeyCode) = NULL;
    void (*g_origKeyUp)(InputHandler*, OIS::KeyCode) = NULL;

    void KeyDown_hook(InputHandler* self, OIS::KeyCode key)
    {
        if (g_capture != NULL && g_capture->line != NULL)
        {
            if (IsModifier(key))
            {
                g_captureMod = key;
                return;
            }
            g_captureMod = 0;
            if (key == OIS::KC_ESCAPE)
            {
                g_capture = NULL;           // отмена; окно не закрываем
                PlaceOwnButtons();
                return;
            }
            if (key == OIS::KC_BACK)
            {
                FinishCapture("NONE");
                return;
            }
            MCM_Key k;
            k.keycode = key;
            k.modifiers = 0;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) k.modifiers |= MCM_KEY_CTRL;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) k.modifiers |= MCM_KEY_SHIFT;
            if (GetAsyncKeyState(VK_MENU) & 0x8000) k.modifiers |= MCM_KEY_ALT;
            const std::string text = FormatKey(k);
            if (text.find("KEY") == std::string::npos)   // клавиши без имени - мимо
                FinishCapture(text);
            return;
        }
        // Esc/Enter в нашем поле закрывают окно: дописанное - сохранить,
        // фокус - снять, пока поле живо (см. DropFocusInside).
        if ((key == OIS::KC_ESCAPE || key == OIS::KC_RETURN || key == OIS::KC_NUMPADENTER) && g_panel != NULL)
        {
            MyGUI::InputManager* const input = MyGUI::InputManager::getInstancePtr();
            if (input != NULL && input->getKeyFocusWidget() != NULL && InsideOurTab(input->getKeyFocusWidget()))
            {
                Poll(true);
                DropFocusInside();
            }
        }
        g_origKeyDown(self, key);
    }

    void KeyUp_hook(InputHandler* self, OIS::KeyCode key)
    {
        if (g_capture != NULL && g_captureMod != 0 && key == g_captureMod)
        {
            FinishCapture(ModifierName(key));
            return;
        }
        g_origKeyUp(self, key);
    }
    void Poll(bool force);
    std::vector<std::pair<DataPanelLine*, Setting*> > g_actionLines;

    // Панель - заново на каждую страницу: игра очищает строки, но холст
    // прокрутки не уменьшает, и после длинной страницы короткая
    // прокручивалась бы в пустоту.
    bool SafeAttach(ModEntry* m)
    {
        __try
        {
            m->attach(m->areaUd, g_area);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    void SafeDetach(ModEntry* m)
    {
        __try
        {
            if (m->detach != NULL)
                m->detach(m->areaUd);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    // Отключить своё у прежней страницы - до того, как её виджеты уйдут.
    void DetachArea()
    {
        if (g_areaOwner != NULL)
        {
            SafeDetach(g_areaOwner);
            g_areaOwner = NULL;
        }
    }

    bool RecreatePanel(ModEntry* m)
    {
        if (g_right == NULL || gui == NULL)
            return false;
        DetachArea();
        DropFocusInside();
        if (g_panel != NULL)
        {
            g_panel->show(false);
            gui->destroy(g_panel);
            g_panel = NULL;
        }
        MyGUI::Gui& mygui = MyGUI::Gui::getInstance();
        if (g_area != NULL)
        {
            mygui.destroyWidget(g_area);
            g_area = NULL;
        }
        if (g_host != NULL)
        {
            mygui.destroyWidget(g_host);
            g_host = NULL;
        }
        const MyGUI::IntSize size = g_right->getSize();
        int top = 0;
        if (m != NULL && m->attach != NULL)
        {
            const int h = size.height * m->areaPercent / 100;
            g_area = g_right->createWidget<MyGUI::Widget>("PanelEmpty", MyGUI::IntCoord(0, 0, size.width, h),
                                                          MyGUI::Align::Default, "MCM_CustomArea");
            top = h + 6;
        }
        g_host = g_right->createWidget<MyGUI::Widget>("PanelEmpty",
            MyGUI::IntCoord(0, top, size.width, size.height - top), MyGUI::Align::Default, "MCM_Lines");
        g_panel = gui->createDatapanel("ModConfigMenuPanel", g_host, true);
        if (g_panel == NULL)
        {
            ErrorLog("ModConfigMenu: createDatapanel failed");
            return false;
        }
        g_panelRootRef->set(g_host);   // своя подложка: детей панели игра пересоздаёт
        if (g_window != NULL && g_window->tabs != NULL && g_tab != NULL)
            g_window->tabs->setItemData(g_tab, g_panel);
        return true;
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

    void ShowMod(ModEntry* m)
    {
        // Недописанное на прежней странице (ползунок, текст) - сейчас,
        // пока её строки живы.
        if (g_current != NULL && g_panel != NULL)
            Poll(true);
        g_current = m;
        g_resetLine = NULL;
        g_actionLines.clear();
        ForgetOwnButtons();
        for (size_t k = 0; k < g_buttons.size(); ++k)
            g_buttons[k]->setStateSelected(k < g_mods.size() && g_mods[k] == m);
        if (m != NULL)
            for (size_t k = 0; k < m->settings.size(); ++k)
            {
                m->settings[k]->line = NULL;
                m->settings[k]->hasBaseline = false;
                m->settings[k]->touched = false;
            }
        if (!RecreatePanel(m))
            return;

        g_panel->setLineSpacing(24.0f);
        if (m == NULL)
        {
            DataPanelLine* none = g_panel->setLine(Tr("No mods with settings were found."), "", CAT, false, true);
            if (none != NULL && none->w1 != NULL)
                none->w1->setTextColour(GameTheme::ReadableColour("Main", "#AFA68B"));
            g_panel->changeCategory(CAT);
            return;
        }

        bool first = true;
        for (size_t k = 0; k < m->settings.size(); ++k)
        {
            Setting* const s = m->settings[k];
            s->line = NULL;
            if (s->kind == K_SECTION)
            {
                if (!first)
                    g_panel->addSpace(CAT, SECTION_GAP);
                // Секция - светло-жёлтым и в квадратных скобках, цвет задан
                // явно: у скина строки по умолчанию текст тёмный, и на
                // ванильном интерфейсе заголовки были чёрными, еле видно
                // (07.10.2026). Оформление одно на все моды - задаёт MCM.
                DataPanelLine* head = g_panel->setLine("[ " + Upper(s->label) + " ]", "", CAT, false, true);
                if (head != NULL && head->w1 != NULL)
                    head->w1->setTextColour(MyGUI::Colour(0.93f, 0.86f, 0.56f));
                first = false;
                continue;
            }
            first = false;

            if (s->kind == K_ACTION)
            {
                // Строка - пустая подпись, кнопка - своя (см. OwnButton).
                // Панель игры хранит строки по тексту подписи: одинаковые
                // пустые подписи слипались в одну строку (все действия - в
                // одну кнопку). Подпись - разное число пробелов.
                DataPanelLine* l = g_panel->setLineTextEditable(std::string(k + 1, ' '), "", CAT, true, false,
                                                                MyGUI::Align::Left, 0.95f);
                s->line = l;
                if (l)
                    AddOwnButton(l, s, OWN_SETTING);
                continue;
            }

            s->committed = ReadIni(m, s);
            Load(s, s->committed);
            s->lastSeen = Current(s);
            s->lastMoveMs = 0;

            std::string label = s->label;
            std::string tip = s->tooltip;
            if (s->flags & MCM_RESTART)
            {
                label += " *";
                tip += std::string(tip.empty() ? "" : "\n") + Tr("* Takes effect after the game is restarted.");
            }

            MyGUI::Widget* tipWidget = NULL;
            switch (s->kind)
            {
            case K_TOGGLE:
            {
                DataPanelLine_CheckBox* l = g_panel->setLineCheckbox(label, &s->b, CAT);
                s->line = l;
                tipWidget = l ? l->getTextBox() : NULL;
                break;
            }
            case K_INT:
            case K_FLOAT:
            {
                DataPanelLine_SliderEditable* l =
                    g_panel->setLineSliderEditable(label, CAT, true, s->fmin, s->fmax, &s->f);
                if (l)
                    l->setPrecision(s->kind == K_INT ? 0 : s->decimals);
                s->line = l;
                tipWidget = l ? static_cast<MyGUI::Widget*>(l->nameText) : NULL;
                break;
            }
            case K_CHOICE:
            case K_HOTKEY:
            {
                // Вариант - кнопкой: щелчок - следующий, SHIFT+щелчок -
                // предыдущий. Клавиша - кнопкой: щелчок, потом нажать
                // клавишу (как в настройках управления игры). Выпадающий
                // список игры (DataPanelLine_DropBox) в окне настроек в игре
                // выбор не отдавал - 07.10.2026, после пяти попыток починить.
                DataPanelLine_TextEditable* l = g_panel->setLineTextEditable(label, "", CAT, true, false,
                                                                             MyGUI::Align::Left, 0.95f);
                s->line = l;
                if (l)
                    AddOwnButton(l, s, OWN_SETTING);
                tipWidget = l ? static_cast<MyGUI::Widget*>(l->getNameBox()) : NULL;
                break;
            }
            case K_COLOUR:
            {
                DataPanelLine_TextEditable* l = g_panel->setLineTextEditable(
                    label, EscapeTags(s->committed), CAT, true, false, MyGUI::Align::Left, 0.95f);
                s->line = l;
                tipWidget = l ? static_cast<MyGUI::Widget*>(l->getNameBox()) : NULL;
                s->lastSeen = s->committed;
                s->colourHasBase = false;
                if (l)
                {
                    AddOwnButton(l, s, OWN_COLOUR_MODE);
                    AddOwnButton(l, s, OWN_COLOUR_OPEN);
                    AddOwnButton(l, s, OWN_COLOUR_SWATCH);
                    if (s->colourOpen && !s->colourHex)
                    {
                        g_panel->addSpace(CAT, static_cast<float>(kPaletteLines));
                        if (DefaultIsWord(s))
                            AddOwnButton(l, s, OWN_COLOUR_CELL, -1);
                        for (int c = 0; c < kPaletteCount; ++c)
                            AddOwnButton(l, s, OWN_COLOUR_CELL, c);
                    }
                }
                break;
            }
            default:
            {
                // Решётка удвоена: иначе MyGUI принимает «#8C3A3A» за смену
                // цвета, и поле выглядит пустым. Поле ввода строка создаёт
                // позже, при показе, - править его здесь бесполезно.
                DataPanelLine_TextEditable* l = g_panel->setLineTextEditable(
                    label, EscapeTags(s->committed), CAT, true, false, MyGUI::Align::Left, 0.95f);
                s->line = l;
                tipWidget = l ? static_cast<MyGUI::Widget*>(l->getNameBox()) : NULL;
                s->lastSeen = s->committed;
                break;
            }
            }
            Tip(tipWidget, tip);
        }

        // Сброс - только у страниц на ini: значений по умолчанию у настроек
        // на функциях (API v3) MCM не знает.
        bool hasIni = false;
        for (size_t k = 0; k < m->settings.size() && !hasIni; ++k)
            hasIni = m->settings[k]->kind != K_SECTION && m->settings[k]->kind != K_ACTION &&
                     (!m->settings[k]->fn || m->settings[k]->fnHasDef);
        if (hasIni)
        {
            g_panel->addSpace(CAT, SECTION_GAP);
            DataPanelLine* reset = g_panel->setLineTextEditable(std::string(m->settings.size() + 2, ' '), "", CAT,
                                                                true, false, MyGUI::Align::Left, 0.95f);
            if (reset)
            {
                AddOwnButton(reset, NULL, OWN_RESET);
                g_resetLine = reset;
            }
        }
        g_panel->changeCategory(CAT);
        PlaceOwnButtons();                  // строки могли пересобраться

        // Своя область мода (API v4) - когда строки уже на месте.
        if (m->attach != NULL && g_area != NULL)
        {
            if (SafeAttach(m))
                g_areaOwner = m;
            else
                ErrorLog("ModConfigMenu: custom area crashed in " + m->id);
        }
    }

    void OnResetLine(DataPanelLine*)
    {
        ModEntry* const m = g_current;
        if (m == NULL)
            return;
        for (size_t k = 0; k < m->settings.size(); ++k)
        {
            Setting* const s = m->settings[k];
            if (s->kind == K_SECTION || s->kind == K_ACTION)
                continue;
            if (s->fn)
            {
                // Настройка на функциях (v3/v5): значение по умолчанию - через
                // setter плагина, он сам проверит, применит и сохранит.
                if (s->fnHasDef && !WriteFn(m->id, s, s->def))
                    ErrorLog("ModConfigMenu: reset of '" + s->label + "' in " + m->id + " was refused");
                s->line = NULL;
                continue;
            }
            Commit(m, s, s->def);
            // Строки этой страницы ещё держат прежние значения - отвязываем,
            // иначе запись перед сменой страницы вернула бы их в ini.
            s->line = NULL;
        }
        ApplyMod(m);
        g_pendingShow = m;          // перечитать страницу - в следующем кадре
        g_hasPending = true;
    }

    int SafeAction(MCM_ActionFn fn, void* ud, char* err, unsigned size)
    {
        __try
        {
            return fn(ud, err, size);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return -1;
        }
    }

    void OnActionLine(DataPanelLine* line)
    {
        for (size_t k = 0; k < g_actionLines.size(); ++k)
        {
            if (g_actionLines[k].first != line)
                continue;
            RunAction(g_actionLines[k].second);
            return;
        }
    }

    void RunAction(Setting* s)
    {
        {
            char err[512] = {};
            if (SafeAction(reinterpret_cast<MCM_ActionFn>(s->get), s->ud, err, sizeof(err) - 1) != 0)
                ErrorLog("ModConfigMenu: action '" + s->label + "' failed: " + err);
            else if (g_debug)
                DebugLog("ModConfigMenu: action '" + s->label + "' done");
            // Действие могло поменять значения - перечитать страницу
            // (в следующем кадре: кнопка живёт в самой панели).
            g_pendingShow = g_current;
            g_hasPending = true;
        }
    }

    void OnOwnButton(MyGUI::Widget* sender)
    {
        const size_t index = static_cast<size_t>(atoi(sender->getUserString("mcm_own").c_str()));
        if (index >= g_ownButtons.size())
            return;
        const OwnButton b = g_ownButtons[index];
        if (b.what == OWN_RESET)
        {
            OnResetLine(NULL);
            return;
        }
        Setting* const s = b.s;
        if (b.what >= OWN_COLOUR_MODE)
        {
            if (b.what == OWN_COLOUR_SWATCH)
                return;
            if (b.what == OWN_COLOUR_MODE)
                s->colourHex = !s->colourHex;
            else if (b.what == OWN_COLOUR_OPEN)
                s->colourOpen = !s->colourOpen;
            else if (g_current != NULL)
            {
                Poll(true);                 // недописанное в других полях
                Commit(g_current, s, b.arg < 0 ? s->def : std::string(kPalette[b.arg]));
                ApplyMod(g_current);
            }
            if (g_debug)
                DebugLog("ModConfigMenu: colour '" + s->label + "' part " + (b.what == OWN_COLOUR_CELL ? "cell" : "mode"));
            // Строки и место под палитру - заново, в следующем кадре (виджет
            // щелчка живёт в самой панели).
            g_pendingShow = g_current;
            g_hasPending = true;
            return;
        }
        if (s->kind == K_ACTION)
        {
            RunAction(s);
        }
        else if (s->kind == K_CHOICE)
        {
            const int n = static_cast<int>(s->values.size());
            if (n > 0)
            {
                const bool back = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                s->i = ((s->i < 0 ? 0 : s->i) + (back ? n - 1 : 1)) % n;
            }
            // запишет Poll в этом же кадре
        }
        else if (s->kind == K_HOTKEY)
        {
            g_capture = (g_capture == s) ? NULL : s;    // повторный щелчок - отмена
            g_captureMod = 0;
        }
        if (g_debug)
            DebugLog("ModConfigMenu: button '" + (s->label.empty() ? s->key : s->label) + "' clicked");
        PlaceOwnButtons();
    }

    void OnModButton(MyGUI::Widget* sender)
    {
        const std::string idx = sender->getUserString("mcm_index");
        const size_t k = static_cast<size_t>(atoi(idx.c_str()));
        if (k < g_mods.size() && g_mods[k] != g_current)
        {
            g_pendingShow = g_mods[k];
            g_hasPending = true;
        }
    }

    // Колесо над кнопкой модов: MyGUI отдаёт его кнопке, а не списку,
    // поэтому листаем список сами (setViewOffset сам упирается в края).
    void OnModListWheel(MyGUI::Widget*, int rel)
    {
        if (g_list == NULL)
            return;
        MyGUI::IntPoint offset = g_list->getViewOffset();
        offset.top += rel > 0 ? BUTTON_STEP : -BUTTON_STEP;
        g_list->setViewOffset(offset);
    }

    extern OptionsWindow* g_captionFor;     // заголовок окна - см. UpdateWindowCaption

    void BuildTab(OptionsWindow* w)
    {
        g_captionFor = NULL;                // окно строится заново - заголовок ищем снова
        g_window = w;
        g_tab = NULL;
        g_list = NULL;
        g_panel = NULL;
        g_right = NULL;
        g_host = NULL;
        g_area = NULL;
        DetachArea();
        g_hasPending = false;
        g_buttons.clear();
        g_current = NULL;
        if (w == NULL || w->tabs == NULL || gui == NULL)
            return;

        Discover();

        const size_t tabCount = w->tabs->getItemCount();
        g_tab = tabCount > 0 ? w->tabs->insertItemAt(tabCount - 1, "MCM")
                             : w->tabs->addItem("MCM");
        g_tabRef->set(g_tab);
        const MyGUI::IntSize size = g_tab->getSize();
        const int listW = std::max(200, size.width / 4);

        g_list = g_tab->createWidget<MyGUI::ScrollView>("Kenshi_ScrollViewEmpty",
            MyGUI::IntCoord(0, 0, listW, size.height),
            MyGUI::Align::Left | MyGUI::Align::VStretch, "MCM_ModList");
        g_list->setVisibleHScroll(false);
        g_list->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
        int y = 8;
        for (size_t k = 0; k < g_mods.size(); ++k)
        {
            MyGUI::Button* b = g_list->createWidget<MyGUI::Button>("Kenshi_Button1",
                MyGUI::IntCoord(4, y, listW - 24, BUTTON_H), MyGUI::Align::Default);
            // Подпись - оригинальное английское название; перевести его можно
            // в mod_config_menu.po (msgid - это название).
            b->setCaption(Tr(g_mods[k]->title.c_str()));
            Tip(b, g_mods[k]->info);
            char idx[16];
            sprintf_s(idx, "%u", static_cast<unsigned>(k));
            b->setUserString("mcm_index", idx);
            b->eventMouseButtonClick += MyGUI::newDelegate(&OnModButton);
            b->eventMouseWheel += MyGUI::newDelegate(&OnModListWheel);
            g_buttons.push_back(b);
            y += BUTTON_STEP;
        }
        // Холст не ниже окна списка: иначе ScrollView ставит кнопки посередине.
        // Модов больше, чем влезает, - холст выше окна, и справа появляется
        // полоса прокрутки скина; листать можно и колесом.
        g_list->setCanvasSize(listW - 20, std::max(y + (BUTTON_H - BUTTON_STEP) + 4, g_list->getViewCoord().height));

        // Разделитель между списком и настройками: две линии в пиксель,
        // тёмная и светлая, - как вдавленная кайма рамок игры. Готового
        // вертикального скина у игры нет (Kenshi_WhiteLine - горизонтальная
        // с завитками).
        {
            // 07.10.2026: толще - тёмная 2 px, светлая 2 px, тёмная 1 px.
            const int x = listW + LIST_GAP / 2 - 2;
            MyGUI::Widget* dark = g_tab->createWidget<MyGUI::Widget>("WhiteSkin",
                MyGUI::IntCoord(x, 4, 2, size.height - 8),
                MyGUI::Align::Left | MyGUI::Align::VStretch, "MCM_DividerDark");
            dark->setColour(MyGUI::Colour(0.05f, 0.05f, 0.05f));
            dark->setNeedMouseFocus(false);
            MyGUI::Widget* light = g_tab->createWidget<MyGUI::Widget>("WhiteSkin",
                MyGUI::IntCoord(x + 2, 4, 2, size.height - 8),
                MyGUI::Align::Left | MyGUI::Align::VStretch, "MCM_DividerLight");
            light->setColour(MyGUI::Colour(0.30f, 0.30f, 0.30f));
            light->setNeedMouseFocus(false);
            MyGUI::Widget* shade = g_tab->createWidget<MyGUI::Widget>("WhiteSkin",
                MyGUI::IntCoord(x + 4, 4, 1, size.height - 8),
                MyGUI::Align::Left | MyGUI::Align::VStretch, "MCM_DividerShade");
            shade->setColour(MyGUI::Colour(0.05f, 0.05f, 0.05f));
            shade->setNeedMouseFocus(false);
        }

        MyGUI::Widget* right = g_tab->createWidget<MyGUI::Widget>("PanelEmpty",
            MyGUI::IntCoord(listW + LIST_GAP, 0, size.width - listW - LIST_GAP, size.height),
            MyGUI::Align::Stretch, "MCM_Settings");
        g_right = right;

        if (g_debug)
        {
            char note[128];
            sprintf_s(note, "ModConfigMenu: tab %dx%d, list %d", size.width, size.height, listW);
            DebugLog(note);
        }
        ShowMod(g_mods.empty() ? NULL : g_mods[0]);
    }

    // Полоса прокрутки панели - только когда страница не влезает. Холст
    // панель пересчитывает сама, поэтому смотрим каждый кадр.
    void UpdateScrollBar()
    {
        if (g_panel == NULL || g_panel->scrollWin == NULL)
            return;
        MyGUI::ScrollView* const view = g_panel->scrollWin->castType<MyGUI::ScrollView>(false);
        if (view == NULL)
            return;
        const bool need = view->getCanvasSize().height > view->getViewCoord().height + 2;
        if (view->isVisibleVScroll() != need)
            view->setVisibleVScroll(need);
    }

    // Изменения ловим опросом. force - писать и недоехавший ползунок.
    void Poll(bool force)
    {
        ModEntry* const m = g_current;
        if (m == NULL)
            return;
        const DWORD now = GetTickCount();
        bool changed = false;
        for (size_t k = 0; k < m->settings.size(); ++k)
        {
            Setting* const s = m->settings[k];
            if (s->kind == K_SECTION || s->kind == K_ACTION || s->line == NULL)
                continue;
            const bool slider = s->kind == K_INT || s->kind == K_FLOAT;
            const bool typing = s->kind == K_TEXT || s->kind == K_COLOUR;
            if (typing)
            {
                DataPanelLine_TextEditable* const t = static_cast<DataPanelLine_TextEditable*>(s->line);
                if (t->getEditBox() == NULL)
                    continue;               // поле ещё не создано
                if (!s->hasBaseline)
                {
                    s->baseline = Current(s);
                    s->lastSeen = s->baseline;
                    s->hasBaseline = true;
                    continue;
                }
                if (MyGUI::InputManager::getInstance().getKeyFocusWidget() == t->getEditBox())
                    s->touched = true;
                if (!s->touched || Current(s) == s->baseline)
                    continue;               // игрок ничего не менял
            }
            const std::string v = Current(s);
            if (v == s->committed)
            {
                s->lastSeen = v;
                continue;
            }
            if ((slider || typing) && !force)
            {
                if (v != s->lastSeen)
                {
                    s->lastSeen = v;
                    s->lastMoveMs = now;
                    continue;
                }
                if (now - s->lastMoveMs < (typing ? 800u : 400u))
                    continue;
            }
            Commit(m, s, v);
            if (typing)
                s->baseline = v;
            changed = true;
        }
        if (changed)
            ApplyMod(m);
    }


    // ---------------------------------------------------------------
    // Хуки окна настроек
    // ---------------------------------------------------------------

    void (*g_origCreate)(OptionsWindow* self) = NULL;
    void (*g_origUpdate)(OptionsWindow* self) = NULL;
    void (*g_origSave)(OptionsWindow* self) = NULL;
    void (*g_origDatapanelDtor)(DatapanelGUI* self) = NULL;

    // Игра удаляет нашу панель сама - по Esc (07.10.2026: в главном меню
    // вышел из настроек, игра молча закрылась). Строки панели уходят
    // вместе с ней, а виджет вкладки - позже; в этот промежуток опрос
    // (Poll, HookChoice) лез в удалённые строки и портил память. Забываем
    // панель и всё, что на ней, в момент удаления.
    void DatapanelDtor_hook(DatapanelGUI* self)
    {
        if (self != NULL && self == g_panel)
        {
            if (g_debug)
                DebugLog("ModConfigMenu: our datapanel is being destroyed - lines forgotten");
            if (g_current != NULL)
                for (size_t k = 0; k < g_current->settings.size(); ++k)
                    g_current->settings[k]->line = NULL;
            g_actionLines.clear();
            ForgetOwnButtons();
            g_resetLine = NULL;
            g_panel = NULL;
            DropFocusInside();
        }
        g_origDatapanelDtor(self);
    }

    void BuildTabSafe(OptionsWindow* self)
    {
        __try
        {
            BuildTab(self);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("ModConfigMenu: building the Mods tab crashed; tab skipped");
            g_panel = NULL;
            g_current = NULL;
        }
    }

    void ShowModSafe(ModEntry* m)
    {
        __try
        {
            ShowMod(m);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ErrorLog("ModConfigMenu: showing a mod page crashed");
            g_panel = NULL;
            g_current = NULL;
        }
    }

    void Create_hook(OptionsWindow* self)
    {
        // Метки - чтобы по журналу было видно, упало ли внутри create или
        // уже после, на отрисовке (08.10.2026).
        if (g_debug)
            DebugLog("ModConfigMenu: options create - begin");
        g_origCreate(self);
        BuildTabSafe(self);
        if (g_debug)
            DebugLog("ModConfigMenu: options create - end");
    }

    // Заголовок окна настроек: на вкладке MCM - название выбранного мода.
    // У окна настроек скин Kenshi_Window, а в нём поля заголовка нет:
    // полоса сверху - только картинка, и setCaption окна ничего не
    // показывает. Поэтому кладём на полосу своё поле - то же, что в скине
    // Kenshi_WindowC у окон с названием («Трактирщик»): EditBox со скином
    // Kenshi_TextboxPaintedText, 3,3 от угла, высота 36. Создаём его как
    // часть скина окна (_createSkinWidget) - иначе оно попало бы в
    // клиентскую область под полосой.
    WidgetRef* g_captionLabel = new WidgetRef();   // не разрушается: MyGUI держит его в списке
    // Подложка под названием - картинка пользователя MCM_Caption.png в
    // корне мода (папка мода у игры - путь ресурсов). 512x64: плашка
    // x 110..401, y 10..54 (края с каймой по 12 px) и по бокам белые
    // гаснущие линии (y 29..35). Режем на пять частей: линия, левый край,
    // середина (растягивается по тексту), правый край, линия.
    WidgetRef* g_captionBack = new WidgetRef();

    const char* const CAPTION_TEXTURE = "MCM_Caption.png";
    const MyGUI::IntCoord CAPTION_PARTS[5] = {
        MyGUI::IntCoord(0, 29, 110, 6),         // линия слева
        MyGUI::IntCoord(110, 10, 12, 44),       // левый край плашки
        MyGUI::IntCoord(122, 10, 267, 44),      // середина
        MyGUI::IntCoord(389, 10, 12, 44),       // правый край
        MyGUI::IntCoord(401, 29, 111, 6)        // линия справа
    };

    void LayoutCaptionBack(MyGUI::Widget* back, const MyGUI::IntCoord& area, const MyGUI::IntSize& text)
    {
        if (back == NULL || back->getChildCount() < 5)
            return;
        const float scale = (text.height + 10) / 44.0f;
        // На пиксель ниже ровного счёта и на пиксель выше центра надписи -
        // подогнано по просьбе пользователя.
        const int plateH = static_cast<int>(44 * scale + 0.5f) - 1;
        const int cap = static_cast<int>(12 * scale + 0.5f);
        const int lineW = static_cast<int>(110 * scale + 0.5f);
        const int lineH = std::max(2, static_cast<int>(6 * scale + 0.5f));
        const int plateW = std::max(text.width + 40, 2 * cap + 4);
        const int cx = area.left + area.width / 2;
        const int cy = area.top + area.height / 2;     // поле надписи на пиксель выше - подложка на месте
        const int left = cx - plateW / 2;
        // Подогнано по просьбам пользователя: низ на пиксель выше ровного
        // счёта, верх - на пиксель выше прежнего.
        const int top = cy - plateH / 2 - 1;
        const int plateDrawH = plateH;
        // Линии - по середине нарисованной плашки, а не надписи: в краях
        // плашки есть начало той же линии, и при сдвигах плашки линии по
        // бокам расходились с ним - на стыке был излом.
        const int lineCy = top + plateDrawH / 2;
        back->getChildAt(0)->setCoord(left - lineW, lineCy - lineH / 2, lineW, lineH);
        back->getChildAt(1)->setCoord(left, top, cap, plateDrawH);
        back->getChildAt(2)->setCoord(left + cap, top, plateW - 2 * cap, plateDrawH);
        back->getChildAt(3)->setCoord(left + plateW - cap, top, cap, plateDrawH);
        back->getChildAt(4)->setCoord(left + plateW, lineCy - lineH / 2, lineW, lineH);
    }
    OptionsWindow* g_captionFor = NULL;
    const char* g_captionShownPtr = NULL;  // название, что сейчас в заголовке
    int g_captionFontHeight = 0;           // высота шрифта заголовка (крупный + 4)
    DWORD g_captionRefreshMs = 0;          // когда заголовок перерисовывали заново

    MyGUI::Window* FindOptionsWindowFrame(OptionsWindow* w)
    {
        for (MyGUI::Widget* p = w->tabs; p != NULL; p = p->getParent())
        {
            MyGUI::Window* const frame = p->castType<MyGUI::Window>(false);
            if (frame != NULL)
                return frame;
        }
        return NULL;
    }

    MyGUI::EditBox* CaptionLabel(OptionsWindow* w)
    {
        if (g_captionFor != w && g_captionLabel->get() != NULL &&
            g_captionLabel->get()->getParent() == FindOptionsWindowFrame(w))
        {
            // Окно открыли снова, рамка та же - поле и подложка на ней живы:
            // берём их, а не плодим новые на каждое открытие (08.10.2026).
            g_captionFor = w;
            g_captionShownPtr = NULL;
        }
        if (g_captionFor != w)
        {
            // Окно настроек собрано заново: прежнее поле (если живо) прячем
            // и забываем, на новом окне создадим своё.
            if (g_captionLabel->get() != NULL)
                g_captionLabel->get()->setVisible(false);
            if (g_captionBack->get() != NULL)
                g_captionBack->get()->setVisible(false);
            g_captionFor = w;
            g_captionLabel->reset();
            g_captionBack->reset();
            g_captionShownPtr = NULL;
        }
        MyGUI::Widget* label = g_captionLabel->get();
        if (label == NULL)
        {
            MyGUI::Window* const frame = FindOptionsWindowFrame(w);
            if (frame == NULL)
                return NULL;
            // Подложка - первой: что создано позже, рисуется поверх.
            MyGUI::Widget* const back = frame->_createSkinWidget(MyGUI::WidgetStyle::Child,
                MyGUI::Widget::getClassTypeName(), "PanelEmpty",
                MyGUI::IntCoord(0, 0, frame->getWidth(), 64), MyGUI::Align::HStretch | MyGUI::Align::Top,
                "", "MCM_WindowCaptionBack");
            if (back != NULL)
            {
                back->setNeedMouseFocus(false);
                for (int i = 0; i < 5; ++i)
                {
                    MyGUI::ImageBox* const part = back->createWidget<MyGUI::ImageBox>(
                        "ImageBox", MyGUI::IntCoord(0, 0, 1, 1), MyGUI::Align::Default);
                    part->setNeedMouseFocus(false);
                    part->setImageTexture(CAPTION_TEXTURE);
                    part->setImageCoord(CAPTION_PARTS[i]);
                }
                back->setVisible(false);
                g_captionBack->set(back);
            }
            label = frame->_createSkinWidget(MyGUI::WidgetStyle::Child, MyGUI::EditBox::getClassTypeName(),
                "Kenshi_TextboxPaintedText", MyGUI::IntCoord(3, 8, frame->getWidth() - 6, 40),
                MyGUI::Align::HStretch | MyGUI::Align::Top, "", "MCM_WindowCaption");
            if (label == NULL)
                return NULL;
            label->setNeedMouseFocus(false);
            MyGUI::EditBox* const edit = label->castType<MyGUI::EditBox>(false);
            if (edit != NULL)
            {
                edit->setEditStatic(true);
                edit->setTextAlign(MyGUI::Align::Center);
                // Крупный шрифт заголовков игры и светло-жёлтый цвет (просьба
                // пользователя); по умолчанию у скина средний шрифт и
                // тёмно-серый цвет, на полосе он почти не читался.
                edit->setFontName("Kenshi_PaintedTextFont_Large");
                // Крупнее самого крупного «рисованного» шрифта игры нет -
                // растягиваем его на четыре единицы.
                edit->setFontHeight(edit->getFontHeight() + 4);
                g_captionFontHeight = edit->getFontHeight();
                edit->setTextColour(MyGUI::Colour(0.93f, 0.86f, 0.56f));
            }
            label->setVisible(false);
            g_captionLabel->set(label);
            g_captionShownPtr = NULL;
        }
        return label->castType<MyGUI::EditBox>(false);
    }

    // Название мода в заголовке - только пока открыта вкладка MCM. Прячем
    // при любом уходе с неё, в том числе когда вкладку снесли или окно
    // открыли заново на другой вкладке (08.10.2026: название оставалось).
    void HideWindowCaption()
    {
        MyGUI::Widget* const label = g_captionLabel->get();
        if (label != NULL && label->getVisible())
            label->setVisible(false);
        MyGUI::Widget* const back = g_captionBack->get();
        if (back != NULL && back->getVisible())
            back->setVisible(false);
    }

    void UpdateWindowCaption(OptionsWindow* w)
    {
        if (w == NULL || w->tabs == NULL || g_tab == NULL)
        {
            HideWindowCaption();
            return;
        }

        const bool onMcm = w->tabs->getItemSelected() == g_tab && g_current != NULL;
        if (!onMcm)
        {
            HideWindowCaption();
            return;
        }

        MyGUI::EditBox* const label = CaptionLabel(w);
        if (label == NULL)
            return;
        // Tr отдаёт постоянный указатель на строку словаря - сравниваем его,
        // а не собираем строку каждый кадр, пока открыта вкладка.
        const char* const title = Tr(g_current->title.c_str());
        MyGUI::Widget* const back = g_captionBack->get();
        // Раз в секунду - заново шрифт, высота и текст: игра иногда
        // перестраивает текстуру шрифта, и надпись, собранная раньше,
        // превращалась в кашу из обрывков букв (10.10.2026, снимок
        // пользователя). Свои надписи игра после этого ставит заново, нашу -
        // нет. Одна надпись раз в секунду - копейки.
        const DWORD now = GetTickCount();
        if (title != g_captionShownPtr || now - g_captionRefreshMs > 1000)
        {
            g_captionRefreshMs = now;
            label->setFontName("Kenshi_PaintedTextFont_Large");
            if (g_captionFontHeight > 0)
                label->setFontHeight(g_captionFontHeight);
            label->setCaption("");
            label->setCaption(EscapeTags(title));
            g_captionShownPtr = title;

            // Плашка - по тексту: поля по 20 px, высота с запасом в 10 px,
            // по центру поля надписи; линии - по бокам от неё.
            LayoutCaptionBack(back, label->getCoord(), label->getTextSize());
        }
        if (!label->getVisible())
            label->setVisible(true);
        if (back != NULL && !back->getVisible())
            back->setVisible(true);
    }

    // Вкладку или панель снесли без нас - забыть всё, что на них
    // указывало. Окно построится заново в следующем create.
    void ForgetIfDestroyed()
    {
        if (g_window == NULL)
            return;
        const bool tabGone = g_tab != NULL && g_tabRef->get() == NULL;
        const bool panelGone = g_panel != NULL && g_panelRootRef->get() == NULL;
        if (!tabGone && !panelGone)
            return;
        DebugLog("ModConfigMenu: the options window was torn down from outside - tab state reset");
        HideWindowCaption();
        g_areaOwner = NULL;                 // его виджетов уже нет - отключать нечего
        g_window = NULL;
        g_tab = NULL;
        g_list = NULL;
        g_panel = NULL;
        g_right = NULL;
        g_host = NULL;
        g_area = NULL;
        g_hasPending = false;
        g_pendingShow = NULL;
        g_buttons.clear();
        g_current = NULL;
        g_resetLine = NULL;
    }

    void Update_hook(OptionsWindow* self)
    {
        g_origUpdate(self);
        ForgetIfDestroyed();
        if (g_debug)
        {
            static DWORD s_lastNoteMs = 0;
            const DWORD noteNow = GetTickCount();
            if (noteNow - s_lastNoteMs > 2000)
            {
                s_lastNoteMs = noteNow;
                char line[200];
                sprintf_s(line, "ModConfigMenu: update window=%p ours=%p panel=%p page=%s",
                          static_cast<void*>(self), static_cast<void*>(g_window), static_cast<void*>(g_panel),
                          g_current != NULL ? g_current->id.c_str() : "-");
                DebugLog(line);
            }
        }
        const DWORD nowMs = GetTickCount();
        if (self == g_window && g_areaOwner != NULL && g_area != NULL && nowMs - g_lastUpdateMs > 500)
        {
            // Окно открыли снова (update не шёл) - пусть мод перечитает данные.
            if (!SafeAttach(g_areaOwner))
                g_areaOwner = NULL;
        }
        g_lastUpdateMs = nowMs;
        if (self == g_window && g_hasPending)
        {
            g_hasPending = false;
            MyGUI::ScrollView* const before = PanelScroll(g_panel);
            g_scrollUntil = 0;
            if (g_debug && g_panel != NULL)
            {
                MyGUI::ScrollView* const win = g_panel->scrollWin != NULL
                    ? g_panel->scrollWin->castType<MyGUI::ScrollView>(false) : NULL;
                char line[200];
                sprintf_s(line, "ModConfigMenu: rebuild, scrollWin %p top %d, scrollView %p top %d",
                          static_cast<void*>(win), win != NULL ? win->getViewOffset().top : -1,
                          static_cast<void*>(g_panel->scrollView),
                          g_panel->scrollView != NULL ? g_panel->scrollView->getViewOffset().top : -1);
                DebugLog(line);
            }
            if (before != NULL && g_pendingShow != NULL && g_pendingShow == g_current &&
                before->getViewOffset().top != 0)
            {
                g_scrollToKeep = before->getViewOffset();
                g_scrollLastSet = MyGUI::IntPoint(0, 0);
                g_scrollUntil = nowMs + 1500;
            }
            ShowModSafe(g_pendingShow);
        }
        if (self == g_window && g_panel != NULL)
        {
            UpdateScrollBar();
            if (g_scrollUntil != 0)
            {
                MyGUI::ScrollView* const view = PanelScroll(g_panel);
                if (view == NULL || nowMs > g_scrollUntil)
                {
                    g_scrollUntil = 0;
                }
                else
                {
                    const MyGUI::IntPoint cur = view->getViewOffset();
                    // Наверху (игра сбросила) или там, где мы оставили
                    // (холст не дорос) - поставить ещё раз; иначе крутил игрок.
                    if (cur != g_scrollToKeep && (cur.top == 0 || cur == g_scrollLastSet))
                    {
                        view->setViewOffset(g_scrollToKeep);
                        g_scrollLastSet = view->getViewOffset();
                        if (g_debug)
                        {
                            char line[160];
                            sprintf_s(line, "ModConfigMenu: scroll back %d -> %d (got %d)", cur.top,
                                      g_scrollToKeep.top, g_scrollLastSet.top);
                            DebugLog(line);
                        }
                    }
                    else if (cur != g_scrollToKeep && cur != g_scrollLastSet)
                    {
                        g_scrollUntil = 0;  // крутит игрок
                    }
                }
            }
            PlaceOwnButtons();
            Poll(false);
        }
        if (self == g_window)
            UpdateWindowCaption(self);
        else
            HideWindowCaption();            // нашей вкладки в этом окне нет
    }

    void Save_hook(OptionsWindow* self)
    {
        ForgetIfDestroyed();
        if (self == g_window && g_panel != NULL)
            Poll(true);
        g_origSave(self);
    }
}


// Открыть окно настроек на странице мода (для плагинов, у которых своя
// клавиша вызова - Hidden-Faction-Relations).
extern "C" __declspec(dllexport) void __cdecl MCM_OpenPage(const char* modId)
{
    OptionsWindow* const w = OptionsWindow::getSingleton();
    if (w == NULL || modId == NULL)
        return;
    ForgetIfDestroyed();
    if (!w->isVisible())
        w->show();
    if (w != g_window || g_tab == NULL || w->tabs == NULL)
        return;
    w->tabs->setItemSelected(g_tab);
    for (size_t k = 0; k < g_mods.size(); ++k)
        if (g_mods[k]->id == modId)
        {
            g_pendingShow = g_mods[k];
            g_hasPending = true;
            break;
        }
}


__declspec(dllexport) void startPlugin()
{
    const std::string ini = ModuleDir() + "ModConfigMenu.ini";
    g_debug = GetPrivateProfileIntA("ModConfigMenu", "Debug", 0, ini.c_str()) != 0;

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&OptionsWindow::create), Create_hook, &g_origCreate))
    {
        ErrorLog("ModConfigMenu: could not hook OptionsWindow::create, no Mods tab");
        return;
    }
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&OptionsWindow::_NV_update), Update_hook, &g_origUpdate))
        ErrorLog("ModConfigMenu: could not hook OptionsWindow::update, changes saved only on close");
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InputHandler::keyDownEvent), KeyDown_hook, &g_origKeyDown))
        ErrorLog("ModConfigMenu: could not hook InputHandler::keyDownEvent - keys cannot be assigned");
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InputHandler::keyUpEvent), KeyUp_hook, &g_origKeyUp))
        ErrorLog("ModConfigMenu: could not hook InputHandler::keyUpEvent");
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&DatapanelGUI::_DESTRUCTOR), DatapanelDtor_hook, &g_origDatapanelDtor))
        ErrorLog("ModConfigMenu: could not hook DatapanelGUI destructor");
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&OptionsWindow::saveOptions), Save_hook, &g_origSave))
        ErrorLog("ModConfigMenu: could not hook OptionsWindow::saveOptions");
    DebugLog("ModConfigMenu: installed");
}
