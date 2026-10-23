# Dynamic Factions — этап 2а «Работа и добыча» Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Люди фракции сами добывают руду у ближайших к Хабу жил и продают её торговцам Хаба: деньги фракции растут от настоящей работы, а не от заглушки.

**Architecture:** Как в этапе 1: решение «кто что делает» — чистое ядро (`core/DfWork`, тесты без игры), исполнение — адаптер (`game/DfMine` — жилы и копание, `game/DfSell` — поход к торговцу и продажа), всё раз в секунду из `ForgottenGUI::update`, каждое обращение к игре под SEH. Работа идёт только у загруженного Хаба; вдали — ничего (абстрактная симуляция — позже). Дом и склад — этап 2б, на выручку этого этапа.

**Tech Stack:** VC10, KenshiLib 0.2.1, RE_Kenshi, MyGUI, Python 3 (`tools/df_make_mod.py`).

**Spec:** `docs/superpowers/specs/2026-10-22-dynamic-factions-design.md` (этап 2 и раздел «Пожелания по проверке этапа 1»). Порядок этапа 2 — от пользователя 23.10: «сначала работа и добыча, потом на вырученные деньги дом и склад и остальное».

## Global Constraints

- Сборка: `D:\DEV\Kenshi\build.ps1 -SrcDir D:\DEV\Kenshi\ours\DynamicFactions -Name DynamicFactions -SubDirs core,game,ui`; тесты ядра: `powershell -File DynamicFactions\tests\build_tests.ps1` — `N checks, 0 failed`.
- В DLL нет кириллицы; тексты — `.po` en/ru/zh (`dynamic_factions`), после копирования — `po2mo.py`; в игровых строках только ASCII и кириллица (без «» — …).
- Объекты игры — только главный поток; каждое обращение — под SEH (`__try` в функции без объектов с деструкторами); оригиналы игры зовём всегда; приказы — только своим людям (исключение этапа 1 — «подожди» рекруту).
- Честная симуляция (пользователь, 23.10): всё, что игрок видит, происходит на виду — копают у жилы, несут, продают торговцу с репликами; денег «из воздуха» нет.
- Бегом на всех задачах (`force speed` = RUN у отряда уже есть).
- Жилы — любые, включая модовые (золото, уран, серебро... — пользователь 23.10): ищем по функции постройки `BF_MINE_NATURAL`, а не по именам; что добывается и почём — из данных игры (предмет жилы и его `value`), ничего не зашито.
- Сроки найма/работы — по часам, которые стоят на паузе (`DfHireClock`, этап 1).
- MyGUI: текст и координаты ставим только при изменении.
- README в ours и в моде — после каждой правки; память по проекту (`kenshi-dynamic-factions.md`) — по ходу.

## Review Focus

1. Модовая жила с незнакомым предметом (золото, уран...) — находится и копается так же, ценность берётся из данных её предмета; предмет без цены (`value` 0) — жила пропускается, а не ломает выбор.
2. Жил рядом нет / все заняты чужими — люди не стоят столбом и не бегут через полкарты: радиус поиска ограничен, свободное место у жилы проверяется, иначе — ждут у Хаба.
3. Торговец без денег / торговцев нет — продажа частичная или откладывается; руда не исчезает, деньги не появляются сверх того, что заплатил торговец.
4. Нападение по дороге (звери, бандиты) — работа прерывается, приказ снимается, своя реакция пакета ИИ (защита) работает; после боя — снова работа.
5. Сохранение посреди похода с рудой — после загрузки руда в инвентаре, задание начинается заново без двойной продажи.
(Шестой риск — посланник найма и рабочий в одном лице — закрыт тестом `test_work_hiring_kept` в Task 3.)

---

## Файлы

```
ours/DynamicFactions/
  core/DfWork.h/.cpp        роли людей: копать / продавать / нанимать / ждать (без игры)
  game/DfMine.h/.cpp        жилы у города, приказ копать, руда в инвентаре
  game/DfSell.h/.cpp        торговец города, поход, реплики, продажа
  DynamicFactions.cpp       раз в секунду: роли -> приказы; заглушка дохода выключена
  ui/DfWindow.cpp           строки «Работа: копают N, несут N; руды N; выручка за день»
  tests/core_tests.cpp      тесты DfWork + отложенные из ревью
tools/df_make_mod.py        (если нужно) правки пакета ИИ
```

---

### Task 1: Отложенные мелочи из ревью этапа 1

