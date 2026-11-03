# Independent WorldCohort review

Verdict: **CLEAR; no open P1/P2 in the final cohort/Borrow contract.**

Final author manifest SHA256:
`bb238e5523ecc45b1f669d778527ff0c93365d80ca0a013a2ad912351f9580ed`.
WorldCohort.h SHA256:
`af238ea65257246f0c68743bb7e92bf4663da292e11cb9c324b7340a637e9a3e`.
All9 owned files,247 dependencies, original logs/artifacts and42 mirrored files
remained unchanged. Reviewer execution/writes stayed in this research mirror;
original game assets were read only. No production, native-game, UI/INI/install
or publication change was made.

## Lifecycle and ordering

Jobs admit exactly two or four distinct views inside one region/cell, with
matching source/material/shape/weather and validated covering grids. Canonical
plan and all source token roles/pins/slots/names are checked before job/bank
mutation. The material/shader/job data is copied. One existing Work/process slot
does file/WIC/pixel work; Provider/tokens/D3D remain on Main.

For each selected SID, every active raster consumes that stage before the bank
advances. All views begin against stage0, four ordered contributions run for
each view, trailing empty slots reuse the last bank, and every Finish completes
before readback. A deliberately premature bank revision refused. The bank is
prepared once per selected SID for the entire cohort, not per view.

One staging trio/query/raw slab/Accepted owner serializes output. Only the current
view is available after GPU completion, worker pixel validation and fresh Main
acquisition. ReleaseView refuses stale/wrong context, withdraws that view, then
permits the next readback. Old labels cannot acquire the reused Accepted object
for the next view. Last release frees raster/staging owners and retains only
the charged source bank/TIFF/raw capacity for reuse.

Inherited abnormal-exit fixes remain present: every Main-borrowable source,
decoded, accepted, raw and height owner survives unsolicited worker exit until
join, and failed exit is latched even if Cancel preceded observation. Unknown
thread/fence completion retains all four raster owners, shared Work/thread and
maximum debt; destructors do not release possibly in-use GPU resources or allow
a replacement worker behind quarantine.

## CPU Borrow contract

Borrow is Main-gated through the same fresh Result path and returns only const
Accepted/Window pointers plus copied scope/frame and CPU charges. Refusals zero
all outputs. Raw+other+Accepted.OwnedBytes+Window.OwnedBytes+sizeof(Window) exactly
equals the producer reservation; unused maximum reserve stays in other. The
consumer must add its own allocation/descriptor and other external owners.

The caller must keep the controller alive and **join every borrowing consumer
before any Pump, Cancel, ReleaseView, Shutdown or destruction**. Signal the
consumer's own cancellation first; only after join may producer state change.
Borrow is not an outstanding-lease counter or automatic consumer owner. This
precondition is essential because Pump can observe an abnormal exit and destroy
Work. Actual CPU-thread adapter conversion and cancelled held-consumer controls
confirmed the supported ordering.

The concrete existing WorldNative::Provider is terminal per job/scope, whereas
this controller takes a generic Provider reference. Fake-provider cache reuse
does not prove native multi-job rebinding. A checked Main proxy/session or a new
controller per native job is a separate integration obligation; no native caller
is included here.

## Fresh tests

The final unchanged suite ran in the mirror with PE imports inspected before
host execution. VC10/Od and/O2 passed448/449 contract checks (poll timing affects
counts). Actual WIC/WARP flow passed1,574,349 controls: four sequential WorldStream
baselines followed by six K2/K4 outputs, all raw/accepted planes byte-exact, and
1,572,864 world heights compared with independent emitted geometry/strip
barycentrics. An actual CPU worker used Borrow to run AcceptedWindow conversion,
preserving RGB/height and changing only normal coverage.

Nine fault scenarios passed: seven cancellation boundaries/recovery, premature
bank advance, WOpened/WDecoded/Complete/released worker exits, failed send, raw
Borrow budget refusal and full four-raster quarantine. Five compiled mutants
failed their intended runtime assertion: OFF result bypass, lost exit error,
understated quarantine debt, GPU destructor loss and missing raw-capacity guard.

An additional317 independent actual pipeline controls passed:

* One extra external retired byte revoked admission after partial Init for both
  K2 and K4, then all GPU reservation drained without quarantine.
* A held CPU consumer was cancelled/joined before any producer mutation; accepted
  backing bytes and Complete remained intact until that join.
* Next-view reuse kept the same Accepted/Window objects, but a consumer with old
  frame labels refused and the fresh current view remained available.
* External debt was rechecked again at Complete with all four rasters live;
  excess admission synchronously withdrew output and drained the full array.

All inspected host imports exclude Ogre/Kenshi. No native callback was executed.

## Bounds and limitations

Copied CPU reservation remains44,514,346B, below64MiB, including the maximum
height window, encoded/decoded16MiB each, raw/accepted4MiB each and fixed owners.
K2 GPU reservation is137,571,380B; K4 is187,062,500B. Both include one83,885,956B
bank, K worst-case512/16-patch rasters and one4,194,304B staging trio. External
resources share the same192MiB ledger; K4 leaves only14,264,092B. A viewer has no
separate allowance. No per-frame growing queue/native world scan or duplicate
worker implementation is introduced. Driver/WIC/RSS overhead and operation
latency are not bounded by these payload numbers.

This proves bounded host equivalence/order/lifetime, not actual native source
authority, global sample alignment/placement, all-world seam repair, full-world
throughput, final writer/mip/index readiness or FPS/leak freedom. Normal alpha0
remains native until the separately reviewed adapter converts it for a writer.
Source/native rebind and session/writer integration require their own review.
