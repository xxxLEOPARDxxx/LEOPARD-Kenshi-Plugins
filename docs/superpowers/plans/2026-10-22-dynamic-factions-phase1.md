# Dynamic Fractions — этап 1 (ядро, Хаб, одна фракция, окно) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** В Хабе появляется фракция людей с уникальным именем, живёт сама (копит деньги, нанимает, растёт в технике), окно по хоткею показывает её состояние; когда все её члены убиты — фракция исчезает и её код больше не выполняется.

**Architecture:** Чистое ядро на C++ (`core/`, без заголовков игры) принимает снимок фракции и время, возвращает действия; тестируется отдельным консольным exe. Тонкий адаптер к игре (`game/`) делает снимок, применяет действия (создание фракции и отрядов, найм, деньги) и хранит состояние рядом с сохранением. Окно (`ui/`) только читает состояние.

**Tech Stack:** VC10, KenshiLib 0.2.1, RE_Kenshi, MyGUI (игровой), Python 3 (сборка `.mod`), shared/ (Localization.h, ModConfigMenu.h, IniDefaults.h, HoldKey.h, WidgetRef.h).

**Spec:** `docs/superpowers/specs/2026-10-22-dynamic-factions-design.md`

## Global Constraints

- Сборка: `D:\DEV\Kenshi\build.ps1 -SrcDir D:\DEV\Kenshi\ours\DynamicFactions -Name DynamicFactions -SubDirs core,game,ui`.
- В DLL нет кириллицы (проверка: ни одной цепочки `(?:[\xd0\xd1][\x80-\xbf]){3,}`); тексты — английские, перевод в `locale/{en_GB,ru_RU,zh_CN}/LC_MESSAGES/dynamic_factions.po` + `po2mo.py`.
- Строки интерфейса на русском — только ASCII и кириллица (без «» — …).
- MCM: `beginMod(api, "DynamicFactions", "Dynamic Factions", ...)` — заголовок без Tr().
- Раздаём только `DynamicFactions.defaults.ini` (IniDefaults::Prepare).
- Объекты игры — только из главного потока (хуки `GameWorld::_NV_mainLoop_GPUSensitiveStuff`, `ForgottenGUI::update`).
- Оригиналы игры зовём всегда; трогаем только персонажей своей фракции.
- Числа этапа 1: старт — 4 человека и 5000 денег; найм — 1500 за человека; предел численности этапа 1 — 30; доход-заглушка — 15 денег в игровой час на живого члена; техника — уровни 0..10, очко техники стоит 200 денег, уровень L→L+1 требует 100*(L+1) очков; тик решений — раз в игровой час, не больше 1 действия найма за тик.
- Хаб: город `18919-Newwworld.mod`; шаблон стартового отряда — копия «Drifter squad» `3950-gamedata.base`.

## Review Focus

1. Загрузка другого сейва / новая игра без перезапуска — состояние прошлого сейва не должно попасть в новый (state привязан к имени сейва; при загрузке — сброс и чтение заново).
2. Сохранение до первого тика (фракция ещё не основана) — после загрузки основание не повторяется дважды и не теряется.
3. Хаб не загружен / игрок далеко — основание и найм не создают персонажей в пустоте (только когда Хаб найден в списке городов).
4. Все члены убиты, пока игрок на другом конце карты — фракция помечается вымершей по численности, а не по видимым персонажам.
5. Деньги ушли в минус (найм в тот же тик, что вычитание) — бюджет никогда не < 0, действие не выполняется.

---

## Файлы

```
ours/DynamicFactions/
  DynamicFactions.cpp        вход плагина: хуки, MCM, связывает слои
  DynamicFactions.ini        → DynamicFactions.defaults.ini
  README.txt
  core/DfState.h/.cpp        состояние фракции + текстовая сериализация
  core/DfNames.h/.cpp        генератор уникальных имён (раса, зерно)
  core/DfBrain.h/.cpp        решения: снимок → действия
  game/DfGame.h/.cpp         адаптер: фракция игры, основание, найм, снимок, файл состояния
  ui/DfWindow.h/.cpp         окно по хоткею
  locale/...                 .po
  tests/core_tests.cpp       тесты ядра (без игры)
  tests/build_tests.ps1      сборка и запуск тестов (VC10, только core/)
tools/df_make_mod.py         сборка DynamicFactions.mod (FACTION + SQUAD_TEMPLATE)
mods/DynamicFactions/        DynamicFactions.mod, DLL, RE_Kenshi.json, locale, defaults.ini
```

---

### Task 1: Каркас и тестовая обвязка ядра

