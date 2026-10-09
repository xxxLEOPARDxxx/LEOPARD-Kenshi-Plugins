// ModConfigMenu (MCM) - настройки модов во вкладке MCM окна «Настройки».
//
// Плагину MCM не обязателен: настройки по-прежнему лежат в его ini и
// читаются им самим. Если MCM установлен, он находит у плагина экспорт
// MCM_Describe и строит по нему страницу. Порядок загрузки не важен: MCM
// опрашивает плагины, когда игра создаёт окно настроек.
//
// Подключение в плагине:
//
//   #include <ModConfigMenu.h>
//
//   extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
//   {
//       api->beginMod(api, "WantedMap", Tr("Wanted posters on the map"),
//                     iniPath.c_str(), &ReloadSettings);
//       api->section(api, Tr("Markers"));
//       api->toggle(api, "WantedMap", "ShowLabels", Tr("Name next to the marker"),
//                   Tr("Tooltip text"), 1, 0);
//       api->integer(api, "WantedMap", "ScanSeconds", Tr("Poster check, s"),
//                    NULL, 2, 1, 60, 0);
//   }
//
// Все строки - UTF-8; MCM их копирует, хранить не нужно. Подписи плагин
// передаёт уже переведёнными (через свой Tr). Изменённое значение MCM сразу
// пишет в ini плагина (WritePrivateProfileString) и вызывает apply - там
// плагин перечитывает ini. Что нельзя применить на ходу, помечается
// флагом MCM_RESTART: подпись получает звёздочку, подсказка - пометку.
//
// Совместимость: поля MCM_Api только дописываются в конец, version растёт.
// Плагин, которому нужно поле новее, проверяет api->version.

#pragma once

#define MCM_API_VERSION 5

// Флаги настройки
#define MCM_RESTART 0x1u   // действует после перезапуска игры

struct MCM_Api;
typedef void (__cdecl *MCM_ApplyFn)();
typedef void (__cdecl *MCM_DescribeFn)(MCM_Api* api);

// --- версия 3: настройки на функциях вместо ini ---
// Для плагинов, которые хранят настройки сами (JSON и т.п.) и умеют их
// проверять и применять. Возврат 0 - успех, иначе ошибка: текст ошибки
// плагин пишет в err (размер errSize), MCM кладёт его в журнал и
// перечитывает значение. Сигнатуры совпадают с Mod Hub (EMC_*Callback),
// так что его функции подходят без обёрток.
struct MCM_Key { int keycode; unsigned modifiers; };   // keycode - OIS::KeyCode, -1 - не задана
#define MCM_KEY_CTRL  0x1u
#define MCM_KEY_SHIFT 0x2u
#define MCM_KEY_ALT   0x4u
typedef int (__cdecl *MCM_GetIntFn)(void* ud, int* out);
typedef int (__cdecl *MCM_SetIntFn)(void* ud, int value, char* err, unsigned errSize);
typedef int (__cdecl *MCM_GetFloatFn)(void* ud, float* out);
typedef int (__cdecl *MCM_SetFloatFn)(void* ud, float value, char* err, unsigned errSize);
typedef int (__cdecl *MCM_GetTextFn)(void* ud, char* out, unsigned outSize);
typedef int (__cdecl *MCM_SetTextFn)(void* ud, const char* value, char* err, unsigned errSize);
typedef int (__cdecl *MCM_GetKeyFn)(void* ud, MCM_Key* out);
typedef int (__cdecl *MCM_SetKeyFn)(void* ud, MCM_Key value, char* err, unsigned errSize);
typedef int (__cdecl *MCM_ActionFn)(void* ud, char* err, unsigned errSize);

// --- версия 4: своя область на странице ---
// parent - MyGUI::Widget*, в котором плагин рисует что хочет (список,
// таблицу...). attach зовётся при показе страницы и при каждом новом
// открытии окна настроек; detach - когда со страницы ушли или окно
// пересоздаётся: виджеты внутри parent плагин убирает сам.
typedef void (__cdecl *MCM_AttachFn)(void* ud, void* parent);
typedef void (__cdecl *MCM_DetachFn)(void* ud);

// Открыть окно настроек на странице мода (экспорт ModConfigMenu.dll):
//   typedef void (__cdecl *MCM_OpenPageFn)(const char* modId);
//   GetProcAddress(GetModuleHandleA("ModConfigMenu.dll"), "MCM_OpenPage")
typedef void (__cdecl *MCM_OpenPageFn)(const char* modId);

struct MCM_Api
{
    int version;
    void* impl;

