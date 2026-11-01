UI Tweaks — доработки интерфейса, которых не сделать разметкой

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 12 (31.10–01.11.2026)

  * Снимки сохранений: игра с HUD, мини-картой и панелями персонажей,
    без меню сохранения/загрузки, меню паузы и его затемнения. Предпросмотр
    выбранного существующего слота под окном сохранения или загрузки.
    Окно сразу открывается выше, оставляя место под снимок; смена строки
    не сдвигает его. Рамка идёт по ширине окна, снимок не обрезается.
  * Ручные сохранения, перезапись, quicksave и autosave используют реальный
    каталог слота. Снимок обновляется только при успешной записи. Старым
    сейвам изображение не требуется; без снимка рамка полностью скрыта.
  * Новые снимки заметно чётче: до 1600×900, с качественным уменьшением
    и без потерь при сжатии. Старые снимки по-прежнему поддерживаются.
  * Высота окон сохранения и загрузки настраивается в MCM: по умолчанию
    120% исходной, чтобы в списке помещалось больше строк. Изменение
    применяется при следующем открытии и учитывает место под снимок.
  * Последовательный обмен с союзниками через ПКМ по портрету работает
    при уже открытом обмене. Штатный подход к далёкой цели сохранён.
  * Защита от клика сквозь инвентарь и отдельный рюкзак сохранена.
    Портреты, их меню и карта больше не считаются окнами инвентаря.
  * Проверка принадлежности виджета выполняется только на новом ПКМ.
    До 64 родителей собираются один раз на стеке; ссылки не кэшируются.
    Существующая защёлка старого клика и пять хуков приказов не изменены.
  * SavePreviews в MCM включён по умолчанию и применяется сразу.
    Настройка переведена на RU/EN/ZH. Личные INI сохраняются.
    Для загрузки заменённой DLL перезапустите игру.

VERSION 12 CHANGES (31 OCTOBER–1 NOVEMBER 2026)

  * Saved game screenshots include the HUD, minimap and character panels,
    excluding save/load and pause menus with their backdrop. Selecting a slot
    shows its preview below Save Game or Load Game. The dialog opens higher
    immediately to reserve preview space and stays put when changing rows.
    The frame spans the dialog; the image keeps its proportions without cropping.
  * Manual saves, overwrites, quicksaves and autosaves use the actual slot
    directory. A preview is committed only after a successful save. Old saves
    need no image; the preview and its frame stay hidden when no image is available.
  * New screenshots are clearer: up to 1600×900, with filtered resizing and
    lossless compression. Previously captured previews remain compatible.
  * Save/load window height is configurable in MCM, defaulting to 120% of
    the original height to show more rows. Changes apply on the next opening
    and respect the available screen and preview space.
  * Sequential squad trading through portrait RMB/Trade works while an
    earlier exchange remains open. Native out-of-range approach is retained.
  * Inventory and detached-backpack click-through protection remains active.
    Portraits, their context menus and the map are not inventory windows.
  * Window ancestry is checked only on a fresh RMB press. At most 64 parents
    are collected once on the stack, with no retained widget references.
    The existing stale-click latch and five order hooks are unchanged.
  * SavePreviews in MCM is on by default and changes immediately.
    Its setting is available in RU/EN/ZH; personal INI is preserved.
    Restart the game to load the replacement DLL.

ИСТОРИЯ: ВЕРСИЯ 11 (30.10.2026)

  * Совместимость с новой верхней строкой Organize the Trader:
    деньги торговца остаются слева, итог сделки — по центру.
    TraderMoneyCentre не перезаписывает разметку этой строки.
    Обычные денежные панели по-прежнему обрабатываются как раньше.
  * Отдельной настройки нет; личный UITweaks.ini сохраняется.
    Для загрузки новой DLL нужен перезапуск игры.
  * Технически пропускается только денежная панель с временной
    меткой LEOPARD_OTT_MoneyHeader. Метку ставит и снимает Organize
    the Trader, когда управляет своей строкой. Остальные окна и
    функция TraderMoneyCentre не отключаются.

VERSION 11 CHANGES (30 OCTOBER 2026)

  * Compatibility with Organize the Trader's custom header:
    trader money stays on the left and the deal in the centre.
    TraderMoneyCentre no longer overwrites this header layout.
    Ordinary money panels retain their previous behaviour.
  * No additional setting is required; UITweaks.ini is preserved.
    Restart the game to load the replacement DLL.
  * The guard applies only to a money panel bearing the temporary
    LEOPARD_OTT_MoneyHeader marker. Organize the Trader acquires and
    releases that marker while managing its custom header. Other
    windows and the TraderMoneyCentre feature remain active.

