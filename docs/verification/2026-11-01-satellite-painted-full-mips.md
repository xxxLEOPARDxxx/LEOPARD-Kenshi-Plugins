# Private painted full-mip runtime proof, 2026-11-01

This is a one-biome, four-pair, 512x512 terrain reference, not a finished global
map. BestMinimap and the ordinary global-map image remain unchanged. The user
prepared SAT-render-proof-20261101 paused with the map open; captures were issued
through the existing file request without changing focus or controlling windows.

## Installed candidate

Source0b14a88; DLL253440 bytes, SHA256
`8adf86b10688163886023a9ca509c672f3119bd1ef8d001f172d03db2961d241`.
156 game imports resolve. Independent review and the complete probe build pass.
The owned painted overlay/colour arrays now have11/12 total mip levels. Their
explicit texel allocation is111848096 bytes, below the unchanged128MiB cap;
native manager getSize counts base levels and is not a VRAM measurement.

The shared GPU helper passed135194 WARP checks, including both channel orders,
four distinct layers and actual lower-mip readback. An omitted-generation mutant
fails. The36 production completion/budget tests and omitted-notification mutant
check single notification/Loaded completion and retained uncertainty on failure.

## Two real requests

Process13024, startUTC2026-11-01T19:18:34.5270794Z. Both use the same owned cell,
inputs and paused scene; tokens1507/1508. Each log confirms11/12 levels, DXGI87,
four slices, one creator notification and Loaded state for each owned array.

| Token | Controller elapsed | Texture bytes before=after | Material bytes before=after |
| --- | --- | --- | --- |
|1507|2646ms|2258431363|16142087|
|1508|1551ms|2258431363|16142087|

Both requests kernel-join/close their one native worker and release all six
owned native RTT creation references with remaining count zero. Scene count
returns to0 and workspace count remains1. Four explicitly retained private
program cache entries remain; they are not reported as destroyed resources.

Both512x512 decoded GBuffer and linear-clamped Sun images match pixel-for-pixel.
The exposed images differ at574 pixels, with maximum channel delta1 and mean
channel delta0.0007298787434895834. Position and exposure diagnostics report no
nonfinite pixels. These are controlled repeatability checks, not proof of correct
geographical biome mixing, final tone mapping or complete terrain/model coverage.

Immediate process-private bytes include transient native/driver allocations:
first8219553792→8481239040; second8233189376→8710946816. A later idle reading was
8339615744. Counter balance and these short samples do not prove zero retained
process/GPU memory or establish an FPS result. No global cache eviction was used.

## Evidence and remaining acceptance

Artifacts under game/_codex_work/global-satellite-20261101/rtv-lifetime-20261101:
`512-painted-full-mip-first/`, `512-painted-full-mip-second/`,
`painted-full-mip-proof.json` and `512-painted-full-mip-preview.png`.
Each request folder records original BMP hashes, log, configuration and memory
samples. The controller derives its full-mip scope from the actual success log.

Native ordinary-PNG versus owned-array byte equivalence is still pending. The
separate offline MipComparison helper, committed5649b14, is not wired into the
probe and does not establish that native comparison. Native load/shutdown
cancellation and resource ownership must be connected before invoking it.
Geographical biome blending, geometry boundaries, water, models/LODs, shadows,
dynamic local lights and the optional global viewer remain unfinished.
