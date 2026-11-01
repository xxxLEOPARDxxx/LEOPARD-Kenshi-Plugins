BEST MINIMAP (QoL)

Миникарта в углу экрана. Требуется RE_Kenshi.


ЧТО ПОКАЗЫВАЕТ

  * Картинку карты игры - ту же, что на большой карте. Моды на карту
    (другая картинка, другая раскраска) подхватываются сами.
  * Персонажей рядом точками: ваш отряд, союзников, нейтралов, врагов,
    животных. Выделенные - крупнее и с белой обводкой. Мёртвые - по желанию,
    блёкло.
  * Метки мода Map-markers (из текущего сохранения) и цели WantedMap.
    Метки за пределами обзора прижимаются к краю и показывают направление.
  * Стрелку в центре: куда смотрит камера. Цвет и размер настраиваются отдельно;
    размер задаётся числом от 4 до 64 пикселей и не зависит от меток.
  * Границу камеры: пунктир или сплошная линия, с цветом и толщиной 1-8 пкс.
    Дальность 0 берётся из проверенного ограничения игры, а не из старого
    максимального расстояния. Высота и рельеф могут уменьшать реальную
    досягаемость относительно проекции на карту. Неизвестные изменения
    камеры требуют ручного расстояния.
  * Тревогу: если персонажа вашей фракции атакуют - пульсирующее кольцо
    на месте атаки (за краем обзора - у края), мигающая рамка и строка в
    сообщениях игры.
  * Поселения и места большой карты (только открытые): значок как на
    большой карте игры (из её интерфейса и модов на интерфейс) или ромб
    (цвет - по отношениям фракции), название, тонкий круг радиуса.
    Подсказка при наведении: фракция и отношения.
  * Носики у точек - куда смотрит персонаж; названия отрядов; награду за
    голову (золотое кольцо и сумма).
  * ПКМ по миникарте - выделенные идут в эту точку (зелёный флажок).
  * Двойной щелчок по миникарте - камера переносится в эту точку, если игра пускает
    её так далеко от ваших персонажей (до линии предела камеры). Камера,
    следившая за персонажем, перестаёт за ним следить.
  * Тонкую линию вокруг ваших персонажей: докуда можно отвести камеру.
    Предел плагин узнаёт сам - один раз отведите камеру до упора.

Цвета точек по умолчанию - «auto»: тот же цвет, которым игра отмечает
персонажа на большой карте. Каждой категории можно задать свой.


УПРАВЛЕНИЕ

  CTRL+M            показать / скрыть
  =  /  -           приблизить / отдалить (удержание - повтор)
  CTRL+=  /  CTRL+- больше / меньше сама миникарта
  колесо мыши       над миникартой - масштаб (до всей карты)
  правая кнопка     над миникартой - выделенные идут туда
  двойной щелчок    по миникарте - камера туда (в пределах дальности)
  CTRL+колесо       над миникартой - размер
  левая кнопка      тащить миникарту (если не закреплена)

Клавиши, форма (круг, квадрат, прямоугольник), рамка, поворот за камерой,
кого показывать и цвета - во вкладке «Моды» окна «Настройки» игры (нужен
ModConfigMenu) или в BestMinimap.ini. Место, размер и масштаб запоминаются.

Пока открыта большая карта или спрятан интерфейс игры (F7), миникарта
спрятана.


WANTEDMAP И MAP-MARKERS

Оба мода необязательны: нет мода - нет и его меток, остальное работает.

Цели WantedMap ищет, пока открыта большая карта (чтобы не тратить на это
каждый кадр). Поэтому на миникарте они - «на момент последнего взгляда на
карту»: открыл карту - метки освежились. Нужна версия WantedMap с
WantedMap_GetMarkers (из этого же набора).


ЕСЛИ ЧТО-ТО НЕ ТАК

  * Нет фона, только точки на тёмном - картинку карты прочитать не вышло;
    в RE_Kenshi_log.txt строка «BestMinimap: could not read the map picture».
  * Синий песок вместо жёлтого - SwapRedBlue=1.
  * Точки сдвинуты относительно карты - пришлите строку «map calibrated» из
    RE_Kenshi_log.txt (Debug=1 не нужен, она пишется всегда).
  * Игра тормозит с миникартой - Debug=1 («Подробный журнал»): раз в 5 с
    в RE_Kenshi_log.txt строка «BestMinimap: render ... ms» - сколько
    времени уходит на каждую часть. Пришлите её.


