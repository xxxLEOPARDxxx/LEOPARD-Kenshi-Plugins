# Ordered staged terrain material proof

This isolated C++03/D3D11 WARP proof splits the authenticated native-derived
terrain pixel calculation into four selected-slot passes and one shared tail.
It uses the existing fixed two-SID real DDS/paint/height sources. It is separate
from Main, the probe, the game, atlas files and a future bank uploader. It does
not establish a general four-distinct-SID runtime renderer or current native
resource-winner admission.

The motivating full four-SID bank can exceed the viewer's192MiB cap. This proof
establishes the shader arithmetic needed to replace that whole bank with ordered
per-slot banks; it does not upload those banks yet.

## Exact source and pass contract

`generate.py` invokes the unchanged existing compact preparation recipe with
THIS component as destination. The recipe reads authenticated local game assets
and writes expanded snapshots only below this component's ignored `_source/`
and `_generated/`. No previous component, game/probe/build directory or installed
file is modified. No expanded game shader is tracked or redistributed here.

The geography source must hash to
`f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1`.
The exact reference here is that previously reviewed private native-derived
GeographyTerrain source and its prewrite oracle, not stock terrainfp4 DXBC.
The private source initializes zero then accumulates slot0; stock terrainfp4's
first biome uses direct compute/multiply before later additions. This proof
preserves the pinned private source literally, including that existing choice;
it does not assert stock binary equality or erase possible signed-zero/FMA
differences between those earlier implementations.
Every helper before main, including `computeGeoBiome`, selector/weight rules and
the native material formula, remains byte exact. The original four-slot loop is
reduced to its literal body with `slot=StageSlot`. Brightness remains before
weight multiplication. Selected slots are processed **0,1,2,3**, including
empty and repeated SIDs. Residual weight remains1 minus existing later channels;
negative weights are preserved, without sorting, deduplication or normalization.

Carry contains eight live FP32 fields:

| Plane | RGBA32F values |
| --- | --- |
|0|albedo RGB and native gloss precursor A|
|1|native accumulated normal RGB and absorbance|

Native `BiomeOutput.normal.a` is dead after componentwise accumulation: the tail
uses only `.rgb`. It is deliberately omitted from carry; no component mixing
depends on it. Stage0 initializes all live values to zero. Later stages Load
the immediately previous set by integer SV_Position, without sampling/filtering.
Empty stages copy the live values. No divide, normal normalization, TBN, wetness
or material writer is applied in a partial stage. No additive blend is used.

Two full sets alternate as RT and SRV: stage0→set0,1→set1,2→set0,3→set1.
The tail reads set1, applies the original `makeWet`, geometric-normal TBN and
writers exactly once. Only the two final packed-material writer substitutions
from the reviewed raw-RGB path are used: direct RGB and removal of zero-metal
overwrite. Reversing those two substitutions reconstructs the original tail
byte for byte. The original native vertex source, raw40B layout and15 distortion
slots are unchanged. Reflected native float CB fields/offsets remain unchanged.

Additional bindings are explicit: PS b1 contains `uint StageSlot; uint3 padding;`
(16B); t8 is previous albedo and t9 previous normal/absorbance. Native b0/t0..7
retain their original ABI. The tail has no live b1 dependency. Future source
banking can change only the active original SID row's four role-index vectors,
keeping original selector codes, weights, row numbering and VS distortion basis.
Each controlled pass renders the complete same fixed-cell viewport; this does
not prove derivative safety for future per-cell scissor batches.

## Ownership, state and budget

`StagedMaterial.h` owns the two ping-pong sets and slot CB. Init checks the90MiB
control and192MiB viewer payload caps before allocation. It retains no source,
shader, CPU image, game pointer or callback. On HRESULT failure it clears every
partial resource. Clear resets accounting and permits reInit. Accessors borrow
uninitialized resources; this owner does not publish completed image frames.
External held/bound COM views and in-flight retired resources remain the caller's
accounting responsibility. The interface requires serialized owning-render-thread
use; it provides no worker or concurrency discipline.

