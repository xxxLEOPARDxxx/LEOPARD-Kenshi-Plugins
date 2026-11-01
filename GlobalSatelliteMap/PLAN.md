# Global Satellite Map: development prototype, 2026-10-31

The requested feature is an optional satellite view on the GLOBAL map. BestMinimap keeps its existing map source. The target atlas is exactly 4 x 4 tiles, each 8192 x 8192 (32768 squared logical pixels).

## First coherent stage

1. Extract C++03 policy into `SatellitePolicy.h`: strict bounded manifest parsing, half-open tile edges, visible-tile selection, mip choice and aggregate memory budget. Test this production header with VC10 before integrating UI.
2. Add a MapScreen::update child overlay, below native/custom markers, with no mouse/key focus. Never change mapImage's texture, coordinates, events or child markers. Use static WidgetRef references for all owned widgets.
3. Accept only a complete genuine capture manifest (16 tiles, six independently loadable BC1 DDS levels 8192..256, SHA-256 per resource). Do not provide fabricated images or an example manifest that can accidentally be enabled. Validate headers before any overlay, hashes before upload. Load at most one visible level per 200 ms; strict compressed-data cache budget 96 MiB, maximum 16 resident textures, no full-atlas RGBA expansion.
4. Keep capture input identity and save identity separate. Manifest sources enumerate exact hashed capture dependencies including mods.cfg and base data. Require every enabled mod filename to occur among verified sources; missing/unresolvable sources fail closed. Save identity hashes native quick.save and all zone/platoon files, with hard file/byte limits. Path, live world pointer and load/new-game generation invalidate resident data. No changes to save/world files.
5. MCM switch defaults off; three locales; Debug=0 is last. Missing/invalid assets leave overlay absent and show localized status once per attempt. Native load/new-game hooks only invalidate plugin state before calling originals. No install, release archive or capture invocation.
6. Build VC10 /GL + /LTCG; run extracted-policy and lifecycle/source-contract tests. Report source/DLL hashes and limitations. Full UI/game/FPS/leak acceptance remains pending.

## Explicit later stages (not substituted by this prototype)

Prove orthographic/global projection and fixed exposure on a small terrain/hill/building overlap. Audit DistantTown ownership, render-only geometry, native world lighting, player/modded building changes and unloaded regions. Then implement a native dynamic layer and efficient staged capture with globally consistent mips/edges. A static captured preview is not the requested continuously lit world; two photographs cannot replace native sun/shadows/lamps. No force-activation of all zones.

## Constraints / review focus

Existing APIs: MapScreen::update and worldToMapCoords; SaveFileSystem::loadGame/newGame/getActiveSave; Ogre's native BC1 image/texture loader. Important risks: map window destroyed/recreated, same-slot reload, interrupted loads, marker stacking, zoom/scroll boundaries, corrupt DDS, memory exhaustion and synchronous disk/GPU stalls. First-stage source validation is bounded but can be expensive: it runs only on explicit enabling/reload with a genuine manifest, never every frame. No runtime performance claims before measurement.


## Active capture phase, 2026-11-01 (DF development paused)

The user explicitly switched the active task to real satellite rendering after
the final focused DF retry correction. The old viewer remains disabled and
uninstalled; it is not the completed satellite feature.

Canonical one-shot diagnostic source: tools/diagnostics/satellite-capture-probe.
Root installed only that temporary diagnostic under game/mods/Codex Satellite
Capture Probe, with an exact mods.cfg backup and a separate SAT-render-proof
save copy. The RE log confirms the corrected C++ startup entry and native byte
profile. First actual manual mode0 request reached Ogre but failed before a
capture: createCamera already attaches to Root SceneNode and attachObject
therefore rejected the second attachment. No image was created.

The matching SDK documents this behavior explicitly. The production ownership
helper now transfers ONLY the newly created camera to the probe node, rejects
the main camera and its node, and leaves all destruction to existing tracked
cleanup. RED reproduced the existing-parent failure; GREEN11attachment tests,
13projection/cleanup checks and55native imports. Corrected81408-byte DLL
SHA256d8db85d1a8d4d7864b7a96cfbd92e8f124752b60e83e599396f3183095cf4a42
was installed with the game closed. All five control modes subsequently produced BMPs.
No shader/game data file or native save was edited.

