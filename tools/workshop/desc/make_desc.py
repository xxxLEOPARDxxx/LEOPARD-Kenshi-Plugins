# -*- coding: utf-8 -*-
"""Описания для игроков (Steam / Nexus): workshop/desc/<Mod>.ru.txt и .en.txt.

README в папке мода - полный справочник (ключи ini, журнал, как устроено).
Здесь - только то, что нужно игроку: что делает, как вызвать, с чем
совместим, где настройки. Абзацы - одной строкой: Steam переносит строки
сам, а ручные переносы README рвали текст посередине.

Исключение - ModConfigMenu: у него раздел для авторов плагинов нужен, и
текст берётся из README как есть, только склеенный по абзацам.

    python make_desc.py
"""
import io
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import build  # noqa: E402


def settings_ru(page, mod, restart="«Включено» действует после перезапуска игры."):
    return ("[h2]НАСТРОЙКИ[/h2]\n\n"
            "Все настройки - прямо в игре, если установлен Mod Config Menu (не обязателен): окно "
            "«Настройки» → вкладка MCM → «%s». У каждой строки подсказка, изменения применяются "
            "сразу; %s Без MCM - файл %s.ini в папке мода, к каждой строке там есть пояснение.\n"
            % (page, restart, mod))


def settings_en(page, mod, restart="\"Enabled\" takes effect after a game restart."):
    return ("[h2]SETTINGS[/h2]\n\n"
            "Every setting is right in the game with Mod Config Menu installed (optional): Options "
            "window → MCM tab → \"%s\". Each row has a tooltip, changes apply at once; %s Without MCM - "
            "%s.ini in the mod folder, every line there is commented.\n" % (page, restart, mod))


# Форки модов Emkej (Emkej8 на Nexus): оригинал, автор и что мы переделали -
# в начале каждого описания. Nexus ID оригиналов - со страницы автора
# (nexusmods.com/profile/Emkej8/mods), 05.10.2026.
EMKEJ = {
    "Wall-B-Gone": ("Wall-B-Gone", 1863),
    "Auto-Pause-on-Load": ("Auto-Pause on Load", 1867),
    "Job-B-Gone": ("Job-B-Gone", 1870),
    "Loot-Scoot-Execute": ("Loot-Scoot-Execute", 1871),
    "Vital-Sense": ("Vital Sense", 1873),
    "Organize-the-Trader": ("Organize the Trader", 1886),
    "Map-markers": ("Map markers", 1887),
    "Organize-the-Crafting-Stations": ("Organize the Crafting Stations", 1900),
    "Vital-Read": ("Vital Read", 1910),
    "Hidden-Faction-Relations": ("Hidden Faction Relations", 1915),
    "Organize-the-Inventory": ("Organize the Inventory", 1936),
    "Container-Highlight": ("Container Highlight", 1953),
}


# Что ещё изменено в форке, кроме MCM и перевода (по README форков, раздел
# «Изменения форка»): (ru, en) - пункты списка.
FORK_EXTRA = {
    "Auto-Pause-on-Load": [(u"пауза снимается после торговли, даже если во время неё переключиться на другого персонажа",
                            "the pause is lifted after trading even if you switch to another character during the trade"),
                           (u"пауза и после переноса (импорта) сохранения - надёжно, когда мир уже догрузился",
                            "a pause after importing a save too - reliably, once the world has finished loading"),
                           (u"новая настройка: пауза, когда бьют персонажа из отряда, который сейчас не выбран; как часто - настраивается ползунком (по умолчанию не чаще раза в 30 секунд на отряд)",
                            "a new option: pause when a character of a squad that is not selected takes a hit; how often is a slider (by default at most once per squad in 30 seconds)")],
    "Wall-B-Gone": [("новая настройка «Разбирать любые свои постройки»: клавиша разбирает и хранилища, станки, дома (если они пусты и никто их не использует)",
                     "a new option \"Dismantle any own building\": the key also dismantles storage, workbenches, houses (if they are empty and unused)"),
                    ("к ней - «Выгружать вещи при разборке»: хранилище или станок с вещами тоже разбирается, вещи выкладываются на землю",
                     "with it - \"Unload items when dismantling\": a storage or workbench with items is dismantled too, the items are put on the ground"),
                    ("по желанию - вопрос перед разборкой: Enter - разобрать, Esc - отмена",
                     "optionally a question before dismantling: Enter - dismantle, Esc - cancel")],
    "Container-Highlight": [("исправлена утечка памяти: каждое нажатие ALT оставляло копии материалов подсветки",
                             "fixed a memory leak: every ALT press left copies of the highlight materials behind"),
                            ("меньше нагрузки: перехват отрисовки включается только для отладки",
                             "lower load: render interception is enabled only for debugging"),
                            ("ещё меньше нагрузки: значки и окраска контейнеров обновляются только при изменении, а не каждый кадр",
                             "even lower load: the markers and the container tint are updated only when they change, not every frame"),
                            ("исправлена утечка памяти при каждом поиске контейнеров",
                             "fixed a memory leak on every container scan"),
                            ("клавиша задаётся одним сочетанием (например CTRL+X) прямо на кнопке клавиши",
                             "the key is set as one combination (e.g. CTRL+X) right on the key button")],
    "Vital-Sense": [("значки больше не просвечивают сквозь окна интерфейса",
                     "markers no longer show through interface windows"),
                    ("окраска больше не попадает в портрет оглушённого",
                     "the tint no longer gets baked into the portrait of a knocked-out character"),
                    ("исправлена утечка памяти при подсветке животных",
                     "fixed a memory leak when highlighting animals"),
                    ("свой цвет у нейтралов - тех, кто вам не враг и не союзник",
                     "a colour of its own for neutrals - those who are neither your enemies nor allies"),
                    ("сколько значков на экране - настраивается; значки получают видимые персонажи, ближайшие к центру камеры (раньше у части видимых их не было)",
                     "the number of markers on screen is a setting; markers go to visible characters, the nearest to the camera centre first (before, some visible characters had none)"),
                    ("окраска целей с наградой - цветом тира награды, как значок $, а не цветом отношения",
                     "bounty targets are tinted in the colour of their bounty tier, like the $ symbol, not in the relation colour"),
                    ("уживается с Character Highlight: его цвета не перекрывают окраску",
                     "works together with Character Highlight: its colours do not override the tint"),
                    ("клавиша подсветки задаётся одним сочетанием (например CTRL+X) прямо на кнопке клавиши",
                     "the highlight key is set as one combination (e.g. CTRL+X) right on the key button")],
    "Vital-Read": [("значки больше не просвечивают сквозь окна интерфейса",
                    "icons no longer show through interface windows"),
                   ("значок «умирает» совпадает со статусом «Умирает» в самой игре, в том числе у скелетов",
                    "the dying icon matches the game's own Dying status, skeletons included"),
                   ("новые значки: голубой плюс у тех, кто лечится в кровати, и голубой ключ у скелета на ремонтной кровати",
                    "new icons: a blue plus for those resting in a bed and a blue wrench for a skeleton in a repair bed")],
    "Organize-the-Trader": [("поиск собирает найденные вещи в начало сетки торговца, без пустых рядов",
                             "search gathers the found items at the start of the trader grid, with no empty rows"),
                            ("меньше нагрузки: панели не ищутся по всему интерфейсу каждый кадр",
                             "lower load: the panels are no longer searched through the whole interface every frame"),
                            ("панели поиска и сортировки встроены в окно торговца и сами подстраиваются под его размер",
                             "the search and sort panels are built into the trader window and fit its size on their own"),
                            ("сортировка внизу окна работает на любом языке игры (подписи были только по-русски)",
                             "sorting at the bottom of the window works in any game language (its labels were Russian only)"),
                            ("список сортировки не обрезает текст на ванильном интерфейсе",
                             "the sort list no longer cuts its text with the vanilla interface"),
                            ("поиск и сортировка - в нижней строке окна, вместо ненужной кнопки «Разложить»; ширина списка и поля поиска настраиваются в MCM",
                             "search and sorting sit in the bottom row of the window instead of the needless Arrange button; the list and search field widths are set in MCM"),
                            ("рядом с деньгами торговца - итог сделки: сколько вы наторговали за это окно (можно выключить)",
                             "next to the trader's money - the deal balance: how much you made in this window (can be turned off)")],
    "Map-markers": [("окно метки шире, рисуется поверх значков карты, есть кнопка «Удалить»",
                     "the marker window is wider, drawn above the map icons and has a Delete button"),
                    ("в окне метки - крестик и выбор цвета подписи; подписи и кнопка «Метки» не обрезаются",
                     "the marker window has a close button and label colours; labels and the Markers button are not cut"),
                    ("меньше нагрузки: пока карта закрыта, мод ничего не делает, а открытая карта и много меток почти не снижают FPS",
                     "lower load: the mod does nothing while the map is closed, and the open map and many markers barely affect FPS")],
    "Loot-Scoot-Execute": [("переведены и сами кнопки «Добить» и «Добить всех»",
                            "the Execute and Execute All buttons themselves are translated too"),
                           ("«Добить всех» несколькими выделенными: каждый к своей цели, сначала ближайшие; любой приказ отменяет добивание",
                            "Execute All with several selected characters: each to their own target, nearest first; any order cancels it"),
                           ("«Добить» и «Добить всех» - пункты самого контекстного меню игры, а не отдельная плашка",
                            "Execute and Execute All are options of the game's own context menu, not a separate panel")],
    "Job-B-Gone": [("меньше нагрузки: подписи панели обновляются только при изменении",
                    "lower load: panel captions are updated only when they change"),
                   ("панель помнит, что её спрятали горячей клавишей (CTRL+B): после перезапуска игры остаётся спрятанной",
                    "the panel remembers that it was hidden with the hotkey (CTRL+B): it stays hidden after a game restart"),
                   ("клавиша панели в Mod Config Menu - одно сочетание (CTRL+B) на кнопке клавиши, без отдельных галок Ctrl/Alt/Shift",
                    "the panel key in Mod Config Menu is one combination (CTRL+B) on the key button, no separate Ctrl/Alt/Shift ticks"),
                   ("меньше нагрузки: панель не переставляется каждый кадр, когда ничего не меняется",
                    "lower load: the panel is not re-laid out every frame when nothing changes"),
                   ("надписи панели - цветом её кнопок: раньше тёмный текст на тёмной рамке",
                    "panel labels in the colour of its buttons: it used to be dark text on a dark frame"),
                   ("режим подтверждения переведён на язык игры",
                    "the confirmation mode follows the game language")],
    "Organize-the-Crafting-Stations": [("меньше нагрузки: панель поиска не ищется по всему интерфейсу каждый кадр",
                                        "lower load: the search panel is no longer searched through the whole interface every frame"),
                                       ("подсказка в пустом поле поиска - на языке игры",
                                        "the hint in the empty search field is in the game language")],
}


def fork_header(mod, lang):
    """Шапка описания форка: оригинал и автор, что изменено, исходники (GPLv3)."""
    name, nexus_id = EMKEJ[mod]
    orig = "[url=https://www.nexusmods.com/kenshi/mods/%d]%s[/url]" % (nexus_id, name)
    git = "[url=https://github.com/Emkej/%s]GitHub[/url]" % mod
    k = 0 if lang == "ru" else 1
    extra = "".join("[*]%s;\n" % e[k] for e in FORK_EXTRA.get(mod, []))
    if lang == "ru":
        return ("Форк мода %s от [b]Emkej8[/b] (исходники оригинала - %s). Спасибо автору за мод!\n\n"
                "[b]Что изменено в этой версии:[/b]\n[list]\n"
                "[*]настройки - в Mod Config Menu (вкладка MCM в окне «Настройки» игры) вместо Emkej's Mod Hub;\n"
                "[*]перевод на русский и китайский;\n%s[/list]\n"
                "Лицензия GNU GPL v3.\n\n"
                % (orig, git, extra))
    return ("A fork of %s by [b]Emkej8[/b] (original source on %s). Thanks to the author for the mod!\n\n"
            "[b]What is changed in this version:[/b]\n[list]\n"
            "[*]settings in Mod Config Menu (the MCM tab in the game's Options window) instead of Emkej's Mod Hub;\n"
            "[*]Russian and Chinese translations;\n%s[/list]\n"
            "License GNU GPL v3.\n\n"
            % (orig, git, extra))


