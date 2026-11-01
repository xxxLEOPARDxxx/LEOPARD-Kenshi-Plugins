# Gate Push Guard — v1

**English** · [Русский](#русский)

An independently authored RE_Kenshi plugin by LEOPARD. It observes movement through closed player-owned gates and doors and returns a character outside after a confirmed outside-to-inside crossing through the door leaf.

Thanks to **requizm**, whose [Gate Push Fix (RE_Kenshi)](https://steamcommunity.com/sharedfiles/filedetails/?id=3730136323) inspired this analogue. The original crowd-push protection idea belongs to its author. **Original source:** not supplied with the installed original and no public source link was found on its distribution page. This is a new implementation, not a source fork; it neither uses nor redistributes the original DLL. **Our source:** [LEOPARD-Kenshi-Plugins / GatePushGuard](https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins/tree/main/GatePushGuard).

## Features and operation

- Protects closed, intact, completed **player-owned** gates and doors. The actual door's outside/inside markers determine direction; the closed leaf's own transformed mesh bounds determine the finite aperture.
- Applies to eligible player characters, allies and NPCs. Faction does not make an observed illegal crossing legitimate; opening the door normally always takes precedence.
- Requires two continuous observations of actual movement. Being close to a gate, already being inside, loading a save or closing a gate never suffices to move someone.
- Returns to the last observed outside position, retaining that actual position rather than inventing a fixed push distance or choosing a side by proximity.
- Leaves unconscious/ragdoll characters, carried people, carriers, turret users, bed/cage/furniture occupants and animation-driven movement alone. This prevents the guard from relocating seated guards or interfering with a carry/repair animation. Standing engineers already inside are unaffected because there is no outside-to-inside crossing.
- A long observation gap, discontinuous jump or ambiguous/unsupported leaf geometry reseeds or skips protection. This is a conservative crowd-push mitigation, not a replacement for Kenshi collision or pathfinding.

There are no hotkeys or player windows. Open a gate normally to allow passage. Disable the original **Gate Push Fix** when using this analogue. If `GatePushFix.dll` is loaded, Gate Push Guard suspends corrections and logs the conflict.

## Requirements and settings

Requires RE_Kenshi and its matching KenshiLib. The current native adapter is verified offline against the Steam 1.0.65 compatible executable produced by RE_Kenshi. Unknown/modified native layouts stay inactive and log an error. GOG and other executable profiles have not been verified.

MCM is optional. With MCM installed, use **Options → MCM → Gate Push Guard**. Without MCM, edit the DLL-adjacent `GatePushGuard.ini` and restart the game to reread it. First launch creates missing defaults from `GatePushGuard.defaults.ini`.

| Setting | Default | Effect |
|---|---|---|
| Enabled | On | Enable/disable protection. MCM applies the change immediately and clears history. |
| Detailed log | Off | Recognized geometry and confirmed corrections in `RE_Kenshi_log.txt`. Last setting; errors are always logged. |

Neither setting needs a game restart when applied through MCM, so neither uses `MCM_RESTART`. Interface strings are available in **English, Russian and Simplified Chinese** (`en_GB`, `ru_RU`, `zh_CN`). Runtime packages contain MO files only; source PO catalogs remain here.

## Compatibility and limits

No save records, character orders, gate state, ownership or collision mesh are replaced. Histories are transient and reset on load/new game/world reset, pausing and configuration changes. Removing the plugin removes its protection.

The implementation tracks up to **64 registered player doors**, processes **8 registry slots per frame**, accepts at most **2048 nearby movement events per batch** and **128 observations per door**, and keeps **256 fixed sample slots per door**. Saturation logs a warning; incomplete coverage is preferable to guessing. Very low FPS, excessive movement speed, unusually crowded doors or unsupported compound door meshes can reduce coverage. Teleports are deliberately excluded.

The actual movement adapter passes an **absolute position** to `Character::teleport`; the SDK's `moveBy` parameter name is misleading. This was verified against the native Steam implementation and its Ogre `Vector3::operator=` import. No GOG comment RVAs are substituted for Steam addresses.

## Performance and lifecycle audit

Door lifecycle/update hooks register complete five-field handles. A native movement-update callback records handle/position/time POD values only, after a fixed private spatial-cell index rejects movement away from watched doors. Main-loop work drains a fixed queue and resolves handles afresh. There is no added whole-world/whole-character traversal or native spatial query. Fixed caches store keys, positions and scalar state, with no retained character/building pointers or STL iterators. Event/registry access uses SRW locks; sample processing stays on the native main-loop callback. Every correction rechecks native door/actor eligibility, registry epoch and position immediately before the native call.

The sample arrays occupy a fixed 24,584 bytes per door (1,573,376 bytes for 64 doors). A shared, fixed 1024-actor revision cache prevents one correction being mistaken for a crossing at a neighboring gate. Our own confirmed outside return remains armed against repeated pressure; it has no suppression window. Both native character teleport overloads temporarily suppress external teleports, including short ones. Existing revisions are refreshed on observation and retained until every old history is necessarily stale; periodic rehashing preserves them. The event/revision tables use at most half their hash slots; saturated event batches reject new entries in constant time. Geometry traversal is bounded to 16 leaf entities/128 box corners and performed on a closed-door epoch or native marker change. No custom widgets, worker threads or growing containers are created. A native access fault disables correction for the rest of the process and logs the error.

## Build and validation

Build with the existing VC10 v100 x64 toolchain:

```powershell
& D:/DEV/Kenshi/ours/build.ps1 -SrcDir D:/DEV/Kenshi/ours/GatePushGuard -Name GatePushGuard
& D:/DEV/Kenshi/ours/GatePushGuard/tests/run.ps1
```

`tests/compile_locales.py --output D:/DEV/Kenshi/build/GatePushGuard` generates PO sources and verifies three MO-only runtime catalogs. `tests/audit_native.py --output D:/DEV/Kenshi/build/GatePushGuard-native` reads the actual executable, maps installed manual KenshiLib exports through the actual Steam RVA table, checks imports/native prefixes and writes the native evidence. Its dependency path uses the established local `SkillLevelUpInfo/tests/_tools` forensic libraries.

Current automated evidence: 76 production-core checks pass; 2,560,000 geometry/cache observations produce 1,280,000 expected corrections with no capacity misses in a fixed-memory soak; the bounded event queue accepts/drains 2,048,000 events and rejects 1,000,000 saturated attempts without growth; the DLL builds with `/GL`/`/LTCG`; all 33 KenshiLib imports resolve; all seven source UI strings round-trip in each MO catalog. The pinned executable SHA-256 is `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`.

Independent source and offline native-contract review completed; the curved-path false positive and unbounded native-query assumption were corrected, and the final movement-event/revision implementation has no remaining actionable review finding. **Live acceptance still required:** player gates and building doors in several orientations, engineer repairs, turret/bed/carry/KO cases, repeated gate opening/closing and sustained pressure, legitimate gatekeeper passage, ordinary load/save/new game, vanilla and modded MCM skins, FPS/CPU and prolonged memory measurements. Automated checks are not live gameplay, performance or leak-free acceptance. This local candidate has not been installed or published by its implementation task.

## v1 changes

- New independently authored closed-door crossing guard with continuous observation, finite leaf bounds and conservative exclusions.
- Optional MCM with live enable/disable, standalone defaults and RU/EN/ZH catalogs.
- Bounded lifecycle-safe tracking, native layout checks, always-on error logging and production-core tests.

## License

Copyright © 2026 LEOPARD. GPL-3.0-or-later; see `LICENSE`. KenshiLib/RE_Kenshi and their authors retain their respective rights and licenses. The original Gate Push Fix code was not obtained or incorporated; no claim is made about its unpublished license.

---

# Русский

**Gate Push Guard v1** — самостоятельно написанный плагин RE_Kenshi от LEOPARD. Он наблюдает движение через закрытые ворота и двери игрока и возвращает персонажа наружу после подтверждённого прохода снаружи внутрь через створку.

Благодарим **requizm**: его [Gate Push Fix (RE_Kenshi)](https://steamcommunity.com/sharedfiles/filedetails/?id=3730136323) послужил вдохновением. Идея оригинальной защиты от проталкивания принадлежит автору оригинала. **Исходники оригинала:** не входят в установленную поставку; публичная ссылка на код на странице распространения не найдена. Это самостоятельная реализация, а не форк исходников; оригинальная DLL не используется и не распространяется. **Наши исходники:** [LEOPARD-Kenshi-Plugins / GatePushGuard](https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins/tree/main/GatePushGuard).

## Возможности и управление

- Защищает закрытые, целые ворота и двери **игрока** в завершённых зданиях. Направление берётся из настоящих внутренних/наружных маркеров двери; конечный проём — из преобразованных границ модели самой закрытой створки.
- Работает с подходящими персонажами игрока, союзниками и NPC. Принадлежность к фракции не разрешает проход сквозь закрытую дверь; обычное открытие двери имеет приоритет.
- Требует непрерывных наблюдений настоящего движения. Близость к воротам, нахождение внутри, загрузка сохранения и закрытие ворот сами по себе никого не перемещают.
- Возвращает в последнее реально замеченное положение снаружи, без выдуманного расстояния отбрасывания и выбора стороны по близости.
- Не трогает персонажей без сознания, в ragdoll, переносимых людей и носильщиков, пользователей турелей, кроватей/клеток/мебели и движение под управлением анимации. Это исключает перемещение сидящих стражников и вмешательство в анимацию переноски/ремонта. Стоящий внутри инженер остаётся на месте: прохода снаружи внутрь нет.
- Длинный перерыв наблюдений, резкий скачок позиции и неоднозначная/неподдерживаемая геометрия сбрасывают историю или пропускают защиту. Это осторожное уменьшение проталкивания, а не замена коллизий и поиска пути Kenshi.

Горячих клавиш и отдельных окон нет. Для прохода открывайте ворота обычным способом. При использовании аналога отключите оригинальный **Gate Push Fix**. При загруженной `GatePushFix.dll` Gate Push Guard приостанавливает исправления и сообщает о конфликте в журнале.

## Требования и настройки

Нужны RE_Kenshi и соответствующая KenshiLib. Нативный адаптер проверен офлайн по совместимому Steam 1.0.65 EXE, подготовленному RE_Kenshi. Неизвестные/изменённые нативные структуры оставляют защиту отключённой с записью ошибки. GOG и другие профили EXE не проверены.

MCM необязателен. При установленном MCM: **Настройки → MCM → Gate Push Guard**. Без MCM редактируйте `GatePushGuard.ini` рядом с DLL и перезапустите игру для перечитывания. При первом запуске недостающие значения создаются из `GatePushGuard.defaults.ini`.

| Настройка | По умолчанию | Действие |
|---|---|---|
| Включено | Да | Включает/отключает защиту. MCM применяет изменение сразу со сбросом истории. |
| Подробный журнал | Нет | Распознанная геометрия и исправленные проходы в `RE_Kenshi_log.txt`. Последняя настройка; ошибки записываются всегда. |

При применении через MCM перезапуск не нужен обеим настройкам, поэтому `MCM_RESTART` не используется. Интерфейс поддерживает **русский, английский и упрощённый китайский** (`ru_RU`, `en_GB`, `zh_CN`). В рабочей поставке только MO, редактируемые PO остаются в исходниках.

## Совместимость и ограничения

Плагин не заменяет записи сохранения, приказы, состояние/владельца ворот и коллизионную модель. Временная история сбрасывается при загрузке, новой игре, сбросе мира, паузе и изменении настроек. Удаление плагина убирает защиту.

Пределы: **64 зарегистрированные двери игрока**, **8 слотов реестра за кадр**, до **2048 ближайших событий движения в пакете** и **128 наблюдений на дверь**, **256 фиксированных слотов истории на дверь**. Переполнение записывается в журнал; неполное покрытие безопаснее догадок. Очень низкий FPS, высокая скорость движения, чрезмерная толпа и неподдерживаемая составная модель двери уменьшают покрытие. Телепортации намеренно исключаются.

В нативный `Character::teleport` передаётся **абсолютная позиция**: название аргумента `moveBy` в SDK вводит в заблуждение. Это проверено по Steam-коду и импорту Ogre `Vector3::operator=`. Адреса GOG из комментариев SDK не подставляются вместо Steam.

## Производительность, жизненный цикл и проверка

Хуки дверей сохраняют полные пятикомпонентные идентификаторы. Хук обновления нативного движения записывает только простые значения ключа/позиции/времени; собственный фиксированный индекс ячеек отсекает движение вдали от наблюдаемых дверей. Основной цикл забирает ограниченный пакет и заново разрешает идентификаторы. Дополнительного полного обхода мира/всех персонажей и нативного пространственного запроса нет. Фиксированные кэши хранят ключи, позиции и числа; указатели на персонажей/здания и итераторы не сохраняются. События/реестр защищены SRW lock, обработка наблюдений идёт в нативном основном цикле. Перед перемещением повторно проверяются дверь, персонаж, эпоха реестра и позиция.

Массив наблюдений занимает 24 584 байта на дверь, 1 573 376 байт на 64 двери. Общий фиксированный кэш ревизий на 1024 персонажа не даёт принять наше исправление у одних ворот за проход через соседние. Подтверждённое возвращение наружу остаётся защищено от повторного давления без окна игнорирования. Оба нативных варианта телепорта временно исключают внешние телепортации, включая короткие. Ревизия продлевается при наблюдении и сохраняется, пока все старые истории гарантированно не устареют; периодическое перестроение её не меняет. Таблицы событий и ревизий заполнены не более чем наполовину; насыщенный пакет событий отклоняет новые записи за постоянное время. Геометрия ограничена 16 частями/128 углами и вычисляется при смене эпохи или нативных маркеров двери. Собственных виджетов, рабочих потоков и растущих контейнеров нет. Ошибка нативного доступа отключает исправления до завершения процесса и записывается в журнал.

Сборка и команды проверки приведены в английском разделе. Успешно: 76 проверок production-ядра; 2 560 000 наблюдений в проверке фиксированного кэша с 1 280 000 ожидаемых исправлений без переполнения; очередь принимает/возвращает 2 048 000 событий и отклоняет 1 000 000 попыток переполнения без роста; VC10 `/GL`/`/LTCG`; все 33 импорта KenshiLib; все семь строк в каждой из трёх MO-локалей. SHA-256 проверенного EXE: `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`.

Независимое ревью исходников и офлайн-контрактов нативного кода завершено. Исправлены ложное срабатывание на обходящей дверь траектории и ошибочное предположение об ограниченном нативном запросе; у итоговой реализации событий движения/ревизий замечаний не осталось. **Ещё нужны игровые проверки:** ворота и двери разных направлений, инженеры, турели, кровати, переноска, KO, циклы открытия/закрытия и постоянное давление толпы, законный проход привратника, save/load/новая игра; MCM на ванильном и модифицированном скинах; реальные замеры FPS/CPU и длительного роста памяти. Автотесты не доказывают игровую работоспособность, производительность или отсутствие любых утечек. В рамках реализации этот локальный кандидат не устанавливался и не публиковался.

## Изменения v1

- Самостоятельно написанная защита от прохода через закрытую дверь: непрерывное наблюдение, конечные границы створки, осторожные исключения.
- Необязательный MCM с немедленным включением/отключением, отдельные настройки и каталоги RU/EN/ZH.
- Ограниченные кэши и безопасный жизненный цикл, проверка нативной структуры, обязательный журнал ошибок, тесты production-ядра.

## Лицензия

Copyright © 2026 LEOPARD. GPL-3.0-or-later, см. `LICENSE`. Авторы KenshiLib/RE_Kenshi сохраняют права и лицензии своих проектов. Код оригинального Gate Push Fix не получен и не использован; его неопубликованная лицензия не выдумывается.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.
