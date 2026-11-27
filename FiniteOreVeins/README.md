# Finite Ore Veins (HARD)

Первая локальная сборка для Kenshi 1.0.65 с RE_Kenshi, KenshiLib и Mod Config Menu. В карточке природной жилы появляется остаток и ёмкость. Уже добытую руду можно забирать штатным способом.

Природные жилы распознаются по происхождению из записей ландшафта в текущем наборе FCS-модов и по фактическому производству. Строящиеся шахты, глубокие бурильни, камень и сельское хозяйство не ограничиваются. Модовая руда поддерживается при обычном оформлении жилы через FOLIAGE_MESH.

Полная ёмкость - сумма случайных значений от 1500 до 3500 единиц на каждое доступное рабочее место. Например, жила на пять рабочих содержит от 7500 до 17500 единиц. Плагин использует максимальное число рабочих и положение жилы на карте, поэтому запас не меняется при смене смены, повторном выборе жилы или загрузке зоны. Качество руды по-прежнему влияет на штатную добычу.

При добыче последней единицы жила исчезает со звуком обрушения. Рабочие завершают добычу, а руда, оставшаяся в выходном слоте, выпадает рядом. В MCM есть две галочки: включение плагина и подробный журнал (по умолчанию выключен).

В записи здания внутри зоны хранится оставшаяся доля запаса. Сейвы предыдущей сборки сохраняют долю при переходе на новую ёмкость. Удаление проходит по штатному пути удаления здания. После импорта состав жил следует поведению Kenshi при импорте мира.

Остаток выводится рядом с содержанием руды и числом рабочих. Мод дополняет раздел ресурсов через ProductionBuilding::getGUIFertility. Assigned Workers вызывает штатный ProductionBuilding::getGUIData и добавляет своих рабочих отдельно. Всплывающая подсказка без области прокрутки пропускается.

## English

An offline local build for Kenshi 1.0.65, RE_Kenshi, KenshiLib and MCM. Natural ore veins have a limited reserve shown in their building panel. Buildable drills, farms and stone are outside the mechanic. Modded natural ores are recognised through their merged foliage placement records.

Each worker slot contributes a random 1500-3500 ore to the vein's capacity. A five-slot vein holds 7500-17500 ore. The maximum worker count and the vein's map position keep its capacity consistent across selection, staffing and zone reloads. Ore quality still affects native mining.

When the last unit is mined, the vein disappears with a debris crash sound. Miners stop working and any leftover output drops nearby. MCM has two toggles: enable the plugin and detailed logging (off by default).

The remaining share persists in the vein's zone record. Saves from earlier builds retain that share with the new capacity. Depletion uses Kenshi's native building removal path. World import follows Kenshi's native import behaviour. This plugin adds the reserve alongside the resource grade and worker count through ProductionBuilding::getGUIFertility. Assigned Workers uses the parent ProductionBuilding::getGUIData. Hover tooltips without a scroll view are skipped.
