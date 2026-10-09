# Полная пересборка всех плагинов Kenshi.
#
# Порядок внутри шага правок значения не имеет: исходники всегда остаются
# английскими, перевод живёт в .po рядом с модами. Раньше translate_ru.py
# регенерировал .cpp и стирал правки, отсюда была жёсткая очерёдность —
# с переходом на Localization.h это ушло.
#
#   .\rebuild_all.ps1              полный цикл
#   .\rebuild_all.ps1 -NoDeploy    собрать, но не копировать в игру
param(
    [switch]$NoDeploy
)
$ErrorActionPreference = "Stop"
$ROOT   = "D:\DEV\Kenshi"
$KENSHI = "D:\SteamLibrary\steamapps\common\Kenshi"

function Step($text) { Write-Host "`n=== $text ===" -ForegroundColor Cyan }

$script:BuildFailed = 0

# Папка мода: Kenshi\mods\<Mod>, а если её нет - папка Мастерской Steam.
# Пользователь переносит туда моды сам (05.10.2026 - Hidden-Faction-Relations),
# и раньше такой мод молча переставал обновляться. $null - нигде нет.
$WORKSHOP = "D:\SteamLibrary\steamapps\workshop\content\233860"
function ModDir($modFolder) {
    foreach ($root in @("$KENSHI\mods", $WORKSHOP)) {
        $p = Join-Path $root $modFolder
        if (Test-Path $p) { return $p }
    }
    if (Test-Path $WORKSHOP) {
        foreach ($d in Get-ChildItem $WORKSHOP -Directory) {
            if (Test-Path (Join-Path $d.FullName "$modFolder.mod")) { return $d.FullName }
        }
    }
    return $null
}

function BuildPlugin($srcDir, $name, $modFolder, [switch]$FromVcxproj) {
    $dll = "$ROOT\build\$name\$name.dll"
    # Успех определяем по самому файлу: обновилась ли DLL. Ловить вывод
    # build.ps1 нельзя - он печатает через Write-Host, прямо в консоль,
    # в конвейер не попадает ничего. Раньше Copy-Item стоял безусловно, и
    # при провале сборки в мод уезжала предыдущая DLL, а в журнале при этом
    # значилось «установлен» - ровно так однажды и вышло.
    $before = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
    & "$ROOT\build.ps1" -SrcDir $srcDir -Name $name -FromVcxproj:$FromVcxproj
    $after = if (Test-Path $dll) { (Get-Item $dll).LastWriteTimeUtc } else { [datetime]::MinValue }
    if ($after -le $before) {
        Write-Host "  СБОРКА НЕ УДАЛАСЬ, установка пропущена: $name" -ForegroundColor Red
        $script:BuildFailed++
        return
    }
    if ($NoDeploy) { return }
    $dest = ModDir $modFolder
    if ($null -eq $dest) {
        Write-Host "  папки мода нет, пропускаю установку: $modFolder" -ForegroundColor Yellow
        return
    }
    Copy-Item "$ROOT\build\$name\$name.dll" $dest -Force
    Write-Host "  установлен: $modFolder" -ForegroundColor Green
}

Step "Правки поверх исходников авторов"
# fix_hookid правит KenshiLib и здесь не нужен: он накатывается отдельно,
# после make_kenshilib.ps1. См. README.
$patches = @(
    "fix_sa_taskdata_include",   # SquadAutonomy.h: недостающее подключение AITaskSystem.h
    "fix_firstload",             # загрузка сейва без файла настроек
    "fix_addai_crash",           # вылет по кнопке добавления пакета ИИ
    "fix_dropbox_align",         # положение выпадающего списка
    "feat_weather_resists",      # защита от среды в Character Inspector
    "fix_ci_piececols",          # ширина колонок в строке брони
    "feat_ci_lighttext",         # названия предметов светлым тоном заголовка
    "feat_ci_theme",             # цвета надписей - из палитры игры
    "feat_ci_localization",      # локализация Character Inspector
    "feat_au2942_localization",  # локализация SquadAutonomy и MoreImmersiveBars
    "fix_bl_vc10",               # BetterLooting: приведение к тулсету v100
    "feat_bl_mygui",             # BetterLooting: окно настроек на интерфейс игры
    "feat_kc_localization",      # KillCounter: перевод подписей, свой цвет убран
    "feat_formations_ru",        # Formations: подсказка и пояснения настроек по-русски
    "feat_xpo_slots",            # XP_Overhaul: адреса xp* из таблицы KenshiLib, без Mod Hub
    "fix_xpo_adjustlevel",       # XP_Overhaul: центральная функция из файла exe, без разгона
    "fix_xpo_carry",             # XP_Overhaul: остаток прибавки, не влезший во float
    "feat_xpo_rate_export"       # XP_Overhaul: XPO_CurrentXpRate для подсказки StatColours
)
foreach ($p in $patches) {
    Write-Host ("  {0,-28}" -f $p)
    python "$ROOT\$p.py" | ForEach-Object { Write-Host "      $_" }
}

