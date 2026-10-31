# -*- coding: utf-8 -*-
"""Подготовка наших модов к загрузке в Steam Workshop.

Для каждого мода в workshop\\<Mod>\\:
  content\\       - то, что уйдёт в Мастерскую (копия папки мода без
                   служебных файлов FCS и бэкапов);
  preview.png     - главная картинка (первая из галереи Nexus);
  screens\\       - остальные картинки галереи;
  description.txt - описание в BBCode Steam (<= 7900 байт);
  item.json       - название, теги, версия, ID на Nexus и в Мастерской.

Сверка с Nexus - nexus_compare.py (файлы совпадают по MD5).
"""
import io
import json
import os
import re
import shutil

HERE = os.path.dirname(os.path.abspath(__file__))
MODS = r"D:\SteamLibrary\steamapps\common\Kenshi\mods"
OURS = r"D:\DEV\Kenshi\ours"
# Mod Config Menu - ссылка в шапке описания всех остальных модов.
MCM_STEAM_URL = "https://steamcommunity.com/sharedfiles/filedetails/?id=3813761584"
MCM_NEXUS_URL = "https://www.nexusmods.com/kenshi/mods/2202"
WORKSHOP = r"D:\SteamLibrary\steamapps\workshop\content\233860"


def mod_dir(mod):
    """Папка мода: Kenshi/mods/<Mod> или папка Мастерской - её именем
    (туда пользователь переносит моды сам) или подпапка-номер с <Mod>.mod.
    Нет нигде - None."""
    # stage - папки форков, удалённых из игры (tools/stage_forks.py)
    for root in (MODS, WORKSHOP, r"D:\DEV\Kenshi\stage"):
        p = os.path.join(root, mod)
        if os.path.isdir(p):
            return p
    if os.path.isdir(WORKSHOP):
        for d in os.listdir(WORKSHOP):
            p = os.path.join(WORKSHOP, d)
            if os.path.isfile(os.path.join(p, mod + ".mod")):
                return p
    return None
LIMIT = 7900   # у Steam 8000; запас