Next: same paused camera with global map open, mode0 then1 (control geometry),
then2/3/4 with the same fixed global cell. Inspect final/GBuffer evidence and
normal-view restoration after each. Obtain an actual512image before extending
to8192, acquiring render-only remote geometry or claiming illumination solved.
The native deferred shaders reconstruct perspective radial depth; plain
PT_ORTHOGRAPHIC still does not establish correct lighting. Existing root/distant
render geometry does not establish complete unloaded/player/modded structures.

### Control evidence obtained, 2026-11-01

All five modes were captured in process31268/startUTC2026-10-31T22:19:51.2039120Z.
Mode0 shows Hub buildings/terrain. Mode1's geometry differs from its supposed main-camera
clone (GBuffer101 versus4367 colors and different depth). Mode2/3 share byte-identical
GBuffer0 but markedly different final illumination; mode3/4 top-down images are close.
This narrows the next work to camera transform-cache timing and native camera-dependent
lighting, rather than assuming an independent camera cannot render geometry.

Private evidence manifest and filtered log are in
game/_codex_work/global-satellite-20261101/controls-31268. Raw images are diagnostic,
not release art or completed8K tiles. Ordinary map/background view was visible afterward;
full state restoration/performance and remote-world coverage remain open.

Next candidate adds bounded per-request parent cache, camera pose, actual view/projection,
cull/LOD and nonfinite-entry telemetry at clone/before-aim/pre-shot. It deliberately does
not refresh the new node before baseline: actual Ogre convertWorldToLocal reads cached
parent data directly. After this measurement, select one corrective transform experiment.
The perspective radial-depth lighting limitation remains independent of this clone defect.

Matrix baseline subsequently proved the clone defect: new-node cached translation0 became
51212.375,0,-2622.64819 before rendering, adding that shift to the copied camera position;
view delta rose from0.049 to50057.6874 while projection stayed equal. OwnedCameraPose now
refreshes only its explicitly bounded root/owned ancestry before conversion, via public
Ogre getter. Production regression first reproduced the live offset; correction and ancestry
refusal tests pass. Live check subsequently confirmed stable parent cache and camera pose,
view delta0.0927734 instead of50057.6874, about1.24mm positional difference and a visible
Hub image from the cloned camera. Exact matrix equality is not claimed. Next: private
orthographic depth reconstruction and image-based/native-lighting validation.

Position reconstruction stage: original HLSL helper plus real SM4 quad is tested in isolated
D3D11 WARP (33cases; initial perspective formula failed23). The candidate capture probe
embeds it and runs an explicitly named private512FP32RGBA pass over its own GBuffer depth.
One fixed definition pair; request-only work; no shared game shaders/providers altered.
Unknown auxiliary workspace ownership retains the bounded resource set and disables further
requests. Next acceptance is actual game shader compilation/reprojection/readback; this
alone still cannot prove depth Y orientation or sun/shadow/fog/water correctness.

Actual mode3 position pass succeeded in the game:262144 valid,0invalid/nonfinite,maxNDC0,
max radial residual0.000370985419m. Native sun/ambient next candidate reuses installed PBR
includes and native no-shadow texture configuration, explicit capture basis/parallel view
direction and private frozen shared values. Ambient UV accounts for the camera already
baked into native worldSize.zw;45actual-HLSL reference-camera cases and an omitted-delta
mutant cover this offset. Its separate linear HDR target/BMP is diagnostic, with no shadows,
fog, local lights, water or tone mapping. Runtime compilation/reference telemetry comes next.

Two actual sun captures compiled/ran and recreated resources, but colour was wrong: compiled
legacy samplers had s0albedo/s1normal/s2depth while implicit SRVs were t0depth/t1albedo/t2normal.
Finite HDR output is therefore explicitly NOT colour acceptance. Corrected shader uses
explicit modern t0..5/s3..5, reflection contracts and64GPU decoder cases (+28failing no-clamp
mutant). A bounded manual-only source override permits future HLSL iterations without DLL
replacement; generated override must pass compile/reflection checks before copying. Next:
repeat colour capture with corrected bindings, then native HDR/tone mapping and shadows.

## Isolated terrain experiment — continuation ruling, 2026-11-01

