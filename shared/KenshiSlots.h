#pragma once

// Адреса функций Kenshi, которых установленная KenshiLib не экспортирует.
//
// ЗАЧЕМ. KenshiLib 0.5.0, которая лежит в игре, экспортирует не все
// функции, до которых дотянулось реверс-инжиниринговое описание. Ровно
// тех, что нужны для перемещения зданий в редакторе, среди экспортов и
// нет: TransformWindow::show, updateState, updateGizmo,
// LevelEditor::update, updateGizmo, setSelectedObject,
// InteriorModeButtonWindow::setSelectedBuilding. Заголовки в репозитории
// их объявляют - потому что исходники KenshiLib новее установленной
// DLL, - но линковаться не с чем.
//
// КАК ОБХОДИМ. Не экспортирована заглушка, а не адрес. KenshiLib держит
// все адреса игры в одной таблице function_pointers: при запуске она
// читает RE_Kenshi\RVAs\<платформа>_<версия>.br и заполняет её. Каждый
// экспорт DLL - это шестибайтовая заглушка «jmp qword ptr [ячейка]».
// Значит, по любому известному экспорту находится адрес его ячейки, а по
// нему - начало таблицы. Дальше берём любую ячейку по номеру.
//
// Версия игры при этом роли не играет: таблицу наполняет сама KenshiLib
// тем, что подходит установленному бинарнику.
//
// ОТКУДА НОМЕРА. Из исходников KenshiLib: symbols.asm задаёт начало
// каждого класса, .inc-файлы - смещение метода внутри класса. Наши
// исходники новее DLL, и сквозная нумерация разъехалась (первое
// расхождение уже на ячейке 1060), но ВНУТРИ класса порядок тот же -
// сдвиг постоянный и вычисляется по экспортированным методам того же
// класса. Восстановленные так адреса совпали с RVA из комментариев
// заголовков по всем одиннадцати проверенным функциям.
//
// ПОЧЕМУ ОБЯЗАТЕЛЬНА САМОПРОВЕРКА. Номера привязаны к конкретной сборке
// KenshiLib. Выйдет 0.5.1 - они поедут, и вызов уйдёт в чужую функцию:
// это не ошибка компоновки, а тихий вылет. Поэтому Init() сверяет
// несколько ЭКСПОРТИРОВАННЫХ имён с их ожидаемыми номерами и при первом
// же несовпадении отказывается работать.

#include <Windows.h>

#include <cstdio>
#include <string>

#include <Debug.h>


namespace KenshiSlots
{
    // Номера ячеек для KenshiLib 0.5.0.
    enum Slot
    {
        // экспортированные - только для самопроверки
        CHECK_TransformWindow_getSingleton      = 6963,
        CHECK_TransformWindow_close             = 6969,
        CHECK_TransformWindow_isVisible         = 6973,
        CHECK_LevelEditor_close                 = 4521,
        CHECK_LevelEditor_levelEditMode         = 4527,
        CHECK_LevelEditor_isInteriorEditMode    = 4528,
        CHECK_Interior_getSelectedBuilding      = 4148,
        CHECK_PlayerInterface_isLevelEditMode   = 5559,
        CHECK_PlayerInterface_getLevelEditor    = 5562,
        CHECK_ManagementScreen_printResearch    = 4753,
        CHECK_CharStats_getStat                 = 2166,

        // ради чего всё затевалось
        TransformWindow_show                    = 6966,
        TransformWindow_setCaption              = 6967,
        TransformWindow_updateState             = 6968,
        TransformWindow_updateGizmo             = 6971,
        TransformWindow_setMode                 = 6977,
        LevelEditor_update                      = 4522,
        LevelEditor_updateGizmo                 = 4523,
        LevelEditor_setSelectedObject           = 4551,
        Interior_setSelectedBuilding            = 4147,
        Interior_toggleInteriorMode             = 4144,

        // Перестройка описания исследования. Нужна, чтобы вернуть на
        // место прокрутку: игра перезаполняет панель и сбрасывает её.
        ManagementScreen_refreshResearchListDescription = 4757,

        // Начисление опыта (XP_Overhaul Асура). Автор собирал под
        // KenshiLib, которая их экспортирует; наша 0.5.0 - ни одной.
        // Сдвиг нумерации для CharStats нулевой по всем 93 экспортам
        // класса, адреса совпали с RVA из заголовков.
        CharStats_xpStat_timeBased              = 2172,
        CharStats_xpStat_eventBased             = 2173,
        CharStats_xpMelee                       = 2219,
        CharStats_xpToughness_RagdollEvent      = 2220,
        CharStats_xpToughness_GetUpEvent        = 2221,
        CharStats_xpToughness_PunchSomething    = 2222,
        CharStats_xpFirstAid                    = 2223,
        CharStats_xpRunning                     = 2224,
        CharStats_xpStealth                     = 2225,
        CharStats_xpStealthHearCheckEvent       = 2226,
        CharStats_xpMassCombat                  = 2228,
        CharStats_xpEngineering                 = 2229,
        CharStats_xpLockpicking                 = 2230,
        CharStats_xpTraining                    = 2231,
        CharStats_xpGeneral                     = 2232
    };