# Форки других авторов: мод -> (название, ссылка на оригинал, автор,
# что изменено ru, en). У BetterLooting страница в Steam снята автором -
# ссылка на исходники на GitHub.
OTHER = {
    "BetterLooting": ("Better Looting", "https://github.com/XxAtreuSSxX/BetterLooting", "XxAtreuSSxX",
                      [("настройки - в Mod Config Menu (вкладка MCM в окне «Настройки» игры) вместо отдельного окна; профили и «все / ни одной категории» - там же",
                        "settings in Mod Config Menu (the MCM tab in the game's Options window) instead of the separate settings window; profiles and all / no categories are there too"),
                       ("текст - на языке игры: английский, русский, китайский",
                        "text follows the game language: English, Russian, Chinese"),
                       ("в журнал пишутся только сбои, подробности - по желанию",
                        "only failures go to the log, details on request"),
                       ("краденым считается только взятое у живого без сознания, а не с мёртвых и животных",
                        "only things taken from a living unconscious person count as stolen, not from the dead or animals"),
                       ("исправлен вылет после сбора с модами, которые кладут подобранное в рюкзак (например, KEO)",
                        "fixed a crash after looting with mods that put picked-up items into the backpack (like KEO)"),
                       ("вернулась раскладка по хранилищам (у автора - кнопка в убранном окне настроек), теперь клавишей ALT+INSERT",
                        "deposit to storage is back (a button in the author's removed settings window), now on ALT+INSERT")]),
    "Character-Highlight": ("Character Highlight", "https://github.com/BFrizzleFoShizzle/KenshiLib_Examples", "BFrizzleFoShizzle",
                            [("сильно оптимизирован: оригинал нагружал игру каждый кадр, даже когда клавиша не зажата; здесь без клавиши мод не делает ничего, а с ней - в разы меньше работы",
                              "heavily optimised: the original loaded the game every frame, even with the key not held; here the mod does nothing without the key and far less with it"),
                             ("настройки в Mod Config Menu: клавиша, кого окрашивать, цвета, дальность, видно ли сквозь стены",
                              "settings in Mod Config Menu: the key, who is tinted, colours, distance, see through walls"),
                             ("новая категория - дипломаты фракций, в том числе из модов, своим цветом (по умолчанию фиолетовым)",
                              "a new kind - faction diplomats, mods' ones included, in a colour of their own (purple by default)"),
                             ("каждую категорию (свой отряд, враги, союзники, нейтралы) можно выключить",
                              "each kind (squad, enemies, allies, neutrals) can be turned off"),
                             ("текст - на языке игры: английский, русский, китайский",
                              "text follows the game language: English, Russian, Chinese"),
                             ("исправлен возможный вылет на персонаже без фракции",
                              "fixed a possible crash on a character without a faction")]),
    "Formations": ("Formations", "https://steamcommunity.com/sharedfiles/filedetails/?id=3795714050", "Slinkysalmon03",
                   [("строй держится и на бегу за ведущим: 11 новых фигур в списке «Следовать строем» на вкладке ИИ, а в Mod Config Menu - своя фигура для каждого отряда; выбор хранится в сейве",
                     "the formation holds while following the leader too: 11 new shapes in the AI tab's \"Follow Formation\" list, and a shape of its own for every squad in Mod Config Menu; kept in the save"),
                    ("те же фигуры и для перетаскивания: каре, ромб, колонна по двое, клещи, алмаз, мотылёк, шеврон",
                     "the same shapes for the drag: square, rhombus, column of two, pincers, diamond, moth, chevron"),
                    ("модификатор - любая клавиша или сочетание, а не только ALT/CTRL/SHIFT",
                     "the modifier is any key or combination, not only ALT/CTRL/SHIFT"),
                    ("«Держать» после построения можно выключить",
                     "Hold after forming up can be turned off"),
                    ("настройки в Mod Config Menu, текст - на языке игры: английский, русский, китайский",
                     "settings in Mod Config Menu, text follows the game language: English, Russian, Chinese")]),
    "KenshiJobManagement": ("Kenshi Job Management", "https://github.com/SirixKC/KenshiJobManagement", "SirixKC",
                            [("окно нарисовано родными рамками, кнопками, шрифтами и цветами игры - выглядит как часть игры и с ванильным интерфейсом, и с модами на интерфейс",
                              "the window uses the game's own frames, buttons, fonts and colours - it looks like part of the game with the vanilla interface and with interface mods"),
                             ("текст и кнопки крупнее, размер окна настраивается",
                              "larger text and buttons, adjustable window size"),
                             ("перевод на русский и китайский",
                              "Russian and Chinese translations"),
                             ("настройки - в Mod Config Menu: включение, клавиша окна, кнопка на панели, размер",
                              "settings in Mod Config Menu: on/off, the open key, the panel button, the size"),
                             ("колесо мыши во вкладке станций и в окне станции прокручивает по строке, а не сразу в конец списка",
                              "the mouse wheel on the Stations tab and in the station window scrolls row by row instead of jumping to the end"),
                             ("новые, более чёткие значки станций, задач и кнопки на панели",
                              "new, sharper icons for stations, jobs and the panel button"),
                             ("проверка на утечки и нагрузку, чистка кода",
                              "checked for leaks and load, code cleanup")]),
}


def other_header(mod, lang):
    """Шапка форка не от Emkej: оригинал и автор, что изменено, исходники."""
    name, url, author, changes = OTHER[mod]
    k = 0 if lang == "ru" else 1
    items = "".join("[*]%s;\n" % e[k] for e in changes)
    where_ru, where_en = (("оригинал - [url=%s]в Мастерской Steam[/url], исходники идут с ним" % url,
                           "the original is [url=%s]on the Steam Workshop[/url], its source ships with it" % url)
                          if "steamcommunity" in url else
                          ("исходники оригинала - [url=%s]GitHub[/url]" % url, "original source on [url=%s]GitHub[/url]" % url))
    if lang == "ru":
        return ("Форк мода %s от [b]%s[/b] (%s). Спасибо автору за мод!\n\n"
                "[b]Что изменено в этой версии:[/b]\n[list]\n%s[/list]\n"
                "Не включайте вместе с оригиналом. Лицензия GNU GPL v3.\n\n" % (name, author, where_ru, items))
    return ("A fork of %s by [b]%s[/b] (%s). Thanks to the author for the mod!\n\n"
            "[b]What is changed in this version:[/b]\n[list]\n%s[/list]\n"
            "Do not enable it together with the original. License GNU GPL v3.\n\n" % (name, author, where_en, items))


def fork_settings_ru(page):
    return ("[h2]НАСТРОЙКИ[/h2]\n\n"
            "Все настройки - прямо в игре, если установлен Mod Config Menu (не обязателен): окно «Настройки» → "
            "вкладка MCM → «%s». У каждой строки подсказка, изменения применяются сразу. Без MCM - файл "
            "mod-config.json в папке мода.\n" % page)


def fork_settings_en(page):
    return ("[h2]SETTINGS[/h2]\n\n"
            "Every setting is right in the game with Mod Config Menu installed (optional): Options window → MCM "
            "tab → \"%s\". Each row has a tooltip, changes apply at once. Without MCM - mod-config.json in the mod "
            "folder.\n" % page)


T = {}

# ---------------------------------------------------------------- LiveXpBars
T["LiveXpBars"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Навык начал расти - на экране появилась его строка: название, уровень, процент до следующего уровня и полоса. Перестал расти - строка плавно погасла. Ничего не нужно выбирать и никуда нажимать.

[list]
[*][b]не качается ничего[/b] - на экране пусто;
[*][b]по желанию[/b] строки не гаснут, пока выделен тот же персонаж;
[*][b]выделено несколько персонажей[/b] - полос нет, чтобы не было каши;
[*][b]не мешают:[/b] сквозь полосы кликается по миру, по F7 они прячутся вместе с интерфейсом.
[/list]

Уровень, проценты и заливка окрашены по одной шкале: от тусклого кирпича на низких значениях до приглушённой зелени у сотни и фиолетового выше сотни.

[h2]ПОДЛОЖКА[/h2]

На светлой земле текст читается хуже - под полосы можно подложить фон: игровую панель с рамкой, как у описаний вещей, или ровную заливку своим цветом, с настраиваемой прозрачностью. По умолчанию подложки нет.

[h2]МЕСТО НА ЭКРАНЕ[/h2]

SHIFT+B показывает рамку с образцами строк - перетащите её куда нужно и нажмите SHIFT+B ещё раз. Место запомнится. Ширину строк, расстояние между ними, ширину колонки названия (насколько близко к нему уровень) и промежутки между колонками можно задать в настройках.

""" + settings_ru("Live Xp Bars", "LiveXpBars") + """
[h2]ЗАЧЕМ, ЕСЛИ ЕСТЬ XpBars[/h2]

XpBars (DougTownsend) тоже рисует полосы опыта, но навыки там отмечаются вручную в отдельном окне. Здесь выбирать ничего не нужно - показывается то, что растёт. Включать оба мода сразу незачем.
""", """[h2]WHAT IT DOES[/h2]

A skill starts growing - its line appears on screen: name, level, percent to the next level and a bar. It stops growing - the line fades out. Nothing to pick, nothing to click.

[list]
[*][b]nothing is levelling[/b] - the screen is clean;
[*][b]optionally[/b] the lines stay while the same character is selected;
[*][b]several characters selected[/b] - no bars, to avoid a mess;
[*][b]out of the way:[/b] clicks go through the bars to the world, F7 hides them with the interface.
[/list]

Level, percent and fill share one colour scale: dull brick for low values, muted green near a hundred, purple above it.

[h2]BACKGROUND[/h2]

On bright ground the text is harder to read - you can put a background under the bars: the game's framed panel, like item tooltips, or a flat fill in your own colour, with adjustable opacity. No background by default.

[h2]POSITION ON SCREEN[/h2]

SHIFT+B shows a frame with sample lines - drag it where you want and press SHIFT+B again. The position is remembered. The row width, the space between rows, the width of the name column (how close the level is to the name) and the space between columns are set in the settings.

""" + settings_en("Live Xp Bars", "LiveXpBars") + """
[h2]WHY, IF THERE IS XpBars[/h2]

XpBars (DougTownsend) also draws XP bars, but there you tick the skills by hand in a separate window. Here there is nothing to pick - whatever grows is shown. No need to run both.
""")

# --------------------------------------------------------------- StatColours
T["StatColours"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

[list]
[*][b]Цвет по величине.[/b] Числа характеристик и навыков окрашены от тусклого кирпича у нуля до приглушённой зелени у сотни; выше сотни (с прибавкой от снаряжения) - фиолетовым.
[*][b]Поправка рядом с числом[/b] - «(+8) 1»: всё, что сейчас прибавлено к навыку или отнято от него - снаряжение, увечья, голод, вес ноши, бой в помещении, боевая стойка у защиты. Прибавка зелёная, штраф красный.
[*][b]Из чего сложилась поправка.[/b] Щёлкните навык - рядом с описанием появится роспись: навык, протезы, снаряжение, увечья, а у боевых навыков ещё бой в помещении, боевая стойка и вес ноши - и итоговое значение. Строки всегда складываются в итог. Все числа - у самой игры.
[*][b]Множители опыта расы.[/b] С зажатым ALT в окне характеристик видно, какие навыки у этой расы качаются быстрее (зелёным) и медленнее (красным).
[*][b]Множители расы у кандидата в найм.[/b] Выделите в мире персонажа, которого можно нанять, и зажмите ALT: над плашкой с его именем появятся множители опыта его расы. Удобно перед наймом, когда рас много и всех не упомнишь.
[*][b]Скорость прокачки.[/b] В описании навыка - «(Тек. скор. прокачки: 0,510)»: во сколько раз сейчас умножается опыт с учётом расы и уровня. Чем выше навык, тем меньше число.
[/list]

Служебные цвета подстраиваются под тему интерфейса, если стоит мод на интерфейс. Все ступени шкалы можно поменять.

""" + settings_ru("Stat Colours", "StatColours", "«Включено» действует после перезапуска игры, остальное - сразу.") + """
[h2]СОВМЕСТИМОСТЬ[/h2]

Строки KillCounter («Убийств», «Нокаутов», «Отрублено») тоже окрашиваются. С XP_Overhaul скорость прокачки считается по его кривой, если его версия это поддерживает; иначе - по кривой игры.
""", """[h2]WHAT IT DOES[/h2]

[list]
[*][b]Colour by value.[/b] Attribute and skill numbers go from dull brick at zero to muted green at a hundred; above a hundred (with a gear bonus) - purple.
[*][b]Modifier next to the number[/b] - "(+8) 1": everything currently added to or taken from the skill - gear, injuries, hunger, carried weight, fighting indoors, combat stance for defence. Bonus in green, penalty in red.
[*][b]Where the modifier comes from.[/b] Click a skill - a breakdown appears next to the description: the skill, robotic limbs, gear, injuries, and for combat skills also fighting indoors, combat stance and carried weight - and the final value. The rows always add up to it. Every number comes from the game itself.
[*][b]Racial XP multipliers.[/b] Hold ALT in the stats window to see which skills this race levels faster (green) and slower (red).
[*][b]Racial multipliers of a recruit.[/b] Select someone in the world who can be recruited and hold ALT: their race's XP multipliers appear above the name plate. Handy before recruiting, when there are too many races to remember.
[*][b]Levelling speed.[/b] The skill description shows "(Current XP rate: 0.510)": how much the skill's XP is multiplied right now, by race and level. The higher the skill, the lower the number.
[/list]

Helper colours follow the interface theme if you use an interface mod. Every step of the scale can be changed.

""" + settings_en("Stat Colours", "StatColours", "\"Enabled\" takes effect after a game restart, the rest at once.") + """
[h2]COMPATIBILITY[/h2]

KillCounter lines ("Kills", "Knockouts", "Limbs severed") are coloured too. With XP_Overhaul the levelling speed follows its curve if its version supports that; otherwise the game's curve.
""")

