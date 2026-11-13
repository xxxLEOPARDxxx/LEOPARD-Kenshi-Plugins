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
GateRestoreLock=1; GateSearchRadius=30 (5..500 m); GateCloseDelay=3 (1..30 s).

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
    GateCloseDelay=3 с (1..30), когда проём на этом этаже свободен. Исходный
    замок восстанавливается после закрытия или отмены ещё закрытых ворот.
    Ручное изменение створок или замка имеет приоритет над автоматикой.
    Задержки и предел ожидания ворот идут по времени симуляции. Во время
    паузы автоматические открытие, возврат к движению, закрытие и запирание
    не выполняются, отложенные намерения сохраняются. Недоступные часы
    приостанавливают только нашу автоматику; штатный интерфейс работает.
  * GateSearchRadius=30 м (5..500). Если навмеш подтверждает переход через
    штатные ворота города, поиск вокруг персонажа не нужен. Запасной поиск
    использует категорию ворот и проверяет стороны навмеша или пересечение
    исходного направления со створкой.
    Подход продолжается до пяти минут симуляции, пока персонаж приближается.
    После 30 секунд без продвижения помощь заканчивается. Исходный приказ
    возобновляется, если персонаж ещё выполняет наш приказ открыть ворота.
    До двух локальных запросов за 250 мс и до восьми персонажей за проход.
    Игра игнорирует maxNumber. Результат с 128 и более воротами пропускается.
  * RecoverStuckNpc=0. После выбранного времени неподвижности здорового
    NPC плагин ищет ближайший доступный навмеш. Если навмеш проходит внутри
    природного камня, две пары встречных лучей подтверждают коллизию, а пробы
    ищут свободное место до 8 м от NPC. У ворот после загрузки можно подтвердить
    застревание по коллизии даже при потере опоры в данных движения. Выход
    сохраняет сторону ворот и область навмеша. Пробы при наличии лица навмеша
    ограничены одной в секунду отдельно от обычной проверки вне навмеша.
    NPC, у которого срываются попытки пройти к цели, может спуститься с
    крутого склона платформы на свободную сушу в той же загруженной зоне,
    без пересечения стен и закрытых ворот. Для спуска нужны выбранное время
    без продвижения, подтверждённый склон либо явное отсутствие проходимого
    лица навмеша под NPC и недавний отказ пути
    при текущем штатном действии движения. Смена цели на кровать, покупки,
    патруль или другую задачу не обнуляет физическое наблюдение на той же
    опоре. Пустая цель ИИ не отменяет ещё активное штатное действие движения;
    без такого действия новое доказательство отказа не создаётся.
    Краткие сбросы штатного пути, включая NONE/COMPLETE и подтверждение
    GOAL_REACHED без реального перемещения, сохраняют только таймер наблюдения.
    В этих состояниях перенос запрещён: нужна текущая активная попытка идти
    к удалённой цели. Реальное прибытие сдвигает стационарную точку и сбрасывает
    таймер. Отказ старше часа не разрешает перенос: нужен подтверждённый
    отказ текущего движения. Физическое наблюдение на той же опоре сохраняется
    отдельно от срока действия этого свидетельства.
    Сохранённый отказ после завершения движения не обновляет свидетельство.
    Текущее штатное движение к удалённой цели с текущим отказом пути считается
    действующей заблокированной попыткой, даже если ИИ пересоздаёт действие.
    Для подтверждённого крутого склона платформы пределы составляют 6 м по
    горизонтали и 5-18 м вниз. При отсутствии лица без подтверждённого склона
    предел спуска - 14 м. У ворот сохраняется предел 14 м, у камней - 8 м.
    Подробный журнал показывает опору, положение, возраст наблюдения, текущие
    цель/действие/путь и конкретный отказ места выхода. Он выключен по умолчанию.
  * UnstuckDelayHours=1 (1..4 игровых часа). Время без физического
    продвижения до попытки освобождения. Меняется сразу, без перезапуска,
    и не меняет частоту проверок. Уже наблюдавшееся время сохраняется;
    повторные пробы продолжают соблюдать свой предел частоты.
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
    GateCloseDelay=3 s (1..30), once the opening on the gate's floor is clear.
    The original lock is restored after closure or cancellation of a gate that
    is still shut. Manual gate and lock changes take precedence.
    Gate delays and episode expiry use active simulation time. Pausing suspends
    automatic opening, movement resume, closing and relocking while preserving
    pending intent. An unavailable clock suspends our automation while normal
    GUI updates continue. No native clock polling occurs without gate work.
  * GateSearchRadius=30 m (5..500). When the navmesh confirms a transition
    through the registered town gate, no spatial query is needed. The fallback
    queries only gate buildings and checks the navmesh regions or movement line.
    A traveller
    making progress has up to five simulation minutes to approach. After 30
    seconds without progress, assistance ends and any door order still owned
    by the plugin gives way to the original movement order.
    Up to two local queries per 250 ms and eight actors per pass. Kenshi
    ignores query maxNumber. Results containing 128 or more gates are skipped.
  * RecoverStuckNpc=0. After the selected delay without progress, a healthy NPC
    can move to nearby walkable ground. When a valid navmesh face lies inside
    natural rock, two opposed pairs of rays confirm enclosure and the landing
    search checks for clear ground within 8 m. Gate collision can confirm
    elevated entrapment after a load loses the standing support handle. The
    landing must stay on the same side and navmesh region. Valid-face probes
    are limited to one per second independently of ordinary off-mesh recovery.
    NPCs repeatedly failing to reach their destinations can leave a steep platform
    slope for clear, dry ground in the same loaded zone while preserving wall
    and closed-gate boundaries. Recovery requires the selected time without progress,
    a collision-confirmed slope or an explicit native walkable-face miss at
    the NPC, plus a recent path failure during
    a current native movement action. Switching to bed, shopping, patrol or
    another goal retains physical observation on the same support. A missing
    GOAP goal does not cancel a still-current native movement action. Without
    that action, a native failed path cannot supply new failure evidence.
    Transient native path resets, including NONE/COMPLETE and GOAL_REACHED
    acknowledgements without physical movement, retain only the observation
    clock. They never admit relocation: a current active remote movement attempt
    is required. Real arrival changes the stationary anchor and resets its
    clock. A failure older than one hour cannot admit relocation: a confirmed
    current movement failure is required. Physical observation
    on the same support is retained separately from witness freshness.
    A retained failed path after movement ended cannot refresh evidence. A
    current native move with a current remote failed path is blocked intent,
    including when GOAP recreates that move action. Platform limits are 6 m horizontally and a 5-18 m descent only for a confirmed steep
    platform. An explicit face miss without steep-slope proof retains 14 m.
    Gates retain their 14 m limit; natural rock retains 8 m. Detailed log records
    support, position, observation age, goal/action/path and the exact rejected
    landing check. It is disabled by default.
  * UnstuckDelayHours=1 (1..4 game hours). Time without physical progress
    before recovery is attempted. Applies immediately without restart and
    does not change the checking rate. Existing observation time is retained;
    retry probes keep their separate cooldown.
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

## Release 3 / Выпуск 3

English

- Added optional recovery for NPCs stranded in rocks, ore deposits, walls, gates and small supports. Characters immobilised by injuries or missing legs are not treated as stuck.
- The wait without progress is adjustable from 1 to 4 game hours, default 1 hour. A nearby safe landing is checked before moving the NPC; recovery is off by default.
- Corrected passage through own gates and movement after opening them. Closing after passage accounts for characters in the opening; the search radius is adjustable up to 500 m.
- Original SleepFix and GateFix are not required: their adapted functions are already included in AI Fixed.

Русский

- Добавлено необязательное освобождение NPC, застрявших в скалах, рудных жилах, стенах, воротах и небольших опорах. Обездвиженные ранениями и персонажи без ног не считаются застрявшими.
- Ожидание без продвижения настраивается от 1 до 4 игровых часов, по умолчанию 1 час. Перед перемещением проверяется безопасное место рядом; функция по умолчанию выключена.
- Исправлены проход собственных ворот и продолжение движения после открытия. Закрытие после прохода учитывает персонажей в проёме; радиус поиска настраивается до 500 м.
- Оригинальные SleepFix и GateFix устанавливать не нужно: их адаптированные функции уже включены в AI Fixed.
