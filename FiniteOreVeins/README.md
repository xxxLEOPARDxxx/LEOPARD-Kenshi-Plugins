# Finite Ore Veins

Первая локальная сборка для Kenshi 1.0.65 с RE_Kenshi, KenshiLib и Mod Config Menu. В карточке природной жилы появляется остаток и ёмкость. Уже добытую руду можно забирать штатным способом.

Природные жилы распознаются по происхождению из записей ландшафта в текущем наборе FCS-модов и по фактическому производству. Строящиеся шахты, глубокие бурильни, камень и сельское хозяйство не ограничиваются. Модовая руда поддерживается при обычном оформлении жилы через FOLIAGE_MESH.

В MCM можно изменить базовую ёмкость, влияние качества, задержку восстановления, время заполнения и порог повторного открытия. По умолчанию это 100 единиц на жилу качества 100%, 12 игровых часов без добычи, 10 игровых дней на полное заполнение и 25% для открытия. Время заполнения 0 делает истощение постоянным. Уже назначенные задания и перенос руды выполняются штатной логикой Kenshi.

Запас хранится в записи здания внутри зоны. Восстановление вычисляется по игровому времени при работе с жилой, открытии карточки или сохранении. В обычном сейве запись загружается вместе с зоной. После импорта состояние зависит от того, сохранил ли Kenshi нативную запись жилы. Можно отключить плагин и продолжить обычную добычу.

Строка остатка строится в ProductionBuilding::getGUIState. Assigned Workers наблюдает родительский ProductionBuilding::getGUIData, вызывает его штатное продолжение и добавляет своих рабочих отдельно. Обе точки подтверждены в Steam и GOG 1.0.65. Фактическая совместная проверка в игре ещё не проводилась.

## English

An offline local build for Kenshi 1.0.65, RE_Kenshi, KenshiLib and MCM. Natural ore veins have a limited reserve shown in their building panel. Buildable drills, farms and stone are outside the mechanic. Modded natural ores are recognised through their merged foliage placement records.

MCM controls base reserve, richness scaling, recovery delay, recovery duration, and the reopening threshold. Defaults are 100 units at 100% quality, a 12-game-hour delay, 10 game days to refill, and reopening at 25%. A refill duration of 0 makes depletion permanent. Existing mining and hauling jobs remain under Kenshi's native AI.

Reserve state persists in the vein's zone record. Recovery uses elapsed game time, including time spent outside that zone. Import follows the native zone record behaviour. Assigned Workers uses the parent panel hook, while this plugin adds the reserve in the native panel state method. The two paths have been checked offline on both executable variants.
