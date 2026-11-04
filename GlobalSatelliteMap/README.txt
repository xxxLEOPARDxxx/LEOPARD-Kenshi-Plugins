Global Satellite Map (development)

EN
Private development plugin. The minimap keeps its own existing map source.
The global map uses one 4x4 grid of 8192x8192 terrain tiles. Zooming and changing
lighting reuse that grid; supported close views receive streamed finer materials.
Fixed zoom steps preserve native centre/cursor anchors and marker alignment.
The user accepted terrain detail and zoom behaviour on 2026-11-04.

MCM: Satellite view; Sunlight; Sky and ambient light; Water; Torches and lamps;
Existing city models; Maximum map zoom (1..8); Detailed log. Layers default OFF,
sun and ambient default ON. Detailed log is last and OFF by default. Errors are
always reported. These controls do not need a restart.

The existing city option combines distant town geometry, one available cached
loaded-town model, and a new supported loaded-instance layer. The current detailed
subset uses native plaster building materials, Wall4, Gate4 and market materials.
It preserves their native dust/wetness, alpha clipping and translucency. Other
materials, ordinary non-instanced objects, full player construction and shadows
are still unfinished. Do not describe the current subset as all city geometry.
Unloaded towns keep the available distant/cached model; zones are not activated.

Loaded source capture is limited to one nearby town and four candidate checks
on the existing 1Hz catalog update. Per-window draws use copied matrices and
retained COM resources, with no native world scan. Identical captures do not dirty
map windows. Source/world/origin changes invalidate snapshots; disabled or refused
detail falls back to existing LOD/terrain. Model-job epochs reject old completion
after toggling the setting. Accepted model height/coverage feeds lamps and water,
so opaque water cannot paint over a higher accepted building surface.

MCM provides independent cache budgets in1GiB steps: video memory1..8GiB and
RAM1..32GiB. Defaults are1GiB VRAM and2GiB RAM. Both settings have a restart
asterisk: one immutable budget is used for the whole process session. These are
lazy ceilings, not startup reservations; leave memory for Kenshi and other apps.
GPU admission includes the viewer/layers, visible/retained fine windows, source
producer and pending/retired GPU resources. Physical residency scales from665
to6144 windows; visible fine work stays bounded at96 per spacing. Offscreen sharp
tiles remain warm until pressure. Only visible widgets are processed per frame.

A file worker retains validated prepared fine tiles in a separate RAM LRU after
production or disk reads. Cache hits publish independent read copies without IO;
eviction never invalidates Main's current read. Failed optional RAM allocation
keeps the valid disk result. After visible work and600ms stability,8..64 pairs
around the viewport can be prewarmed in RAM without speculative GPU uploads.
Ordinary map close/reopen keeps the worker/cache; world change or disabling the
plugin drains it. The existing disk LRU remains512 files, independent of RAM.

Producer/IO scratch remains separately capped at64MiB, including a conservative
3MiB loaded-capture reserve and cache metadata. The detailed raster/compositor
adds2,884,080 owned GPU bytes when initialized. Native mesh buffers/textures have
separate32MiB/64MiB retained-reference limits. These are resource payload bounds,
not total game/driver RAM or VRAM guarantees. Viewer/layers have40MiB inside the
selected aggregate. GPU retirement debt and failed-fence admission remain counted.
Actual end-to-end town appearance, frame time and long-session growth require
game measurements; offline tests do not prove freedom from every possible leak.

Before full terrain generation allow at least 24GB free; the raw grid is about
23GB. Normal viewing reuses it. Keep matching captures/ready.json/capture.json,
windows.gsi and all 16 GST files. No new full capture is needed for this update.
Exploration fog, biome zones, diplomatic zones and roads remain future options.

Build: GlobalSatelliteMap/build.ps1, native VC10 x64 /GL /LTCG.
Focused proofs: tests/loaded-town; atlas-models/loaded-instances/run_town.py;
atlas-water/run_pipeline_tests.ps1; tests/local_composition_tests.py.
UI: en_GB, ru_RU and zh_CN. Runtime catalogs are MO only. No personal INI or
capture files belong in a public package. This module has no public release yet.

