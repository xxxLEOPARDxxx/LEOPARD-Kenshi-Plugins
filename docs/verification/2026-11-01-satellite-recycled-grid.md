# Private Terrain recycling: live evidence

Source f150aef, DLL SHA256
`984f32675a81954743e4a5f4c12a8cb5251f0edfed469cf6f7847672a826e524`.
Unattended controls were authorized by the user. DF development stayed paused.

## Actual runs

| Output | Process | Request | Elapsed | Cells | Recycles | Joined workers | Destroyed Scenes |
|---|---:|---:|---:|---:|---:|---:|---:|
|512²|13260|1|10922ms|16|15|16|1|
|512²|13260|2|9584ms|16|15|16|1|
|2048²|13956|3|31574ms|16|15|16|1|
|2048²|13956|4|30265ms|16|15|16|1|

Each batch retained one Scene/camera/render surface set while replacing the
owned native Terrain outside compositor iteration. The real native worker was
kernel-joined before destruction, and its omitted original HANDLE close repaired.
No gameplay zones were activated, no main Terrain cache written, no global
render queue eviction performed. The geometry1GiB/16M limits were unchanged.

At512 all32 raw arrays exactly match independent fresh-cell references. At2048
all16 raw GBuffer arrays match between batches, and cell1,1 matches the earlier
independent2048 reference. Raw Sun is not fully identical: maximum difference14
8-bit levels at very few pixels, average channel difference~0.000003 across the
16 images. Native-exposure variants differ by at most12 levels, mean~0.000977.
Do not turn these measurements into an assertion of identical lighting/meshes.
All96 position/raw-Sun/exposed-Sun GPU readbacks were finite. Position diagnostics
covered all4194304 pixels per cell, with zero invalid/nonfinite values.

## Resource and memory limits

Native peak751566080B, maximum4037 patch records,3714 Ogre drawables and1764
visible patches. Native reported triangle estimate15121006 is only~5.5% below
the16M limit. It is the native first-index-count estimate, not an exact sum.
Named texture/material counters returned to1956615101/10766708B; shader cache
retained its fixed3 programs, as designed. The512 process likewise balanced
named resources and workers.

Whole-process retained memory is NOT accepted:2048 idle private bytes were
7750610944 before the first batch, about8132333568 after it, and8224743424 after
the second. Idle handle/thread counts remained near14600/123. Other installed
mods and native allocators/driver caches were still present; static matching
delete calls and balanced Ogre counters cannot identify all retained allocations.
No global heap trimming/cache eviction was used to conceal this trend.

## Evidence

Private artifacts under
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261101/`:

- `isolated-recycle-512-13260-first` / `second`: images, controller log, memory,
  production identity snapshots, exact reference checks in recycle-proof.json.
- `isolated-recycle-2048-13956-first` / `second`:48 BMPs each, logs/INI/telemetry,
  controller timings and recycle-proof.json.
- `isolated-recycle-2048-13956-comparison.json`: pixel comparison and finite checks.
- `check-recycled-grid.py`, `check-recycled-2048.py`: actual image/log validators.

Final8192 tiles, full native-height mesh refinement, adjacent-LOD seams, detailed
biome/building/LOD/water/shadow passes and global-map integration remain unfinished.
The next explicit high-resolution raster control preserves2048-equivalent Terrain
geometry while measuring4096/8192 allocation; it does not resolve these limits.
