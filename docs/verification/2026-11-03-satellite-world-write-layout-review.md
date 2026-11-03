# Independent write-worker and layout review

**Superseded worker pin:** A later concrete parent-integration review found P2 EOF error loss. Independently reproduced at both optimizations; see `eof-addendum.md`. The original worker CLEAR below is historical and does not authorize the unfixed worker. The amended `daf61ef1` header/`35b1612f` manifest now pass the same14-control EOF regression at O2/Od and close the worker finding. Parent post-join error handling remains separately reviewed. Layout remains CLEAR.

Verdict: CLEAR for these two primitives at the pins below. No remaining confirmed P1/P2 finding and no source correction requested. This is conditional on the documented immutable CPU borrow, factory/config and parent quarantine contracts; it is not approval of an unreviewed Main/native integration or atlas publication.

| Scope | Header SHA256 | Frozen manifest SHA256 |
|---|---|---|
| atlas-cache/world-write-worker | `2a7fd2fc5d38c45777cf18df7ec0b8f30435ee68c339100c7ee49d23a9096195` | `0654beaf1ce3f667394a5f3627f8201b5acc9aef17d185f564221dda4552d646` |
| atlas-terrain/world-layout | `20bbbceabcf45ce1609929f51b2ea0e17c9ddb37ce351c98a35f1c9bb59ebcda` | `eba0746849e05ffb552cbd136aaf0a28af3ba4a4249b1155b9b097c02bb2b23a` |

The final manifests'75 source/dependency/evidence entries were independently verified against disk.42 original files in the isolated build closure were rechecked unchanged after testing. All builds, test mutations and generated outputs are in this research folder. No original helper/test output, production source, game, UI, INI or installation was changed.

## Worker findings

Main owns the wrapper and a single pending command slot; the dedicated worker constructs/destroys Factory and Session and performs their file operations. Publication uses Interlocked/event barriers: Submit finishes writing copied command/borrow/ledger before waking the worker; the worker stops accessing borrowed Accepted/window planes before publishing completion; Poll consumes that completion once and returns the slot to Idle. A second pending Submit cannot overwrite it. Scope world/source/capture is stable, and Session independently checks exact Accepted/frame/source/catalog authority before IO.

Cancel sets the stopping latch before waking Stop. Poll cannot acknowledge a completed window once cancellation has begun. The separate zero-wait TryJoin returns0 for a still-live borrower without blocking/quarantining it; the5s Join remains teardown-only. Reaping frees Work/events only after the thread handle is signalled **and** the cleanup marker exists. Unknown waits/absent marker retain Work; that retention is deliberate and does not establish ordinary cleanup.

The caller must explicitly stop and obtain known acknowledgement/join before ANY parent Cohort Pump/Cancel/ReleaseView/Shutdown/destruction. While cancellation is pending, keep all borrowed owners alive and unchanged; signal the writer event rather than mutating those owners. On quarantine, retain the complete worker, parent/cohort and their CPU/GPU debts and do not restart over that unknown job. Destructor fallback does not make early parent destruction safe. A zero-wait poll is not itself current-world proof; Main still checks current scope/frame/save/world before consuming success.

FixedCpuBytes plus borrowed raw/other-controller charges and external charges reach Session without losing the accepted/window auto-charges. Independent inclusive/excess tests exercised the combined Worker+Work+Factory+Session/file reservation+normal copy+Accepted+height+raw+other-cohort ledger. Config must remain fixed CPU-only state; dynamic external referents require truthful additional charges and valid lifetimes. Factory/file destructors must not throw or reenter. No helper can attest unreported external allocations or prevent caller misuse of a plain Borrowed struct.

Idle waits are event-driven. Main Poll/TryJoin use zero-time handle queries and atomic state, with no file/pixel scan. Normal pending work adds no growing queue or repeated worker. The tests establish selected ownership/refusal behavior, not FPS, opaque allocator costs or universal long-run leak freedom.

## Layout findings

Base input bounds precede multiplication; all1024 material cells produce four distinct512/9m windows covering all4096 atlas positions. Region ordering is checked against actual catalog origins. Geometry rectangles cover each footprint within its material cell and remain bounded to16 patches. Fine1m/3m placement preserves the existing key phase/namespace and uses the minimal covering patch rectangle, at most2x2, with arithmetic bounded by the world extent. Invalid requests clear placement outputs. Placement performs no allocation, IO, thread or engine operation and proves no source/material seam quality by itself.

## Fresh evidence

- Original suites mirrored into this folder: worker45 checks each `/Od` and `/O2`, plus3 unknown-wait and2 missing-marker controls; layout157,189 each.
- Added25 real Accepted worker ownership/budget checks each optimization. A real IO barrier proves no acknowledgement while owners remain borrowed, normal TryJoin takes under100ms and returns0, then known join releases the borrow. Inclusive combined64MiB and+1byte cases respectively write/refuse before file creation.
- An actual pending-borrow WAIT_FAILED cut passes14 checks: no acknowledgement, no file close, owners remain ready and quarantine forbids join/release. This isolated process exits without running source destructors; it intentionally preserves unknown work, not a leak-free teardown demonstration.
- A compiled premature-acknowledgement mutant fails the barrier assertion at runtime.
- Added13,824 layout cross-module checks each optimization: all4096 placements agree with actual Session mapping/source-grid edges, plus independent fine interval/world-end checks.
- Host worker imports were independently inspected: only ADVAPI32 and KERNEL32, no Ogre/Kenshi/D3D/native game execution.

Reproduce with `mirror.py` then `extra.py` using bundled Python. Source copies/pins, frozen manifest snapshots and build/run/import logs are retained here. The worker tests use the actual aligned Accepted fixture and real Session with controlled RandomFile callbacks; they do not perform real disk writes or native cohort calls. Separate Session review/real-file evidence and integration review remain required before running the native producer.