ITEMS = [
    dict(mod="LiveXpBars", title="Live Xp Bars (QoL)", nexus=2188, version="11",
         tags=["GUI", "RE_Kenshi"], lang=True,
         drop=["ТРИ МЕСТА, ГДЕ ЛЕГКО ОШИБИТЬСЯ", "КАК ОПРЕДЕЛЯЕТСЯ РОСТ"],
         en="Live XP bars for the skills that are levelling right now: name, "
            "level, progress to the next level. Nothing to pick or click - a "
            "bar appears when a skill grows and fades out when it stops."),
    dict(mod="StatColours", title="Stat Colours (QoL)", nexus=2189, version="12",
         tags=["GUI", "RE_Kenshi"], lang=True,
         drop=["ЗАЧЕМ ОН, ЕСЛИ ЕСТЬ StatsColor"],
         en="Colours the numbers in the character stats window by value, shows "
            "what gear, injuries and fighting indoors add to a skill, racial XP "
            "multipliers (hold ALT) and the current XP rate of the selected "
            "skill."),
    dict(mod="GearCompare", title="Gear Compare Alt (QoL)", nexus=2190, version="7",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Hover armour or a weapon and hold ALT: a second panel next to the "
            "tooltip shows what is worn in that slot now, what changes, and a "
            "verdict - better, worse or mixed."),
    dict(mod="AssignedWorkers", title="Assigned Workers (QoL)", nexus=2191, version="7",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en='See who is assigned to a building, select idle characters and spot switched-off or unpowered player machines by red lightning markers.'),
    dict(mod="BestMinimap", title="Best Minimap (QoL)", nexus=2242, version="2",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="A minimap in the corner of the screen on the game's own map picture: "
            "characters nearby, discovered towns, attack alerts, Map-markers and "
            "WantedMap marks, camera range; right click to send the selected, "
            "double click to move the camera."),
    dict(mod="UITweaks", title="UI Tweaks (QoL)", nexus=2192, version="12",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en='Interface fixes for queues, descriptions, squad panels and tooltips, plus fewer redundant name-label and status-bar updates. Works with vanilla and modded interfaces.'),
    dict(mod="PatientSelfAid", title="Patient Self Aid (QoL)", nexus=2193, version="4",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en='Let a conscious patient help with their own treatment, including repair of robotic limbs on biological characters.'),
    # --- новые, 05.10.2026 (Nexus 2201-2205) ---
    dict(mod="ModConfigMenu", title="Mod Config Menu (QoL)", nexus=2202, version="10",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en='In-game settings for RE_Kenshi plugins, with tooltips, page scrolling and keyboard or mouse binding support.'),
    dict(mod="QuickStartSelect", title="Quick Start Select (QoL)", nexus=2201, version="6",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="The 'Choose your beginning' window as a list of all starts: two "
            "layouts, sorting by name and difficulty, search, race filter, the "
            "game's advanced options inside with right-click reset."),
    dict(mod="WantedMap", title="Wanted Map (QoL)", nexus=2203, version="6",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Select a character carrying 'Wanted' posters and the world map "
            "shows where the targets are: exact, approximate, at their home "
            "town, imprisoned or recently dead."),
    dict(mod="BuildModeGizmo", title="Build Mode Gizmo (QoL)", nexus=2204, version="4",
         tags=["Gameplay", "RE_Kenshi"], lang=True,
         drop=["СТАРЫЙ РЕЖИМ: МЕНЮ РАЗРАБОТЧИКА", "КАК СДЕЛАНО",
               "ТЕХНИЧЕСКОЕ: ДОСТУП К НЕЭКСПОРТИРОВАННЫМ ФУНКЦИЯМ",
               "ПОЧЕМУ ПРАВКА ПОКРЫВАЕТ СРАЗУ ДВА СЛУЧАЯ"],
         en="Select your own building site that is not built yet and move or "
            "rotate it with the editor's arrows - no dismantling, no developer "
            "menu."),
    dict(mod="RangedDamageXp", title="Ranged Damage Xp (QoL)", nexus=2205, version="2",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en="Crossbow (and turret) experience for the damage a hit actually "
            "dealt after armour, on top of the vanilla XP for shooting; a share "
            "goes to Perception."),
    # --- форки модов Emkej (оригиналы - Emkej8 на Nexus), настройки в MCM.
    # Наши страницы на Nexus - 2206-2215 (05.10.2026).
    # Описания - desc/make_desc.py (шапка fork_header: автор и что изменено).
    dict(mod="Wall-B-Gone", title="Wall B Gone (MCM fork) (QoL)", nexus=2215, version="4",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en="A key to dismantle your walls, beds and furniture - a fork of Emkej8's mod "
            "with settings in Mod Config Menu."),
    dict(mod="Auto-Pause-on-Load", title="Auto-Pause on Load (MCM fork) (QoL)", nexus=2206, version="5",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en="Pauses the game after a save loads, on trade and on inventory - a fork of "
            "Emkej8's mod with settings in Mod Config Menu."),
    dict(mod="Loot-Scoot-Execute", title="Loot Scoot Execute (MCM fork) (QoL)", nexus=2210, version="5",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en="Execute and Execute All for downed enemies in the context menu - a fork of "
            "Emkej8's mod with settings in Mod Config Menu."),
    dict(mod="Vital-Sense", title="Vital Sense (MCM fork) (QoL)", nexus=2214, version="6",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Hold ALT for state and bounty markers over visible characters - a fork of "
            "Emkej8's mod with settings in Mod Config Menu."),
    dict(mod="Organize-the-Trader", title="Organize the Trader (MCM fork) (QoL)", nexus=2212, version="6",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Search and sorting in the trader window - a fork of Emkej8's mod with settings "
            "in Mod Config Menu."),
    dict(mod="Vital-Read", title="Vital Read (MCM fork) (QoL)", nexus=2213, version="5",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="State icons on squad portraits: downed, dying, starving, crippled - a fork of "
            "Emkej8's mod with settings in Mod Config Menu."),
    dict(mod="Container-Highlight", title="Container Highlight (MCM fork) (QoL)", nexus=2207, version="2",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Hold a key to mark nearby lootable containers - a fork of Emkej8's mod with "
            "settings in Mod Config Menu."),
    dict(mod="Hidden-Faction-Relations", title="Hidden Faction Relations (MCM fork) (QoL)", nexus=2208,
         version="2", tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Your relations with every faction, hidden ones included, in the Options window - "
            "a fork of Emkej8's mod with settings in Mod Config Menu."),
    dict(mod="Job-B-Gone", title="Job B Gone (MCM fork) (QoL)", nexus=2209, version="5",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="A panel to clear job queues for yourself, the selection, the squad or everyone - "
            "a fork of Emkej8's mod with settings in Mod Config Menu."),
    # --- форк BetterLooting (XxAtreuSSxX, GitHub, GPLv3); страница в Steam
    # автором снята, на Nexus - чужая перезаливка 2178. nexus - после
    # нашей выкладки на Nexus.
    dict(mod="BetterLooting", title="Better Looting (MCM fork) (QoL)", nexus=2218, version="4",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en='Rule-based looting, required-item lists, protected player storage, optional ground pickup and deposit to base storage.'),
    dict(mod="Map-markers", title="Map markers (MCM fork) (QoL)", nexus=2211, version="3",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Your own markers with labels on the world map, per save - a fork of Emkej8's "
            "mod with settings in Mod Config Menu."),
    # nexus - после нашей выкладки на Nexus (08.10.2026: готовится).
    dict(mod="Organize-the-Crafting-Stations", title="Organize the Crafting Stations (MCM fork) (QoL)",
         nexus=2219, version="1", tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="A search bar in crafting and research windows - a fork of Emkej8's mod with "
            "settings in Mod Config Menu."),
    # --- форк Kenshi Job Management (SirixKC, GitHub, GPLv3; весь код - Codex).
    # Папка мода та же, что у оригинала (Steam 3785306982) - вместе не включать.
    # nexus - после нашей выкладки на Nexus.
    dict(mod="KenshiJobManagement", title="Kenshi Job Management (MCM fork) (QoL)", nexus=2225,
         version="4", tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="A window for squad job queues and work stations: who does what, in which order and "
            "who works where. A fork of SirixKC's mod with the game's own interface, translations "
            "and settings in Mod Config Menu."),
    # --- интерфейс без DLL: гибрид Dark UI (Eldryn) и Russian Dark UI simplistic (FG)
    # с нашими правками разметки (fix_darkui_*.py). До 16.10.2026 лежал у
    # пользователя как «Russian Dark UI simplistic».
    dict(mod="Dark UI Advanced", title="Dark UI Advanced", nexus=2213, version="2",
         tags=["GUI"], lang=False, plugin=False, drop=[],
         en="A dark interface for Kenshi - a hybrid of Dark UI by Eldryn and Russian Dark UI "
            "simplistic by FG, with layout fixes. Works with any game language."),
    dict(mod="ItemMarkers", title="Item Markers (QoL)", nexus=2224, version="5",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Item grade colours, stolen goods and faction uniforms right on the item icons - "
            "a heavily optimised replacement for LtEast's Rarity Backgrounds and Item Status "
            "Indicators."),
    # --- форк Character Highlight (BFrizzleFoShizzle, KenshiLib_Examples, GPLv3).
    dict(mod="Character-Highlight", title="Character Highlight (MCM fork) (QoL)", nexus=2233,
         version="2", tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en="Hold ALT to tint characters by how they stand to you - squad, enemies, allies, "
            "neutrals, diplomats. A heavily optimised fork of BFrizzleFoShizzle's mod with "
            "settings in Mod Config Menu."),
    # Форк Formations от Slinkysalmon03 (исходники шли с модом в Мастерской, GPLv3) - 17.10.2026.
    dict(mod="Formations", title="Formations Extended (MCM fork) (QoL)", nexus=2235, version="2",
         tags=["Gameplay", "RE_Kenshi"], lang=True, drop=[],
         en="Bannerlord-style squad formations: drag a shape, march in order, and keep a formation "
            "while following the leader - 13 follow shapes, a shape of its own for every squad. A fork of "
            "Slinkysalmon03's Formations with settings in Mod Config Menu."),
    dict(mod="RecruitMarkers", title="Recruit Markers (QoL)", nexus=2232, version="4",
         tags=["GUI", "RE_Kenshi"], lang=True, drop=[],
         en='Recruit bubbles, trader coins and gold squad-leader stars, with a shared show key and independent star size.'),
]

