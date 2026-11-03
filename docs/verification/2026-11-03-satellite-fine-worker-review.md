# FineWorker independent review: CLEAR

2026-11-03. Reviewed the actual `GlobalSatelliteMap/atlas-cache/fine-worker/FineWorker.h`, normal/syscall fault tests and documented integration contract. No production edits, game calls/input/files, configuration, installation, publication or commits by reviewer.

Frozen header SHA256: `850c67bc5a269daeba7271104f1ef55cc862e5d93899cf623b3f4abb1c17e97d`.

Frozen author verification SHA256: `3e71f38a058b3a150d0312741273484de2d525e8fbfd43b136e7ab122f03c81b`.

No open P1/P2 in this persistent synchronous-file-worker owner scope.

## Ownership and failure review

- Main preparation copies owned Config/identity/key/ledger and actual producer Borrow. One global slot and one outstanding command prevent a second hidden worker/queue. Native/file operations remain in the worker; Main performs nonblocking polls and explicit CPU read release.
- The worker clears every Borrow field before publishing PutDone/Failed and also clears it on exit. A successful Put acknowledgement releases the producer borrow. Failure alone does not qualify producer mutation: caller waits for known consumer join or conservatively retains the whole producer on quarantine.
- ReadDone keeps the Blob until Main explicitly ReleaseRead. Cancellation that already sees ReadDone latches the hold; a Stop received behind a newly completed Get also leaves the Blob/read state intact. TryJoin returns pending rather than freeing it. Explicit fatal Join/destruction with an unreleased read retains the whole owner/debt instead of returning a fictitious successful teardown.
- Get final close racing Cancel cannot clear the Main-visible blob asynchronously. Fresh PeekRead requires exact64bit scope/key/identity, current read state and the consuming enabled flag. Published pointers become invalid only at explicit ReleaseRead or final retained process teardown; the coordinator must not keep them afterward.
- FineStore unresolved close, worker WAIT_FAILED, unknown Main join, missing finished marker, still-owned Session/Blob or unknown kernel close preserves the complete heap Work/Session in static debt; the slot remains occupied. Later physical thread exit does not erase that ownership debt. External producer CPU/GPU retention remains additional and is explicitly required.
- Known thread join latches Work.error **before** deleting Work. Abnormal EOF and final Close failure cannot turn into success merely through reaping the thread. Cleanup proceeds only after known finished marker, no retained flag and a closed/deleted Session. Normal resource handles close before Work deletion and slot release; any unknown close retains the remaining owner.
- FixedCpuBytes includes Worker+Work; FineStore automatically counts Session and consumed Accepted/window/adapter/output data. SubmitPut adds the actual Borrow other/raw fields and caller external owners with overflow-safe additive checks. SubmitGet external ledger must still account the producer retained elsewhere. Quarantine reports the full64MiB CPU cap and blocks restart; it is conservative debt, not an ordinary64MiB allocation claim.

## Independent focused evidence

`review.cpp` includes the actual production FineWorker header and uses the production Accepted fixture; it does not substitute a second worker implementation. VC10x64 **/Od and /O2 each pass25 additional controls** (the count includes bounded pending polls). PE imports are inspected before execution with no Kenshi/Ogre/D3D/UI DLL imports.

Independent checks cover exact64MiB Start versus one-byte excess refusal before output-directory creation; actual global one-slot exclusion for a simultaneous second owner; no directory created for the rejected owner; high64bit source mismatch before queueing; an actual unwritten-key miss acknowledged once with no held Blob/stale view; known normal close/join with error/debt clear; and slot release permitting the previously refused second worker only after old owner reaping.

Author normal27 controls and128 runtime fault controls were inspected. They use actual canonical-height Accepted data and actual Windows syscall boundaries: early worker EOF with error surviving Work deletion, worker wait failure, live failed file close, blocked WriteFile cancellation while producer remains valid, unknown join debt surviving later physical exit, Get final-close publication racing Cancel, and fatal Join while Main holds a read. Unresolved-quarantine cases run in child processes so their retained owners survive until process exit. The self-review amendment for explicit Join with an unreleased read is present in this frozen header.

Review artifacts are `review.cpp`, `run.py`, `generated/compile-{Od,O2}.log`, `imports-{Od,O2}.log`, `run-{Od,O2}.log`, and cleanup manifests. Only empty own fixture directories were created by these additional miss tests; paths/reparse status and contents were inventoried before removal. None remain.

This review does not prove a later native coordinator's actual producer freeze, GPU bank drain, shared byte measurements, global cross-DLL ownership exclusion, native pixels/lighting, hidden/load transition behavior, gameplay FPS or long-running memory growth. Those are integration/hardware acceptance requirements rather than claims of this file worker primitive.
