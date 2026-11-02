ITEM MARKERS (QoL)

Пометки на иконках вещей - в инвентаре, у торговца, в сундуках, на
снаряжении персонажа:

  - цвет сорта у брони, оружия, арбалетов и протезов, как в подсказке
    вещи. Четыре стиля: заливка, рамка со свечением, градиент снизу,
    звезда в выбранном углу (по умолчанию слева снизу). Рамка собрана из полос по краям,
    толщина в пикселях, - на длинной ячейке оружия она не растягивается;
  - курсор кражи игры на краденых вещах;
  - курсор-рука игры на броне, которая является формой фракции;
  - глаз на изученных чертежах: у торговца сразу видно, какой чертёж
    покупать незачем (раньше был в Dark Ui Tweaks);
  - такой же глаз на картах мест, если ВСЕ перечисленные места уже
    открыты. Карты без списка мест и карты с неизвестным местом не помечаются.

Один плагин вместо LtEast's Rarity Backgrounds, LtEast's Item Status
Indicators и LtEast's Mod Settings Core. Их нужно ОТКЛЮЧИТЬ, иначе
пометки будут двойными.


ПОЧЕМУ БЫСТРЕЕ

Плагины LtEast перехватывают обновление окна инвентаря - игра зовёт его
каждый кадр для каждого открытого окна - и оттуда обходят иконки. В
торговле открыто несколько окон с сотнями вещей: падение FPS, при
продаже - рывки. Здесь пометки ставятся один раз, когда игра создаёт
иконку, и уходят вместе с ней. Между обновлениями проверяется клавиша
показа, если она задана. Состояние видимых чертежей и карт обновляется
раз в 0,4 с; в торговле с той же частотой проверяются мешочки сделок.


СОРТ

Сорт - по уровню вещи 0..100, как его хранит игра: до 15 - сорт 0,
до 35 - 1, до 50 - 2, до 65 - 3, до 80 - 4, выше - 5. Подложка видна
сразу, наводить мышь не нужно. Цвета - ItemGrade0..5 из
data/gui/colours/kenshi_colours.xml с учётом модов на интерфейс; цвета
ItemGrade5 в ванили нет - берётся цвет сорта 4, как у самой игры.
По умолчанию красятся сорта 2 и выше (с синего): 0 и 1 - серый и
бежевый, обычные вещи, - настройка MinGrade.


НАСТРОЙКИ В ИГРЕ

Окно «Настройки» -> вкладка MCM -> Item Markers (нужен мод
ModConfigMenu): включение, клавиша показа (NONE - всегда), стиль и
яркость цвета сорта (шаг 10%), с какого сорта красить (MinGrade), угол
звезды, значки краденого и формы фракции (размер, заметность), глаз на
изученных чертежах (Blueprint), глаз на известных картах мест (MapsKnown),
подробный журнал.
Меняется сразу, без перезапуска. Те же настройки - в ItemMarkers.ini.


ФАЙЛЫ

ItemMarkers.dll         - плагин RE_Kenshi
ItemMarkers.ini         - настройки
gui/gfx/itemmarkers_*   - картинки стилей (белые, цвет даёт плагин)
locale/                 - перевод (en, ru, zh)

ФАЙЛ НАСТРОЕК И ОБНОВЛЕНИЯ МОДА

STOLEN-ITEM DIAGNOSIS / ДИАГНОСТИКА КРАЖИ

Native stolen-state indication is preserved during dragging. The temporary
presentation suppression was withdrawn: hiding the badge does not repair
ownership after a sale. Native ownership evidence remains in tests/native-proof;
rare ground-pickup stacking failures and self-theft after sale remain unresolved.

При перетаскивании сохраняется штатная индикация кражи. Временное подавление
значка отменено: скрытие пометки не исправляет владельца вещи после продажи.
Материалы разбора остаются в tests/native-proof. Причины редкого отказа стопок
после подбора с земли и «кражи у себя» после продажи пока не устранены.

ItemMarkers.ini — ваши настройки: обновление мода его не трогает.
В моде лежит ItemMarkers.defaults.ini — значения по умолчанию с пояснениями.
Нет ItemMarkers.ini — плагин при запуске сделает его копией defaults.
Появились в обновлении новые настройки — плагин допишет их в ваш файл
со значениями по умолчанию, остальное оставит как было.

ИЗМЕНЕНИЯ 15.10.2026

- Исправлен вылет: игра сама удаляла детей иконки при её перестройке, а пометки оставались в списке и удалялись второй раз (смена настроек в MCM, клавиша показа). Теперь каждая пометка под присмотром MyGUI (Unlinker, карта пометка -> иконка).

