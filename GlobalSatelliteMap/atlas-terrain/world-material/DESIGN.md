# General material source and ordered bank plan

Implemented here: a small allocation-free C++03 **base-snapshot CPU planner**,
plus offline authentication of real case assets. It covers all1024 coarse cells,
not just the two-SID control. No renderer, native process, existing helper,
installed file, current atlas, personal INI or GPU allocation was changed.

The concrete production cutpoint is one fixed6-diffuse/6-normal source bank,
uploaded for one selected SID at a time, four ordered contribution passes and
one common tail. The GPU bank loader is not implemented here. Current native
resource-winner proof and multi-cell/paint-boundary raster parity remain gates.

## Measured cases and cap

| Case | Actual selected SID order | Native selector | Unique BC3/BC1 layers |
|---|---|---|---|
|(3,3),cell10|2743,48690-Newwworld.mod|18,4,0,0|6/6|
|(6,6),cell3|55399,2730,5663,51632-Newwworld.mod|68,49,37,3|23/23|

All SID numbers in the table have the `-Newwworld.mod` suffix. There are49
four-SID cells. Whole region(3,3) contains SIX selected SIDs across its16 cells;
it must not be called a universal four-SID region. Planner work units are actual
9216m coarse cells within36864m data/paint regions.

Native nine-level2048 BC3 layers cost5,592,384B; BC1 layers2,796,192B.
The worst actual four-SID DDS bank alone costs192,937,248B. One full painted
pair27,962,024B +global blend5,592,404B +selector/interior72B raises sources to
**226,491,748B**, already above201,326,592B before meshes/targets/viewer owners.
Its monolithic source allocation is therefore impossible under192MiB.

One fixed6/6 bank is50,331,456B. With one full painted pair, full global blend,
selector/interior and two RGBA32F ping-pong sets, source/accumulator payload is
**88,080,260B**. At256-square, add existing four-patch RGB raster2,008,168B
and16B stage b1: **90,088,444B before readback**. A complete separate-format
readback owner needs512KiB for RGB16F,256KiB for normal RGBA8 and256KiB for
height R32F: **91,137,020B with all three staging textures**, below the separate
90MiB diagnostic ceiling94,371,840B. This is a conditional design estimate,
not an allocated readback implementation or proof of current source winners.
The separate staged FP32 proof has90,612,732B of render/source payload plus
one at-most1,048,576B test observer; those are different formats and owners.
Accumulator sizing is specifically256-square;
2048-square would need256MiB ping-pong and a new1GiB capture ledger.

`otherGpuBytes` in Planner::Build includes every other live/pending/retired owner,
including mesh/outputs/CBs/b1/readback and viewer resources. It never raises caps.
Current viewer189,006,000B makes every producer plan RefuseBudget. The modes
MonolithicCandidate/OrderedStageCandidate describe arithmetic only, not GPU
readiness, source approval or permission to publish.

## CPU interface and staged-renderer contract

`Planner::Init(nonzeroEpoch)` validates the compiled immutable64-region/57-SID
catalog once. `Build(region0..63,cell0..15,expectedEpoch,otherGpuBytes,plan)`
constructs bounded arrays on the stack; no heap, filesystem, engine or D3D call.
Bad bounds/epoch leave the old output unchanged. All1024 cells are supported.
The offline runner additionally checks catalog/snapshot/source hashes; compiled
metadata validation by itself is not disk authentication.

- Plan retains ALL FIVE native source slots, root mask and tree length/hash,
  including non-null unused slots. Those identities must survive native checks.
- Selected SID order and selector byte values are unchanged. No sorting of the
  accumulation order, compressed shader-row remap or blend-channel renumbering.
  Binding lists are sorted for deterministic storage only.
- Bank content is deduplicated by format+SHA256. Canonical keys are stable; the
  upload file is an actual native role path in this plan. `sourceFiles` retains
  every distinct actual role path, including content-identical aliases. Hashing
  only a canonical representative is insufficient source proof.
- Each Stage exposes original StageSlot0..3, selector code, native channel,
  residual flag, original regional shader row, two<=6-layer banks and twelve
  role-to-layer mappings. The fixed owner may reserve6/6 capacity regardless of
  the smaller unique count; never credit unused capacity as released memory.
