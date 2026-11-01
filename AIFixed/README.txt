AI Fixed — модульный сборник / modular AI fixes

Собственные исправления LEOPARD и адаптированные модули других авторов;
сборник может расширяться независимо от отдельных модулей.
Идеи прохода ворот и сна — Basusu (GateFix/SleepFix). Спасибо Basusu
за оригиналы, открытые исходники и подробные исследования поведения игры.
LEOPARD original fixes plus selected adaptations, with room for future modules.
Gate passage and sleep adapt Basusu's GateFix/SleepFix. Thank you, Basusu,
for the originals, open source and investigation notes.
Оригиналы / Originals:
  GateFix: https://steamcommunity.com/sharedfiles/filedetails/?id=3787399736
  SleepFix: https://steamcommunity.com/sharedfiles/filedetails/?id=3787375792
Исходники оригинала / Original sources:
  https://github.com/Basusu0/Basusu-kenshi-plugins
Наши исходники / Our sources:
  https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins
GPLv3: LICENSE; точная основа / exact provenance: UPSTREAM.md.
RE_Kenshi/KenshiLib — BFrizzleFoShizzle; Localization.h — Asur и участники.

Prepared release 2 / Подготовленный выпуск 2 (2026-11-02)

English

- A modular AI collection. AI Fixed combines our robotic-repair fixes with selected GateFix and SleepFix adaptations by Basusu. Each module can be configured separately; new sleep, robotic-health and gate assistance are off by default. Original SleepFix and GateFix are not required.
- Rest and robotic care. Optional automatic sleep treats bandaged biological wounds. Powered repair beds handle robotic health and wear, including prosthetics. Food takes priority when hunger affects the character.
- Nearby suitable beds. A configurable search radius, ownership and free-bed checks keep automatic rest nearby. Your faction beds are used by default; allied beds can be allowed.
- Manual orders take priority. Travel, Stop and queued orders interrupt our rest. Completed rest releases the character to existing jobs. Continuing healing is not cut short by a fixed total timer; pausing does not consume rest or gate waiting time.
- Optional own-gate passage. A stalled movement order can open your gate and resume its destination. Locked-gate opening, closing after passage and restoring the original lock are separate options. Manual gate actions take priority; cancelled assistance may leave the gate open or unlocked.
- Optional detailed logging. Language and translation summaries follow this plugin's Detailed log setting, off by default. Loading and format errors remain visible; enabling the log later reports the saved summary once.
- A clean update. English, Russian and Simplified Chinese use one compiled catalog each. Personal settings and saved games are preserved. Thank you, Basusu, for the original mods and open sources.

Русский

- Модульный сборник исправлений ИИ. AI Fixed объединяет наши исправления ремонта робочастей и адаптации GateFix и SleepFix автора Basusu. Модули настраиваются отдельно; новые сон, ремонт здоровья робочастей и помощь у ворот по умолчанию выключены. Оригинальные SleepFix и GateFix устанавливать не нужно.
- Отдых и уход за робочастями. Необязательный автоматический сон помогает лечить перевязанные живые раны. Ремкровати с питанием восстанавливают здоровье и износ робочастей, включая протезы. При влиянии голода на персонажа еда получает приоритет.
- Подходящие кровати рядом. Настраиваемый радиус, проверка собственности и свободного места ограничивают поиск отдыха ближайшими подходящими кроватями. По умолчанию используются кровати своей фракции; союзные можно разрешить.
- Ручные приказы важнее. Движение, Stop и очередь приказов прерывают наш отдых. После восстановления персонаж возвращается к прежним работам. Продолжающееся лечение не обрывается общим лимитом времени; пауза не расходует ожидание отдыха и ворот.
- Необязательная помощь у своих ворот. Остановившийся персонаж с приказом движения может открыть свои ворота и продолжить путь. Открытие запертых ворот, закрытие после прохода и возврат исходного замка настраиваются отдельно. Ручные действия важнее; отмена помощи может оставить ворота открытыми или отпертыми.
- Необязательный подробный журнал. Сводки языка и переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию. Ошибки загрузки и формата видны всегда; включение журнала позже выводит сохранённую сводку один раз.
- Чистое обновление. Английский, русский и упрощённый китайский используют по одному скомпилированному каталогу. Личные настройки и сохранения сохраняются. Спасибо Basusu за оригинальные моды и открытые исходники.

