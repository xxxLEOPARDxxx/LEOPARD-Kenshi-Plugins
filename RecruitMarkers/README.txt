RECRUIT MARKERS (QoL)

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 4 (30.10.2026)

Звезда требует фактического лидера и хотя бы одного другого живого участника
того же загруженного отряда. Одиночные торговцы и рекруты больше не отмечаются.
Мёртвые, пустые и сменившие отряд записи не считаются последователями.

VERSION 4 CHANGES (30 OCTOBER 2026)

A star requires the actual leader and at least one other living member of the
same loaded squad. Solo traders and recruits no longer receive a leader marker.
Dead, empty and stale roster entries do not count as followers.

ИЗМЕНЕНИЯ ВЕРСИИ 3 (29–30.10.2026)

  * Над фактическим лидером загруженного отряда появляется золотая
    звезда. Используется лидер, назначенный самой игрой, а не первый
    персонаж в списке. ShowLeaders включает звёзды, ShowPlayerLeaders
    разрешает их и для своих отрядов, если у них назначен лидер.
  * У звезды независимый размер LeaderSize: 20 пикселей по умолчанию,
    диапазон 8..64. IconSize по-прежнему управляет только наймом и
    торговцами. Квадратная звезда выровнена с ними по нижнему краю,
    несколько значков одного героя располагаются рядом.
  * Цвет звезды LeaderColour по умолчанию #ffda40. Общие HoldKey (ALT,
    NONE — всегда), прозрачность, высота, дальность и лимит значков
    действуют и на звёзды. В MCM доступны отдельные переключатели
    лидеров, размер и цвет; настройки применяются без перезапуска.
  * При переполнении отбираются ближайшие цели из всех загруженных
    подходящих персонажей. Лимит считает отдельные значки найма,
    торговца и лидера. Проверка диалогов ограничена двумя кандидатами
    за проход с интервалом не менее 50 мс (до 40 диалогов/с), очередь —
    до 256 целей, кэш — до 2048 с удалением давно исчезнувших целей.
  * Виджеты переиспользуются и хранят безопасные ссылки. Координаты,
    цвет и прозрачность меняются при отличии; кость головы кэшируется.
    На загрузке и в меню значки скрываются, игровое состояние очищается.
    Это ограничения работы плагина, а не обещание прироста FPS.
  * Debug — последняя настройка MCM, выключен по умолчанию (Debug=0). Сводка
    ограничена интервалом 10 с; ошибки пишутся независимо от Debug.
    Личный INI сохраняется. Переводы ru_RU/en_GB/zh_CN; в выпуске .mo,
    .po остаются в исходниках.

VERSION 3 CHANGES (29–30 OCTOBER 2026)

  * A gold star marks the actual game-assigned leader of a loaded
    squad. ShowLeaders controls stars; ShowPlayerLeaders includes
    player squads when the game has assigned a leader.
  * LeaderSize is independent: 20 px by default, range 8..64. IconSize
    still controls recruit and trader icons. The square star aligns
    with their bottom edge; multiple icons for one character sit
    beside each other.
  * LeaderColour defaults to #ffda40. Stars share HoldKey (ALT, or NONE
    for always visible), opacity, height, range and the total marker
    budget. Leader toggles, size and colour are exposed in MCM; all
    settings apply without restarting the game.
  * Overflow keeps nearest targets from all eligible loaded characters.
    Recruit, trader and leader icons each count toward the limit.
    Dialogue scans process at most two candidates per pass, at least
    50 ms apart (up to 40 dialogues/s); the queue holds up to 256 and
    the cache up to 2048 entries, pruning long-absent targets.
  * Widgets and safe references are reused. Geometry, colour and alpha
    update only when changed; head-bone lookup is cached. Loading and
    menus hide markers and clear session state. These are work limits,
    not a claim of an FPS increase.
  * Detailed log is last in MCM and off by default; summaries are limited
    to once per 10 s. Errors are logged regardless of Debug. Updates
    preserve the player's INI. UI translations: ru_RU/en_GB/zh_CN;
    releases contain .mo catalogs, with .po sources in the repository.

СПРАВОЧНИК И ИСТОРИЯ

Значок над головой у тех, кого можно нанять. По умолчанию виден, пока
зажат ALT (клавиша и «всегда» - в настройках).

  - уникальные (именные) персонажи - светло-жёлтый значок, мягко
    пульсирует; обычные наёмники и животные - оранжевый;
  - только те, кого игра даст нанять прямо сейчас: проверяются условия
    самого диалога (деньги, отношения, плен и прочее) для ближайшего
    выделенного персонажа. Можно выключить - тогда отмечены все, у кого
    найм вообще есть в диалоге;
  - цвета, размер, прозрачность, высота над головой, дальность, число
    значков - в Mod Config Menu (вкладка «Моды» в настройках игры) или в
    RecruitMarkers.ini.

Замена LtEast's Recruitable NPC Highlighter (и его Mod Settings Core).
Его нужно ОТКЛЮЧИТЬ, иначе значки будут двойными.


КАК ОПРЕДЕЛЯЕТСЯ

По живому диалогу персонажа (Character::dialogue), а не по названиям
пакетов: разговоры на событие «игрок заговорил со мной»
(EV_PLAYER_TALK_TO_ME), вглубь по веткам. Нанимаемый - если найдена
реплика с действием DA_JOIN_SQUAD_WITH_EDIT или DA_JOIN_SQUAD_FAST, и
путь до неё открыт:
  - реплика не заперта (Dialogue::isLocked), одноразовая - ещё не сказана;
  - условия каждой реплики на пути выполняются
    (DialogLineData::checkConditions, собеседник - ближайший выделенный
    персонаж игрока, иначе ближайший из отряда); CheckConditions=0 -
    условия не смотрим;
  - разговор не предназначен другому конкретному персонажу (isCharacter).