# --------------------------------------------------------------- GearCompare
T["GearCompare"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Наведите курсор на броню или оружие - в инвентаре, в рюкзаке, в магазине - и зажмите ALT. Рядом с описанием вещи появится вторая панель: что сейчас надето в том же слоте и чем вещь под курсором отличается.

По каждому отличающемуся свойству - «надето → станет → разница»: зелёным, если станет лучше, красным - если хуже. Одинаковые свойства не показываются. Разница везде одна: «станет» минус «надето». Для брони показывается и эффективность сопротивления разрезу, для оружия - пробитие доспеха, длина и урон против рас (роботы, люди, звери, скелеты, ульи...).

Внизу - итог по трём осям и вердикт:

[list]
[*][b]защита (с покрытием)[/b] - с учётом того, какие части тела закрыты; у оружия - [b]урон (рез. + дроб.)[/b], в процентах к надетому;
[*][b]свойства[/b] - счёт строк: «5 лучше / 4 хуже»;
[*][b]вес[/b] - сколько прибавится;
[*][b]итог[/b] - лучше, хуже, смешанно или равноценно.
[/list]

[h2]С ЧЕМ СРАВНИВАЕТ[/h2]

[list]
[*][b]броню[/b] - с надетой в том же слоте;
[*][b]оружие[/b] - с надетым оружием того же типа, иначе с оружием в слоте, куда вещь встала бы, а если он свободен - с оружием в соседнем слоте;
[*][b]пустой слот[/b] - панели нет (по желанию можно сравнивать и с «ничем»);
[*][b]чьё надетое[/b] - персонажа, у которого лежит вещь, а в магазине, сундуке или на земле - выделенного.
[/list]

Клавишу можно сменить на CTRL или SHIFT, либо показывать панель всегда.

[h2]СРАВНЕНИЕ В РЕМЕСЛЕ[/h2]

Выбираете, что ковать, - и не знаете, что лучше? Во вкладке «Ремесло» нажмите ПКМ по строке списка «Предметы» - она подсветится зелёным (A). ПКМ по другой строке - синим (B), и рядом со списком появится та же панель: каждое свойство A и B, разница и вердикт «B против A».

[list]
[*][b]броня и протезы[/b] - в мастерском качестве, лучшем, что можно сделать;
[*][b]оружие[/b] - ровно той модели, что написана в строке (у оружия модели - отдельные строки);
[*][b]протезы[/b] - прочность, урон без оружия и множители навыков;
[*]ПКМ по помеченной строке снимает пометку, по третьей - заменяет B; закрыли окно - пометки сняты.
[/list]

Работает и с поиском по списку из Organize the Crafting Stations: подсветка остаётся на своей вещи.

""" + settings_ru("Gear Compare Alt", "GearCompare") + """
[h2]СОВМЕСТИМОСТЬ[/h2]

Окошко описания игры плагин не трогает, поэтому уживается с LtEast's Rarity Backgrounds и KEP.
""", """[h2]WHAT IT DOES[/h2]

Hover armour or a weapon - in the inventory, a backpack, a shop - and hold ALT. A second panel appears next to the item tooltip: what is worn in that slot now and how the hovered item differs.

For every property that differs - "worn → new → difference": green if it gets better, red if worse. Equal properties are not shown. The difference is always "new" minus "worn". Armour also shows cut resistance efficiency, weapons - armour penetration, length and damage against races (robots, humans, animals, skeletons, hives...).

At the bottom - a summary on three axes and a verdict:

[list]
[*][b]protection (with coverage)[/b] - with the covered body parts taken into account; for weapons - [b]damage (cut + blunt)[/b], in percent of the worn one;
[*][b]properties[/b] - a count of rows: "5 better / 4 worse";
[*][b]weight[/b] - how much is added;
[*][b]verdict[/b] - better, worse, mixed or equal.
[/list]

[h2]WHAT IT COMPARES WITH[/h2]

[list]
[*][b]armour[/b] - with what is worn in the same slot;
[*][b]weapons[/b] - with a worn weapon of the same type, otherwise with the weapon in the slot the item would go to, and if that slot is free - with the weapon in the other slot;
[*][b]empty slot[/b] - no panel (comparing with "nothing" can be turned on);
[*][b]whose gear[/b] - the character who carries the item; in a shop, chest or on the ground - the selected one.
[/list]

The key can be changed to CTRL or SHIFT, or the panel can be always on.

[h2]COMPARING ON THE CRAFTING TAB[/h2]

Not sure what to craft? On the Crafting tab, right-click a line of the Items list - it lights up green (A). Right-click another line - blue (B), and the same panel appears next to the list: every property of A and B, the difference and a verdict "B against A".

[list]
[*][b]armour and robotic limbs[/b] - at masterwork grade, the best that can be made;
[*][b]weapons[/b] - at exactly the model written in the line (weapon models are separate lines);
[*][b]limbs[/b] - durability, unarmed damage and skill multipliers;
[*]right-click a marked line to unmark it, a third line replaces B; close the window - the marks are gone.
[/list]

Works with the list search from Organize the Crafting Stations: the highlight stays on its item.

""" + settings_en("Gear Compare Alt", "GearCompare") + """
[h2]COMPATIBILITY[/h2]

The game's tooltip is left untouched, so it works with LtEast's Rarity Backgrounds and KEP.
""")

# ----------------------------------------------------------- AssignedWorkers
T["AssignedWorkers"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Выделите машину, залежь руды, сундук, исследовательский стол, турель - любую постройку, с которой работают, - и зажмите ALT. Рядом с окном постройки появится панель:

[code]Закреплены
Маус     приоритет 2, работает сейчас
Колби    приоритет 1[/code]

[list]
[*][b]кто[/b] из ваших персонажей закреплён за постройкой;
[*][b]приоритет[/b] - каким номером эта работа стоит в его списке работ;
[*][b]«работает сейчас»[/b] - он у этой постройки прямо сейчас.
[/list]

В обычной игре видно только «Рабочие: 5/5», но не кто именно. На большой базе, чтобы это выяснить, приходится открывать список работ у каждого по очереди.

Показываются работы, закреплённые за постройкой («Работа с машиной» и подобные); «Медик» или «Инженер» ни к какой постройке не привязаны. У своей постройки без работников панель пишет «никто».

Клавишу можно сменить на CTRL или SHIFT, либо показывать панель всегда. Работает с любым модом на интерфейс.

[h2]ВЫДЕЛИТЬ СВОБОДНЫХ[/h2]

CTRL+SHIFT+I выделяет разом всех ваших персонажей, которым сейчас нечего делать: «Цель: Нет цели» и никакого действия. Вверху экрана - сколько нашлось. Удобно на большой базе: сразу видно, кого занять. По умолчанию - только тех, кто не приписан ни к станку или машине; по желанию - и приписанных, но простаивающих (станку нечего делать, место занято).

""" + settings_ru("Assigned Workers", "AssignedWorkers"), """[h2]WHAT IT DOES[/h2]

Select a machine, ore deposit, storage, research bench, turret - any building people work at - and hold ALT. A panel appears next to the building window:

[code]Assigned
Maus     priority 2, working now
Colby    priority 1[/code]

[list]
[*][b]who[/b] of your characters is assigned to the building;
[*][b]priority[/b] - the number of this job in their job list;
[*][b]"working now"[/b] - they are at this building right now.
[/list]

The vanilla game only shows "Workers: 5/5", not who. On a big base you have to open every character's job list to find out.

Jobs bound to a building are shown ("Operate machinery" and the like); "Medic" or "Engineer" are not bound to any building. On your own building with nobody assigned the panel says "nobody".

The key can be changed to CTRL or SHIFT, or the panel can be always on. Works with any interface mod.

[h2]SELECT IDLE CHARACTERS[/h2]

CTRL+SHIFT+I selects all of your characters who have nothing to do right now: "Goal: No goal" and no action. A notice at the top of the screen shows how many were found. Handy on a big base: you see at once whom to put to work. By default only those not assigned to a bench or machine; optionally also assigned ones standing idle (nothing to craft, the spot is taken).

""" + settings_en("Assigned Workers", "AssignedWorkers"))

# -------------------------------------------------------------- UITweaks
T["UITweaks"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Эти части интерфейса игра строит своим кодом, поэтому ни один мод на интерфейс их не меняет. Каждая доработка включается отдельно.

[list]
[*][b]Строки очередей исследований и ремесла выше[/b] - на русских шрифтах текст больше не налезает сам на себя.
[*][b]Полоса в строке очереди - во всю рамку.[/b] У игры заливка не доходила до конца строки.
[*][b]Прокрутка в описании на вкладках «Изучение» и «Ремесло»[/b] - длинные описания (у доспехов) теперь помещаются.
[*][b]Ячейки отрядов на вкладке «Отряды» выше[/b] - портреты больше не обрезаны на полутора рядах; по желанию ячейки растут под самый большой отряд.
[*][b]Полоса «Следующий уровень» в описании навыка[/b] - аккуратная рамка, заливка внутри каймы, подпись по центру.
[*][b]Отношение фракции у имени выбранного[/b] - «Кочевники (+15)», как в окне фракций: зелёным выше нуля, красным от -10 и ниже; только у персонажей, по желанию - пока зажата клавиша.
[*][b]Иконка вещи в списке ремесла[/b] - зажмите ALT над рецептом во вкладке «Ремесло» - видно, как выглядит вещь (клавишу можно сменить или отключить в Mod Config Menu).
[*][b]Деньги торговца по центру окна[/b] - с интерфейсами, где строка денег растянута на всё окно (как Dark UI); в ванильном окне ничего не меняется.
[*][b]Щелчок ПКМ по окну торговли не уходит в мир[/b] - в игре щелчок по своему рюкзаку в окне торговли отправлял персонажа в точку под окном, и после закрытия окна он туда бежал. Исправление работает всегда.
[*][b]Кнопка «Случайно» у имени в редакторе персонажа[/b] - при старте новой игры, найме или у хирурга: случайное имя из списков игры, мужское или женское по персонажу. Моды, расширяющие списки имён (например, Expanded Name Set), тоже учитываются.
[*][b]Журнал вкладки «Диалоги» на тёмном интерфейсе[/b] - реплики NPC больше не чёрные на чёрном.
[*][b]Больше кадров с интерфейсом[/b] - игра каждый кадр двигает подписи над персонажами на то же место, и интерфейс каждый раз их все перерисовывал. Такие сдвиги пропускаются; подписи двигаются как обычно.
[/list]

[h2]С КАКИМ ИНТЕРФЕЙСОМ РАБОТАЕТ[/h2]

Раньше мод назывался Dark Ui Tweaks - название сбивало с толку, переименован в UI Tweaks. С любым - с ванильным и с модами на интерфейс: плагин не заменяет файлов разметки и скинов, а правит размеры прямо в игре. Если мод на интерфейс рисует рамки иначе, отступы полос подгоняются в настройках. Скрытые окна плагин не трогает, так что на производительность почти не влияет.

""" + settings_ru("UI Tweaks", "UITweaks",
                  "«Включено», прокрутка описаний и кнопка случайного имени действуют после перезапуска игры."), """[h2]WHAT IT DOES[/h2]

The game builds these parts of the interface in code, so no interface mod can change them. Each fix can be turned on and off separately.

[list]
[*][b]Taller research and crafting queue rows[/b] - text no longer overlaps itself with larger fonts.
[*][b]Full-width queue bars.[/b] In the game the fill never reached the end of the row.
[*][b]Scrolling descriptions on the Research and Crafting tabs[/b] - long descriptions (armour) now fit.
[*][b]Taller squad cells on the Squads tab[/b] - portraits are no longer cut at a row and a half; optionally the cells grow to the largest squad.
[*][b]A proper "Next level" bar in the skill description[/b] - a neat frame, fill inside the border, centred caption.
[*][b]Faction relation by the selected character's name[/b] - "Nomads (+15)", like in the factions window: green above zero, red from -10 down; characters only, optionally while a key is held.
[*][b]Item icon in the crafting list[/b] - hold ALT over a recipe on the Crafting tab to see what the item looks like (the key can be changed or turned off in Mod Config Menu).
[*][b]The trader's money at the centre of the window[/b] - with interfaces that stretch the money line across the whole window (like Dark UI); the vanilla window stays as it is.
[*][b]A right click on the trade window stays in the window[/b] - in the game a click on your own backpack in the trade window sent the character to the point under the window, and they ran there once it was closed. This fix is always on.
[*][b]A "Random" button by the name in the character editor[/b] - at a new game start, when recruiting or at the surgeon: a random name from the game's name lists, male or female by the character. Mods that extend the name lists (like Expanded Name Set) are used too.
[*][b]The Dialogues tab log on a dark interface[/b] - NPC lines are no longer black on black.
[*][b]More frames with the interface shown[/b] - every frame the game moves the names above characters to the same place, and the interface redrew all of them each time. Such moves are skipped; the names move as usual.
[/list]

[h2]WHICH INTERFACE IT WORKS WITH[/h2]

Formerly called Dark Ui Tweaks - the name was misleading, so it is now UI Tweaks. Any - vanilla and interface mods: the plugin replaces no layout or skin files, it fixes sizes in the running game. If an interface mod draws frames differently, the bar insets can be adjusted in the settings. Hidden windows are skipped, so the performance cost is negligible.

""" + settings_en("UI Tweaks", "UITweaks",
                  "\"Enabled\", description scrolling and the random name button take effect after a game restart."))

# ---------------------------------------------------------------- BestMinimap
T["BestMinimap"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Миникарта в углу экрана - на той же картинке, что большая карта игры (моды на карту подхватываются сами).

[list]
[*][b]Персонажи рядом[/b] - точками: ваш отряд, союзники, нейтралы, враги, звери; выделенные крупнее, носики показывают, куда смотрят.
[*][b]Поселения и места[/b], которые вы уже открыли, - значками большой карты, с названием; при наведении - фракция и отношения.
[*][b]Тревога[/b]: если ваших атакуют - пульсирующее кольцо на месте атаки (за краем обзора - у края) и строка в сообщениях игры.
[*][b]Метки[/b] Map-markers и цели WantedMap, если эти моды стоят; за краем обзора метки прижимаются к краю и показывают направление.
[*][b]Предел камеры[/b] - тонкая линия вокруг ваших персонажей: дальше камеру не отвести.
[*][b]ПКМ по миникарте[/b] - выделенные идут в эту точку; [b]двойной щелчок[/b] - камера переносится туда, если дотягивается.
[/list]

[h2]УПРАВЛЕНИЕ[/h2]

[list]
[*]CTRL+M - показать / скрыть;
[*]= и - (или колесо мыши над миникартой) - масштаб, до всей карты;
[*]CTRL+= и CTRL+- (или CTRL+колесо) - размер самой миникарты;
[*]правая кнопка по миникарте - выделенные идут в эту точку;
[*]двойной щелчок по миникарте - камера переносится в эту точку, если игра пускает её так далеко от ваших персонажей (до линии предела камеры);
[*]левая кнопка - перетащить миникарту, за угол - изменить размер.
[/list]

Форма (круг, квадрат, прямоугольник), рамка, поворот за камерой, кого показывать и цвета - в настройках. Место, размер и масштаб запоминаются. Пока открыта большая карта или спрятан интерфейс (F7), миникарта спрятана.

""" + settings_ru("Best Minimap", "BestMinimap"), """[h2]WHAT IT DOES[/h2]

A minimap in the corner of the screen, on the same picture as the game's big map (map mods are picked up by themselves).

[list]
[*][b]Characters nearby[/b] as dots: your squad, allies, neutrals, enemies, animals; the selected ones are bigger, small noses show where they look.
[*][b]Towns and places[/b] you have discovered, with the big map's icons and names; hover for the faction and relations.
[*][b]Alert[/b]: when your people are attacked - a pulsing ring where it happens (at the edge if out of view) and a line in the game's messages.
[*][b]Marks[/b] of Map-markers and WantedMap targets, if those mods are installed; out of view they stick to the edge and point the way.
[*][b]Camera range[/b] - a thin line around your characters: the camera cannot go farther.
[*][b]Right click on the minimap[/b] - the selected go there; [b]double click[/b] - the camera moves there, if it can reach.
[/list]

[h2]CONTROLS[/h2]

[list]
[*]CTRL+M - show / hide;
[*]= and - (or the mouse wheel over the minimap) - zoom, up to the whole map;
[*]CTRL+= and CTRL+- (or CTRL+wheel) - size of the minimap itself;
[*]right click on the minimap - the selected go to that spot;
[*]double click on the minimap - the camera moves to that spot, if the game lets it go that far from your characters (up to the camera range line);
[*]left button - drag the minimap, by the corner - resize it.
[/list]

Shape (circle, square, rectangle), frame, rotation with the camera, who to show and the colours are in the settings. Position, size and zoom are remembered. While the big map is open or the interface is hidden (F7), the minimap is hidden.

""" + settings_en("Best Minimap", "BestMinimap"))

