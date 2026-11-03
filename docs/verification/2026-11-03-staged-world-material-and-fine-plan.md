# Staged material and fine-window integration checkpoint

These are reusable components and shader controls, not a completed production
map or a new installed viewer. No release/changelog entry is implied.

## All-cell source planning

`world-material/` covers the64 regions/1024 coarse cells in the authenticated
base snapshot, including49 four-SID cells. Worst whole-bank sources226,491,748B
exceed the192MiB shared cap before geometry/outputs. Per-selected-SID planning
preserves original slot order, selector bytes, regional15-row basis and all
texture-role aliases. It does not substitute the two-SID control elsewhere.

Root fresh /O2 and /Od each passed61,105 checks and all11 compiled negative
controls. Independent review added289 role/mip/identity/no-heap checks. Root
resolved the review's P2 estimate-label error before commit: RGB16F render and
source/carry payload90,088,444B BEFORE readback; explicit512/256/256KiB staging
raises this to91,137,020B. The separate FP32 oracle proof is a different owner.
The previously omitted transitive RegionCatalog.h validator is also pinned.
Header/66 authenticated source assets are unchanged by that amendment.

Final planner freeze:
`2f4e4ce3b117a1ec4fd38a63d698afea1bdd7638ac3c9f8e6ddc0d5facd74e97`.
Amended independent report CLEAR:
`research/gsm-world-material-review-20261103/report.md`, SHA256
`31c4bfa10955528620688b94a95a867a284f6254b76c055df0d548f85f7bf9df`.

## Ordered shader proof

`staged-material/` computes one selected slot per pass, retaining albedo4,
normalXYZ3 and absorbance1 in two RGBA32F planes. Two complete ping-pong sets
cost4,194,304B; b1 adds16B. Normalization, wetness, TBN and final writers run
ONCE after ordered accumulation. The source/geometry/FP32 proof render payload
is90,612,732B BEFORE its transient at-most1,048,576B observer. Driver/runtime
allocations are not measured by those payload counts.

Root fresh full test passed4,719,113 checks: nine actual controls/all three
planes, three synthetic four-slot/repeated/empty/negative-residual algebra
controls,14 actual allocation/health failure cutpoints and10 compiled negatives.
The synthetic controls matter: actual two-SID fixture weights largely select
one slot, so those actual images alone do not prove four-addition ordering.
Independent review added31 owner/binding/unwind checks.

This is exact parity with the PINNED PRIVATE monolithic native-derived shader,
including its zero-initialized accumulation. Stock terrainfp4 assigns the first
weighted biome directly; stock DXBC rounding/signed-zero parity is not proved.
This proof is not a general source-bank loader or production renderer.

Freeze:
`e878ccd4c8161c6afc5cb72dc45061b932c2822537253b7b1dead0ce4dce226d`.
Independent report CLEAR:
`research/gsm-staged-material-review-20261103/report.md`, SHA256
`456963b0d1fa11c3b8f26711e5e16fb0a786dd5f3de6703f6175033a2dcd5b0c`.

## Maximum-zoom window planning

`fine-windows/` anchors256-square windows on the same global origin at1m/3m/9m.
All levels align with logical4x4 tile boundaries. Planning uses the worst visible
translation phase, explicit retained/source/raster/window reservations,128-key
maximum, bounded priorities and full64-bit source/material stamps. A complete
matching key is required for publication; the coarser fallback is retained.

Root independently read the whole production header and ran both builds:
458,358 checks each, all nine real-body mutants rejected, zero C++ allocations.
The author's earlier negative-coordinate unsigned-promotion priority bug was
caught, fixed and independently distance-asserted before this final freeze.
Earlier a687 freeze is withdrawn. Final freeze:
`49da8d195854c8315ddd6ab08faa3f123dd9eab0a87500fba2613a10353a2672`.

This planner does not allocate a cache or raise the old diagnostic90MiB cap.
The Main adapter must reserve a stable reusable pool once, include every
source/pending/retired owner, and enforce the unchanged192MiB shared cap.
Native current source-winner receipts, general source-bank residency, complete
staged raster sequencing and actual production/viewer integration remain.
