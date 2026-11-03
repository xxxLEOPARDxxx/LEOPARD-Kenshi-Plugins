# Independent WorldPixels review

Verdict: **CLEAR for the isolated CPU helper; no P1/P2 finding.**

Reviewed author manifest SHA256
`8bdad5e8c4d7ef997f04da8fb45a044a3e51930943541baa9acf6665e242ee60`,
header SHA256
`8a0413d58f28728dd0d83e6a3d8a48cbbc9affc23c74d11e9d73453c32d9cc22`.
The original sources, dependency/fixture/evidence pins and54 mirrored files
remained unchanged. All execution/output was confined to this research mirror;
the original game TIFF and frozen input planes were read only. No production,
game, INI, UI, install or publication changes were made. The concurrently authored
WorldStream integration is expressly outside this review.

## Code findings

Build withdraws the previous publication before each enabled attempt. It checks
prior-slab input aliases before freeing that slab and validates pointer extents,
exact plane sizes and pairwise overlap before reading. A complete source/scope/
frame/CPU-budget admission precedes allocation. There is one bounded slab, with
no per-pixel allocation or retained raw/window pointer. Every row and final
publication checks cancellation; partial or failed scans release the slab.

All four FP16 channels are checked by exponent bits, including gloss. Finite
negative/HDR/subnormal/signed-zero representations are copied untouched. Normal
alpha must be literal zero; clear/near-zero stored normals refuse. Radial must be
finite in(0,1) and within the fixed5cm distance policy for the authenticated
triangle position. It is never stored as world height or used to invert height.

Height uses native18m source vertices and the native anti-diagonal. The source
Window must have complete coverage, the exact64-bit epoch, canonical fullmap SHA
and physical file identity. Output height is the piecewise planar float result.
Acquire independently checks the exact current scope/frame and complete saved
source identity and clears the returned View on refusal. It keeps no authority
over whether the caller's current labels came from an actual current job.

The output View separates material, normal and typed float world-height planes.
Its pointers borrow the Accepted owner and require serialized owner access and
owner lifetime through downstream use. The helper alone does not mark an atlas
tile/index/mip ready, authenticate image origin, certify GPU upload or provide an
atomic writer/publication step. These remain consumer integration obligations.

## Fresh verification

Bundled Python ran the unchanged `run_tests.py` from the mirror. VC10/O2 and/Od
each passed317 production controls across14 actual WARP-input windows and
1,900,544 pixels. All output material/normal bytes were compared; every height
was independently checked against emitted40B geometry packets and actual strip
indices by barycentrics within1mm. Maximum measured radial discrepancy remained
0.002003347100981m, below the fixed0.05m policy. The tolerance is an acceptance
choice with refusal outside its domain, not a universal hardware parity claim.

Ten real-event cancellation cutpoints, two allocation failures, fourteen
functional mutants and two cancellation mutants passed their expected green/red
controls. Mutant compilation errors/crashes were not counted as successful kills.

`additional_tests.py` then passed110 independent controls each under/O2 and/Od:

* All63,488 finite FP16 encodings in each of four channels were accepted and
  preserved byte-for-byte at both256 and512; invalid half encodings were excluded
  from this positive input, with original negative tests covering NaN/Inf.
* All three input-plane types pointing at a one-byte overlap with the prior
  output slab refused without reading freed data. Adjacent non-overlapping
  planes in one valid caller slab were accepted.
* Inclusive64MiB payload boundaries succeeded at both target sizes; one extra
  byte and maximum-U64 otherCpu refused and cleared publication. A genuinely
  invalid closed-event wait cancelled conservatively.
* Integer near-zero normal code length²19 was admitted, length²11 refused;
  alpha1 refused in addition to the original alpha255 case.
* For positive and negative radial errors, the nearest representable float
  inside the independently reconstructed5cm boundary succeeded; the immediately
  adjacent float outside refused. The oracle used actual indexed geometry.
* Closing the authenticated Source preserved its owned Window snapshot, as
  documented. A second genuinely authenticated TIFF Window with a different
  high64 epoch invalidated Build and Acquire. Worker-style scope labels were
  accepted without an artificial current-thread restriction on this CPU scan.

The machine-readable review manifest pins scripts, logs and unchanged original
closure. This review did not regenerate or overwrite author fixtures.

## Cost and limits

Time is O(side²), with bounded square-root/triangle work per pixel. The single
owned payload is16×side² bytes (4MiB maximum), plus the owner object; old/new
slabs do not coexist. Caller raw buffers, retained height windows, other owners
and pending results must remain in otherCpu. GPU accounting is unchanged.
No file/native/D3D call, growing cache, queue or thread is added by Accepted;
only cancellation-handle polling is performed. OS/CRT overhead, current-source
provenance, live scheduling, end-to-end latency/FPS and unrestricted pointer or
concurrent-access misuse are not proven by these offline tests. No source
correction is requested before the parent's separately reviewed integration.
