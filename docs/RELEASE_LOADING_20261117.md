# Prepared loading/UI update - 17 November2026

Status: Nexus publication confirmed by the owner; all three Steam updates
were published and verified on17 November2026. Receipt:
`docs/releases/2026-11-17-loading-publication.json`. User confirmed faster save loads and
warmed area transitions with the already installed KEO build. This is feedback,
not an overall timing multiplier from a native microbenchmark. No new game run,
capture/profile, live gameplay log or process-memory collection was performed.
Preparation adds no new gameplay code; it packages earlier installed changes.

Published registry records KEO2, SkillLevelUpInfo4 and BestMinimap4 on13 November.
Prepared versions are3/5/5. Prior working2/4 labels were stale. The registry now records3/5/5; ALL_CHANGELOG_MODS.txt contains exactly one
confirmed Russian entry per release.

## KEO3

Optional startup/nearby RAM preparation and bounded model/texture/terrain video
memory retention; faster main-menu startup and CPU work in save parsing, NPC
models, skeletons, body shapes, equipment and bone data. Progress text hides
when idle. New memory options defaultOFF, all are live, with supported-native
fallback. Exact native work and errors remain owned by the engine. Resource
and NPC/save reviewers found no P1/P2. Seven prior job-compatibility edits were
independently reviewed/tested and committed, retaining the installed behavior.
See KEO/leopard/release-review-20261117.md in the fork source for test details.

## SkillLevelUpInfo5

Unused MassCombat excluded from notifications/recent bars before registration;
native XP and all other skills unchanged.37-slot diagnostic layout retained,
33 distinct supported fields.197 module assertions,3MO catalogs,38 imports and
independent review pass. DLL33ad71b5de7c534a9dd8d493ded562c1e3650853bb59f547e514bdd537fc7a33.

## BestMinimap5

Shared rectangular map/info frame, visible glyph/shadow centering with descender
clearance and skin margins, and tracked town widget identity instead of stale
native marker records. Rendering worker remains. Independent isolated runner:
8581 numbered assertions plus town lifecycle fixture, two semantic negative
controls correctly fail. Current vanilla/Dark UI/MyGUI contracts match retained
evidence. No P1/P2 found. Final in-game glyph placement and long-session FPS/
memory were not retested. DLL28cf78e5edbbba23fc8fe69fd59265c22b6b55d35761188cebb2422214a91e9a.

## Package gate

Canonical prepare_release.py with clean candidate inputs produced9/9/8 members
respectively. ZIP CRC and all hashes equal Steam content and installed DLLs;
exactly3 compiled catalogs, one README, canonical default INI with DebugOFF,
no personal INI/log/PO/object files. x64/startPlugin/MCM exports and installed
KenshiLib imports verified. EN/RU guides, complete separate notes, fork credits,
privacy and Steam limit pass. KEO descriptions7497RU/4511EN bytes. Six personal
files/modlist hashes and37 unrelated/source archive hashes unchanged.

First description preflight rejected oversized KEO Russian guide before ZIP
replacement. Repeated version updater briefly crossed into an unrelated record;
replaced it with exact AST keyword boundaries, restored original metadata, then
verified repeat idempotence and that only the selected3 versions differ from
HEAD. Final normal preparation and real pack/input-boundary tests pass.

ZIPs and bilingual Nexus descriptions are under D:/DEV/Kenshi/workshop/nexus.
The owner confirmed the Nexus updates, then the existing Steam IPC uploader
published each existing item with one bilingual change note and a separate
Russian translation submission without another change note. Both descriptions
were queried back through Steam and matched the prepared text.
