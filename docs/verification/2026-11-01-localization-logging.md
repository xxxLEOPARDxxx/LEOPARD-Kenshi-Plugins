# Per-plugin localization diagnostics, 2026-11-01

Localization success summaries now obey each module's detailed-log setting. The common loader retains a bounded 256-byte summary per DLL; enabling diagnostics later reports the existing catalogue once, without reloading dictionaries or invalidating translation pointers. No new work is added to Tr/TrFmt. Translation-file and format errors use unconditional ErrorLog.

All 40 canonical Localization.h users have a settings binding (including the private DF and global-map projects); Formations does not use the shared loader. The locally maintained XP_Overhaul integration is preserved in tools/maintenance/xp-overhaul-localization-log.patch. Upstream KEO has its own copied loader and is intentionally outside this maintenance; its separate MCM fork remains scheduled after the satellite map.

Verification:
- 11 VC10 production-helper checks, including multiple translation units, Debug off/on/off, repeated unchanged settings, bounded summary and failed-save rollback.
- Cross-repository source audit: 40 canonical users, unconditional translation errors.
- 41 fresh /GL + /LTCG x64 builds. All exact RE_Kenshi startup exports, MCM exports and imports against installed native DLLs verified.
- 39 existing installed DLLs replaced from verified builds, with old DLL backups. GlobalSatelliteMap remains uninstalled; Organize-the-Inventory is not installed and its verified build is retained.
- 69 settings/config files preserved at deployment, excluding the intentional new KJM factory-default file. First game startup added only the new factory-default KJM Debug=0 key to its existing INI; existing values retained.
- Live Russian MCM run: startup summaries only for already-enabled MCM, DF and AI Fixed; disabled SkillLevelUpInfo, Stack Sort and Best Minimap stayed quiet. Enabling KJM Detailed log after registration produced exactly one existing-catalogue summary (320 strings); disabling it added none. KJM Debug=0 restored. No save was loaded.
- Independent review corrected an early include in Better Looting and prevented summaries during failed setting-save rollback in the Organize/HFR forks. Removed locale logging from Taunt's AI finally block; UI/startup configuration owns it.

This is a bounded logging/lifecycle review and main-menu integration test, not a long-running FPS/memory benchmark. Native renderer satellite memory work is separate.

Evidence: D:/DEV/Kenshi/build/LocalizationMaintenance/{build-validation.json,deployment.json,runtime-validation.json,runtime-before-toggle.log,runtime-debug-enabled.log,runtime-after-disable.log}. Old binaries: D:\DEV\Kenshi\backup\localization-maintenance-20261101-114231.

No Nexus or Steam publication was performed, and no release version was incremented per edit. README maintenance notes have been updated; public release packaging will use the next actual release of each module, rather than overwriting published ZIPs under existing version numbers.
