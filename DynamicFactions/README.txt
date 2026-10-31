Dynamic Factions - development build / рабочая сборка 31.10.2026

Требуется RE_Kenshi с KenshiLib 0.5.0; поддержка MCM необязательна.
Это проверочная сборка одной фракции в Хабе, не публичный релиз.

ЧТО РАБОТАЕТ

Фракция получает начальных людей, навыки и бюджет один раз по сценарию.
Дальше почасового дохода за численность нет. Деньги за покупку абстрактных
очков техники не списываются, исследования до следующего этапа приостановлены.

Работники ищут естественные жилы, в том числе добавленные модами, добывают
штатным заданием игры, забирают реальные предметы и несут их торговцам Хаба.
Носильщик подходит к торговцу и продаёт без диалога. Предмет попадает
в реальный запас лавки, который отображается в торговом окне, а деньги
переводятся из кошелька торговца в бюджет фракции. После подтверждённой
сделки над носильщиком появляется штатная всплывающая сумма. Проверяются цена этого торговца/города, деньги, вместимость,
принадлежность, здоровье, бой и допустимая дистанция. Нет оплаты - груз остаётся.

Полный носильщик освобождает очередь забора, недостижимые назначения получают
паузу перед повтором. Найм посланника имеет приоритет. Пауза игры останавливает
экономику. Первая готовая единица вызывает носильщика: старого ожидания 30
единиц нет; при подходе он забирает доступный товар в пределах вместимости.
Строка рабочего навыка в карточке нашего действующего шахтёра остаётся видимой
и вне камеры; значение подставляется только на время отрисовки карточки. Доступные публичные жилы в радиусе поиска от Хаба используются
сразу, без обязательного маршрута разведчика по городу. Фракция пока работает
только в загруженных участках; полноценная дальняя экономика ещё не реализована.

СОХРАНЕНИЕ

Настоящие вещи, кошельки отрядов и персонажей сохраняет Kenshi. Общая казна
NPC-фракции движком не сохраняется: её точная сумма вместе с именами, жилами
и историей записывается в dynamicfactions.txt внутри того же сейва. После
загрузки груз восстанавливается из настоящего инвентаря, включая рюкзак;
повторных выдач груза и повторной выплаты по журналу задач нет. Выручка,
показанная как "за сеанс", сбрасывается. Казна восстанавливается точной суммой
однократно после загрузки мира, до найма/работы; стартовый грант не повторяется.
Старый сейв без снимка казны не получает выдуманную компенсацию автоматически.

Старые условные запасы сохранены совместимо, но не прибавляются к настоящим,
не добываются снова и не продаются. Испорченное состояние отвергается; уже
существующая фракция принимается без второй стартовой выдачи.

Подтверждённый отказ добавления возвращает единицу источнику. Подтверждённое
неприменение оплаты позволяет вернуть оплату и товар по проверенным остаткам.
Если результат native операции не удаётся однозначно сверить, работа
останавливается с ошибкой. Блокировка записывается в сейв и сохраняется при
загрузке; слепого повторения или выдуманной компенсации нет. Для такого случая
нужна сверка или предыдущий исправный сейв. Это ограничение не следует
путать с успешной обычной продажей или обещанием восстановить любой сбой движка.

НАСТРОЙКИ [DynamicFactions]

Enabled=1             включение плагина; требуется перезапуск, в MCM звёздочка.
Difficulty=medium     easy/medium/hard/veryhard: навыки основателей10/30/50/70.
StartMoney=5000       только новая фракция; личный INI при установке сохраняется.
WindowKey=CTRL+F10    окно наблюдения; NONE отключает клавишу.
Work=1                реальная добыча/переноска/торговля, применяется сразу.
MineRadius=3000       радиус поиска местных жил от Хаба,500-5000метров.
Debug=0               подробный журнал, последняя настройка MCM. Ошибки всегда.