Step "Правки поверх чужих модов (не исходники, а файлы в игре)"
# Эти моды обновляются через мастерскую и затирают наши правки. Скрипты
# идемпотентны: если правка стоит, они молча это скажут.
$modPatches = @(
    "fix_gec_skin",      # Guild Escort Contracts: центровка и кегль кнопок
    "gec_shorten_ui",    # Guild Escort Contracts: подписи, не влезавшие в кнопки
    "gec_locale",        # он же пересобирает ru.json из порций перевода
    "fix_gec_category",  # Guild Escort Contracts: категория «The Mercenarie» -> «Гильдия»
    "fix_gec_dialogue",  # Guild Escort Contracts: реплики диалогов из .mod по-русски
    "fix_gec_bounty_lang", # Guild Escort Contracts: реплики охоты за наградой - ru вместо en (DLL)
    "fix_darkui_stats",     # Russian Dark UI: обрезанная «Кулинария» в навыках
    "fix_darkui_research",  # Russian Dark UI: тесное описание на вкладке изучения
    "fix_darkui_queuerow",  # Russian Dark UI: перекошенная строка очереди
    "fix_bsl_descriptions", # Broken Skeleton Limbs: русские описания конечностей
    "fix_map_names"         # имена мест на карте до исследования (unexplored name) - по-русски
)
foreach ($p in $modPatches) {
    Write-Host ("  {0,-28}" -f $p)
    python "$ROOT\$p.py" | ForEach-Object { Write-Host "      $_" }
}

Step "Компиляция каталогов перевода (.po -> .mo)"
# Код читает .mo, .po остаётся исходником для правки. Скрипт сам проверяет
# каждый файл сторонним разбором и удаляет битый: неверный .mo не просто
# не переводит, с ним Kenshi вообще не запускается.
python "$ROOT\po2mo.py"