# ------------------------------------------------------------ PatientSelfAid
T["PatientSelfAid"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Когда один персонаж оказывает первую помощь другому, пациент в обычной игре просто стоит и ждёт, даже если у него в сумке своя аптечка.

С плагином пациент в это время лечит себя сам, а скелет - чинит себя своим ремкомплектом. Лекарь и пациент работают над одним телом одновременно, так что лечение идёт быстрее. Это то же, что можно сделать руками - выделить пациента и указать им на самого себя. Плагин делает это за вас.

[h2]КОГДА СРАБАТЫВАЕТ[/h2]

[list]
[*][b]свои:[/b] и лекарь, и пациент - ваши персонажи;
[*][b]лечение идёт:[/b] лекарь лечит пациента - по приказу или работой медика;
[*][b]пациент ждёт:[/b] стоит, в сознании и на ногах;
[*][b]есть чем:[/b] ему есть что лечить, и у него есть аптечка (скелету - ремкомплект).
[/list]

Пациента, который сам кого-то лечит или дерётся, плагин не трогает.

""" + settings_ru("Patient Self Aid", "PatientSelfAid"), """[h2]WHAT IT DOES[/h2]

When one character gives first aid to another, in the vanilla game the patient just stands and waits - even with their own first aid kit in the bag.

With the plugin the patient treats their own wounds meanwhile, and a skeleton repairs itself with its repair kit. Healer and patient work on the same body at once, so treatment goes faster. It is the same as doing it by hand - select the patient and point them at themselves. The plugin does it for you.

[h2]WHEN IT TRIGGERS[/h2]

[list]
[*][b]yours:[/b] both the healer and the patient are your characters;
[*][b]treatment is on:[/b] the healer treats the patient - by order or as a medic job;
[*][b]the patient waits:[/b] standing, conscious and on their feet;
[*][b]has the means:[/b] there is something to treat, and they have a kit (a skeleton - a repair kit).
[/list]

A patient who is treating someone else or fighting is left alone.

""" + settings_en("Patient Self Aid", "PatientSelfAid"))

# ---------------------------------------------------------- QuickStartSelect
T["QuickStartSelect"] = ("""Своя версия «Choose Your Beginning Quicker» (Redronn), написанная заново.

[h2]ЧТО ДЕЛАЕТ[/h2]

Вместо окна «Выберите предысторию», где старты листаются стрелками по одному, открывается список всех стартов:

[list]
[*][b]две раскладки:[/b] широкая (список слева, описание справа) и компактная - переключаются первой кнопкой и запоминаются;
[*][b]сортировка[/b] по названию (A-Z / Z-A) и по сложности (ЛЕГКО-ТЯЖЕЛО / ТЯЖЕЛО-ЛЕГКО);
[*][b]поиск[/b] по названию;
[*][b]фильтр по расе[/b] отряда, с числом стартов у каждой расы;
[*][b]о старте:[/b] название, сложность, деньги, стиль игры и описание старта;
[*][b]другие настройки игры[/b] (голод, шанс смерти, урон, скорости, гнёзда, бандиты, разведка) прямо в окне, с подсказками игры; правый клик по строке - значение по умолчанию;
[*][b]«Случайный старт»[/b] - случайный старт из тех, что в списке; меняется только выбор старта, остальные настройки не трогаются;
[*][b]«Начать»[/b] или двойной щелчок по строке - с подтверждением (отключается).
[/list]

Сложность в списке окрашена от зелёного (легко) до красного (очень тяжело) и сортируется по смыслу, как бы её ни написал автор мода - на английском или на русском. Нестандартную сложность («Бип!!») можно вписать в Mod Config Menu под любой из шести цветов. Старты-пустышки без отряда (остатки переводов модов, которых нет) не показываются.

Список открывается с начала, выбран первый старт. Окно подстраивается под любое разрешение экрана.

""" + settings_ru("Quick Start Select", "QuickStartSelect"), """Our own take on "Choose Your Beginning Quicker" by Redronn, written from scratch.

[h2]WHAT IT DOES[/h2]

Instead of the "Choose your beginning" window, where starts are flipped one by one with arrows, a list of all starts opens:

[list]
[*][b]two layouts:[/b] wide (list on the left, description on the right) and compact - switched with the first button and remembered;
[*][b]sorting[/b] by name (A-Z / Z-A) and by difficulty (EASY-HARD / HARD-EASY);
[*][b]search[/b] by name;
[*][b]race filter[/b], with the number of starts for each race;
[*][b]about the start:[/b] name, difficulty, cash, play style and description of the start;
[*][b]the game's advanced options[/b] (hunger, chance of death, damage, speeds, nests, bandits, prospecting) right in the window, with the game's tooltips; right click a row - back to the default;
[*][b]"Random start"[/b] - a random start among those in the list; only the start choice changes, other settings stay as they were;
[*][b]"Begin"[/b] or a double click on a row - with a confirmation (can be turned off).
[/list]

Difficulty in the list is coloured from green (easy) to red (very hard) and sorted by meaning, however the mod author wrote it - in English or Russian. An unusual difficulty ("Beep!!") can be put under any of the six colours in Mod Config Menu. Empty starts without a squad (leftovers of translations for mods you do not have) are hidden.

The list opens from the top with the first start selected. The window adapts to any screen resolution.

""" + settings_en("Quick Start Select", "QuickStartSelect"))

# ----------------------------------------------------------------- WantedMap
T["WantedMap"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Выделите персонажа, у которого есть листовки «Разыскивается» - в инвентаре или в рюкзаке, - и на большой карте появятся метки с именами целей там, где они находятся. Удобно держать все листовки у одного «навигатора». По желанию листовки ищутся у всех ваших персонажей, кто бы ни был выделен.

[list]
[*][b]красная[/b] - сама цель, точное место;
[*][b]оранжевая[/b] - её отряд далеко, место примерное;
[*][b]жёлтая[/b] - отряд ещё не появлялся, метка на поселении или лагере, где он живёт;
[*][b]синяя[/b] - цель в плену;
[*][b]серая[/b] - цель убита: метка на месте смерти держится игровые сутки.
[/list]

Убрали листовку из инвентаря - её метка пропала.

На самой листовке в инвентаре видно, что с целями: зелёная галка - все убиты, замок - остальные в плену, «2/3» - сколько уже сделано.

[h2]КАК НАХОДИТ ЦЕЛИ[/h2]

Плагин сам сопоставляет листовки с целями по данным игры и всех включённых модов, на любом языке. На сборке из двухсот модов находятся цели 97 листовок из 100. Для редких случаев есть файл WantedMap_overrides.txt с ручными парами - его можно дополнить своими.

Показываются только живые цели. О смерти цели плагин узнаёт, даже если это случилось при закрытой карте, и помнит её для каждого прохождения отдельно: загрузили сейв, где цель ещё жива, - метка вернётся. Если цель освободили или она снова жива по данным игры, метка тоже вернётся.

Плагин работает, только пока открыта большая карта, так что на игру в остальное время не влияет.

""" + settings_ru("Wanted Map", "WantedMap"), """[h2]WHAT IT DOES[/h2]

Select a character who carries "Wanted" posters - in the inventory or a backpack - and the world map shows markers with the targets' names where they are. Handy to keep all posters on one "navigator". Optionally the posters of all your characters count, whoever is selected.

[list]
[*][b]red[/b] - the target itself, exact spot;
[*][b]orange[/b] - their squad is far away, approximate spot;
[*][b]yellow[/b] - the squad has not appeared yet, the marker is on the town or camp where it lives;
[*][b]blue[/b] - the target is a prisoner;
[*][b]grey[/b] - the target is dead: the marker stays at the place of death for a game day.
[/list]

Remove a poster from the inventory - its marker is gone.

The poster itself in the inventory shows how its targets are: a green tick - all dead, a lock - the rest imprisoned, "2/3" - how many are done.

[h2]HOW IT FINDS TARGETS[/h2]

The plugin matches posters to targets by itself, from the data of the game and all enabled mods, in any language. On a 200-mod build the targets of 97 posters out of 100 are found. For rare cases there is WantedMap_overrides.txt with manual pairs - you can add your own.

Only live targets are shown. The plugin learns about a target's death even with the map closed, and remembers it per playthrough: load a save where the target is still alive - the marker comes back.

The plugin works only while the world map is open, so it does not affect the game otherwise.

""" + settings_en("Wanted Map", "WantedMap"))