The standalone `Proof::Run` is a test sequencer, not a production renderer. It
saves graphics state plus native modified pixel slots and added b1/t8/t9, unbinds
carry/output read aliases across all six stages, disables predication for every
clear/upload/draw and restores state in reverse scope order. Each pass unbinds
t8/t9 before switching targets and binds the opposite previous set only after
OM switches away from it. Depth is cleared between passes; blending is disabled.
Its fixed geometry is four actual65² chunks, with the same viewport/derivatives
in all stages. Source/native CB fixtures are unchanged, without runtime pointers.

| Controlled GPU payload | Bytes |
| --- | ---: |
|Two complete two-plane carry sets|4,194,304|
|Extra b1|16|
|Completed fixed two-SID source owner|83,885,956|
|Four native patch VBs|676,000|
|Shared strip IB|17,144|
|VS448/PS3856 CBs|4,304|
|RGBA32F/RGBA8/R32F/D32 proof output|1,835,008|
|Complete proof including sources|90,612,732|

One bounded readback adds at most1,048,576B, so the observed proof payload plus
readback is91,661,308B, below94,371,840B. Synthetic selector/blend controls add
68B while live. The proof checks complete source admission as well as carry cap+1.
Driver/runtime overhead is excluded; these numbers do not authorize allocating
several simultaneous source banks or raising a budget. A future production RGB16F
output would use524,288B less than this FP32 oracle target.

The sequencer uses20 fixed draws (four patches × five passes), bounded state
snapshots and no growing queue/cache. CPU source uploads are serial and bounded;
the resource owner retains no decoded source data. This is not a game FPS,
streaming-throughput or long-running leak measurement.

## Actual checks

Run bundled Python with VC10/SDK7.1, fxc and headless D3D11 WARP:

```
C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py
```

The runner authenticates the read-only borrowed closure before/after, reconstructs
only private local snapshots, compiles both shaders, checks reflected ABI and
runs actual resources/draws/readback. It never runs another component's tests or
generators in their original directories. The unchanged native fxc X3206 warning
comes from assigning a sampled float4 to the original float2 interior variable.

All nine existing actual raw-RGB controls compare directly with fresh original
prewrite RGBA32F bytes and frozen oracle bytes:3m/9m/1m, even64px/odd1px shifts,
wet, underwater and two HDR cases. **Every material float byte matches exactly;
max float error is0.** Native normal/alpha and radial bytes also match. The
original native prewrite's cross-camera drift is reproduced, not silently fixed.

Three explicitly synthetic selector/blend algebra controls retain the same real
two-SID material textures and native parameters: four live repeated slots with
positive residual, an empty middle slot and a negative residual. Their complete
three-plane outputs match the unchanged monolithic RGB oracle byte for byte.
They establish slot algebra, not four live distinct SIDs or material provenance.
Weights are `[39,67,95,0]/255`, `[57,0,0,0]/255`, `[217,0,0,0]/255`; selector codes
are `[2,4,18,36]`, `[2,0,4,0]`, `[2,4,2,4]`. The last residual is `1−3×217/255`.

Fourteen actual allocation/device-health failure cutpoints check cleanup, COM,
context and retry. Ten compiled negative controls must fail actual assertions:
lost carry albedo, normalize per slot, wet per slot, clamp residual, deduplicate
SIDs, reverse slot order, omit final wetness, omit predication disable, read/write
the same ping-pong set, and remove allocation cleanup. Compiler failures are not
accepted as evidence. The predication observer alternates distinct3m/9m outputs,
priming the opposite frame first to prevent stale-output false positives.

Native arbitrary inputs remain outside this bounded control: finite inputs alone
do not prove finite native wetness/material output. No clamp, different formula,
fullworld source bank, runtime snapshot, GPU completion/publication guarantee,
lighting, models, water or installed feature is supplied by this shader proof.