Переводы интерфейса: русский, английский, китайский. В поставке только MO;
редактируемые PO остаются в исходниках. Мод не заменяет ИИ прочих фракций.

ENGLISH

Experimental loaded-world mining, hauling and trading for one faction in the
Hub. Founding grants are explicit and one-time. Ongoing passive income and
cash-for-tech research are disabled. Real items are mined, carried and
transferred to the real shop stock shown by its trade window; the merchant
pays the faction from its real purse. Sales do not start dialogue. A native-style
currency popup above the hauler shows only the confirmed payment. Native town/trader pricing, inventory room, funds,
ownership, distance and actor condition are checked again before mutation.

Cargo, characters and platoon wallets are saved by Kenshi. The shared NPC
faction treasury is not: its exact saved balance is written with metadata,
known resources and events in dynamicfactions.txt. On load, work is rebuilt
from actual inventory contents, including backpacks. It never replays payment
or creates cargo from a saved task counter. Displayed session revenue resets;
the real treasury is restored absolutely once after world loading, before
economic actions. No starting grant is replayed. Legacy saves lacking this
snapshot receive no guessed refund. Legacy virtual ore stays preserved but inactive.

Confirmed rejection restores goods; confirmed unpaid trades can compensate
only when original balances and counts are proven. Unknown native outcomes
quarantine the work, including after saving/loading, rather than replaying an
uncertain mutation. Such a fault needs investigation or a prior healthy save;
this is not a guarantee of recovery from arbitrary engine corruption.

Work toggles immediately; disabling also stops saved native mining/movement
jobs of the faction's available workers. Enabled requires restart. Search
radius500-5000m; public local deposits are available immediately, without a
mandatory scouting circuit. Combat and recruiting
take priority; full carriers/unreachable targets use bounded retry. The first real output starts delivery; the former 30-unit wait is removed.
The native work-skill row stays stable for managed, actually operating members
offscreen; its transient value is supplied only during GUI rendering. No virtual
production is active; a persistent distant economy is a later stage.

UI: ru_RU/en_GB/zh_CN. Detailed logging is off by default and last in MCM;
errors are always logged. CTRL+F10 opens the observation window. No public
Nexus/Steam release is prepared for this experimental build.

DEVELOPMENT REFERENCES

docs/superpowers/specs/2026-10-30-dynamic-factions-fair-simulation.md
docs/superpowers/plans/2026-10-30-dynamic-factions-economy-foundation.md
docs/dependencies/kenshilib-sdk-audit-2026-10-30.md
docs/dependencies/2026-10-31-dynamic-factions-native-ai.md
docs/superpowers/plans/2026-10-31-dynamic-factions-supplies.md

ПРОВЕРКА ХРАНИЛИЩА МАГАЗИНА / SHOP STOCK CHECK

Исправлен адресат продажи: личные карманы NPC не совпадают с запасом окна
торговли. На один прибывший рейс создаётся временный штатный ShopTrader;
из его карты выбирается реальная секция склада лавки или рюкзака отряда.
Сверяется именно эта секция: рост остатка в другой секции того же инвентаря
не подтверждает продажу и не разрешает оплату.
Товар переносится именно туда, а оплата разрешается только после сверки
настоящего остатка. Копии торгового окна не считаются запасом. Предел проверки
128 секций; нет места или доступного склада - продажи нет. Пока игрок торгует
с этим магазином, NPC ждёт: окно и предмет на курсоре не перестраиваются.
Проверены native-маршрут, сборка и модель отказов; показ товара в игре после
новой продажи, закрытия окна и сохранения/загрузки ещё требует приёмки.
Уже проданные старой C5 сборкой вещи остаются в личном инвентаре торговца;
эта правка не переносит их автоматически и не выплачивает деньги повторно.

