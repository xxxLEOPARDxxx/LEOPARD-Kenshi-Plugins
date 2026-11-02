# Fixed-window native water HDR pipeline

This GPU helper is independently reviewed for the existing256-pixel GST2 windows.
Root owns the map caller, native snapshot lifetime, settings, asset integration
and live acceptance. It does not recapture terrain, create world objects, read
game state, call Ogre, retain native input pointers or run a worker.

## Caller interface

Include `WaterPipeline.h`. `AtlasWater::Pipeline::Init` takes device, then
pointer/byte-count pairs for HDR terrain, water and tone shaders, then the
current caller-owned GPU payload count. `Render` arguments are:

```cpp
HRESULT Render(const AtlasView::Cell& cell,
    const AtlasView::Uniforms& terrain,
    ID3D11ShaderResourceView* const terrainLight[3],
    ID3D11SamplerState* const terrainSampler[3],
    const AtlasWater::Uniforms& water,
    ID3D11ShaderResourceView* const waterInputs[5],
    ID3D11SamplerState* const waterSamplers[5],
    ID3D11Texture2D* ownedRgba8, unsigned __int64 callerOwnedBytes);
```

The five water views are actual colourmap, flowmap, normalmap, irradiance cube
and water specularity cube, with their corresponding native samplers. The
caller holds every reference and a coherent current native parameter snapshot
through the synchronous call. Water inputs may have one visible mip: the
unchanged native explicit3/7 cube LODs clamp to the actual available range.
No mips are synthesized. This supersedes the earlier standalone helper's
conservative4/8-minimum cube requirement.

For each window copy `terrain.tileBounds` exactly into `water.tileBounds` and
set `water.sourceWindow` to `{256,0,0,0}`. These are local256-plane inputs;
the absolute source-window position and selected source LOD are already encoded
in the terrain bounds/span. GSI windows are256-aligned, so resetting the local
pixel origin preserves the native2x2 derivative phase. Keep actual waterY,
mapBounds, material parameters, native time, lights and samplers; this helper
does not fill defaults. Stock world-aligned water is the audited scope.

`water.waterPlane[1]==0` returns `S_FALSE` before checking initialization,
device, resources or budgets and changes nothing. The caller uses the original
terrain renderer on OFF. Enabled errors return a failed HRESULT; caller keeps
terrain visible by using that same original renderer. Validation/alias failures
leave display contents and context unchanged. A removed device cannot promise
an intact final copy and requires the caller's normal device-failure handling.

Render/Clear/Init are serialized on the render thread. No concurrent use,
copying or reuse after world/device retirement is permitted. Clear before
reinitialization. Private COM ownership is bounded and independent of native
texture/resource accounting.

## Shader assets and default compatibility

Run bundled Python `generate_pipeline.py` or import and call `generate()` with
the atlas-water directory on `sys.path`. It verifies the pinned installed native
water/lighting source, then writes only `_generated/pipeline`:

- `WaterPipelineShaders.h`: `AtlasTerrainHdrShader`,
  `AtlasWaterMaterialShader`, `AtlasWaterToneShader` unsigned-byte arrays.
- `TerrainHdr.cso`, `Water.cso`, `Tone.cso`, `pipeline-shaders.json`.

Root's generator includes/copies these arrays into its build; use `sizeof` for
the byte lengths. Generated native-derived HLSL and DXBC stay ignored.

The existing `AtlasView::Relighter::Init` gains a trailing optional output
format; old calls still select exposed RGBA8. Only the optionalRGBA16F mode
accepts fixed256 side, and its destination must be DEFAULT, single mip/layer/
sample, CPU0/Misc0, exactSRV bind. The default nativeRGBA8 Misc1/SRV|RT case
remains accepted. Default terrain DXBC is still byte-identical
`e9d39a51c87a02b4dffe81b40c41a48e549dbf17fc0cf504d34d0a77b3e485b6`.
Native water material DXBC remains `9c5b4147...13b5d3`.

The HDR variant bypasses tone/gamma. Native distant water composes into the
private HDR work texture, then `ToneWater.hlsl` invokes existing NativeExposure
exactly once into RGBA8. It preserves explicit cache coverage alpha and never
clamps linear HDR before exposure. All three stages restore compute state and
predication; tone scopes read aliases to the verified-owned display output.
Writable input/output aliases and native input aliases to display are refused.

## Inclusive payload

| Private object | Bytes |
|---|---:|
| HDR terrain256 scratch +176B constants |524464|
| HDR work256 RGBA16F/SRV |524288|
| Water512 RGBA16F scratch +192B constants |2097344|
| Tone256 RGBA8 scratch +16B constants |262160|
| **Additional total** |**3408256**|

The current quality profile uses one authoritative192MiB cap shared by terrain,
water and pipeline. Init and every enabled Render require caller payload
plus3408256 <=201326592B. The144-window/default-renderer ledger189006000B
gives192414256B total, leaving8912336B. This replaces the original72-window/
96MiB profile; archived verification for that profile remains historical.
Include every simultaneous pending/retired/caller allocation;
do not add this pipeline's private total twice. No full HDR atlas or2048 HDR
intermediate exists. Shader/driver allocation metadata and borrowed native
assets are outside this payload count; it is not a residency measurement.

Render creates no resource or C++ heap storage and performs three bounded
256-window stages. Water retains its512 scratch but dispatches only256 here.
Native flow gradients add four flow samples per submerged pixel. GPU/CPU/FPS
and global animated-window update cadence require separate runtime measurement.

## Verification and limits

`run_pipeline_tests.ps1` compiles production assets and actual native oracles,
then runs19 policy and126 WARP checks plus10 functional compiled mutants.
Actual raw SunProbe HDR delta is0; the complete native terrain/water/one-exposure
reference differs by at most1UNORM level. References include physical one-mip
water cubes with normal native samplerMaxLOD, day/night,9m footprints, native
time changes, current waterY100/110/sentinel, HDR>1 and coverage/invalid height.
OFF/on/OFF returns the exact original terrain bytes.

The full context is snapshotted around nondefault native state, all-stage
display SRVs, real append counter7 and a completed false predicate configured
to suppress commands. Predicate equality suppresses commands per
[Microsoft's SetPredication contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-setpredication).
The missing-predicate mutant fails rendered contents, while the state-restoration
mutant fails the context snapshot. Repeated Init/Render/Clear and partial final
shader failure return observed WARP device references to baseline. Nonempty
dynamic class instances are preserved by production code but not instantiated.

Fresh old regressions also pass: terrain29policy/53reader/92WARP+6mutants,
water2source/11policy/163WARP+8mutants. Independent peer and root fresh pipeline
runs are CLEAR. Logs and exact hashes are recorded by `pipeline-verification.json`.

Native material fixture parity is not whole-game water parity. Near-water,
scum/rain/reflection, depth-biased coastline, rotated water meshes, buildings,
shadows and local lights remain outside this layer. Height-mip means limit
shore detail; native `depth_bias -1 -1` is not reproduced by the geometric
height mask. Runtime binding/lifecycle, paused/native-time behavior and visual
OFF/on/OFF remain root acceptance. Independently refreshed cached windows can
show different native-time epochs; these per-call tests do not establish
whole-viewport animated-water synchronization or seamless temporal boundaries.
