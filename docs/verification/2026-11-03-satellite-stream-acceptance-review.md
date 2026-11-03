# Independent WorldStream pixel-acceptance integration review

Verdict: **CLEAR after two independently reproduced findings were corrected.**
No open P1/P2 remains in the reviewed contract. Final author freeze SHA256 is
`4135661491281d7971ec745c7554171b101e7f11f0942f251ae12ee7bc8af94e`;
WorldStream.h SHA256 is
`85ed283b33e0fcc602f04b1a0d420ca428058e0a9272d7d27beaba5da7f362f5`.
The initial reviewed header was
`25c093e3eb146acc3e41feff7cc456c604f09ab009269c43641ca8347bd4ab6f`.

No production files were edited by this reviewer. Tests ran in an isolated mirror
under this research directory. Game assets were read only; there were no native
game calls, UI, game/INI/install changes, commits or publication.

## Findings and corrections

| Severity | Initial finding | Final disposition |
| --- | --- | --- |
| P1 | Unsolicited worker wake-wait failure unconditionally ran DropInput, releasing the encoded WorkerFileLease blob and decoded RGBA behind an earlier Main WOpened/WDecoded observation. Main Accept/Upload can already hold those buffers when cleanup occurs. | Fixed: all Main-borrowable Work payloads, including lease/RGBA, survive unsolicited exit until Main joins/deletes Work. Explicit Stop still clears all payloads; Decoder.Clear remains on the worker. |
| P2 | After worker failure following Complete, explicit Cancel before Pump observed the exit caused terminal Failed with Error=S_OK and Pump returning S_OK. The failure latch was skipped because state was already Cancelling. | Fixed: exited-worker failure is latched regardless of Cancelling; transition/fence work remains conditional. |

P1 was reproduced with an actual authenticated source/WIC decode at UploadingSource
and WDecoded:1,707,240 encoded bytes and4,194,304 decoded bytes existed before
fault injection, then the initial worker cleared both before Main join. The test
does not dereference freed memory. It proves loss of the ownership guarantee;
the code path allows deletion to race Main's prior observation and upload.

P2's initial actual worker/WARP run ended state14/Failed with Error/Pump00000000
despite worker error80004005. After correction, the exact same run ends Failed
with Error/Pump80004005. `abnormal-cancel-initial-red.log` and
`source-exit-initial-red.log` preserve initial evidence; `abnormal-cancel.log`
and `source-exit.log` record final green results. The production diff contains
only these two statements and explanatory comments.

## Final fresh verification

The complete final9-file source set was recopied after the author froze it. All
37 borrowed pins, original author logs/binaries and the unchanged borrowed mirror
closure were checked. Bundled Python ran the final unchanged `run_tests.py`:

* VC10/Od and/O2 each passed12 lifecycle and29 real worker acceptance controls.
* 393,834 fresh actual streamed-frame checks passed. Three frames completed;
 393,216 accepted height pixels matched independent emitted40B geometry/strip/
 barycentric reconstruction within1mm. Material/normal bits remained unchanged;
 diagnostic RawPlane kept radial while default Plane2 supplied world height.
* Actual unsolicited exits at WOpened, WDecoded and after WValidated retained
 their Main-borrowable owners. Complete-exit-then-Cancel failure reporting passed.
* Five compiled mutants failed their intended runtime assertion: source-owner
 loss, missing cancellation failure latch, accepted/raw/window loss, quarantined
 GPU destruction and reduced quarantine reservation. Query/send/wake/fence,
 OFF/pending-validation cancel and world-change controls passed.

Polling check counts vary slightly with completion timing. The final mirror also
passed65,758 independent controls each/O2 and/Od (`additional_controls.py`):

* Twelve fresh scope/frame mutations and seven full height-source identity
 mutations caused Result, default Plane and diagnostic RawPlane to refuse before
 Pump. Restoring the exact current identity restored valid consumption.
* An actual foreign caller thread could not start/submit/pump/read/cancel/release/
 shut down the creating-thread controller or alter its completed accepted frame.
* Every fake metadata provider Capture/Refresh/BeginBank asserted execution on
 the creating Main thread. Host imports contain no Ogre/Kenshi module. The Work
 object contains copied labels and owned CPU/Win32/WIC data, with no Provider,
 MainToken, engine or D3D pointer reachable by the worker.
* A failed Main thread-status wait after Complete quarantined the whole Work/GPU
 owner, withdrew every getter, preserved borrowed accepted/raw bytes and height
 Window through controller destruction, retained maximum debt and blocked restart.

The final header passed both original independent reproductions and the final
independent context/quarantine suite. No source correction beyond the two
reported issues was requested.

## Ownership, freshness and cost assessment

GPU readback is completed before Main sends ValidatePixels. The worker builds
Accepted from immutable current raw storage and its authenticated Window; no
image scan/native call is moved to Main. WValidated is the interlocked handoff.
Main checks the current scope/frame and Accepted.Acquire before Complete and
again for every Result/Plane read. Raw diagnostic access shares this acceptance
gate. ReleaseResult clears Accepted only while Main owns a completed WValidated
handoff. OFF/world/source replacement and pending acceptance cancel publish no
partial result and drain the worker/GPU before releasing owners.

Unsolicited exit now retains every Main-borrowable payload until Main observes
and joins it; lease/height destructors then use only heap/Win32 cleanup. WIC/COM
decoder cleanup still runs on its worker thread. Explicit Stop occurs during
serialized Main Shutdown. Unknown worker/fence completion retains whole heap
owners rather than deleting potentially in-use storage.

The copied CPU reservation is44,496,018B including both4MiB maximum raw and4MiB
accepted payloads, below the64MiB explicit cap. Accepted's embedded owner is
counted once. Worker validation's other-owner charge plus RequiredBytes(512)
equals that reservation. GPU maximum reservation stays112,825,820B, with all
external debt still admitted under the shared192MiB cap. Quarantine records both
full reservations. Native provider/token owners outside the controller remain
the integration caller's additional CPU responsibility.

This is a bounded one-window helper review, not installed Main/native acceptance,
current-source authority, universal finite shader output, atomic GST2 writer/
index publication, final map readiness or whole-world scheduling proof. One
transition per Pump remains unsuitable as an unbatched full-world throughput
plan. OS/WIC/COM/driver allocations are outside explicit payload accounting;
game FPS/latency and long-run memory behavior need separate measurements. All
controller use/destruction is serialized on its creating thread, and borrowed
views must not outlive owner/context changes. Those preconditions remain explicit.