The sale destination now uses the shop's real backing section, not the NPC's
personal pockets or copied trade-window items. One disposable native ShopTrader
view resolves stock per arrived batch; at most128 backing sections are checked.
Payment requires growth in that exact published section; a hidden section of
the same parent inventory cannot substitute for visible shop stock.
Only a confirmed real stock increase permits payment. Full/unavailable stock
refuses the sale. NPC trades defer while the player uses that shared shop;
the open window and pending mouse item are not rebuilt. Native routing, build
and failure models are checked; live visibility after a new sale, reopening and
save/load still needs acceptance. Items sold by the older C5 build remain in the
merchant's personal inventory; no automatic relocation or duplicate payment occurs.

ПРОГРЕСС ПРИ СБЛИЖЕНИИ / APPROACH PROGRESS

Движение торговца обновляет точку назначения, но не сбрасывает отсчёт
застревания. Его сбрасывает реальное продвижение носильщика; неподвижный
носильщик больше не может бесконечно ждать только из-за блуждания цели.
Moving merchants update the native destination without resetting the stuck
worker deadline. Actual worker movement remains the progress criterion.

30.10: пользователь подтвердил всплывающую сумму при продаже бармену.
Физический запас магазина и save/load после сделки ещё требуют приёмки.
The user confirmed the currency popup when selling to the barman. Physical
shop stock and save/load after the trade still await acceptance.

ПРОВЕРЕНО31.10 / VERIFIED31 OCTOBER

В отдельном DF-earned-20261031 после обычной загрузки без импорта восстановлены
та же фракция из6участников и14578катов (14000+578). В настоящих рюкзаках двух
покупателей найдены все6сохранённых единиц:5железа и1медь. Для этого случая
повторная выдача/оплата не наблюдалась. Сохранение с грузом в пути, повторные
загрузки и остальные пограничные случаи ещё не закрыты.
The isolated DF-earned-20261031 reload restored the same six-member faction
and14578cats (14000+578). The two buyers' physical backpacks retained all six
saved units: five iron and one copper. No repeated grant/payment was observed
in this case. In-transit saves, repeated reloads and other boundaries remain open.

СНАБЖЕНИЕ: ЭКСПЕРИМЕНТАЛЬНАЯ C8 / SUPPLIES: EXPERIMENTAL C8

C8 - внутреннее обозначение тестовой сборки, не новый публичный выпуск.
Первый игровой цикл еды и загрузка после него проверены31.10; пограничные
случаи и длительный перф-прогон остаются в плане.
У обычных живых участников DF отключена штатная подпитка NPC. Функция игры
вызывается всегда; её расход голода, fed, поедание, расовые правила и штрафы
не заменяются. Скелеты, животные, рабство/тюрьма и чужие фракции сохраняют
своё штатное поведение. Проверка действует и до готовности окна после загрузки,
чтобы задержка тика не выдавала питание заново.

При малом запасе участник идёт к настоящему торговцу и покупает подходящую
для своей расы свежую еду. Выбирается доступная цена за питательность и место;
деньги перечисляются из казны фракции в кошелёк магазина. Резерв на уже начатый
найм защищён. Чужая открытая торговля откладывает сделку. После снабжения
участник возвращается к добыче/перевозке. Работа не перебивает лечение, сон,
спасение и бой. Выключение Work останавливает добычу, но не потребность в еде.

Один маршрут и максимум одна покупка на шаг5секунд; до4кандидатов за шаг,
ограниченные обходы, максимум64вида предложения за одну оценку. Надетые
рюкзаки учитываются через настоящие секции. Стопки считаются по питательности,
включая начатую порцию. Оплата подтверждается до передачи еды: при отказе
инвентаря и доказанном отсутствии передачи возвращаются реальные деньги.
Еду из надкушенной стопки для отмены не извлекаем. Неизвестный исход сделки
блокирует дальнейшие операции и сохраняется как workFault, без повторной оплаты.
Недостижимые/дорогие магазины перебираются; маршрут уступает очередь не позже
3минут активного времени, даже если участник продолжает двигаться.

