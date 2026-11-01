ModConfigMenu (MCM) — настройки модов прямо в игре

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 10 (29–30.10.2026)

  * Колесо над любой штатной строкой MCM прокручивает страницу: над
    подписью, флажком, ползунком, полем и кнопкой. Случайного изменения
    значения ползунка от колеса больше нет. Событие доходит до штатной
    DatapanelGUI один раз, с её полосой прокрутки и ограничением краёв.
  * Обработка ограничена открытой живой страницей MCM. Собственные
    области плагинов (список источников MultiXPMining), соседние окна
    и отдельные вложенные области прокрутки сохраняют управление.
    Защита от рекурсии не даёт одному событию прокрутить страницу
    дважды. Нет покадрового обхода дерева или повторного навешивания
    обработчиков на каждый элемент при перестройках.
  * Плагин может разрешить мышь в обычной строке назначения клавиши:
    LMB/RMB/MMB/X1/X2, в том числе с Ctrl, Shift и Alt. Первый щелчок
    только включает захват; назначается следующее нажатие. Esc отменяет,
    Backspace снимает назначение. Повторы и отпускание захваченного
    нажатия не передаются остальному интерфейсу.
  * Для авторов: MCM_ALLOW_MOUSE=0x2 — явное согласие строки на мышь;
    коды MCM_KEY_MOUSE_LEFT..MCM_KEY_MOUSE_X2 зарезервированы в диапазоне
    0x1000..0x1004. Включайте флаг только если обработчик плагина умеет
    эти коды. Без него прежние callbacks получают только клавиатуру,
    щелчок во время захвата неподдерживаемую кнопку не назначит.
    Структура и порядок полей API v6 не изменились. Приём мыши проходит
    через штатный InputManager; SDK и руководства обновлены на 3 языках.
  * В этот выпуск также входят: подробный журнал всегда последней настройкой
    каждой страницы; порядок обычных настроек и динамических блоков
    сохраняется. Для динамических строк api->rows больше не копится
    память прошлых показов: она освобождается после завершения
    удаления родной панели, когда callbacks и поля уже не используют
    настройки, включая отложенные и аварийные пути удаления.
  * Подписи собственных кнопок обновляются при реальном изменении,
    а не собираются заново каждый кадр. Длинные текстовые настройки
    читаются целиком до 8191 UTF-8 байта, включая списки правил
    BetterLooting, без старых ограничений в 1023/2047 байт. Чтение —
    при построении страницы и восстановлении отклонённого значения.
  * Звёздочку и пояснение перезапуска добавляет MCM только для
    MCM_RESTART. Живые переключатели не получают её автоматически.
    Новые механизмы колеса и мыши не требуют отдельных настроек;
    для загрузки новой DLL нужен обычный перезапуск игры.
  * Собственная страница Mod Config Menu содержит подробный журнал:
    Debug=0 по умолчанию, изменение действует сразу; ошибки пишутся
    независимо от него. Личный ModConfigMenu.ini сохраняется.
    Переводы ru_RU/en_GB/zh_CN; в выпуске .mo, .po — в исходниках.

VERSION 10 CHANGES (29–30 OCTOBER 2026)

  * The wheel scrolls the settings page over native MCM labels,
    checkboxes, sliders, fields and buttons, without changing control
    values. Each event reaches native DatapanelGUI once, retaining
    its scrollbar and page bounds.
  * Routing is limited to the open, live MCM page. Plugin-owned areas
    such as MultiXPMining's source list, sibling windows and independent
    nested scroll areas retain their own wheel handling. A recursion
    guard prevents duplicate scrolling; no per-frame tree scan or
    repeated per-control handler installation is needed.
  * Plugins can opt into mouse buttons in regular hotkey rows:
    LMB/RMB/MMB/X1/X2, with Ctrl, Shift and Alt modifiers. The initial
    click only arms capture; the next press is assigned. Esc cancels,
    Backspace clears. Repeats and release of the captured press are
    suppressed from the rest of the interface.
  * For authors: MCM_ALLOW_MOUSE=0x2 opts a row in. Reserved mouse codes
    MCM_KEY_MOUSE_LEFT..MCM_KEY_MOUSE_X2 are 0x1000..0x1004. Set the flag
    only when the consumer understands these codes. Existing callbacks
    without it remain keyboard-only; mouse clicks during capture cannot
    assign an unsupported button. API v6 structure and field order are
    unchanged. Capture uses native InputManager; the SDK and its three
    language guides were updated.
  * Also included in this release: Detailed log is moved to the end of every page while
    retaining normal API order and dynamic-block placement. Old
    api->rows settings no longer accumulate between page visits;
    memory is freed after the native panel has finished destruction,
    including deferred and failure paths, when fields and callbacks
    have stopped using it.
  * Owned button captions update on actual changes instead of being
    rebuilt every frame. Text settings, including BetterLooting lists,
    read up to 8191 UTF-8 bytes without the former 1023/2047-byte
    truncation. Reads occur on page construction or rejected-value
    restoration, not every frame.
  * MCM adds the restart star and explanation only for MCM_RESTART;
    live toggles are not automatically marked. Wheel and mouse support
    need no new settings. Restart the game to load the replacement DLL.
  * The Mod Config Menu page exposes Detailed log, off by default
    (Debug=0) and applied immediately. Errors are always logged.
    Updates preserve ModConfigMenu.ini. UI translations cover
    ru_RU/en_GB/zh_CN; releases contain .mo catalogs, .po stays in source.

