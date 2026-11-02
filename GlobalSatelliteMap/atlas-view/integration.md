# GST2 reader and terrain relighting helper, 2026-11-02

This is an offline-tested helper cutpoint. It is **not wired into GlobalSatelliteMap.cpp**, does not install anything and does not prove live whole-map terrain, buildings, local lights, shadows, FPS or leak-free gameplay.

`run_tests.ps1` builds with VC10/SDK7.1, compiles the shader against the installed game assets, and runs29 budget checks,53 file/crypto checks,81 real D3D11 WARP checks. All163 pass. Six deliberate mutants fail for their intended reason: lost compute-shader restoration, lost owned-output SRV restoration, omitted plane hash check, wrong dispatch-block origin, ignored fractional coverage, clipped HDR reflectance. Generated executables/fixtures/logs stay in `_generated/`.

## Root integration contract

1. Validate the ready manifest, all16 unique canonical filenames/tile coordinates, row/column convention, world/source/engine/save identity and header SHA strings. This helper deliberately does not invent those identities or parse arbitrary JSON. Root supplies a nonzero32-byte expected header digest and expected tile coordinates to each `ReadRequest`.
2. Reserve the cell in `Budget` **before allocating GPU resources**. `Set` refuses replacing a live slot; release the old caller-owned output and `Cell` first, then `Remove`. Count every live/current/pending cell, including outputs retained by MyGUI/Ogre. The budget is a caller-maintained ownership ledger, not a D3D driver memory measurement.
3. Run exactly one `ReadWorker(&request)` on the existing file-worker lifecycle. It reads only the selected mip's three planes, up to64MiB plus1024 header bytes, hashes in64KiB chunks and checks `cancel` between chunks. No game/UI/Ogre/D3D calls run on this worker. Cancel with `InterlockedExchange(&request.cancel,1)`; poll the thread/event and retain the request until it has signalled. Do not free a running request, spawn replacement jobs while one is still pending, or wait indefinitely on the render thread. A synchronous local file read can stall in the OS; this routine does not promise a wall-clock I/O deadline or forcibly terminate threads.
4. The reader requires flags exactly1, all48 base bits, formats10/28/41, exact1431307264-byte length, reserved zeros and all18 canonical contiguous descriptors/nonzero hashes. It verifies the pinned complete header and all selected plane hashes, denying write/delete sharing throughout that snapshot. Flags2/raw and6/interrupted conversion are refused. It does not read/hash the other15 plane payloads; ready-manifest creation verifies those offline. Failure/cancellation releases every partial plane buffer.
5. On the render thread, call `Cell::Upload` once, then `ReadRequest::Clear` to release CPU storage. Three immutable textures retain the half-float linear RGB/gloss, encoded world normal/coverage and float height. Root creates and retains ONE owned Ogre output per cell, validates the native texture getter/identity and provides a borrowed `ID3D11Texture2D*` throughout each call. Output must be DEFAULT, non-MSAA, one-mip/one-layer, UNORM **RGBA8 DXGI28**, with no CPU/misc flags. A common Ogre BGRA87 mapping is not accepted.
6. Initialize one `Relighter` with the compiled `AtlasRelight.hlsl` bytecode and same native device. A maximum supported cell side2048 allocates only a512-square scratch; it can be initialized once for all supported levels. No texture/buffer/shader allocations occur in `Render`. Root must account for any old renderer still alive before creating another.
7. Root supplies a value snapshot of the proven native lighting getters and borrowed SRVs/samplers from the native no-shadow lighting pass. Sources: ambientMap2D, irradianceCube with at least4 visible mips, specularityCube with at least8. Native sampler objects are passed explicitly; the helper does not guess source filtering or poll shared parameters. Snapshot reference camera and worldOffset/worldSize together, as `ReadLightingSnapshot` does. `Uniforms` has176 bytes, elevenfloat4 vectors, matching b0. `dispatchWindow` is helper-owned; initialize it to zero. Positive native exposure key and decoded luminance are required.
8. Cell bounds are absolute minX/minZ/spanX/spanZ, rows minZ-first; referenceSide.xyz is the paired native reference origin. Normal and view/sun directions are world-space; top-down view is surface-to-camera +Y. Retone only a visible map whose lighting/sources/identity changed. Root owns cadence, per-tick cell work and light-generation consistency; this helper creates no polling loop or per-frame work.
9. `Render` refuses a write-bound output (UAV/RTV/DSV), and refuses writable-bound native lighting resources before changing context state. Read-only SRV bindings of the verified-owned output are temporarily unbound across all six shader stages and restored to their original slots exactly. It also refuses wrong devices/formats/descriptors/mips/uniforms. Root must guarantee ownership before passing an output: this adapter is not permission to change arbitrary native targets. Actual WARP covers previous-frame output PS7, all six stages, CS/OM UAV refusal and RTV refusal.
10. Root logs every returned HRESULT/Win32 error, invalidates stale identities/jobs on world/save/load/reset, and keeps detailed success summaries behind the plugin Debug setting. The helper itself has no logger or INI reads. Borrowed native resources/samplers must remain valid for the full synchronous call; no borrowed pointer is stored across calls.

## Native shader scope

