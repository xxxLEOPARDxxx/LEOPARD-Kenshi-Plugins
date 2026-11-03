# Bounded terrain cell cohort

`WorldCohort.h` is an isolated controller for exactly two or four terrain views.
It has no native caller, file writer, atlas publication or viewer integration.
The caller supplies Main-authenticated source tokens and coherent current
world/source/material authority. SourceEpoch currently admits only the pinned
base profile; this does not establish arbitrary mod winners or native-listener
GPU equivalence.

The fixed copied `Job` contains one scope, material snapshot900+60, canonical
plan, pinned shader bytecodes, TIFF path, count and four frame/grid slots. Active
frames share region/cell/source/material generation, side, spacing, camera,
far clip, water and wetness. Positions must differ and their bounded grids must
cover each footprint inside that exact cell. Both256 and512 sides are admitted;
the GPU ledger conservatively reserves512/16 patches per view in either case.
The current actual output control covers the four512/9m quadrants of cell27/10.
No general four-view packing, cache replacement or cross-cell bank rebind exists.

One persistent `WorldStream::Work` and its existing process worker slot are
reused directly, without copied worker implementations. It authenticates the
TIFF once per matching source epoch/path and retains its deny-write handle.
Each cohort reads one whole-cell513-vertex square plus two-sample normal halo,
517²×2=534,578B away from world edges. Its maximum reserved window remains
2,117,682B. Worker access reaches copied requests/metadata, files, WIC and CPU
buffers only; Provider, native snapshots/tokens and all D3D owners stay on Main.

The Main-owned token table has all4×15 slots. SourceFactory checks exact
canonical roles, full64-bit identities, source hashes/bytes, slot and logical
group/name before submission and again for every source request. One file is
opened/accepted/decoded/uploaded/released at a time. MainGate commits all15
receipts under one selected-SID transaction before bank completion.

For each source stage, Main renders that stage into EVERY active raster before
advancing the shared bank revision. Padded stages reuse the final bank. Separate
raster owners retain each view's carry/output; Finish runs for every view before
any view is read back. Each Pump advances at most one raster Init, BeginFrame,
RenderStage or Finish. Source uploads remain the existing bounded whole-source
operations, including their existing Main hashes/mip generation. Pump returns
`S_FALSE` if no state transition is ready, including an unreleased Complete view;
it never waits for worker/file/query completion in the normal frame path.

After all stages, one shared staging trio/query serializes readback. Main copies
one immutable frame/scope to the worker for pixel acceptance and independent
world-height reconstruction. Only successful worker validation plus fresh Main
acquisition exposes Complete. `Result(view,enabled,currentScope,exactFrame)`,
`Plane(view,plane,...)` and diagnostic `RawPlane` recheck freshness per call.
Default planes are unchanged HDR16F material/gloss, unchanged native normalRGBA8,
and reconstructed world-Y R32F. RawPlane2 remains radial, solely diagnostic.
Native normal alpha remains0; this is NOT yet GST2 coverage data. The separate
accepted-window cache adapter is not integrated here.

The current result stays unchanged across Pump calls with no active consumer until
`ReleaseView(view,enabled,currentScope,exactFrame)`. **Before ReleaseView, Cancel,
Shutdown or destruction, the caller must finish/join every consumer borrowing
Plane/RawPlane pointers.** While a CPU consumer is unjoined, the caller must also
forbid Pump, which can observe a dead worker/context and begin joined cleanup.
Signal the CONSUMER's own cancellation event first and continue joining it; only
after known completion may Main Pump/Cancel/ReleaseView/Shutdown/destruct.
This controller has no knowledge of an external file
writer and cannot join it. A caller exporting asynchronously must defer stream
cancellation until that consumer is joined, as the private frame controller does.
Wrong/stale release refuses without freeing the current result. Successful
release clears the one Accepted slab only in WValidated, then schedules the next
view; the final release drains completed raster/staging owners and retains only
the bank/TIFF/raw capacity for another same-epoch cohort.

