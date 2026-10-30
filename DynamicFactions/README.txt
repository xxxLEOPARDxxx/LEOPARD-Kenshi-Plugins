Dynamic Factions - development build / рабочая сборка 30.10.2026

Требуется RE_Kenshi с KenshiLib 0.5.0; поддержка MCM необязательна.
Это проверочная сборка одной фракции в Хабе, не публичный релиз.

ЧТО РАБОТАЕТ

Фракция получает начальных людей, навыки и бюджет один раз по сценарию.
Дальше почасового дохода за численность нет. Деньги за покупку абстрактных
очков техники не списываются, исследования до следующего этапа приостановлены.

Работники ищут естественные жилы, в том числе добавленные модами, добывают
штатным заданием игры, забирают реальные предметы и несут их торговцам Хаба.
Перед продажей носильщик подходит к торговцу и произносит реплику. Продажа
передаёт предмет в настоящий инвентарь торговца и переводит его деньги в
бюджет фракции. Проверяются цена этого торговца/города, деньги, вместимость,
принадлежность, здоровье, бой и допустимая дистанция. Нет оплаты - груз остаётся.

Полный носильщик освобождает очередь забора, недостижимые назначения получают
паузу перед повтором. Найм посланника имеет приоритет. Пауза игры останавливает
экономику. Известными становятся жилы, замеченные людьми фракции, а не только
камерой игрока. Свободные люди разведывают окрестности. Фракция пока работает
только в загруженных участках; полноценная дальняя экономика ещё не реализована.

СОХРАНЕНИЕ

Настоящие вещи, кошельки и персонажей сохраняет Kenshi. Плагин пишет имена,
известные жилы и историю в dynamicfactions.txt внутри того же сейва. После
загрузки груз восстанавливается из настоящего инвентаря, включая рюкзак;
повторных выдач груза и повторной выплаты по журналу задач нет. Выручка,
показанная как "за сеанс", сбрасывается; настоящий бюджет фракции сохраняется.

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
MineRadius=3000       радиус разведки и поиска от Хаба,500-5000метров.
Debug=0               подробный журнал, последняя настройка MCM. Ошибки всегда.

Переводы интерфейса: русский, английский, китайский. В поставке только MO;
редактируемые PO остаются в исходниках. Мод не заменяет ИИ прочих фракций.

ENGLISH

Experimental loaded-world mining, hauling and trading for one faction in the
Hub. Founding grants are explicit and one-time. Ongoing passive income and
cash-for-tech research are disabled. Real items are mined, carried and
transferred to the actual merchant inventory; the merchant pays the faction
from its real purse. Native town/trader pricing, inventory room, funds,
ownership, distance and actor condition are checked again before mutation.

Cargo and wallets are saved by Kenshi. The plugin saves faction metadata,
known resources and events in dynamicfactions.txt. On load, work is rebuilt
from actual inventory contents, including backpacks. It never replays payment
or creates cargo from a saved task counter. Displayed session revenue resets;
the real treasury does not. Legacy virtual ore stays preserved but inactive.

Confirmed rejection restores goods; confirmed unpaid trades can compensate
only when original balances and counts are proven. Unknown native outcomes
quarantine the work, including after saving/loading, rather than replaying an
uncertain mutation. Such a fault needs investigation or a prior healthy save;
this is not a guarantee of recovery from arbitrary engine corruption.

Work toggles immediately; disabling also stops saved native mining/movement
jobs of the faction's available workers. Enabled requires restart. Search
radius500-5000m; resources must be observed by members. Combat and recruiting
take priority; full carriers/unreachable targets use bounded retry. No virtual
production is active; a persistent distant economy is a later stage.

UI: ru_RU/en_GB/zh_CN. Detailed logging is off by default and last in MCM;
errors are always logged. CTRL+F10 opens the observation window. No public
Nexus/Steam release is prepared for this experimental build.

DEVELOPMENT REFERENCES

docs/superpowers/specs/2026-10-30-dynamic-factions-fair-simulation.md
docs/superpowers/plans/2026-10-30-dynamic-factions-economy-foundation.md
docs/dependencies/kenshilib-sdk-audit-2026-10-30.md
