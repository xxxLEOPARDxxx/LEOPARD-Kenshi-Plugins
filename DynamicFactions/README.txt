Dynamic Factions - development build / рабочая сборка 30.10.2026

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
