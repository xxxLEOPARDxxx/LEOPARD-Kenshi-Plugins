SKILL LEVEL UP INFO / XP INFO PARITY

Original / Оригинал: XP Info - experience gain notifications, semper1dem
(Steam: Alexander). Thanks to the original author for this feature design.
Благодарим автора оригинала за эту механику отображения опыта.
https://www.nexusmods.com/kenshi/mods/2160
Original sources / Исходники оригинала: the author identifies a source archive
with the original release on that page; no verified public repository was found.
Автор указывает архив исходников оригинального выпуска на этой странице;
подтверждённый отдельный репозиторий не найден.
Our sources / Наши исходники:
https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins/tree/main/SkillLevelUpInfo
License: GPLv3, full terms in LICENSE. Original DLL/assets are not redistributed.
Independent implementation of published behavior; exact defaults, colors and
timing checked through read-only local original binary inspection.
Our existing release / Наша версия: https://www.nexusmods.com/kenshi/mods/2257
Steam: 3829848328. DLL/INI/MCM ID remain SkillLevelUpInfo. No duplicate page.

ENGLISH REFERENCE

Three independent XP Info displays:
- Blue gain text, such as +3% Athletics. Small positive gains accumulate per
  skill until the threshold and minimum collection interval are reached.
- Gold level-up text, such as Athletics 24. Whole levels do not wait for the
  fractional threshold/interval. Several levels at once show the final level.
- Recent training bars below vanilla character-panel statistics, separated by
  a divider. Skill name is on the left, progress and percent in the value column.
  Fewer rows appear when the panel has limited space.

Simultaneous skills merge per character, separately for blue/gold labels.
Four lines are shown with a count of remaining skills. Native ScreenLabel owns
rise, fade and destruction. Gold (1,.84,.35) uses medium text; blue (.62,.86,1)
uses small text, both RS_NORMAL. Accepted model-relative placement is preserved:
in the user's mod list labels start around the torso. Exact head placement
is not claimed.

Options -> MCM -> Skill Level Up Info, all changes immediate:
  Enabled=1                  Master switch.
  FloatingLabels=1           Fractional XP text.
  LevelUpLabels=1            Whole-level text.
  PanelBars=1                Character-panel recent training bars.
  MaxRows=2                  1..2 rows, subject to available space.
  MinPercent=1.00            Accumulated percentage threshold, 0..100.
  AnnounceEverySeconds=1.0   Minimum collection time, .1..60 seconds.
  RecentSeconds=120          Recent training lifetime, 1..3600 seconds.
  SelectedOnly=0             Floating text only for selected player characters.
  LogToGame=0                Copy reached levels to the game message log.
  Debug=0                    Detailed RE_Kenshi_log.txt; last MCM row.

All three display defaults and 2/1%/1s/120s match the original binary. The old
local xp_info.ini (0/1/0, 2%, 2s) was personal configuration, not factory defaults.
Existing personal SkillLevelUpInfo.ini values are retained; new settings use
defaults. INI also works without MCM. No setting needs restart. No new hotkey:
the original had none. MCM replaces its Mods-list Settings button/window.

Panel slots stay stable; a new skill replaces an old slot only when the old
skill has gained no XP for over five seconds. Skills expire after RecentSeconds.
Pending small notifications expire after 120 seconds without further XP.
SelectedOnly limits floating text while everyone retains baselines. Selecting
a hero later never replays hidden gains. LogToGame and Debug are independent.

Requires RE_Kenshi/KenshiLib; MCM optional. Disable original xp_info to avoid
duplicate bars/text. LiveXpBars is a separate tracking window: disable PanelBars
here if a second XP display is unwanted. A UI overhaul replacing the character
panel needs visual verification. XP_Overhaul should load before SkillLevelUpInfo.
This plugin observes resulting base skill gains without changing XP formulas.

All original skill enums are covered: 34 distinct supported base fields on
Steam 1.0.65, including hidden MassCombat. HiveMedic/Vet alias Medic; the unused
Survival dummy is excluded. Equipment/injury modifiers are excluded. A native stat
reference index filters unrelated increaseStat events before formatting or
allocating. Worker hooks update only scalar accumulators. Native Character,
MyGUI and Ogre operations stay on the GUI thread. A 250ms baseline fallback
catches positive changes outside the hook without double counting. Pending
notifications are evaluated every GUI frame independently of that scan, so a
100ms collection interval is honored and levels appear on the next GUI update.
GUI update gaps retain pending XP and recent activity until normal expiry.
Load/new game/settings reset baselines silently. Departed heroes
remove their references. No game save or sidecar is written.

No live label/bone cache; new labels query at most three bones, stable scans
query none. Panel rows are reused; unchanged progress/coordinates are not
rewritten. No global widget-tree scan. At most 512 panel lines are inspected;
selection is bounded to 37 candidate enums/five slots. Errors are always logged.
en_GB/ru_RU/zh_CN translations; release MO only, source PO stays here.