ИСТОРИЯ: ВЕРСИЯ 10 (30.10.2026)

  * SkipSameMoves теперь пропускает и повторную установку относительного
    размера обычного MyGUI::Widget, если штатное преобразование в
    пиксели даёт прежний IntSize. Игра обновляла так шесть полос
    состояния каждый кадр, заставляя MyGUI заново перестраивать
    неизменный интерфейс. Прежняя оптимизация одинаковых координат
    подписей над персонажами сохранена.
  * Перевод долей в пиксели выполняет сама MyGUI: округление, размеры
    родителя и видимой области не пересчитываются своей формулой.
    Проверка ограничена точным виджетом текущего setRealSize и только
    обычным Widget. Производные элементы и реальные изменения размера
    проходят штатным путём.
  * Явный setSize, в том числе обновление с тем же размером, сохраняет
    обычные уведомления. Временный маркер потребляется до callbacks,
    поэтому повторный вызов из обработчика не блокируется. Предыдущее
    состояние восстанавливается при выходе, включая исключения.
    Обрезка родителем и вложенные обновления не отключаются.
  * Настройка MCM «Пропуск неизменной геометрии» использует прежний
    ключ SkipSameMoves, по умолчанию включена и применяется на ходу.
    Новой личной настройки или ручного переноса INI не требуется.
    Enabled, ScrollDescription и RandomName по-прежнему требуют
    перезапуска и отмечены MCM_RESTART; остальные строки применяются
    сразу. Новая DLL загружается после перезапуска игры.
  * Это ограничение лишних обновлений интерфейса, а не изменение
    игровой симуляции. Диагностический пробник не входит в публичный
    пакет. Число пересборок и вершин MyGUI не является числом вызовов
    отрисовки GPU; README не обещает фиксированного прироста FPS.
  * Подробный журнал — последняя настройка MCM, выключен по умолчанию
    (Debug=0); ошибки пишутся всегда. Личный UITweaks.ini сохраняется.
    Переводы ru_RU/en_GB/zh_CN; в пакете .mo, .po остаются в исходниках.

VERSION 10 CHANGES (30 OCTOBER 2026)

  * SkipSameMoves now also skips repeated relative sizing of plain
    MyGUI::Widget objects when native conversion produces the same
    integer pixel size. The game did this for six status bars every
    frame, repeatedly rebuilding unchanged UI geometry. Existing
    unchanged-position filtering for overhead labels remains active.
  * MyGUI still performs relative-to-pixel conversion, retaining its
    rounding and parent/view dimensions. The guard is limited to the
    exact widget in the active setRealSize call and plain Widget type.
    Derived controls and actual pixel-size changes take the native path.
  * Explicit setSize refreshes, including same-size refreshes, retain
    normal notifications. The temporary marker is consumed before
    callbacks, so reentrant updates are preserved; previous state is
    restored on exit, including exceptions. Parent clipping and nested
    updates remain functional.
  * MCM's "Skip unchanged UI geometry" uses the existing SkipSameMoves
    key, defaults to on and applies immediately. There is no new personal
    setting or manual INI migration. Enabled, ScrollDescription and
    RandomName still require restart and are marked MCM_RESTART;
    other settings apply live. Restart to load the replacement DLL.
  * This reduces redundant UI updates without changing the simulation.
    The diagnostic probe is excluded from public packages. MyGUI
    rebuild/vertex counts are not GPU draw-call counts; no fixed FPS
    improvement is promised.
  * Detailed log is last in MCM and off by default (Debug=0); errors
    are always logged. Updates preserve UITweaks.ini. UI translations:
    ru_RU/en_GB/zh_CN; release .mo catalogs only, .po remains in source.

СПРАВОЧНИК И ИСТОРИЯ

Требуется RE_Kenshi.


ЧТО ДЕЛАЕТ

Всё ниже игра задаёт своим кодом, а не файлами разметки, поэтому ни один
мод на интерфейс этого не меняет. Каждая доработка включается и
выключается отдельно в UITweaks.ini.

1. Строки очередей исследований и ремесла выше.
   Игра даёт строке около 16 пикселей, и на русских шрифтах текст налезает
   сам на себя. Высота — QueueRowHeight (30).