СПРАВОЧНИК И ИСТОРИЯ

Требуется RE_Kenshi.


ЧТО ДЕЛАЕТ

В окне «Настройки» игры (главное меню или Esc в игре) появляется вкладка
MCM. Слева — список модов, у которых есть настройки, справа — сами
настройки выбранного мода, теми же строками, что и остальные вкладки игры:
флажки, ползунки с полем для числа, кнопки выбора и клавиш, поля ввода.
Цвет выбирается из палитры (кнопка «Палитра» раскрывает клетки под
строкой); кнопка Hex - ввести код #RRGGBB вручную, рядом образец цвета.
Если у настройки есть слово по умолчанию (auto, skin), оно - первой
кнопкой палитры.
Наведите на подпись — внизу окна появится подсказка. Пока открыта
вкладка MCM, в заголовке окна настроек — название выбранного мода,
крупным шрифтом заголовков игры. Надпись раз в секунду
собирается заново: игра иногда перестраивает шрифты, и без этого она
превращалась в кашу из букв. Если модов больше, чем
влезает в окно, у списка появляется полоса прокрутки; листать его можно
и колесом мыши.

Изменение сразу записывается в ini мода и применяется. Помеченное
звёздочкой (*) вступает в силу после перезапуска игры. Кнопка «Сбросить по
умолчанию» внизу страницы возвращает все значения мода.

Разделы страницы — светло-жёлтым и в квадратных скобках: «[ ПАНЕЛЬ ]».
Оформление одно на все моды, его задаёт MCM, авторам плагинов ничего
делать не нужно.


КАКИЕ МОДЫ

Любой плагин RE_Kenshi, который экспортирует настройки для MCM.

Наши плагины: AI Fixed, Assigned Workers, Best Minimap, Build Mode Gizmo, Gate Push Guard, Gear Compare Alt, Item Markers, Live Xp Bars, Multi XP Mining, Patient Self Aid, Quick Start Select, Ranged Damage Xp, Recruit Markers, Skill Level Up Info, Stat Colours, UI Tweaks, Wanted Map.

Наши форки Emkej8: Auto Pause on Load, Container Highlight, Hidden Faction Relations, Job-B-Gone, Loot-Scoot-Execute, Map-markers, Organize the Crafting Stations, Organize the Inventory, Organize the Trader, Vital Read, Vital Sense, Wall-B-Gone.

Наши форки других авторов: Better Looting (XxAtreuSSxX), Character Highlight (BFrizzleFoShizzle), Character Inspector (shutori), Formations (Slinkysalmon03), Kenshi Job Management (SirixKC), Rotate Items (jacobericson), Kill Counter (Rokeshi), Stack Sort (Jakematt2004), Taunt Expansion (tata / Tata-Gari).

В список входят подготовленные и отключённые сборки. Dynamic Factions
продолжает разрабатываться; Organize the Inventory — локальная сборка.
Taunt Expansion и Gate Push Guard подготовлены и ожидают игровой приёмки.
Global Satellite Map - локальный прототип отдельного слоя глобальной карты.
Плагины устанавливаются отдельно; без MCM читают настройки из файлов.

SUPPORTED PLUGINS

