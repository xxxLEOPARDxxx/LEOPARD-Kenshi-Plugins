# Local plugin release inventory, 2026-11-01

Read-only audit of the 37-package localization-log queue. Publication versions below come from the local ledger dated 2026-11-01; no external publication state was checked. Verify actual versions before preparing new releases. No packages or metadata were changed.

## Counts

- packages: 37
- packages_with_publication_ledger_entry: 29
- packages_missing_publication_ledger_entry: 8
- archives: 37
- archives_crc_ok: 37
- build_matches_active_local: 34
- build_differs_published_zip: 37
- published_zip_matches_workshop_content: 37
- archives_exact_three_mo_no_po: 17
- archives_po_only: 16
- archives_mixed_po_and_mo: 4
- archives_with_readme: 25
- archives_with_private_files: 0
- source_readme_present: 37
- source_readme_matches_content: 1
- covers_present: 37
- workshop_previews_present: 37
- active_local_directories_with_three_mo_languages: 23
- active_local_directories_with_three_languages_mo_or_po: 37
- archives_with_three_languages_mo_or_po: 37
- packages_with_explicit_bilingual_release_notes_entry: 17
- packages_with_existing_baseline_changelog: 37
- stale_release_note_published_markers: 10
- canonical_workshop_files_checked: 15
- canonical_workshop_files_identical: 15

All queued packages need a newly numbered release and fresh bilingual notes for the detailed-log change. Existing archives are the old release baseline, not the new maintenance update. The next numbers below are provisional until actual public versions are verified. Nexus is uploaded by the user first; Steam follows the supplied Nexus links.

## Additional changes and note problems

- **UITweaks:** Save and Load previews; one-time higher opening anchor; hide slots without images; filtered lossless PNG up to 1600x900; legacy JPEG compatibility; next-open dialog height 120% (100..160). Evidence: d9f6f9e, 805daf8, cc621bc, e8cf96e. README labels new preview/height work as version 12 although the local publication ledger already records 12. Prepare version 13 once after verifying the actual public version; update the full bilingual release note and player descriptions.
- **Container-Highlight:** Retire stale model tint bindings after collection/restoration failure; preserve replacement materials; avoid repeated restoration faults. Evidence: 9101ad8. Pending README text is present; canonical release metadata/changelog still describes recorded version 2. Next recorded-baseline release would be 3.
- **Vital-Read:** Preflight optional icon textures/aliases before native loading to avoid missing-texture diagnostic spam. Evidence: 594717a. Existing metadata must be updated for this scope before the next release.
- **Vital-Sense:** Preflight optional icon textures/aliases and explain standard-icon fallback in settings. Evidence: 594717a, 75535ed. Existing metadata must be updated for this scope before the next release.
- **StackSort:** Find optional Arrange buttons without noisy layout warnings, including equipment windows lacking the button. Evidence: bec8201. Existing metadata must be updated for this scope before the next release.
- **ItemMarkers:** Temporary stolen-badge suppression was reverted. Native stolen indication remains; rare ownership/stacking reports are unresolved and must not be described as fixed. Evidence: 6843b7a. Existing metadata must be updated for this scope before the next release.
- **Organize-the-Trader:** German/unsupported-language fallback was audited: English fallback works; no production localization fix or new language was added. Evidence: 173e4bd. Existing metadata must be updated for this scope before the next release.

## Per-package status

