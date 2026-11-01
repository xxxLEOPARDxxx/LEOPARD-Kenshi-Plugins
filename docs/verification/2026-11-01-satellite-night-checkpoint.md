# Satellite map: unattended checkpoint

This is an engineering checkpoint, not a completed/public satellite-map release.
User authorized unattended launch/load/input while asleep; DF stayed paused.

## What is actually working

- Isolated real native Terrain, no NPC-zone activation or main-terrain cache writes.
- All16 requested primary world cells at512 diagnostic resolution, repeated.
  Three one-lifetime batches matched48 independent reference pixel arrays exactly.
- One worker/scene per512 batch, around7–10seconds per full grid.
- Native sun/ambient and exposure snapshot, fixed across a batch; raw HDR preserved.
- Standalone2048 captures, repeatable raw geometry/light, about3–4seconds/cell.
- Native worker HANDLE/startup race and private material-accounting defects repaired.
-370 production/GPU checks, actual import/profile contracts and independent review.
  Source commits through58a749f pushed to origin/main.

## Findings which prevent calling it finished

- Complete8K tiles are still disabled;2048 multi-cell geometry crosses the current
  native memory budget after five cells. It aborts and cleans up; recovery succeeds.
- Whole-process retained memory still grows. Named resource/handle cleanup is
  proven, but this does not establish absence of every allocator/driver/native leak.
- Native LOD and height/normal morph depend on camera distance, so seam-free
  adjacent geometry is not yet proven. Simply increasing target size is insufficient.
- Detailed biome materials, full unloaded/player/modded building geometry, water,
  shadows/fog/local lights and interactive global-map/day-night refresh are unfinished.

## Review artifacts

Actual diagnostic image (no invented content/color grading):
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/isolated-native-exposure-9868-first/native-exposure-grid.png`.

Standalone2048 cell:
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/isolated-2048-28960-first/isolated_terrain_28960_70911109_1_sun_noShadow_native_exposure.bmp`.

Full measured evidence and limitations:
`docs/verification/2026-11-01-satellite-isolated-live.md`.

Game closed normally. Private probe removed from mods.cfg and its request API
disabled; other mods.cfg bytes preserved. Public mod files/settings and named
source saves were not overwritten. Natural autosaves occurred during the tests.
