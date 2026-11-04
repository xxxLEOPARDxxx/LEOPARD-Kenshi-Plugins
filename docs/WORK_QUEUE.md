# Active work and next requested tasks

Updated 2026-11-05. This is a private working queue, not a release announcement.
The user is at the PC. Ask for scene preparation; use background telemetry and
file requests. Do not take desktop focus or collide with Minecraft autotests.
Always check the actual process immediately before replacing a loaded DLL.

1. **Global satellite map — PAUSED BY USER, 2026-11-04.**
   Final private installed build bdb15ea; preserve atlas/settings/source. Final
   actual log: FinePresent about0.02ms percall, LoadedCapture about15ms each1s.
   User still confirms FPS drops and map-icon flicker. No further map changes,
   rendering, restart requests or speculative optimization until user resumes.
   Evidence: game/_codex_work/20261104-satellite-pause/final_measurement.json and
   PAUSED.md. Real1024px parent rendering remains unimplemented; AcceptedCrop
   is only a tested prerequisite. No public release/personal changelog entry.

2. **KEO — ACTIVE PRIMARY TASK, refined by user2026-11-05.**
   Gameplay priorities: (1) pathfinding: short-route detours, circling/stuck
   movement, squad routing and avoiding water; (2) excess squads: distinguish
   genuinely created platoons from existing platoons loaded earlier; (3) mouse
   input lag is LOW PRIORITY, a single-player unconfirmed report, absent locally.
   Investigate input only with a concrete lead or reproducible evidence.

   The outside-Squin crowd also reproduced with ZERO KEO/KEOCompat modules in
   actual PID30780. This excludes running KEO as necessary for that observation,
   but not already displaced saved positions. By explicit user request both
   spawn/readiness experiments5772425 andfb8c46a were rolled back. Current source
   022bb70 has native src exactly5f68039, retaining MCM, compactRU/EN/ZH labels,
   navmesh/priority/pause/TLS/jobcancel fixes. Installed DLL32c40117...653423.
   Original personal INI and mod order were restored; no temporary test override
   remains. Do not reintroduce these spawn experiments as a proven fix.

   Personal keoRouting=false/plannerMode=on remains the user's choice. Recent
   travel tests did not exercise the KEO routing master ON. Arrange an explicit
   controlled ON test before interpreting route behavior or changing preferences.

   GOG PORT IS NOW ACTIVE IN PARALLEL: user supplied game/DropMeFiles_VfFSw.zip
   containing two EXEs. The smaller RE image36708352B matches GOG1.0.65 MD5
   213349bf76a0a758067f9ed0aef2ab01. Sources were not executed. Evidence at
   game/_codex_work/20261105-keo-gog-native. Build a verified native profile,
   including raw-base accesses, inline patches and anonymous return/byte tables;
   mapped exported entries alone are insufficient. Do not offer startup-only
   stubs as a working GOG port. Keep other-module Ogre/PhysX checks separate.

   Preserve native multithreading/player intent. MCM/RU/EN/ZH and detailed-log-last
   rules apply. Temporary maintenance fork: credit the original author and
   withdraw our version if original development resumes. Never push to the
   author's origin. Back up local KEO commits as patches in our own repository.

3. **Dynamic Factions — PAUSED BY USER.** Preserve all work and plans. No
   new implementation, game testing or publication until explicitly resumed.

4. **Save screenshot previews — IMPLEMENTED/USER ACCEPTED2026-11-01.** Capture the
   game window/frame when a save succeeds; display its preview below the save
   list when selecting that slot, both from the main menu and during play.
   User reference screenshots16:56:51/16:57:14 show placement. Latest user
   instruction fixes the module: **UI Tweaks**, implemented alongside satellite.
   Detailed task: docs/superpowers/specs/2026-11-01-save-previews-backlog.md.
   The screenshot includes HUD, minimap and character panels; exclude only
   save/load/pause menus and their backdrop, not the entire interface. Use the
   actual native game target, never a desktop/menu screenshot. The preview spans
   the dialog width; preserve image aspect ratio and cap height to available
   space. Hide the entire panel before selection or when no image exists.
   Latest user refinement: show existing-slot previews in SAVE as well as LOAD;
   raise the dialog once on opening, never jump on selection or crop the image.
   New snapshots use filtered1600x900 lossless PNG; legacy640 JPEG stays readable.
   MCM height100..160%, default120%, applies on next opening; list must grow too.
   Auto/quicksave preserve the visible HUD too. Existing saves without image
   remain usable; only confirmed successful saves receive a new preview.
   Installed b6a0589 candidate328192B; actual accepted test sidecar1600x900 PNG
   matches its native save revision. Full automated/native-geometry review passes;
   long-run performance measurement remains separate. Prepare the next public
   release once, relative to the recorded published baseline12, not repeated bumps.