ПОДРОБНЫЙ СПРАВОЧНИК ВОЗМОЖНОСТЕЙ / DETAILED FEATURE REFERENCE

Новые настройки / New settings (live / сразу):
AutoSleep=0; SleepBelowHealth=90 (10..99); WakeAboveHealth=99 (trigger..100).
RepairHealth=0; RepairBelowHealth=90 (10..99); BedSearchRadius=100 (10..500 m).
OwnBedsOnly=1; GatePassage=0; GateOpenLocked=0; GateCloseAfterPassage=1.
GateRestoreLock=1; GateSearchRadius=30 (5..100 m); GateCloseDelay=3 (1..30 s).

  * Один планировщик для сна живых частей, ремонта здоровья робочастей
    и прежнего ремонта износа. AutoSleep=0, RepairHealth=0 — по желанию;
    SelfRepair=1 сохраняется. Здоровые работники не получают приказ сна.
    Живые раны сначала перевязываются, кроме самовосстанавливающихся частей.
    Обычная кровать не ждёт ремонта протезов, который в ней невозможен.
    Голод по штатному штрафу движения отдаёт приоритет еде: наш отдых
    приостанавливается, затем может продолжиться после исчезновения штрафа
    при неизменном намерении и оставшихся попытках. Это не гарантия питания
    при любой модификации ИИ, отсутствии еды или заблокированном пути.
  * BedSearchRadius=100 м (10..500), только то же поселение; реальная функция
    кровати, мебель, доступность места, собственность и этаж. OwnBedsOnly=1;
    при 0 допускаются союзные, но не нейтральные или вражеские кровати.
    Разные этажи одного здания и перепад высоты более 10 м исключаются.
    Ремкровать должна реально чинить с питанием. Название мебели не является
    признаком кровати; модовые флаги и анимации требуют игровой проверки.
  * Ручное движение, Stop, другая цель и очередь приказов отменяют владение;
    данный эпизод не повторяется до устранения причины отдыха. Заменяется
    только наш единственный точный приказ; постоянные работы не удаляются.
    Подход: 60 с симуляции, до трёх попыток через 20 с симуляции. В кровати
    отсутствие улучшения 5 минут симуляции завершает наш отдых и блокирует
    повтор эпизода. Отдых с продолжающимся восстановлением не имеет общего
    лимита времени; пауза не расходует таймеры. Измеряется штатное игровое
    время, GUI опрашивает его не чаще раза в 3 с реального времени. Новое
    повреждение понижает базу сравнения: последующее лечение тоже считается
    прогрессом. Потеря питания завершает только наш собственный приказ.
  * GatePassage=0 помогает пройти свои закрытые ворота по ручному приказу
    наземного движения. Нужны затихшие клики, остановившийся персонаж и путь
    через створку. Штатный приказ открывает ворота, затем возобновляется
    исходная точка пути. Нет телепортации или принудительного закрытия.
    GateOpenLocked=0 не трогает запертые ворота.
  * GateCloseAfterPassage=1, GateRestoreLock=1: обычное закрытие после прохода,
    GateCloseDelay=3 с (1..30), если в радиусе 8 м нет людей. Исходный замок
    восстанавливается только после полного закрытия. Ручные приказы, кнопки
    ворот и изменение замка отменяют помощь. При отмене/таймауте ворота могут
    остаться открытыми или отпертыми: намерение игрока имеет приоритет.
    Задержки и предел ожидания ворот идут по времени симуляции. Во время
    паузы автоматические открытие, возврат к движению, закрытие и запирание
    не выполняются, отложенные намерения сохраняются. Недоступные часы
    приостанавливают только нашу автоматику; штатный интерфейс работает.
  * GateSearchRadius=30 м (5..100), до двух локальных запросов за 250 мс,
    до восьми персонажей за проход. Игра игнорирует maxNumber запроса — это
    НЕ жёсткий лимит его работы/памяти. Наш код отказывается от результата
    с 128+ постройками уже после запроса. Нет покадрового обхода мира.
  * SleepFix.dll/GateFix.dll подавляют соответствующую автоматику AI Fixed
    для исключения конкурирующих приказов. Не используйте две реализации
    одной механики. Gate Push Guard исправляет другой случай: выталкивание
    сквозь закрытые ворота. Цепочки оригиналов сохранены, нужен совместный
    игровой тест. Новые настройки работают сразу; Debug последний и выключен.
  * Тесты не доказывают исправление всех сообщений о турелях, дальнем враге
    за стенами, NeedSleep и любой модовой мебели. Явная матрица и ограничения:
    tests/verification-basusu-integration-20261102.md.

