# Reusable staged world terrain raster

`WorldTerrainRaster.h` is a VC10/D3D11 resource owner and ordered render API. It
uses the unchanged pinned native-derived vertex shader, four ordered partial
contributions and one final wetness/TBN/raw-RGB tail. It preserves three outputs:
RGBA16F RGB/gloss, RGBA8 native normal and R32F native radial distance. A D32 depth
target is private. No packed-material decoder, neighboring-pixel chroma lookup,
additive blending or per-pass wetness is introduced.

## Integration contract

All methods run on the same serialized render thread/immediate context. Keep the
`WorldBank::Sources` owner alive throughout an active frame. This helper owns its
D3D references and immutable GPU geometry; it retains no caller height/material
buffer. Its raw bank pointer is an identity check, not an owned bank reference.

1. `Init(device, side, shaders, samples, grid, ledger)` accepts side256 or512 and
   1–16 globally64-aligned65² source chunks. Supply the complete two-source-sample
   normal halo. It validates all three bytecodes and builds immutable geometry.
2. Prepare the exact planner stage0 bank. `BeginFrame(true, frame, plan, material,
   bank, ledger)` copies all900 material and60 distortion floats, the plan and
   full64 source/material/frame generations. OFF returns S_FALSE before queries.
3. Call `RenderStage(slot, bank, ledger)` for slots0,1,2,3 in that exact order.
   Upload each next nonempty SID first; trailing empty slots reuse the last real
   bank. Bank allocation identity, device, revision, source and original role
   row are checked at every phase. Same-slot replacement invalidates the lease.
4. `Finish(bank, ledger)` requires all four passes and the unchanged final lease.
   Only `Ready(currentFrame)` / `Output(currentFrame, plane)` publish a healthy
   complete exact frame key. `Texture` and carry accessors are allocation-only
   diagnostics. Errors, exceptions and `Cancel` withdraw publication; partially
   overwritten pixels are never exposed as a completed frame.

Frame admission requires finite values, spacing1–9m, full owned-geometry coverage
inside one9216m coarse cell, the same source/material region and conservative
camera/far bounds. Canonical native material/weather domain is still a caller
prerequisite: arbitrary finite inputs do not guarantee finite native results.
Source samples remain18m; 1–3m output improves material/render sampling only.

`Ready` means successful healthy command submission, not GPU completion. Caller
owns completion fences, current native source authority, epoch invalidation and
acceptance of actual uploaded bytes. The present source profile is the frozen
base snapshot; this owner does not establish live mod winner/weather provenance.
Completed output no longer depends on subsequent source-bank contents.

## Bounds and state

The shared ledger cap is201,326,592B. Carry is side²×64B (fourRGBA32F textures),
outputs/depth side²×20B. Mesh/CB/index bytes are patches×169,000+21,464. With16
patches, raster payload is8,230,488B at256 or24,745,560B at512. Adding the existing
83,885,956B source bank gives92,116,444B or108,631,516B respectively. Readback,
other owners, retained views, pending and retired/in-flight banks remain charged
by the caller; overflow or omitted required categories refuse. Driver overhead
is outside payload accounting. No per-phase caller C++ heap allocation is used.

Writable aliases and stream output refuse before mutation. Read aliases of all
owned resources are detached across all six stages and restored. Graphics state,
extra b1/t8/t9 bindings, predicate and hidden UAV counters are restored on normal
and exceptional exits. Predication is disabled for updates/clears/draws. Init
failure releases partial owners and supports retry. `Clear` releases context and
resources; externally retained references/in-flight work remain caller debt.
No queues, native pointers, world scans, file I/O or worker threads are added.

## Verification

Run with the bundled Python:

```
C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py
```

The runner verifies immutable borrowed pins before/after, prepares only private
fixtures, compiles VC10 and executes actual WARP. Nine256 controls compare all
three planes byte-for-byte with frozen raw-RGB output. Five512 controls compare
against a separately bound monolithic two-SID oracle using actual DDS/TIFF inputs.
The actual four-SID cell uses staged source uploads and tests coverage, finiteness
and state; its >192MiB full-bank monolithic oracle is deliberately not allocated.
Even/odd shifted controls preserve inherited native floating-point drift rather
than claiming universal overlap equality. Fault tests cover42 Init HRESULT and42
C++ exception cutpoints, phase failures/source mutation/cancel and ten functional
mutants plus cleanup mutation. The read-restore observer explicitly reseeds live
aliases immediately before the first observed healthy pass.

`dependencies.json` records absolute local dependencies, including historical
snapshot data, original game assets, generated borrowed bytecodes and reference
planes. These assets are not redistributed. Reproduce prerequisite generators
before running in a clean checkout; missing/mismatched inputs refuse. `freeze.py`
records own sources, generated fixtures and test logs; `freeze.py --verify`
checks the final manifest. This is offline helper evidence, not game FPS,
whole-world seam, current-source or universal lifetime proof.