- Original b0 is3856B, VS b0 is448B. Keep all15 regional SID parameter/distortion
  positions. Stage role offsets are `240+(row*60+44)*4` for difA/difB and
  `240+(row*60+52)*4` for nmlA/nmlB; VS distortion is `144+row*16`.
  Replace six role-index floats in each pair, preserving unrelated fields/padding.
  Original sampler roles and t0..7 stay; t7 aliases diffuse t0 since stage indices
  are0..5. The staged proof's b1 is only StageSlot,16B; t8/t9 carry previous state.
- Painted sources are the actual overlay/color pair for that data region, not
  all four pairs of its73728m parent primary. The existing shader's one-layer
  control uses this core as paintedTileBase. Original full chains11/12 remain.
  Selector compact4x4 rows must be populated from `region.cells`; do not memcpy
  the first64B of the legacy8x8 selector resource, whose row stride differs.

This contract was coordinated with the separate staged-material proof author.
Actual native parameters come from copied/current typed records through the
existing FillRecord/FadeRow/Distortion logic. The planner does not fabricate
scalar values, force zero fade, invent missing roles or produce fake SID records.

## Exact algebra and retained intermediate fields

Pinned stock `terrainfp4.hlsl` SHA050de26a...27d7 defines the per-biome core;
private GeographyTerrain SHA f9326a91...d9a1 supplies explicit gradients, regional
selectors/banks and fifteen-SID cliff outputs. Vertex SHA6ef9efa2...54e9 and the
native wet/include closure remain pinned in the authenticated evidence.

For each original selected slot, compute the complete existing per-SID core,
including its own scales/slope/absorbance/fade/cliff/paint/road math. Multiply
`part.albedo.rgb` by that SID's brightness BEFORE weighting. Slot0 weight starts
at1 and subtracts each present later channel in order; later slots use their
original blendRGBA channel. Do not clamp negative residuals or normalize weights.

The private reference performs, in slot order:

```
A = 0; N = 0; B = 0;
for slot = 0..3 when selected:
    part = complete_per_SID_core(...);
    part.albedo.rgb *= SID_brightness;
    A += part.albedo * weight[slot];
    N += part.normal.rgb * weight[slot];
    B += part.absorbance * weight[slot];
```

A nonzero selected code still executes its material core even if its weight is
zero. Do not omit/reorder/deduplicate that SID/pass based on weight; only a truly
empty code copies carry unchanged. This also preserves exceptional-value behavior.

Carry exactly eight LIVE float32 values: albedoRGBA4, tangent-normalRGB3 and
absorbance1. BiomeOutput.normal is declared float4, but its alpha is dead after
the componentwise core/accumulation; tail reads RGB only. Do not normalize the
normal or divide albedo/absorbance by weight sums between passes. Read previous
state with unfiltered integer loads; write the next set, then swap. Two planes
are2MiB per set; safe SRV-to-RT ping-pong is4MiB. Additive ROP blending on one
2MiB set would split multiply/add rounding and needs different proof.

Execute the common tail ONCE after all slots: `absorb=1-A.a+B`, makeWet on A,
TBN from the shared full-precision geometric normal, worldNormal from N*2-1,
then gloss/normal/radial/direct-RGB writers. Do not reconstruct this TBN from an
already quantized RGBA8 normal. Preserve native normal-alpha0 independently of
any later coverage flag. Wetness singularities/output nonfinite handling remain
the existing canonical-domain/output-admission boundary, not an invented clamp.

Stock terrainfp4 initializes the first biome directly then multiplies by the
residual; the private reference starts zero and adds slot0 too. These are
algebraically equivalent for ordinary finite inputs, but signed-zero/compiler
mad/rounding parity to stock DXBC is not proved here. The separate staged proof
establishes exact parity to the PINNED PRIVATE monolithic reference, not native
byte equality. No shader implementation or stronger claim is added by this plan.

## Regions, paint interpolation and concrete seam proof

Native core ownership is half-open36864m; UV support adds18m on each side,
span36900m. Keep `(worldXZ-(coreMin-18))/36900` and explicit world derivatives
divided by36900. Never crop the border, stretch a core to an image or reuse the
neighbor's paint pair. Preserve native full mips/samplers; mip0-only savings are
not required by this plan.