5. **Container Highlight maintenance — installed2026-11-01.** Confirmed
   stale-model restoration retry fault addressed in fork commit9101ad8. Earlier
   fixes covered other paths; the player's exact freeze remains unverified.
   48 extracted production checks and independent review pass. Installed matching
   ba4ce0f candidate; PID13024 startup confirmed. Runtime travel/interior/Alt/save-load
   acceptance remains pending; do not claim the player's exact incident cured.
6. **Organize the Trader unsupported locale — audit closed2026-11-01.** English
   fallback passes for current and v7 loaders with packaged catalogs, including
   German variants. No production defect/new language added. User accepts this
   result; reopen only with concrete symptoms or logs.
7. **Item Markers known-location maps — IMPLEMENTED/USER ACCEPTED2026-11-03.**
   Same lower-left eye as learned blueprints, only when all original listed
   locations are discovered. Independent MapsKnown setting, three languages,
   bounded shared visible400ms snapshot and fail-closed unknown/refusal cases.
   Root and independent tests/native build pass; reviewed DLL and threeMO are
   installed and the user confirmed the known/unknown map check. Version7 ZIP,
   bilingual guide and notes were published as version7 on2026-11-04 after the
   user uploaded Nexus. Steam EN/RU and the personal release journal are updated.
   Do not repackage this unchanged feature as a new release.

## Maintenance completed in this continuation

- **LoK after author's FPS update: NCE checked2026-11-01.** Installed upstream
  `.mod` differs from the earlier file, but every ARMOUR record is unchanged.
  Regenerated NCE candidate is byte-identical to installed THE_NCE_Patch; no
  replacement is necessary. Evidence in
  `translations/LegendaryOfKenshiRU/nce-verification-20261101.json`.

## Completion boundary

A diagnostic render or screenshot is not completion of the satellite feature.
Completion requires an actual optional layer in the global map, user controls,
full requested geometry/lighting coverage and measured stability/performance.


## Deferred release batch: detailed localization logs (user, 2026-11-01)

COMPLETE2026-11-02: user uploaded all37 Nexus packages; all37 corresponding Steam
versions/content/ENRU descriptions/previews and single bilingual notes published.
Dark UI Advanced/Formations received description-only changes. See
docs/releases/2026-11-02-steam-publication.md and synchronized published ledger.
OTI/XP_Overhaul remain local; DF/global map remain private. Do not prepare or
publish this same batch again or increment versions merely for local fixes.


2026-11-02 user follow-up: optional global-map biome zones, diplomatic zones,
and road/path overlay layers, alongside the previously planned exploration fog.
Separate MCM toggles; native source evidence and no atlas recapture. See
GlobalSatelliteMap/PLAN.md. Future work after the active core coverage fixes.


## New MCM task — reset confirmation (user2026-11-05)

Implement centrally in Mod Config Menu for every plugin's common Reset button.
Clicking Reset opens one modal with a red warning that this plugin's current
settings will be lost. No setter, INI write, apply or reset before explicit
confirmation. Cancel/default focus, Escape and close dismiss safely. Changing
plugin/page, closing/rebuilding options or repeated clicks cannot reset another
plugin or retain a dangling UI/plugin reference. RU/EN/ZH, inherited vanilla/Dark
skins, bounded lifecycle and meaningful actual reset-flow tests. IMPLEMENTED and
installed locally 05Nov01:30MSK; MCM13 ZIP/ENRU descriptions prepared. Independent
review CLEAR; native VC10 GL/LTCG and actual reset45/modal51 checks Od/O2 pass.
User in-game cancellation/appearance check pending. Private local installation
receipt: docs/releases/2026-11-05-mcm-reset-confirmation.json.
Published baseline is MCM12 (confirmed ledger); one next public package is13,
not one increment per intermediate fix. No Nexus/Steam publication authorized
for this new package yet; no personal ALL_CHANGELOG entry until published.
