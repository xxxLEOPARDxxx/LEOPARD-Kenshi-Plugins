# Optional native distant-water HDR layer

This isolated helper adapts the installed `water.hlsl` **DISTANT + NO_REFLECTION** material to a bounded atlas compute pass. It has no game/Ogre calls, zone activation, worker, polling, model clones, file cache or installation hook. It is not yet connected to the map viewer. The terrain viewer can ship independently.

## Integration contract

`Layer::Init(device, bytecode, bytecodeBytes, callerOwnedBytes)` creates one compute shader, a 192-byte constant buffer and one 512x512 RGBA16F UAV scratch texture. `Render(values, sources, samplers, ownedHdr, callerOwnedBytes)` composites one <=512 block in place, before exposure/tone mapping. Success is S_OK; explicit OFF is S_FALSE and reads no resources or device/context. Failure leaves the caller responsible for displaying terrain alone. Do not silently enable water when a current native snapshot is unavailable.

The frozen `atlas-view` renderer currently writes exposed RGBA8. Integration therefore needs a <=512 straight **LINEAR_HDR RGBA16F cutpoint** before its tone operation; feeding that RGBA8 output to this helper is refused. Do not allocate a full 2048 RGBA16F intermediate. The destination is caller owned, single mip/layer/sample, DEFAULT usage, no CPU access/misc flags. It may be the same texture as source0. None of sources1..7 may alias it; all writable native/input/output bindings cause refusal before state mutation.

`callerOwnedBytes` must count every simultaneously resident owned GPU payload: selected material/coverage/height, display outputs, HDR block, terrain scratch/constants, pending/retired resources and other renderer buffers. Init and every enabled Render require that count plus **2,097,344 bytes** (512x512x8 + 192) <= **100,663,296 bytes (96 MiB)**. Borrowed native textures/samplers are not copied. This is a payload ledger, not a measurement of driver residency or the entire process. Reinitialization requires Clear; loss/reset or world/device switch must Clear and recreate on the render thread. Render and Clear must be serialized; no background calls.

All pointers are borrowed only for the duration of Render. The caller must hold their COM references and a coherent native material/light/time/water-height snapshot for that call. Read native parameters on the existing render thread; do not change global native water parameters or invoke scene loading. Selected cell row0 is absolute minZ and column0 minX. Origins must be integer/even to preserve native pixel-quad derivatives. All selected sides 256/512/1024/2048 and block sizes 1..512 are validated.

| Binding | Input |
|---|---|
| t0 | <=512 RGBA16F straight unexposed terrain HDR block |
| t1 | Full selected-cell RGBA8 world-normal/coverage; A is explicit validity |
| t2 | Full selected-cell R32_FLOAT **absolute** terrain height |
| t3/t4/t5 | Actual `waterDistant` colourmap / flowmap / normalmap SRVs |
| t6 | Actual water irradiance cube, >=4 visible mips |
| t7 | Actual water specularity cube (`mp_specuwatery`), >=8 visible mips |
| s0..s4 | Native colour / flow / normal / irradiance / water-specularity samplers |
| u0 | Helper-owned 512 RGBA16F scratch |

The CB is b0, 12 consecutive float4 rows, 192 bytes, shared by `WaterUniforms.hlsl` and `Uniforms`:

| Byte | Row |
|---|---|
| 0 | tileBounds: absolute minX,minZ,spanX,spanZ |
| 16 | dispatchWindow: helper writes width,height,0,0 |
| 32 | sourceWindow: selected side,block originX,originZ,0 |
| 48 | waterPlane: **current absolute waterY**, explicit enable0/1,0,0 |
| 64 | nativeMapBounds: actual material minX,minZ,maxX,maxZ |
| 80 | normalScale: actual water VP `scale` |
| 96 | waterSunDirection: native water-plane-local sun XYZ |
| 112 | waterSunColour: native sunColour |
| 128 | waterEnvColour: native envColour, including W |
| 144 | materialParams: native invStrength,distortion.x,gloss,glow |
| 160 | waterTime: native gameTime,0,0,0 |
| 176 | parallelView: world/plane surface-to-map-camera direction, XYZ |

The audited stock global water plane has world-aligned axes; rotated arbitrary mod water meshes are outside this contract. Do not substitute terrain's specular cube or a remembered waterY=100. The stock initialization uses 100, while native setters can change the shared water height. `waterHeightRel` is camera-origin relative and may be a -1e8 sentinel; it is not the absolute snapshot required here.