RU
Закрытая разработка спутниковой глобальной карты. Миникарта использует прежний
источник. Основа - одна сетка 4x4 из плиток 8192x8192. При зуме и смене света
она используется повторно; вблизи подгружаются более подробные материалы.
Фиксированные ступени сохраняют привязку центра/курсора и совпадение меток.
04.11.2026 пользователь подтвердил качество рельефа и работу зума.

В MCM: спутниковый вид, солнце, свет неба, вода, факелы и фонари, модели городов,
максимальный зум 1..8 и подробный журнал. Слои по умолчанию выключены, солнце
и фоновый свет включены. Журнал последний, по умолчанию выключен; ошибки видны
всегда. Для этих настроек перезапуск не требуется.

Модели городов объединяют дальние модели, доступную кэшированную модель города
и новый слой поддерживаемых объектов загруженной сцены. Сейчас подробный слой
охватывает штукатурные материалы зданий, Wall4, Gate4 и материалы рынка.
Сохраняются штатные пыль, намокание, отсечение прозрачных участков и просвечивание.
Другие материалы, обычные неинстансные объекты, все постройки игрока и тени ещё
не закончены. Это пока часть городской геометрии. Непрогруженные города сохраняют
доступные дальние модели; ради карты новые зоны не активируются.

Снимок исходной геометрии ограничен одним ближайшим городом и четырьмя кандидатами
при существующем обновлении каталога раз в секунду. Отрисовка участков использует
скопированные матрицы и удерживаемые графические ресурсы, без обхода мира для
каждой плитки. Неизменившийся снимок не запускает лишнюю перерисовку. При смене
мира/сцены/начала координат снимок сбрасывается. Отказ подробного слоя сохраняет
прежние модели и рельеф. Устаревший ответ фоновой проверки после переключения
настройки отбрасывается. Высота принятой геометрии используется светом и водой,
чтобы вода не перекрывала более высокое здание.

В MCM есть два независимых бюджета с шагом1ГиБ: видеопамять1..8ГиБ и оперативная
память1..32ГиБ. По умолчанию1ГиБ VRAM и2ГиБ RAM. Обе настройки помечены звёздочкой:
бюджет фиксируется до конца запуска. Память выделяется постепенно; оставляйте
запас для самой Kenshi и других программ. Вместимость GPU-кэша растёт от665
до6144 плиток. В общий бюджет входят слои карты, подробные плитки, производитель
и ресурсы, ожидающие завершения работы GPU. За кадр обрабатываются только видимые
виджеты, а не все тысячи сохранённых плиток. Видимый план ограничен96 участками
на каждый шаг разрешения.

Рабочий поток сохраняет проверенные готовые подробные плитки в RAM после
производства или чтения с диска. Попадание в кэш обходится без диска; выданная
копия не зависит от вытеснения записи. Неудачная необязательная аллокация RAM
не портит рабочий результат на диске. После видимых плиток и600мс стабильности
прогреваются8..64 пары вокруг текущего вида без загрузки лишних текстур в VRAM.
Обычное закрытие карты сохраняет кэш. Смена мира и отключение плагина завершают
его работу. Дисковый LRU по-прежнему ограничен512 файлами отдельно от RAM.

Рабочие буферы производителя и IO имеют отдельный предел64МиБ, включая метаданные
и резерв3МиБ для снимка города. Новый растеризатор занимает2 884 080 GPU-байт.
Для просмотра и слоёв отведено40МиБ внутри выбранного GPU-бюджета. Удерживаемые
штатные буферы и текстуры имеют отдельные лимиты32/64МиБ. Это границы данных
ресурсов, а не всей памяти игры или видеодрайвера. Реальное изображение, FPS
и рост памяти проверяются отдельно в игре.

Перед полным рендером нужно не менее 24ГБ свободного места; сетка занимает около
23ГБ. Просмотр использует готовую сетку. Для этого обновления полный рендер не
требуется. Туман исследования, биомы, дипломатические зоны и дороги - следующие
необязательные слои. Публичного выпуска этого плагина пока нет.