# ------------------------------------------------------------ BuildModeGizmo
T["BuildModeGizmo"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Поставили здание, подтвердили - а место оказалось неудачным. Раньше оставалось разобрать площадку и ставить заново или лезть в меню разработчика.

Теперь достаточно выделить свою ещё не построенную постройку: на ней появятся стрелочки, как в редакторе. Тяните стрелочку, чтобы сдвинуть здание; кнопкой в окне преобразования можно переключиться на поворот, ввести координаты или вернуть как было.

[list]
[*][b]после перемещения[/b] здание переносится целиком - вместе с местом, где работают строители;
[*][b]рамка выделения[/b] не мешает, пока тянете стрелочку;
[*][b]по желанию[/b] поднятое стрелкой здание остаётся на этой высоте, а не опускается на землю;
[*][b]стрелочки держатся[/b], пока выделена площадка, и убираются, когда выделили другое, площадку достроили или разобрали, окно закрыли крестиком или начали ставить новую постройку;
[*][b]если что-то пойдёт не так[/b], плагин отключит стрелочки до перезапуска, а не уронит игру.
[/list]

[h2]МЕНЮ РАЗРАБОТЧИКА[/h2]

В меню разработчика (Shift+F12) игра даёт двигать здания стрелочками только в режиме интерьеров. С плагином - и в обычном режиме строительства: поставили дома, подтвердили, щёлкнули по любому зданию - своему или чужому, достроенному или нет - и сразу двигаете его стрелочками. Щёлкайте ближе к середине здания: у самого края большого дома щелчок может не попасть.

""" + settings_ru("Build Mode Gizmo", "BuildModeGizmo"), """[h2]WHAT IT DOES[/h2]

You placed a building, confirmed it - and the spot turned out bad. Before, you had to dismantle the site and place it again, or dig into the developer menu.

Now just select your own building site that is not built yet: editor-style arrows appear on it. Drag an arrow to move the building; a button in the transform window switches to rotation, lets you type coordinates or reset.

[list]
[*][b]after a move[/b] the whole building moves - including the spot where builders work;
[*][b]the selection box[/b] does not get in the way while you drag;
[*][b]optionally[/b] a building raised with the arrow stays at that height instead of settling on the ground;
[*][b]the arrows stay[/b] while the site is selected and go away when you select something else, the site is finished or dismantled, the window is closed with its cross, or you start placing a new building;
[*][b]if something goes wrong[/b], the plugin turns the arrows off until restart instead of crashing the game.
[/list]

[h2]DEVELOPER MENU[/h2]

In the developer menu (Shift+F12) the game lets you move buildings with the arrows only in the interior mode. With the plugin - in the normal build mode too: place your buildings, confirm, click any building - yours or not, built or not - and move it with the arrows right away. Click closer to the middle of a building: right at the edge of a large house a click may miss.

""" + settings_en("Build Mode Gizmo", "BuildModeGizmo"))

# ------------------------------------------------------------ RangedDamageXp
T["RangedDamageXp"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

В игре арбалетчик получает опыт за саму стрельбу, и неважно, что сделал выстрел - оцарапал броню или выбил противнику руку. Плагин добавляет к этому опыту прибавку за нанесённый урон - после брони цели, так что выстрел в тяжёлый доспех даёт меньше, чем в голого:

[list]
[*][b]Арбалеты[/b] - основная прибавка;
[*][b]Восприятие[/b] - её часть;
[*][b]Турели[/b] - вместо Арбалетов, если стрелок сидит за турелью.
[/list]

Обычный опыт за стрельбу остаётся как был, прибавка идёт сверху. Действует всё, что обычно влияет на опыт: расовый множитель, XP_Overhaul, полосы LiveXpBars.

Не считаются попадания по своим, по лежачим без сознания и совсем слабые царапины (всё это настраивается). Сколько опыта даёт урон и потолок за одно попадание - тоже.

""" + settings_ru("Ranged Damage Xp", "RangedDamageXp"), """[h2]WHAT IT DOES[/h2]

In the game a crossbowman gets experience for shooting itself, no matter what the shot did - scratched the armour or took the enemy's arm off. The plugin adds a bonus for the damage dealt - after the target's armour, so a shot into heavy armour gives less than one into a naked target:

[list]
[*][b]Crossbows[/b] - the main bonus;
[*][b]Perception[/b] - a share of it;
[*][b]Turrets[/b] - instead of Crossbows when the shooter mans a turret.
[/list]

The usual shooting experience stays as it was, the bonus goes on top. Everything that normally affects experience applies: racial multiplier, XP_Overhaul, LiveXpBars.

Hits on your own people, on unconscious targets and tiny scratches do not count (all adjustable). The XP per damage and the cap per hit are adjustable too.

""" + settings_en("Ranged Damage Xp", "RangedDamageXp"))


# ------------------------------------------------------------- ModConfigMenu
# Свой текст (раньше - README целиком): без плашки заголовка и путей к
# файлам, но с разделом для авторов плагинов - для MCM он нужен.
T["ModConfigMenu"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Все настройки модов - прямо в игре. В окне «Настройки» (главное меню или Esc в игре) появляется вкладка MCM:

[list]
[*][b]слева[/b] - моды, у которых есть настройки;
[*][b]справа[/b] - настройки выбранного мода, теми же строками, что во всей игре: флажки, ползунки с полем для числа, кнопки выбора и клавиш, поля ввода, цвета - из палитры или кодом;
[*][b]внизу[/b] - подсказка к строке под курсором;
[*][b]в заголовке окна[/b] - название выбранного мода.
[/list]

Изменение сразу сохраняется и применяется. Строки со звёздочкой (*) действуют после перезапуска игры. Кнопка «Сбросить по умолчанию» внизу страницы возвращает все значения мода.

[h2]КАКИЕ МОДЫ[/h2]

Любой плагин RE_Kenshi, который поддерживает MCM. Уже умеют:

[list]
[*][b]наши моды:[/b] Live Xp Bars, Stat Colours, Gear Compare Alt, Assigned Workers, UI Tweaks, Patient Self Aid, Wanted Map, Build Mode Gizmo, Ranged Damage Xp, Quick Start Select, Item Markers, Recruit Markers;
[*][b]наши форки модов Emkej8:[/b] Auto-Pause on Load, Container Highlight, Hidden Faction Relations, Job B Gone, Loot Scoot Execute, Map markers, Organize the Crafting Stations, Organize the Trader, Vital Read, Vital Sense, Wall B Gone;
[*][b]наши форки модов других авторов:[/b] Better Looting (XxAtreuSSxX), Character Highlight (BFrizzleFoShizzle), Formations Extended (Slinkysalmon03), Kenshi Job Management (SirixKC).
[/list]

Для всех них MCM не обязателен: без него настройки остаются в их файлах, как раньше.

[h2]ДЛЯ АВТОРОВ ПЛАГИНОВ[/h2]

Плагин от MCM не зависит: настройки остаются в его ini. Чтобы появиться во вкладке, плагин экспортирует одну функцию - [b]MCM_Describe[/b] - и описывает в ней свои настройки:

[list]
[*]флажки, целые и дробные числа, варианты на выбор, текст, клавиши, цвета (палитра);
[*]строки на своих функциях - для плагинов, которые хранят настройки не в ini;
[*]своя область на странице - для своих виджетов (списки, таблицы);
[*]строки, которые строятся при каждом показе страницы, - для того, что есть только в загруженной игре (например, отряды);
[*]подсказка к моду в списке и значения по умолчанию для кнопки сброса.
[/list]

MCM находит плагины сам, порядок загрузки модов не важен. Всё нужное - в папке sdk мода: заголовок ModConfigMenu.h и руководство на английском, русском и китайском (MCM_API_en.md, MCM_API_ru.md, MCM_API_zh.md). Для модов на Emkej's Mod Hub есть готовый мост - McmModHubBridge.h.

[h2]ИСХОДНИКИ[/h2]

Исходный код MCM и остальных наших плагинов - [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins]LEOPARD-Kenshi-Plugins[/url], наших форков модов Emkej8 и других авторов - [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks]LEOPARD-Kenshi-Forks[/url] (GitHub).
""", """[h2]WHAT IT DOES[/h2]

Every mod setting - right in the game. The Options window (main menu or Esc in game) gets an MCM tab:

[list]
[*][b]left[/b] - the mods that have settings;
[*][b]right[/b] - the settings of the selected mod, drawn with the same rows as the rest of the game: check boxes, sliders with a number field, choice and key buttons, text fields, colours - from a palette or as a code;
[*][b]bottom[/b] - a tooltip for the row under the cursor;
[*][b]window caption[/b] - the name of the selected mod.
[/list]

A change is saved and applied at once. Rows marked with an asterisk (*) take effect after a game restart. The "Reset to defaults" button at the bottom of the page returns all values of the mod.

[h2]WHICH MODS[/h2]

Any RE_Kenshi plugin that supports MCM. Already supported:

[list]
[*][b]our mods:[/b] Live Xp Bars, Stat Colours, Gear Compare Alt, Assigned Workers, UI Tweaks, Patient Self Aid, Wanted Map, Build Mode Gizmo, Ranged Damage Xp, Quick Start Select, Item Markers, Recruit Markers;
[*][b]our forks of Emkej8's mods:[/b] Auto-Pause on Load, Container Highlight, Hidden Faction Relations, Job B Gone, Loot Scoot Execute, Map markers, Organize the Crafting Stations, Organize the Trader, Vital Read, Vital Sense, Wall B Gone;
[*][b]our forks of other authors' mods:[/b] Better Looting (XxAtreuSSxX), Character Highlight (BFrizzleFoShizzle), Formations Extended (Slinkysalmon03), Kenshi Job Management (SirixKC).
[/list]

MCM is optional for all of them: without it their settings stay in their files, as before.

[h2]FOR PLUGIN AUTHORS[/h2]

A plugin does not depend on MCM: its settings stay in its ini. To show up in the tab, the plugin exports one function - [b]MCM_Describe[/b] - and describes its settings there:

[list]
[*]toggles, integers and decimals, choices, text, hotkeys, colours (palette);
[*]rows on your own functions - for plugins that keep settings elsewhere than an ini;
[*]your own area on the page - for your own widgets (lists, tables);
[*]rows built each time the page is shown - for what exists only in the loaded game (squads, for one);
[*]a tooltip for the mod in the list and default values for the reset button.
[/list]

MCM finds plugins by itself, the load order does not matter. Everything you need is in the mod's sdk folder: the ModConfigMenu.h header and a guide in English, Russian and Chinese (MCM_API_en.md, MCM_API_ru.md, MCM_API_zh.md). A bridge for mods built on Emkej's Mod Hub is included - McmModHubBridge.h.

[h2]SOURCE CODE[/h2]

The source code of MCM and our other plugins - [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins]LEOPARD-Kenshi-Plugins[/url], of our forks of Emkej8's and other authors' mods - [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks]LEOPARD-Kenshi-Forks[/url] (GitHub).
""")


# ===================================================== форки модов Emkej
# ---------------------------------------------------------------- ItemMarkers
T["ItemMarkers"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Пометки прямо на иконках вещей - в инвентаре, у торговца, в сундуках и на снаряжении персонажа:

[list]
[*][b]цвет сорта[/b] у брони, оружия, арбалетов и протезов - тот же, что в подсказке вещи (у брони, которая ничего не закрывает, сорта нет, как и в игре). Четыре стиля: рамка со свечением, заливка, градиент снизу, звезда в углу;
[*][b]краденое[/b] - значок кражи на краденых вещах;
[*][b]форма фракции[/b] - значок на броне, в которой вас принимают за своего;
[*][b]изученные чертежи[/b] - глаз на чертеже, который вы уже знаете: у торговца сразу видно, что покупать незачем;
[*][b]вещи этой сделки[/b] - мешочек в углу: золотой - вы продали, зелёный - купили, в том числе в уже лежащую стопку;
[*][b]мейто[/b] - свой цвет, отдельно от мастерского класса, и в иконке, и в подсказке.
[/list]

Цвета сортов берутся из интерфейса игры - и из модов на интерфейс, в том числе подписанных в Мастерской, - поэтому интерфейс меняет их сам. По умолчанию выделяются вещи получше обычных - начиная с синего сорта.

[h2]СИЛЬНО ОПТИМИЗИРОВАННАЯ ВЕРСИЯ[/h2]

Это замена модам [b]LtEast's Rarity Backgrounds[/b], [b]LtEast's Item Status Indicators[/b] и [b]LtEast's Mod Settings Core[/b] - один мод вместо трёх. Моды LtEast пересчитывали иконки каждый кадр в каждом открытом окне инвентаря: в торговле с сотнями вещей FPS падал примерно вдвое, а при продаже были рывки.

Здесь пометка ставится один раз, когда игра рисует иконку, и исчезает вместе с ней. В остальное время мод почти ничего не делает: у торговца на 160 вещей уходит несколько миллисекунд один раз - при открытии окна, а во время торговли он лишь несколько раз в секунду смотрит, какие видимые вещи перешли из рук в руки.

[b]Моды LtEast нужно отключить[/b], иначе пометки будут двойными.

""" + settings_ru("Item Markers", "ItemMarkers"), """[h2]WHAT IT DOES[/h2]

Marks right on item icons - in the inventory, at traders, in containers and on the character's gear:

[list]
[*][b]grade colour[/b] on armour, weapons, crossbows and limbs - the same as in the item tooltip (armour that covers nothing has no grade, as in the game). Four styles: frame with a glow, fill, gradient from the bottom, star in a corner;
[*][b]stolen goods[/b] - the steal badge on stolen items;
[*][b]faction uniform[/b] - a badge on armour that makes you look like one of them;
[*][b]learned blueprints[/b] - an eye on a blueprint you already know: at a trader you see at once what is not worth buying;
[*][b]items of this trade[/b] - a bag in the corner: gold - you sold it, green - you bought it, stacks included;
[*][b]Meitou[/b] - a colour of its own, apart from the masterwork class, on the icon and in the tooltip.
[/list]

Grade colours come from the game's interface - and from interface mods, Workshop subscriptions included - so the interface changes them by itself. By default only items better than ordinary stand out - from the blue grade up.

[h2]A HEAVILY OPTIMISED VERSION[/h2]

A replacement for [b]LtEast's Rarity Backgrounds[/b], [b]LtEast's Item Status Indicators[/b] and [b]LtEast's Mod Settings Core[/b] - one mod instead of three. LtEast's mods re-processed the icons every frame in every open inventory window: at a trader with hundreds of items FPS dropped by about half, and selling stuttered.

Here a mark is put once, when the game draws the icon, and goes away with it. The rest of the time the mod does almost nothing: 160 items at a trader take a few milliseconds, once, when the window opens, and during a trade it only checks a few times a second which visible items changed hands.

[b]Disable LtEast's mods[/b], or the marks will be doubled.

""" + settings_en("Item Markers", "ItemMarkers"))