КАК УСТРОЕНО

Миникарта - своя текстура MyGUI, которую плагин заполняет сам: кусок
картинки карты вокруг центра, с поворотом, маской формы, рамкой и точками.
Никаких шейдеров - работает с любым рендером игры. Картинка карты один раз
читается в память (уменьшенная до MaxSourceSize), дальше - 20 раз в секунду
(UpdateFps) перерисовка. Пиксели считает отдельный поток; основной поток
игры только собирает, что и где рисовать, и пишет готовую картинку в
текстуру - так миникарта не дёргает кадры.

Значки поселений берутся из того же места, откуда их берёт большая карта
игры: атлас интерфейса один раз читается в память, значок вырезается и
уменьшается. Пока большая карта ни разу не построила свои метки (бывает до
первого её открытия), вместо значков - ромбы.

Где мировая точка на картинке, плагин спрашивает у самой игры
(MapScreen::worldToMapCoords) - поэтому и карты модов, где мир другого
размера, ложатся правильно.


ENGLISH GUIDE

Best Minimap uses the game's map image and follows map replacements. Requires
RE_Kenshi; MCM provides in-game settings. UI text supports English, Russian and
Chinese. The same minimap works with vanilla and modified interface themes.

Features: nearby characters by relation, selected-character highlights, discovered
towns, tooltips, attack alerts, Map-markers and WantedMap integration. A camera
arrow shows direction; its colour and size are independent settings. Size is an absolute 4-64 pixel value that can be reduced independently of markers. The camera boundary supports dotted and solid
lines, colour and a 1-8 pixel width. Range 0 reads the verified game restriction;
unknown camera modifications require a manual distance. The planar map outline
can slightly overestimate actual reach when camera height or terrain differs.

Controls: drag to move an unlocked minimap; wheel zooms, Ctrl+wheel resizes.
Right-click sends selected characters to a point, double left-click moves the
camera within reach. The assigned keys toggle visibility, zoom and resize.
MCM settings or BestMinimap.ini control appearance, filters, alerts and colours.
Camera arrow size, line width and manual range apply without a game restart.
Detailed logging is the last setting and defaults to off.

Prepared update 2 / Подготовленный выпуск 2
EN: Added independent camera arrow size and adjustable, clearer boundary width.
Auto range uses the verified game limit instead of a persisted maximum observed
distance. Range checks include the complete player-character list used by the
game. Existing personal settings are preserved; obsolete CameraRangeLearned is
ignored. UI translations remain English, Russian and Chinese.
RU: Добавлены отдельный размер стрелки камеры и настройка толщины более заметной
границы. Автодальность берётся из проверенного предела игры вместо сохранённого
максимального замера. Проверки учитывают полный список персонажей игрока, как
сама игра. Личные настройки сохраняются; CameraRangeLearned игнорируется.
Интерфейс переведён на английский, русский и китайский.

Implementation / Реализация
Auto range verifies a 359-byte Native restrictPosition profile resolved through
KenshiLib, masking only 12 audited RIP-relative operands, then reads the actual
radius and squared threshold within readable main-image pages. Unknown opcodes,
detours or inconsistent constants disable automatic range with a diagnostic;
manual override remains available. Steam 1.0.65 was verified from the actual
unpacked executable. GOG has not been tested at runtime; its RVA alone does not
establish support. No code patching or calls to restrictPosition are performed.
The checked limit is cached once; the old four-per-second observation scan and
periodic learned-range INI writes were removed. Worker jobs own line-width values;
arrow size is copied into draw commands. Dotted and solid circles are capped at
20,000 samples each. Runtime FPS and long-run memory still require game testing.

Legacy CameraArrowSize=0 is migrated once to the actual previous pixel size,
so the displayed value can be reduced. Russian in-game text uses ASCII hyphens;
typographic dashes are unsupported by the game font.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.

Prepared release 3 / Подготовленный выпуск 3 (2026-11-02)

English

- Optional language diagnostics. Language and translation-catalogue summaries follow this plugin's Detailed log setting, which is off by default.
- Useful errors remain visible. Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.
- Clean update package. English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.

Русский

- Необязательная диагностика языка. Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.
- Ошибки остаются видимыми. Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.
- Чистый пакет обновления. Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.