2. Полоса в строке очереди — во всю рамку.
   Игра считает сто процентов полосы по ширине надписи со временем
   («2 hrs»), а не по рамке строки, поэтому заливка не доходила до конца.
   Плагин прячет игровую полосу и рисует свою, от чёрной линии рамки до
   чёрной линии. Отступ от края — QueueBarInset (5).

3. Прокрутка в описании на вкладках «Изучение» и «Ремесло».
   Длинное описание (у доспехов — полтора десятка строк) не влезало. У
   панели описания есть штатная прокрутка, игра её просто не включает.
   Плагин включает и держит ползунок на месте, когда описание обновляется.
   ScrollDescription (1).

4. Ячейки отрядов на вкладке «Отряды» выше.
   Портреты в ячейке были видны на полтора ряда. Теперь минимум
   SquadPortraitRows рядов (2), а при SquadAutoGrow=1 ячейки растут под
   самый большой отряд: добавился боец и нужен новый ряд — ячейки
   вырастут, ушёл — сожмутся обратно. Высота у всех ячеек одна (так
   устроен список игры), не больше шести рядов.

5. Полоса «Следующий уровень» в описании навыка.
   Игра делала её тонкой, заливка лежала поверх чёрной каймы, подпись —
   поверх полосы. Плагин делает рамку выше (ProgressBarPad), вписывает
   заливку внутрь каймы, ставит текст по середине (ProgressBarDrop,
   ProgressBarGrowUp) и сдвигает строки ниже, чтобы полоса на них не
   наезжала.

6. Отношение фракции у имени выбранного персонажа.
   В плашке над панелью выбранного после названия фракции - её отношение
   к вам, как в окне фракций игры: «Кочевники (+15)». Скобки серые,
   число зелёным - выше нуля, красным - от -10 и ниже (там уже
   нападают), между -10 и 0 - цветом надписи. Только у персонажей: у
   своих и у построек приписки нет. FactionRelation (1).
   По желанию - только пока зажата клавиша: FactionRelationKey (NONE -
   всегда; ALT, CTRL+B, ...).

7. Глаз на изученных чертежах - с 09.10.2026 в моде Item Markers (там все
   пометки на иконках вещей).

8. Иконка вещи в списке ремесла.
   Во вкладке «Ремесло» при наведении на строку списка «Предметы» у мыши
   появляется иконка вещи. Иконки игра рисует сама (папка data/icons:
   сначала с материалом рецепта, для оружия - с производителем игрока; у
   вещей с готовой картинкой - поле icon). Вещь, которой у вас ещё не
   было, рисуется один раз на лету. CraftIcon (1). Клавиша иконки -
   CraftIconKey (ALT): иконка видна, пока клавиша зажата; NONE - просто по
   наведению.
   В MCM есть кнопка «Нарисовать иконки всех вещей»: в загруженной игре
   рисует иконки всем вещам, у которых их ещё нет, по одной за кадр, ход -
   надписью вверху экрана. Рисунки остаются в data/icons. Вещи, которые
   игра создать не даёт (тренировочное оружие из мода), остаются без иконки.

9. Кнопка «Случайно» у имени в редакторе персонажа (v8, 18.10.2026).
   В окне редактора (новая игра, найм, хирург) справа от поля имени -
   кнопка: случайное имя из списков игры namesM.txt (мужские), namesF.txt
   (женские) и namesMF.txt (общие) - мужчине мужские и общие, женщине
   женские и общие, по полу персонажа в редакторе. Списки читаются тем же
   загрузчиком ресурсов, что у игры, поэтому мод, подменяющий их
   (Expanded Name Set и подобные), учитывается сам. Имя ставится в поле и
   применяется обработчиком самого окна (CharacterEditWindow::nameChanged)
   - так же, как при вводе с клавиатуры. Поле имени становится уже на
   ширину кнопки. RandomName (1), после изменения - перезапуск игры.

10. Сдвиг на то же место (v9, 22.10.2026).
   Игра каждый кадр зовёт Widget::setPosition для каждой подписи над
   персонажами (~150 раз за кадр), почти всегда с теми же координатами.
   MyGUI равенство не проверяет и каждый раз помечает слой Back на
   пересборку: все подписи пересобирались каждый кадр. Плагин пропускает
   такой вызов, когда место то же. SkipSameMoves (1), действует сразу.