ITEMS.extend([{'mod': 'SkillLevelUpInfo', 'title': 'Skill Level Up Info (QoL)', 'nexus': 2257, 'version': '2', 'tags': ['GUI', 'RE_Kenshi'], 'lang': True, 'drop': [], 'en': 'Full XP Info displays: blue experience gains, gold level increases and recent-training bars, configured through MCM.'}, {'mod': 'MultiXPMining', 'title': 'Multi XP Mining (QoL)', 'nexus': 2258, 'version': '1', 'tags': ['Gameplay', 'RE_Kenshi'], 'lang': True, 'drop': [], 'en': 'Extra Strength, Toughness and Dexterity training from actual mining work. Detects mining sources from all loaded mods.'}, {'mod': 'KillCounter', 'title': 'Kill Counter (MCM fork) (QoL)', 'nexus': 2261, 'version': '1', 'tags': ['GUI', 'Gameplay', 'RE_Kenshi'], 'lang': True, 'drop': [], 'en': 'Kills, knockouts and severed-limb counters in the character stats window, with optional rows and in-game settings.'}, {'mod': 'Character Inspector', 'title': 'Character Inspector (MCM fork) (QoL)', 'nexus': 2259, 'version': '2', 'tags': ['GUI', 'RE_Kenshi'], 'lang': True, 'drop': [], 'en': 'Inspect body-part armour coverage, protective layers and environmental resistance using the inventory STATS button.'}, {'mod': 'KenshiRotate', 'title': 'Rotate Items (MCM fork) (QoL)', 'nexus': 2260, 'version': '1', 'tags': ['GUI', 'Gameplay', 'RE_Kenshi'], 'lang': True, 'drop': [], 'en': 'Rotate inventory and backpack items repeatedly with one keyboard or mouse binding. Rotations remain saved.'}])

