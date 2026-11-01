# Active work and next requested tasks

Updated2026-11-01 from the user's current requests. This is a working queue,
not a public release announcement. Latest instructions override historical notes.

1. **Global satellite map — ACTIVE, unfinished.** Deliver an optional working
   GLOBAL map layer,4x4 primary tiles at8192²; preserve BestMinimap's current map.
   Current proof covers private near-terrain layers and four native painted
   pairs in one512 cell with balanced resource counters; it is not a finished
   game feature. Complete geographically correct biome mixing/mips, geometry boundaries, building/model
   coverage, water/shadows/day-night and viewer integration. User authorized
   autonomous launch/load/input when away. Latest clarification2026-11-01:
   while the user writes at the PC, do not launch/focus/switch windows; ask the
   user to prepare game scenes, then use file requests/read-only telemetry in
   the background. Preserve personal settings and use test saves.
   User clarification2026-11-01: dynamic lighting includes actual torches,
   lanterns and similar local sources, not only sun/day-night. MCM must offer
   separate switches for lighting components, city models and other render layers;
   see GlobalSatelliteMap/PLAN.md. Full atlas rerender on every lamp change is
   not an acceptable update strategy. Article finished; still check process and
   avoid taking window focus without a new instruction.
2. **KEO MCM fork — AFTER satellite.** New user request explicitly authorizes
   a fork under MCM, bug fixes and improvements. This supersedes the historical
   local-only KEO intent. Before implementation read current upstream source,
   license and player reports/issues; record actionable reports and reproduction
   cases. Fix verified causes with performance/lifecycle review and game checks.
   Do not claim every unknown report fixed merely because it was read. Apply
   normal three-language, debug-last, release/credit and Nexus→Steam rules.
3. **Dynamic Factions — PAUSED by user.** Preserve current work and plans;
   resume when requested, after the satellite priority. No publication.
4. **Save screenshot previews — DEFERRED, requested2026-11-01.** Capture the
   game window/frame when a save succeeds; display its preview below the save
   list when selecting that slot, both from the main menu and during play.
   User reference screenshots16:56:51/16:57:14 show placement. Plugin not chosen;
   decide between a UI Tweaks feature and a dedicated module when work starts.
   Detailed task: docs/superpowers/specs/2026-11-01-save-previews-backlog.md.
   Record now only; do not start implementing ahead of the active satellite task.

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

After current satellite work, prepare releases for all 37 affected public packages. Exact list and exclusions: tools/maintenance/localization-release-queue.json. Verify actual published versions, then increment once per package; prepare ZIP/CRC/EN-RU notes and current README. Debug implementation details stay out of player descriptions. Nexus is uploaded by the user first; Steam follows. OTI remains a local-only build and is excluded from publication, preserving the user decision recorded09.10; it is not in the release catalog. DF/global map are private, XP_Overhaul is a local compatibility build. Current public ZIPs have not yet been rebuilt and must not be mistaken for the new DLLs.