11. Снимки сохранений.
   При успешном сохранении сохраняется кадр игры с HUD, мини-картой и
   панелями персонажей. Меню сохранения/загрузки, меню паузы и его затемнение
   в снимок не входят. Переключения окон и захвата рабочего стола нет.
   При выборе существующего слота его снимок появляется под окном сохранения
   или загрузки, в главном меню и в уже загруженной игре. Поддерживаются ручные сохранения,
   перезапись, quicksave и сменяемые autosave. Старые сейвы без снимка
   загружаются как обычно; без изображения рамка полностью скрыта.
   До выбора строки рамка скрыта, в том числе при автоматически подсвеченной
   первой строке. Нажатие на эту строку тоже показывает снимок. Пропорции
   сохраняются без обрезки: рамка растягивается по ширине окна.
   Окно сразу при открытии поднимается выше, оставляя место под снимок,
   и не сдвигается при выборе другой строки. При нехватке высоты снимок
   уменьшается без обрезки и располагается по центру рамки;
   список, кнопки и правая панель остаются штатными.
   SavePreviews (1), MCM «Сохранённые игры», действует сразу.
   «Высота окна сохранения/загрузки, %» — 100–160, по умолчанию 120;
   применяется при следующем открытии окна. 100 возвращает исходную высоту.
   Увеличивается сам список сейвов; размер не накапливается при повторных
   открытиях. Ограничения экрана и места под снимок учитываются автоматически.


С КАКИМ ИНТЕРФЕЙСОМ РАБОТАЕТ

С любым: с ванильным и с любым модом на интерфейс. Плагин не заменяет ни
одного файла разметки или скина — он правит размеры виджетов, которые
игра строит сама, прямо во время игры.

Оговорки:

  * пункты 1, 3, 4 и 5 от разметки не зависят вовсе;
  * пункт 2 находит в строке очереди надписи ItemValue и полосу Progress
    по их именам из ванильной разметки. Если мод на интерфейс их
    переименует, плагин эту строку просто не тронет — полоса останется
    игровой;
  * отступы заливки (пункты 2 и 5) подогнаны под рамку ванильного скина
    Kenshi_Button1 — у неё кайма в шесть пикселей. Если мод на интерфейс
    рисует рамку иначе, подгоните QueueBarInset и ProgressBarPad в ini.

Правки разметки мода «Russian Dark UI simplistic» (ширина колонок, место
под кнопкой удаления в очереди и т.п.) — не часть плагина: они лежат в
самом моде интерфейса.


НАГРУЗКА

Каждый кадр плагин обходит окна интерфейса и ищет очереди, ячейки
отрядов и панели описания — игра переписывает их размеры постоянно.
Скрытые окна он пропускает целиком: их не видно, и правка им не нужна.
Открытое окно поправляется в том же кадре или в следующем.

Снимки сохранений не сканируют дерево интерфейса или каталоги сейвов
каждый кадр. Читается только выбранный файл; в памяти — одна GPU-миниатюра.
Захват — один раз на принятый запрос сохранения, не более 8 294 400 пикселей
(бюджет 3840×2160). Кадры большего размера пропускаются с записью ошибки.
На CPU снимок уменьшается до 1600×900 без растяжения, с фильтрацией WIC Fant;
один фоновый поток кодирует PNG без потерь и записывает файл, очередь не растёт.
Подробности — ниже.