РУССКИЙ СПРАВОЧНИК

Три независимых режима XP Info:
- Синий текст прибавок, например +3% Атлетика. Малые положительные прибавки
  каждого навыка накапливаются до порога и минимального интервала.
- Золотой текст уровней, например Атлетика 24. Целый уровень не ждёт
  дробного порога/интервала. Несколько повышений дают итоговый уровень.
- Полосы недавних тренировок под обычными характеристиками с разделителем.
  Название слева, прогресс и процент в столбце значений. При нехватке места
  показывается меньше строк.

Одновременные навыки объединяются для каждого героя, отдельно синие и золотые.
До четырёх строк и счётчик остальных. Штатная ScreenLabel отвечает за подъём,
затухание и удаление. Золотой (1,.84,.35), средний текст; синий (.62,.86,1),
малый текст, оба RS_NORMAL. Принятая привязка к модели сохранена: в сборке
пользователя надпись начинается примерно от торса. Точное совпадение с головой
не заявляется.

Настройки -> MCM -> Skill Level Up Info, изменения сразу:
  Enabled=1                  Общее включение.
  FloatingLabels=1           Дробные прибавки опыта.
  LevelUpLabels=1            Достигнутые целые уровни.
  PanelBars=1                Недавние тренировки в панели персонажа.
  MaxRows=2                  От 1 до 2 строк, если хватает места.
  MinPercent=1.00            Порог накопления, 0..100 процентов.
  AnnounceEverySeconds=1.0   Минимальное накопление, .1..60 секунд.
  RecentSeconds=120          Время недавней тренировки, 1..3600 секунд.
  SelectedOnly=0             Надписи только над выбранными своими героями.
  LogToGame=0                Копировать уровни в журнал игры.
  Debug=0                    Подробный RE_Kenshi_log.txt; последняя строка.

Три режима включены по умолчанию; 2 строки, 1%, 1с, 120с сверены с исходной
DLL. Старый xp_info.ini сборки (0/1/0, 2%, 2с) был личной настройкой,
а не заводскими значениями. Личный SkillLevelUpInfo.ini сохраняется; новые
параметры используют умолчания. INI работает без MCM. Перезапуск не нужен.
Горячая клавиша не добавлена: у оригинала её не было. MCM заменяет исходную
кнопку Settings в Mods и её окно настроек.

Строки сохраняют порядок. Новый навык вытесняет старый, только если старый
не получал опыт больше пяти секунд. Навык исчезает по RecentSeconds.
Малые накопленные сообщения удаляются после 120с без новых прибавок.
SelectedOnly ограничивает надписи; исходные значения всех своих героев
сохраняются, поздний выбор не воспроизводит скрытые прибавки. LogToGame -
обычные сообщения об уровнях, Debug - отдельная диагностика.

Нужны RE_Kenshi/KenshiLib, MCM необязателен. Исходный xp_info отключить
во избежание дублей. LiveXpBars - отдельное окно; если второй показ опыта
не нужен, выключите PanelBars здесь. Замена панели модом интерфейса требует
визуальной проверки. XP_Overhaul должен грузиться раньше SkillLevelUpInfo.
Расчёт опыта не подменяется, наблюдаются полученные базовые значения.

Полный набор оригинальных навыков: 34 различных базовых поля Steam 1.0.65,
включая скрытый MassCombat. HiveMedic/Vet используют поле медицины, пустышка
Survival исключается. Бонусы экипировки и ранений не учитываются. Индекс полей
своих героев отсекает чужие increaseStat до текста и выделения памяти.
Рабочие потоки меняют только накопители. Character/MyGUI/Ogre используются
в потоке GUI. Запасной опрос раз в 250мс учитывает прибавки вне хука без
дублей. Накопленные сообщения проверяются каждый кадр GUI независимо от
опроса: интервал 100мс соблюдается, целые уровни видны при следующем
обновлении GUI. Перерывы GUI сохраняют прибавки и недавнюю активность
до обычного истечения их срока. Загрузка, новая игра и настройки
молча сбрасывают исходные значения; ссылки ушедших удаляются. Сейвы и sidecar
не пишутся. Кэша живых меток/костей нет, на новую метку максимум три запроса,
без событий запросов нет. Строки переиспользуются, неизменные прогресс
и координаты не пишутся. Нет полного обхода UI: максимум 512 строк панели,
37 кандидатов/пять мест. Ошибки пишутся всегда. en_GB/ru_RU/zh_CN: MO в пакете,
PO в исходниках.

VERSION 2 - UNPUBLISHED / ВЕРСИЯ 2 - НЕ ОПУБЛИКОВАНА
- Restore blue fractional XP text and native recent-training bars alongside
  gold levels. All original controls in MCM; RU/EN/CN, synchronized observations
  and reused UI. Accepted anchor and existing mod identity remain.
- Возвращены синие дробные прибавки и полосы недавних тренировок вместе
  с золотыми уровнями. Исходные настройки в MCM; RU/EN/CN, защищённые события,
  повторное использование UI. Принятая привязка и имя сохранены.

