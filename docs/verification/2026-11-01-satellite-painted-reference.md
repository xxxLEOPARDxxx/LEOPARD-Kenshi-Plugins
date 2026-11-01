# Native painted input reference, 2026-11-01

Accepted scope: the four real painted overlay/colour pairs of primarycell1,1
in a512 single-biome private Terrain reference. This is not the completed global
map. No biome mixing, water, buildings, shadows/fog/local lights or final atlas
viewer are accepted. Best Minimap and shared game terrain are unchanged.

## Coordinates and shader

Native overlay bounds are documented in the preceding biome-reference report.
The guarded exported Terrain callbacks exist, but the callback int is always0,
not LOD; coarse quadtree patches can cross paint boundaries. A center-selected
material would incorrectly stretch one painted tile across others. No callbacks
or native fields were installed.

Instead a private fragment shader retains the exact installed terrainfp4 math,
with only painted-resource sampling and explicit register declarations changed.
Sources/hash guard and includes are flattened at build time into19845 ASCII
characters (+one terminating embeddedLF). The four painted pairs form two
four-layer Texture2DArrays. Each pixel selects x+2*z from absolute worldXZ and
uses the native18m border. SampleGrad takes derivatives of worldXZ/36900, not
the discontinuous layer UV. Texture t0..t4 and sampler s0/s2/s4 bindings were
verified from real compiled DXBC. Existing native terrain shader files are intact.

The source Native HLSL warnings are two existing vector truncations in the
interior-clipping statements; the resulting private PS compiles and reflects.

## Inputs and ownership

Eight exact installed PNGs: tiles2,2;3,2;2,3;3,3. Overlay1024RGBA and colour2048RGBA,
all have nonconstant alpha0..255 which is preserved. Preflight SHA/size on the
same bounded bytes before decode, then actual shape/depth/faces/format/byte checks
before allocating either GPU array. Only actual supported Ogre12/13 RGBA formats
are accepted; texture format must equal source format.

Encoded45799579B + decoded83886080B =129685659B simultaneously owned CPU inputs,
plus object/codec overhead. Encoded and decoded each have128MiB caps. The exact
fixture prevents arbitrary inputs; generalized modded PNG selection is future work.
Both CPU groups remain alive through both uploads, then are freed. GPU base
allocation80MiB. Initial reference has mip-count0 only: native minification with
mip chains remains unfinished. Unknown upload/ownership quarantines one bounded
set. Native Terrain/material are destroyed before painted arrays. Each native
upload sends its own credit followed by one owned setToLoaded completion.

One extra fixed program is cached via its own identity/source/manager guard,
without retaining a program smart pointer across requests. Other three fixed
programs and their cache policy are unchanged. No per-frame image IO, world
traversal, callbacks or active simulation-zone loads were added.

## Verification

- 76 production C++ bounds/shape tests, including all16primary cells, max edge,
  tiny central-seam coordinates and >32bit dimension truncation RED/GREEN cases.
- 64 actual HLSL coordinate cases on isolated D3D11 WARP; prior shader suites pass.
- Actual privatePS DXBC5texture/3sampler contract;8real PNG identities/alpha.
- Full native/source suites pass,154 native imports (17KenshiLib/137Ogre) resolved.
- Independent lifecycle/performance review found no blocker before the trial.

Installed DLL249344B SHA256
`eb159be9e12ac47ff35981239136c164dc995ea7ed9cb3d87d601d7fc2736589`.
GamePID31500, startUTC2026-11-01T14:00:12.4530600Z. User-loaded paused global map;
file requests1505/1506 without focus changes.

| Token | Controllerms | Texture bytes before/after | Material bytes before/after | RTV final refs |
|---|---:|---:|---:|---|
|1505|2227|2201047008 /2201047008|13132148 /13132148|six times0|
|1506|1994|2201047008 /2201047008|13132148 /13132148|six times0|

Scene0, one main workspace, one closed/joined native worker each. Four cached
programs after first request; high-program bytes966457 remain unchanged on second.
Native PNG decode/upload and absence of per-request texture/material accounting
drift are confirmed. This is a short controlled proof, not a universal leak/FPS
claim. Whole-process private samples include native game/plugins/driver work.
Idle private samples after first/second requests are8332255232 /8337141760B;
the latter was sampled several minutes later. These are not long-run leak proof.

Raw GBuffer and linear-clamped Sun BMP pixels match exactly between requests.
Exposed preview:351of262144 pixels differ, maximum channel1, mean absolute
channel difference0.0004475911458333333. All position/HDR readbacks finite.
First frame clipping52881/262144 in the deliberately un-tonemapped exposure
preview; preceding neutral-colour reference clipped168296. Different process/
lighting snapshots mean this is not a controlled brightness benchmark.

The image shows actual painted colour patterns. A single Okran biome still covers
the entire73728m cell, so its detailed textures are not geographically correct.
No final native display/tonemapping or seam-complete8K atlas quality is claimed.

Evidence: game `_codex_work/global-satellite-20261101/rtv-lifetime-20261101/`
`512-painted-reference-{first,second}/` plus `painted-reference-proof.json`.
Images, exact configs/logs, memory samples and direct PNG preview are retained.
Generic older capture.json scope labels are superseded by this report; the
controller now records which private material mode actually ran.