Новые события/подсказки доступны RU/EN/CN; подробный журнал по умолчанию0.
В тестовой копии куплены3настоящих предмета модовой еды за5367катов:
товар ушёл от продавца, деньги сошлись в обоих кошельках, началось поедание.
После отдельной продажи меди казна9304и все6персонажей/предметы/остатки
еды сохранились при обычной загрузке нового DF-food-earned-20261031.
Это один цикл; голод при отсутствии еды, прочие расы и FPS/долгая память ещё
не приняты в игре. Рюкзаки покупкой, мародёрство, экипировка и полный прогноз
резерва для необязательных улучшений остаются следующими этапами. Код
Squad Autonomy и переключатели player/NPC не переносились.

C8 is an internal test-build label, not a new public release. Source and
the first live food cycle and its subsequent reload were checked on31October.
Boundary cases and long performance runs remain open. Ordinary
organic DF members no longer receive the native NPC hunger floor. The original
function always runs; native consumption, fed, eating, racial rules and penalties
remain intact. Robots, animals, slave/prison states and other factions retain
native behavior. The scope works before delayed UI/session readiness on load,
so the tick grace period does not grant food again.

A low-stock member physically approaches a real shop and buys fresh food its
race can eat. Selection considers actual price per nutrition and inventory room.
Faction treasury pays the shop purse; existing recruitment commitments remain
protected. Player trading defers shared-shop operations. Normal work resumes
after supply; medical, rescue, sleep and combat goals take precedence. Turning
off Work stops mining, but food procurement continues.

One route and at most one purchase per5-second step; at most four candidates
are scanned per step and64offer types per quote. Equipped backpacks are visited
through their physical sections. Nutrition includes partly eaten stacks. Payment
is confirmed before food insertion; proven refused/restored insertion refunds
real balances. No food is extracted from partly eaten stacks for compensation.
Unknown outcomes durably quarantine work rather than replay mutations. Other
shops are tried after local price/shape failures, and any route yields within
3minutes of active time, including a supplier that keeps moving without arrival.

RU/EN/CN strings are present; detailed logging defaults off. Three real modded
food items were bought for5367cats: stock and both purses reconciled, and native
eating began. After a separate copper sale, the new DF-food-earned-20261031
reload retained9304cats and the same six members, possessions and food charges.
This verifies one cycle; no-food consequences, other races and long FPS/memory
acceptance remain open. Backpack
procurement, looting, role-based gear and the full optional-upgrade reserve
forecast are later stages. Squad Autonomy code/player-NPC switches were not copied.


C9: медикаменты, первый рюкзак и резерв (31.10.2026, НЕ РЕЛИЗ)

Единый маршрут снабжения теперь покупает настоящие аптечки для живых,
ремкомплекты для скелетов и биологических персонажей с протезами, а затем
первый подходящий рюкзак. Еда важнее медикаментов и снаряжения; новый голод
может прервать текущий поход. Штатные лечение, сон, спасение и бой сохраняют
приоритет. Покупки оплачиваются из настоящей казны в настоящий кошелёк лавки.

Сначала учитываются уже имеющиеся остатки и рюкзаки. При выборе рюкзака
проверяются вместимость, стаки, вес, снижение веса, штрафы скорости/боя/скрытности
и расовые ограничения. Лёгкие персонажи отказываются от тяжёлых штрафов.
Свой рюкзак из основного инвентаря надевается штатным Inventory::equipItem.
Отказ надевания оставляет вещь на месте и не вызывает повторную покупку.
Вложенные рюкзаки не извлекаются автоматически; замена занятого рюкзака,
апгрейды оружия/брони и мародёрство остаются следующими этапами.

Фракция узнаёт цены и ассортимент только при физическом посещении лавки.
В резерв входят дефицит еды/медикаментов, обещанные наймы и запас не меньше
1000 катов (250 на участника, если больше). Уже имеющиеся припасы убирают
дефицит. Реальные конечные количества распределяются между всем составом;
один общий склад нескольких продавцов не считается несколько раз. Неполный
или устаревший расчёт откладывает плановые покупки. Еда может использовать
резерв выживания, но не обещанные деньги найма.