# ------------------------------------------------------------ RecruitMarkers
T["RecruitMarkers"] = ("""[h2]ЧТО ДЕЛАЕТ[/h2]

Над головой у тех, кого можно нанять, - значок-облачко, а над торговцами - стопка монет. По умолчанию значки видны, пока зажат ALT; клавишу можно сменить или сделать значки постоянными.

[list]
[*][b]уникальные персонажи[/b] - светло-жёлтый значок, мягко пульсирует;
[*][b]обычные наёмники и животные[/b] - оранжевый;
[*][b]только те, кого можно нанять прямо сейчас[/b] - мод смотрит сам диалог персонажа и его условия (деньги, отношения, плен и прочее) для вашего ближайшего выделенного персонажа. Поэтому работает и с модами, которые добавляют нанимаемых;
[*][b]торговцы[/b] - продавцы, бармены, торговцы караванов, хирурги - стопка монет своим цветом; торговца, которого можно и нанять, отмечают оба значка рядом. Охрана лавок и караванов монет не получает.
[/list]

Цвета, размер, прозрачность, высота над головой, дальность и число значков настраиваются.

[h2]ЗАМЕНА МОДА LTEAST[/h2]

Это замена [b]LtEast's Recruitable NPC Highlighter[/b] и его Mod Settings Core: тот же вид значков, настройки - в Mod Config Menu. Диалоги проверяются понемногу и запоминаются на несколько секунд, а пока клавиша показа не зажата, мод ничего не делает.

[b]Мод LtEast нужно отключить[/b], иначе значки будут двойными.

Работает на Kenshi 1.0.65 и 1.0.68 (Steam и GOG).

""" + settings_ru("Recruit Markers", "RecruitMarkers", "перезапуск не нужен."), """[h2]WHAT IT DOES[/h2]

A speech-bubble marker over the heads of characters you can recruit, and a stack of coins over traders. By default the markers show while ALT is held; the key can be changed or the markers made permanent.

[list]
[*][b]unique characters[/b] - a light yellow marker that gently pulses;
[*][b]ordinary recruits and animals[/b] - orange;
[*][b]only those you can recruit right now[/b] - the mod reads the character's own dialogue and its conditions (money, relations, imprisonment and so on) for your nearest selected character. So it works with mods that add recruitable characters too;
[*][b]traders[/b] - shopkeepers, barmen, caravan traders, surgeons - a stack of coins in a colour of its own; a trader you can also recruit gets both markers side by side. Shop and caravan guards get no coins.
[/list]

Colours, size, opacity, height above the head, distance and the number of markers are adjustable.

[h2]A REPLACEMENT FOR LTEAST'S MOD[/h2]

A replacement for [b]LtEast's Recruitable NPC Highlighter[/b] and its Mod Settings Core: the same markers, settings in Mod Config Menu. Dialogues are checked a little at a time and remembered for a few seconds, and while the show key is not held the mod does nothing.

[b]Disable LtEast's mod[/b], or the markers will be doubled.

Works with Kenshi 1.0.65 and 1.0.68 (Steam and GOG).

""" + settings_en("Recruit Markers", "RecruitMarkers", "no restart needed."))


# ---------------------------------------------------------- Dark UI Advanced
T["Dark UI Advanced"] = ("""Тёмный интерфейс Kenshi - гибрид модов [b]Dark UI[/b] (Eldryn) и [b]Russian Dark UI simplistic[/b] (FG), с нашими доработками.

[h2]ЧТО ЭТО[/h2]

Тёмные окна и панели вместо светлого пергамента игры. Разметка окон рассчитана на русский перевод: длинные русские надписи помещаются и не налезают друг на друга. С английским и другими языками тоже работает - места просто больше.

[h2]ЧТО ДОРАБОТАНО[/h2]

[list]
[*][b]окно торговца[/b] - строка денег растягивается вместе с окном, сумма не застревает посередине (с модом UI Tweaks - точно по центру);
[*][b]вкладка «Изучение»[/b] - блок описания выше примерно на треть, длинные описания доспехов помещаются целиком;
[*][b]окно навыков[/b] - в столбце «Ремесло» снова видны все шесть строк;
[*][b]очереди исследований и ремесла[/b] - аккуратный крестик удаления в строке;
[*][b]оружие мейто[/b] - свой, фиолетовый цвет сорта (с модом Item Markers), мастерское остаётся жёлтым.
[/list]

[h2]ИСТОЧНИКИ[/h2]

[list]
[*][b]Dark UI[/b] от Eldryn ([url=https://steamcommunity.com/sharedfiles/filedetails/?id=1200632417]Steam[/url]) - основа;
[*][b]Russian Dark UI simplistic[/b] от FG - переделка Dark UI под русский перевод.
[/list]

Спасибо авторам - без них этого мода бы не было.

[h2]СОВМЕСТИМОСТЬ[/h2]

Только файлы интерфейса: разметка окон, скины и палитра. На сохранения не влияет, включать и выключать можно когда угодно. Это замена Dark UI и Russian Dark UI simplistic - вместе с ними не включайте. Ставьте ниже других модов на интерфейс в списке загрузки.
""", """A dark interface for Kenshi - a hybrid of [b]Dark UI[/b] by Eldryn and [b]Russian Dark UI simplistic[/b] by FG, with our improvements.

[h2]WHAT IT IS[/h2]

Dark windows and panels instead of the game's light parchment. The window layouts are made for the Russian translation, so long Russian captions fit without overlapping. English and other languages work too - there is simply more room.

[h2]WHAT IS IMPROVED[/h2]

[list]
[*][b]trader window[/b] - the money line stretches with the window, so the amount no longer sticks in the middle (with UI Tweaks - right at the centre);
[*][b]Research tab[/b] - the description box is about a third taller, long armour descriptions fit in full;
[*][b]skills window[/b] - all six rows of the Crafting column are visible again;
[*][b]research and crafting queues[/b] - a neat remove button in the row;
[*][b]Meitou weapons[/b] - a grade colour of their own, purple (with Item Markers); masterwork stays yellow.
[/list]

[h2]SOURCES[/h2]

[list]
[*][b]Dark UI[/b] by Eldryn ([url=https://steamcommunity.com/sharedfiles/filedetails/?id=1200632417]Steam[/url]) - the base;
[*][b]Russian Dark UI simplistic[/b] by FG - Dark UI reworked for the Russian translation.
[/list]

Thanks to the authors - this mod would not exist without them.

[h2]COMPATIBILITY[/h2]

Interface files only: window layouts, skins and the palette. Saves are not affected, it can be turned on and off at any time. It replaces Dark UI and Russian Dark UI simplistic - do not enable it together with them. Place it below other interface mods in the load order.
""")


def fork(mod, page, ru, en):
    # последний пункт списка - с точкой, а не с «;»
    fix = lambda t: t.replace(";\n[/list]", ".\n[/list]")
    T[mod] = (fix(fork_header(mod, "ru") + ru.strip("\n") + "\n\n" + fork_settings_ru(page)),
              fix(fork_header(mod, "en") + en.strip("\n") + "\n\n" + fork_settings_en(page)))


fork("Wall-B-Gone", "Wall-B-Gone", """[h2]ЧТО ДЕЛАЕТ[/h2]

Клавиша разборки (по умолчанию X): выделите свою стену, кровать, спальник, стул, скамью, стол или трон и нажмите её - объект разберётся так же, как кнопкой игры, материалы вернутся как обычно. Удобно при перестройке базы: не нужно каждый раз щёлкать по кнопке «Разобрать».

[list]
[*]кровать или спальник, на которых кто-то лежит, не разбираются;
[*]стена - только если на ней и в ней ничего не построено;
[*]по желанию клавиша разбирает любую свою постройку - хранилища, станки, дома; без людей внутри и без мебели. Вещи в хранилище либо мешают разборке, либо (отдельная настройка) выкладываются на землю перед сносом;
[*]по желанию перед разборкой - вопрос: Enter - разобрать, Esc - отмена;
[*]клавишу можно сменить и добавить к ней CTRL, SHIFT или ALT.
[/list]

Баланс не меняется: это та же разборка игры, только быстрее.
""", """[h2]WHAT IT DOES[/h2]

A dismantle key (X by default): select your own wall, bed, sleeping bag, chair, bench, table or throne and press it - the object is dismantled exactly like with the game's button, materials are refunded as usual. Handy when redesigning a base: no need to click the "Dismantle" button every time.

[list]
[*]an occupied bed or sleeping bag is never dismantled;
[*]a wall - only if nothing is built on or in it;
[*]optionally the key dismantles any of your buildings - storage, workbenches, houses; with nobody inside and no furniture. Items in a storage either block dismantling or (a separate option) are put on the ground first;
[*]optionally a question before dismantling: Enter - dismantle, Esc - cancel;
[*]the key can be changed, with CTRL, SHIFT or ALT if you like.
[/list]

No balance changes: it is the game's own dismantling, just faster.
""")

fork("Auto-Pause-on-Load", "Auto-Pause on Load", """[h2]ЧТО ДЕЛАЕТ[/h2]

Ставит игру на паузу в нужный момент:

[list]
[*]после загрузки или переноса сохранения - мир не начинает жить, пока вы осматриваетесь;
[*]когда начинается торговля;
[*]когда открывается инвентарь;
[*]по желанию - когда бьют персонажа из отряда, который сейчас не выбран.
[/list]

После торговли и инвентаря игра может сама сняться с паузы - это настраивается. Каждый вид паузы включается отдельно. Баланс и содержимое игры не меняются.
""", """[h2]WHAT IT DOES[/h2]

Pauses the game at the right moment:

[list]
[*]after a save is loaded or imported - the world does not start moving while you look around;
[*]when trading starts;
[*]when an inventory opens;
[*]optionally when a character of a squad that is not selected takes a hit.
[/list]

After trading and inventory the game can unpause by itself - configurable. Each pause can be turned on separately. No balance or content changes.
""")

fork("Job-B-Gone", "Job-B-Gone", """[h2]ЧТО ДЕЛАЕТ[/h2]

Над панелью отряда появляется панель для быстрой чистки очередей работ:

[list]
[*]удалить все работы у себя, у выделенных, у всего отряда или у всех сразу;
[*]удалить одну конкретную работу - в тех же пределах;
[*]для «отряда» и «всех» - двойное подтверждение с числом затронутых персонажей и работ, чтобы не снести лишнего случайно;
[*]панель сворачивается, перетаскивается за заголовок и помнит своё место;
[*]CTRL+B прячет и показывает панель.
[/list]

Баланс и содержимое игры не меняются - только удобство работы с очередями.
""", """[h2]WHAT IT DOES[/h2]

A panel above the squad panel for quick job queue cleanup:

[list]
[*]delete all jobs for yourself, the selected characters, the whole squad or everyone;
[*]delete one specific job - in the same scopes;
[*]for "squad" and "all" - a two-step confirmation with the number of characters and jobs affected, so nothing is removed by accident;
[*]the panel collapses, can be dragged by its header and remembers its position;
[*]CTRL+B hides and shows the panel.
[/list]

No balance or content changes - only easier queue management.
""")