The shader includes installed `constants.hlsl`, `lightingFunctions.hlsl` and our existing `NativeExposure.hlsl` (which includes installed `hdrutils.hlsl`). It evaluates native `CalcPunctualLight`, `GetSpecularDominantDir`, `FresnelDiffuse`, `LazarovEnvironmentBRDF`, current sun/ambient/IBL scaling and native exposure. Ambient lookup uses explicit LOD0 because compute threads have no screen-space derivatives. The source sampler states remain caller-provided.

Native terrainfp4.hlsl:270-276 initializes the GBuffer, writes metalness0, gloss and world-normal RGB. Installed gbuffer.hlsl:69 initializes all channels0 and writeNormal:88 writes RGB only. Therefore this terrain base pass has zero translucency/emissive; cache coverage alpha is **not** a native material flag. Half-float reflectance above1 survives to lighting/exposure. Height is checked for validity and represents world Y; native ambient lookup uses XZ and this parallel-view BRDF has no height-dependent point-light attenuation. No fabricated height shadowing, main-light mutation, model shading, torch lighting or occlusion is included.

The test oracle invokes the existing `SunProbe.hlsl` against packed native GBuffer fixture inputs, with explicit ambient LOD0. Cached material fixture values are the correctly decoded half-float RGB of those packed bytes, not an assumed gray Y value. Actual readback matches every channel of all65536 fixture pixels with maximum delta1UNORM level (half-float material quantization), including spatially varying ambient input. Earlier constant-ambient fixture was exactly0LSB. An additional low-exposure1.125-vs1.0 regression proves HDR material is not clipped before lighting/exposure; its saturation mutant fails. This is fixture parity, not proof of current game's atlas capture/native world parity.

## Allocation, state and lifecycle evidence

| Configuration | Inputs + owned outputs | Shared scratch + constants | Total owned payload |
|---|---:|---:|---:|
|One2048 cell|83886080B|1048576B +176B|84934832B|
|All16 cells at512|83886080B|1048576B +176B|84934832B|
|Inclusive admission boundary|99614544B|1048576B +176B|100663296B =96MiB|

These are allocated resource payloads, not driver allocation granularity, queued-resource retirement or complete process VRAM measurements. Already-existing native ambient/IBL textures are borrowed and excluded; the helper does not duplicate them. CPU storage is one selected level, not a full1.43GB cell or22.9GB atlas. WARP tests allocate extra readback/native-fixture resources solely for verification; they are absent from the production helper.

A cell2048 uses16 bounded512 dispatch/copy blocks; a512 cell uses one; no CPU texture expansion or staging readback occurs in `Render`. Full-size boundary alpha markers prove global source coordinates and destination copy origins. All16 overview outputs are allocated/shaded in the real WARP test; the final cell/final pixel is read back. The allocation ledger rejects over-budget and live-slot replacement before reservation, including exact +1-byte boundary.

Private state writes are CS shader/class instances, b0, t0..5, s0..2, u0, predication and only those six-stage SRV slots that alias the verified-owned output. Every acquired COM reference is scoped/released. Restoring u0 uses NULL initial counts so existing append/counter values survive. Native predication is disabled around private dispatch **and copy**, then restored exactly. Writable alias checks prevent D3D automatic unbinding outside the saved slots. The test compares an exhaustive D3D11.0 context snapshot before/after: all six shader stages/classes,128 SRV/14 CB/16 sampler slots per stage, eight CS/OM UAV slots, OM RTV/DSV/blend/depth, IA, SO, raster/viewports/scissors and predication. Non-default VS/PS, native RTV/raster/viewport/scissor/topology and six-stage SRVs are installed. Append counter7 survives separately. Dynamic class-instance machinery is captured/restored by production code and the snapshot, but the current WARP fixture does not instantiate a nonempty dynamic-linkage class list.

Clear cells/outputs/renderer at generation retirement on the render thread after detaching owned widgets. No game singleton or native texture pointer escapes through this helper. COM Release can defer actual GPU retirement; static ownership and WARP readback are not a long-session leak/FPS measurement. Cancellation cleanup must retain worker ownership until completion, independently of widget/world destruction.

## Independent writer compatibility

Fresh writer101 VC10 checks and all18 atlas-cache Python suites also passed during this review. The audited decoder retains original cross-tile edges and rolling rows, marks all raw tiles6 before in-place conversion, preserves invalid coverage before replacing NaN data, and publishes the ready manifest only after complete headers. The renderer/reader matches its8/4/4-byte ABI. No full22.9GB production atlas was built by this subtask. Root integration, independent helper review, actual capture/readiness and live acceptance remain outstanding.

Device acquisition uses the already-verified native ambient SRV, not an unknown hardware-buffer getter:

```cpp
AtlasView::Com<ID3D11Device> device;
ambientSrv->GetDevice(&device.p); // GetDevice adds a reference; Com releases it.
// Keep native ambient/IBL SRVs and samplers alive through each synchronous call.
// Root validates the Ogre output getter, ownership and DXGI28 identity separately.
HRESULT hr = relighter.Init(device.p, bytecode, byteCount, 2048, budget.Bytes());
```

Ogre PF_A8B8G8R8 is the intended caller output mapping; the helper still validates its actual D3D descriptor as DXGI28. A512 scratch supports every selected cell side256..2048, so initializing maximum2048 once avoids subsequent scratch reallocations. At most16 block submissions occur in one cell Render. The96MiB ledger includes176-byte private GPU constants; texture allocator overhead/driver residency remains outside a payload count.
