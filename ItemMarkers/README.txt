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
    покупать незачем (раньше был в Dark Ui Tweaks).

Один плагин вместо LtEast's Rarity Backgrounds, LtEast's Item Status
Indicators и LtEast's Mod Settings Core. Их нужно ОТКЛЮЧИТЬ, иначе
пометки будут двойными.


ПОЧЕМУ БЫСТРЕЕ

Плагины LtEast перехватывают обновление окна инвентаря - игра зовёт его
каждый кадр для каждого открытого окна - и оттуда обходят иконки. В
торговле открыто несколько окон с сотнями вещей: падение FPS, при
продаже - рывки. Здесь пометки ставятся, когда игра создаёт иконку, и
уходят вместе с ней. Уже существующая проверка мешочков сделки обходит
видимые иконки раз в 0,4 секунды только при открытой торговле. Помимо
этого в кадре есть опрос клавиши показа, если она задана. Исправление
пометки при перетаскивании не добавляет опроса или обхода каждый кадр.


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
изученных чертежах (Blueprint), подробный журнал.
Меняется сразу, без перезапуска. Те же настройки - в ItemMarkers.ini.


ФАЙЛЫ

ItemMarkers.dll         - плагин RE_Kenshi
ItemMarkers.ini         - настройки
gui/gfx/itemmarkers_*   - картинки стилей (белые, цвет даёт плагин)
locale/                 - перевод (en, ru, zh)

ФАЙЛ НАСТРОЕК И ОБНОВЛЕНИЯ МОДА

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

ПОМЕТКА ПРИ ПЕРЕТАСКИВАНИИ — ПОДГОТОВЛЕНО, RUNTIME-ПРИЁМКА ЕЩЁ НУЖНА

Native isStolen(includeUnknown) зависит от текущего инвентаря. При взятии
вещи мышью игра временно отсоединяет её и очищает current-inventory hand;
разрешимый properOwner при этом может остаться. Тогда чистая собственная
вещь может получить true. Переход на isStolen(false) не является общим
решением: ветка с разрешимым properOwner тоже может вернуть true.

Пометка drag-иконки сохраняет ответ Native ДО отсоединения только для
собственного источника. Ворованная вещь с исходным true остаётся помеченной.
Чужой источник, неразрешимый владелец или ошибка чтения сохраняют обычный
Native-путь без override. Источник определяется публичными SDK API;
снятые/вложенные рюкзаки — существующим ограниченным обходом holder до
персонажа (до четырёх уровней), без предположения по имени фракции.

Снимок привязан к точному drag-item и живёт в записи его иконки. При split
отслеживается именно возвращённая игрой копия, а не другая похожая вещь.
Повторная отрисовка MCM использует этот снимок; новая иконка начинает с
обычной проверки. При удалении виджета запись освобождает существующий
MyGUI Unlinker. Scoped-контекст восстанавливается при обычном возврате,
C++ exception и SEH. Отказ одного из двух новых хуков отключает override
и всегда пишет ошибку. Успешный захват не добавляет постоянного кэша вещей.

Меняется только изображение пометки. Код не исправляет/переписывает
properOwner, theft state, holder, quantity, charges, поворот, оплату или
логику объединения. Native tooltip «украдено у себя» после продажи и редкое
необъединение поднятых вещей — отдельные вопросы, этим патчем не заявлены
исправленными. Асинхронная замена drag-иконок другим модом не проверена;
если иконка не создана в подтверждённом scoped-пути, остаётся Native.

Существующий Debug выключен по умолчанию и остаётся последней настройкой
MCM. Он дополнительно пишет до64пар source/current stolen для drag-иконок;
обычная проверка имеет прежний лимит200диагностических вещей. Новых
настроек или UI-строк нет, переводы ru_RU/en_GB/zh_CN не изменялись.

Проверки: tests/run_tests.py компилирует VC10 host fixtures и точные тела
production-адаптера/SEH из ItemMarkers.cpp. Моделируются clean/stolen drag,
split, чужой источник, снятый/вложенный рюкзак, отмена/новая иконка,
неподходящий clone, C++/SEH unwinding и10тысяч redraw без новых Native
predicate вызовов. Game DLL в host-тесты не загружается. Native-код и
экспорты official KenshiLib0.5.0 сверены offline; доказательства —
tests/native-proof. Живая проверка vanilla/modded UI ещё требуется.

ENGLISH — DRAG BADGE PRESENTATION (RUNTIME ACCEPTANCE PENDING)

Native isStolen depends on the current holder. Detaching a clean owned
item can temporarily leave a resolvable properOwner with no current
inventory and return true; includeUnknown=false does not generally cure
that branch. The exact player-owned drag icon now preserves the native
answer from before detachment. A genuinely stolen source remains marked;
foreign/unknown origins retain the ordinary native predicate.

Public SDK selection/owner APIs identify the source, including equipped,
unequipped and bounded nested backpacks. A split follows the exact returned
clone. The answer belongs to the icon, not the item: settings redraws keep
it, new icons reset it, and normal MyGUI unlink removes it. Scoped state
is restored on success/failure, C++ exceptions and SEH. No per-frame scan
or persistent item cache was added; existing trade-bag rechecks remain.

Only the badge is changed. Ownership/theft, quantity/charges, rotation,
prices, buyback, stacking and save state are untouched. Native post-sale
ownership tooltips and rare ground-item stacking are separate unresolved
issues. Third-party asynchronous icon replacement needs live acceptance.
Optional Debug remains default0/last in MCM, with capped drag diagnostics;
no new player-facing strings/settings require locale changes.

Run tests/run_tests.py with bundled Python: production policy and exact
adapter bodies are exercised in host fixtures without loading game DLLs.
Native disassembly/export evidence is in tests/native-proof. Clean and
real-stolen drag, cancellation, moves, nested bags, split identity,
exceptions, lifetime reset and redraw cost are covered. Runtime validation
in the user's vanilla and modded interface remains pending.
