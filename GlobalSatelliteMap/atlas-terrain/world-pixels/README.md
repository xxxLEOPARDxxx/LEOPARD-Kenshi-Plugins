# Accepted terrain pixels and fixed-source world height

`WorldPixels.h` is a serialized CPU-only acceptance/conversion primitive. It consumes the three tightly packed WorldStream/WorldRaster raw planes and publishes a typed owned window only after every pixel passes. It makes no file, engine, native metadata, WIC, D3D, INI or UI call. Run it on bounded worker work rather than adding this pixel scan to render/UI callbacks.

Input is FP16 **full RGB plus native gloss**, normalRGBA8, and radialR32. Material alpha is gloss, not opacity or a coverage mask. Normal alpha is the native literal zero. The helper preserves every finite material bit, including negative/HDR RGB, signed zero, subnormals and finite gloss; it copies normals exactly. There is no clamp, sharpening, color-space conversion, relighting or YCoCg decoding.

The output planes are FP16 RGB/gloss, normalRGBA8 and **world-height R32**. Height comes exclusively from the authenticated source samples, never by inverting radial depth. Output1/3/9m pixel sampling still samples the native fixed18m piecewise planar geometry; it invents no finer terrain geometry.

## Contract

`Raw` carries copied `SourceReceipts::Scope` and `WorldRaster::Frame` labels plus pointer/exact byte count for each raw plane. A checked producer/consumer must establish those labels from the actual current job. They are not cryptographic proof of image origin or current resource winners.

`Accepted::Build(enabled, raw, now, currentFrame, heightWindow, cancelEvent, otherCpu)`:

1. OFF clears previous output before consulting inputs/source. Every new attempt withdraws the old accepted data. Inputs overlapping the destination's previous slab are refused before freeing it; this avoids reading a dangling prior result.
2. Full64-bit world/source/job scope, source profile, material/frame generations and exact Frame bits must match. Only the renderer's256/512, spacing1..9, bounded finite camera/weather, one selected region/cell and far-plane domain are accepted. Plane sizes must be exact8/4/4 bytes per pixel, non-overlapping and free of pointer extent overflow. Pointers must refer to valid immutable readable storage for the call; this is not a Windows SEH memory-probing API.
3. The actual `WorldHeight::Window` must be ready, have the exact full64-bit source epoch and canonical fullmap SHA, a physical file ID, and cover all needed triangle vertices. No caller-supplied verified Boolean or untyped sample array substitutes for it. The existing WorldHeight source authenticates the full TIFF with bounded reads; this helper does not reread or allocate the TIFF. Copied windows remain valid snapshots after the source handle closes, as that source API permits.
4. All four FP16 material channels must be finite. Normal alpha must be zero, the clear RGB000 code is refused, and a decoded near-zero normal (integer signed-code squared length≤16) is refused. Native normals are not required to be unit length: actual fixtures reach decoded length²1.537. These are stored-normal usability/coverage checks, not proof of the shader's original pre-UNORM vector or its material direction.
5. Every radial value must be finite and strictly between0 and1. The pixel must be below the top-down camera by more than1m. Its radial camera distance must agree with the authenticated CPU triangle position within the fixed tolerance below. Cleared/clipped or inconsistent data refuses the whole window.
6. Cancellation is checked before allocation, before **every output row**, and before publication. Invalid/failed waits cancel conservatively. Any allocation, stale, malformed, pixel or cancellation failure frees the entire slab and leaves no accepted data. Partial internal rows are never available through the API.

`Acquire(enabled, now, currentFrame, currentHeightWindow, View&)` zeroes the View on refusal and independently rechecks enabled/full scope/exact frame, height coverage and full source file/epoch/hash identity. Call it with the freshly checked context when consuming a completed worker result; a successful Build against an earlier copied context does not establish that the world stayed current afterward.

The View borrows only the accepted owner's slab; no raw input or Window pointer is retained. It remains valid only while this owner remains unchanged/alive and the acquisition context remains current. Consume/copy or hold the owner through downstream upload/writing. Build, Acquire, Clear and destruction may not race. The helper creates no worker or queue.