fork("Loot-Scoot-Execute", "Loot-Scoot-Execute", """[h2]ЧТО ДЕЛАЕТ[/h2]

В контекстном меню поверженного врага - среди пунктов самой игры - появляются «Добить» и «Добить всех»:

[list]
[*][b]Добить[/b] - выделенный персонаж подойдёт и добьёт этого врага;
[*][b]Добить всех[/b] - добьёт всех лежащих врагов поблизости, сначала ближайших; несколько выделенных персонажей бегут каждый к своей цели; при наведении цели подсвечиваются красным;
[*]отменить добивание - любым приказом, например щелчком по земле;
[*]работает только на врагах без сознания, своих никогда не трогает;
[*]выбор - как в обычном меню игры: навести и отпустить правую кнопку.
[/list]

Расстояние, радиус «Добить всех» и размер кнопок настраиваются.
""", """[h2]WHAT IT DOES[/h2]

The context menu of a downed enemy gets "Execute" and "Execute All" among the game's own options:

[list]
[*][b]Execute[/b] - the selected character walks up and finishes this enemy;
[*][b]Execute All[/b] - finishes all downed enemies nearby, nearest first; several selected characters each run to their own target; hovering highlights the targets in red;
[*]cancel an execution with any order, e.g. a click on the ground;
[*]works only on unconscious enemies, never on your own people;
[*]selection works like the game's own menu: hover and release the right mouse button.
[/list]

Distance, the Execute All radius and the button size are adjustable.
""")

fork("Vital-Sense", "Vital Sense", """[h2]ЧТО ДЕЛАЕТ[/h2]

Зажмите ALT - над видимыми персонажами появятся значки состояния, отпустите - пропадут:

[list]
[*]без сознания, кома, умирает, притворяется мёртвым, мёртв - значком и коротким текстом;
[*]цвет по отношению: враг, нейтрал, союзник, свой отряд;
[*]значок награды над разыскиваемыми, цвет - по сумме награды;
[*]по желанию - подсветка тела лежащих персонажей, чтобы их было видно издалека.
[/list]

Каждое состояние, цвета, размеры, клавиша и число значков на экране настраиваются. Только наглядность: баланс, ИИ и содержимое игры не меняются.
""", """[h2]WHAT IT DOES[/h2]

Hold ALT - state markers appear over visible characters; release - they are gone:

[list]
[*]unconscious, coma, dying, playing dead, dead - as an icon and short text;
[*]colour by relation: enemy, neutral, ally, your squad;
[*]a bounty icon over wanted characters, coloured by the bounty amount;
[*]optionally a body highlight on downed characters, so they are visible from afar.
[/list]

Every state, colours, sizes, the key and the number of markers on screen are adjustable. Readability only: no balance, AI or content changes.
""")

fork("Organize-the-Trader", "Organize the Trader", """[h2]ЧТО ДЕЛАЕТ[/h2]

В окне торговца появляются поиск и сортировка:

[list]
[*]поиск по названию прямо во время набора; найденные вещи собираются в начало сетки;
[*]чертежи ищутся по названию того, что они открывают; «b:» - только чертежи;
[*]сортировка по названию, цене за штуку, цене стопки, весу и цене за вес - по возрастанию или убыванию;
[*]поиск и сортировка - в нижней строке окна, на месте кнопки «Разложить»; ширина списка и поля поиска задаётся в настройках;
[*]рядом с деньгами торговца - итог сделки: сколько вы наторговали за это окно, зелёным в плюс, красным в минус (можно выключить);
[*]CTRL+F или / - сразу в поиск.
[/list]

Цены, товар и сами вещи не меняются - только то, как они показаны.
""", """[h2]WHAT IT DOES[/h2]

The trader window gets search and sorting:

[list]
[*]search by name as you type; the found items gather at the start of the grid;
[*]blueprints are found by the name of what they unlock; "b:" - blueprints only;
[*]sort by name, unit price, stack value, weight and value per weight - ascending or descending;
[*]search and sorting sit in the bottom row of the window, in place of the Arrange button; the list and search field widths are settings;
[*]next to the trader's money - the deal balance: how much you made in this window, green for a gain, red for a loss (can be turned off);
[*]CTRL+F or / - straight to search.
[/list]

Prices, stock and items are not changed - only how they are shown.
""")

fork("Organize-the-Crafting-Stations", "Organize the Crafting Stations", """[h2]ЧТО ДЕЛАЕТ[/h2]

В окнах ремесла и исследований появляется поиск:

[list]
[*]рецепты и чертежи отбираются по названию прямо во время набора;
[*]найденные строки собираются наверх, без пустых промежутков;
[*]рядом - счётчик «найдено / всего» и кнопка очистки;
[*]панель поиска перетаскивается за ручку «::», место запоминается;
[*]ширина и высота поля, автофокус и счётчик настраиваются.
[/list]

Рецепты, исследования и сами станки не меняются - только то, как список показан. С Organize the Trader не конфликтует: окна торговца он не трогает.
""", """[h2]WHAT IT DOES[/h2]

Crafting and research windows get a search bar:

[list]
[*]recipes and blueprints are filtered by name as you type;
[*]the found rows gather at the top, with no empty gaps;
[*]next to it - a "found / total" counter and a clear button;
[*]the search panel is dragged by its "::" handle, the position is remembered;
[*]field width and height, autofocus and the counter are adjustable.
[/list]

Recipes, research and the stations themselves are not changed - only how the list is shown. Works alongside Organize the Trader: it does not touch trader windows.
""")

fork("Map-markers", "Map markers", """[h2]ЧТО ДЕЛАЕТ[/h2]

Свои метки на большой карте - для заметок: где руда, где опасно, куда вернуться.

[list]
[*]щелчок средней кнопкой по пустому месту карты - новая метка, сразу можно вписать подпись (Enter - сохранить);
[*]левый щелчок по метке - выбрать, по пустому месту - перенести выбранную; удалить - кнопкой «Удалить» в окне метки или клавишей Delete;
[*]у меток есть типы и короткие подписи;
[*]кнопка «Метки: Вкл/Выкл» внизу карты прячет и показывает их;
[*]метки свои у каждого сохранения.
[/list]
""", """[h2]WHAT IT DOES[/h2]

Your own markers on the world map - for notes: where the ore is, where it is dangerous, where to come back.

[list]
[*]middle-click empty map space - a new marker, you can type a label right away (Enter saves);
[*]left-click a marker to select it, empty space to move the selected one; remove it with the Delete button in the marker window or the Delete key;
[*]markers have types and short labels;
[*]the "Markers: On/Off" button at the bottom of the map hides and shows them;
[*]markers are kept per save.
[/list]
""")

fork("Vital-Read", "Vital Read", """[h2]ЧТО ДЕЛАЕТ[/h2]

На портретах отряда - значки состояния, видно с одного взгляда, кому плохо:

[list]
[*]без сознания, кома, умирает, притворяется мёртвым;
[*]голодает, повреждена рука, повреждена нога;
[*]лечится в кровати - голубой плюс; скелет на ремонтной кровати - голубой ключ;
[*]по желанию - короткий текст рядом со значком;
[*]размер, угол портрета и отступы настраиваются.
[/list]

Только наглядность: правила лечения, ИИ и баланс не меняются.
""", """[h2]WHAT IT DOES[/h2]

State icons on squad portraits - see at a glance who is in trouble:

[list]
[*]unconscious, coma, dying, playing dead;
[*]starving, crippled arm, crippled leg;
[*]resting in a bed - a blue plus; a skeleton in a repair bed - a blue wrench;
[*]optionally a short text next to the icon;
[*]size, portrait corner and margins are adjustable.
[/list]

Readability only: medical rules, AI and balance are not changed.
""")

fork("Hidden-Faction-Relations", "Hidden Faction Relations", """[h2]ЧТО ДЕЛАЕТ[/h2]

В окне «Настройки» игры появляется вкладка со списком фракций и вашими отношениями с каждой - включая скрытые фракции, которых игра нигде не показывает.

[list]
[*]поиск по названию;
[*]сортировка по отношению или по алфавиту;
[*]все фракции или только скрытые; можно оставить только ненулевые отношения;
[*]цвет: враждебные - красным, дружеские - зелёным, нейтральные - жёлтым.
[/list]

Баланс и содержимое игры не меняются.
""", """[h2]WHAT IT DOES[/h2]

The game's Options window gets a tab with the list of factions and your relation to each - including hidden factions the game never shows.

[list]
[*]search by name;
[*]sort by relation or alphabetically;
[*]all factions or hidden only; you can keep only non-zero relations;
[*]colour: hostile in red, friendly in green, neutral in yellow.
[/list]

No balance or content changes.
""")

fork("Container-Highlight", "Container Highlight", """[h2]ЧТО ДЕЛАЕТ[/h2]

Пока зажата клавиша подсветки, ближние контейнеры, в которых можно поживиться, отмечаются на экране:

[list]
[*]метка со значком и короткой подписью, с обводкой - читается на любом фоне;
[*]по желанию сами контейнеры подкрашиваются в мире;
[*]дальность поиска, цвета, подпись и клавиша (можно SHIFT, ALT или CTRL) настраиваются.
[/list]

Рассчитан на контейнеры игры и похожие на них; хранилища из модов-построек могут не подсвечиваться. В сохранение ничего не пишет.
""", """[h2]WHAT IT DOES[/h2]

While the highlight key is held, nearby lootable containers are marked on screen:

[list]
[*]a marker with an icon and a short outlined label - readable on any background;
[*]optionally the containers themselves are tinted in the world;
[*]scan distance, colours, label and key (SHIFT, ALT or CTRL work too) are adjustable.
[/list]

Made for vanilla and vanilla-like containers; storage from building mods may not be highlighted. Adds nothing to save files.
""")


# ============================================ форки других авторов
def other_fork(mod, ru, en):
    fix = lambda t: t.replace(";\n[/list]", ".\n[/list]")
    T[mod] = (fix(other_header(mod, "ru") + ru.strip("\n") + "\n"),
              fix(other_header(mod, "en") + en.strip("\n") + "\n"))


other_fork("Character-Highlight", """[h2]ЧТО ДЕЛАЕТ[/h2]

Пока зажата клавиша (по умолчанию ALT), персонажи окрашиваются по тому, как они к вам относятся:

[list]
[*][b]свой отряд[/b] - синий;
[*][b]враги[/b] - красный;
[*][b]союзники[/b] - зелёный;
[*][b]нейтралы[/b] - от красного через жёлтый к зелёному, по отношению их фракции к вам (или одним цветом);
[*][b]дипломаты фракций[/b] - фиолетовый, каким бы ни было отношение их фракции; дипломаты из модов - тоже.
[/list]

Каждую категорию можно выключить; цвета, клавиша, дальность и «видно сквозь стены» настраиваются.

[h2]СИЛЬНО ОПТИМИЗИРОВАННАЯ ВЕРСИЯ[/h2]

Оригинал ел FPS всё время - даже когда никого не подсвечивал: каждый кадр он перебирал всех персонажей вокруг и сбрасывал им окраску. А с зажатой клавишей каждый кадр заново выяснял отношение каждого персонажа к каждому вашему - в городе это сотни персонажей на десятки ваших.

Здесь отношение запоминается и пересчитывается раз в секунду, цвет записывается только при изменении, а пока клавиша не зажата, мод не делает ничего.

Уживается с Vital Sense: его окраска лежащих и разыскиваемых остаётся поверх.

""" + settings_ru("Character Highlight", "CharacterHighlight", "перезапуск не нужен."), """[h2]WHAT IT DOES[/h2]

While the key is held (ALT by default), characters are tinted by how they stand to you:

[list]
[*][b]your squad[/b] - blue;
[*][b]enemies[/b] - red;
[*][b]allies[/b] - green;
[*][b]neutrals[/b] - red through yellow to green by their faction's relation to you (or one colour);
[*][b]faction diplomats[/b] - purple, whatever their faction's relation; diplomats from mods too.
[/list]

Each kind can be turned off; colours, the key, distance and see-through-walls are adjustable.

[h2]A HEAVILY OPTIMISED VERSION[/h2]

The original ate FPS all the time - even when it highlighted nobody: every frame it walked every character around and reset their tint. With the key held, every frame it worked out again how every character stands to every one of yours - in a town that is hundreds of characters times dozens of yours.

Here the relation is remembered and re-checked once a second, the colour is written only when it changes, and while the key is not held the mod does nothing.

Works together with Vital Sense: its tint of downed and wanted characters stays on top.

""" + settings_en("Character Highlight", "CharacterHighlight", "no restart needed."))