ITEMS.append(dict(mod="StackSort",title="Stack Sort (MCM fork) (QoL)",nexus=2262,version="2",tags=["GUI","Gameplay","RE_Kenshi"],lang=True,drop=[],en="Pack inventories, consolidate compatible stacks and reserve free space, with Rotate Items support and MCM settings."))

SKIP = re.compile(r"(^_.*\.(info|img)$)|(\.bak$)|(\.pre[^.]*$)|(\.prelog$)|"
                  r"(\.bak-[^.]*$)|(\.log$)|(_runtime_table\.txt$)|"
                  # mod-config.json у форков Emkej - личные настройки; плагин
                  # сам создаст его по умолчанию (mod-config.defaults.json едет)
                  r"(^mod-config\.json$)|"
                  # BetterLooting.ini - тоже личные настройки; без него плагин
                  # берёт свои значения по умолчанию и сам создаст файл
                  r"(^BetterLooting\.ini$)|"
                  # отчёт кнопки «Нарисовать иконки всех вещей» (UITweaks)
                  r"(^icons_failed\.txt$)|"
                  # Formations: личные настройки (плагин сам создаст файл) и журнал
                  r"(^Formations_settings\.txt$)|(_log\.txt$)|"
                  # копии, сделанные руками в Проводнике («Файл — копия.png»)
                  r"( [—-] копия[^.]*\.)|( - Copy[^.]*\.)|"
                  # любые архивы в папке мода: исходники больше не раздаём (они на
                  # GitHub, 09.10.2026); у Job-B-Gone/Map-markers лежал zip всего репо
                  r"(\.zip$)|(^UnknownSources\.ini$)|(^99_UserOverride\.ini$)|"
                  r"(\.(cpp|h|vcxproj|sln)$)", re.I)
# Папки, которые плагин пишет сам по ходу игры: WantedMap\memory - память
# о прохождениях игрока. В раздачу не идут.
# ModConfigMenu\strings - выгрузка подписей при Debug=1 (служебное).
SKIP_DIRS = set(["memory", "strings"])



# One runtime catalog per language: prefer compiled MO when it exists.
# PO-only upstream plugins retain PO until their loader is migrated.

ITEMS.append(dict(mod="AIFixed",title="AI Fixed (QoL)",nexus=2256,version="1",tags=["Gameplay","RE_Kenshi"],lang=True,drop=[],en="Focused repair-bed AI fixes and optional automatic wear repair for skeletons and characters with prosthetics."))