    // Начать страницу мода. id - латиницей, уникальный; title - подпись в
    // списке модов: оригинальное английское название мода, без перевода
    // (перевести его можно каталогом самого MCM, mod_config_menu.po);
    // iniPath - полный путь к ini; apply - может быть NULL.
    void (__cdecl *beginMod)(MCM_Api* api, const char* id, const char* title,
                             const char* iniPath, MCM_ApplyFn apply);
    // Подзаголовок внутри страницы.
    void (__cdecl *section)(MCM_Api* api, const char* title);
    // 0/1 в ini.
    void (__cdecl *toggle)(MCM_Api* api, const char* iniSection, const char* key,
                           const char* label, const char* tooltip, int def, unsigned flags);
    // Целое в пределах [min, max], ползунок с полем ввода.
    void (__cdecl *integer)(MCM_Api* api, const char* iniSection, const char* key,
                            const char* label, const char* tooltip, int def, int min, int max,
                            unsigned flags);
    // Дробное в пределах [min, max]; decimals - знаков после точки в ini.
    void (__cdecl *number)(MCM_Api* api, const char* iniSection, const char* key,
                           const char* label, const char* tooltip, float def, float min,
                           float max, int decimals, unsigned flags);
    // Выбор из списка: values - что пишется в ini, labels - что видно
    // (labels может быть NULL - тогда видно values).
    void (__cdecl *choice)(MCM_Api* api, const char* iniSection, const char* key,
                           const char* label, const char* tooltip, const char* def,
                           const char* const* values, const char* const* labels, int count,
                           unsigned flags);
    // Произвольная строка.
    void (__cdecl *text)(MCM_Api* api, const char* iniSection, const char* key,
                         const char* label, const char* tooltip, const char* def, unsigned flags);
    // Сочетание клавиш в виде текста ("SHIFT+B", "F12").
    void (__cdecl *hotkey)(MCM_Api* api, const char* iniSection, const char* key,
                           const char* label, const char* tooltip, const char* def, unsigned flags);
    // Цвет #RRGGBB (или "auto", если плагин это понимает).
    void (__cdecl *colour)(MCM_Api* api, const char* iniSection, const char* key,
                           const char* label, const char* tooltip, const char* def, unsigned flags);

    // --- версия 2 ---
    // Кратко, что делает мод: подсказка при наведении на него в списке.
    // Уже переведённая. Проверять: if (api->version >= 2) api->info(...).
    void (__cdecl *info)(MCM_Api* api, const char* text);

    // --- версия 3: строки на функциях (страницу начинать beginMod с
    // iniPath = NULL; кнопки «Сбросить по умолчанию» у такой страницы нет -
    // значений по умолчанию MCM не знает). Проверять api->version >= 3.
    void (__cdecl *toggleFn)(MCM_Api* api, const char* label, const char* tooltip,
                             MCM_GetIntFn get, MCM_SetIntFn set, void* ud, unsigned flags);
    void (__cdecl *integerFn)(MCM_Api* api, const char* label, const char* tooltip,
                              MCM_GetIntFn get, MCM_SetIntFn set, void* ud, int min, int max,
                              unsigned flags);
    void (__cdecl *numberFn)(MCM_Api* api, const char* label, const char* tooltip,
                             MCM_GetFloatFn get, MCM_SetFloatFn set, void* ud, float min,
                             float max, int decimals, unsigned flags);
    // Выбор: values - числа, которые получает и отдаёт плагин; labels - что видно.
    void (__cdecl *choiceFn)(MCM_Api* api, const char* label, const char* tooltip,
                             MCM_GetIntFn get, MCM_SetIntFn set, void* ud, const int* values,
                             const char* const* labels, int count, unsigned flags);
    void (__cdecl *textFn)(MCM_Api* api, const char* label, const char* tooltip,
                           MCM_GetTextFn get, MCM_SetTextFn set, void* ud, unsigned flags);
    void (__cdecl *colourFn)(MCM_Api* api, const char* label, const char* tooltip,
                             MCM_GetTextFn get, MCM_SetTextFn set, void* ud, unsigned flags);
    // Клавиша: в поле - текстом вида CTRL+SHIFT+B, NONE - не задана.
    void (__cdecl *hotkeyFn)(MCM_Api* api, const char* label, const char* tooltip,
                             MCM_GetKeyFn get, MCM_SetKeyFn set, void* ud, unsigned flags);
    // Кнопка действия. После нажатия страница перечитывается.
    void (__cdecl *action)(MCM_Api* api, const char* label, const char* tooltip,
                           MCM_ActionFn run, void* ud, unsigned flags);

    // --- версия 4 ---
    // Своя область вверху страницы, heightPercent - её доля высоты (10-90);
    // строки настроек - под ней. Проверять api->version >= 4.
    void (__cdecl *customArea)(MCM_Api* api, MCM_AttachFn attach, MCM_DetachFn detach, void* ud,
                               int heightPercent);

    // --- версия 5 ---
    // Значение по умолчанию для последней добавленной строки на функциях
    // (v3) - строкой, как её понимает MCM: 0/1, число, число варианта
    // выбора, текст, клавиша CTRL+B, цвет #RRGGBB. С ним у страницы есть
    // «Сбросить по умолчанию»: значение уходит через setter строки.
    void (__cdecl *defaultValue)(MCM_Api* api, const char* value);
};
