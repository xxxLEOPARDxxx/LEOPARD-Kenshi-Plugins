# Sealed accepted256 child crops

`AcceptedCrop::Tile` is an owned CPU snapshot of one aligned256x256 child of an
already authenticated `WorldPixels::Accepted` parent. It copies FP16 material,
normal RGB and reconstructed world height exactly. Accepted normal alpha0
becomes coverage255. It never consumes raw radial depth and never revalidates
parent depth using the different child camera centre.

`Build` runs on a worker, with parent and height window alive and unchanged
until that worker joins. It calls the actual parent's `Acquire` once, checks
256-aligned in-bounds pixel offsets, derives the child frame, checks the full
64MiB CPU ledger, then copies only the child rows. There are no native world,
D3D, disk or per-pixel allocation operations. The slab is exactly1MiB; the
charged `RequiredBytes()` includes the Tile object. `ledger.retainedRaw` and
`ledger.other` include every additional live owner, including other Tiles.
The budget includes accepted parent, height slab and height object separately.

After Build, the tile retains no parent/window/file pointers. `ParentFrame()`
preserves the accepted parent's original camera provenance. `ChildFrame()`
contains the derived256 centre with the same scope/source/material/job stamp.
The authenticated height identity is carried into every acquired view.

`Acquire(enabled, now, childFrame, cancel, out)` checks the exact sealed scope
and child frame, then exposes immutable owned planes in O(1). It does not scan
the parent or child. It clears `out` on failure and leaves the snapshot intact.
`Error()` reports the most recent Build result. Every consumer must join before
Build, Clear or destruction; there is no internal synchronization. Build failure
withdraws previous output and frees its slab. Main-thread copying is refused.

Supported geometry is256,512 or1024 parent side, subject to acceptance by the
real parent validator. This module does not enable1024 in WorldRaster,
WorldCohort or WorldPixels, change CPU caps, provide a scratch loan, or integrate
FineStore/Main. Those coordinated changes remain required before actual1024
rendering can use its16 children. No arbitrary raw-plane constructor exists.

Run `python run_tests.py --mutants` using the project's VC10 toolchain. Tests
reuse the existing512 GPU planes and authenticate the existing canonical height
source; no new game assets or raster fixture are generated. Four children are
compared byte-for-byte and by three-plane hashes, then used after parent/window
release. The actual parent-depth/child-camera negative control is rejected.
Artifacts stay in `D:/DEV/Kenshi/research/gsm-large-parent-20261104`.
Host tests do not establish in-game1024 rendering, FPS or long-term memory use.