def runtime_file_allowed(mod, folder, name):
    rel = os.path.relpath(os.path.join(folder, name), mod_dir(mod)).replace(os.sep, "/")
    if mod == "KenshiRotate" and rel.startswith("lang/"):
        return False
    if name.lower().endswith(".po") and os.path.isfile(os.path.join(folder, name[:-3] + ".mo")):
        return False
    if name.lower().endswith(".pot"):
        return False
    if name.lower() == "readme.md":
        other=os.path.join(folder,"README.txt")
        if os.path.isfile(other) and open(other,"rb").read().replace(b"\r\n",b"\n") == open(os.path.join(folder,name),"rb").read().replace(b"\r\n",b"\n"):
            return False
    if mod == "ModConfigMenu" and rel.startswith("sdk/") and name.lower().endswith(".h"):
        return True  # public integration header, not plugin implementation
    return not SKIP.search(name)

def source_dir(mod):
    for folder in (os.path.join(OURS,mod), os.path.join(FORKS,"CharacterInspector" if mod=="Character Inspector" else mod)):
        if os.path.isdir(folder):
            return folder
    return None


def heading(line):
    # Заголовок раздела README: с начала строки, кириллица только
    # заглавная (латиница может быть любой: «ЗАЧЕМ ОН, ЕСЛИ ЕСТЬ XpBars»).
    if not line or line[0] in " 	*-[":
        return False
    s = line.strip()
    return (len(re.findall(u"[А-ЯЁ]", s)) >= 4 and not re.search(u"[а-яё]", s)
            and "=" not in s)


def readme_to_bbcode(text, drop, overrides=None):
    """README -> BBCode. overrides: {заголовок: текст} - раздел целиком
    берётся из workshop\\ru_sections\\<Mod>\\<заголовок>.txt, а не из README
    (README описывает нашу сборку, описание - то, что увидят все)."""
    overrides = overrides or {}
    lines = text.replace("\r\n", "\n").split("\n")
    # Первая строка - заголовок мода, затем «Требуется RE_Kenshi.» - они
    # уже есть в шапке.
    body = lines[1:]
    out = []
    skipping = False
    for ln in body:
        if ln.strip() == u"Требуется RE_Kenshi.":
            continue
        if heading(ln):
            name = ln.strip()
            skipping = name in drop or name in overrides
            if name not in drop:
                out.append("[h2]%s[/h2]" % name)
            if name in overrides:
                out.append("")
                out.append(overrides[name].strip("\n"))
                out.append("")
            continue
        if not skipping:
            out.append(ln)
    s = "\n".join(out)
    s = re.sub(r"\n{3,}", "\n\n", s).strip("\n")
    return s


MAX_IMG = 1000 * 1024


def steam_preview_source(mod, landscape_source):
    """Dedicated Steam artwork never replaces the original Nexus/gallery file."""
    from PIL import Image
    square = os.path.join(HERE, "steam-covers", mod, "000_LEOPARD_Steam_Cover.png")
    if not os.path.isfile(square):
        return landscape_source
    with Image.open(square) as image:
        if image.width != image.height:
            raise ValueError("Steam preview override must be square: " + square)
    return square


def prepare_image(src, target_noext):
    """Скопировать картинку; тяжёлую - ужать. Возвращает имя файла."""
    from PIL import Image
    if os.path.getsize(src) <= MAX_IMG and src.lower().endswith(".png"):
        shutil.copy2(src, target_noext + ".png")
        return os.path.basename(target_noext) + ".png"
    im = Image.open(src).convert("RGB")
    if im.width > 1920:
        im = im.resize((1920, int(im.height * 1920.0 / im.width)), Image.LANCZOS)
    q = 92
    while True:
        im.save(target_noext + ".jpg", "JPEG", quality=q, optimize=True)
        if os.path.getsize(target_noext + ".jpg") <= MAX_IMG or q <= 60:
            break
        q -= 8
    return os.path.basename(target_noext) + ".jpg"


# В шрифтах игры только ASCII и кириллица: «», —, …, № и прочее рисуются
# пустым местом (09.10.2026 - снова «ёлочки» в подсказках MCM). Переводы
# ru/en проверяем перед каждой упаковкой; китайский - своими шрифтами.
GLYPH_BAD = re.compile(u"[^\x00-\x7f\u0400-\u04ff]")