**Files:** Modify `DynamicFactions.cpp`, `game/DfHire.cpp/.h`, `game/DfRecruit.cpp`, `game/DfGame.cpp`, `core/DfBrain.*`; Test `tests/core_tests.cpp`

Список — из памяти проекта («Отложенные мелочи»), каждая отдельной правкой:
- `g_own` строится сразу в `ReadState`/при основании, а не после 10 с; присваивается только при изменении.
- `UpdateWindow` — только при `g_ready`.
- `g_skipUntil` — по `hand`, чистится при смене игры.
- `DfHireStart(DfHireTask&, Faction*, TownBase*, int maxPrice, Character* recruit, int price)` — найденный рекрут передаётся, второго поиска нет.
- `HireBelow` на лимите узлов — кэшируется как «найм есть» (блок).
- `Found`: деньги начисляются сразу после `createRandomSquad`, до необязательных шагов.
- `Join`: сначала списать деньги, потом `setFaction`.
- Убрать `stuckNear/kNearDistance/closest/closerMs`, `DfRules::recruitCost`.

- [ ] **Step 1:** Тест `test_brain_negative_money_no_spend`: `Snap(4, -500, 11.0, true)`, `recruitPrice = 0` → нет `DF_RECRUIT`, нет `DF_RESEARCH`, `DF_INCOME` есть.
- [ ] **Step 2:** Run tests — Expected: тест проходит сразу или падает; если проходит — это проверка существующего поведения, оставить (отмечено в ревью как пробел покрытия).
- [ ] **Step 3:** Правки адаптера по списку. Build — Expected `OK`, тесты `0 failed`.
- [ ] **Step 4:** Commit `DynamicFactions: phase-1 review leftovers`.

### Task 2: Разведка в игре — жилы, копание, продажа (только при Debug=1)

**Files:** Create `game/DfMine.h/.cpp` (поиск жил), `game/DfSell.h/.cpp` (поиск торговцев); Modify `DynamicFactions.cpp` (журнал раз в игровой час при Debug)

**Interfaces — Produces:**
```cpp
struct DfNode { hand building; Ogre::Vector3 pos; std::string item; int itemValue; float distance; int freeSlots; };
// любые жилы, в т.ч. модовые: ZoneManager::getObjects + getSpecialFunction()==BF_MINE_NATURAL;
// item/itemValue - предмет, который даёт жила, и его value из GameData (как его найти у постройки -
// Ruling Task 2); itemValue <= 0 - жила не берётся
void DfFindNodes(TownBase* town, float radius, std::vector<DfNode>& out);
struct DfTrader { hand who; int money; };
void DfFindTraders(TownBase* town, std::vector<DfTrader>& out);   // живые с флагом шаблона «is trader» в городе (как RecruitMarkers), деньги - Ownerships его взвода
```
- [ ] **Step 1:** В журнал (Debug) раз в игровой час: жилы в радиусе 600 м от центра Хаба — что дают (предмет, его value), расстояние, свободные места; торговцы Хаба — имя, деньги. В сборке пользователя есть модовые жилы (золото, уран, серебро) — они должны попасть в список.
- [ ] **Step 2:** Отладочная проба (Debug, один раз за игру): ближайшему свободному члену фракции — `OrdersReceiver::addOrder(OPERATE_MACHINERY, node.building, node.pos, true, false)`; в журнал каждые 5 с: цель (goal), расстояние до жилы, число руды в инвентаре (`Inventory::countItems(GameData*)`).
- [ ] **Step 3:** В игре (пользователь): новая игра, 2–3 игровых часа у Хаба. Записать в ledger правилами (`Ruling:`): есть ли жилы и какие; копает ли NPC по приказу (или нужен другой TaskType / свой путь, как `setDestination` в этапе 1); куда идёт руда (инвентарь / под ноги / в жилу); у кого из торговцев есть деньги; какую цену продажи брать — функция игры для цены продажи (искать в `Item.h`/`Inventory.h` по `getValue`/`price`) или `value × 0.5`, если функции нет.
- [ ] **Step 4:** Commit `DynamicFactions: phase 2a recon - nodes, mining probe, traders (debug only)`.

### Поправки по разведке (Task 2, 23.10)

