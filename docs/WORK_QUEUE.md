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
   LATEST GOG TESTER FAILURE/FIX05Nov03:26: initial archive fab7... failed
   startup with MISMATCH au ours=0, not a successful GOG runtime test. The native
   profile omitted au storage21322B0; exact official SDK slot7627 and both EXE
   RIP references prove GOG2130220. Local commit84056bcb adds only that mapping
   plus reverse; startup guard is unchanged. All107 actual Resolve entries now
   tested, including its9-global loop; old-profile RED and removed/wrong/null
   target controls catch the former coverage blindspot. Independent review CLEAR.
   Rebuilt PROD0d0ff2b79838d74db4236662f7d8cefa702e2162bdd732d592be22487fd1d766;
   both private GOG/weight test ZIPs refreshed, each762976B SHA
   ff14c090184028d6165f03b68dfbdec07fefe84698c6af04877238c9c4009a18.
   The current local Steam game PID20904 still runs weight buildd530..., which
   is unaffected by the GOG-only table entry. No local replacement while running.
   Tester must retry actual GOG startup; do not claim the port now runs in-game.
   Three Steel jobs are NOT implemented: owner had misunderstood prior wording,
   corrected explicitly. Only carried weight is installed, OFF by default.
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

   Latest actual INI check05Nov: keoRouting=true/plannerMode=on, enabled by the
   user. The user says routing currently appears to work, but broad gameplay
   testing remains required; do not equate this with proving every route fixed.
   Preserve the user's settings and measure the active loaded DLL each session.

   GOG NATIVE CANDIDATE READY05Nov: local source e6082d6, full production DLL
   eaa75821e941f85fe95b4e3a7ce7fa3fde17c564d03a508a48132b85303b2859.
   Private tester ZIP workshop/testing/KEO/KEO-GOG-test-20261105.zip, CRC/exact11
   files/144SDK+29MyGUI imports verified. Real GOG gameplay remains UNVERIFIED;
   package is not installed or published. Patch backup is under
   tools/maintenance/keo/native-gog-e6082d6264b6.patch in our own repository.

   GOG provenance: user supplied game/DropMeFiles_VfFSw.zip
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

   NEW05Nov user request: compare actual KenshiQOL sources with KEO and port
   useful missing features carefully, including dynamic carried-body weight.
   Source: workshop/content/233860/KenshiQOL/src, Workshop3772744830, author
   Steel confirmed by its original page; bundled GPLv3 retained. Read-only audit:
   game/_codex_work/20261105-kenshi-qol-audit/report.md and evidence.json.
   Identified overlaps: global backpack routing vs KEO wearer choice; QOL zone
   auto-rebuild triggered by KEO preload; nearest-town preview overwrite;
   shared getDropped/order hooks. Do not copy the combined navmesh switch.
   FIRST TRANSFER INSTALLED05Nov03:07MSK: source f568b971, PROD DLL
   d53062b8bc2b0dee5107ad5cc4174de10b73a7267507411042fb57e20bb58f37,
   1489920B; opt-in carriedWeight default OFF and MCM restart marker. Personal
   KEO.ini/KEOCompat.ini/REjson/mods.cfg byte-preserved, backup
   D:/DEV/Kenshi/backup/keo-weight-20261105. Root repeated125/411/13 Od/O2 and31
   native contract controls; independent review CLEAR; both native builds and
   config/layout/MCM regressions PASS. Private test ZIP
   workshop/testing/KEO/KEO-weight-test-20261105.zip, CRC12files verified.
   Real weight gameplay/FPS/memory/save-load acceptance is still pending.
   New jobs/gate/drop changes are NOT installed. Gate native registration gap
   depends on data flags; do not claim universal missing registration.
   Upstream post-drop copy treats native query -1 as success; never copy that.
   Implementation: opt-in dynamic body/inventory weight, startup-only,
   bounded allocation-free traversal and thread-local recursion protection;
   disable this feature if the original KenshiQOL DLL is loaded. Preserve native
   inventory weight semantics, robot/hunger distinction and save data.
   Next isolated candidates: gate registration and post-drop building handle.
   New jobs require separate all-or-nothing capability/save-lifecycle design:
   Gather Output first, shop-counter stocking second, queued movement last.
   Do not silently migrate all native task242 rows or modify saves with OFF.
   User clarified05Nov: expose all three jobs as separate MCM switches with
   meaningful descriptions. Actual upstream job291 Gather Output empties machine
   outputs even without a matching storage demand; job293 Stock Shop Counter
   moves the output types of that actor's Gather jobs (or native ditchable
   resources when none exist) into a counter, without money changing hands;
   job292 Waypoint Route loops through its Jobs-panel waypoint rows. Preserve
   saved rows when OFF, suspend only our own task execution/assignment, and keep
   dormant deserialization safe. Never turn unrelated native jobs into ours.
   For stocking, verify ownership and capacity, bounded transfers and loss-free
   rollback; do not inherit upstream's unguarded arbitrary shop destination.

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
User confirmed the in-game cancellation/appearance check: works. Private local installation
receipt: docs/releases/2026-11-05-mcm-reset-confirmation.json.
Published baseline is MCM12 (confirmed ledger); one next public package is13,
not one increment per intermediate fix. No Nexus/Steam publication authorized
for this new package yet; no personal ALL_CHANGELOG entry until published.