Use systematic-debugging, writing-plans and executing-plans techniques without
skill-only approval gates, per the user's persistent override and unattended
computer authorization. Reuse the current diagnostic checkout; no public release.
The exact native audit is docs/verification/2026-11-01-satellite-terrain-audit.md.

- [x] Prove current native ABI exports, cache178/180 and persistent helper ownership.
  Preserve real four-shot evidence; correct colours now live, far resident terrain
  remains coarse. Main Terrain selection/build caches are not transactional.
- [ ] Add a separate private Terrain experiment, not an expansion of main-Terrain
  cache snapshots. Files: TerrainExperiment.h, TerrainExperimentPolicy.h,
  terrain_experiment_tests.cpp, exact additional ABI entries. A fresh SceneManager
  owns fresh Terrain/stream/Landscape and camera. Do not copy Terrain, use shared
  detailed-material callbacks, activate zones, move actors or write saves.
- [ ] Fixed native full-world bounds +/-147456; primary cells73728. Diagnostic
  output512² first, camera altitude12000 (then16000 comparison), far120000.
  Keep floating-origin conversion consistent with native material worldOffset.
  Use genuine DistantTerrain, an isolated three-surface GBuffer and existing
  private sun/position passes. Default disabled and one explicit INI token/cell.
- [ ] Cap warmup at60s/600frames, per-frame builds32/8, memory/patch/triangle
  budgets. Require stable nonzero geometry under deadline; never label a timeout
  complete. Abort on save/load/world/manager/context change; no background retry.
- [ ] Destroy workspaces before materials/textures and Terrain before its borrowed
  camera, then SceneManager. A failed identity guard retains one bounded resource
  set and disables the diagnostic. Test production ownership/stability policy,
  allocation failure paths, clock wrap and primary-cell boundaries.
- [ ] Build/profile/import/source contracts, independent review, install with game
  closed. Root uses CUA to launch/load SAT-render-proof; no user action needed.
- [ ] Capture one large primary cell, two altitudes and repeat cleanup. Archive
  actual counters/images/memory; assess LOD/material shortcomings honestly.

Ruling: postpone8192 allocation until isolated512 coverage and lifetime pass.
This avoids paying gigabytes for a coarse/incomplete scene. Unloaded town proxies,
detailed biome materials, full sun/shadows/fog/water and runtime day/night refresh
remain independent unfinished requirements; do not publish a static preview as
that complete feature. BestMinimap remains unchanged; DF remains paused.

### Unattended native proof checkpoint
The isolated Terrain construction/world bounds/guards/worker teardown and actual
512 primary-cell experiment above are now implemented and live-verified. See
`docs/verification/2026-11-01-satellite-isolated-live.md`:60 captures plus clean
autosave abort, two identical16-cell surface grids, explicit native HANDLE repair,
and17 fresh-material accounting-balanced captures. Whole-process memory acceptance
is still OPEN. Shader reuse is an opt-in next A/B, not a demonstrated memory fix.
Do not allocate final8K targets by simply increasing the diagnostic Side constant.

### Exposure and bounded-resolution checkpoint
One-lifetime batch083ee5d passed three16-cell512 runs (all48 raw pixel arrays
match independent fresh-cell references), oneworker perbatch. Source340e7ca
adds native exposure snapshot and frozen lighting, with two live16-cell runs
accepted. Raw and exposed diagnostics are separate, no shared HDR writes.
See verification report and private evidence, not old pending stage notes above.

Next bounded trial: DiagnosticSide2048(single primary cell), immutable from
startup, explicit named-surface/CPU-vector estimates, old geometry limits intact.
This is a staged refinement/resource experiment, not final8K. Native viewport
height increases LOD metric, so keep detail(8,8,0) unchanged for comparison.
Compare geometry/nativepeak/output vs512, repeat cleanup, then assess a primary
boundary with overlap before choosing the final large-target/budget strategy.

2048 proof58a749f is now accepted for standalone cells (including actual transient
index-count recovery); repeated raw surfaces are identical. The16-cell batch
hits its1GiB native limit after five cells and cleans up. Do not increase the
limit blindly. Next design must recycle native geometry between primary cells
while retaining bounded render resources, investigate process-memory retention,
then measure adjacent world samples/LOD morph seams. Final8192 targets remain
disabled. At checkpoint the game is closed/private probe disabled, source pushed.