Our plugins: AI Fixed, Assigned Workers, Best Minimap, Build Mode Gizmo, Gate Push Guard, Gear Compare Alt, Item Markers, Live Xp Bars, Multi XP Mining, Patient Self Aid, Quick Start Select, Ranged Damage Xp, Recruit Markers, Skill Level Up Info, Stat Colours, UI Tweaks, Wanted Map.

Our Emkej8 forks: Auto Pause on Load, Container Highlight, Hidden Faction Relations, Job-B-Gone, Loot-Scoot-Execute, Map-markers, Organize the Crafting Stations, Organize the Inventory, Organize the Trader, Vital Read, Vital Sense, Wall-B-Gone.

Our forks of other authors: Better Looting (XxAtreuSSxX), Character Highlight (BFrizzleFoShizzle), Character Inspector (shutori), Formations (Slinkysalmon03), Kenshi Job Management (SirixKC), Rotate Items (jacobericson), Kill Counter (Rokeshi), Stack Sort (Jakematt2004), Taunt Expansion (tata / Tata-Gari).

Includes prepared and disabled builds. Dynamic Factions remains in
development; Organize the Inventory is a local build. Taunt Expansion and
Gate Push Guard are prepared and await in-game acceptance. Global Satellite
Map is a local prototype of a separate world-map layer. Install plugins
separately; without MCM they read their own configuration files.


ДЛЯ АВТОРОВ ПЛАГИНОВ

Плагину MCM не обязателен: настройки по-прежнему лежат в его ini. Чтобы
появиться во вкладке, плагин экспортирует функцию MCM_Describe и описывает
в ней свои настройки. MCM находит плагины сам, порядок загрузки модов не
важен. Всё нужное — в папке sdk: заголовок ModConfigMenu.h и руководство
MCM_API_ru.md (английское — MCM_API_en.md, китайское — MCM_API_zh.md).


НАСТРОЙКИ

ModConfigMenu.ini рядом с DLL: Debug=1 — подробности в RE_Kenshi_log.txt.


ИЗМЕНЕНИЯ

Версия 2:
  * Вылет при возврате в главное меню модом ReturnToMainMenu. Тот сносит
    интерфейс игры, а MCM продолжал обращаться к своей удалённой панели.
    Теперь MCM замечает, что его вкладку или панель удалили, забывает о
    них и строит вкладку заново при следующем открытии окна.
  * Выпадающие списки (например, клавиша показа у Assigned Workers и
    Gear Compare Alt) не сохраняли выбор: выбранный пункт теперь берётся
    у самого списка.
  * Разделы — светло-жёлтым и в скобках: на ванильном интерфейсе
    заголовки были чёрными, еле видно. Разделитель между списком модов и
    настройками толще.
  * Выпадающих списков больше нет. Выпадающий список игры в окне
    настроек в игре выбор не отдавал (в главном меню работал), и клавиши
    у модов не переключались. Теперь вариант — кнопкой: щелчок —
    следующий, SHIFT+щелчок — предыдущий. Клавиша — как в настройках
    управления игры: щелчок по кнопке, потом нажать клавишу или
    сочетание; Backspace — без клавиши, повторный щелчок — отмена.
  * Исправлен вылет при закрытии окна настроек: игра удаляет панель
    строк раньше вкладки, и MCM успевал обратиться к удалённой строке.

ФАЙЛ НАСТРОЕК И ОБНОВЛЕНИЯ МОДА

ModConfigMenu.ini — ваши настройки: обновление мода его не трогает.
В моде лежит ModConfigMenu.defaults.ini — значения по умолчанию с пояснениями.
Нет ModConfigMenu.ini — плагин при запуске сделает его копией defaults.
Появились в обновлении новые настройки — плагин допишет их в ваш файл
со значениями по умолчанию, остальное оставит как было.

ИЗМЕНЕНИЯ 15.10.2026

- Исправлен вылет после смены размера шрифта: `ForgottenGUI::changeFontSize` пересоздаёт текстуры шрифтов, а текст в окнах модов держал старую. После вызова игры всем надписям интерфейса шрифт назначается заново (shared/FontRefresh.h).
- Своя страница в списке модов: подробный журнал.
- Данные вкладки очищаются сразу после удаления панели и после сбоя её сборки - окно настроек не держит адрес удалённой панели (замечание Асура).

ИЗМЕНЕНИЯ 16.10.2026

