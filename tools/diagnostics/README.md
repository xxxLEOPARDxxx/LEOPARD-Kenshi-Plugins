# DynamicFactions read-only shop probe / Проверка запасов магазина

## English

`df_readonly_shop_probe.py` captures physical shop stock and NPC squad money for
an explicitly supplied, independently verified current Kenshi PID. Run only
when inspecting that running game is authorized. It does not discover PIDs,
open trade UI, invoke game functions, inject code, write process memory, save,
or install anything. Its only output is a new local JSON file.

Windows64-bit Python, standard library only:

```powershell
python D:\DEV\Kenshi\ours\tools\diagnostics\df_readonly_shop_probe.py --pid <CURRENT_PID> --output D:\SteamLibrary\steamapps\common\Kenshi\_codex_work\shop-current.json
```

Replace `<CURRENT_PID>` with the separately verified PID. Both arguments are
required; existing evidence files are refused. First the tool obtains metadata
with query-only rights. It checks the executable name, exact installed game path
and audited disk SHA256, the KenshiLib module path/hash, the unpacked reference
image and Steam RVA table hashes. Only then does it open `QUERY_INFORMATION |
VM_READ` (`0x410`). Process creation time/path are checked again to reject PID
reuse. Unknown builds and other processes are refused before any VM read.
The profile is specific to this installation's Steam1.0.65/KenshiLib; updates
require a new native audit, not merely replacing hashes to bypass rejection.

The scan resolves current globals, factions and characters anew. It searches
loaded actors within3000m of the Hub, matching names/templates including
`Трактирщик`, `Бармен`, `Торговец скрытых`, `Barman`, `Bartender`, `Barkeeper`
and merchant terms. It follows the native ShopTrader home's physical furniture
inventories and inspects each parent's **first canonical section**, exactly as
the native stock constructor does. Personal pockets and visual stock copies do
not count as shop stock. `sid1866_in_canonical_stock` counts only
`1866-gamedata.base`; item pointers, quantities, section names/parents and money
are included as local evidence.

Offline ownership proof (Steam RVAs resolved through installed KenshiLib slots):
ShopTrader ctor`0x952a79` calls Character::getOwnerships`0x794b00`. For a
non-player owner (`Faction.isPlayer+0x250 == NULL`) with an existing platoon, it
follows `Character+0x658 -> ActivePlatoon+0x78 -> virtual0x98`.
Platoon::getOwnerships`0x7f1780` returns `this+0x148`; home handle is then+0x38
and money+0x88. The probe verifies the NPC branch, actual platoon relationship
and virtual getter bytes; it also checks the loaded Character ownership body.
The player and missing-platoon fallback branches are unsupported, never guessed.

Limits: VM reads total at most4MiB, each at most256KiB; bounded actors,
containers and section vectors. Simulation is not paused: double checks detect
ordinary purse/ore/identity changes but are not an atomic snapshot. `observed`
with `stable_observation=true` describes current physical stock, not which units
came from a particular sale. A pre-sale sample and a separately saved/reloaded
checkpoint are needed for those claims. House-less backpack shops, unfamiliar
getters, unreadable data and handle remaps are `unknown`/`null`, not zero.
No matching loaded actors gives no stock conclusion; an unloaded Hub on another
scene is expected. Disk hashes are not a full RAM image attestation or a proof
that every other mod leaves trade behavior unchanged. Current read-only runtime
acceptance remains pending after the tool's offline tests.

Offline tests (no process access):

```powershell
python -m unittest discover -s D:\DEV\Kenshi\ours\tools\diagnostics -p test_df_readonly_shop_probe.py -v
```

## Русский

`df_readonly_shop_probe.py` снимает реальные запасы магазина и деньги отряда
NPC по явно указанному, отдельно проверенному актуальному PID Kenshi. Запускать
при разрешённой проверке этой игры. Инструмент не ищет PID, не открывает окно
торговли, не вызывает функции игры, не внедряет код, не пишет в память процесса,
не сохраняет игру и ничего не устанавливает. Записывается только новый
локальный JSON. Команды выше требуют64-битный Python Windows; сторонние пакеты
не нужны. Вместо `<CURRENT_PID>` укажите проверенный текущий номер процесса.
Оба аргумента обязательны; старые файлы доказательств не перезаписываются.

Сначала доступны только метаданные процесса. Проверяются имя и точный путь EXE
в установленной игре, проверенный дисковый SHA256, путь/хеш загруженной
KenshiLib, хеши распакованного эталона и таблицы Steam RVA. Права чтения памяти
`0x410` запрашиваются после проверок. Путь и время создания процесса проверяются
повторно против повторного использования PID. Чужие процессы и неизвестные
сборки отвергаются до чтения памяти. Профиль привязан к текущей установке
Steam1.0.65/KenshiLib: после обновления нужен новый native-аудит, а не подмена
хешей ради обхода отказа.

Каждый запуск заново разрешает globals, фракции и персонажей. Область поиска —
загруженные NPC в3000м от Хаба. Фильтр включает «Трактирщик», «Бармен»,
«Торговец скрытых», Barman/Bartender/Barkeeper и варианты merchant.
Проверяются первые реальные секции инвентарей мебели домашнего здания — те,
которые использует native-конструктор магазина. Личные карманы и визуальные
копии окна торговли в запас магазина не включаются. Поле
`sid1866_in_canonical_stock` считает только `1866-gamedata.base`; JSON содержит
указатели предметов, количества, имена секций/родительские инвентари и деньги.

Offline-проверка ownership: вызов из ShopTrader`0x952a79` ведёт в
Character::getOwnerships`0x794b00`. Для NPC с `owner->isPlayer == NULL` и
существующим отрядом путь — `Character+0x658 -> ActivePlatoon+0x78 -> virtual0x98`.
Platoon::getOwnerships`0x7f1780` возвращает `this+0x148`; домашний handle лежит
по+0x38, деньги по+0x88. Probe проверяет условия ветки, связь с отрядом,
машинный код фактического virtual-getter и загруженной функции Character.
Ветки фракции игрока и отсутствующего отряда не поддерживаются и не угадываются.

Ограничения: максимум4МиБ чтения суммарно, по256КиБ за запрос; обходы ограничены.
Игра не ставится на паузу: повторные проверки денег, руды и identity обнаруживают
обычные изменения, но не дают атомарный снимок. `observed` и
`stable_observation=true` подтверждают текущее физическое наличие, а не
происхождение конкретной руды. Для привязки к продаже нужны снимок до неё и
отдельная проверка сохранённого/перезагруженного checkpoint. Магазины без дома
с запасами в рюкзаках, неизвестные getter, нечитаемые данные и remap handle
помечаются `unknown`/`null`, не нулём. Отсутствие подходящих загруженных NPC не
доказывает пустой магазин; на другой сцене Хаб может быть выгружен. Дисковые
хеши не проверяют весь образ RAM и не доказывают отсутствие влияния других модов.
После offline-тестов новая runtime-приёмка ещё требуется.
