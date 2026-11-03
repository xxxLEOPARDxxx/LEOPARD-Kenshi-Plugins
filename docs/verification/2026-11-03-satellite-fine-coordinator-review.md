# FineCoordinator independent review: CLEAR after exposed-read P2 fix

2026-11-03. No production edits, game input/files/configuration, installation, publication or commits by reviewer. No additional GPU/native benchmark was run while the root's full native atlas capture was active.

Final header SHA256: `dd8fd15aa783e209a1803764692b4d09eea6ebb6c63e66330c5922e916f5f302`.

Final author verification SHA256: `3683e4e3c9015285315d862dcbb95660ece038bf8af16c07b00d3e12aef9b299`.

No open P1/P2 remains in the reviewed coordinator owner scope.

## P2 found and corrected

Initial header `77ba1281d5342cf64872585e1db02d4fa63501ee52c881b636a4a686da81f5f4` automatically called FineWorker.ReleaseRead during hard Tick/Shutdown even if Controller.PeekRead had already returned a CPU View which the caller had not explicitly released. That bypassed the file worker's held-read guarantee. Main serialization alone did not make the documented public ownership contract safe; the author confirmed the gap.

The final header tracks `readExposed` only after an actually successful Peek. Hard Tick may discard a completed **unexposed** read, but waits for explicit Controller.ReleaseRead when exposed. A missed hard-stop deadline retains the whole owner; final Shutdown with an exposed View also retains instead of freeing its bytes. Release clears exposed/read-ready state and permits consumer join before producer cancellation/drain. Soft drain's explicit read release behavior is preserved. The README now states this contract.

The author's new actual WARP→file→Get→Peek→Cancel mode5 was RED before this change and GREEN after it: recorded161 controls each/Od and/O2. It proves Cancel/Hard Tick remains pending until Controller.ReleaseRead, then terminates without quarantine and finally tears down all native-style owners. Reviewer inspected that production test and the changed branches; no duplicate GPU run was added.

## Handoff and phase review

- Queue regenerates the pair's canonical expected frames/grids/source plan and copies all shader bytes/job state before rebind/submit. It checks current scope/full Identity, fresh plan/pair and the actual aggregate producer budget.
- Once a wanted view is borrowed by FineWorker, Tick does not Pump, Cancel, ReleaseView or rebind the producer until the real Put acknowledgement has cleared all Borrow pointers. The sealed receipt's exact key/Identity/serial is checked; the producer is freshly validated before ReleaseView. An unwanted K2 mate is released without publication.
- A same-scope viewport change withdraws old result publication, allows an in-flight authenticated Put to acknowledge its old key, then cancels/drains the producer asynchronously. Cache Session/index remain owned for later exact-key reuse. A held read waits for caller release. There is no unbounded viewport queue.
- OFF/unadmitted map/device loss or changed full source/world/base/material Identity triggers consumer Cancel and nonblocking known join **first**. Only then can producer Cancel/Pump(false) drain GPU resources. Tick never invokes the blocking source Shutdown. `Terminated` denotes consumer joined/source GPU drained; explicit final Shutdown still releases the idle source worker/provider.
- Read requests and fresh Peek admission require producer reservation zero, no pending borrow, exact current plan/key/scope/Identity and actual shared resource ledgers. Pending and retired GPU bytes remain counted. The coordinator does not publish GPU textures itself.
- MakeBudget charges actual NativeSession.CpuReservation plus copied owner/token/job/shader fields, FineWorker Work, FineStore Session and the larger Put/Get scratch allowance under one64MiB cap. Put adds only native extra/provider overhead because the real Borrow decomposition supplies producer slabs/controller ownership; Get includes the whole still-retained native owner. All viewer/fine/pending/retired GPU owners share192MiB and the producer phase retained allowance32MiB.
- Unknown consumer close/wait/join, source quarantine or teardown timeout retains the whole heap Owner, both source/file workers and their metadata/resources. Static debt blocks a new hidden owner. CPU debt conservatively reserves64MiB; producer GPU reservation is retained and external viewer owners remain the caller's additional responsibility.

## Independent CPU-only controls

`run.py` pins the final production header and creates a test copy with **only Controller access visibility and include spelling changed**. Every tested method body is the actual production method. An empty native owner exposes the real frozen Cohort CPU reservation but never starts a source/file worker or GPU. No second coordinator algorithm is used as an oracle.

VC10x64 **/Od and /O2 each pass21 additional controls**, with PE imports checked before execution and no Kenshi/Ogre/D3D/UI DLL imports. They check complete SHA32 base/catalog/material first/last-byte differences despite equal generations, full64bit world/capture/material changes, pending+retired aggregation at exactly32MiB and plus-one refusal, old fine bytes added at render admission, exact64MiB CPU boundary/plus-one/overflow, correct distinct Put-extra versus Get-whole-native ledger composition, and empty-owner explicit teardown. Source hash is rechecked after execution.

Artifacts: `review.cpp`, `run.py`, `generated/ReviewCoordinator.h`, compile/import/run logs for both modes. These additional controls create no source/cache fixture files and execute no rendering. Author WIC/WARP integration/fault tests were inspected separately: actual K2 output is compared with independent sequential production output, file acknowledgements precede GPU drain, soft pan preserves an old key, hard OFF/base-hash replacement joins the consumer first, and unknown consumer wait retains actual producer GPU debt.

## Limits

The complete-base authority, regional material snapshot pins and native device/context/map/world checks remain caller inputs. This review cannot turn synthetic test hashes/provider metadata into native game proof. Production viewer upload/eviction/relighting, cross-DLL global ownership exclusion, current-source validity, real hardware latency/FPS and long-run memory growth still require the root's integration/hardware acceptance. No such completion is claimed here.
