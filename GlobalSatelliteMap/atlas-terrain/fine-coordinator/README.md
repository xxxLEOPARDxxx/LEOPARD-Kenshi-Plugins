# Two-phase fine-detail owner

`FineCoordinator::Controller<NativeSession>` joins the existing native metadata
session, actual WorldCohort producer, FineJob phase policy and persistent
FineWrite/FineStore consumer. It is a private Main/compositor owner, **not** a
complete game viewer integration. Caller supplies checked device/context/map
admission, verified complete-base Identity, actual regional material snapshots,
current viewport demand and every retained external CPU/GPU allocation.

## Main API

- `Begin(scope, verifiedIdentity, newCacheConfig, externalCpuLedger)` owns the
  native session and persistent file worker. `scope.job/sourceEpoch` remain
  stable for this world/base session; viewport revision is separate. Native
  provider rebinding still creates its own fresh full64bit binding transaction.
- `MakeBudget(residency, externalCpuLedger, FineJob::Budget&)` derives the real
  native CPU reservation and all coordinator/worker/Session scratch overhead.
  `Residency` explicitly contains viewerGpu, fineGpu, pendingGpu, retiredGpu and
  occupied fine slots. Pending/retired owners join the aggregate viewer
  allowance, never disappear into zero. Fine textures and the producer share
  one512MiB cap; all CPU owners share one64MiB cap.
- `Queue(plan,pair,currentDemand,currentIdentity,copiedNativeJob,residency,cpu)`
  accepts one exact K2 pair. It regenerates the pair's expected canonical native
  placement/plan and checks exact frames/grids. All shader bytes and job data
  are copied before caller memory may change. Main performs only native metadata
  capture; source bytes, TIFF authentication, WIC decoding and cache I/O stay on
  their respective workers.
- `Tick(admitted,device,currentDemand,currentIdentity,residency,cpu)` advances
  one bounded phase. Returns1 for one completed pair,2 for source GPU fully
  drained,3 for a completed read,4 for a cache miss,0 pending/idle,-1 failure or
  completed hard cancellation. **None of these publishes a GPU texture.**
- `Drain()` requires the completed pair/consumer idle state. It cancels the
  source producer and pumps cancellation asynchronously until its real
  `ReservedGpuBytes()==0`. Normal Tick never calls the up-to5s Shutdown loop.
- `RequestRead(plan,key,currentDemand,currentIdentity,residency,cpu)` is admitted
  only after actual source GPU drain. It reuses the persistent512-record disk
  index. `PeekRead(enabled,currentDemand,currentIdentity,residency,cpu,View&)`
  rechecks exact full64bit scope/revision/Identity, wanted key, CPU budget, GPU
  phase/slot capacity and absence of producer borrows before exposing CPU data.
- `ReleaseRead()` must follow synchronous CPU upload/use. Root must not retain
  those pointers afterward. It also works while soft viewport drain is waiting
  for an already-held read; the caller releases that read before new work.
  A successful Peek marks the read as exposed. Hard Cancel/Tick cannot reclaim
  that exposed payload either: they wait for explicit ReleaseRead, and a missed
  teardown deadline retains the complete owner. Final Shutdown with an exposed
  read also retains ownership instead of freeing behind a live View. Only a
  completed read never exposed to Main may be discarded automatically.
- `Cancel` begins hard cancellation. `Terminated` means the file consumer is
  known joined and source GPU drained, but the idle source worker/native metadata
  may still be owned. **Explicit final teardown** `Shutdown()` then joins/frees
  those owners; it is not a normal live-viewer frame operation. Destructor is
  final teardown too.

## Consumer-first ownership

Once a wanted view is borrowed, Tick does not Pump, Cancel, ReleaseView, rebind
metadata or mutate the producer until FineWorker's actual completion publication
has cleared all Borrowed pointers. The sealed receipt's full key/Identity is
checked, then the producer is freshly validated and released. An unwanted K2
companion is released without a cache write. After pair completion the source
bank remains accurately reserved; it must be drained before upload/read phase.

FineStore accounts its own Session/adapter/Accepted/window objects. FineWorker
adds only its own fixed wrapper/Work bytes and the producer's Borrowed raw/other
CPU decomposition. Coordinator adds NativeSession extra/provider peak and its
own copied owner fields, not a second full producer charge. Read phase explicitly
includes the entire retained native CPU reservation plus one pending read Blob.
MakeBudget conservatively reserves the larger Put/Get scratch peak.