## Exact height and radial policy

Pixel column0/row0 corresponds to minimum world X/Z, matching the renderer's top-down projection. Centers are `center + (pixel + .5 - side/2) * spacing`. Source coordinates are `(world +147456)/18`. Each global source quad uses the native anti-diagonal (top-right to bottom-left). Interpolate the lower triangle for `fractionX+fractionZ≤1`, otherwise the upper triangle. Vertex height uses the unchanged native float expression `ushort*(9800.0f/65535.0f)`; interpolation is double and final storage is float32.

Radial is checked as `abs(radial*farClip - length(pixelWorld-camera)) ≤0.05m`. This5cm world-distance margin is a fixed **acceptance policy**, not a claim of universal hardware/terrain parity or height precision recovered from depth. Fourteen actual256/512 WARP windows at1/3/9m, including192m/3m phase shifts, dry/wet/underwater/HDR variants, show maximum0.002003347100981m (about2.01mm). The margin exceeds that measured maximum by about25×. Unsupported geometry/hardware/input combinations outside it refuse; the tolerance is never raised automatically. The CPU height is unaffected by radial error. Near-horizontal camera rays do not turn this radial sanity bound into a bound on hypothetical inverted height error.

Tests independently reconstruct height from actual emitted40B geometry packets and strip indices, choose the containing indexed triangle by barycentrics, and compare every output height within1mm. They do not reuse `WorldPixels::Triangle` for the oracle. This proves the tested fixed-source interpolation and orientation; it is not arbitrary active-world or adjacent material-region acceptance.

## Ownership and budget

There is one exact `float[4*side*side]` allocation:16 bytes per pixel, at most4MiB for512. Its first8 bytes/pixel store material bits, next4 normal bytes, and last float/pixel world height. `RequiredBytes(side)` and `OwnedBytes()` include the owner object plus this payload. The destination is cleared before replacement, so old/new slabs do not coexist. No per-pixel allocation, retained input, growing cache or background lifetime is added.

`otherCpu` excludes this destination but must include all caller-owned raw storage, retained height/source window, pending readbacks/results and other producer payloads. The sum plus RequiredBytes must fit the existing64MiB payload cap before allocation. Multiple output owners must count each other. This is an explicit owned-payload ledger, not a claim about CRT/OS/driver allocator overhead. GPU192MiB accounting is unchanged and remains with the producer/uploader.

## Reproduction and limits

Run bundled Python `run_tests.py` with VC10 x64/SDK7.1. It verifies fixture pins, runs actual production controls under `/O2` and `/Od`, allocation/cancellation controls and compiled mutants. If this folder's fixtures are missing, `prepare_fixtures.py` reads the existing frozen helper fixtures/assets and captures five new512 WARP cases into this folder only; it never invokes or writes a borrowed generator. The nine256 inputs are frozen raw-rgb controls. Existing frozen world-raster height, shaders and world-bank RGBA inputs must already be prepared. `measure_radial.py` uses bundled NumPy offline to reproduce the14-case error table.

The317 production controls validate1,900,544 real pixels across14 windows, all output bytes/heights, malformed/stale/source identity/normal/alpha/depth/alias/CPU boundaries and clean reuse. Ten representative real-event cancellation boundaries plus two allocation failures exercise preallocation, first/middle/last row and final publication for both sizes. Fourteen functional and two cancellation mutants must fail runtime assertions, never merely fail compilation or crash.

The fixture/helper is pinned **base-snapshot terrain**. Current native resource winners, material provenance, raw image origin, global map scheduling, live frame/thread adapters, pixel lighting, GST2 tile/index/mip readiness and atomic atlas publication remain separate. The class does not certify SourceReceipts or GPU upload success. No game FPS, universal finite shader promise or long-run leak claim follows from these tests. There are no changes to WorldStream, renderer, game, INI or installed data.