def check_glyphs(mod, src):
    import gettext
    bad = []
    for root, _dirs, names in os.walk(src):
        if not ("ru_RU" in root or "en_GB" in root):
            continue
        for name in names:
            # Releases contain MO only; checking PO alone silently skipped all
            # runtime translations after PO sources were removed from packages.
            if name.endswith(".mo"):
                path = os.path.join(root, name)
                with open(path, "rb") as stream:
                    catalog = gettext.GNUTranslations(stream)._catalog
                for key, value in catalog.items():
                    if key and isinstance(value, str) and GLYPH_BAD.search(value):
                        bad.append("%s: %s" % (os.path.relpath(path, src), "".join(sorted(set(GLYPH_BAD.findall(value))))))
                continue
            if not name.endswith(".po"):
                continue
            path = os.path.join(root, name)
            for n, line in enumerate(io.open(path, encoding="utf-8-sig").read().splitlines(), 1):
                if line.startswith("msgstr") and GLYPH_BAD.search(line):
                    bad.append("%s:%d  %s" % (os.path.relpath(path, src), n, "".join(sorted(set(GLYPH_BAD.findall(line))))))
    if bad:
        raise SystemExit("%s: символы, которых нет в шрифтах игры (заменить на \" - ...):\n  %s" % (mod, "\n  ".join(bad)))


# Языки мода - по папкам его locale (09.10.2026: в шапке было «русском или
# английском», а китайский перевод есть у всех).
LANG_NAMES = [("en_GB", u"английском", "English"), ("ru_RU", u"русском", "Russian"),
              ("zh_CN", u"китайском", "Chinese")]


# Моды без папки locale, у которых тексты на трёх языках зашиты в код.
LANG_BUILT_IN = {"Formations": ["en_GB", "ru_RU", "zh_CN"]}


def mod_languages(mod):
    d = mod_dir(mod)
    loc = os.path.join(d, "locale") if d else ""
    have = set(os.listdir(loc)) if loc and os.path.isdir(loc) else set(LANG_BUILT_IN.get(mod, []))
    langs = [l for l in LANG_NAMES if l[0] in have] or LANG_NAMES[:2]
    def join(words, last):
        return words[0] if len(words) == 1 else ", ".join(words[:-1]) + " " + last + " " + words[-1]
    return join([l[1] for l in langs], u"или"), join([l[2] for l in langs], "or")


FORKS = r"D:\DEV\Kenshi\forks\LEOPARD-Kenshi-Forks"


def defaults_ini(mod):
    """Эталон настроек по умолчанию - ours/<Mod>/<Mod>.ini (у наших плагинов)
    или forks/<Mod>/mod/<Mod>.ini (форки на IniDefaults, KenshiJobManagement)."""
    base_name = "CharacterInspector" if mod == "Character Inspector" else mod
    for path in (os.path.join(OURS, mod, base_name + ".ini"),
                 os.path.join(FORKS, mod, "mod", base_name + ".ini"),
                 os.path.join(FORKS, "CharacterInspector" if mod == "Character Inspector" else mod, base_name + ".ini")):
        if os.path.isfile(path):
            return path
    return None


def is_player_ini(mod, name):
    """<Mod>.ini в корне мода - настройки игрока; у наших плагинов не раздаём.
    И любой X.ini, рядом с которым лежит X.defaults.ini (16.10.2026: у
    Character-Highlight ini называется CharacterHighlight.ini - по имени DLL,
    а не мода, и личные настройки попали бы в пакет)."""
    if defaults_ini(mod) is not None and name.lower() == (mod + ".ini").lower():
        return True
    d = mod_dir(mod)
    low = name.lower()
    return (bool(d) and low.endswith(".ini") and not low.endswith(".defaults.ini")
            and os.path.isfile(os.path.join(d, name[:-4] + ".defaults.ini")))