ИЗМЕНЕНИЯ 15.10.2026 (вечер)

- Мешочек сделки (`TradeBags`, цвета `TradeSoldColour` / `TradeBoughtColour`): вещь перешла из рук в руки в открытом окне торговли, если игра пишет у неё «Выкупить» (`Item::getBuyBackGUIInfo` даёт строку). Белая картинка `itemmarkers_bag.png` - значок кражи игры, обесцвеченный (красный значок при перекраске оставался красным). Купили или продали - по хозяину: вверх по цепочке «вещь -> рюкзак -> персонаж», наш персонаж - купили.
- Стопки: игра при сделке в уже лежащую стопку новой иконки не создаёт - пока заданы участники торговли (`ForgottenGUI::tradeA/tradeB`), видимые иконки раз в 0,4 с спрашиваются заново; вещь - по `hand`, не по указателю.
- Мейто: класс оружия «Meitou» (`materialData->stringID == 52293-rebirth.mod`), цвет - ключ палитры `ItemGradeMeitou`, иначе `ItemGrade5`. Строка класса в подсказке перекрашивается в перехватах `Weapon/Sword::getTooltipData1/2` (цвет `ItemGrade5` в строках -> цвет мейто, только у мейто).
- `GameTheme` ищет палитру и в модах Мастерской (`workshop\content\233860\<id>\<Мод>.mod`), корень игры - от kenshi_x64.exe.

ИЗМЕНЕНИЯ 17.10.2026

- Броня без покрытия (деревянные сандалии, часть штанов) - без пометки сорта: Graded() для ARMOUR спрашивает Armour::hasArmourCoverage(). Сорт у неё в данных есть, но ни на что не влияет, и игра его не показывает (замечание игрока AZ45bbc на Nexus).


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.

Known location maps (unreleased) / Известные карты мест (ещё не выпущено)

English
- The same lower-left eye as learned blueprints marks a location map only
  when every location from its original list is discovered. A map with no
  listed locations, a missing location or an unreadable location stays unmarked.
- MapsKnown is enabled by default and independent of Blueprint. Both settings
  apply without restarting. Existing visible icons refresh within 0.4 seconds,
  including inventory, containers and merchant stock.

Русский
- Тот же глаз слева снизу, что у изученных чертежей, отмечает карту мест,
  только если открыты все места из её исходного списка. Карты без мест,
  с отсутствующим или недоступным для проверки местом остаются без глаза.
- MapsKnown включён по умолчанию и не зависит от Blueprint. Перезапуск
  не нужен. Уже видимые иконки обновляются в пределах 0,4 с, в том числе
  в инвентаре, контейнерах и товарах торговца.

Implementation / Реализация
The marker never calls selectTowns, consume or tooltip helpers and never writes
town discovery or item references. Native selectTowns removes discovered towns
and randomizes candidates; its handles and the consumed counter are unsuitable.
Only the visible-icon 400 ms cadence may build one ephemeral 32 KiB table,
shared by that update's icons. Creation and MCM redraw defer map checks to this
cadence. Limits: 8192 world entries, 2048 distinct town data records, 256 source
references per map; saturation, missing data and read faults fail closed.
All native pointers in the table die at return; there is no world cache or
per-frame world traversal. Unchanged state does not create/delete widgets.
Native/host tests are not game FPS, memory-growth or skin acceptance measurements.

Prepared release 6 / Подготовленный выпуск 6 (2026-11-02)

English

- Native stolen-item indication. The game's stolen-item indicator remains visible while dragging. Item ownership and stack-merging rules are unchanged.
- Optional language diagnostics. Language and translation-catalogue summaries follow this plugin's Detailed log setting, which is off by default.
- Useful errors remain visible. Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.
- Clean update package. English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.

Русский

- Штатная отметка краденого. Значок краденой вещи сохраняется при перетаскивании. Правила владения и объединения стопок не меняются.
- Необязательная диагностика языка. Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.
- Ошибки остаются видимыми. Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.
- Чистый пакет обновления. Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.


Prepared release 7 / Подготовленный выпуск 7 (2026-11-03)

English

- Known location maps. An eye marks a map when all locations listed on it have already been discovered. The marker has its own setting.
- Up-to-date markers. Open inventory and merchant windows reflect newly discovered locations and learned blueprints.

Русский

- Карты открытых мест. Глаз отмечает карту, все перечисленные места которой уже открыты. Для метки есть отдельная настройка.
- Актуальные пометки. Уже открытые окна инвентаря и торговли учитывают новые открытые места и изученные чертежи.
