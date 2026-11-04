# Persistent fine-cache file worker

`FineWorker.h` is a concrete single-operation asynchronous owner around
`FineStore::Session`, not a renderer or a viewer coordinator. All file access,
source-independent hashing and cache operations run on its one persistent
worker. Main performs only copied request preparation, nonblocking event/thread
polls, fresh CPU view checks and explicit release of a completed CPU Blob. No
native game, D3D or UI call occurs in this component.

## API and ownership

Construct `FineWrite::Worker` on the real native Main thread. Its scope.thread
must equal that thread; all public methods except immutable byte-count constants
are Main-only.

1. `Start(scope, identity, config, externalLedger)` deep copies the bounded
   Config and full Identity; it creates only worker synchronization primitives
   on Main. Worker creates its own heap FineStore Session/new cache directory.
2. `PollBoot()` returns1 once on successful initialization,0 pending/idle,-1 on
   failure. No initialization acknowledgement is inferred from thread exit.
3. `SubmitPut(actualCohortBorrow, key, identity, externalLedger)` copies the
   actual fresh `WorldCohort::Borrowed`, exact anchored key and ledger. Only one
   request may be outstanding. **Do not Pump, Cancel, ReleaseView, Shutdown,
   mutate or destroy the producer while this file consumer borrows it.**
4. `PollPut(Receipt&)` returns1 once for a completed sealed disk window,0 pending,
   -1 failure. The worker clears **every Borrowed field before** publishing its
   completion state. Only a successful acknowledgement, or a known completed
   join of the stopped consumer, permits the producer to change. A failed poll
   alone is not such a join.
5. `SubmitGet(key, identity, externalLedger)` requests prepared bytes from
   worker-owned RAM first, then the validated disk cache on a miss.
   `PollGet()` returns1 once for a completed read,2 once for a cache miss,0 pending,
   -1 failure. Work owns exactly one Blob. No second operation is admitted until
   the read is explicitly released.
6. `PeekRead(enabled, currentScope, currentKey, currentIdentity, View&)` is valid
   only after PollGet1 and verifies the full64bit world/source/capture scope,
   full key/global-material Identity and current read state. It exposes const CPU
   planes, not GPU objects. Coordinator must additionally require current demand,
   zero producer bank reservation and its aggregate GPU phase ledger before upload.
7. `ReleaseRead()` withdraws the completed CPU read on Main. Worker is waiting
   and cannot asynchronously access the Blob until the next command. The CPU
   planes may not be retained after release; GPU upload must have copied them.
8. `Cancel()` signals the **consumer first**. `TryJoin()` is a zero-timeout poll:
   1 known joined/reaped,0 pending,-1 quarantine/refusal. Explicit teardown-only
   `Join()` waits at most5s; normal frame progression uses TryJoin only.

Cancel is allowed while a read is held, but it does not reclaim that read.
Main must ReleaseRead before the stop can finish. This also covers cancellation
racing the last Get/close instruction before PollGet: TryJoin discovers the
completed read and returns0; repeated ReleaseRead can acknowledge it without a
Peek. A stop command received behind such a read leaves the worker waiting and
the bytes untouched, until Main releases them and signals stop again.
Explicit teardown `Join` with an unreleased read records quarantine rather than
returning an unregistered pending owner. The focused mode7 test first reproduced
that failure, then passed after this correction; the RED log is retained under
the ignored generated evidence directory.

## Failure, retention and byte accounting

Work owns its Session, Config, copied command/key/Identity, one Blob and both
events. Worker clears all Borrowed fields on every exit path. A failed event wait,
uncertain thread completion, missing finished marker, unresolved Session file
close, or unknown kernel-handle close retains the **entire heap Work/Session**
and thread handles in a static debt record. An existing global slot prevents
another worker from hiding that debt. `Quarantined()` remains true even if the
physical thread later exits. Caller must retain the whole producer and its
CPU/GPU owners too; its own reservation is additional to the reported worker debt.

`RetainedCpuBytes()` conservatively reports the64MiB scratch cap on quarantine. This
is intentional fail-closed ownership, not a claim that an ordinary cache leaks
64MiB. The destructor does not free quarantined owners. It also refuses to
delete a Main-held read without acknowledgement. A normal stopped, finished and
known-closed worker is reaped and releases the global slot.

The worker latches `Work.error` **after a known thread join and before deleting
Work**. An EOF or final Session.Close failure cannot become success merely
because the file thread has ended. FineStore's own file/header/hash/Session byte
reservation remains inside Session. `FixedCpuBytes()` is only sizeof Worker+Work;
external ledger adds it once, without adding another Session reservation.
SubmitPut additionally charges Borrowed.otherCpuBytes and retainedRawBytes;
FineStore automatically accounts the Accepted and authenticated height-window
owners it consumes. SubmitGet's external ledger must include the still-retained
producer and all unrelated base/model/read allocations. FineStore also counts
its output Blob object conservatively; there is no unbounded queue or second Blob.

Prepared RAM entries live separately in this actual worker. Successful Put packs
the still-held accepted view before clearing/acknowledging Borrow. RAM hits create
independent scratch Blobs; eviction cannot reclaim Main's read. Optional retention
OOM preserves a successful disk result. `MemoryStats()`/`RamBytes()` are Main-only
atomic telemetry; `RetainedRamBytes()` conservatively reports the boot RAM cap on
quarantine separately from64MiB scratch. See `../fine-ramcache/README.md`.

## Healthy terminal cache cleanup

The512-record file index cannot be reused after its Session ends. Before Close,
the worker therefore discards known closed indexed payloads for a normal stop or
graceful cancellation while the Session is still healthy. This prevents every
map reopening from leaving another512MiB of unusable session payloads.
FineStore performs the bounded physical-identity inventory and HANDLE-relative
deletions **on this worker**, never Main. Empty session directories remain.

An active Borrow or exposed/held Blob cannot reach this cleanup: the consumer
must finish and Main must acknowledge its read first. Unknown close/wait/join,
poisoned partial writes/corruption and abnormal worker failures preserve forensic
data and their existing retention/quarantine rules. Cleanup errors remain errors;
a newly uncertain cleanup close retains the whole Session rather than freeing it.
Graceful cancellation before a Get opens its file still cleans prior known
records even though the worker keeps the cancellation HRESULT.

## Verification

Run bundled Python `run_tests.py`. VC10/Od and/O2 compile and execute the actual
headers with no Kenshi/Ogre/D3D/UI imports. A separate fixture worker authenticates
the real canonical TIFF and builds actual production Accepted1m data. Tests
exercise persistent Put→ack→producer-clear→Get, fresh full64bit gates, single
held-read ownership, cancellation and known join. Runtime syscall faults cover:

- abnormal early EOF with the error retained after Work deletion;
- worker event WAIT_FAILED and retained Session ownership;
- real file close failure that leaves its HANDLE live;
- actual blocked WriteFile: cancel must retain the producer through known join;
- unknown Main join while that write is blocked: debt persists after later exit;
- Get's final actual close racing cancel before its read publication.
- explicit teardown Join while a completed read remains unreleased;
- healthy closed-cache removal only after explicit held-read release;
- cancellation at Get's preopen check, retaining the cancellation error while
  removing earlier known closed payloads on its owning worker.

Quarantine scenarios execute in separate child processes because unresolved
ownership intentionally prevents restart. Those processes keep fixture producer
owners alive until process teardown. Every test data directory is inventoried
and gets a cleanup manifest after the child exits, then is removed. There is no
gameplay/FPS/GPU/long-run-leak claim and no production viewer integration here.