Authenticated actual pairs(6,6)/(7,6), sampled at SAME world X110592 and16 Z
points in[73728,82944], differ at most0.10727 raw byte for overlay and0 for color
under CPU mip0 bilinear-WRAP native UV math. This is a source measurement, not
all-mip/GPU seam acceptance and not a reason to blur away differences.

A256-square<=9m footprint can cross up to four coarse cells/paint regions.
Unioning their SIDs can exceed four and the cap. Prepare separate actual-cell
plans and reuse the SAME bank/pair serially. Keep one global view/projection,
pixel grid, world18m geometry and source halo. Preserve per-SID cliff cosines at
global mesh vertices, not pixels or translated patch coordinates.

Four StageSlot passes assume one SID for that slot's contributing job area;
StageSlot alone is NOT a universal multi-cell bank selector. Before adopting a
cell/scissor/mask extension, compare actual4SID(6,6)/cell3 and its neighboring
cells/paint(7,6): odd/even pixel shifts, helper-lane derivatives, clipped triangles,
region row remaps, all pass carries and common tail. Derivative setup is before
selection in the source, and texture gradients are explicit, which gives a
concrete proof cutpoint; it does not automatically prove a new scissor path.
Outputs outside a job's owned half-open pixel area must preserve previous carry.
Only publish after all contributing cells and the final tail are coherent.

## Provenance gate and invalidation

The single missing native fact for production admission is an authoritative
mapping from current GameData role references to the actual resolved texture/
paint resource winners (group/archive/mod precedence + content), tied to the
same world/resource epoch. getOrigin/group/name alone is not that proof.
Existing PrivateGeography validates current SID/path/ordered native slots/tree
and disk hashes, but does not establish that winner mapping. Current atlas
validation also hashes mod sources, all219 DDS/128 PNG/base/shader files and
finalized headers; it validates that captured generation, not a new renderer's
native winner or geometry basis.

A future prepared receipt must bind current typed SID scalars/role strings,
all five native slots/tree identities, actual winner content/shape, shader/source/
define/ABI closure, height source, sampler/capture basis and engine/world/mod/
device generations. Copy typed values; retain no native pointers on the worker.
Unknown/changed/unresolved profiles refuse fine output. Keep the validated
coarse atlas, or vanilla map if that base is invalid; never invent two-SID data.

Worker loading remains one bounded blob: stream-authenticate required actual
paths, stage one DDS<=5,592,560B or one PNG decode pair at a time, upload then
release. Do not collect all46 files or all paint pairs in RAM. Reuse admitted
6/6 GPU resources between passes on the serialized context rather than allocating
two live banks. Source/state/alias/predicate/counter/retired-owner rules still apply.

For fine mode, explicitly cancel/join old detail jobs, hide/release old detail
widgets/SRVs/outputs, retain16 coarse fallbacks, and only then credit those old
128 detail owners. The earlier candidate fine ledger194,634,724B can retain FULL
paint mips by replacing mesh+raster reserves6,291,456B with actual max16 RGB
raster4,036,168B, adding paint difference6,990,504B, using9-mip DDS(-432B), and
stage b1(+16B): **199,369,524B**, leaving1,957,068B under192MiB. Remaining cached
town/pending/retired/driver headroom must fit or refuse; this remains a conditional
residency design, not a measured/implemented all-feature peak.

Any world/mod/resource/device/source/math/density change invalidates incompatible
fine entries immediately. Publish a coherent new visible generation, keeping
coarse fallback during preparation. Keys must include the new fixed18m geometry,
shader/parameters/sampler/source/paint basis; do not reuse old adaptive-LOD GST
identity solely because its asset hashes match. Cold fine raw-cache reads and
source jobs must share one bounded queue; no per-frame catalog/file/world scans.

## Delivered proof

`run_tests.py` authenticates pinned snapshots/catalog/ABI sources and66 actual
case files before writing its local evidence. C++ tests cover all1024 plans,
49 four-SID cases, exact role/hash/path mapping, original slots/rows/CB offsets,
paint indices, native9-mip arithmetic, existing-viewer refusal and cap/epoch
boundaries. /O2 and /Od each pass61,105 checks; eleven compiled runtime controls
fail. `_generated/authenticated-plans.json` records every cell, both real cases,
file hashes, paint measurement and observed current capture/ready metadata hashes.
No current-native winner, renderer-seam, FPS, all-leaks or completed map claim.
