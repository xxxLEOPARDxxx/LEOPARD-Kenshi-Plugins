SKILL LEVEL UP INFO

[Nexus — our release / наша версия](https://www.nexusmods.com/kenshi/mods/2257)

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 1 (29–30.10.2026)

  * Расчёт начала метки: кость головы в координатах мира минус позиция
    персонажа, плюс 0.4 по вертикали. Учитывается поза и размер модели;
    используются Bip01 Head, Head, head. Нет подходящей кости —
    безопасный запасной отступ 2.6. Поиск только при создании метки,
    до трёх запросов; постоянные проходы не опрашивают кости.

  * Уведомления о целых уровнях навыков своих персонажей: «Сила 25»,
    без дробных прибавок опыта и полос. Проверяются 33 базовых навыка
    без бонусов экипировки. Несколько уровней за проход дают итоговый
    уровень, одновременные повышения собраны в одну многострочную метку.
  * Восстановлен вид исходного xp_info: золотистый цвет (1, 0.84, 0.35),
    средний текст, подъём и плавное затухание. Штатная ScreenLabel
    использует RS_NORMAL и привязку к персонажу; время задаёт игра.
    Duration больше не используется. Оригинальный xp_info следует
    отключить, чтобы одно событие не показывалось дважды.
  * MCM -> Skill Level Up Info: Enabled, SelectedOnly и LogToGame.
    По умолчанию мод включён для всей фракции игрока, копия в игровой
    журнал выключена. SelectedOnly ограничивает показ выбранными,
    а исходные уровни сохраняются для всех своих героев. Копия в
    игровой журнал содержит имя героя и новые уровни.
  * Первый проход после включения, смены настроек, загрузки или новой
    игры только запоминает уровни. Старые повышения не всплывают
    повторно. Длительный перерыв в обновлении также сбрасывает кэш;
    ушедшие герои удаляются на ближайшем проходе.
  * Опрос не чаще раза в 250 мс. Без повышений не строятся строки
    уведомлений; кэш переиспользуется. Выключенный мод не читает навыки.
    Расчёт опыта, в том числе XP_Overhaul, не подменяется: плагин
    наблюдает уже полученный базовый уровень.
  * Все настройки MCM применяются сразу. Подробный журнал — последняя
    строка и по умолчанию выключен (Debug=0). Он пишет настройки,
    повышения и сброс исходных уровней; ошибки пишутся всегда.
    Личный INI сохраняется. Переводы ru_RU/en_GB/zh_CN; в выпуске .mo,
    исходные .po остаются в репозитории.

VERSION 1 CHANGES (29–30 OCTOBER 2026)

  * Label placement uses headWorld minus characterPosition, plus
    0.4 world Y. Uses Bip01 Head / Head / head with pose/model validation
    and a bounded 2.6 fallback. At most three bone queries per new label;
    stable scans make none. Native character tracking remains in use.

  * Whole skill-level notifications for player characters, such as
    "Strength 25", without fractional XP messages or bars. The plugin
    observes 33 base skills without equipment bonuses. Multiple levels
    between scans show the final level; simultaneous increases share
    one multiline label.
  * The original xp_info presentation is retained: gold (1, 0.84, 0.35),
    medium text, upward movement and a smooth fade. A native tracked
    ScreenLabel uses RS_NORMAL; the game controls its lifetime.
    Duration is no longer used. Disable the original xp_info to avoid
    duplicate notifications.
  * MCM -> Skill Level Up Info exposes Enabled, SelectedOnly and
    LogToGame. Defaults are enabled for the whole player faction, with
    game-log copies off. SelectedOnly limits announcements to selected
    characters while retaining baselines for everyone. Log copies
    include the character name and reached levels.
  * The first scan after enabling, settings changes, loading or a new
    game records baselines silently. A long update gap also resets the
    cache; departed characters are pruned on the next scan.
  * Scans run at most every 250 ms. Stable scans reuse state without
    building message text; disabled updates do not read skills.
    The plugin observes resulting base levels without replacing XP
    calculations, including when XP_Overhaul is active.
  * All MCM settings apply immediately. Detailed log is last and off
    by default (Debug=0), covering settings, increases and baseline
    resets. Errors are always logged. Updates preserve the player's
    INI. UI translations: ru_RU/en_GB/zh_CN; releases use .mo catalogs,
    while .po sources remain in the repository.

СПРАВОЧНИК

Показывает только достигнутый целый уровень навыка: золотистый текст
среднего размера поднимается над персонажем и плавно исчезает, как в
оригинальном xp_info. Используется тот же игровой тип метки и анимация.
Дробных прибавок опыта, полос и отдельного окна настроек нет.

MCM -> Skill Level Up Info: включение, только выбранные персонажи,
копия сообщения в игровой журнал, подробный журнал отладки.
Все настройки применяются сразу. Debug=0 по умолчанию; при Debug=1
в RE_Kenshi_log.txt записываются настройки, повышения и сброс исходных
уровней при загрузке. Duration больше не используется: время и затухание
задаёт исходная игровая анимация. Личный INI при обновлении сохраняется.

Первое наблюдение после загрузки задаёт исходные уровни без уведомлений.
Учитываются базовые навыки без бонусов экипировки. Несколько уровней за
один проход дают итоговый достигнутый уровень. Совместим с XP_Overhaul:
наблюдает результат, не подменяет расчёт опыта. Оригинальный xp_info
нужно отключить для исключения повторных сообщений.

Переводы: русский, английский, упрощённый китайский.

29.10.2026, проверка производительности: выключенный мод сразу выходит
из хука и не читает навыки. Включённый проверяет 33 базовых навыка каждого
живого героя не чаще четырёх раз в секунду; текст собирается только при
повышении целого уровня. Стабильный проход без повышений переиспользует
исходные уровни без временного набора персонажей и строковых потоков.
Ушедшие герои удаляются из кэша на ближайшем проходе; загрузка и новая
игра сбрасывают исходные уровни. Первый проход после включения молчит.
Подробный журнал — последняя настройка MCM, по умолчанию выключен.


ДВА ВИДА ЗАПИСЕЙ / TWO DIFFERENT OUTPUTS

«Повышения в журнале игры» (LogToGame) — необязательная копия имени героя
и достигнутых уровней в обычных сообщениях игры. «Подробный журнал» (Debug)
— техническая диагностика в RE_Kenshi_log.txt для поиска неисправностей.
Оба по умолчанию выключены и независимы; для всплывающих надписей ни один
не обязателен. Debug остаётся последним пунктом MCM.

“Level-ups in game log” (LogToGame) optionally copies the character name
and reached levels to the game's messages. “Detailed log” (Debug) writes
technical diagnostics to RE_Kenshi_log.txt. Both are independent and off
by default; neither is required for floating labels. Debug stays last.

Head-anchor verification: native ScreenLabel setTracking/update were read
from the running game, confirming that the offset is added in world space.
Nineteen regressions cover tall/short/prone poses, translation, invalid
or missing bones and no-event overhead. The game still owns rising, fade
and label destruction; the plugin keeps no live label/bone cache.


ВИЗУАЛЬНАЯ ПРОВЕРКА / VISUAL ACCEPTANCE (30.10.2026)

В сборке пользователя новая надпись начинается примерно от торса,
а не строго от головы. Пользователь принял текущий результат и попросил
оставить его. Точное совпадение с головой для всех моделей не заявляется.
In the user's mod list the label starts around the torso rather than
exactly at the head. The user accepted this result and requested no
further anchor changes. Exact head placement for every model is not claimed.
