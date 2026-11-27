# Finite Ore Veins (HARD)

Первая локальная сборка для Kenshi 1.0.65 с RE_Kenshi, KenshiLib и Mod Config Menu. В карточке природной жилы появляется остаток и ёмкость. Уже добытую руду можно забирать штатным способом.

Природные жилы распознаются по происхождению из записей ландшафта в текущем наборе FCS-модов и по фактическому производству. Строящиеся шахты, глубокие бурильни, камень и сельское хозяйство не ограничиваются. Модовая руда поддерживается при обычном оформлении жилы через FOLIAGE_MESH.

Полная ёмкость - сумма случайных значений от 1500 до 3500 единиц на каждое доступное рабочее место. Например, жила на пять рабочих содержит от 7500 до 17500 единиц. Плагин использует максимальное число рабочих и положение жилы на карте, поэтому запас не меняется при смене смены, повторном выборе жилы или загрузке зоны. Качество руды по-прежнему влияет на штатную добычу.

В MCM можно изменить задержку восстановления, время заполнения и порог повторного открытия. По умолчанию восстановление начинается через 48 игровых часов без добычи, полное заполнение занимает 365 игровых дней, а при пороге 0% жила открывается после восстановления первой единицы. Время заполнения 0 делает истощение постоянным. Уже назначенные задания и перенос руды выполняются штатной логикой Kenshi.

В записи здания внутри зоны хранится оставшаяся доля запаса. Сейвы предыдущей сборки сохраняют эту долю при переходе на новую ёмкость. Восстановление вычисляется по игровому времени при работе с жилой, открытии карточки или сохранении. В обычном сейве запись загружается вместе с зоной. После импорта состояние зависит от того, сохранил ли Kenshi нативную запись жилы. Можно отключить плагин и продолжить обычную добычу.

Остаток выводится рядом с содержанием руды и числом рабочих. Мод дополняет раздел ресурсов через ProductionBuilding::getGUIFertility. Assigned Workers вызывает штатный ProductionBuilding::getGUIData и добавляет своих рабочих отдельно. Всплывающая подсказка без области прокрутки пропускается.

## English

An offline local build for Kenshi 1.0.65, RE_Kenshi, KenshiLib and MCM. Natural ore veins have a limited reserve shown in their building panel. Buildable drills, farms and stone are outside the mechanic. Modded natural ores are recognised through their merged foliage placement records.

Each worker slot contributes a random 1500-3500 ore to the vein's capacity. A five-slot vein holds 7500-17500 ore. The maximum worker count and the vein's map position keep its capacity consistent across selection, staffing and zone reloads. Ore quality still affects native mining.

MCM controls recovery delay, recovery duration and the reopening threshold. By default, veins start recovering after 48 game hours without mining, take 365 game days to refill, and reopen after recovering the first unit at the 0% threshold. A refill duration of 0 makes depletion permanent. Existing mining and hauling jobs remain under Kenshi's native AI.

The remaining share persists in the vein's zone record. Saves from earlier builds keep that share when moving to the new capacity. Recovery uses elapsed game time, including time spent outside that zone. Import follows the native zone record behaviour. This plugin adds the reserve alongside the resource grade and worker count through ProductionBuilding::getGUIFertility. Assigned Workers uses the parent ProductionBuilding::getGUIData. Hover tooltips without a scroll view are skipped.
