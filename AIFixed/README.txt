AI Fixed (QoL)

[Nexus — our release / наша версия](https://www.nexusmods.com/kenshi/mods/2256)

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 1 (30.10.2026)

  * Спасатель предпочитает подходящую свободную ремкровать с питанием
    в том же поселении, когда несёт раненого скелета. Сначала выполняется
    обычный поиск игры; если ремкровати нет, остаётся его выбор.
    Своя мебель имеет приоритет перед союзной, затем выбирается ближайшая.
  * Проверка «вылечился — можно вставать» учитывает роботизированные части.
    В работающей ремкровати персонаж остаётся до восстановления прочности
    и снятия износа; потерянные части не требуется восстанавливать.
    Если кровать не чинит, например без питания, сохраняется решение игры.
  * Персонажи игрока могут сами идти чинить износ: скелеты и живые
    с протезами. По умолчанию включено, порог 50. Нужна свободная
    работающая ремкровать в том же поселении. Проверка не чаще раза в 3 с;
    несколько персонажей не получают одну кровать одновременно.
    Отмена приказа: повтор не раньше 20 с, не больше трёх попыток,
    пока износ не опустится ниже порога. После ремонта снимается
    собственный USE_BED_ORDER, остальные работы сохраняются.
  * Временные назначения и брони используют hand с идентификатором
    экземпляра вместо сохранённых сырых Character*/Building*. Загрузка,
    новая игра и удаление мира делают прежнее состояние недействительным
    до вызова оригинала. Сканы выполняет GUI-поток только при готовом
    активном мире. Новые объекты на старых адресах не наследуют назначения.
  * Дополнительные диагностические хуки ставятся только при Debug=1
    на старте. Debug отмечен MCM_RESTART и расположен последним.
    Выключение немедленно прекращает дополнительные проверки и записи;
    для включения полного режима требуется перезапуск. По умолчанию 0.
    Ошибки регистрации хуков пишутся независимо от этой настройки.
  * Настройки и подсказки: русский, английский, упрощённый китайский.
    Заголовки PO нормализованы в UTF-8; в пакет включены только три MO.
    Личные AIFixed.ini сохраняются, распространяется defaults.ini.

VERSION 1 CHANGES (30 OCTOBER 2026)

  * Rescue prefers a suitable free powered repair bed in the same
    settlement when carrying a wounded skeleton. The original search
    runs first; its result remains when no repair bed is suitable.
    Own furniture takes priority over allied furniture, then distance.
  * The healed/get-out check accounts for robotic parts. In a working
    repair bed the character stays until health and wear are repaired;
    missing limbs do not need restoration. Without repair power,
    the game's ordinary decision remains.
  * Player skeletons and biological characters with prosthetics can
    seek wear repairs automatically. Enabled by default, threshold 50;
    requires a free working repair bed in the same settlement. Checks
    run at most every 3 s, with separate bed reservations per character.
    Cancellation retries are at least 20 s apart, at most three until
    wear drops below the threshold. Completion removes this plugin's
    USE_BED_ORDER, preserving other work.
  * Temporary assignments and reservations use serial-bearing hand
    identities instead of retained raw Character*/Building* pointers.
    Load, new game and world destruction invalidate the old session
    before the original function. Only the GUI owner scans, and only
    with an active ready world. Reused addresses do not inherit state.
  * Extra diagnostic hooks install only with Debug=1 at startup.
    Detailed log is last and marked MCM_RESTART. Disabling immediately
    stops extra checks and records; enabling the complete mode requires
    a restart. Default 0. Hook-registration errors are always reported.
  * English, Russian and Simplified Chinese settings/help. Standard
    UTF-8 PO metadata; packages contain only three MO catalogs.
    Personal AIFixed.ini survives updates; only defaults are shipped.

НАСТРОЙКИ / SETTINGS

Требуется RE_Kenshi/KenshiLib. Mod Config Menu необязателен.
Requires RE_Kenshi/KenshiLib; Mod Config Menu is optional.
Настройки → MCM → AI Fixed. / Options → MCM → AI Fixed.
Без MCM — AIFixed.ini рядом с DLL. / Without MCM, use AIFixed.ini.

Enabled=1             Весь плагин, после перезапуска. / Whole plugin, restart.
SkeletonRepairBeds=1  Выбор ремкровати и оставаться до починки, сразу.
                      Repair-bed choice and stay-until-repaired, live.
SelfRepair=1          Самостоятельный ремонт износа, сразу. / Auto repair, live.
SelfRepairWear=50     Порог износа части, 0..100, сразу. / Wear threshold, live.
Debug=0               Последний пункт; полный режим после перезапуска.
                      Final setting; complete diagnostics after restart.

При первом запуске AIFixed.ini создаётся из AIFixed.defaults.ini;
при обновлении добавляются новые ключи, старые значения сохраняются.
First run creates AIFixed.ini from defaults; updates add new keys
without replacing existing values.

УСЛОВИЯ САМОСТОЯТЕЛЬНОГО РЕМОНТА / AUTOMATIC REPAIR REQUIREMENTS

Не отправляются персонажи в бою, без сознания, уже лежащие, несущие кого-то
или переносимые другим. Порог сравнивается с износом роботизированных частей,
а не с процентом общего здоровья. Без свободной работающей кровати в том же
поселении плагин не отправляет персонажа через всю карту к другой базе.

Characters in combat, unconscious, already lying down, carrying someone
or being carried are left alone. The threshold uses robotic-part wear,
not a percentage of total health. Without a free working bed in the same
settlement, the plugin does not send a character across the map.

СОВМЕСТИМОСТЬ И ПРОВЕРКИ / COMPATIBILITY AND VALIDATION

Сохранены вызовы исходных функций и штатные приказы. Брони и попытки —
временный учёт собственных походов, не замена движка ИИ. Совместимость со
всеми модами на ИИ не гарантируется: другие приказы, расы, мебель и анимации
проверяются на конкретной сборке. Каждая правка отключается отдельно.
Необычная поза лежащего робота сама по себе не доказывает, идёт ли ремонт.

Original functions and native orders remain. Reservations/retries track
this plugin's trips rather than replacing the AI engine. Other AI mods,
custom races, furniture and animations need testing in the specific mod
list. Individual fixes can be disabled. An unusual resting pose alone
does not prove whether the bed is applying repairs.

Регрессии проверяют смену мира, повторное использование идентификаторов
и отключение диагностики. Они не заменяют игровую проверку спасения,
потери питания, ручной отмены и возврата к работе с вашими расами/кроватями.
Regression tests cover session invalidation, identity reuse and diagnostic
guards. They do not replace gameplay checks of rescue, power loss,
manual cancellation and return to work with your actual races/beds.

Автор / author: LEOPARD. Основа / foundation: RE_Kenshi/KenshiLib.
Локализация / localization: общий Localization.h Asur; en_GB/ru_RU/zh_CN.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.