def render_descriptions(item):
    """Render the same complete headers/bodies without rebuilding files/screens."""
    mod = item["mod"]
    src = mod_dir(mod)
    readme_path = os.path.join(src, "README.txt") if src else ""
    readme = io.open(readme_path, encoding="utf-8-sig").read() if readme_path and os.path.isfile(readme_path) else ""
    nexus = "https://www.nexusmods.com/kenshi/mods/%d" % item["nexus"] if item.get("nexus") else None
    # Русское описание - README с Nexus, шапка по-русски.
    # plugin=False - мод без DLL (интерфейс): ни RE_Kenshi, ни MCM ему не нужны.
    plugin = item.get("plugin", True)
    ru = [u"[b]Требуется RE_Kenshi.[/b]"] if plugin else []
    # Ссылка на MCM - у всех модов, кроме него самого (просьба игрока в Steam,
    # 05.10.2026: «чтобы новичкам было легко найти»). В Steam - на Мастерскую,
    # для Nexus nexus_desc.py меняет её на страницу Nexus.
    if mod != "ModConfigMenu" and plugin:
        ru.append(u"Настройки прямо в игре - с модом [url=%s]Mod Config Menu[/url] (не обязателен)." % MCM_STEAM_URL)
    if item["lang"]:
        ru.append(u"[i]Текст в игре - на языке игры: %s.[/i]" % mod_languages(mod)[0])
    ru += ["", (u"Версия %s · Nexus: %s" % (item["version"], nexus)) if nexus
           else (u"Версия %s" % item["version"]), ""]
    overrides = {}
    odir = os.path.join(HERE, "ru_sections", mod)
    if os.path.isdir(odir):
        for f in os.listdir(odir):
            if f.endswith(".txt"):
                overrides[f[:-4]] = io.open(os.path.join(odir, f), encoding="utf-8").read()
    # Текст для игроков (desc/<Mod>.ru.txt, make_desc.py) - если есть; иначе
    # README. README - полный справочник с ключами ini и устройством, игрокам
    # в Steam и на Nexus это не нужно.
    player_ru = os.path.join(HERE, "desc", mod + ".ru.txt")
    if os.path.isfile(player_ru):
        body_ru = io.open(player_ru, encoding="utf-8").read().strip("\n")
    else:
        body_ru = readme_to_bbcode(readme, item["drop"], overrides)
    desc_ru = "\n".join(ru) + "\n" + body_ru

    # Английское - перевод тех же разделов (workshop\\en\\<Mod>.txt).
    en = [("[b]Requires RE_Kenshi.[/b] " if plugin else "") + item["en"]]
    if mod != "ModConfigMenu" and plugin:
        en.append("In-game settings with [url=%s]Mod Config Menu[/url] (optional)." % MCM_STEAM_URL)
    if item["lang"]:
        en.append("[i]In-game text follows the game language: %s.[/i]" % mod_languages(mod)[1])
    en += ["", ("Version %s · Nexus: %s" % (item["version"], nexus)) if nexus
           else ("Version %s" % item["version"]), ""]
    player_en = os.path.join(HERE, "desc", mod + ".en.txt")
    body_en = io.open(player_en if os.path.isfile(player_en) else os.path.join(HERE, "en", mod + ".txt"),
                      encoding="utf-8").read().strip("\n")
    desc_en = "\n".join(en) + "\n" + body_en

    return desc_ru, desc_en


def runtime_source_path(mod, folder, name):
    # StackSort.cfg contains advanced player tuning. Ship the reviewed factory
    # copy rather than republishing a live installation's personal values.
    if mod == "StackSort" and name == "StackSort.cfg" and os.path.normcase(os.path.abspath(folder)) == os.path.normcase(os.path.abspath(mod_dir(mod))):
        factory = os.path.join(source_dir(mod), name)
        if not os.path.isfile(factory):
            raise ValueError("Missing factory StackSort.cfg")
        return factory
    return os.path.join(folder, name)