Техническая проверка: 1276 проверок ядра, 40 маршрута, 37 инвентаря,
35 предложений/транзакций, 23 рынка, 20 рюкзаков и 39 жизненного цикла.
Все прошли; итоговая DLL совместимо собрана, 132 импорта сверены с реальными
экспортами установленных KenshiLib/Ogre/MyGUI, привязка голода проверена в PE.
Независимое ревью исправило учёт конечного/общего склада, защиту сохранения
при надевании и временное владение памятью на аварийном пути.

Перф: шаг по-прежнему раз в 5 секунд, одна покупка и до четырёх кандидатов
плюс активный снабженец. Кэш до 32 лавок по 128 записей, срок 120 секунд;
содержит только SID, полные идентификаторы складов и числовые котировки.
Все просмотренные склады регистрируются до заполнения списка предложений.
Прогноз проверяет лимит 2 мс между обходами лавок/членов; это не гарантирует
прерывание одного штатного вызова. При загрузке/смене мира runtime-кэш сбрасывается,
владение и остатки вновь читаются из реальных предметов. Нативные объекты
используются только в главном потоке.

Игровая приёмка C9 ещё НЕ завершена: нужны настоящая покупка/надевание,
сверка обоих кошельков и загрузка отдельного заработанного сохранения.
Длительные FPS/память и скелетная фракция отдельно не проверены. Проверки
исходников и ревью не доказывают отсутствие любых утечек.

C9: medicine, an initial backpack and operating reserve (31 October 2026)

This is an internal test build, NOT a public release. The existing single
supply route now buys real first aid for organics, repair supplies for robots
and biological prostheses, then a suitable initial backpack. Food precedes
medical stock and gear, including hunger that begins during a gear trip.
Native medical, rescue, sleep and combat tasks retain priority. Real faction
funds pay the real shop purse; no items or income are generated.

Owned supplies remove their deficits. Bag selection considers useful capacity,
stacking, weight reduction, empty weight, athletics/combat/stealth penalties
and native race restrictions. Light characters reject heavy penalties. An
owned root-inventory bag is equipped through native Inventory::equipItem;
refusal retains the item and prevents repurchase. Nested bags are not pulled
out automatically. Replacing occupied bags, weapon/armour upgrades and looting
remain later work.

Market knowledge comes only from actual shop visits. The reserve protects
remaining food/medicine deficits, recruitment commitments and at least1000cats
(or250per member when larger). Limited observed quantities are shared across
the whole faction; multiple shopkeepers cannot multiply their common stock.
Incomplete or stale forecasts defer planned spending. Food can use survival
reserves but cannot spend committed recruitment funds.

Verification passed:1276core,40route,37inventory,35offer/transaction,23market,
20bag and39session/lifecycle checks. The final compatible DLL build passed;
132imports match installed KenshiLib/Ogre/MyGUI exports and the nonvirtual
hunger PE binding is verified. Independent review led to finite/shared-stock
allocation, reentrant-save protection around equip and non-owning POD storage
across the native SEH count boundary.

Performance/lifetime: one purchase per existing5-second step, at most four
candidate stock reads plus the active supplier. Cache:32shops x128records,
120-second freshness, value/identifier data only. Every backing pool is noted
before offers fill the cap. Forecast checks its2ms budget between shops and
members, without claiming it can interrupt a single native call. Runtime
quotes/routes reset before world teardown and on load; possessions are reread.
All native objects stay on the game thread.

C9 live acceptance is still OPEN: real purchases/equip, both purses and a
separate earned-save reload must be compared. Long FPS/memory runs and a
skeleton faction remain untested. Source tests/review do not prove the absence
of all leaks.