DETAILED FEATURE REFERENCE (ENGLISH)

  * One scheduler handles biological sleep, robotic health and existing wear
    repair. AutoSleep=0 and RepairHealth=0 are opt-in; SelfRepair=1 remains.
    Healthy workers are left alone. Wounds wait for bandaging unless the part
    heals itself. Ordinary beds finish flesh care without waiting for prosthetics.
    The native hunger movement penalty gives food priority: our rest pauses,
    and can resume after that penalty clears if intent and attempt budget permit.
    This does not guarantee feeding with arbitrary AI mods, no food or blocked paths.
  * BedSearchRadius=100 m (10..500) bounds same-settlement choices. Real bed
    function, furniture, usable/free slot, ownership and floor are required.
    OwnBedsOnly=1; disabling permits allies, never neutral/enemy owners.
    Different floors in one building or >10 m vertical separation are rejected.
    Repair beds must actually repair with power. Custom flags need game checks.
  * Manual travel, Stop, another target or queued orders cancel our ownership
    and suppress that injury episode until its trigger clears. Only our sole
    exact order can be replaced; permanent jobs remain. Approach: 60 simulation s, at most
    three attempts spaced 20 simulation seconds. Five simulation minutes without
    healing ends our order and suppresses the episode. Continuing recovery has
    no overall duration limit; pausing consumes no deadline budget. Native world
    time supplies deadlines, sampled at most every 3 wall seconds. New damage
    lowers the comparison baseline so subsequent healing counts as progress too.
    Power loss also ends only our own order.
  * GatePassage=0 assists explicit ground movement through your closed gates
    after quiet input and a stalled actor. Native door orders open the leaf,
    then resume the original destination. No teleporting or forced-close fallback.
    GateOpenLocked=0 leaves locked gates untouched.
  * GateCloseAfterPassage=1 and GateRestoreLock=1 request normal closure after
    GateCloseDelay=3 s (1..30), with no characters within 8 m. Original locking
    returns only after full closure. Manual orders, direct door buttons and lock
    changes cancel assistance. Cancellation/timeouts may leave a gate open or
    unlocked; preserving player intent takes precedence over guaranteed closure.
    Gate delays and episode expiry use active simulation time. Pausing suspends
    automatic opening, movement resume, closing and relocking while preserving
    pending intent. An unavailable clock suspends our automation while normal
    GUI updates continue. No native clock polling occurs without gate work.
  * GateSearchRadius=30 m (5..100). At most two local queries per 250 ms and
    eight actors processed per pass. Kenshi ignores query maxNumber, so this is
    NOT a hard native work/allocation bound. We decline 128+ returned buildings
    after native work. No per-frame world traversal or raw-pointer registry.
  * Original SleepFix.dll/GateFix.dll suppress the corresponding AI Fixed
    automation to avoid competing orders. Do not combine both implementations.
    Gate Push Guard handles pushing through closed gates, a different feature.
    Original chains are preserved; a combined gameplay check is still required.
    New settings apply live. Detailed log stays last/default off; full old
    diagnostics retain their actual restart requirement.
  * Tests do not prove every turret, distant threat behind walls, NeedSleep or
    modded-bed report fixed. Explicit matrix and limitations:
    tests/verification-basusu-integration-20261102.md.

Далее исторический выпуск 1 / Historical version 1 follows.

AI Fixed (QoL)

[Nexus — our release / наша версия](https://www.nexusmods.com/kenshi/mods/2256)

ИСТОРИЯ: ВЕРСИЯ 1 (30.10.2026)

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

Авторы / authors: LEOPARD (собственные правки / original fixes),
Basusu (адаптированные GateFix/SleepFix / adapted modules). Foundation: RE_Kenshi/KenshiLib.
Локализация / localization: общий Localization.h Asur; en_GB/ru_RU/zh_CN.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.
