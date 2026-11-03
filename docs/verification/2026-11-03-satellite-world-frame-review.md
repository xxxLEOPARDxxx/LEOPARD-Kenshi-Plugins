# Independent WorldFrameControl review

Verdict: **CLEAR after the EOF receipt audit correction; no open P1/P2.**

Reviewed final author manifest SHA256:
`e54f78a42faa041677799f8267db218b14f55a8bf6c16260d32c50cc860b2d68`.
WorldFrameControl.h SHA256:
`a237c23b6fe20a57641e8b6e28b9cf34f4930ed672bb5df4083abe0211526ce2`.
All four owned files,220 dependencies and34 independently mirrored files remained
identical. All ten fresh host executable import sets match the author manifest.
The CaptureProbe caller is explicitly outside this review.

No production files were edited by the reviewer. Execution/writes were confined
to this research mirror; original game assets were read only. No native module
was executed, no UI/game/INI/install changes or publication were made.

## Audit correction

Before freeze, the writer set Export.done at the end of Save, but Main initially
trusted thread termination and error0 without reading that EOF receipt. An
unexpected early writer return could therefore acknowledge an unfinished export.
The author added a required done==1 check after a proven join, before any success
acknowledgment. The final actual-thread early-return test refuses; removing that
guard fails its intended assertion. No Main file I/O or extra write phase was
introduced. This issue is closed in the reviewed header.

## Ownership and freshness

Begin translates constructor/provider exceptions and refusals into HRESULTs and
cleans partially constructed owners without leaking the process worker slot.
The owner copies the Job and shader bytes; caller temporary buffers are not
retained. Provider/native state stays on Main. Export reaches only copied
header/scalars/path and borrowed accepted CPU pixels, with no controller,
provider or D3D pointer reachable by its thread entry argument.

While a writer exists, Stop requests writer cancellation and defers stream
withdrawal. Known join precedes ReleaseResult, stream cancellation/shutdown or
owner deletion. Unknown wait/destructor completion retains the complete Owner,
requests cancellation and records full CPU/GPU debt; new ownership is refused.
The quarantine therefore preserves the provider, stream, accepted pixels and
export context, rather than retaining only the export descriptor.

At EOF, Main validates admission, current full Scope/exact Frame, current device
through fresh stream.Pump, stream.Result, the token and the EOF receipt. Only
then can Tick return1. Scope/frame/device/token changes after the file finishes
produce refusal and joined cleanup. Error/partial/cancelled paths do not become
positive completion. The caller remains responsible for the authority of its
current world/material/source labels.

The artifact contract is deliberately private: Save may already have renamed a
diagnostic frame.gwf before Main sees a later context change. That historical file
may remain with its original world/source/job/token header, but it is not a
successful current-frame acknowledgment, GST/GSI file or atlas-completion record.
A held writer cancelled before writing produces no final file. materialSha
covers parameters[900] only; distortion[60]/complete material authority are not
implicitly certified by that field.

## Fresh tests

Bundled Python compiled and ran the final host controls in this research mirror.
Each executable's imports were checked for Ogre/Kenshi **before execution**.
VC10/Od and/O2 each passed65,609 basic controls: real Win32/WIC/WARP streaming,
accepted RGB/normal export parity, all65,536 world-height pixels checked against
independent actual emitted geometry/strip barycentrics, exception/refusal cleanup,
copied CPU admission, cancellation and late world/frame/device/token refusal.

The actual-thread fault suite passed:

* 5 abnormal EOF controls, including early writer return without done.
* 6 actual private write-failure controls with cleanup/refusal.
* 10 held borrowed-writer cancellation controls: accepted pixels stay intact and
 stream remains Complete until the writer joins.
* 10 failed writer-wait controls: full Owner/debt survives controller destruction
 and restart refuses.

All four compiled negative controls failed the intended runtime assertion:
missing EOF receipt, early stream cancellation, owner loss and stale EOF success.
Compilation failures or crashes were not counted as kills. One redundant direct
fault-run attempt collided with the already running builder's same executable;
the builder's own complete fault stage subsequently passed. The fresh test stages
all passed; mirror manifest generation initially lacked three borrowed metadata
files. Those were copied unchanged and their exact original pins were verified
independently by finalize_review.py; this did not alter any test or source.

## Bounds and remaining integration obligations

There is one bounded stream and one export thread. Export borrows the accepted
slab and writes64KiB chunks; it does not duplicate pixel payloads. Directory
length is limited and the CPU reservation includes Owner/controller/string
storage plus bounded scratch. Existing stream CPU/GPU accounting and quarantined
maximum debt remain charged. No growing queue, world scan, per-frame source
rebuild, native worker access or Main file I/O was found.

Directory containment/installed-atlas protection and native callback admission
belong to the separately reviewed caller. No actual native Provider execution,
installed-game performance, whole-world scheduling, final viewer integration or
GST2 index/mip publication is proven here. OS/WIC/COM/driver overhead and arbitrary
invalid-pointer/concurrent misuse are outside the explicit payload/lifetime
contract. This CLEAR supports the parent's next controlled native frame test.
