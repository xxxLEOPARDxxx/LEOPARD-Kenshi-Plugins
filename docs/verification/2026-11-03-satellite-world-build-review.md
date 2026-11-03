# Independent WorldBuildControl review

Verdict: **CLEAR after one independently reproduced P2 was corrected. No open
P1/P2 remains in the reviewed private-control contract.**

Final parent manifest SHA256:
`4e69f046846571e3c80faf8fe0dfd866dcf03ff6642270db4828e253a24e7378`.
WorldBuildControl.h SHA256:
`1ec7a61071ded9363af95e153df28c468e8b32cf8c9d0ac98a377605dfe7c471`.
Amended WorldWriteWorker manifest:
`35b1612f85f2f48ff48d7f3f296b4ae4009e1a047297be4cc71c18be1d76dad7`.
Final7 parent sources,54 dependencies,11 author evidence pins and60 copied repo
inputs remained unchanged. Reviewer modified no production/game/INI/install/UI
data and executed no native game API. All review work stayed in this directory.

## Closed finding

P2: after real successful K2 writes, the idle writer could exit on a wake-wait
failure before Complete/Poll. Complete used cached readiness, known TryJoin
deleted Work without transferring its failed terminal error, and the parent
returned graceful2/Error=S_OK. This was reproduced with actual WIC/WARP,
Accepted, Session and file-worker execution; only the worker wait was faulted.

The worker now latches failed Work.error before deleting a known joined Work.
The parent checks writer.Error after successful TryJoin before declaring graceful
completion. The same independent reproduction changed from result2/Error00000000
to result-1/Error80004005, with all file owners still closed. Initial and final
logs are retained. The worker amendment also received its own independent peer
review. The ownership guarantee was safe before this fix; error/success reporting
was not. This issue is closed in the final hashes above.

## Parent ownership and freshness

Owner contains the native rebind session/cohort, persistent file worker, copied
job/shaders/tokens and operation state. Initial CPU ledger adds native peak and
root extra storage; each write uses Borrow's producer decomposition plus native
extra/root/external charges. The worker adds its own fixed/config/factory cost;
Session adds its fixed/file/normal/Accepted/height charges. These are explicit
payload charges, not ownership of arbitrary caller references or an RSS bound.

While writing, Tick does not Pump, Cancel, Release or shut down the producer.
Only Poll1, published after the worker clears Borrow, permits fresh producer
Pump/Result/device/context/placement checks followed by ReleaseView. One return1
acknowledges one whole cohort, not atlas readiness. Complete is allowed only at
Ready with written windows, and return2 follows known writer join and producer
drain; OFF/stale context supersedes that requested success with refusal.

Cancellation signals the writer first. Until known TryJoin, the producer and its
accepted memory remain unchanged. Unknown wait or the5s async writer-join deadline
retains the entire Owner with64MiB CPU debt and reserved GPU debt, blocks restart
and avoids freeing native/provider/producer/borrowed storage. Destructor handling
has the same complete-owner retention policy.

## Fresh tests

The final unchanged suite ran in an isolated mirror; host PE imports were
inspected before execution and contain no Ogre/Kenshi module. VC10/Od and/O2
passed197/198 integrated controls (poll timing changes the count). Two consecutive
native-style StageAdapter cohorts produced four real WIC/WARP accepted views
through the actual Session worker under one capture. Every material/worldheight
byte and normal RGB row matched preceding frozen sequential outputs; only normal
coverage changed0→255. File calls stayed on the worker and one file was reused.

Early cancellation, actual blocked Session IO with a stale world, copied job/
shader ownership, duplicate queue refusal, graceful completion and known cleanup
passed. The final normal suite includes the actual idle-writer failure EOF
regression; the original independent reproduction also passed after amendment.

Independent held-writer fault controls additionally passed187 timeout/rollover
and185 unknown-join controls. A test clock placed Stop near DWORD wrap:

* At4999ms Tick remained pending and returned without blocking.
* At exactly5000ms across rollover it retained the full parent and refused.
* Injected failed writer join retained immediately.
* Producer stayed Complete, its accepted bytes and native-style owners stayed
  intact, and no producer Pump/Cancel/Release occurred behind the writer.
* Parent destruction preserved64MiB CPU/full GPU debt and blocked another Begin.
  External test referents remained alive until the actual writer thread finished.

No unsupported pointer was dereferenced and no native game process was accessed.

## Explicit scope qualifications

Factory/Config may contain references; all such external referents must outlive
the operation including quarantine. The host Stats/expected buffers did so.
Production must use owned fixed configuration rather than short-lived pointers.
CPU accounting alone cannot retain those objects.

The producer currently receives a zero external GPU ledger. Private Main must
drain the viewer and other helper GPU owners first; this control cannot claim
a second192MiB allowance. No actual native caller or production IO factory is
provided here, so their gates/path freshness need separate review.

The writer wait phase uses zero-time polls. **Terminal Tick is not strictly
nonblocking overall:** Finish calls the inherited producer Shutdown with a5s
teardown bound. Destructor teardown is likewise explicit. Future interactive
integration must account for this latency or provide asynchronous producer join.

Whole-world coverage/catalog authority, actual native source winners, final
writer directory policy, full map/mip/index publication, runtime FPS/IO latency
and long-run total memory remain separate. This CLEAR supports the next private
native integration step within those documented limits.
