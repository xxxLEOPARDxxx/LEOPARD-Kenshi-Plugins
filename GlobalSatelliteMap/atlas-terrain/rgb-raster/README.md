# Direct RGB fixed-terrain raster

`RgbTerrainRaster.h` is an owned C++03/D3D11.0 primitive. It submits the frozen
native-derived RGB terrain pixel shader into 256-square RGBA16F material,
RGBA8 native world-normal/alpha and R32F native radial-depth planes, with private
D32 depth. There is no packed YCoCg writer/decoder or pixel-neighbor dependency.
This component is separate from Main, the probe, the viewer and atlas files.

The original geography calculation is the already reviewed `raw-rgb` shader:
only its two final active GBuffer writer changes are present. The shader's
RGB/gloss, normal and radial math are unchanged. `Init` checks exact VS/PS byte
counts and SHA256; bytecode is borrowed during initialization, not copied into
this source tree. VS pin is `dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238`
(4440 bytes), PS pin is `1dd118fe042b8f7297fe7ce5ab9d20c5392cce85acdffc8b33bd8ac006b7af2b`
(36968 bytes). VS448/PS3856 constant layout and native 40-byte vertex declaration
remain unchanged.

## Inputs and lifetime

`Init(device, shaders, samples, grid, otherGpuBytes)` accepts 1..16 globally
aligned 65-square patches, with a nonzero caller epoch and complete two-sample
normal halo. The existing fixed geometry builder creates immutable buffers.
The supplied source and shader spans are borrowed only for Init. No CPU mesh,
source pointer or callback is retained. Source identity, correct raw decoding,
the epoch and actual resource winners remain the outer adapter's responsibility.

`Render(enabled, sources, frame, otherGpuBytes)` requires externally serialized
owning-render-thread access and a completed unchanged `CompactMaterial::Inputs`
on the same device. OFF returns S_FALSE before any query. Enabled Render has no
heap allocations, resource creation, files, native/Ogre callbacks or game queries.

This is the fixed two-SID cell10 domain, not an arbitrary global material adapter.
Frame supplies absolute X/Z center, spacing [1,9] m, camera Y, far clip, wetness
[0,1], water height [-10000,10000], and brightness multiplier [0,8] of the two
frozen selected rows. All values must be finite. Camera must exceed the highest
owned vertex by over1m and be at most20000m. Far clip is positive, at most1000000m,
and greater than the conservative radial extent down to height0. The complete
256-square footprint must fit the supplied patches and [-18432,-9216] on X/Z.
No height detail finer than the actual18m source grid is invented. Identity world,
native morph0, cliff phase, UVs, normals and fifteen distortion slots are retained.
Arbitrary live parameters, other cells, input material finiteness over every
possible world height/weather combination, and current native source admission
are not established by these fixture controls.

`Texture(0..2)` borrows allocated resources for diagnostics/alias checks; those
resources are uninitialized until a healthy complete Render. `HasFrame()` and
`Output(0..2)` publish only a submitted healthy complete frame. Preflight refusal
and OFF retain the previous frame. Once drawing starts, an exception or failing
post-draw device-health status withdraws all output SRVs until a healthy retry.
This is synchronous command-submission validity, not GPU completion or proof of
arbitrary input finiteness. C++ exception unwinding is tested; SEH/driver crashes
are not recoverable guarantees. Clear releases owner references, resets charge
and publication, and allows reInit. Borrowed pointers expire at Clear. Retained
caller COM references, bound views, GPU work/fences and their accounting remain
the caller's responsibility.

## State and ownership

The existing unchanged `TerrainRaw::Targets` owns the four targets. The parent
packed raster's public Grid, validation/constants and PixelState guard are reused;
GraphicsState and SavedOutputReads are reused unchanged. Parent Raster's buffer,
packing and draw internals are private, so this primitive supplies those small
pieces locally rather than editing any frozen parent or duplicating its target
owner. There is no alternate shader formula or copied native source snapshot.