VERSION 1 - RELEASED 30 OCTOBER 2026 / ВЫПУСК 30.10.2026
- Whole levels only, gold native rise/fade, accepted model-relative anchor;
  33 skills, merging, 250ms scan. MCM Enabled/SelectedOnly/LogToGame/Debug.
  Silent baselines after load/new game/settings/long gaps; no save writes;
  RU/EN/CN translations.
- Только целые уровни, золотой цвет и штатные подъём/затухание, принятая
  привязка; 33 навыка, объединение, опрос 250мс. MCM Enabled/SelectedOnly/
  LogToGame/Debug. Молчаливые исходные значения после загрузки/новой игры,
  настроек/перерывов; без записи сейвов; переводы RU/EN/CN.

VERIFICATION / ПРОВЕРКИ
See XP_INFO_PARITY.md and tests/run_tests.py for reproducible checks.
The layout with one or two bars was accepted in-game; vanilla fonts await acceptance.
Разметка с одной или двумя полосами принята в игре; штатный шрифт ожидает проверки.

PANEL LAYOUT CORRECTION / ИСПРАВЛЕНИЕ ПОЛОС

The native value TextBox covers almost the full row and is right-aligned.
XP bars now use the native progress geometry aligned to its right edge,
instead of mistaking its left edge for a separate value column. This fixes
the zero-row result with the actual game layout. Both this panel and the
separate LiveXpBars overlay may be used together when their positions differ.
Removed the redundant MCM sentence from the MCM-page description.

Исправлен расчёт полос под характеристиками: значение штатной строки
выровнено вправо на всю её ширину, а не находится в отдельном узком поле.
Полосы используют штатную геометрию прогресса с выравниванием по правому краю.
Это устраняет нулевое число строк при настоящей разметке игры. Эту панель
и отдельное окно LiveXpBars можно использовать вместе, если они размещены
в разных местах. Из подсказки внутри MCM убрано лишнее упоминание MCM.

STABLE ROW SPACING / УСТОЙЧИВЫЙ ШАГ СТРОК

Native UI layouts / Штатные и заменённые интерфейсы

XP names use the actual runtime widget type, skin and rendered font of the
native attribute labels. The generic progress-line name stays hidden: its
vanilla frame can leave only3px of text after resizing. At most two owned
labels replace it, with mouse/key/picking disabled. Long names remain a single
native text column instead of wrapping into an invisible second line.

Rows preserve native widget/font sizes. When space is tight, only empty top
and inter-row spacing is redistributed inside the real viewport. The original
layout returns when XP is disabled/expired or the context changes; external
native relayout becomes the new baseline. Current measured vanilla/Dark UI
profiles show two XP rows with five attributes and one with six. MaxRows1..2
is an upper bound, and old larger INI values clamp without rewriting the file.

The complete native progress bar and its frame move together2px down beside
the name. Actual23/24px bar heights, scaled skin offsets, palettes and full
25/26px name regions are retained. Percentage text uses the same offset only
when its actual client/text region fits; unfamiliar insets retain a full row.
A1px divider has its own space and does not cross glyphs. Stable frames create
no widgets, repeat no type lookup, and write no geometry. Debug counters for
owned labels/native placement are available to the bounded read-only probe.

Названия опыта используют реальный тип виджета, skin и шрифт штатных атрибутов.
Стандартное имя progress-строки скрывается: его рамка vanilla после сжатия
может оставить всего3px для текста. Вместо него создаются максимум два своих
штатных текстовых виджета без перехвата мыши, клавиш и выбора. Длинные названия
остаются одной колонкой, а не переносятся в невидимую вторую строку.

Размеры штатных виджетов и шрифтов сохранены. При нехватке места перераспределяются
только пустые верхние/межстрочные отступы внутри реальной видимой области.
Исходная геометрия восстанавливается при выключении/истечении XP и смене
контекста; штатное переразмещение игры становится новой исходной геометрией.
Измеренные vanilla/Dark UI показывают две XP-строки при пяти атрибутах и одну
при шести. MaxRows1..2 — верхняя граница; старые большие INI-значения ограничиваются
при чтении без перезаписи личного файла.

Вся штатная полоса вместе с рамкой опущена на2px. Сохранены высота23/24px,
масштабированные элементы skin, цвета и полная область названия25/26px.
Проценты сдвигаются только при достаточной реальной клиентской области;
неизвестные отступы сохраняют полную строку. Разделитель1px занимает отдельное
место без пересечения букв. Стабильный кадр не создаёт виджеты, не повторяет
определение типа и не меняет геометрию. Диагностические счётчики своих надписей
и штатного размещения доступны ограниченному read-only probe.

Technical evidence / Техническая проверка: UI_PROFILE_REPAIR.md,
tests/ui_profiles.json (address-free native captures), tests/test_skin_profiles.py.
Native acceptance of the new repair remains pending the next user launch.