SAVE PREVIEWS / ТЕХНИЧЕСКИЙ СПРАВОЧНИК

  SavePreviews=1: live MCM toggle, no restart flag. The preview appears below
  the existing Save Game or Load Game dialog. Native controls retain their layout.
  The dialog is raised once when opened to reserve preview space; row changes
  never move it. User dragging is respected. The frame inherits the XML skin.
  Missing/invalid previews have no visible frame or placeholder.
  SaveDialogHeightPercent=120 (range 100..160) expands the native window and
  its vertically stretched GamesList. It applies at the next opening, including
  when screenshots are disabled. Separate weak-root base-size records for Save
  and Load prevent repeated or alternating opens from compounding the size.
  Height is capped by the viewport and reserved preview space; 100 restores the
  native base height. No game restart is required for this setting.

  uitweaks-preview.bin is an optional, separately owned file inside the exact
  save directory. It contains a 40-byte versioned header and a lossless PNG (up to
  1600x900, at most 6 MiB). Old version-1 JPEG sidecars remain readable. The header
  binds the image to quick.save's size and last-write
  time. A stale image after an overwrite is ignored. Copying the complete save
  with preserved quick.save timestamps carries the preview; a missing image does
  not affect loading. External edits deliberately preserving both size and time
  cannot be detected by this lightweight revision stamp.

  Capture reads the current game's backbuffer after HUD drawing, never the
  desktop or undefined old swapchain contents. Only the backend's main
  IRenderTarget is accepted; GUI RTTs and title screens are excluded.
  With no menu open, a single normal GUI pass is captured. If a save/load/pause
  modal is visible, one bounded private GPU scene backup precedes an extra HUD
  draw with only modal draw alpha masked. Native alpha traversal includes
  private skin children and InheritsAlpha=false descendants. Exact own alphas,
  GPU scene pixels and OM render-target bindings are restored before the normal
  full GUI pass for presentation. Widget visibility and input focus are untouched;
  no masked frame is presented. The backup exists for one accepted save only,
  capped at 64 MiB including sample count; no continuous image cache is kept.
  SaveFileSystem::saveGame only accepts the request: commit
  additionally requires COMPLETE and an empty failedToCopyError, inspected before
  native sync clears it, and the same actual save path. Load/new game, disabling
  the option and world/renderer changes invalidate pending work.

  Worker data is owned BGR bytes, a path, revision and synchronization state.
  It never receives game, MyGUI or Ogre objects. Before atomic replacement it
  rechecks the native main-file revision and cancellation. Only one writer and
  one pending capture exist. GUI textures upload through Ogre blitFromMemory,
  which handles driver row pitch. No per-frame INI reads, decode or save scans.

  ForgottenGUI::shutDown releases the owned panel, texture and widget-unlink
  registrations before the native manager dies; registrations can be rebuilt
  after GUI recreation. Ogre::Root::shutdown cancels and waits up to five seconds
  for the CPU worker, outside DllMain. A timed-out job remains bounded and owned
  until process exit; it cannot access engine objects. No thread is terminated.

  Ошибки захвата/формата/записи видны всегда. Успешные снимки и запись файла —
  только при Debug=1. Журнал по умолчанию выключен и остаётся последним в MCM.
  Рабочие INI и сейвы разработчика не включаются в пакет. При отключении функции
  уже записанные файлы не удаляются: их можно удалить отдельно без ущерба сейву.

  Offline tests cover revision gating, copy errors cleared by native sync,
  refused saves, load/new cancellation, writer cancellation/atomic replacement,
  WIC round trips and malformed inputs, size bounds and aspect-preserving layout.
  Native save/selection/target ABI was audited read-only in Steam 1.0.65 with the
  installed KenshiLib 0.5. Runtime manual/quicksave/rolling-autosave, both Load
  contexts, vanilla/Dark UI, FPS and long-run memory measurements remain required
  before declaring the new feature accepted in game. Offline tests do not prove
  freedom from every possible driver leak or universal UI compatibility.


НАСТРОЙКИ В ИГРЕ

Если установлен мод ModConfigMenu, все настройки есть прямо в игре:
окно «Настройки», вкладка MCM, страница «UI Tweaks». У каждой строки
подсказка внизу окна, изменения сразу пишутся в ini и применяются.
После перезапуска игры действуют «Включено», полоса прокрутки описаний
и кнопка случайного имени — эти строки помечены звёздочкой (*).


НАСТРОЙКИ

UITweaks.ini рядом с DLL, читается при запуске игры; из вкладки
MCM — сразу. Если файла нет,
плагин создаст его сам с настройками по умолчанию.

ПЕРЕИМЕНОВАНИЕ

До 11.10.2026 мод назывался Dark Ui Tweaks (папка и файлы DarkUiTweaks):
название сбивало с толку - мод работает с любым интерфейсом, не только с
Dark UI. После обновления включите UI Tweaks в списке модов заново. Если
ставили вручную - удалите старую папку DarkUiTweaks: два плагина сразу
не нужны (новый, увидев старый, сам выключится и напишет об этом в
журнал). Настройки переносятся сами: при первом запуске UI Tweaks берёт
их из DarkUiTweaks.ini.


ФАЙЛ НАСТРОЕК И ОБНОВЛЕНИЯ МОДА

UITweaks.ini — ваши настройки: обновление мода его не трогает.
В моде лежит UITweaks.defaults.ini — значения по умолчанию с пояснениями.
Нет UITweaks.ini — плагин при запуске сделает его копией defaults.
Появились в обновлении новые настройки — плагин допишет их в ваш файл
со значениями по умолчанию, остальное оставит как было.

ИЗМЕНЕНИЯ 15.10.2026

