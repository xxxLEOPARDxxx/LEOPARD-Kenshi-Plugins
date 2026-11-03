# Loaded-town HDR composition

This helper does not enumerate or admit native geometry. It accepts the current
verified `LoadedInstances::Raster::InitRawRgb` output; the native Snapshot,
Packet and Raster admission contract is unchanged. No world, file, Ogre, menu,
worker or game mutation is performed here.

## API and ownership

`TownCompose::Layer::Init(device, bytecode, bytes, otherGpuBytes)` allocates four
fixed 256x256 textures and a 224-byte constant buffer. `PrivateBytes=1,573,088`.
`otherGpuBytes` includes every coexisting producer/cache/HDR/layer/output/retired
allocation, excluding this layer's own `PrivateBytes` (charged exactly once).
The same measured ledger is checked before each Render; exact-cap admission
passes, cap+1 fails before dispatch. No render-time allocation or CPU readback.

`Render(enabled, uniforms, sources[10], samplers[3], otherGpuBytes)` runs on the
thread that initialized this layer, on the device's immediate context. OFF
returns S_FALSE before accessing the inputs/context. Inputs are synchronously
borrowed, no persistent source COM references are stored. `Texture(i)` and
`Output(i)` are borrowed handles valid until Clear/destruction, for indices0..3.
Init failure clears all partially owned resources and permits retry.

|Input|Format/role|
|---|---|
|0|RGBA16F: native raw linear RGB, gloss in alpha; verified black metal|
|1|RGBA8_UNORM: native world normal, native translucency/emissive in alpha|
|2|R32F: native radial distance divided by farClip (may exceed1 at corners)|
|3|RGBA16F: previous accepted linear RGB/gloss|
|4|RGBA8_UNORM: previous accepted normal/coverage|
|5|R32F: previous accepted cache-worldY|
|6|RGBA16F: pristine, already-lit HDR before this composition|
|7|Current native 2D ambient map, sampler0|
|8|Current native irradiance cube (LOD3), sampler1|
|9|Current native specularity cube (LOD0..7), sampler2|

All first7 views must expose one mip, one array slice, exact256 square shape.
Sources7..9 use the existing Relighter LightView shape/format/device contract.
No source may alias any owned output. Write-bound inputs/outputs are refused
before any state/output mutation. Existing read bindings of owned outputs are
temporarily unbound/restored; full CS0..9 SRVs, CB0, samplers0..2, UAV0..3,
shader/class instances and predication are restored without resetting counters.

Outputs0..2 have the formats/meaning of inputs3..5. Output3 is straight linear
HDR16F. Raw normal alpha is decoded for native lighting BEFORE accepted
normal/coverage alpha1 is emitted; this is not an opacity blend or coverage
test. Rejected/missing/uncertain geometry preserves all accepted input planes
and pristine HDR. Render always starts from those inputs, never accumulates
its own previous output. Downstream water/local-light/exposure passes consume
the accepted planes/HDR under their existing contracts, not raw model alpha.

## Coordinates, lighting and external freshness

`Uniforms.light` is the existing 176-byte AtlasView native light block.
`light.tileBounds` is absolute/cache minXZ and positive spans; `referenceSide`
is the CURRENT native lighting eyeXYZ plus side256. It is not the private
raster camera. `cameraFar` is the private top-down raw-scene eyeXYZ/farClip.
`sceneToCache.xyz` is the verified additive raw-scene -> cache/world translation.
The camera XZ must equal `(tileMinXZ - sceneToCacheXZ) + spanXZ/2`; viewDirection
must be top-down surface-to-eye (+Y). `limits.x` is numerical reconstruction
allowance,16*FLT_EPSILON (accepted range up to0.001).

At every covered pixel, radial geometry reconstructs the lower positive root
below the top-down eye and adds sceneToCacheY. The propagated error interval
must place it strictly above the already accepted height. Equal/uncertain
surfaces keep the previous value; missing coverage or invalid previous height
can accept a valid model. Horizon/invalid/NaN roots are not fabricated asY0.

TownLight uses the existing native SunProbe sunlight/environment closure and
the installed, hash-pinned constants.hlsl + lightingFunctions.hlsl, preserving
translucency, emissive, gloss and linear HDR. Ambient uses spatial worldUV
with explicit LOD0; irradiance/specularity use the unchanged native LOD3/7
formula. There is no invented Lambert tint, tone mapping or HDR saturation.
The generator records native include hashes and pins the reference SunProbe.

The caller MUST validate world/source/device/footprint/light epochs before
dispatch, pin current SRVs/samplers, and retain native sources plus this layer
until its own real GPU-completion fence. This helper's finite/layout/thread
checks are not native authority. Do not keep the lit output through a changed
light epoch without recomposition; invalidate/regenerate matching footprint
geometry when Snapshot contents or origin change. No Main integration is
included in this isolated helper.

## Focused evidence

`run_town.py` generates the explicit shader, compiles the real helper on VC10
/Od and /O2 and runs actual D3D11 WARP.46 controls per build cover independent
native SunProbe lighting comparisons (8 lighting scenarios, full planes),
real frozen Dual43/Alpha48 raw-raster triangles, HDR/backlit translucency,
nonzero origin, radial>1, accepted-height occlusion, holes/coplanar/NaN roots,
format/cube/foreign device/thread/alias refusals, exact-cap/+1, complete context
state after32 dispatches, hidden append counter, input/output COM ownership,
failed shader initialization/retry and Clear.7 functional mutants are killed:
translucency, occlusion, origin, environment, HDR clamp, budget and state restore.
The initial stub failed Init as expected before implementation. The original
test's normal word was corrected from -Z to -Y to match its +Y sunlight and
analytic backlight expectation. GPU/readback tests do not claim live city
admission, map integration, native shadow parity, measured FPS or every OOM.