- Жилы видны игре (и моду), только пока загружен их участок карты (рядом камера или люди): `buildingGrid` даёт BF_MINE_NATURAL (27) с «is foliage»; у Хаба запомнено 13 жил (железо, медь, олово GenMod) в 1,5–3 км.
- Копание: `OrdersReceiver::addOrder(OPERATE_MACHINERY, жила)` — член фракции идёт и копает (goal 0x57), стоит в ~35 м (use range). Камера ушла — жила выгрузилась, работа «моргает».
- Руда (пользователь): копится в хранилище самой жилы, предел 50; на пределе копание стоит, пока руду не заберут; положить руду обратно в жилу нельзя.
- Отсюда роли: копатели (MINE) и носильщик (HAUL): забирает руду из жилы в инвентарь (столько, сколько влезет), несёт торговцу, продаёт.
- Невидимый слой: жила не загружена — копание условное (свой счётчик руды жилы, темп — замер в Task 4 при загруженной), носильщик ходит на виду, а груз — условный, продажа у торговца (он в Хабе, загружен) — по-настоящему деньгами торговца.
- Пожелания пользователя на 2б: покупать рюкзаки (носить больше), стройматериалы и железные листы для построек, если фракция сама их не делает.

### Task 3: Ядро — кто что делает

**Files:** Create `core/DfWork.h/.cpp`; Test `tests/core_tests.cpp`

**Interfaces — Produces:**
```cpp
enum DfRole { DF_ROLE_IDLE, DF_ROLE_MINE, DF_ROLE_HAUL, DF_ROLE_HIRE };
struct DfMember { int id; bool able; bool hiring; int carried; int node; DfRole role; };   // node - индекс жилы или -1
struct DfNodeState { int value; float distance; int stored; int cap; int maxMiners; };     // cap = 50 у жилы
const int kDfHaulAt = 30;            // в жиле столько - зовём носильщика
void DfAssignRoles(std::vector<DfMember>& members, const std::vector<DfNodeState>& nodes,
                   bool homeLoaded, bool traderHasMoney);                                  // пишет role и node

// Выгода жилы: цена добычи с поправкой на дорогу - value / (1 + distance / 100).
// Индексы жил по убыванию выгоды; value <= 0 - не попадает.
struct DfNodeOffer { int value; float distance; int freeSlots; };
void DfRankNodes(const std::vector<DfNodeOffer>& nodes, std::vector<int>& order);
```
Правила: дом не загружен — все IDLE (HIRE не трогаем); `hiring` — HIRE; не `able` — IDLE, node −1; HAUL с грузом (`carried` > 0) — остаётся HAUL (донести); в жиле `stored ≥ kDfHaulAt` и у неё нет носильщика, торговцу есть чем платить — один из её копателей (иначе свободный) становится HAUL этой жилы; MINE — у жил по `DfRankNodes`, пока `stored < cap` и копателей меньше `maxMiners`; копатель полной жилы (`stored ≥ cap`) переходит к следующей жиле с местом, нет такой — IDLE; остальные IDLE. Роль держится, пока не требует состояние (копатель не прыгает между жилами без причины).

- [ ] **Step 1:** Тесты: `test_rank_value_beats_distance` (золото value 800 в 400 м выше железа 150 в 100 м: 800/5 = 160 > 150/2 = 75); `test_rank_near_cheap_beats_far_rich` (железо 150 в 10 м = 136 выше золота 800 в 1500 м = 50); `test_rank_skips_no_value` (value 0 — нет в порядке); `test_work_home_unloaded_all_idle`; `test_work_hiring_kept`; `test_work_miners_limit` (3 человека, жила maxMiners 2 → 2 MINE, 1 IDLE); `test_work_haul_when_stored` (stored 30 → ровно один HAUL этой жилы); `test_work_no_trader_money_no_haul`; `test_work_hauler_keeps_load` (HAUL, carried 5 → HAUL); `test_work_hauler_done_back_to_mine` (HAUL, carried 0, stored 0 → MINE); `test_work_full_node_moves_miner` (stored = cap → копатель к другой жиле с местом).
- [ ] **Step 2:** Run — Expected: не компилируется (нет `DfWork.h`).
- [ ] **Step 3:** Реализовать `DfAssignRoles`.
- [ ] **Step 4:** Run — Expected `0 failed`. Commit `DynamicFactions: work roles core`.

### Task 4: Копание (по-настоящему и условно)

**Files:** Modify `game/DfMine.h/.cpp`, `DynamicFactions.cpp`

