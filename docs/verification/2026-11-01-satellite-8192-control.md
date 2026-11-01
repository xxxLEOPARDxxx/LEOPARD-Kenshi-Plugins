# Genuine8192 raster control: unattended runtime proof

This is real engine output, not a generated illustration. It is still a private
distant-terrain experiment, not the requested complete satellite global map.
The output resolution is8192², while native Terrain detail preserves the proven
2048 control. Full native-height mesh quality and seams are not accepted.

## Source and controls

- a02503f introduced explicit high-resolution allocation control and exact native
  viewport-height validation; independent review clear,437 production/GPU checks.
- 6e46a6b corrected waiting on unpublished indices when native Info.unbuilt=0;
  independent review clear,442 production/GPU checks and116 native imports.
- Installed6e46a6b DLL SHA256:
  `7f5a7a1e9193e81554904b04281be2d7556846d97233cbf176ceae1fb1800fd2`.
- DiagnosticSide8192, ResolutionOnlyControl1, detailT=F32/D0 preserves the2048/T8
  native LOD decision/morph ratios when actual viewport height is8192.
- Existing1GiB native/16M estimated-index limits were not raised. Host physical/
  commit preflight requires8053063680B before Scene allocation. Actual adapter16GiB,
  roughly11GiB free before large requests, separately measured.

## Live evidence

| Resolution | Process | Request | Scope | Elapsed |
|---|---:|---:|---|---:|
|4096²|1036|505|cell1,1|4874ms|
|4096²|1036|605|same-cell repeat|3406ms|
|8192²|240|905|cell1,1|8627ms|
|8192²|240|1005|same-cell repeat|7242ms|
|8192²|32140|1100|all16 primary cells, one Scene|123535ms|

4096/8192 raw GBuffer and raw Sun repeat arrays match byte-for-byte. Native exposure
repeat images differ by at most one8-bit level from separately sampled live
adaptation. Individual position diagnostics were valid and finite at every pixel:
16777216 at4096 and67108864 at8192.

The full grid saved exactly16 tiles for each of three diagnostic outputs (48
BMPs,12884904480B),15 native Terrain recycles,16 kernel-joined/closed workers and
one removed Scene. Every tile is8192² with expected268435510B BMP size. All48 GPU
position/raw-Sun/exposed-Sun validations were finite; one frozen sun/exposure
snapshot per batch. No extra micro-tiles or rescaled source outputs. Only the
scientific contact sheet is reduced for review.

The120s batch deadline is cooperative between work units: the last cell started
within it, but final readbacks/file writes completed at123.5s. This is not a hard
120s wall-clock guarantee. Large synchronous proof readbacks/IO should not be
part of the eventual interactive map path.

Two earlier8192 attempts inPID32020 deliberately aborted before any image because
the native estimate was exact uint64(0)-2 with unbuilt=0. Native audit established
that unbuilt counts deferred GPU drawable builds, not CPU-worker completion.
After6e46a6b, PID240 visibly passed through this same state, recovered and required
three valid stable observations. The policy does not clamp/accept an unknown
estimate; all other budgets/deadlines remain.

## Memory/performance remains open

Full-grid native geometry peak751566080B. Whole-process private bytes peaked
16507719680, compared with7713075200 before the batch. After idle it retained
11256291328. Idle handles/threads14606/123 compared with14608/124 initially.
Thirty external GPU samples during part of the batch were around8.5–8.7GiB;
they do not prove the absolute GPU peak. After idle, aggregate GPU usage7969MiB
compared with4668MiB before. Named Ogre counters/Scene counts returned, but this
does NOT establish absence of allocator/driver/native retained resources. Two
earlier separate8K requests retained~14.5GB process private bytes. No global cache
eviction, heap trimming or driver reset hid these measurements.

## Artifacts and shutdown

Private evidence root:
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/`.

- `isolated-control-4096-1036-first` / `repeat`, `control-4096-1036-proof.json`.
- `isolated-control-8192-32020-first` / `retry`: rejected attempts.
- `isolated-control-8192-240-first` / `repeat`, `control-8192-240-proof.json`.
- `isolated-recycle-8192-32140-first`:48 images, baseline/idle/samples/log/INI,
  GPU samples, atlas-proof.json with16 source hashes, and
  `satellite-8192-grid-review.png` (actual reduced contact sheet).
- `clean-shutdown-after-8192`: closed-process proof, exact mods.cfg before/after,
  request settings/final log. Only the private probe line removed; other bytes
  preserved. Request API/control/batch disabled; Side512.

Game closed normally. Named source saves were not manually overwritten; natural
autosaves occurred. BestMinimap/public packages were not changed; DF development
remains paused. Nothing was published.

## Still required

1. Identify/reduce retained large resources and full-size proof buffers.
2. Prove full geometry quality and shared boundary samples/LOD morph continuity.
3. Genuine near biome materials: current DistantTerrain colour texture is visibly
   coarse. Larger raster dimensions do not make that texture detailed.
4. Loaded/unloaded/player/modded buildings and model LOD coverage, water, shadows,
   fog/local lights, then correct native day/night refresh.
5. Package/stream the optional global-map layer with real dependency identity,
   seam-safe mips and UI/FPS/memory acceptance. This grid is not that layer.
