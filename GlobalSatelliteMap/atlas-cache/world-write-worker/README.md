# One persistent worker for incomplete world-window writes

`WorldWriteWorker.h` runs the existing WorldWindow::Session entirely on one
dedicated owning worker. It does not render, acquire native metadata, operate
UI, publish a ready atlas or permit an engine pointer on the file worker.
The caller supplies a Factory/Config containing CPU-only file configuration;
Factory construction, Session Begin/Write/Close/destruction and all file calls
execute on that worker. Config is copied; any external referents must remain
alive and be explicitly charged until known join. Use a fixed configuration
without native, controller, D3D or UI pointers in production.

Start establishes one immutable source scope/capture and atlas material label.
Poll returns2 once for initialized idle,1 once for a completed window,0 while
pending/idle and-1 on refusal. Submit copies one freshly acquired CPU-only
WorldCohort::Borrowed plus the ledger; a second pending request cannot overwrite
it. The actual Accepted and authenticated HeightWindow remain borrowed through
the Session call. They are cleared from Work before completion publication.
The caller still checks current world/frame/scope and honors any cancellation
before releasing the matching cohort view; worker EOF does not establish that
the live game context is still current.

**Keep the complete parent/cohort alive and do not Pump, Cancel, ReleaseView,
Shutdown or destroy it while the writer is unacknowledged.** Cancel signals the
writer event first. TryJoin polls with zero wait (0 pending,1 joined,-1 refusal)
for ordinary asynchronous cancellation. Join is an explicit teardown operation with a5s bound and
returns true only after the thread exits with the final cleanup marker. Main
also reads Error after a known join: cleanup completion does not imply successful
file work. Reap preserves a terminal worker HRESULT before destroying Work.
must finish this join before cancelling or releasing the parent. Unknown waits,
timeouts or an absent finish marker retain the complete worker and report
Quarantined; the containing parent must likewise be retained with its CPU/GPU
debt until restart. Destruction on a foreign thread is unsupported and retains
the worker rather than freeing potentially borrowed memory.

FixedCpuBytes accounts for Worker, Work/config and Factory. Borrow's raw and
other-controller charges plus all external payloads are added with checked
arithmetic. Session separately charges its own fixed/file-bound reservation,
one normal conversion, Accepted and authenticated height owner. Do not include
those auto-charged owners again in external charges. This is copied-payload
accounting; allocator/OS stacks and opaque driver costs are not an RSS bound.
Session independently refuses over-budget writes before file IO.

The worker waits on one event when idle. Main Poll uses only a zero-time thread
query/atomic status and never scans pixels or performs file IO. There is no
growing command queue, per-window thread creation or normal-frame blocking wait.
A failed Session write latches terminal refusal and cannot be retried over the
poisoned/partial destination. No successful acknowledgment can reappear after
cancellation/join. Failed/partial files remain incomplete and are not published.

Factory and file destructors must not throw. Run bundled Python `run_tests.py`: actual aligned WARP fixture data passes real
WorldPixels acceptance before being handed to the real Session worker.45
controls each/Od and/O2 verify worker-owned file calls, exact placement, single
pending ownership, duplicate refusal, active-write cancellation using a real
event barrier, partial IO failure, stale scope and overflow rejection. Isolated
wait-failure and missing-cleanup-marker controls exercise conservative retention.
Host imports are checked before execution; these are CPU-only tests with no
native game calls. They do not establish full native integration, actual disk
throughput, world completion or game FPS/long-run memory behavior.
