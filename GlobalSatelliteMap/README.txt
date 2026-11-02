Global Satellite Map (development)

EN
Private development plugin, not a public release. Best Minimap keeps its existing
map source. The global-map overlay uses one verified4x4 grid of8192x8192 tiles.
Terrain capture is reused: zooming, sunlight and lamps do not recapture the world.

MCM: Satellite view, Sunlight, Sky and ambient light, Water, Torches and lamps,
Existing city models, Detailed log. Main view and optional layers default OFF;
sun/ambient default ON. Detailed log is last and defaults OFF. Errors always log.
No restart is required for these settings. Maximum map zoom is an optional1..8
multiplier, default1, active with satellite view;2shows native terrain pixels at
1:1 in the tested interface. Higher values enlarge the captured pixels. Native input anchors and marker thresholds are retained; reducing the limit recentres/clamps
through the native zoom method. The original maximum is restored on disable.

Current renderer: geographic terrain materials, current native sun/ambient/HDR
exposure, optional distant water, native point/spot lights in loaded areas.
A paused unchanged light snapshot does not repeatedly dirty all windows. View
streaming keeps16coarse fallbacks plus up to128detail windows. At most4windows
are shaded per200ms map tick. The original map remains available on refusal.

City-model development subset: already registered static DistantTown proxies,
up to64towns;2source admissions per200ms and coherent1Hz town catalog. No zone
activation, original Entity clone, native model unload or per-window GPU-state
creation. Packet COM references are retired on OFF/load/world/scene transitions.
Each refresh starts from pristine terrain planes, so removed models leave no
accumulated silhouettes. One owned cached-town LOD now covers the nearest
eligible loaded town without a proxy, using its actual existing cache file.
Full loaded-building materials, player construction and shadows remain unfinished;
the separate loaded-windmill helper is not an active feature.

Owned texture/constant payload is capped at192MiB (driver/native overhead is
separate). With terrain/water/local lights/model working surfaces it is196086608
bytes. Borrowed native model buffers have a separate32MiB steady-state cap;
diffuse resources64MiB. A single incoming packet can temporarily retain up to
another32MiB of existing native buffers before global admission/refusal. These
are resource payload bounds, not an absolute VRAM or process-memory guarantee.
Native model imports and long-run FPS/memory still require game measurements.

Before full terrain generation allow at least24GB free: the current raw grid is
about23GB. Only one current grid is needed. Existing diagnostic captures are not
release content. The display requires matching captures/ready.json/capture.json,
windows.gsi and16GSTtiles; changed game/mod sources refuse the old data.

Build: GlobalSatelliteMap/build.ps1 (VC10 x64 /GL and /LTCG).
Checks: tests/run_viewer_tests.ps1 and tests/run_local_composition_tests.ps1;
atlas-models/run_tests.ps1, decode/run_tests.ps1 and surface/run_tests.ps1.
UI catalogs are en_GB/ru_RU/zh_CN; runtime packages use MO only.

Extended zoom uses a proportional button/wheel step on close views, preserving
native centre/cursor anchors;1 restores normal input. One private cached-town
LOD is read in the existing bounded worker and imported on the render thread,
without activating zones. It may miss recent town changes. Native whole-building
materials remain separate work. Normal explicit cleanup is required while the
renderer is alive; a passive exit owner retains at most one bounded graph until
OS teardown if cleanup is omitted. Runtime DLL unloading is unsupported.

RU
Закрытая разработка спутникового слоя глобальной карты, не публичный выпуск.
Миникарта сохраняет прежний источник карты. Используется одна проверенная сетка
4x4 из16плиток8192x8192. При масштабе и смене освещения мир повторно не снимается.

В MCM: спутниковый вид, солнце, фоновое освещение, вода, факелы и фонари,
существующие модели городов, подробный журнал. Основной слой и дополнительные
слои по умолчанию выключены, солнце и фоновый свет включены. Подробный журнал
последний и выключен; ошибки видны всегда. Перезапуск для этих настроек не нужен.
Добавлен множитель максимального зума1..8, по умолчанию1, для спутникового вида.
В проверяемом интерфейсе2позволяет показывать пиксели рельефа1:1. Большие значения
увеличивают сами пиксели снимка. Штатные привязки ввода и метки сохраняются; при уменьшении
предела масштаб и центр обновляются штатным способом. Отключение возвращает
прежний предел.

Реализованы географические материалы рельефа, текущее солнце и фоновый свет,
экспозиция, необязательная вода и реальные источники света загруженных областей.
При неизменном освещении на паузе участки не перерисовываются без причины.
Сохраняются16грубых подложек и до128детальных участков; не более4перерисовок за
200мс при открытой карте. При отказе слоя остаётся обычная карта.

Экспериментальные модели городов используют уже зарегистрированные статичные
дальние модели: до64городов, две проверки источников за200мс, список раз в секунду.
Зоны не активируются, оригинальные модели не клонируются и не выгружаются.
Ресурсы повторно используются, очищаются при отключении и смене мира/сцены.
Перед каждой отрисовкой моделей копируется чистый рельеф, чтобы удалённые здания
не оставляли силуэтов. Полные материалы загруженных зданий, стройка игрока и тени ещё не подключены.
Одна собственная готовая LOD-модель поддерживает ближайший загруженный город без
дальнего proxy, если его файл уже существует. Отдельный модуль мельницы не активен.

Собственные текстуры и константы ограничены192МиБ; все текущие слои требуют
196086608байт. Удерживаемые родные буферы моделей ограничены32МиБ, текстуры64МиБ;
временно принимаемый пакет может удерживать ещё до32МиБ существующих буферов.
Это границы полезных ресурсов, а не оценка полной памяти драйвера и процесса.
Игровые замеры FPS и длительного роста памяти проверяются отдельно.

Перед полной съёмкой нужно хотя бы24ГБ свободного места: сырая сетка занимает
около23ГБ. Достаточно одной текущей сетки. Диагностические захваты не входят в
релиз. При несовпадении источников игры/сборки старая сетка не используется.
Три языка интерфейса: русский, английский, китайский; в игре нужны только MO.

Управление увеличенным зумом: при множителе больше1 шаг кнопок растёт с
масштабом, колесо ускоряется на близком виде. Сохраняются штатные привязки:
кнопки относительно центра окна, колесо относительно курсора. Значение1
возвращает штатный шаг. Это меняет управление, а не разрешение рельефа.

Для ближайшего загруженного города без действующей дальней модели можно
использовать один существующий cached LOD. Файл читается в прежнем фоновом
потоке, импортируется в отдельный незарегистрированный Mesh и рисуется тем же
слоем. Например, это позволяет показать Хаб без активации дополнительных зон.
Это готовая упрощённая модель: недавние изменения города могут в ней отсутствовать.
Полные материалы и геометрия загруженных зданий пока не переносятся.
При отключении/смене мира собственная модель явно очищается в игровом потоке.
Если явная очистка при завершении процесса не произошла, один ограниченный граф
владения сохраняется до завершения ОС без поздних вызовов Ogre/COM. Выгрузка DLL
на лету не поддерживается; отсутствие любых утечек этой мерой не утверждается.