## Soft viewport change versus hard invalidation

A viewport revision/rectangle change with the **same full scope/base Identity**
withdraws the old pair and stops new metadata/Borrow submissions. An already
running Put finishes and acknowledges its old authenticated key; only then does
the source producer cancel/drain asynchronously. The file worker/Session and
bounded cache index remain alive. Old records may be reused when those exact
keys are wanted again. No outdated pair-success or GPU publication is emitted.
Caller coalesces only the latest viewport; this owner has no request queue.

If a read was already exposed before the viewport changed, Main must
ReleaseRead. A newly completed but unexposed old read can be discarded by Main
during soft drain. After drain, caller can Queue the latest plan or RequestRead
its existing keys without creating another cache directory.

OFF, hidden/unadmitted map, missing device, world/load/source scope replacement,
or a changed full base/material/source Identity triggers **hard** cancellation:
file consumer Cancel + nonblocking known join first, then producer Cancel and
asynchronous GPU drain. Even equal source/material generation numbers do not
hide different SHA32 Identity pins. The old result is withheld.

Unknown file close/join/wait or source drain quarantines the complete heap Owner,
including both workers, native source owners, copied metadata and GPU allocation
debt. A static retained owner prevents restart. CPU debt conservatively reserves
64MiB; GPU debt is the actual producer reservation. Caller separately retains
and charges any external viewer/fine/pending/retired owners. Quarantined owners
are never deleted by the destructor.

## Authority and integration requirements

The coordinator cannot turn nonzero test hashes into proof of a verified base.
Only the root's completed-source/parameter identity validation may authorize
Begin and current Identity. Native metadata/device/context/map/world checks and
foreign graphics-state restoration remain prerequisites of the existing producer
and the caller's checked compositor callback. Cross-DLL global budget ownership
must also make the private probe absent or inert before a production fine owner
is admitted; local counters are not a process-wide GPU budget.

The minimal path renders true origin-anchored256x256 material/normal/world-Y at
1/3m spacing. It does not invent sub18m height samples, sharpen/upscale an image,
or change any native material/relighting formula. Viewer GPU upload, live native
lighting, viewport coalescing and resource eviction are caller responsibilities.

## Verification

Bundled Python `run_tests.py` compiles/runs VC10/Od and/O2 with real WIC/WARP,
actual original source files, a test-owned native-style provider, unmodified
NativeRebind/Cohort/Accepted/Worker/Store, and actual fine disk files. No engine
DLL is imported. Two1m outputs are bit-exact in HDR/world height and normal RGB
to preceding independent sequential production renders; coverage is255. Actual
retained viewer/pending/retired WARP textures have nonzero ledger charges.

Tests prove read/upload admission refuses the retained source bank, soft pan
keeps cache hits after GPU drain, stale full64bit viewport gates publication,
and hard cancellation closes consumer before drain. Four runtime tests block a
real FineStore WriteFile while borrowing actual accepted pixels:

1. Same-base viewport pan permits the active Put ack, caches exactly that old
   key, drains without publishing the stale pair, then reads it under latest demand.
2. Hidden/OFF cancellation leaves the producer unchanged until the blocked file
   consumer completes cancellation and known join.
3. Unknown consumer wait retains the whole owner and its real GPU debt; a new
   owner cannot hide that reservation.
4. Full base SHA change with identical generation numbers takes the hard path.
5. Hard Cancel while Main holds an exposed View waits for explicit ReleaseRead.
   This scenario reproduced a real RED before the exposed-read fix and passes
   in both optimizations afterward; its RED log remains in generated evidence.

Every generated fixture is inventoried and cleanup-manifested after its test
process exits. Quarantine tests deliberately keep whole owners until process
teardown. Tests establish component ownership/correctness, **not native game
visual acceptance, instantaneous pan refinement, FPS or long-running leak proof**.

Normal-frame shutdown uses `RequestShutdown` then `PollShutdown` after `Terminated`: source Idle, GPU0 and known file-consumer join are prerequisites. Poll waits zero milliseconds. Failed/unknown close retains ownership. A known empty source owner after startup failure can be deleted without a wait. Soft viewport drain acknowledges any borrowed Put before producer mutation even if retired old fine textures temporarily exceed the32MiB producer allowance. Within the shared512MiB aggregate cap and a valid64MiB CPU ledger this condition waits for the owned fence, preserving the persistent file session. The actual WARP test retires a64MiB texture and confirms a subsequent cache hit.
