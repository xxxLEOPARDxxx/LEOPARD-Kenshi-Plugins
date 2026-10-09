# LEOPARD Kenshi Plugins

Наши плагины для Kenshi (RE_Kenshi / KenshiLib). Настройки - во вкладке MCM окна
«Настройки» игры (Mod Config Menu).

| Плагин | Что делает |
|---|---|
| AssignedWorkers | кто из отряда закреплён за постройкой - панель у её окна |
| BuildModeGizmo | стрелочки для перемещения своей недостроенной постройки |
| DarkUiTweaks | доработки частей интерфейса, которые игра строит в коде |
| GearCompare | панель «что надето» рядом с описанием вещи |
| ItemMarkers | цвет сорта, краденое, форма фракции, изученные чертежи на иконках вещей |
| LiveXpBars | полосы опыта навыков, которые качаются прямо сейчас |
| ModConfigMenu | вкладка MCM: настройки всех модов в игре |
| PatientSelfAid | пациент лечит себя, пока его лечат |
| QuickStartSelect | список предысторий с поиском и сортировкой |
| RangedDamageXp | опыт стрелку за нанесённый урон |
| StatColours | цвета навыков и поправок снаряжения |
| WantedMap | метки разыскиваемых на большой карте |

Форки модов других авторов (Emkej, BetterLooting) - в репозитории
[LEOPARD-Kenshi-Forks](https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks).

## Сборка

Visual C++ 2010 (VC10) + Windows SDK 7.1, KenshiLib 0.2.1, boost 1.60.

```powershell
.\build.ps1 -SrcDir ours\ItemMarkers -Name ItemMarkers
.\rebuild_all.ps1            # всё сразу: сборка и установка в игру
```

`shared/` - общие заголовки: Localization.h (перевод через .po), WidgetRef.h,
HoldKey.h, GameTheme.h, ModConfigMenu.h (API вкладки MCM).

Перевод - `ours/<Плагин>/locale/<язык>/LC_MESSAGES/*.po`; в DLL только английский.