def build(item):
    mod = item["mod"]
    check_glyphs(mod, mod_dir(mod))
    dst = os.path.join(HERE, mod)
    content = os.path.join(dst, "content")
    if os.path.exists(content):
        shutil.rmtree(content)
    src = mod_dir(mod)
    files = []
    for root, dirs, names in os.walk(src):
        dirs[:] = [d for d in dirs if d.lower() not in SKIP_DIRS]
        for n in names:
            if not runtime_file_allowed(mod, root, n) or (root == src and is_player_ini(mod, n)):
                continue
            rel = os.path.relpath(os.path.join(root, n), src)
            os.makedirs(os.path.join(content, os.path.dirname(rel)), exist_ok=True)
            runtime_source = runtime_source_path(mod, root, n)
            shutil.copy2(runtime_source, os.path.join(content, rel))
            files.append((rel.replace(os.sep, "/"), os.path.getsize(runtime_source)))

    # <Mod>.ini - настройки игрока: не раздаём, иначе обновление их затирает
    # (жалобы игроков, 11.10.2026). Раздаём <Mod>.defaults.ini - значения по
    # умолчанию из исходников; плагин сам создаёт из него <Mod>.ini и
    # дописывает новые ключи (shared/IniDefaults.h).
    src_ini = defaults_ini(mod)
    if src_ini:
        name = ("CharacterInspector" if mod == "Character Inspector" else mod) + ".defaults.ini"
        shutil.copy2(src_ini, os.path.join(content, name))
        files = [f for f in files if f[0] != name] + [(name, os.path.getsize(src_ini))]

    # Картинки: у Steam превью до 1 МБ, скриншоты с экрана весят по 5 МБ.
    # Тяжелее MAX_IMG - в JPEG, не шире 1920. Имена - латиницей
    # (<Mod>_NN), оригиналы в images\ не трогаются.
    idir = os.path.join(HERE, "images", mod)
    imgs = sorted(os.listdir(idir)) if os.path.isdir(idir) else []
    sdir = os.path.join(dst, "screens")
    if os.path.exists(sdir):
        shutil.rmtree(sdir)
    os.makedirs(sdir)
    out_names = []
    for k, f in enumerate(imgs):
        target = os.path.join(dst, "preview") if k == 0 else os.path.join(sdir, "%s_%02d" % (mod, k))
        if k == 0:
            for ext in (".png", ".jpg"):        # прежнее превью другого формата
                if os.path.exists(target + ext):
                    os.remove(target + ext)
        image_source = os.path.join(HERE, "images", mod, f)
        if k == 0:
            image_source = steam_preview_source(mod, image_source)
        name = prepare_image(image_source, target)
        if k > 0:
            out_names.append(name)
    imgs = ([imgs[0]] + out_names) if imgs else [None]

    # У форков Emkej README.txt в папке мода нет (их README - в исходниках);
    # описание тогда только из desc/.
    readme_path = os.path.join(src, "README.txt")
    readme = io.open(readme_path, encoding="utf-8-sig").read() if os.path.isfile(readme_path) else ""
    nexus = "https://www.nexusmods.com/kenshi/mods/%d" % item["nexus"] if item.get("nexus") else None

    desc_ru, desc_en = render_descriptions(item)

    for name, text in (("description_ru.txt", desc_ru), ("description_en.txt", desc_en)):
        io.open(os.path.join(dst, name), "w", encoding="utf-8", newline="\n").write(text)
    old_desc = os.path.join(dst, "description.txt")
    if os.path.exists(old_desc):
        os.remove(old_desc)
    size = max(len(desc_ru.encode("utf-8")), len(desc_en.encode("utf-8")))

    meta_path = os.path.join(dst, "item.json")
    old = json.load(io.open(meta_path, encoding="utf-8")) if os.path.exists(meta_path) else {}
    meta = dict(mod=mod, title=item["title"], tags=item["tags"],
                version=item["version"], nexus=item.get("nexus"),
                workshop_id=old.get("workshop_id", 0),
                visibility=0,
                files=files, screens=imgs[1:], description_bytes=size,
                title_ru=item["title"])
    json.dump(meta, io.open(meta_path, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    return meta


if __name__ == "__main__":
    import sys
    only = set(sys.argv[1:])            # build.py [Mod ...] - только эти
    for it in ITEMS:
        if only and it["mod"] not in only:
            continue
        if mod_dir(it["mod"]) is None:
            print("%-16s нет папки мода в игре - пропущен" % it["mod"])
            continue
        idir = os.path.join(HERE, "images", it["mod"])
        if not os.path.isdir(idir) or not os.listdir(idir):
            print("%-16s нет картинок в images/%s - собрано без них (в Steam рано)" % (it["mod"], it["mod"]))
        m = build(it)
        flag = "" if m["description_bytes"] <= LIMIT else "  <-- СЛИШКОМ ДЛИННОЕ"
        print("%-16s files=%d screens=%d desc=%d bytes%s" % (
            m["mod"], len(m["files"]), len(m["screens"]), m["description_bytes"], flag))
