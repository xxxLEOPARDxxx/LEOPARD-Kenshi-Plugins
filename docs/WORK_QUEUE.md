# Active work and next requested tasks

Updated 2026-11-04. This is a private working queue, not a release announcement.
The user is at the PC. Ask for scene preparation; use background telemetry and
file requests. Do not take desktop focus or collide with Minecraft autotests.
Always check the actual process immediately before replacing a loaded DLL.

1. **Global satellite map — new reported zoom/seam defects, 2026-11-04.**
   Current private installed build75f41fe, local VRAM8GiB/RAM32GiB. MCM offers
   VRAM1..8 and RAM1..32 in1GiB restart-required steps; RAMLRU fills on demand,
   not an eager32GiB allocation. Live232 entries/243478872B and418hits confirmed.
   The accepted16-tile atlas is unchanged; no new full render is needed here.

   User reports x2..x4 blur, slow x6/x8 cold refinement and a horizontal seam.
   Confirmed96-window planning cap rejected the entire3m intermediate view.
   New bounded512 planner admits those views, retaining96 as the1m threshold.
   NativeMain progresses up to8 already-ready transitions/1.5ms perframe, stops
   immediately on unchanged asyncwait, and paints once. Fair initial-priority
   round-robin shading prevents tail starvation with the wider visible set.
   Source/host tests and independent reviews pass; new DLL runtime acceptance
   is pending. No fivefold speedup/FPS or universally leak-free claim.

   Hub supported geometry was rejected by its128MB-worth of existing texture
   references; scoped loaded-town fresh32/128MiB and old+fresh64/256MiB bounds
   now preserve legacy32/64 elsewhere. Native borrowed bytes are not newly
   allocated mapGPU. Actual Hub scene acceptance still pending. Ordinary objects,
   additional material families, full unloaded-city detail and shadows unfinished.

   Current seam source planes are continuous, matching lightingepochs and GUI
   bounds; finalRGBAoutput diagnosis is next, via explicitprivate two-tile512KiB
   asynchronous readback. Do not claim seam fixed or recapture fullatlas onguess.
   Detailed evidence: docs/verification/2026-11-04-gsm-zoom-followup.md and memory-
   caches.md. Atlas, personalINI, unrelated mods and publishedjournal preserved.
   Fog, biome/diplomaticzones androads after core fixes. DF remains paused.
   No satellite public release authorized; private devbuild only.

2. **KEO — after terrain/detail; satellite remains the main task.** User2026-11-02
   explicitly asks to read current Steam reports/source now: broken pathfinding,
   town inhabitants spawning outside and slowly returning, excess squads at normal
   settings, GOG refusal, and possible mouse-input latency (unconfirmed). The user
   has now explicitly requested parallel agent implementation. A scoped startup
   routing-OFF fix has passed independent review; it is not a full KEO release.
   Read-only spawn/GOG audits are in research/keo-spawn-audit-20261102 and
   research/keo-gog-audit-20261102. Neither report proves the spawn defect fixed. Keep
   player reports separate from verified causes; inspect native streaming/spawn
   readiness, routing and build gates. Research: `research/keo-audit-20261102`.
   New user request explicitly authorizes
   a fork under MCM, bug fixes and compatibility. This supersedes the historical
   local-only KEO intent. Before implementation read current upstream source,
   license and player reports/issues; record actionable reports and reproduction
   cases. Fix verified causes with performance/lifecycle review and game checks.
   Do not claim every unknown report fixed merely because it was read. Apply
   normal three-language, debug-last, release/credit and Nexus→Steam rules.
   User clarification2026-11-02: this is a temporary maintenance fork, not an
   independent development project. At release explicitly explain in EN/RU that
   the fork is a necessary interim measure to fix bugs and let more players try
   KEO. If the original author returns and continues development, withdraw our
   version to avoid confusing players. Do not promise new independent features.
   Preserve original authorship, license and source links; no withdrawal or
   publication is authorized merely by recording this future release policy.
   2026-11-03: reviewed local6e15b3a with MCM21player rows, three MO catalogues,
   debug-last/defaultOFF and request-local path budgets is installed. Current
   process loaded the module and MCM registered it; no ordinary diagnostic flood
   with DebugOFF. Personal INI/KEOCompat preserved. One optional module-census
   notice is overclassified as an error; correction is in the next scoped build.
   Pre-enqueue priority publication race is reproduced; the scoped fix in local
   commit680a2b5 passed independent review and awaits installation/game checks.
   Do not confuse this with complete routing/spawn
   repair or GOG support. Origin remains the author's repository; reviewed local
   commits are backed up as exact patches in ours/tools/maintenance/keo and pushed
   to our repository, never to the author's origin.
3. **Dynamic Factions — PAUSED by user.** Preserve current work and plans;
   resume when requested, after the satellite priority. No publication.
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
   bilingual guide and notes are prepared relative to published6. Await user's
   Nexus upload; no Steam publication or personal release-journal entry yet.

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
