# Extra Durability (HARD)

**Версия 1 / Version 1** · LEOPARD · [English](#english)

Оружие и броня изнашиваются в бою у игрока и NPC. У каждого предмета своя прочность. В подсказке показаны текущая и максимальная прочность.

## Как работает износ

- По умолчанию оружие ближнего боя теряет 8 единиц за засчитанное попадание или блок. Арбалет теряет 8 за выстрел, даже при промахе. Слой брони или одежды, выбранный штатной проверкой покрытия, теряет 5 за попадание.
- До 50% прочности оружие наносит обычный урон. Ниже 50% урон линейно снижается до 20%. Изношенная броня реже останавливает попадания, прошедшие штатную проверку покрытия.
- При 0% прочности оружие перестаёт наносить урон, а броня перестаёт защищать в проверке покрытия. Галочка «Уничтожать предметы при 0%» удаляет сломанный предмет без остатка. Когда галочка выключена, сломанный предмет остаётся для ремонта или переплавки. Мейто сохраняет 20% урона даже при нуле прочности.
- На предметы, которые принимает печь, износ снижает выход материала от 100% до 20% обычного. Игра округляет итоговый выход.

| Прочность | Оружие ближнего боя | Броня и арбалеты |
| ---: | --- | --- |
| 600 | Rusted Junk, Rusting Blade | Prototype |
| 1300 | Mid-Grade Salvage | Shoddy |
| 2000 | Old Refitted Blade, Refitted Blade | |
| 3000 | Catun No. 1 | Standard |
| 5000 | Catun No. 2 | |
| 7000 | Catun No. 3 | High |
| 10000 | Mk I, Mk II, Mk III | |
| 15000 | Edge Type 1, 2, 3 и остальные грейды от 70 | Specialist |
| 30000 | Штатный грейд Meitou | Masterwork |

По умолчанию оружие ближнего боя с числовым грейдом 100 и выше получает защиту Мейто от поломки автоматически, в том числе с новым производителем или названием грейда из мода. Порог можно изменить в MCM. Защита оставляет выбранный минимум урона даже при нуле прочности. Штатный грейд Мейто защищён при любом пороге.

## Ремонт

Обычные **Инструменты** можно изготовить на оружейных станках I-III после базовой ковки или в кузне тяжёлых доспехов после изучения пластинчатой брони. Рецепт требует три железных листа и одну кожу. Инструменты сохраняют штатное применение для вскрытия замков и снятия кандалов.

Возьмите инструменты в инвентарь или надетый рюкзак и удерживайте ПКМ на этом персонаже. Выберите «Починить оружие» или «Починить броню». Если в категории несколько повреждённых вещей, сначала откроется выбор предмета. Надетая экипировка идёт первой, затем вещи из инвентаря и рюкзака.

Прочность восстанавливается постепенно. Персонаж использует штатную анимацию и звук скелетного ремкомплекта. При нулевом навыке полный заряд даёт 25% максимальной прочности предмета или 500 единиц, если четверть меньше. По умолчанию при навыке 100 Инструменты восстанавливают на 50% больше единиц, а ремонт идёт вдвое быстрее. Персонаж получает опыт соответствующего вида ковки за восстановленные единицы прочности. Как только предмет исправен, остаток заряда сохраняется. Движение, бой или потеря предмета прерывают ремонт.

## Настройки MCM

Откройте «Настройки» → MCM → «Extra Durability (HARD)». Изменения применяются сразу. Для ручной настройки закройте игру и отредактируйте `ExtraDurability.ini` в папке мода. При первом запуске плагин создаёт его из `ExtraDurability.defaults.ini`.

«Включить плагин» управляет износом, боевыми эффектами и ремонтом. «Износ у NPC» применяет к их экипировке те же правила, что и к отряду игрока. «Уничтожать предметы при 0%» управляет удалением сломанных вещей и включено по умолчанию. «Уведомления о поломке» включены по умолчанию и показывают имя бойца зелёным, а текст и название предмета красным. Уведомления появляются только для отрядов игрока при достижении нуля. Выход печи можно настроить отдельно. Полоску прочности внутри значков оружия, брони и одежды можно расположить сверху или снизу либо скрыть. Полоска появляется только после износа и исчезает при полном ремонте.

- **Износ и поломка.** Порог защиты по умолчанию равен 100. Значение 0 защищает только штатный Мейто. Расход прочности настраивается отдельно для удара или блока оружием, выстрела арбалетом и попадания в слой брони. Значение 0 отключает износ для соответствующего действия.
- **Урон оружия** и **Защита брони.** У каждого раздела свои начальная и конечная границы штрафа и минимальный эффект. По умолчанию падение начинается ниже 50%, а при 0% достигает 20% штатного эффекта. Сломанный предмет при 0% теряет свой боевой эффект. Если конечная граница выше начальной, используется начальная. При совпадении границ эффект падает сразу до минимума на этой границе, кроме полностью исправных вещей.
- **Прочность по ступеням.** Девять значений из таблицы можно изменить. Сейв хранит остаток прочности в единицах, поэтому смена максимума может изменить процент уже изношенной экипировки.
- **Ремонт.** Настраиваются доля и минимум восстановленных единиц за полный комплект, расход времени на полный заряд, бонусы эффективности и скорости от навыка, а также множитель опыта. Если доля и минимум равны 0, ремонт отключается.
- **Переплавка.** При включённом учёте износа выход растёт линейно от заданного минимума при 0% прочности до 100% при полной прочности.
- **Подробный журнал.** Последняя настройка, выключена по умолчанию (`Debug=0`). Включает диагностику ремонта, удаления снаряжения и загрузки переводов. Ошибки записываются всегда.

## Установка, совместимость и сохранения

Для Kenshi 1.0.65 x64 (Steam и GOG) с RE_Kenshi и совместимой KenshiLib. При ручной установке распакуйте папку `Extra Durability (HARD)` в `mods` и включите `Extra Durability (HARD).mod` в списке модов. Mod Config Menu нужен для настройки через интерфейс игры. Текст доступен на английском, русском и упрощённом китайском.

Используйте одну систему износа экипировки за раз. Если другой мод меняет рецепт обычных Инструментов или список рецептов в кузне, проверьте, какой рецепт доступен с вашим порядком загрузки.

Сохранившаяся экипировка сохраняет исходные записи и свойства. Уничтожение предмета записывается в сейв и необратимо при отключении плагина. При включённом уничтожении уже сломанные предметы проверяются после загрузки и при осмотре в инвентаре. Очередь обрабатывается порциями до 16 записей за кадр. Если сохранить игру до конца обработки, оставшиеся предметы с нулевой прочностью снова проверяются после загрузки. Плагин хранит прочность в своём числовом поле записи предмета. После отключения через MCM игра восстанавливает штатные боевые эффекты. Отдельный ремкомплект предыдущей сборки поддерживается для уже созданных предметов. Текущие действия ремонта не переносятся в другой мир.

### Изменения в версии 1

- Добавлен сохраняемый боевой износ оружия ближнего боя, арбалетов, брони и одежды. Износ экипировки NPC можно отключить.
- Изношенное снаряжение теряет урон или защиту. По умолчанию сломанные предметы уничтожаются. Можно настроить защиту оружия по грейду, сохранять сломанные вещи и показывать уведомления о поломке в своём отряде.
- Прочность показана в подсказках. На значках изношенной экипировки можно показать полоску прочности.
- Обычные Инструменты чинят снаряжение через меню ПКМ на персонаже. Инструменты можно изготовить на оружейных станках и в кузне тяжёлых доспехов. Профильная ковка улучшает ремонт и получает за него опыт.
- Износ может уменьшать выход материалов в печи. В MCM настраиваются износ, боевые штрафы, ступени прочности, ремонт и переплавка. Текст в игре доступен на английском, русском и упрощённом китайском.

## English

Individual weapons and armour wear in combat for both your squads and NPCs. Hover over a piece of equipment to see its current and maximum durability.

### Combat wear

- By default, a melee weapon loses 8 points for each registered hit or block and a crossbow loses 8 per shot, including misses. A layer of armour or clothing selected by the normal coverage check loses 5 per hit.
- Weapons deal full damage down to 50% durability. Below 50%, weapon damage drops linearly towards 20%. Worn armour is less likely to stop a hit even when the normal coverage check selects it.
- At zero condition, broken weapons stop dealing damage and broken armour stops protecting the wearer. The destruction setting removes the item entirely. With destruction off, broken gear stays available for repairs or smelting if the furnace accepts it.
- Vanilla Meitou always stays intact, and melee weapons with a numerical grade of 100 or above do so by default, including modded grades. Protected weapons retain 20% damage at zero with default settings. Changing the protection threshold does not change the item's durability tier.
- Smelting yield falls with condition, from 100% at full durability to 20% at zero by default. The game rounds the result.

| Durability | Melee weapons | Armour and crossbows |
| ---: | --- | --- |
| 600 | Rusted Junk, Rusting Blade | Prototype |
| 1300 | Mid-Grade Salvage | Shoddy |
| 2000 | Old Refitted Blade, Refitted Blade | |
| 3000 | Catun No. 1 | Standard |
| 5000 | Catun No. 2 | |
| 7000 | Catun No. 3 | High |
| 10000 | Mk I, Mk II, Mk III | |
| 15000 | Edge Type 1, 2, 3 and other grades of 70 or above | Specialist |
| 30000 | Vanilla Meitou | Masterwork |

### Repair

Craft ordinary **Tools** from three Iron Plates and one Leather at Weapon Smiths I-III after Basic Weapon Smithing or at the Heavy Armour Smithy after Plate Armour Crafting. Tools still open locks and remove shackles.

Put Tools in a character's inventory or worn backpack. Hold right-click on that character and choose **Repair Weapon** or **Repair Armour**. If several items match, choose one from the list. Equipped items appear first, followed by items in the inventory and backpack.

The character repairs gradually with the normal skeleton repair animation and sound. At smithing skill zero, a full charge restores 25% of the item's maximum durability or 500 points, whichever is greater. At skill 100 with default settings, a charge restores 50% more and work proceeds twice as fast. The character earns experience in the relevant smithing skill for durability restored. A partial repair leaves unused charge for later. Moving, fighting or losing access to the item interrupts the repair.

### MCM settings

Open Options → MCM → **Extra Durability (HARD)**. Changes apply immediately. For manual settings, close the game and edit `ExtraDurability.ini` in the mod folder. The plugin creates it from `ExtraDurability.defaults.ini` on first launch.

- **Durability.** Enable the plugin, NPC wear, destruction and player squad breakage notices. A condition bar can appear at the top or bottom of worn gear icons, disappearing when the item returns to full condition.
- **Wear and breakage.** Set wear per melee hit or block, crossbow shot and armour hit. A value of 0 disables new wear for that action. Set the protected melee grade to 0 to keep only vanilla Meitou protected.
- **Weapon damage and armour protection.** Set each curve's start, end and minimum effect. The end is capped at the start. Matching endpoints apply the minimum at that threshold, with full-condition items keeping their full effect.
- **Durability by tier.** Adjust the nine capacities listed above. The save stores remaining points, so changing the maximum can affect the percentage displayed on worn gear.
- **Repair.** Set the restoration percentage and minimum, time per full charge, skill bonuses and experience multiplier. Setting both restoration amounts to 0 disables repairs.
- **Smelting.** Enable condition-based yield and set the minimum yield at zero. Yield increases linearly to the normal amount at full durability.
- **Detailed log.** The final setting is off by default (`Debug=0`). It enables diagnostics for repairs, gear removal and translation loading. Errors are always logged.

### Installation, compatibility and saves

For Kenshi 1.0.65 x64 (Steam and GOG) with RE_Kenshi and a compatible KenshiLib. For manual installation, extract the `Extra Durability (HARD)` folder into `mods` and enable `Extra Durability (HARD).mod` in the mod list. Mod Config Menu provides the in-game settings. Text is available in English, Russian and Simplified Chinese.

Use one equipment-wear system at a time. If another mod edits the regular Tools recipe or smithy recipes, check which recipe is available with your load order.

Remaining gear keeps its original records and properties. Item destruction is saved and persists after the plugin is turned off. When destruction is enabled, broken gear is checked after loading and when viewed in inventory. The game processes pending removals in batches of up to 16 per frame, and remaining zero-condition items are checked after reloading. The plugin stores condition in its own numeric field on the saved item record. Disabling it in MCM restores normal combat effects for remaining items. Previously crafted Equipment Repair Kits still work. Active repairs end when entering another world.

### Version 1 changes

- Added persistent combat wear for melee weapons, crossbows, armour and clothing, with optional wear for NPC equipment.
- Worn equipment loses damage or protection. Broken items are destroyed by default, with configurable melee grade protection, an option to keep broken gear, and breakage notices for player squads.
- Item tooltips show durability and worn equipment icons can show a condition bar.
- Ordinary Tools repair gear through character right-click commands. Craft Tools at weapon and heavy armour smithies. The relevant smithing skill improves repairs and earns experience.
- Gear condition can reduce material yield at the item furnace. MCM controls wear, combat penalties, durability tiers, repair and smelting. In-game text is available in English, Russian and Simplified Chinese.

## Исходный код / Source code

Поведение мода Durability изучалось как пример взаимодействия с Kenshi. Формулы, сохранение, меню и рецепт этой реализации входят в Extra Durability. Лицензия приложена в файле `LICENSE`.

Durability was studied as a reference for integration with Kenshi. Extra Durability includes its own rules, persistence, menus and recipe. See the bundled `LICENSE`.

[Исходники Extra Durability / Extra Durability source code](https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins/tree/main/ExtraDurability)