`Borrow(view,enabled,currentScope,exactFrame,Borrowed&)` uses that same fresh
Result gate and zeros the entire output on refusal. It exposes only const
Accepted/HeightWindow owner pointers, copied scope/frame and CPU ledger scalars;
no Work, native, Provider or D3D pointer crosses the boundary. The caller keeps
the controller alive and follows the consumer join rule above. For the separate
AcceptedWindow adapter, set `ledger.retainedRaw=borrow.retainedRawBytes` and
`ledger.other=borrow.otherCpuBytes+externalCpuOwners`. The adapter automatically
adds Accepted.OwnedBytes, Window.OwnedBytes, sizeof(Window) and its own required
allocation. Those first three plus retainedRaw+otherCpu exactly reconstruct
`borrow.controllerCpuBytes==CpuReservation()`, including conservative unused
capacity. The controller does not charge an external writer/thread payload;
the caller adds all of its own adapter, writer, copied metadata and retained
owners exactly once. Borrow checks its current raw/Accepted/window allocation
sizes and refuses an inconsistent or over-cap reservation.

OFF, source/world replacement and explicit cancellation withdraw publication,
cancel worker IO, fence submitted GPU work and asynchronously clear after both
worker and GPU completion. Same-thread Shutdown has the inherited bounded5s
join/drain policy; it is not a normal-frame operation. Destruction and all
controller/Provider/D3D operations require the creating Main thread. Unsolicited
worker exits preserve ALL borrowed source/decoded/accepted/raw/window owners
until Main joins/deletes Work, and the failed HRESULT is latched even when
explicit Cancel preceded observation. Unknown thread/fence completion retains
the whole GPU owner array, Work, thread handle and full admitted CPU/GPU debt;
the process worker slot stays occupied. There is no retry allocation behind that
quarantine.

GPU payload reservation is source bank83,885,956B plus K raster owners24,745,560B
each plus ONE4,194,304B staging trio: K2=137,571,380B; K4=187,062,500B.
External retained/pending/retired/output resources participate through the same
ledger, with exact192MiB boundary and overflow rejection. There is no additional
192MiB allowance for a viewer; its owners must be drained or explicitly fit in
the remaining ledger. Actual owned GPU diagnostics count every allocated raster
and shared staging plane. Reservation/quarantine uses the full admitted peak
even after partial allocation. Copied CPU reservation44,514,346B includes all
controller/Work/GPU owner structures, token/receipt arrays, shader copy, maximum
height window, encoded16MiB, decoded16MiB, raw4MiB and Accepted4MiB slab plus owner.
Opaque WIC/COM/driver/RSS allocations are not a hard total-memory bound.

Run `python run_tests.py`. It compiles with exact VC10 x64 /MT and inspected PE
imports before executing any host binary. Contract controls run /Od and /O2.
The actual WIC/WARP flow first finishes/fences/releases four sequential original
WorldStream512 views, saving private runtime outputs in `_generated`, then
compares K2/K4 raw and accepted outputs bit-exact and1,572,864 accepted heights
against independently emitted strip geometry/barycentrics. No sequential/cohort
GPU owner overlap is admitted. A pinned whole-cell fixture is read in bounded
rows from the installed TIFF; production workers separately authenticate that
TIFF. Fault copies instrument only event/query boundaries and private owner
inspection: seven cancellation boundaries plus recovery, premature bank advance,
opened/decoded/complete/released worker exits, failed send, inconsistent Borrow
raw-capacity refusal and all-four-owner quarantine. Mutants remove OFF gating,
abnormal-exit error latching, complete quarantine debt, destructor retention and
Borrow allocation consistency; each must fail its semantic control.

`freeze.py` records or verifies exact owned sources, actual compiler include
closure, frozen helper manifests, shader/fixture/source pins, compiler/link tools,
libraries, binary/import hashes and current logs. Host correctness does not prove
native integration, full-world throughput, FPS, total RSS, long-run leak freedom,
all OS faults or finished map readiness. No WIC/file IO or blocking GPU Map waits
are introduced in normal Main frames, but existing whole-source hashes/uploads
and monolithic raster initialization still need real latency measurements.