Step "Сборка"
$F = "$ROOT\forks"
BuildPlugin "$F\Au2942-plugins\SquadAutonomy"     "SquadAutonomy"     "SquadAutonomy"
BuildPlugin "$F\Au2942-plugins\MoreImmersiveBars" "MoreImmersiveBars" "MoreImmersiveBars"
BuildPlugin "$F\CharacterInspector-src\CharacterInspector\CharacterInspector\src" `
            "CharacterInspector" "Character Inspector"
BuildPlugin "$F\BetterLooting-src\Source"         "BetterLooting"     "BetterLooting"
# Свой плагин, не форк: живёт в ours\, а не в forks\.
BuildPlugin "$ROOT\ours\StatColours"               "StatColours"       "StatColours"
# У StatColours есть свои файлы рядом с DLL: настройки цветов и описание.
# Эталон лежит в ours\, в моде - копия, иначе она потеряется при пересоздании папки.
# Уже правленный игроком ini не трогаем.
$scMod = "$KENSHI\mods\StatColours"
if (Test-Path $scMod) {
    Copy-Item "$ROOT\ours\StatColours\README.txt" $scMod -Force
    if (-not (Test-Path "$scMod\StatColours.ini")) {
        Copy-Item "$ROOT\ours\StatColours\StatColours.ini" $scMod
        Write-Host "  положен StatColours.ini" -ForegroundColor Green
    }
}
BuildPlugin "$ROOT\ours\BuildModeGizmo"            "BuildModeGizmo"    "BuildModeGizmo"
# Настройки и описание - как у StatColours: эталон в ours\, в моде копия,
# уже правленный игроком ini не трогаем.
$bgMod = "$KENSHI\mods\BuildModeGizmo"
if (Test-Path $bgMod) {
    Copy-Item "$ROOT\ours\BuildModeGizmo\README.txt" $bgMod -Force
    if (-not (Test-Path "$bgMod\BuildModeGizmo.ini")) {
        Copy-Item "$ROOT\ours\BuildModeGizmo\BuildModeGizmo.ini" $bgMod
        Write-Host "  положен BuildModeGizmo.ini" -ForegroundColor Green
    }
}
BuildPlugin "$F\KillCounter-src"                  "KillCounter"       "KillCounter"
# Свой плагин: полосы опыта тех навыков, которые качаются прямо сейчас.
# Мод XpBars (DougTownsend) им заменён и отключён в mods.cfg.
BuildPlugin "$ROOT\ours\LiveXpBars"               "LiveXpBars"        "LiveXpBars"
# Доработки интерфейса - отдельный мод: от разметки не зависят и
# работают с любым интерфейсом. Правки разметки Dark UI (fix_darkui_*)
# остаются в самом моде интерфейса.
BuildPlugin "$ROOT\ours\DarkUiTweaks"             "DarkUiTweaks"      "DarkUiTweaks"
$duMod = "$KENSHI\mods\DarkUiTweaks"
if (Test-Path $duMod) {
    Copy-Item "$ROOT\ours\DarkUiTweaks\README.txt" $duMod -Force
    if (-not (Test-Path "$duMod\DarkUiTweaks.ini")) {
        Copy-Item "$ROOT\ours\DarkUiTweaks\DarkUiTweaks.ini" $duMod
        Write-Host "  положен DarkUiTweaks.ini" -ForegroundColor Green
    }
}
# Настройки и описание - как у остальных наших: эталон в ours\, в моде
# копия, уже правленный игроком ini не трогаем.
$lxMod = "$KENSHI\mods\LiveXpBars"
if (Test-Path $lxMod) {
    Copy-Item "$ROOT\ours\LiveXpBars\README.txt" $lxMod -Force
    if (-not (Test-Path "$lxMod\LiveXpBars.ini")) {
        Copy-Item "$ROOT\ours\LiveXpBars\LiveXpBars.ini" $lxMod
        Write-Host "  положен LiveXpBars.ini" -ForegroundColor Green
    }
}
# Плагин Асура: навыки растут до 100. Собирается у нас, потому что его
# готовая DLL требует KenshiLib с экспортом CharStats::xp* (см.
# feat_xpo_slots.py). Настроек-файла нет: всё в GLOBAL CONSTANTS.
BuildPlugin "$F\XP_Overhaul-src\XP_Overhaul"      "XP_Overhaul"       "XP_Overhaul"
if (Test-Path "$KENSHI\mods\XP_Overhaul") {
    Copy-Item "$F\XP_Overhaul-src\README.txt" "$KENSHI\mods\XP_Overhaul" -Force
    # Страница MCM: подписи en/ru/zh (названия навыков - из main.po игры)
    Copy-Item "$F\XP_Overhaul-src\XP_Overhaul\locale" "$KENSHI\mods\XP_Overhaul" -Recurse -Force
}
# Свой плагин: сравнение вещи под курсором с надетой. Перевод - .po прямо
# рядом с DLL (Localization.h читает .po сам); эталон в ours\.
BuildPlugin "$ROOT\ours\GearCompare"              "GearCompare"       "GearCompare"
$gcMod = "$KENSHI\mods\GearCompare"
if (Test-Path $gcMod) {
    Copy-Item "$ROOT\ours\GearCompare\README.txt" $gcMod -Force
    Copy-Item "$ROOT\ours\GearCompare\locale" $gcMod -Recurse -Force
    if (-not (Test-Path "$gcMod\GearCompare.ini")) {
        Copy-Item "$ROOT\ours\GearCompare\GearCompare.ini" $gcMod
        Write-Host "  положен GearCompare.ini" -ForegroundColor Green
    }
}
# Свой плагин: пациент лечит себя сам, пока его лечат. Настройки и
# описание - как у остальных наших.
# Свой плагин: кто из отряда закреплён за выделенной постройкой -
# строки в панели постройки, по зажатому ALT.
BuildPlugin "$ROOT\ours\AssignedWorkers"          "AssignedWorkers"   "AssignedWorkers"
$awMod = "$KENSHI\mods\AssignedWorkers"
if (Test-Path $awMod) {
    Copy-Item "$ROOT\ours\AssignedWorkers\README.txt" $awMod -Force
    Copy-Item "$ROOT\ours\AssignedWorkers\locale" $awMod -Recurse -Force
    if (-not (Test-Path "$awMod\AssignedWorkers.ini")) {
        Copy-Item "$ROOT\ours\AssignedWorkers\AssignedWorkers.ini" $awMod
        Write-Host "  положен AssignedWorkers.ini" -ForegroundColor Green
    }
}
# Опыт стрелку за нанесённый урон: ловит раны от снарядов (addWound).
BuildPlugin "$ROOT\ours\RangedDamageXp"           "RangedDamageXp"    "RangedDamageXp"
$rdMod = "$KENSHI\mods\RangedDamageXp"
if (Test-Path $rdMod) {
    Copy-Item "$ROOT\ours\RangedDamageXp\README.txt" $rdMod -Force
    if (-not (Test-Path "$rdMod\RangedDamageXp.ini")) {
        Copy-Item "$ROOT\ours\RangedDamageXp\RangedDamageXp.ini" $rdMod
        Write-Host "  положен RangedDamageXp.ini" -ForegroundColor Green
    }
}
# ModConfigMenu: вкладка «Моды» в настройках игры (shared\ModConfigMenu.h).
BuildPlugin "$ROOT\ours\ModConfigMenu"            "ModConfigMenu"     "ModConfigMenu"
$mcmMod = "$KENSHI\mods\ModConfigMenu"
if (Test-Path $mcmMod) {
    Copy-Item "$ROOT\ours\ModConfigMenu\locale" $mcmMod -Recurse -Force
    # sdk: заголовок API (источник - shared) и руководство для авторов плагинов
    Copy-Item "$ROOT\shared\ModConfigMenu.h" "$ROOT\ours\ModConfigMenu\sdk\" -Force
    Copy-Item "$ROOT\shared\McmModHubBridge.h" "$ROOT\ours\ModConfigMenu\sdk\" -Force
    Copy-Item "$ROOT\ours\ModConfigMenu\sdk" $mcmMod -Recurse -Force
    Copy-Item "$ROOT\ours\ModConfigMenu\README.txt" $mcmMod -Force
    # Подложка заголовка окна (картинка пользователя). Только из gui\gfx мода
    # игра кладёт её в группу ресурсов GUI, где её ищет MyGUI.
    New-Item -ItemType Directory -Force "$mcmMod\gui\gfx" | Out-Null
    Copy-Item "$ROOT\ours\ModConfigMenu\gui\gfx\MCM_Caption.png" "$mcmMod\gui\gfx" -Force
    if (-not (Test-Path "$mcmMod\ModConfigMenu.ini")) {
        Copy-Item "$ROOT\ours\ModConfigMenu\ModConfigMenu.ini" $mcmMod
    }
}

# Форки модов Emkej (forks\emkej, ветка leopard): список файлов - из vcxproj.
foreach ($e in @("Container-Highlight", "Vital-Sense", "Vital-Read", "Job-B-Gone", "Organize-the-Trader",
                  "Organize-the-Inventory", "Organize-the-Crafting-Stations", "Auto-Pause-on-Load",
                  "Loot-Scoot-Execute", "Wall-B-Gone", "Hidden-Faction-Relations", "Map-markers")) {
    BuildPlugin "$ROOT\forks\emkej\$e" $e $e -FromVcxproj
    # Переводы страниц MCM (en/ru/zh) - в папке пакета форка
    $loc = "$ROOT\forks\emkej\$e\$e\locale"
    $eDir = ModDir $e
    if (-not $NoDeploy -and (Test-Path $loc) -and $eDir) {
        Copy-Item $loc $eDir -Recurse -Force
    }
    # Значения по умолчанию для «Сбросить» в MCM (авторский mod-config.json)
    $defaults = "$ROOT\forks\emkej\$e\$e\mod-config.defaults.json"
    if (-not $NoDeploy -and (Test-Path $defaults) -and $eDir) {
        Copy-Item $defaults $eDir -Force
    }
}
# Моды Emkej - под GPLv3: к изменённой DLL - её исходники архивом в корне
# папки мода (<Mod>-source.zip; только включённые в mods.cfg).
if (-not $NoDeploy) {
    & python -X utf8 "$ROOT\tools\pack_emkej_sources.py" | Select-String -Pattern "КБ"
}

# Метки разыскиваемых на большой карте. Цели листовок плагин подбирает
# сам по данным игры; ручные пары - tools\wanted_overrides.txt
BuildPlugin "$ROOT\ours\WantedMap"                "WantedMap"         "WantedMap"
$wmMod = "$KENSHI\mods\WantedMap"
if (Test-Path $wmMod) {
    Copy-Item "$ROOT\ours\WantedMap\README.txt" $wmMod -Force
    Copy-Item "$ROOT\tools\wanted_overrides.txt" "$wmMod\WantedMap_overrides.txt" -Force
    Remove-Item "$wmMod\WantedMap_targets.txt" -ErrorAction SilentlyContinue
    Copy-Item "$ROOT\ours\WantedMap\locale" $wmMod -Recurse -Force
    if (-not (Test-Path "$wmMod\WantedMap.ini")) {
        Copy-Item "$ROOT\ours\WantedMap\WantedMap.ini" $wmMod
        Write-Host "  положен WantedMap.ini" -ForegroundColor Green
    }
}
BuildPlugin "$ROOT\ours\PatientSelfAid"           "PatientSelfAid"    "PatientSelfAid"
$psMod = "$KENSHI\mods\PatientSelfAid"
if (Test-Path $psMod) {
    Copy-Item "$ROOT\ours\PatientSelfAid\README.txt" $psMod -Force
    if (-not (Test-Path "$psMod\PatientSelfAid.ini")) {
        Copy-Item "$ROOT\ours\PatientSelfAid\PatientSelfAid.ini" $psMod
        Write-Host "  положен PatientSelfAid.ini" -ForegroundColor Green
    }
}

BuildPlugin "$ROOT\ours\QuickStartSelect"         "QuickStartSelect"  "QuickStartSelect"
$qsMod = "$KENSHI\mods\QuickStartSelect"
if (Test-Path $qsMod) {
    Copy-Item "$ROOT\ours\QuickStartSelect\README.txt" $qsMod -Force
    Copy-Item "$ROOT\ours\QuickStartSelect\locale" $qsMod -Recurse -Force
    if (-not (Test-Path "$qsMod\QuickStartSelect.ini")) {
        Copy-Item "$ROOT\ours\QuickStartSelect\QuickStartSelect.ini" $qsMod
        Write-Host "  положен QuickStartSelect.ini" -ForegroundColor Green
    }
}

# Formations собирается MSVC 2022, а не VC10: автор не линкует KenshiLib
# и достаёт MyGUI через GetProcAddress, поэтому старый тулсет ему не нужен.
& "$ROOT\build_formations.ps1" | Select-String -Pattern "OK:|НЕ УДАЛАСЬ|установлен"
if (-not $NoDeploy -and (Test-Path "$KENSHI\mods\Formations")) {
    Copy-Item "$F\Formations-src\README.txt" "$KENSHI\mods\Formations" -Force
}

Step "Переводы наших плагинов (ours\*\locale -> мод)"
# Подписи страниц MCM и прочий текст: en_GB / ru_RU / zh_CN. У StatColours и
# LiveXpBars эталон каталогов лежит в самом моде - их папок в ours нет.
if (-not $NoDeploy) {
    foreach ($dir in Get-ChildItem "$ROOT\ours" -Directory) {
        $loc = Join-Path $dir.FullName "locale"
        $mod = Join-Path "$KENSHI\mods" $dir.Name
        if ((Test-Path $loc) -and (Test-Path $mod)) {
            Copy-Item $loc $mod -Recurse -Force
        }
    }
}

Step "Контроль: в собранных DLL не должно быть кириллицы"
# Признак того, что перевод случайно попал в бинарник вместо .po.
# С переездом окна BetterLooting на интерфейс игры (feat_bl_mygui.py) под
# это правило попали все наши плагины: русского в бинарниках нет ни в
# каком виде, весь перевод живёт в .po.
# Formations в списке нет намеренно: он не линкует KenshiLib, а значит не
# может спросить у игры язык, и его русский лежит прямо в бинарнике.
foreach ($m in @("SquadAutonomy", "MoreImmersiveBars", "Character Inspector", "BetterLooting", "StatColours", "BuildModeGizmo", "KillCounter", "LiveXpBars", "PatientSelfAid", "GearCompare", "XP_Overhaul", "DarkUiTweaks", "AssignedWorkers", "RangedDamageXp", "WantedMap", "ModConfigMenu", "Container-Highlight", "Vital-Sense", "Vital-Read", "Job-B-Gone", "Organize-the-Trader", "Organize-the-Inventory", "Organize-the-Crafting-Stations", "Auto-Pause-on-Load", "Loot-Scoot-Execute", "Wall-B-Gone", "Hidden-Faction-Relations", "Map-markers")) {
    $dll = Get-ChildItem "$KENSHI\mods\$m" -Filter *.dll -ErrorAction SilentlyContinue |
           Where-Object { $_.Name -notlike "*.orig" } | Select-Object -First 1
    if (-not $dll) { continue }
    # Ищем цепочку от четырёх кириллических букв подряд: одиночные байты
    # D0/D1 сплошь и рядом встречаются в машинном коде, и проверка
    # «хоть один байт» давала ложную тревогу на всех DLL.
    $bytes = [System.IO.File]::ReadAllBytes($dll.FullName)
    $cyr = 0
    $run = 0
    for ($i = 0; $i -lt $bytes.Length - 1; $i++) {
        if (($bytes[$i] -eq 0xD0 -or $bytes[$i] -eq 0xD1) -and
            $bytes[$i+1] -ge 0x80 -and $bytes[$i+1] -le 0xBF) {
            $run++
            $i++
            if ($run -eq 4) { $cyr++ }
        } else {
            $run = 0
        }
    }
    if ($cyr -eq 0) {
        Write-Host ("  {0,-22} чисто" -f $m) -ForegroundColor Green
    } else {
        Write-Host ("  {0,-22} НАЙДЕНА КИРИЛЛИЦА: {1}" -f $m, $cyr) -ForegroundColor Red
    }
}

Step "Проверка хеш-таблиц в .mo"
# Отдельно от проверки внутри po2mo: разбор Python хеш-таблицу игнорирует,
# а реализация gettext в игре вполне может искать именно по ней.
python "$ROOT\check_mo_hash.py"

Step "Проверка каталогов перевода"
python "$ROOT\check_locale.py"
if ($LASTEXITCODE -ne 0) {
    Write-Host "  в каталогах есть проблемы, смотри выше" -ForegroundColor Red
} else {
    Write-Host "  каталоги в порядке" -ForegroundColor Green
}

Step "Пакет исходников для сообщества"
# Собирается из форка каждый раз. Руками его делать нельзя — один раз уже
# разошёлся с форком на две правки, а по датам файлов выглядел новее.
python "$ROOT\make_export.py"

if ($script:BuildFailed -gt 0) {
    Write-Host ("`nНЕ СОБРАЛОСЬ ПЛАГИНОВ: {0}" -f $script:BuildFailed) -ForegroundColor Red
}
Step "Готово"