The mask is coverage>0, finite height and height<=current waterY: a geometric approximation of the stock global planar surface. Native `waterDistant` uses raster `depth_bias -1 -1`; the cached-height mask and geometric-depth fixture do not reproduce that bias or claim exact native coastline pixels. Native distant water is opaque. Its RGB replaces terrain RGB; **base coverage alpha is retained**. Invalid cached samples remain unchanged. Fractional coverage is validity rather than opacity. Semantic height mips contain means, so the selected raster cannot promise fine coastline geometry. Runtime cell bilinear seams/gutters remain a separate viewer concern.

## Native source boundary

Generation pins the installed water and lighting source SHA256, preprocesses the real DISTANT/NO_REFLECTION path and extracts its lighting, sampleNormal and waterFP bodies. No near-water depth, rain, scum, biome blending or perspective reflection is implemented. Generated native-derived HLSL stays ignored in `_generated`; a local installation is required to regenerate it.

`audit_native.py` separately reads the SHA-pinned **RE_Kenshi/Kenshi_x64.exe** (504b362c...) offline and records exact instruction bytes/import references in `_generated/native-water.json`: derived-node height getter9d5850, parent/shared-parameter setter9d5a80, initialization's thunk to that setter with100, constructor map bounds +/-147456 and global plane500000, and zone parent/position code. It uses the local pefile/Capstone audit dependencies. This static evidence does not obtain a current runtime height, prove current SRV/sampler bindings, or support arbitrary mod water geometry; integration must supply the actual current snapshot.

Two native view dependencies are explicitly removed: the distance>=4000 selector (the map intentionally selects the far material), and NO_REFLECTION's `saturate(UNORM_reflection.rrr+1)` which is exactly one. No `mainrt_reflection` is borrowed or sampled. The reference fixture compiles the **unmodified actual native pixel shader** and supplies a private black UNORM-equivalent reflection value, not the game's main-camera reflection. A parallel view direction replaces the perspective vector; the reference uses the same normalized direction with distance5000 solely to pass native selection.

Colour/flow/normal sampling uses explicit gradients. Normal gradients include actual flow displacement differences from globally phased 2x2 pixel quads for all three native temporal samples. This costs four additional flow samples per submerged pixel; cube LODs remain native 3 and (1-gloss)*7. Shader work is bounded by one block dispatch; there are no per-Render C++ allocations/resource creations, world scans, growing caches or resource retention. Driver-internal allocation/FPS cannot be established by this static bound.

Native flow normalization divides by zero at exactly zero velocity. The adapter defines stationary direction=(0,0); nonzero flow math stays native. Tests compare that extension with the actual native shader's small-velocity limit, not with undefined native NaNs. Nonfinite material output falls back to unchanged terrain HDR.

## Verification

Run `./run_tests.ps1`. It needs VC10 x64, Windows SDK7.1, FXC10.0.26100.0, bundled Python and the installed native shader files. All generated executables, DXBC, copied negative controls and native-derived source remain in ignored `_generated`.

Current results: 2 source tests; 11 C++ policy checks; **163 real D3D11 WARP checks**; 8 independently compiled functional mutants rejected. Native comparisons cover day/night, moving flow/time, two zoom footprints, local offsets, both sides of primary-cell borders, current height100/110 and -1e8, hardware-depth shoreline masking, stationary extension, IBL contribution, HDR>1, coverage and ON/OFF/ON. Maximum observed absolute HDR difference is **0.0009765625**, acceptance<=.001 after RGBA16F quantization. This is selected-material fixture parity, not native whole-world water/render parity.

State verification snapshots the complete D3D11.0 context, nondefault shader/state/CB/SRVs/samplers/predicate, owned-output read bindings in all six stages and a real append counter7. Native CS, bindings and predication are restored; the append counter remains7. Output/native writable aliases and bad formats/coordinates/height/budgets are refused. Nonempty dynamic class-linkage instances are preserved by API code but not instantiated by these fixtures.

Negative controls remove CS restoration, reset the append counter, hardcode initial water height, restart local coordinates, clamp HDR, remove the stationary guard, force normal mip0 and remove native reflection energy. Each compiles and fails a functional GPU assertion. During fixture development VC10 /O2 miscompiled an inlined large-struct-return initializer; the fixture now uses a noninlined output-reference initializer and explicitly checks complete native lighting rows and nonzero IBL contribution.

No game calls, full-atlas water rendering, runtime native snapshot/lifetime integration, graphics-device recovery trial, CPU/FPS/memory growth measurement or player visual acceptance has been performed for this helper. Initial viewer integration remains separate.
