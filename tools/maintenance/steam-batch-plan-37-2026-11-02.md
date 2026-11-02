# Steam 37-item publication plan, 2026-11-02

LOCAL PLAN ONLY. Do not run the publisher until the user confirms the Nexus uploads are complete and the corresponding links are available. No Steam API/client call, write, upload, new item, restart/UI action or ledger mutation was performed.

The 37 existing Workshop IDs, versions, staged content trees, primary previews, both descriptions and one bilingual note per item are pinned in the companion JSON. All item IDs are nonzero, unique and match the previously verified official item IDs. AI Fixed stays at2 and its latest ZIP is `d1fb63b1106e4695b6d33ea41af182ec2f8a2985dc5cdbd29955033ee965e28b`. The other36 candidate hashes/versions remain unchanged.

Reviewed publisher: `D:/DEV/Kenshi/workshop/steam_upload.py`, SHA256 `9e21fd7b3b81ab694b32e62ca4e5cd6e7b05de2555a9b5c37c8a039ece0be3b8`, identical to its canonical source. `upload` makes the English content/preview/metadata submission with the full bilingual changelog, then the Russian title/description-only submission with no second note. English is the API default; `russian` is set on the second handle. [Steamworks language contract](https://partner.steamgames.com/doc/api/ISteamUGC#SetItemUpdateLanguage), [NULL change-note contract](https://partner.steamgames.com/doc/api/ISteamUGC#SubmitItemUpdate).

Exact future command (review only; not executed):

```powershell
$env:PYTHONIOENCODING = 'utf-8'
Remove-Item Env:KENSHI_RELEASE_INPUT_ROOT -ErrorAction SilentlyContinue
$python = 'C:\Users\User\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$publisher = 'D:\DEV\Kenshi\workshop\steam_upload.py'
$mods = @(
    'AssignedWorkers',
    'BestMinimap',
    'BuildModeGizmo',
    'GatePushGuard',
    'GearCompare',
    'ItemMarkers',
    'LiveXpBars',
    'ModConfigMenu',
    'MultiXPMining',
    'PatientSelfAid',
    'QuickStartSelect',
    'RangedDamageXp',
    'RecruitMarkers',
    'SkillLevelUpInfo',
    'StatColours',
    'UITweaks',
    'WantedMap',
    'Container-Highlight',
    'Vital-Sense',
    'Vital-Read',
    'Job-B-Gone',
    'Organize-the-Trader',
    'Organize-the-Crafting-Stations',
    'Auto-Pause-on-Load',
    'Loot-Scoot-Execute',
    'Wall-B-Gone',
    'Hidden-Faction-Relations',
    'Map-markers',
    'KenshiJobManagement',
    'Character-Highlight',
    'Character Inspector',
    'BetterLooting',
    'KillCounter',
    'KenshiRotate',
    'TauntExpansion',
    'StackSort',
    'AIFixed'
)
& $python $publisher upload @mods
```

One process uses the already running/logged-in Steam client. No Steam restart or window interaction is part of this plan. Clear the preparation-only input override before publication so the publisher’s final FCS metadata/preview copies target the installed/disabled-stage mod folders, not clean preparation inputs.

Partial-failure recovery: after checking which EN submissions succeeded, use the existing `desc` mode for an item whose RU text failed; it updates EN/RU title/description with no change note or content upload. Do not blindly repeat `upload` for an already submitted EN release note. Example, review only:

```powershell
& $python $publisher desc 'AIFixed'
```

The publisher writes local `_Mod.info`/`_Mod.img` only after both submissions succeed, adds gallery screenshots only for new items, and does not update the published ledger. Since all37 IDs already exist, no new item/gallery-upload path is intended. Verify successful callback results/public output before recording publication. The current ledger hash stays `5134f95b79d8907ef7593b8a57b6f434fa690715c70040c15d909e1424738536`. Its wait loop has no timeout/resume journal; retain progress and stop on a reported error.

AI Fixed public compatibility and the one EN/RU v2 note explicitly say original SleepFix/GateFix are not required. Author thanks, original Steam/GitHub links and GPL provenance remain. Our repository links are confined to the MCM description.

| Package | Version | Existing Workshop ID | Content files | Selected preview |
| --- | --- | --- | --- | --- |
| AssignedWorkers | 8 | 3812150807 | 10 | `D:\DEV\Kenshi\workshop\AssignedWorkers\preview.jpg` (1254×1254) |
| BestMinimap | 3 | 3824087152 | 8 | `D:\DEV\Kenshi\workshop\BestMinimap\preview.jpg` (1254×1254) |
| BuildModeGizmo | 5 | 3813761881 | 8 | `D:\DEV\Kenshi\workshop\BuildModeGizmo\preview.jpg` (1254×1254) |
| GatePushGuard | 2 | 3831174166 | 10 | `D:\DEV\Kenshi\workshop\GatePushGuard\preview.jpg` (1254×1254) |
| GearCompare | 8 | 3812150687 | 9 | `D:\DEV\Kenshi\workshop\GearCompare\preview.jpg` (1254×1254) |
| ItemMarkers | 6 | 3816608451 | 17 | `D:\DEV\Kenshi\workshop\ItemMarkers\preview.jpg` (1254×1254) |
| LiveXpBars | 12 | 3812150449 | 8 | `D:\DEV\Kenshi\workshop\LiveXpBars\preview.jpg` (1254×1254) |
| ModConfigMenu | 11 | 3813761584 | 15 | `D:\DEV\Kenshi\workshop\ModConfigMenu\preview.jpg` (1254×1254) |
| MultiXPMining | 2 | 3829848414 | 11 | `D:\DEV\Kenshi\workshop\MultiXPMining\preview.jpg` (1254×1254) |
| PatientSelfAid | 5 | 3812149895 | 8 | `D:\DEV\Kenshi\workshop\PatientSelfAid\preview.jpg` (1254×1254) |
| QuickStartSelect | 7 | 3813761725 | 8 | `D:\DEV\Kenshi\workshop\QuickStartSelect\preview.jpg` (1254×1254) |
| RangedDamageXp | 3 | 3813761935 | 8 | `D:\DEV\Kenshi\workshop\RangedDamageXp\preview.jpg` (1254×1254) |
| RecruitMarkers | 5 | 3820514162 | 11 | `D:\DEV\Kenshi\workshop\RecruitMarkers\preview.jpg` (1254×1254) |
| SkillLevelUpInfo | 3 | 3829848328 | 9 | `D:\DEV\Kenshi\workshop\SkillLevelUpInfo\preview.jpg` (1254×1254) |
| StatColours | 13 | 3812150560 | 8 | `D:\DEV\Kenshi\workshop\StatColours\preview.jpg` (1254×1254) |
| UITweaks | 13 | 3812150876 | 8 | `D:\DEV\Kenshi\workshop\UITweaks\preview.jpg` (1254×1254) |
| WantedMap | 7 | 3813761812 | 11 | `D:\DEV\Kenshi\workshop\WantedMap\preview.jpg` (1254×1254) |
| Container-Highlight | 3 | 3813852014 | 12 | `D:\DEV\Kenshi\workshop\Container-Highlight\preview.jpg` (1254×1254) |
| Vital-Sense | 7 | 3813852288 | 19 | `D:\DEV\Kenshi\workshop\Vital-Sense\preview.jpg` (1254×1254) |
| Vital-Read | 6 | 3813852230 | 16 | `D:\DEV\Kenshi\workshop\Vital-Read\preview.jpg` (1254×1254) |
| Job-B-Gone | 6 | 3813852087 | 9 | `D:\DEV\Kenshi\workshop\Job-B-Gone\preview.jpg` (1254×1254) |
| Organize-the-Trader | 8 | 3813852189 | 9 | `D:\DEV\Kenshi\workshop\Organize-the-Trader\preview.jpg` (1254×1254) |
| Organize-the-Crafting-Stations | 2 | 3816121254 | 9 | `D:\DEV\Kenshi\workshop\Organize-the-Crafting-Stations\preview.jpg` (1254×1254) |
| Auto-Pause-on-Load | 6 | 3813851956 | 9 | `D:\DEV\Kenshi\workshop\Auto-Pause-on-Load\preview.jpg` (1254×1254) |
| Loot-Scoot-Execute | 6 | 3813852124 | 14 | `D:\DEV\Kenshi\workshop\Loot-Scoot-Execute\preview.jpg` (1254×1254) |
| Wall-B-Gone | 5 | 3813852362 | 9 | `D:\DEV\Kenshi\workshop\Wall-B-Gone\preview.jpg` (1254×1254) |
| Hidden-Faction-Relations | 3 | 3813852055 | 9 | `D:\DEV\Kenshi\workshop\Hidden-Faction-Relations\preview.jpg` (1254×1254) |
| Map-markers | 4 | 3813852161 | 9 | `D:\DEV\Kenshi\workshop\Map-markers\preview.jpg` (1254×1254) |
| KenshiJobManagement | 5 | 3818460323 | 33 | `D:\DEV\Kenshi\workshop\KenshiJobManagement\preview.jpg` (1254×1254) |
| Character-Highlight | 3 | 3820546396 | 9 | `D:\DEV\Kenshi\workshop\Character-Highlight\preview.jpg` (1254×1254) |
| Character Inspector | 3 | 3829848451 | 48 | `D:\DEV\Kenshi\workshop\Character Inspector\preview.jpg` (1254×1254) |
| BetterLooting | 5 | 3816121198 | 8 | `D:\DEV\Kenshi\workshop\BetterLooting\preview.jpg` (1254×1254) |
| KillCounter | 2 | 3829848599 | 9 | `D:\DEV\Kenshi\workshop\KillCounter\preview.jpg` (1254×1254) |
| KenshiRotate | 2 | 3829848534 | 10 | `D:\DEV\Kenshi\workshop\KenshiRotate\preview.jpg` (1254×1254) |
| TauntExpansion | 2 | 3831174059 | 11 | `D:\DEV\Kenshi\workshop\TauntExpansion\preview.jpg` (1254×1254) |
| StackSort | 3 | 3830285370 | 11 | `D:\DEV\Kenshi\workshop\StackSort\preview.jpg` (1254×1254) |
| AIFixed | 2 | 3829845490 | 10 | `D:\DEV\Kenshi\workshop\AIFixed\preview.jpg` (1254×1254) |

The publisher selects `.png` before `.jpg` when both are present; alternate preview files exist for 0 items and are listed in JSON. Nothing was deleted. Dark UI Advanced2/Formations2 and private/local prototypes are not included in this37-item upload command.

All 37 staged Steam content trees were cross-checked file-for-file against the frozen Nexus ZIP payloads, including CRC and SHA256. The Steam session itself was not checked or initialized. Local Dark UI Advanced2/Formations2 description corrections are outside this exact37-item content command; a separately scoped `desc`-only update can apply those texts without publishing another version or note. The `desc` recovery path also does not write local FCS auxiliary files automatically.
