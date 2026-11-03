# Fine terrain job and phase policy

`FineJob.h` is a pure bounded policy/job adapter, not the live map coordinator.
It reuses frozen `WorldLayout::Fine` and actual `WorldCohort::ValidJob`, the
real native source/material planner and shader ABI. It allocates no heap,
creates no worker/resources, performs no IO and calls no engine API.

The input demand carries full source/world/capture/revision identities and the
actual visible world rectangle/screen dimensions. Only anchored 256x256@1m
and @3m detail keys are accepted. Render priority is center-first; all wanted
keys are grouped into distinct same-cell K2 views. An odd group gets a distinct
same-cell companion explicitly marked unwanted. `CenterPair` returns only the
first two wanted same-cell keys, or one wanted plus an unwanted companion.
It still reserves all viewport pan phases; it does not pretend the entire view
is refined after two outputs. No 9m key enters the fine namespace.

Producer and publication phases are separate. `Budget.viewerGpu` MUST be a
measured aggregate of all live coarse/relighter/water/model/local-light,
pending and retired GPU owners. `fineGpu` counts all old fine/pending/retired
fine bytes; `occupied` counts their slots. These are owner-supplied measurements,
not native receipts authenticated by this header. During production their
coarse viewer aggregate must be <=32MiB. Existing fine tiles may remain
within the measured shared192MiB cap after adding frozen K2's actual
137571380B reservation. K4 is not admitted. `producerCpu` MUST be
the actual owning NativeSession::CpuReservation(), including provider/controller
and accepted payload costs. A mandatory frozen-payload lower floor rejects
zero/tiny invented costs. `otherCpu` includes every other live CPU owner, writer,
headers, index, metadata and retained blob; the shared cap is64MiB.

At publication, `CanUpload` requires the current exact demand and wanted key,
a KNOWN joined/acknowledged consumer, and the ACTUAL producer reservation zero
after asynchronous bank/fence drain. It counts the next20B-per-pixel resource
against96 slots and the same192MiB GPU cap. Do not pass true/zero merely because
cancellation was requested. The policy does not drain or join on its own.
Budget values must be read again by the real owner before each allocation.

`MakeJob` takes an immutable base-wide source/material stamp plus the actual
region-specific snapshot/settings, copies those into a K2 native ABI job, and
requires exact current scope/demand. The global base material stamp and
per-region frame material generation remain distinct. Shader pointers in the
returned transient job are borrowed; existing producer Queue must deep-copy
before their storage ends. Complete base identity, material snapshot digests,
source authority, map/world visibility, native context, all lifecycle/cancellation
and GPU state restoration remain the integrating coordinator's responsibility.
This primitive alone cannot qualify an incomplete atlas for fine rendering.

Run `python -B run_tests.py` with bundled Python/VC10x64/SDK7.1. /Od and /O2 each
pass34600 production controls, including all1024 native cells' boundary keys,
all local1m/3m mesh phases, independent viewport coverage, odd4cell pairing,
QHD77-window capacity, fresh64bit scope/revision, strict zero-bank upload,
CPU/GPU debt/caps, reuse-aware pan capacity and exact retained-fine
production headroom, and real source-plan/shader jobs in all64 regions at both
1m and3m. PE imports are inspected before each executable and prohibit engine
DLLs. Historical RED rejected the initial anchored key; a second observed RED
caught a fabricated one-byte producer budget before its floor was added.
These checks do not establish native game output, pixel continuity, worker IO,
latency, FPS, absence of long-run leaks or completed live-viewer integration.