- Вылет при открытии настроек после смены размера шрифта (дамп Асура: последние MCM и LiveXpBars, а вылет остался). FontRefresh спотыкался на первой надписи без шрифта (MyGUI читает имя через пустой шрифт) и дальше не шёл - остальной интерфейс держал старые текстуры. Теперь сбой одной надписи пропускается, обход идёт дальше; кроме TextBox шрифт заново получают все виджеты с текстом в скине. В логе: сколько надписей без шрифта пропущено (норма) и отдельно - если шрифт не встал (ошибка).

ИЗМЕНЕНИЯ 17.10.2026

- API v6: api->rows(fn, ud) - строки, которые строятся при каждом показе страницы (отряды загруженного сейва и т.п.). MCM зовёт fn перед показом, строки встают туда, где был вызван rows; до v10 строки прошлого показа уходили в retired. В v10 они освобождаются после завершения удаления родной игровой панели, когда её поля и callbacks уже не используют память настройки. Первый пользователь - Formations (своя фигура строя у каждого отряда).

ИЗМЕНЕНИЯ 21.10.2026

- FontRefresh больше не ловит исключение на надписях без шрифта: сначала проверяет, есть ли у текстовой части шрифт (EditText::mFont), и только потом спрашивает его имя. Раньше чтение имени через пустой шрифт давало нарушение доступа (ModConfigMenu.dll+0x14E34, чтение по 0x18) - мы его гасили, игра не падала, но KEO записывал его в crash_dump.txt при каждом открытии настроек (Асур).

ИЗМЕНЕНИЯ 21.10.2026 (v9)

- Место под открытую палитру цвета - две пустые строки панели высотой в строку настройки (OWN_SPACER), а не DatapanelGUI::addSpace: тот давал пустоты в несколько строк, разные на разных страницах.


ИЗМЕНЕНИЯ 29.10.2026 (v10)

- Подробный журнал всегда последней настройкой на всех страницах; порядок регистрации API и позиции динамических блоков сохраняются.
- Убрано накопление прежних динамических настроек. Их память освобождается после настоящего деструктора панели, включая аварийные и отложенные сценарии.
- Подписи собственных кнопок не собираются заново каждый кадр; обновляются при реальном изменении.
- Звёздочка означает настоящее требование перезапуска (MCM_RESTART), а не сам факт наличия выключателя. Живые переключатели применяются во время игры.
- В релизе только рабочие .mo на русском, английском и китайском; исходные .po остаются в репозитории.

Текстовые настройки MCM сохраняют длинные списки до 8191 UTF-8 байта
без прежнего ограничения чтения в 1023/2047 байт. Чтение выполняется при
построении страницы и восстановлении отклонённого значения, не покадрово.


30.10.2026: колесо над любыми нативными строками настроек MCM прокручивает
страницу: над флажками, ползунками, полями, подписями и кнопками. Значение
настройки от колеса не меняется. Используется штатная прокрутка DatapanelGUI,
включая ползунок полосы и ограничение краёв страницы. Собственные области
плагинов (например, список источников MultiXPMining) и вложенные отдельные
области прокрутки сохраняют своё управление. Обхода дерева на каждом кадре
и новых делегатов на каждой перестройке нет; закрытая страница не перехватывает
колесо. Новых настроек нет; обновлённая DLL загружается после обычного перезапуска игры.


30.10.2026: плагины могут разрешить назначение кнопок мыши в обычной строке
клавиши (флаг MCM_ALLOW_MOUSE). Поддерживаются LMB/RMB/MMB/X1/X2 с CTRL,
SHIFT и ALT. Первый щелчок только открывает захват; назначается следующее
нажатие. Esc отменяет, Backspace убирает назначение. Отпускание и повтор
назначенного нажатия не передаются остальному интерфейсу. Без разрешающего
флага старые плагины по-прежнему получают только клавиатурные коды; щелчок
мыши во время захвата не назначает неподдержанную кнопку. Структура API v6
и порядок её полей не менялись.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.

Prepared release 11 / Подготовленный выпуск 11 (2026-11-02)

English

- Optional language diagnostics. Language and translation-catalogue summaries follow this plugin's Detailed log setting, which is off by default.
- Useful errors remain visible. Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.
- Clean update package. English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.

Русский

- Необязательная диагностика языка. Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.
- Ошибки остаются видимыми. Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.
- Чистый пакет обновления. Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.