**Interfaces — Produces:**
```cpp
bool DfMineStep(Character* c, const DfNode& node);   // жила загружена: OPERATE_MACHINERY, повтор только если цель не 0x57; нет - идти к точке жилы
int DfCountInNode(const hand& node, GameData* item);  // руда в хранилище жилы (есть с Task 2)
// Запомненные жилы - в файле состояния (позиция, предмет по stringID, условный запас), по сейвам.
```
- [ ] **Step 1:** Раз в секунду (часы найма — стоят на паузе): роли из `DfAssignRoles`, MINE → `DfMineStep` к закреплённой жиле (жила закрепляется за человеком, пока он MINE). Новому копателю — первая по `DfRankNodes` жила со свободным местом.
- [ ] **Step 2:** Бой (`isInCombatMode`) — приказ снять, роль IDLE до конца боя (Review Focus 3).
- [ ] **Step 3:** Темп: при загруженной жиле журнал раз в минуту — `stored` и число копателей; по замеру в ledger — `kDfOrePerMinerHour` для условного копания (жила не загружена, копатель у её точки: запас растёт этим темпом до 50).
- [ ] **Step 4:** Build — `OK`; в игре: люди бегут к жилам, копают, запас жилы растёт и при камере, и без неё (журнал Debug + окно). Commit `DynamicFactions: mining`.

### Task 5: Носильщик и продажа торговцу

**Files:** Modify `game/DfSell.h/.cpp`, `DynamicFactions.cpp`, `ui/DfWindow.cpp`, `.po`

**Interfaces — Produces:**
```cpp
struct DfSellTask { hand seller; hand trader; int phase; unsigned long phaseMs; int line; };   // GO -> TALK -> DEAL
DfHireResult DfSellStep(DfSellTask& t, Faction* f, int& earned);   // DEAL: руда из инвентаря продавца - торговцу, деньги торговца (сколько есть) - фракции
```
- [ ] **Step 0:** Носильщик бегом к жиле; загружена — переносит руду из хранилища жилы в свой инвентарь (`Inventory::removeItemDontDestroy_returnsItem` → `addItem`, сколько влезет); не загружена — забирает условный запас (сколько унёс бы по весу). Потом к торговцу.
- [ ] **Step 1:** GO: бегом к торговцу (как посланник найма: `setDestination`, повтор при смене цели); TALK с 10 м: две реплики (`Tr("Got ore. Buying?")` / `Tr("Show me.")`), по 3 с; DEAL: за каждую единицу — цена по Ruling Task 2 от value её предмета (железо, золото, уран — каждое по своей цене); торговцу не хватает денег — продаётся сколько оплачено, остаток руды остаётся у продавца (Review Focus 2); событие `sold <n> <money>` в журнал фракции.
- [ ] **Step 2:** Окно: «Работа: копают N, несут N, руды N; выручка за сегодня N»; событие «Продала N руды за N».
- [ ] **Step 3:** Build — `OK`; в игре: полный цикл копать → нести → продать, деньги фракции растут ровно на заплаченное торговцем (журнал). Commit `DynamicFactions: selling ore`.

### Task 6: Доход — только работа; README, MCM, ревью

**Files:** Modify `core/DfBrain.cpp` (`incomePerMemberHour = 0` в правилах этапа 2), `tests/core_tests.cpp`, `README.txt`, `DynamicFactions.ini`, MCM, `.po`

- [ ] **Step 1:** Тест `test_brain_phase2_no_stub_income`: `DfPhase2Rules().incomePerMemberHour == 0` и `DfDecide` с ним не выдаёт `DF_INCOME`.
- [ ] **Step 2:** Run — FAIL; ввести `DfRules DfPhase2Rules()` (как этап 1, доход 0); адаптер — на этап 2; Run — `0 failed`.
- [ ] **Step 3:** MCM: `Work=1` (вкл/выкл работы, MCM_RESTART не нужен), `MineRadius` 200–1500 (по умолчанию 600). Окно и журнал называют добычу по имени предмета (модовые — как в игре). README (что делают, числа), перевод, `po2mo`.
- [ ] **Step 4:** Итоговое ревью ветки (свежий ревьюер, самая сильная модель), исправления, память проекта, push. Commit `DynamicFactions: phase 2a - income from work only`.

---

## Дальше (отдельный план после проверки 2а)

**2б — на выручку:** покупка дома в Хабе (здания на продажу у фракции города), склад — руда в склад вместо продажи по одной ходке, продажа партиями; затем животные и караваны (пожелания 23.10).