**Files:**
- Create: `ours/DynamicFactions/tests/build_tests.ps1`, `ours/DynamicFactions/tests/core_tests.cpp`, `ours/DynamicFactions/core/DfState.h`
- Modify: `ours/rebuild_all.ps1` (BuildPlugin DynamicFactions + копирование в мод, как у AIFixed; в список проверки кириллицы)

**Interfaces:**
- Produces: `tests/build_tests.ps1` — компилирует `core/*.cpp` + `tests/core_tests.cpp` в `D:\DEV\Kenshi\build\DynamicFactionsTests\core_tests.exe` (окружение VC10 как в `ours/build.ps1`: `$VC\bin\amd64\cl.exe`, INCLUDE/LIB VC10+SDK 7.1) и запускает; код возврата = число проваленных проверок.
- Produces: макрос `CHECK(cond)` в `core_tests.cpp` — печатает `FAIL file:line cond`, увеличивает счётчик.

- [ ] **Step 1:** Тест-заглушка `test_harness_runs` с `CHECK(1 + 1 == 2)`; `main` возвращает число провалов.
- [ ] **Step 2:** Run: `powershell -File D:\DEV\Kenshi\ours\DynamicFactions\tests\build_tests.ps1` — Expected: `0 failed`.
- [ ] **Step 3:** Commit `DynamicFactions: core test harness`.

### Task 2: Генератор уникальных имён

**Files:** Create `core/DfNames.h/.cpp`; Test: `tests/core_tests.cpp`

**Interfaces:**
- Produces: `std::string DfMakeName(const std::string& race, unsigned seed, const std::vector<std::string>& taken);` — race ∈ {"human","shek","skeleton","hive"}; имя = прилагательное + существительное из словаря расы (ASCII, английские; перевод не нужен — это имя собственное); если занято — следующий вариант по зерну; после 64 попыток — суффикс ` II`, ` III`.

- [ ] **Step 1:** Тесты: `DfMakeName("human", 7, {})` == `DfMakeName("human", 7, {})` (детерминизм); при `taken = {DfMakeName("human",7,{})}` результат другой; для каждой из 4 рас имя не пустое и только ASCII; `DfMakeName("unknown",1,{})` использует словарь "human".
- [ ] **Step 2:** Run tests — FAIL (нет символа).
- [ ] **Step 3:** Реализовать (словари: по 16 прилагательных и 16 существительных на расу).
- [ ] **Step 4:** Run tests — PASS. Commit `DynamicFactions: unique faction names`.

### Task 3: Состояние фракции и его файл

**Files:** Create `core/DfState.h/.cpp`; Test: `tests/core_tests.cpp`

**Interfaces:**
- Produces:
```cpp
struct DfEvent { double hour; std::string text; };            // текст - английский ключ + числа
struct DfFactionState {
    std::string recordId;      // StringID записи FACTION из .mod
    std::string name, race;
    bool founded, extinct;
    double foundedHour, lastTickHour;
    int techLevel; int techPoints;
    std::vector<DfEvent> events;   // не больше 20, старые выбрасываются
};
void DfAddEvent(DfFactionState& s, double hour, const std::string& text);
std::string DfSerialize(const std::vector<DfFactionState>& all);   // строки key=value, секции [faction N]
bool DfDeserialize(const std::string& text, std::vector<DfFactionState>& out);  // false - испорчено, out пуст
```
Деньги и численность в состоянии НЕ хранятся — их берём у игры.

- [ ] **Step 1:** Тесты: круговой путь `DfDeserialize(DfSerialize(x))` сохраняет все поля двух фракций (имя с пробелами и `=`); 25 событий → хранится 20 последних; мусор `"[faction 0]\nname"` → false и пустой out; пустая строка → true и пустой out.
- [ ] **Step 2:** FAIL. **Step 3:** Реализовать (экранирование `\n`, `=` в значениях). **Step 4:** PASS. Commit `DynamicFactions: faction state file`.

### Task 4: Мозг этапа 1

**Files:** Create `core/DfBrain.h/.cpp`; Test: `tests/core_tests.cpp`