other_fork("Formations", """[h2]ЧТО ДЕЛАЕТ[/h2]

Строй как в Bannerlord. Выделите бойцов, зажмите модификатор (по умолчанию ALT) и:

[list]
[*][b]протяните правой кнопкой по земле[/b] - отряд встанет фигурой вдоль линии;
[*][b]щёлкните правой кнопкой[/b] - марш: к точке ведёт самый медленный, остальные идут строем за ним и на месте встают фигурой, никто не отстаёт;
[*][b]клавиши фигур[/b] - линия, круг, прямоугольник, клин, каре, ромб, колонна по двое, клещи, алмаз, мотылёк, шеврон (1-4, 6-9, 0, -, =); 5 - плотный или свободный строй;
[*][b]V[/b] - точка сбора (флаг виден, пока зажат модификатор), [b]X[/b] - отход к ней, по пути подбирая раненых. Бойцы с работой «Медик» сами относят раненых к точке сбора и лечат.
[/list]

[h2]СТРОЙ ПРИ СЛЕДОВАНИИ[/h2]

В игре отряд бежит за ведущим одним из трёх построений: случайным, военным или караванным. Здесь к ним добавлены ещё одиннадцать - алмаз, каре, клещи, клин, колонна по двое, кольцо, линия, мотылёк, ромб, шеврон. Список на вкладке ИИ в окне карты задаёт строй для всех отрядов, а в Mod Config Menu каждому отряду можно дать свой.

Бежит отряд сам, как в обычной игре: мод меняет только то, где каждый боец держится относительно ведущего. Выбор сохраняется вместе с сейвом.

[h2]НАСТРОЙКИ[/h2]

Все настройки - прямо в игре, если установлен Mod Config Menu (не обязателен): окно «Настройки» → вкладка MCM → «Formations». Модификатор (любая клавиша или сочетание), интервал, фигура по умолчанию, свободный строй, «Держать» после построения, строй на марше, клавиши фигур и строй каждого отряда. Изменения применяются сразу; свой строй отряда - с Mod Config Menu версии 7 и новее. Без MCM - файл Formations_settings.txt в папке мода.

[h2]СОВМЕСТИМОСТЬ[/h2]

Данных игры не меняет, работает с любым списком модов и любым сейвом. Если мод убрать, отряды вернутся к обычным построениям игры.

Исходники этой версии - [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks/tree/main/Formations]GitHub[/url].
""", """[h2]WHAT IT DOES[/h2]

Bannerlord-style formations. Select your people, hold the modifier (ALT by default) and:

[list]
[*][b]right-drag across the ground[/b] - the squad lines up in a shape along the line;
[*][b]right-click[/b] - a march: the slowest leads to the point, the rest follow in formation and form up there; nobody is left behind;
[*][b]shape keys[/b] - line, circle, rectangle, wedge, square, rhombus, column of two, pincers, diamond, moth, chevron (1-4, 6-9, 0, -, =); 5 - tight or loose ranks;
[*][b]V[/b] - a rally point (its flag shows while the modifier is held), [b]X[/b] - retreat to it, picking up the wounded on the way. People with the Medic job carry the wounded to the rally point on their own and heal them.
[/list]

[h2]FORMATIONS WHILE FOLLOWING[/h2]

In the game a squad follows its leader in one of three layouts: random, military or caravan. This adds eleven more - chevron, column of two, diamond, line, moth, pincers, rhombus, ring, square, wedge. The list on the AI tab of the map window sets the formation for every squad, and Mod Config Menu can give each squad its own.

The squad moves by itself, as in the plain game: the mod only changes where each fighter keeps relative to the leader. The choice is saved with the game.

[h2]SETTINGS[/h2]

Every setting is right in the game with Mod Config Menu installed (optional): Options window → MCM tab → "Formations". The modifier (any key or combination), spacing, default shape, loose ranks, Hold after forming up, marching in formation, the shape keys and each squad's formation. Changes apply at once; a squad's own formation needs Mod Config Menu version 7 or newer. Without MCM - Formations_settings.txt in the mod folder.

[h2]COMPATIBILITY[/h2]

No game data is changed; works with any mod list and any save. Remove the mod and squads go back to the game's own layouts.

The source of this version is on [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks/tree/main/Formations]GitHub[/url].
""")

other_fork("BetterLooting", """[h2]ЧТО ДЕЛАЕТ[/h2]

Собирает добычу одной клавишей: откройте инвентарь персонажа (или труп, сундук) и нажмите Insert - выбранный персонаж заберёт всё подходящее вокруг себя, без щелчков по каждому телу.

[list]
[*]берутся только вещи дороже заданной цены и с хорошим отношением цены к весу - хлам остаётся лежать;
[*]категории на выбор: оружие, броня, материалы, еда, медикаменты, инструменты, боеприпасы, чертежи, книги, робототехника, наркотики, отрубленные конечности, прочее;
[*]сначала самое ценное - в полный рюкзак попадёт лучшее; можно заполнять сначала рюкзак;
[*]краденое - по желанию; краденым считается только взятое у живого без сознания;
[*]три готовых профиля: «баланс», «деньги», «ресурсы».
[/list]

[h2]РАЗЛОЖИТЬ ПО ХРАНИЛИЩАМ[/h2]

ALT+INSERT - персонаж раскладывает свои вещи (кроме надетого) по ближайшим хранилищам вашей базы, которые их принимают: руду - в ящик для руды, доспехи - в шкаф для доспехов, остальное - в сундуки. Не нашлось места - вещь остаётся у персонажа. По желанию раскладываются только вещи, подходящие под правила сбора.

[h2]НАСТРОЙКИ[/h2]

Все настройки - прямо в игре, если установлен Mod Config Menu (не обязателен): окно «Настройки» → вкладка MCM → «Better Looting». Клавиши сбора и раскладки, радиус, цены, категории и профили; изменения применяются сразу. Без MCM - файл BetterLooting.ini в папке мода (появится после первого сохранения настроек; без него действуют значения по умолчанию: Insert, 20 м, от 500 и от 300 за кг).
""", """[h2]WHAT IT DOES[/h2]

Loot with one key: open your character's inventory (or a body, a chest) and press Insert - the selected character picks up everything suitable around them, no clicking through every body.

[list]
[*]only items above a set price and with a good value to weight ratio are taken - junk stays on the ground;
[*]categories to choose: weapons, armour, materials, food, medicine, tools, ammo, blueprints, books, robotics, narcotics, severed limbs, other;
[*]the most valuable first - a full backpack holds the best finds; the backpack can be filled first;
[*]stolen goods - optional; only things taken from a living unconscious person count as stolen;
[*]three ready profiles: "balanced", "money", "resources".
[/list]

[h2]DEPOSIT TO STORAGE[/h2]

ALT+INSERT - the character puts their items (except what is worn) away into the nearest storage of your base that accepts them: ore into the ore box, armour into the armour storage, the rest into chests. No room - the item stays with the character. Optionally only items that pass the looting rules are put away.

[h2]SETTINGS[/h2]

Every setting is right in the game with Mod Config Menu installed (optional): Options window → MCM tab → "Better Looting". Looting and deposit keys, radius, prices, categories and profiles; changes apply at once. Without MCM - BetterLooting.ini in the mod folder (it appears once the settings are saved; without it the defaults apply: Insert, 20 m, from 500 and from 300 per kg).
""")


other_fork("KenshiJobManagement", """[h2]ЧТО ДЕЛАЕТ[/h2]

Одно окно для всех постоянных задач отряда: кто что делает, в каком порядке и кто где работает. Открывается клавишей Ctrl+J или кнопкой JM рядом с кнопкой «Работа» на нижней панели; пока окно открыто, игра стоит на паузе.

[list]
[*][b]Задачи отряда[/b] - очередь каждого персонажа карточками по приоритетам. Перетаскивание, копирование и вставка, выбор нескольких задач и персонажей (Ctrl), очистка очереди одной кнопкой;
[*][b]все отряды[/b] - переключение по TAB или кнопками внизу, отряды сворачиваются;
[*][b]лечение одной кнопкой[/b] - добавить недостающие «Медик», «Робототехника», «Найти и спасти», «Найти и нести в койку», «Шина» и поднять лечение вверх очереди;
[*][b]удалить недействительные[/b] - убрать задачи, которые уже нельзя выполнить (например, залежи после импорта);
[*][b]станции[/b] - все рабочие места базы сеткой по категориям: название, район, сколько назначено и что мешает работе (например, нет энергии);
[*][b]назначение работников[/b] - щелчок по станции открывает окно: кто назначен, кто может работать, работники отсортированы по нужному навыку.
[/list]

[h2]НАСТРОЙКИ[/h2]

Если установлен Mod Config Menu (не обязателен): окно «Настройки» → вкладка MCM → «Kenshi Job Management». Включение мода, клавиша окна, кнопка на панели и размер окна. Без MCM - файл KenshiJobManagement.ini в папке мода. Какие категории станций показывать - кнопка «Настройки» в самом окне.

[h2]ВАЖНО[/h2]

[list]
[*]не включайте вместе с оригиналом - у них одна папка и одно окно;
[*]сохранитесь перед первым использованием: кнопки отмены нет.
[/list]
""", """[h2]WHAT IT DOES[/h2]

One window for all permanent jobs of your squads: who does what, in which order and who works where. Opens with Ctrl+J or the JM button next to the Jobs button on the bottom panel; the game is paused while the window is open.

[list]
[*][b]Squad jobs[/b] - each character's queue as cards by priority. Drag and drop, copy and paste, several jobs and characters at once (Ctrl), clear a queue with one button;
[*][b]all squads[/b] - switch with TAB or the buttons at the bottom, squads collapse;
[*][b]healing in one click[/b] - add the missing Medic, Robotics, Rescue, Put in Bed and Splint jobs and move healing to the top of the queue;
[*][b]remove invalid jobs[/b] - drop jobs that can no longer be done (for example, resource nodes after an import);
[*][b]stations[/b] - every workstation of your base in a grid by category: name, area, how many are assigned and what stops the work (for example, no power);
[*][b]assigning workers[/b] - click a station to open a window: who is assigned, who can work there, workers sorted by the skill it needs.
[/list]

[h2]SETTINGS[/h2]

With Mod Config Menu installed (optional): Options window → MCM tab → "Kenshi Job Management". Mod on/off, the open key, the panel button and the window size. Without MCM - KenshiJobManagement.ini in the mod folder. Which station categories are shown - the Options button in the window itself.

[h2]IMPORTANT[/h2]

[list]
[*]do not enable it together with the original - they share the folder and the window;
[*]save before the first use: there is no undo button.
[/list]
""")



# Prepared 29.10.2026, not published by this script.
for _mod, _ru, _en in [
    ("PatientSelfAid", "Также ремонтирует повреждённые протезы у биологических пациентов, если у пациента есть ремкомплект. Все настройки MCM действуют сразу.", "Also repairs damaged prosthetics on biological patients with their own repair kit. All MCM settings apply immediately."),
    ("AssignedWorkers", "Красная молния над выключенными достроенными машинами игрока по той же клавише ALT. В MCM можно включить отметки нехватки питания и настроить размер, дальность и высоту значка.", "A red lightning bolt above switched-off completed player machines, using the same ALT key. MCM also offers missing-power warnings and icon size, range and height."),
    ("RecruitMarkers", "Золотая звезда над лидерами отрядов. Общая клавиша и дальность со значками найма и торговцев; размер звезды настраивается отдельно (по умолчанию 20 пикселей). Показ лидеров игрока и цвет звезды - в MCM.", "A gold star above squad leaders, with the same show key and range as recruit and trader markers. Star size is independent (20 pixels by default). MCM controls player-leader visibility and star colour."),
]:
    _a, _b = T[_mod]
    if _mod in ("PatientSelfAid", "AssignedWorkers"):
        _a = _a.replace("«Включено» действует после перезапуска игры.", "перезапуск не нужен.")
        _b = _b.replace('"Enabled" takes effect after a game restart.', 'no restart needed.')
    T[_mod] = (_a.rstrip()+"\n\n"+_ru+"\n", _b.rstrip()+"\n\n"+_en+"\n")


# Player descriptions maintained together for the current QoL release.
import json
with io.open(os.path.join(HERE, "qol-20261029.json"), encoding="utf-8") as _qol_file:
    for _mod, _pair in json.load(_qol_file).items():
        T[_mod] = (_pair["ru"], _pair["en"])

def unwrap(text):
    """Склеить ручные переносы внутри абзацев (README -> одна строка на абзац).
    Строки списков и с отступом не склеиваются."""
    out = []
    for line in text.split("\n"):
        s = line.rstrip()
        prev = out[-1] if out else ""
        glue = (s and prev and not prev.startswith("[h2]") and not s.startswith(("[h2]", "  ", "* ", "- "))
                and not prev.startswith(("  ", "* ", "- ")) and not re.match(r"^\d+\.", s))
        if glue:
            out[-1] = prev + " " + s.strip()
        else:
            out.append(s)
    return "\n".join(out)


def mcm():
    """ModConfigMenu - README целиком (с разделом для авторов), абзацами."""
    base = os.path.join(build.HERE, "ModConfigMenu")
    ru = io.open(os.path.join(base, "description_ru.txt"), encoding="utf-8").read()
    en = io.open(os.path.join(base, "description_en.txt"), encoding="utf-8").read()
    # шапку (Требуется RE_Kenshi, версия) build.py добавит сам
    cut = lambda t: t[t.index("[h2]"):]
    return unwrap(cut(ru)), unwrap(cut(en))


if __name__ == "__main__":
    only = set(sys.argv[1:])
    for mod, (ru, en) in sorted(T.items()):
        if only and mod not in only:
            continue
        for lang, text in (("ru", ru), ("en", en)):
            io.open(os.path.join(HERE, "%s.%s.txt" % (mod, lang)), "w", encoding="utf-8",
                    newline="\n").write(text.strip() + "\n")
        print("%-16s ru %5d  en %5d" % (mod, len(ru.encode("utf-8")), len(en.encode("utf-8"))))
