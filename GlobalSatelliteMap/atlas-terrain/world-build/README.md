# Serialized cohort producer and window writer

`WorldBuildControl.h` owns a native-provider rebind Session, its actual cohort
controller, one persistent WorldWrite worker, copied job/shaders and source
tokens. It connects the reviewed components without changing them. It does not
select world coordinates/materials, publish a complete atlas, create a native
caller or change the game. Config/Factory are CPU-only file state; production
must use an owned fixed path rather than pointers into short-lived callers.
Any caller-owned configuration referents and external ledger payloads must
outlive the whole operation, including quarantine. Accounting is not ownership.

Begin starts both owners with one stable source scope/capture and atlas-wide
material label. Ready requires both initialized, producer Idle and no queued
or borrowed view. Queue copies exactly one valid two/four-view job plus all
shader bytecodes; source snapshots are rebound only at its own known Idle
boundary. Caller changes cannot overwrite the active job. Current per-region
material data and exact native winner authority remain the caller's obligation.

Tick advances only the producer until one Accepted view is available, then
passes its CPU-only Borrowed owners to the file worker. **While writing, no
producer Pump/Cancel/Release/Shutdown is called.** A successful worker result is
published only after it clears the CPU borrow. Main then rechecks current
scope/device/exact frame/result, verifies returned GST2 placement and releases
that one view. One return1 acknowledges a complete cohort, not a ready atlas.
Source/worker/bank owners persist for a distinct next cohort with the same
capture. The writer's Session preserves its incomplete-file ledger across them.

OFF, absent/replaced current context, file/source failure or explicit Cancel
first cancels the file consumer. Tick uses zero-wait TryJoin; only known writer
completion permits producer Cancel/Pump/drain. A5s asynchronous consumer join
deadline retains the whole Owner with conservative64MiB CPU and producer GPU
debt instead of freeing memory under IO. Unknown waits/missing worker cleanup
markers and producer quarantine do the same. No retry allocation follows that
retention. Destructor teardown has the explicit worker5s join contract and
requires the creating Main thread. Terminal Tick also invokes the inherited cohort Shutdown, with its own
bounded5s producer teardown; only the normal producer/writer polling and writer
cancellation wait phase are nonblocking, not every terminal path.
Finished files can remain forensic partial
data after a later scope change; they are never a successful live-world ack or
COMPLETE/ready publication. Complete() requests graceful close only at Ready
after at least one written cohort; return2 is emitted after writer close and
producer drain with an unchanged admitted scope. A later OFF/context change
still turns that close into refusal. This is not a completeness proof for4096
base windows; the outer scheduler must establish its own expected coverage.

CPU admission composes the native Session/controller peak, root owner/job/token
and shader storage, writer fixed/config/Factory storage, actual file reservation,
accepted pixels/height/normal conversion and supplied external payloads. The
worker Session checks the64MiB ledger again before IO. Do not add its automatic
owners twice. This is a copied-payload bound, not a stack/driver/RSS promise.
The producer currently receives a **zero external GPU ledger**: the future Main
caller must drain the viewer/other private GPU owners before admission. This
controller cannot be installed alongside an uncounted active viewer and claim
the shared192MiB cap. It never creates another GPU budget allowance.

Runtime RED against a rejecting Begin stub preceded implementation (after a
test-scaffold type-name collision was corrected). `python run_tests.py` uses
the actual frozen StageAdapter/NativeFakeStage and WIC/WARP producer, real
Accepted/Window/Session/worker, two consecutive K2 cohorts with one capture and
four unique base windows. Every sink row in all three planes matches preceding
sequential outputs, except the required normal coverage0→255 conversion.
All file calls execute on the worker. Additional cases cancel before production
and change world scope while actual Session IO is blocked at an event barrier;
stale EOF never acknowledges or releases the parent before known consumer join.
Host PE imports are checked before execution. This establishes the isolated
integration only: no actual native game run, physical destination throughput,
whole-world completion, FPS or long-session leak claim is made here.

Independent review found a terminal-error propagation defect at graceful close:
a joined writer could have failed after its last successful row, but the parent
discarded that HRESULT and returned2. The committed regression first reproduced
that false success using an actual wait failure after two written windows.
Worker reap now preserves its terminal error and the parent inspects it after
known join; the same regression returns-1 with E_FAIL and closed file owners.