    struct Anchor
    {
        const char* name;
        int slot;
    };


    // Адрес ЯЧЕЙКИ, на которую прыгает экспортированная заглушка.
    // Ноль - имени нет или заглушка непривычного вида.
    inline uintptr_t* StubCell(HMODULE lib, const char* name)
    {
        const unsigned char* code =
            reinterpret_cast<const unsigned char*>(GetProcAddress(lib, name));

        if (code == NULL)
            return NULL;

        // ff 25 <rel32> = jmp qword ptr [rip + rel32]
        if (code[0] != 0xFF || code[1] != 0x25)
            return NULL;

        int rel = 0;
        memcpy(&rel, code + 2, 4);

        return reinterpret_cast<uintptr_t*>(
            const_cast<unsigned char*>(code) + 6 + rel);
    }


    inline uintptr_t* FindTable()
    {
        static uintptr_t* table = NULL;
        static bool tried = false;

        if (tried)
            return table;
        tried = true;

        HMODULE lib = GetModuleHandleA("KenshiLib.dll");
        if (lib == NULL)
        {
            ErrorLog("KenshiSlots: KenshiLib.dll is not loaded");
            return NULL;
        }

        static const Anchor anchors[] =
        {
            { "?getSingleton@TransformWindow@@SAPEAV1@XZ",
              CHECK_TransformWindow_getSingleton },
            { "?close@TransformWindow@@QEAAXXZ",
              CHECK_TransformWindow_close },
            { "?isVisible@TransformWindow@@QEBA_NXZ",
              CHECK_TransformWindow_isVisible },
            { "?close@LevelEditor@@QEAAXXZ",
              CHECK_LevelEditor_close },
            { "?levelEditMode@LevelEditor@@QEAAX_N@Z",
              CHECK_LevelEditor_levelEditMode },
            { "?isInteriorEditMode@LevelEditor@@QEAA_NXZ",
              CHECK_LevelEditor_isInteriorEditMode },
            { "?getSelectedBuilding@InteriorModeButtonWindow@@QEAAPEAVBuilding@@XZ",
              CHECK_Interior_getSelectedBuilding },
            { "?isLevelEditMode@PlayerInterface@@QEBA_NXZ",
              CHECK_PlayerInterface_isLevelEditMode },
            { "?getLevelEditor@PlayerInterface@@QEAAPEAVLevelEditor@@XZ",
              CHECK_PlayerInterface_getLevelEditor },
            { "?printResearch@ManagementScreen@@SAXPEAVDatapanelGUI@@PEAVGameData@@"
              "AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@2H@Z",
              CHECK_ManagementScreen_printResearch },
            { "?getStat@CharStats@@QEBAMW4StatsEnumerated@@_N@Z",
              CHECK_CharStats_getStat }
        };

        const int count = sizeof(anchors) / sizeof(anchors[0]);

        // Начало таблицы считаем по первому якорю...
        uintptr_t* first = StubCell(lib, anchors[0].name);
        if (first == NULL)
        {
            ErrorLog("KenshiSlots: cannot read the export stub for "
                     + std::string(anchors[0].name));
            return NULL;
        }

        uintptr_t* candidate = first - anchors[0].slot;

        // ...и требуем, чтобы по нему сошлись все остальные. Это и есть
        // проверка, что нумерация KenshiLib не изменилась.
        for (int i = 1; i < count; ++i)
        {
            uintptr_t* cell = StubCell(lib, anchors[i].name);
            if (cell == candidate + anchors[i].slot)
                continue;

            char note[320];
            sprintf_s(note,
                "KenshiSlots: slot numbering does not match this KenshiLib "
                "(%s expected #%d), giving up - the plugin stays off",
                anchors[i].name, anchors[i].slot);
            ErrorLog(note);
            return NULL;
        }

        table = candidate;
        return table;
    }


    // Адрес функции игры по номеру ячейки. NULL - таблица не опознана
    // или в ячейке пусто.
    inline void* At(int slot)
    {
        uintptr_t* table = FindTable();
        if (table == NULL)
            return NULL;

        const uintptr_t address = table[slot];
        return address ? reinterpret_cast<void*>(address) : NULL;
    }


    inline bool Ready()
    {
        return FindTable() != NULL;
    }
}
