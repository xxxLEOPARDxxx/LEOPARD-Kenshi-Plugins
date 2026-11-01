# Satellite map: unattended checkpoint,08:32 Moscow

This is an engineering checkpoint, not a completed/public satellite-map release.
User authorized unattended launch/load/input while asleep; DF stayed paused.

## What is actually working

- Isolated real native Terrain, no NPC-zone activation or main-terrain cache writes.
- All16 requested primary world cells at512 and2048 diagnostic resolution, repeated.
  Native Terrain recycles per cell while one Scene/camera/render set remains.
- A complete4×4 control grid of16 genuine8192² tiles in123.5seconds,15 Terrain
  recycles,16 joined/closed workers and one removed Scene. All48 GPU readbacks finite.
- Native sun/ambient and exposure snapshot, fixed across a batch; raw HDR preserved.
- Standalone4096/8192 captures with identical raw repeats, about3–9seconds/cell.
- Native worker HANDLE/startup race and private material-accounting defects repaired.
-442 production/GPU checks, actual import/profile contracts and independent reviews.
  Source through6e46a6b pushed to origin/main; runtime evidence recorded separately.

## Findings which prevent calling it finished

-8192 raster control intentionally preserves2048-equivalent native Terrain geometry.
  Final full-height mesh quality/seam continuity are not accepted. Native distant
  colour texture is visibly coarse; larger raster does not make it detailed.
- Whole-process retained memory still grows. Named resource/handle cleanup is
  proven, but this does not establish absence of every allocator/driver/native leak.
  Full8K grid process private bytes7.71GB baseline,16.51GB peak,11.26GB idle.
- Native LOD and height/normal morph depend on camera distance, so seam-free
  adjacent geometry is not yet proven. Simply increasing target size is insufficient.
- Detailed biome materials, full unloaded/player/modded building geometry, water,
  shadows/fog/local lights and interactive global-map/day-night refresh are unfinished.

## Review artifacts

Actual diagnostic image (no invented content/color grading):
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/isolated-recycle-8192-32140-first/satellite-8192-grid-review.png`.

Full8192 original tiles, hashes, identities/config/log/memory evidence are beside it.

Full measured evidence and limitations:
`docs/verification/2026-11-01-satellite-8192-control.md` and
`docs/verification/2026-11-01-satellite-recycled-grid.md`.

Game closed normally. Private probe removed from mods.cfg and its request API
disabled; private raster/control/batch reset512/off. Other mods.cfg bytes preserved.
Public mod files/settings and named
source saves were not overwritten. Natural autosaves occurred during the tests.