- Деньги торговца по центру окна (`TraderMoneyCentre`, MCM «Окно торговца»). Только в окне торговца (опознание по именам разметки, как в Organize-the-Trader) и только когда плашка денег шириной почти во всё окно (Dark UI) - ванильное окно не трогается. Строка денег растягивается на ширину блока, правый край надписи суммы - центр окна плюс половина текста. Сама разметка Dark UI правится скриптом D:\DEV\Kenshi\fix_darkui_tradermoney.py (плашка и блок растягиваются с окном).

ИЗМЕНЕНИЯ 16.10.2026

- ПКМ сквозь окно торговли (исправление, без настройки). На паузе в торговле щелчок ПКМ по окну уходил и в мир, и после закрытия окна персонаж бежал в эту точку. Журнал показал: приказ «идти» (addOrderSelectedCharacters, task 29) приходит не в момент щелчка, а после закрытия окна - сериями по ~30 кадров, курсор уже в мире: игра доигрывает нажатие ПКМ, запомненное при открытом окне. Правило: нажатие ПКМ над окном инвентаря (открыто окно инвентаря или торговли и у курсора фокус мыши MyGUI) - «чужое», приказы игрока (addOrderSelectedCharacters, newPlayerTaskSelectedCharacters, updateLastMoveWaypointSelectedCharacters, playerMove, Character::playerMoveOrderDefault) отбрасываются, пока ПКМ не нажмут снова над миром. Нажатие ловится по переходу кнопки (GetAsyncKeyState, с учётом обмена кнопок) каждый кадр и в самих хуках. В журнал - одна строка на серию.
- Итог разбора (16.10.2026, журнал с диагностикой): виновник - Character::playerMoveOrderDefault. Его зовут прямо при ОТПУСКАНИИ ПКМ над содержимым рюкзака своего персонажа (backpack_content) в окне торговли, на паузе: игра считает, что курсор над миром. Пауза держит персонажа, после закрытия окна он бежит. Перехват этой функции отбрасывает приказ (7 из 7 в пробе, баг не повторился). В строке журнала об отброшенном приказе - откуда он пришёл (модуль и смещение), чтобы понять, игра это или мод.
- Ревью: «над интерфейсом» - только при открытом окне инвентаря и с фокусом мыши на элементе. Шире нельзя: ПКМ по большой карте тоже приказ идти, а с открытым инвентарём можно водить персонажей по миру. Перебор всех окон в поисках окна торговца убран (в торговле открыты три окна инвентаря, фокус на элементе есть всегда).

ИЗМЕНЕНИЯ 21.10.2026 (v9)

- Журнал вкладки «Диалоги» (MessagesTextBox) на тёмном интерфейсе: игра пишет реплики NPC с зашитым в код тегом #000000; при палитре из мода на интерфейс (GameTheme::PaletteFromMod) тег заменяется на #a0a0a0. Перебор - только когда текст журнала изменился (длина), прокрутка сохраняется. На ванильном пергаменте не трогается.

- Сдвиг на то же место (SkipSameMoves, MCM «Производительность»): перехват MyGUI Widget::setPosition(IntPoint) и setPosition(int,int) - вызов пропускается, если место не изменилось. Замер пробой LayerProbe (MyGUI LayerNode::outOfDate, кто помечает слой): слой Back - 156 пометок и 80 пересобранных пачек за кадр, все от игры через setPosition подписей над персонажами; после правки - 1-3 пачки (только то, что действительно сдвинулось).
- Важно: setPosition(IntPoint) - виртуальная из ICroppedRectangle, this в ней - ICroppedRectangle* (Widget+8). Первая сборка принимала его за Widget* и читала место со сдвигом: подписи всё равно пересобирались, а изредка совпавший мусор пропускал нужный сдвиг - пропадали портреты во вкладке «Отряды». Исправлено до выпуска.

30.10.2026, v10: оптимизация повторных размеров интерфейса
- SkipSameMoves теперь включает проверенный путь относительных размеров
  обычных Widget: штатная MyGUI переводит доли в пиксели, затем одинаковый
  размер отсекается. Это убирает повторную перестройку скинов полос состояния.
- Изменение пиксельного размера выполняется как обычно. Производные элементы,
  явные setSize-обновления и обновления из callbacks не блокируются.
- Маркер существует только на время вызова, привязан к конкретному элементу
  и очищается до callbacks; сохранение состояния и потоки рендера не меняются.
- Оптимизация действует вместе с существующим SkipSameMoves; настройка
  применяется на ходу. Сама новая DLL загружается после перезапуска игры.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.
