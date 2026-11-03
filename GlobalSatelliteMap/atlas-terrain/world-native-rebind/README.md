# Checked replacement of terminal native source providers

This isolated Main-only session supplies the missing provider lifetime boundary
for a persistent WorldCohort. No frozen helper, Main caller, game installation,
file writer or atlas publisher is changed. Host tests never invoke actual native
provider methods. The actual NativeSession DLL is compiled/linked/import-inspected
only and is never loaded or executed by these tools.

Include `NativeSession.h` for `WorldNativeRebind::NativeSession`, an alias of
`Session<WorldNative::Provider>`. Generic `NativeRebind.h` deliberately does not
include Ogre; its default type is exactly StageAdapter<NativeStage>, and the
native convenience header supplies that complete definition. Host controls use
the REAL frozen StageAdapter with a plain NativeFakeStage, not a substitute for
the terminal adapter lifecycle.

The session owns both a replaceable single-cohort provider and the actual
`WorldCohort::Controller<Session<Owned>>` constructed with that session. `Cohort()`
returns its reference for the existing Submit/Pump/Result/Plane/Borrow/ReleaseView
API. This avoids treating a bool or an unrelated controller as idle proof.
The constructor's C4355 reference warning is intentional: the frozen controller
stores references and performs no Provider callback during construction.

On the creating Main thread:

1. `Init(scope)` validates the initial scope/CPU peak and starts the one existing
   plain worker. Pump `Cohort()` through Booting to Idle normally.
2. `RebindCapture(job,capturedSources)` admits only its OWN Idle controller, no
   active bank, no replacement/capture already underway, and a valid Main scope.
   It freshly Pumps that Idle controller to observe a worker exit/context change
   before replacement. A world/source change can initiate the normal asynchronous
   drain; finish that drain and retry when actually Idle.
3. On admitted replacement it withdraws Ready, constructs a fresh terminal
   provider, initializes/binds/captures every selected SID/role, creates checked
   session tokens and deletes the previous provider. The old/new object peak is
   explicit. A capture/allocation/constructor failure deletes old and candidate,
   clears every caller token, and leaves no Ready binding. Later known-idle
   recapture may recover. Rejected BUSY admission preserves the current active
   provider and cannot Pump or replace an unreleased view.
4. `ReadyFor(job)` reports the completed capture's exact scope/first frame/plan;
   it is not an independent current-native-winner check. Submit its captured
   tokens to `Cohort()`, then consume/release every view normally. The controller
   retains its worker, authenticated TIFF handle, height window and GPU sourcebank
   between successful cohorts.

The frozen provider permits CaptureSources once per object. Each replacement
therefore gets a fresh object; no reset, stale native pointer adoption or group
scan is invented. Its original raw native snapshots remain Main-owned here.
Public MainTokens carry a unique nonzero64-bit session-binding ID in snapshot
`profile`; every other snapshot field and the COMPLETE original WorkerRequest
remain unchanged. This is a documented proxy token, not a native profile claim.
Refresh/BeginBank first compare that complete stamped snapshot, then route the
ORIGINAL raw snapshot to the actual provider and compare its fresh answer before
restamping it. Native listener identity, original profile, aliases, group, archive,
path, role and pin semantics are preserved. Old MainTokens/receipts/banks cannot
reach a new provider even when source/world/job/role/name are identical and only
the high32 binding bits changed.

Capture is permitted only during this session's synchronous owned capture
transaction and only for the exact current native-token request/slot/input.
Outside callers, including stale same-scope WorkerRequests, cannot recapture
through a newly rebound provider. This requires a second bounded metadata
Capture pass after the frozen terminal provider's own CaptureSources; it does
not reread source files or the TIFF. MainGate later performs the normal file
receipt acceptance and all15-source bank transaction. Main-only BeginBank
translates all15 checked snapshots into a fixed scratch array, enforces a full
unique same-SID role set, serializes the selected underlying provider and routes
EndBank correctly. There are no worker-visible native tokens or session pointers.

All session/native/controller methods and destruction require the creating Main
thread; foreign routing rejects before native calls, including EndBank. All
borrowed consumers MUST be known joined before final ReleaseView makes the
controller Idle. While a consumer is unjoined, do not Pump/Cancel/Release/Shutdown,
destroy the session, or attempt rebinding; signal that consumer's cancellation
and continue joining first. Owning the actual controller proves its state, not
that a caller obeyed the frozen pointer lifetime contract. Unknown completion
requires retaining the WHOLE outer owner/session. Status/errors/quarantine are
the existing `Cohort().State()/Error()` and WorldCohort::Quarantine record. No
automatic replacement/restart bypasses a quarantined worker slot.

`CpuReservation()` is the frozen cohort CPU reservation plus
`ExtraCpuPeakBytes() = sizeof(Session)-sizeof(Cohort)+2*sizeof(Owned)`. This
includes fixed raw60 snapshots, native60-token scratch, selected15-snapshot bank
scratch, factory/scope/plan and overlapping old/new providers, without double
counting the embedded controller. The fake native-stage control measures
45,071,954B and proves that a type whose old/new peak exceeds64MiB refuses BEFORE
allocating a worker/provider. Native instantiation uses its actual sizeof(Owned)
and performs the same admission; the fake byte count is not a native measurement.
For a borrowed AcceptedWindow adapter, add ExtraCpuPeakBytes AND the caller's
external owner/writer/token/copy allocations to Borrowed.otherCpuBytes. Do not
also add the full session CpuReservation. The adapter separately charges its
own memory and automatically charged Accepted/window owners. Opaque native,
WIC/COM/driver/RSS allocations are not a hard total-process bound. No additional
GPU allocation or viewer allowance is introduced; the frozen cohort's complete
192MiB ledger still applies.

`python run_tests.py --all` runs /Od+/O2 controls, actual WIC/WARP repeated cohorts,
native compile-only and semantic mutants. Host PE imports are inspected BEFORE
execution and must exclude Ogre/Kenshi. Controls cover64 regions, equal-low32 /
different-high32 binding replay, complete source/bank identities, missing/late
captures, partial provider constructor unwinding, foreign routing, active bank
and view refusal, and recovery. The repeated WARP test replaces real frozen
terminal StageAdapter instances across two cohorts/four512 views while TIFF
authentication remains one and sourcebank remains retained; accepted outputs
match prior sequential fixtures. Native profile translation is checked by the
fake stage requiring its original profile5 inside BeginBank. Five mutants remove
high64 stamps, capture-transaction gating, own-idle admission, failure withdrawal
and the old/new provider peak; each must fail its semantic control.

This establishes a bounded reusable provider route and host lifecycle/output
controls. It does not establish real native repeated-session success, arbitrary
mod source support, historical native-transformed stream equivalence, FPS,
full-world throughput, total RSS, all OS faults, long-run leak freedom or atlas
readiness. Source/world/material/native winner authority stays with the external
Main adapter; current helpers admit only the pinned base source profile.