Before mutation, writable source/target aliases in CS/OM UAVs, RTVs and DSVs
and any stream output refuse. Source views must belong to the original device.
Read aliases of outputs are unbound/restored across all six shader stages.
Graphics shaders/classes, IA0..15 (higher slots untouched), graphics b0, all
modified pixel resources/samplers, raster/viewports/scissors, OM/hidden append
counters, depth/blend and predicate pointer/value are restored in reverse scope
order. Predication is disabled during both clears, constant uploads and draws.
It is restored exactly under both comparison senses, after success, reported
device failure and C++ draw exceptions. Output texture contents cannot be rolled
back after a partial draw; publication is withdrawn instead.

## Budget and performance

The unchanged control cap is90MiB, viewer cap192MiB. `otherGpuBytes` must include
at least the completed compact source's83,885,956B and every other retained or
pending GPU allocation outside this raster. Both Init and Render enforce caps.
The nested target initialization receives other bytes plus geometry/CB bytes,
so its own admission cannot omit the source or double-count targets. Fits bounds
the addition before it is performed. No cap is raised.

| Payload | Bytes |
| --- | ---: |
| Three outputs plus D32 | 1,310,720 |
| Increase over packed targets | 262,144 |
| Shared R16 strip index buffer | 17,144 |
| VS/PS constant buffers | 4,304 |
| Each immutable patch | 169,000 |
| Four-patch raster total | 2,008,168 |
| Sixteen-patch raster total | 4,036,168 |
| Source plus four-patch raster | 85,894,124 |
| Source plus sixteen-patch raster | 87,922,124 |

Init's largest local mesh is169,000B plus17,144B indices, with builder scratch
53,868B nested during construction. Render uses bounded stack CB/state snapshots,
two UpdateSubresource calls, one to sixteen draws, and bounded D3D binding scans.
At four patches it submits32,768 real terrain triangles. SavedOutputReads scans
3x6x128 SRV slots; preflight writable-alias checks scan fixed-size binding tables.
These checks are intentionally per submitted window, not a per-frame world scan.
There is no cache, growing queue or retained CPU buffer. COM query references and
repeated Init/Clear are checked. Payload figures exclude driver/runtime overhead,
external retained views and in-flight allocations; this is not an FPS or long-run
game-memory measurement.

## Verification

Prerequisites are the existing frozen fixed-geometry, compact-material, raster
and raw-rgb headers/artifacts, plus actual local DDS source files named by the
compact pins. This runner reads and authenticates dependencies; it never runs
their generators or edits them. Private expanded native HLSL remains ignored in
its original component. Nothing is copied into game/probe/build directories.

Run using bundled Python (VC10/SDK7.1 and headless WARP are selected by runner):

```
C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py
```

The actual component is compared byte-for-byte to all three planes in the nine
frozen raw controls:3m/9m/1m, even64px and odd1px shifts, wet, underwater and HDR.
Foreign-state tests alternate distinct frames under both senses of an actual
false-result predicate, preventing stale-output false positives. Tests cover
budget+1, wrong shader, grid/halo/epoch/input bounds, writable aliases, stream
output, OFF, references, Clear and retry. All29 allocation/device-health Init
cutpoints are faulted in private header copies. Post-draw failure and C++ draw
exception restore exact foreign context and withdraw publication. Five narrow
compiled functional mutants and a cleanup-loss mutant must fail actual asserts;
compilation failure is never counted as a successful negative control.

The reference's native prewrite itself has same-world camera drift:
0.000869780779 for even64px and0.00122564659 for odd1px; normal drift is at most
one byte in these controls. This raster reproduces those controls exactly,
including FP16 storage and HDR8.1171875. It does not claim perfect cross-camera
float equality or apply a packed checkerboard correction. Fullworld source
streaming, atlas writing, lighting, water/models and in-game acceptance are separate.
