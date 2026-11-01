# Offline bounded mip comparison helper — 2026-11-01

This is the explicitly allowed helper cutpoint. MipComparison.h and
MipComparisonPolicy.h are not included by CaptureProbe.cpp and do not register
any startup flag, file request, callback or game action. The installed full-mip
candidate from0b14a88 is unchanged. Native PNG load/accounting and92-mip live
comparison remain unimplemented/unproven in this step.

## Implemented boundary

- Comparison::Begin borrows a full ordinary2D SRV and a full four-slice array
  SRV only during preflight, then owns bounded COM references to their textures,
  shared device and immediate context. It rejects invalid inputs/partial views,
  mismatched dimensions/format/full chains/flags/sample counts, alias resources,
  cross-device sources, over-budget allocations and an already active job.
- Native texture ownership, creation/Loaded/accounting are the caller's job.
  The helper contains no engine/native calls or guessed ABI. The future native
  TexturePtr owner must outlive its comparison and use the exact research
  contract's create→setters→direct base loadImage route, with one built-in
  Loaded/notification completion and quarantine on uncertainty.
- One pair traverses all mips. A Tick submits one subresource copy OR attempts
  Map READ/DO_NOT_WAIT once. A still-drawing result leaves the fixed job pending;
  no sleep, Flush, Present, spin loop or mapped pointer survives Tick. A caller
  supplies current DWORD time; the job fails on its1..120000ms deadline, handles
  wrap, and separately caps Map attempts at65536.
- One staging resource has one mip/slice and exact current-level dimensions.
  The old staging resource is released before the next is allocated. A tight
  CPU snapshot is bounded16MiB; padded mapped rows are copied/compared correctly.
  Every byte of every RGBA channel is compared exactly. Results contain SHA256
  hashes, per-channel mismatch counts/max/sums and dimensions; Count exposes
  completed levels only. Mismatch reports Different; it never becomes Equal
  from a healthy device status or absent readback.
- Completion, failure, cancellation and destructor paths release all owned COM,
  cryptography handles and snapshot storage. GPU commands can still be pending
  in the runtime, but no helper reference is retained. Caller cleanup of native
  engine ownership is a separate lifecycle/accounting operation.
- Plan budgets one four-layer array +one complete native2D chain +one largest
  staging mip +explicit retained texel bytes, under the existing128MiB cap.
  Peak colour plan128625316bytes fits; adding the other array refuses. Snapshot
  CPU memory, codec/native/view/driver overhead need separate host accounting.
- Schedule is a production pure gate: explicit opt-in, no competing capture and
  verified lifecycle are required. It advances only complete11/12-level pairs,
  four layers per kind, and finishes at92 comparisons. Cancellation cannot
  publish completion. This is not yet a runtime config/request adapter.

## Verification

Run run_mip_comparison_tests.ps1. Initial failing-stub proof is retained in
build/mip-comparison/red-stub.log. Full output is build/mip-comparison/full-run.log.

The real WARP fixture compares ordinary D3D2D versus arrays populated with the
already reviewed production GpuMipUpload.h. It covers full1024/2048 chains for
all four layers in both DXGI28/RGBA and DXGI87/BGRA:184 native-size mip comparisons,
223696192 compared native-size bytes, plus8×8 positive/negative fixtures. The
ordinary WARP resource uses native-equivalent UpdateSubresource+GenerateMips;
it does **not** call the installed native Ogre PNG loader and cannot establish
that loader's pixel equivalence.

Additional checks cover known independent raw-fixture SHA256, one-pixel changes
in each RGB/alpha channel, wrong slice, partial chain/view, invalid formats and
budgets, cross-device refusal, one Map attempt per tick, preserved OM/SRV/
viewport/IA state, cancellation during copy/map stages, deadline wrap and16 real
source/view destruction-witness cycles including queued copies. The headless
test harness Flush/drain calls are external to the production helper.

Generated header copies omit the array copy, use a wrong mip, skip alpha or
omit cancellation Release. Each must fail its real GPU/ownership assertion.
Production has no skip/debug mutation flags. COM refcount checks compare stable
baselines; real private-data witnesses also prove final object destruction.

Independent review reported CLEAR and reran the complete standalone runner:
build/mip-comparison/independent-review.log records2506 successful checks,
all184 full-size mip comparisons in both formats, and all four correctly failing
mutants. The total assertion count varies slightly with nonblocking Map polling;
the fixed mip/byte coverage does not. Existing probe policy/accounting suites and
GlobalSatelliteMap tests also passed without rebuilding the private probe DLL.

Reviewed source SHA256 identities:

| File | SHA256 |
| --- | --- |
| MipComparison.h | bdd1f92e1c812cdd93c351610e91872c1950f77c34a4902ea2a3aa574113156a |
| MipComparisonPolicy.h | 22eb75ee048c34d5bbff6c03384ae1ae820a3e8c053635bb65ad72af13d9fafb |
| mip_comparison_warp.cpp | 2f8d197cbc95e5e7509087329a44080cb02aed541d817156b98b83bc570ee3df |
| run_mip_comparison_tests.ps1 | 385c5bf789c4d5de92fe9996b2070c4728a26d6d37700e25cf6cd0bb29f57b20 |

The separately installed8adf86 full-mip candidate's game results belong to that
candidate and do not establish native PNG parity for this unwired helper. This
step performs no private DLL rebuild, installation or game/process action.

## Remaining safe integration work

The current probe's asynchronous hooks are MapScreen::Update and workspace
callbacks. They do not yet provide a verified load/shutdown cancellation hook
for a native2D owner when MapScreen stops ticking. That lifecycle must be
established before the new native job is wired in; COM-only Cancel cannot safely
debit an uncertain engine resource after Ogre Root disappears.

Then add a startup-latched default0 opt-in and a distinct request token with
no replay, mutually exclusive with normal captures/private Terrain/RTTs. Validate
all eight exact CPU PNGs before allocation and retain them. Own only one array
kind and one ordinary2D at a time. Use profiled create/setters/direct loadImage,
guard creator/unloaded/Loaded/full descriptors, and call this comparison helper
once per pair. The ordinary direct base loadImage already sends its credit;
never run array notify/setToLoaded completion on it. Preserve one bounded
quarantine after unknown native effects; do not retry/debit blindly.

Require92 exact result records,184 successful readbacks,111848096 compared bytes
and balanced native manager/name counters per array/ordinary texture/full job.
Measure cancellation/load/shutdown and comparable idle process/GPU retention
before any runtime acceptance. Existing WARP/static proof cannot establish
universal leak freedom or a hard bound on an individual driver call.