**Interfaces:**
- Consumes: `DfFactionState` (Task 3).
- Produces:
```cpp
struct DfSnapshot { int members; int money; double hour; bool homeLoaded; };
enum DfActionType { DF_FOUND, DF_RECRUIT, DF_INCOME, DF_RESEARCH, DF_EXTINCT };
struct DfAction { DfActionType type; int amount; };            // amount: деньги или очки
struct DfRules { int startMembers, startMoney, recruitCost, maxMembers, incomePerMemberHour, techPointCost, maxTech; };
DfRules DfPhase1Rules();   // 4, 5000, 1500, 30, 15, 200, 10
std::vector<DfAction> DfDecide(const DfFactionState& s, const DfSnapshot& snap, const DfRules& r);
void DfApplyBookkeeping(DfFactionState& s, const std::vector<DfAction>& done, double hour); // техника, события, lastTickHour, extinct
```
Правила: не основана и `homeLoaded` → `[DF_FOUND]`; основана и `members == 0` → `[DF_EXTINCT]`; вымершая → `[]`. Иначе за прошедшие целые часы `h = floor(hour - lastTickHour)` (не больше 24): `DF_INCOME` = h*members*income; найм — если `homeLoaded`, `members < maxMembers` и `money + income ≥ recruitCost + 1000` (запас) → один `DF_RECRUIT`; исследование — на остаток сверх запаса 1000 и после найма, не больше 5 очков за тик, `DF_RESEARCH amount = очки`; деньги после всех действий ≥ 0. Уровень растёт в `DfApplyBookkeeping`, когда `techPoints ≥ 100*(techLevel+1)` (остаток переносится), не выше maxTech.

- [ ] **Step 1:** Тесты с точными числами: (a) не основана, homeLoaded=false → пусто; homeLoaded=true → один DF_FOUND. (b) основана, members=0 → DF_EXTINCT; после Bookkeeping `extinct == true`, повторный Decide → пусто. (c) members=4, money=0, 2 часа → DF_INCOME 120, без найма. (d) members=4, money=3000, 1 час (+60) → DF_RECRUIT (3060 ≥ 2500), исследование 0 (остаток 1560 − 1000 запас = 560 → 2 очка = 400) → DF_RESEARCH 2. (e) members=30 → найма нет. (f) 50 часов → считается 24. (g) techPoints 99 + 1 очко на уровне 0 → уровень 1, очки 0. (h) уровень 10 → очки не растут.
- [ ] **Step 2:** FAIL. **Step 3:** Реализовать. **Step 4:** PASS. Commit `DynamicFactions: phase-1 brain`.

### Task 5: `.mod` с записями фракции и отряда

**Files:** Create `D:\DEV\Kenshi\tools\df_make_mod.py`; output `mods/DynamicFactions/DynamicFactions.mod`

**Interfaces:**
- Produces: записи с StringID `df-faction-human-1` (FACTION, копия полей нейтральной фракции «Nameless» `204-gamedata.base`, имя «Dynamic Faction», отношения по умолчанию — нейтральные ко всем) и `df-squad-human-start` (SQUAD_TEMPLATE, копия «Drifter squad» `3950-gamedata.base`, ссылка faction → `df-faction-human-1`, 4 человека).
- Запись `.mod` — через `tools/startdiff.records` + `leopard_difficulty.serialize` (как `leopard_prices.py`); заголовок мода — как у `mods/AIFixed/AIFixed.mod`, число записей = 2.

- [ ] **Step 1:** Скрипт с `--check`: читает готовый .mod, печатает две записи и их поля; без файла — ошибка.
- [ ] **Step 2:** Сгенерировать; `python df_make_mod.py --check` → `df-faction-human-1 FACTION`, `df-squad-human-start SQUAD_TEMPLATE faction=df-faction-human-1`.
- [ ] **Step 3:** `python tools/recdump.py DynamicFactions.mod` (после включения мода в mods.cfg) показывает обе записи без ошибок разбора. Commit `DynamicFactions: mod records`.

### Task 6: Адаптер к игре — фракция, основание, снимок, файл состояния

**Files:** Create `game/DfGame.h/.cpp`; Modify `DynamicFactions.cpp` (хук mainLoop, хуки `SaveManager::load` обеих перегрузок и `SaveManager::save*` — имя сейва)

