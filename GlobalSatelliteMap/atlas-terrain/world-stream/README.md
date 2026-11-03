# One-window staged source/render controller — development

`WorldStream.h` joins a Main-only metadata gate and D3D owner to one persistent
worker. This is a bounded **one-window diagnostic integration core**, not the
complete atlas/fine-cache scheduler or an installed game feature.

Main supplies a typed `Job`, a checked current world/source/job scope and captured
tokens for all15 source roles of each real SID. Order is overlay PNG, painted
colour PNG, blend PNG, six diffuse DDS, six normal DDS. Canonical asset IDs and exact native-style logical group/name pairs,
full encoded pins, family/layer/slot,64-bit source/world/job and shader pins are
checked. Shader bytes are copied before Submit returns. Material/frame/grid are
copied. A live-game adapter must establish their actual provenance and current
state; a supplied scope is not independently proof of the active world.

The worker authenticates and retains the exact TIFF handle across successful
same-epoch/path jobs, reads only the bounded height window, and opens one copied
source request at a time. Main accepts that handle lease; the worker then decodes
one canonical PNG if needed. Main uploads it and the worker releases the lease.
All15 sealed receipts are refreshed under a bank transaction before a SID can be
completed/drawn. Worker state reaches neither Provider/MainToken nor game/D3D
objects. One process-local worker slot prevents multiple owners or repeated
quarantines from creating unbounded workers. For NativeMetadata's16-role limit,
an adapter needs per-SID metadata providers or an equivalent guarded wrapper;
the supplied tests use a clearly test-owned metadata provider, not live Ogre.

The event query is created before any GPU-mutating initialization. Every drawing
step restores the existing state guards. Output copies wait asynchronously for
the event; staging Map uses DO_NOT_WAIT. Only a completed result can be consumed,
and consumption separately requires enabled/current Scope/exact Frame even if no
subsequent Pump occurred. Result buffers are borrowed only until ReleaseResult,
Cancel, Shutdown or another job: consume/copy while the exact result is retained.

The three outputs are raw RGBA16F material, normalRGBA8 and **radialR32**, not world
height. `Complete` means this diagnostic readback finished; it does not certify
finite/covered imagery. A separate validated height/pixel conversion is required
before writing GST2 completion bits. No final atlas publication occurs here.

`CountedLedger` includes a worst-case512/16-patch source/raster/readback reservation
of112,825,820B plus all external retained/pending/retired owners under192MiB.
`OwnedGpuBytes` is only the currently visible COM payload, whereas
`ReservedGpuBytes` also protects active/unknown in-flight work. Successful result
release keeps the83,885,956B source bank and TIFF cache; both remain charged.
OFF/world/source replacement drains and clears them asynchronously. UI/render
integration must not restore the larger viewer reservation until this drain
completes. Source textures are reused, but DDS content still uploads per SID/job.

All status/GPU/controller/destruction methods run on the creating Main thread.
The controller alone forwards cancellation to the worker. Ordinary Cancel polls
without blocking the UI. Explicit Shutdown allows up to5 seconds for joined
worker and GPU completion. An unknown wait/completion retains one complete heap
owner and conservative CPU/GPU reservation in `Quarantine()` until process exit;
new starts refuse. C++ member destruction cannot silently free that quarantined
owner. The host must retain/report this debt and require restart before resuming
an incompatible viewer reservation. No claim is made for invalid caller thread
or pointer use, native-module unload while active, or absence of every OS fault.

Explicit copied CPU payload is bounded below64MiB. As documented by PinnedPng,
opaque WIC/COM/driver allocations and decode latency are not hard-bounded by that
payload calculation. File/hash/decode work stays on the worker. Main contains
bounded uploads and readback copies, but no end-to-end FPS/latency claim follows.
The one-transition-per-Pump API is deliberately unsuitable as an unbatched
4096-window full-world scheduler: measured/budgeted batching and grouping views
by SID are required before production throughput acceptance.

`run_tests.py` runs production lifecycle tests under VC10/Od and/O2, then real
Win32/WIC/WARP flow: two256 frames byte-exact against allthree frozen planes,
one512/1m material observation, shader-source mutation after Submit, persistent
TIFF authentication, OFF drain and world-change cancellation. The metadata
provider is explicitly test-owned; no native API, game/INI, installed atlas or
live save is changed. Generated observations remain ignored.

Logical resource names are now created and checked by world-native/SourceFactory: DDS basenames in Landscape, painted basenames and exact blendMap.png in Overlaymaps. Canonical content pins remain separate. The controller initializes one bounded copied factory per Submit; every later source checks its complete expected role/pin/slot/name, without rebuilding plans per input. The test-owned flow provider uses explicit parent archives for these same logical names; it does not call native APIs. The former full-canonical-path-only controller was observed RED on this real request shape before integration.
