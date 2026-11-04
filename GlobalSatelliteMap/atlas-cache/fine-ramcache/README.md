# Prepared fine-tile RAM retention

`FineRamCache.h` is wired into the actual `FineWrite::Worker`. It retains
immutable, validated256x256 material/normal/height bytes, not GPU objects or
native game pointers. All packing, allocation, lookup, copying and eviction run
on that one file worker. Main does no tile copy and no cache file access.

## Ownership and budgets

- `Begin(scope, fullIdentity, budget)` binds one worker/world. Worker copies the
  process-session `GsmCacheProfile::RamBudgetBytes()` on Main before starting:
  settings1..32GiB, default2GiB. There is no eager GiB allocation.
- A fixed16384-bucket pointer index costs128KiB on x64. Each node charges its
  complete metadata (Blob fields and links) and exactly1MiB of payload. The cache
  header is conservatively included too. Capacity uses64bit subtraction before
  division; even32GiB cannot hide an extra node.
- IO, accepted producer, height adapter and published read clones still use the
  independent64MiB scratch ledger. Retained RAM payload is not added to that
  unrelated limit. Published reads are separately reserved1MiB Blobs.
- Key spacing/coordinates, full64bit source/material generations, complete
  base/catalog/material digests and source scope prevent reuse across worlds.
- Hash lookup has bounded chains; intrusive LRU touch/removal allocates nothing.
  Nodes are allocated lazily. Eviction and healthy terminal clear are worker-only.

After successful physical `Session::Put`, while the original immutable
`WorldCohort::Borrowed` is **still held**, worker packs a retained copy through
the actual `AcceptedWindow::Adapter`. It repeats fresh scope/frame/window and
normal checks and creates canonical normal coverage255. Only then are all Borrow
pointers cleared and the Put acknowledgement published. `Blob::CaptureAccepted`
is an internal trusted-consumer API: it requires a successful real Put and
unchanged Borrow, not an arbitrary untrusted Receipt. It does not claim to verify
Receipt SHA without the Session's private canonical file header.

Get checks RAM before disk. A hit clones into the one read Blob under the old
scratch ledger. A validated disk hit is also retained opportunistically.
Index/node/payload allocation refusal skips optional retention without changing
a successful Put or poisoning the valid disk Session. Read-clone allocation
failure exposes no cache pointer; disk/coarse fallback remains available.
Cancellation is checked between bounded64KiB copies.

## Stop, quarantine and telemetry

Published reads never point into LRU nodes. Eviction, clear and cancellation
therefore cannot free Main's view. Explicit ReleaseRead and consumer-first
cancellation/join remain unchanged. Healthy stop clears RAM on the worker,
then removes known closed disk files. Unknown HANDLE/worker completion or an
unreleased read retains the entire owner.

`Worker::MemoryStats()` exposes Main-only, individually atomic telemetry:
bytes/hits/misses/entries. During publication they can represent adjacent points
in time; **do not use the tuple for admission**. Admission uses immutable boot
cap. `RetainedRamBytes()` conservatively reports the full RAM cap on quarantine,
separate from RetainedCpuBytes64MiB scratch debt. Healthy joined owners report0.

The cache creates no speculative queue. Viewer policy prioritizes visible work
before bounded prewarm pairs; worker handles one submitted operation at a time.
Keeping this owner warm across map close is a separate viewer lifecycle policy.

## Verification

Bundled Python `run_tests.py` compiles actual headers VC10/Od and/O2 and uses
authenticated canonical TIFF/Accepted data plus real owned fine-store files.
The original warm test first failed because pre-change Worker called disk Get.
Now the first post-Put Get makes zero ReadFile calls and preserves exact HDR,
height SHA, normal RGB and coverage255.

114 controls per optimization cover exact-cap LRU/touch/eviction, full identity,
lazy32GiB arithmetic, independent64MiB clone admission, actual allocation refusal,
disk fallback and subsequent RAM hit, cancel/held-view lifetime, cleanup and
separate quarantine RAM debt under the real32GiB worker boot snapshot. Worker29 controls and150 runtime-fault controls
per optimization also pass. Disk-close fault paths deliberately refuse only the
optional128KiB RAM index, so warm hits cannot bypass the file fault under test.

Fixtures are inventoried/removed by existing worker cleanup. No game/native/GPU
calls are made by these tests; they do not prove live FPS or long-session memory
stability across all machines.