**Interfaces:**
- Consumes: Task 2–4.
- Produces:
```cpp
Faction* DfBindFaction(const std::string& recordId);   // ou->gamedata->getData(sid) → ou->factionMgr->getOrCreateFaction(data)
TownBase* DfFindHub();                                 // TownList::getAllTowsByGameData(data "18919-Newwworld.mod"), первый загруженный
DfSnapshot DfTakeSnapshot(Faction* f, const DfFactionState& s);  // members = f->countPopulation(NULL); money = f->factionOwnerships->money; hour = ou->getTimeFromStamp_inGameHours(ou->getTimeStamp()); homeLoaded = DfFindHub() != NULL
bool DfFound(Faction* f, TownBase* hub);               // createRandomSquad(f, hub->getPosition(), hub, 4, NULL, squad "df-squad-human-start", NULL, NULL, NULL, true, hand(), NULL, 1.0f, SQ_RESIDENT, false); +5000 денег (`factionOwnerships->addMoney(5000)`)
std::string DfStatePath();                             // <папка сейва>/<имя сейва>.dynamicfactions.txt
```
- Тик: раз в игровой час — снимок → `DfDecide` → выполнить (Task 7 для найма) → `DfApplyBookkeeping` → `Faction::setName(state.name)` при расхождении. Вымершая — тик выходит сразу.
- Загрузка сейва: сбросить состояние, прочитать файл; нет файла — одна фракция `{recordId "df-faction-human-1", race "human", name DfMakeName("human", GetTickCount(), имена всех фракций игры)}`. Сохранение: записать файл.
- Все обращения к объектам игры — под SEH-обёртками (образец — `Safe*` в AIFixed.cpp), сбой — ErrorLog один раз, фракция пропускает тик.

- [ ] **Step 1:** Собрать; в игре (новый сейв, отряд в Хабе): в журнале `DynamicFactions: founded "<имя>" in the Hub, 4 members`; у NPC в Хабе — название фракции = имя.
- [ ] **Step 2:** Сохранить, выйти, загрузить: основание не повторяется, имя прежнее (журнал `restored`). Загрузить другой сейв — у него своё состояние (Review Focus 1, 2).
- [ ] **Step 3:** Commit `DynamicFactions: game adapter - faction, founding, state file`.

### Task 7: Исполнитель действий — доход, найм, исследование, вымирание

**Files:** Modify `game/DfGame.cpp`

**Interfaces:**
- Produces: `bool DfExecute(Faction* f, TownBase* hub, DfFactionState& s, const DfAction& a);` — DF_INCOME: `factionOwnerships->addMoney(amount)`; DF_RECRUIT: `createRandomCharacter(f, hub->getPosition(), активный взвод фракции в Хабе, шаблон персонажа из df-squad-human-start, NULL, 0)` и −1500; DF_RESEARCH: −amount*200; DF_EXTINCT: только событие. Деньги перед вычитанием проверяются заново (Review Focus 5); не хватило — действие пропускается.
- Вымирание по `countPopulation(NULL)` (Review Focus 4). Что `countPopulation(NULL)` считает всех членов фракции, включая незагруженных, — проверить в Task 6 Step 1 по журналу (численность после основания = 4); если нет — сумма `countPopulation` по всем городам + отряды вне городов, решение записать в README.

- [ ] **Step 1:** В игре: через 1–2 игровых часа на ускорении в журнале — найм и исследование; численность в Хабе растёт, деньги ≥ 0.
- [ ] **Step 2:** Убить всех членов (в т.ч. проверка вдали от Хаба — Review Focus 4, 3): журнал `extinct`, найма больше нет, тик пустой.
- [ ] **Step 3:** Commit `DynamicFactions: action executor`.

### Task 8: Окно по хоткею и MCM

**Files:** Create `ui/DfWindow.h/.cpp`; Modify `DynamicFactions.cpp`, `DynamicFactions.ini`, `locale/*/dynamic_factions.po`

**Interfaces:**
- Produces: `void DfWindowToggle(); void DfWindowUpdate(const std::vector<DfFactionState>&, const std::vector<DfSnapshot>&);` — окно MyGUI (слой Overlapped, скин `Kenshi_WindowCX`, цвет текста — палитра GameTheme), строки: имя, раса, основана (день), численность, деньги, уровень техники и очки/нужно, вымерла ли, последние 10 событий. Обновление раз в 1 с, текст ставится только при изменении (правило из [kenshi-perf-profiling]).
- Хоткей `[DynamicFactions] WindowKey=CTRL+F10` (HoldKey, по нажатию — переключение); MCM: Enabled (MCM_RESTART), WindowKey, Debug.

- [ ] **Step 1:** В игре: CTRL+F10 открывает/закрывает окно; числа совпадают с журналом; язык ru/en.
- [ ] **Step 2:** Проверить кириллицу в DLL (0) и FPS с открытым окном (не ниже, чем без него, по счётчику). Commit `DynamicFactions: info window and MCM`.

### Task 9: README, папка мода, ревью

- [ ] Ревью по [feedback-ai-fixed-rules] и [kenshi-perf-review]: утечки (lektor, MyGUI), перф тика, SEH, главный поток.
- [ ] README.txt (что делает этап 1, числа правил, файл состояния, хоткей), `workshop/images/DynamicFactions` создать.
- [ ] Commit `DynamicFactions: phase 1 README`. Не публиковать (решает пользователь).
