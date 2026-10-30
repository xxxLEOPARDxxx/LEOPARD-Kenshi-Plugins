MULTI XP MINING - универсальная добыча с MCM

[Nexus — our release / наша версия](https://www.nexusmods.com/kenshi/mods/2258)

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 1 (29–30.10.2026)

  * Универсальная добыча по объединённой базе игры: BUILDING ->
    functionality, функция BF_MINE или BF_MINE_NATURAL и stat used=
    Labouring. Подходящие источники олова, угля и других модовых
    ресурсов входят автоматически, без ручного перечня ID. Фермы
    с Farming исключены. 172 источника — результат аудита текущей
    сборки, не предел плагина и не гарантированное число для всех.
  * Сила, стойкость и ловкость получают опыт за настоящий тик труда
    рабочего. Автоматическая машина без труда героя опыта не создаёт.
    Дробные доли накапливаются: 0 отключает навык, 0.25 даёт один сырой
    тик на четыре тика Labouring, 1 — один на каждый. Общий множитель
    масштабирует дополнительный опыт. Расовые поправки и XP_Overhaul
    проходят через штатный вызов опыта.
  * Необязательные пределы и общий множитель пределов ограничивают
    только дополнительную прокачку, не обычный опыт игры. Общие
    [Defaults] для автоматических источников: сила 0.25 до 15,
    стойкость и ловкость 0. Поэтому одинаковые исходные числа у разных
    жил ожидаемы. У каждого ID собственные сохранённые правки.
  * MCM -> Multi XP Mining: прямой список, поиск по имени и ID,
    сортировка по имени, отдельные параметры выбранного источника.
    Совпадающие названия различаются ID. Над полями показаны имя и ID
    выбранной записи, поиск выровнен по вертикали. Повторный щелчок
    по выбранной записи повторяет запрос построения параметров, если
    предыдущий запрос пришёлся на пересоздание окна MCM.
  * Колесо листает по три строки, стрелки — по одной, дорожка —
    страницу. Штатный бегунок имеет минимум до 36 пикселей с учётом
    доступной высоты. Фиксированный пул покрывает только видимые
    строки, отступы удерживают их внутри области. Длинные имена
    сокращаются без разрыва Unicode; полные имя и ID — в подсказке.
    Текст обновляется при открытии, фильтре или прокрутке, не покадрово.
  * SourceData/*.ini читаются по именам файлов; явные правила имеют
    приоритет над автоматикой. MCM пишет только изменённый ключ
    выбранного ID в SourceData/99_UserOverride.ini без перечитывания
    таблиц при движении ползунка. Сброс берёт значения таблиц без
    личного override, для автоматических — [Defaults]. После ручной
    правки файлов есть отдельная кнопка перезагрузки таблиц.
  * Необязательный UnknownSources.ini регистрирует прочие неизвестные
    рабочие места с Enabled=0, не начисляя опыт. Запись вне XP-хука,
    до 16 записей за кадр; существующие значения сохраняются, ошибки
    повторяются через 5 с. Этот файл не заменяет таблицы SourceData.
  * Очереди опыта и остатки сбрасываются при загрузке, новой игре
    и изменении настроек. Выключенный мод пропускает собственную
    работу до таймеров и сканов. Кэш и буферы переиспользуются, нулевые
    навыки не читаются; полный каталог строится при открытии настроек.
  * Все настройки MCM применяются сразу. Подробный журнал — последняя
    строка, Debug=0 по умолчанию. Сводки рабочих не чаще раза в 5 с;
    редактор различает выбор ID, запрос страницы и фактическую сборку
    полей. Ошибки пишутся всегда. Личный INI и override сохраняются;
    вторую DLL исходного мода не включать. Переводы ru_RU/en_GB/zh_CN;
    в выпуске .mo, исходные .po остаются в репозитории.

VERSION 1 CHANGES (29–30 OCTOBER 2026)

  * Mining sources are discovered from the game's merged database:
    BUILDING -> functionality, BF_MINE or BF_MINE_NATURAL with
    stat used=Labouring. Eligible tin, coal and other modded resources
    need no hard-coded IDs. Farming sources are excluded. The current
    audit found 172 sources; this is neither a plugin limit nor a
    guaranteed count for every mod list.
  * Strength, Toughness and Dexterity XP follows real Labouring ticks.
    Automatic machinery without character work grants none. Fractions
    accumulate: 0 disables a skill, 0.25 adds one raw tick per four work
    ticks and 1 adds one per tick. A global multiplier scales extra XP;
    native XP calls retain racial and XP_Overhaul modifiers.
  * Optional caps and their global multiplier affect only extra
    training, leaving normal game XP unchanged. Automatic sources
    share [Defaults]: Strength 0.25 up to 15, Toughness/Dexterity 0.
    Equal starting values are expected; each source still has its own
    ID and independently saved edits.
  * MCM -> Multi XP Mining has a direct name/ID filter and sorted source
    list, followed by the selected source's fields. Duplicate names
    include IDs. A heading identifies the selected name and ID; search
    text is vertically centred. Clicking the same source again retries
    refresh if options-window rebuilding deferred or rejected it.
  * Wheel scrolling moves three rows, arrows one row and the track one
    page. The native scrollbar thumb has a minimum up to 36 px within
    available space. A fixed pool contains only visible rows. Padding
    keeps them inside the viewport; Unicode-safe truncation and full
    name/ID tooltips handle long labels. Text updates on opening,
    filtering or scrolling, rather than every frame.
  * SourceData/*.ini loads in filename order; explicit rules override
    automatic defaults. MCM writes only the selected ID's changed key
    to SourceData/99_UserOverride.ini without reloading tables during
    slider movement. Reset uses table values without the personal
    override, or [Defaults] for automatic entries. A separate reload
    action applies manual file edits.
  * Optional UnknownSources.ini records other workplaces disabled,
    without awarding XP. Writes occur outside the XP hook, at most
    16 per frame, preserve existing values and retry failures after
    5 s. Discovery records do not replace SourceData rules.
  * Load, new game and settings changes clear queued XP and fractional
    carry. Disabled updates exit before timers/scans; buffers are
    reused and zero-multiplier skills are not read. Full catalog
    enumeration happens on settings opening, not per frame.
  * All MCM settings apply live. Detailed log is last and off by default;
    worker summaries are limited to once per 5 s. Editor diagnostics
    distinguish selection, refresh requests and actual field builds.
    Errors are always logged. Personal INI and overrides are preserved;
    do not enable a second DLL of the original mod. UI translations:
    ru_RU/en_GB/zh_CN; release .mo only, .po sources in the repository.

СПРАВОЧНИК

Дополнительный опыт силы, стойкости и ловкости начисляется за реальную
работу персонажа. Каталог строится по всем загруженным данным игры:
функция добычи и навык труда. Названия ресурсов и ID модов не задаются
вручную. Поддерживаются олово, уголь и остальные ресурсы текущих и новых
модов. Фермы с навыком сельского хозяйства не попадают в этот каталог.
Автоматическая машина без реального тика труда не создаёт опыт сама.

MCM -> Multi XP Mining: вверху прямой список источников. Щелчок по имени
выбирает источник, поиск сразу фильтрует видимые строки. Перещёлкивать
весь каталог больше не нужно. Список сортируется по названию, совпадающие
названия различаются ID; полное имя и ID есть в подсказке.
Ниже - общие множители и настройки выбранного источника: включение,
по два поля для силы, стойкости и ловкости - опыт и предел.
Подробный журнал - последняя настройка, по умолчанию выключен.

Опыт 0 отключает навык. Опыт 0.25 даёт один дополнительный сырой тик на
четыре тика труда; 1 - один на каждый тик. Затем действуют расовые
модификаторы и XP_Overhaul. Предел ограничивает только дополнительный
опыт этого мода, обычная прокачка игры не меняется.

SourceData/*.ini и 99_UserOverride.ini сохраняют приоритет над автоматикой.
Правка в MCM меняет только выбранный ключ. Таблицы не перечитываются
при движении ползунка; отдельная кнопка нужна после ручной правки файлов.
Сброс восстанавливает поля выбранного источника по таблицам без личного
99_UserOverride.ini, для автоматически найденных - по [Defaults].

Запись прочих работ в UnknownSources.ini - необязательный журнал
ненастроенных рабочих мест, не являющихся добычей. Дополнительный опыт
для них автоматически не включается. Запись выполняется вне XP-хука,
не более 16 источников за кадр; ошибки всегда пишутся в RE_Kenshi_log.txt,
неудачная запись остаётся в очереди с повтором через 5 секунд. Уже
существующие пользовательские значения этого файла сохраняются.

Отключение мода прекращает собственные игровые вызовы до таймера/скана.
Очередь опыта очищается при загрузке/новой игре и изменении настроек.
Нулевые навыки не читаются, узлы очередей и векторы переиспользуются.
Полный каталог перечисляется при открытии настроек, не в каждом кадре.
Debug=1 включает ограниченные сводки; Debug=0 по умолчанию.
Переводы: русский, английский, упрощённый китайский.
Личный INI и таблицы обновление не заменяет. Вторую DLL оригинала не включать.

Колесо мыши работает над любой строкой: три строки за шаг. Стрелки полосы
прокрутки перемещают на одну строку, щелчок по дорожке - на страницу.
Бегунок имеет удобный минимальный размер. У списка есть отступы от краёв;
длинные названия сокращаются с тремя точками, полное имя и ID - в подсказке.
Создаются только строки видимой части списка, без покадрового пересоздания.

Над полями выбранного источника показаны его имя и точный ID. У разных
автоматически обнаруженных жил исходные числа могут совпадать: это не
означает общий профиль. Каждая правка сохраняется отдельно для этого ID.
При Debug=1 журнал различает щелчок по источнику, запрос обновления MCM
и построение его строк с фактическими значениями. Нет записи построения
после запроса - страница MCM ещё не была перестроена.
Повторный щелчок по уже выбранному источнику повторяет запрос обновления
его параметров, если предыдущее обращение попало на пересоздание окна.