| Package | Recorded → next | Build vs active local | Build vs old ZIP | ZIP locale form | ZIP README |
| --- | --- | --- | --- | --- | --- |
| AIFixed | 1 → 2? | same | different | 3 MO | yes |
| AssignedWorkers | 7 → 8? | same | different | 3 MO | yes |
| BestMinimap | 2 → 3? | same | different | 3 MO | yes |
| BuildModeGizmo | unrecorded; catalog 4 → 5? | same | different | PO only | yes |
| GatePushGuard | 1 → 2? | same | different | 3 MO | yes |
| GearCompare | unrecorded; catalog 7 → 8? | same | different | PO only | yes |
| ItemMarkers | unrecorded; catalog 5 → 6? | same | different | PO + MO | yes |
| LiveXpBars | unrecorded; catalog 11 → 12? | same | different | PO only | yes |
| ModConfigMenu | 10 → 11? | same | different | 3 MO | yes |
| MultiXPMining | 1 → 2? | same | different | 3 MO | yes |
| PatientSelfAid | 4 → 5? | same | different | 3 MO | yes |
| QuickStartSelect | unrecorded; catalog 6 → 7? | same | different | PO + MO | yes |
| RangedDamageXp | unrecorded; catalog 2 → 3? | same | different | PO only | yes |
| RecruitMarkers | 4 → 5? | same | different | 3 MO | yes |
| SkillLevelUpInfo | 2 → 3? | same | different | 3 MO | yes |
| StatColours | unrecorded; catalog 12 → 13? | same | different | PO + MO | yes |
| UITweaks | 12 → 13? | same | different | 3 MO | yes |
| WantedMap | unrecorded; catalog 6 → 7? | same | different | PO only | yes |
| Container-Highlight | 2 → 3? | same | different | PO only | **missing** |
| Vital-Sense | 6 → 7? | same | different | PO only | **missing** |
| Vital-Read | 5 → 6? | same | different | PO only | **missing** |
| Job-B-Gone | 5 → 6? | same | different | PO only | **missing** |
| Organize-the-Trader | 7 → 8? | same | different | 3 MO | yes |
| Organize-the-Crafting-Stations | 1 → 2? | same | different | PO only | **missing** |
| Auto-Pause-on-Load | 5 → 6? | same | different | PO only | **missing** |
| Loot-Scoot-Execute | 5 → 6? | same | different | PO only | **missing** |
| Wall-B-Gone | 4 → 5? | same | different | PO only | **missing** |
| Hidden-Faction-Relations | 2 → 3? | **different/missing** | different | PO only | **missing** |
| Map-markers | 3 → 4? | **different/missing** | different | PO only | **missing** |
| KenshiJobManagement | 4 → 5? | same | different | PO only | **missing** |
| Character-Highlight | 2 → 3? | same | different | PO + MO | **missing** |
| Character Inspector | 2 → 3? | same | different | 3 MO | yes |
| BetterLooting | 4 → 5? | same | different | 3 MO | yes |
| KillCounter | 1 → 2? | same | different | 3 MO | yes |
| KenshiRotate | 1 → 2? | same | different | 3 MO | yes |
| TauntExpansion | 1 → 2? | same | different | 3 MO | yes |
| StackSort | 2 → 3? | **different/missing** | different | 3 MO | yes |

## Outliers and synchronization

- **BestMinimap:** release_notes published_version marker 1 is behind ledger 2; refresh it during the next release preparation.
- **BuildModeGizmo:** missing from the publication ledger. Catalog/archive version 4 is only a local fallback, not verified publication state.
- **GatePushGuard:** release_notes published_version marker 0 is behind ledger 1; refresh it during the next release preparation.
- **GearCompare:** missing from the publication ledger. Catalog/archive version 7 is only a local fallback, not verified publication state.
- **ItemMarkers:** missing from the publication ledger. Catalog/archive version 5 is only a local fallback, not verified publication state.
- **LiveXpBars:** missing from the publication ledger. Catalog/archive version 11 is only a local fallback, not verified publication state.
- **ModConfigMenu:** release_notes published_version marker 9 is behind ledger 10; refresh it during the next release preparation.
- **QuickStartSelect:** missing from the publication ledger. Catalog/archive version 6 is only a local fallback, not verified publication state.
- **RangedDamageXp:** missing from the publication ledger. Catalog/archive version 2 is only a local fallback, not verified publication state.
- **RecruitMarkers:** release_notes published_version marker 3 is behind ledger 4; refresh it during the next release preparation.
- **SkillLevelUpInfo:** release_notes published_version marker 1 is behind ledger 2; refresh it during the next release preparation.
- **StatColours:** missing from the publication ledger. Catalog/archive version 12 is only a local fallback, not verified publication state.
- **UITweaks:** release_notes published_version marker 11 is behind ledger 12; refresh it during the next release preparation.
- **WantedMap:** missing from the publication ledger. Catalog/archive version 6 is only a local fallback, not verified publication state.
- **Organize-the-Trader:** release_notes published_version marker 6 is behind ledger 7; refresh it during the next release preparation.
- **Hidden-Faction-Relations:** build differs from its currently resolved local DLL. See JSON hashes and paths; do not assume the new candidate is installed.
- **Map-markers:** build differs from its currently resolved local DLL. See JSON hashes and paths; do not assume the new candidate is installed.
- **Character Inspector:** release_notes published_version marker 1 is behind ledger 2; refresh it during the next release preparation.
- **TauntExpansion:** release_notes published_version marker 0 is behind ledger 1; refresh it during the next release preparation.
- **StackSort:** build differs from its currently resolved local DLL. See JSON hashes and paths; do not assume the new candidate is installed.
- **StackSort:** release_notes published_version marker 1 is behind ledger 2; refresh it during the next release preparation.

The JSON contains exact SHA256 identities, source/installed/stage paths, MO languages and sizes, archive contents, README and cover status for each package. A DLL hash difference establishes different files, not a semantic code comparison or public-site update. Private DynamicFactions, GlobalSatelliteMap, Organize-the-Inventory and the noncanonical XP_Overhaul build remain excluded.

All 37 current local directories and old archives contain all three languages through MO or PO; the issue is release format/refresh, not evidence that translations are missing. Default INI templates and the official MultiXPMining SourceData INIs are allowed data, not personal settings. The source release_notes file has explicit entries for only 17 queued packages; existing changelog history is checked separately and must be preserved. Formations and Dark UI Advanced are outside this 37-package logging queue.