Обход - под SEH: сломанный диалог даёт «не нанимаемый» и строку в журнале
(первые пять).


НАГРУЗКА

  - кандидаты - раз в 0,75 с из списка обновляемых персонажей игры
    (GameWorld::getCharacterUpdateList): живые, не наши, не дальше
    MaxDistance от центра камеры;
  - обход диалога - не больше двух персонажей за кадр, ответ хранится
    10 с; кого минуту нет рядом - забываем;
  - значки - пул ImageBox на слое Back (под окнами, как имена над головами
    у игры), ссылки WidgetRef, без поиска по имени. В кадре - только
    позиция головы (кость «Bip01 Head») и проекция на экран;
  - пока клавиша показа не зажата - ничего, кроме опроса клавиши.

Проекция: объект UtilityT игры не экспортирован, адрес берётся по версии
игры (1.0.65 / 1.0.68, Steam и GOG; таблица из Vital-Sense). На другой
версии значков нет, в журнале - строка об этом.

Хуки: GameWorld::_NV_mainLoop_GPUSensitiveStuff (обход и значки),
ForgottenGUI::update (в главном меню и на загрузке значки прячутся, кэш
сбрасывается).


НАСТРОЙКИ (RecruitMarkers.ini, секция [RecruitMarkers])

  Enabled, HoldKey (ALT / CTRL+B / ... / NONE - всегда), ShowUnique,
  ShowGeneric, ShowAnimals, ShowTraders, TraderColour, ShowLeaders,
  ShowPlayerLeaders, LeaderSize (8..64, по умолчанию 20), LeaderColour,
  CheckConditions, UniqueColour, GenericColour
  (#RRGGBB), IconSize (12..96), Opacity (1..10), PulseUnique,
  HeightAboveHead (см), MaxDistance (м, 20..5000), MaxMarkers (8..128), Debug.

В раздаче - только RecruitMarkers.defaults.ini; RecruitMarkers.ini
создаётся из него при первом запуске и принадлежит игроку.


ИЗМЕНЕНИЯ 16.10.2026

  - Положение значка - как у LtEast (снято с его DLL): кость «Bip01 Head»,
    «Head» или «head», если она правдоподобна (не дальше 12 по горизонтали
    и от -4 до +20 по высоте от позиции персонажа), плюс HeightAboveHead по
    вертикали мира; кости нет - позиция + 2,2 + отступ. Значок низом на
    точке, зазор 4 пикселя, размер постоянный. Промежуточные сборки с
    отступом пикселями и в масштабе персонажа уводили значок от головы при
    приближении и отдалении камеры - убраны.
  - Дальность - до 5000 м.
  - Уникальный - по флагу «unique» шаблона персонажа, не по
    Character::isUnique() (у двух одинаковых наёмников он отвечал
    по-разному). Значок - облачко реплики из атласа игры, как у LtEast,
    обесцвеченное, чтобы цвета из настроек ложились при любом интерфейсе.


ИЗМЕНЕНИЯ 22.10.2026

  - Торговцы: над теми, кто торгует, - стопка монет своим цветом
    (ShowTraders, TraderColour, по умолчанию #80e0ff). Торговец - по
    флагу «is trader» шаблона персонажа (не Character::isATrader(): та
    отвечает «да» всему торговому отряду, с охраной), диалог не разбирается.
    Торговца, которого можно и нанять, отмечают оба значка рядом: облачко
    найма слева, монеты справа. Значок монет - gui/gfx/recruitmarkers_trader.png
    (белый, цвет задаёт настройка). MaxMarkers считает значки.


ЛИДЕРЫ ОТРЯДОВ (29.10.2026)

Золотая звезда над фактическим лидером загруженного отряда, назначенным
игрой. Значки найма, торговца и лидера располагаются рядом.
Используются общие клавиша, прозрачность и дальность показа.
Размер звезды отдельный — LeaderSize.
Звезда требует хотя бы двух живых участников в одном загруженном отряде,
включая самого лидера. Одиночные торговцы, рекруты и прочие одиночки больше
не получают звезду только потому, что игра считает их лидерами самих себя.
Мёртвые, пустые и уже сменившие отряд записи не считаются последователями.
Торговец с настоящим живым отрядом по-прежнему может быть его лидером.
MCM: "Squad leaders", "Include player leaders", "Leader star size"
и "Leader colour".
Лидеры игрока отмечаются, если игра назначила их отрядам лидера.
The star requires the actual leader and at least one other living member in
the same loaded squad. Solo merchants/recruits are excluded; dead, empty or
stale roster entries do not count. A merchant leading a real living group can
still be marked. Roster checks run only for leaders on the existing scan;
they stop after finding the two members and do not add a world scan or cache.

Исправления по проверке 29.10.2026:
Звезда лидера: отдельная настройка LeaderSize, 8..64 пикселя, по умолчанию 20. Размер других значков не меняется; звезда квадратная, выровнена по нижнему краю. Применяется сразу.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.

Prepared release 5 / Подготовленный выпуск 5 (2026-11-02)

English

- Optional language diagnostics. Language and translation-catalogue summaries follow this plugin's Detailed log setting, which is off by default.
- Useful errors remain visible. Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.
- Clean update package. English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.

Русский

- Необязательная диагностика языка. Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.
- Ошибки остаются видимыми. Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.
- Чистый пакет обновления. Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.
