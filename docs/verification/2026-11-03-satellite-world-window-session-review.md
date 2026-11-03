# Independent WorldWindowSession review

Verdict: **CLEAR; no P1/P2 finding.** No production correction requested.

Final author manifest SHA256:
`866f6792a3a452cdeb1760ff6792b13254ed5319202afc1dc4afa4cd2be7d78f`.
WorldWindowSession.h SHA256:
`58897825667c60f408c990651085ac02737bdaf136d8a1f6e0b72a993e0cb71e`.
Original source/dependency/generated pins remained unchanged. Review execution
and temporary writes stayed under this research mirror; installed/game/INI/UI
and production source were not changed.

## Contract reviewed

Locate computes exact canonical512/9m base addresses, rejecting unsupported
shape/spacing, nonnumeric/outside/misaligned positions without rounding. All4096
positions map to16 tiles and256 windows each. Fine1m/3m addressing is outside
this Session. Corrected harmless coordinate/duplicate/budget refusals preserve
existing healthy state; source/capture/catalog/OFF/cancel/IO failures poison it.

Source/capture scope is fixed for one session. Atlas-wide material/catalog
generation is separate from each window's exact material generation. Every
window must pass actual Accepted acquisition through the adapter; copied labels
remain caller authority, not native winner/image authentication. Only normal
coverage is copied; full RGB16F/gloss and CPU worldheight stay borrowed unchanged.

Write is synchronous on its owning worker. Adapter/file IO consumes all three
planes while the producer remains immutable, then releases the adapter. No
Accepted/Window/raw pointer survives between calls. The session never controls
the producer: Main must cancel the consumer first and wait for its acknowledgment/
join before Pump/Cancel/Release/Shutdown/destruction of that producer. Factory
must outlive files/session, transfer unique fresh file objects and have honest
CPU bounds; physical CREATE_NEW/path containment are the integrating factory's
responsibility. This header does not protect an arbitrary factory that opens an
installed/existing file.

IO false/exception/cancellation poisons every writer, closes owned files and
disallows resume. All writes/flushes precede the counted window; final cancellation
is checked after flush. Partial bytes may remain in a failed private directory,
with no rollback or readiness claim. Session Close releases files and withdraws
healthy progress; one-shot ownership prevents stale writer-pointer reuse.

## Fresh tests and independent extensions

Bundled Python ran the full frozen suite from an isolated copy. The only test
change retargeted its hard-coded owned-directory safety assertion from the
author's path to this mirror. The mirror manifest changed only that test pin;
normalized source comparison proves no other test/source edit. No borrowed
fixture generator was invoked or rewritten.

VC10/O2 and/Od each passed8,775 primary and36,498 adversarial controls. Two real
aligned WARP→Accepted windows were written to a newly created Win32 GST2 file and
read at independently calculated offsets. All RGB/gloss/normal/coverage/height
bytes matched;524,288 heights matched an independent literal-source plane
equation within0.00006103515625m. Decoded8/4/4 formats/flags0 stayed unchanged;
two windows did not advertise coarse/atlas readiness. The1,431,307,264B logical
temporary file was closed/deleted in the mirror, including mutation unwinding.

All26 IO/exception cuts,13 real in-IO cancellation cuts, stale scope/catalog/
capture/source cases, foreign-thread refusals, CPU boundaries, factory failures
and12 compiled runtime mutants passed their intended assertions. No crash or
compile failure was counted as a kill.

An additional101 controls each/O2 and/Od populated all16 distinct memory-only
file slots and checked exact close-once ownership. A reused factory object
returned for another tile poisoned safely, deleted its one existing owner once
and did not double-delete on Close. Excess budget after16 live files caused no
IO or progress change; corrected admission wrote another window without opening
a17th file. Closed sessions could not revive stale writer state.

The16-slot extension used explicitly synthetic finite material/normal input
with radial values derived from actual authenticated height windows and the
real Accepted gate. It isolates ownership/bookkeeping, not rendering provenance
or16 measured physical file handles. The two primary windows remain the actual
render/readback evidence. Host extra-control imports were inspected before
execution and contain no Ogre/Kenshi module. No native API was executed.

## Bounds and integration limits

Reservation checks sizeof(Session)+16×FileCpuBound with overflow refusal, and
Write additionally charges the one1MiB normal payload, Accepted, height window/
owner, retained raw and caller other. The adapter's embedded owner is subtracted
from the forwarded session charge, preventing double counting while retaining
the complete reservation. All buffers/file slots/window bits are fixed; no
per-window metadata queue grows. Factory/source/cache/worker objects outside
the automatic charges must remain in the caller ledger. GPU accounting does
not change. OS/CRT/filesystem/RSS overhead is not a hard-bounded claim.

The final mirror contains **zero temporary .gst.partial files**. No full-world
capture was performed. Current native provenance, whole-world material continuity,
long-running cohort scheduling, failed-directory cleanup, mip/index construction,
atomic atlas publication and game IO/FPS remain separate. Root's new write-worker
and layout were not reviewed here and are assigned to another reviewer.
