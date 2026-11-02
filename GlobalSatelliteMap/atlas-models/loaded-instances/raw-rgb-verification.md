# Isolated full-RGB material output, 2026-11-03

Scope: only `atlas-models/loaded-instances`, based on committed `6c4d9cc`. No Main, installed files, GST decoder, native source adapter, MCM, INI, game/process/UI or publication changes. This primitive is not an available player feature. Independent review is separate from this implementer evidence.

## Production change

`InitRawRgb` explicitly selects256² RGBA16F native material RGB/gloss. Native normal RGBA8/alpha, radial R32F and private D32 are unchanged. `Init` still selects the original packed RGBA8. Init commits the mode only after every allocation succeeds; failure and Clear release the owned objects/reset the mode. `OwnedGpuBytes` reports1048848B packed /1310992B RGB /0 uninitialized. Both initialization and Render check the extra262144B under the unchanged201326592B cap before allocation/output mutation. Concurrent/retained owners remain the caller's accounting responsibility.

`generate.py` still emits the same four native bytecodes and adds exactly two PS variants. The pinned original objects source differs in only two final writer sites: direct RGB replaces packed `writeAlbedo`, and the black-metalness output writer is omitted. Native black-metalness sampling/absorption, dust, wetness, gloss, Alpha48 normal-alpha clip/translucency, normal encoding and radial depth stay literal. Reversing those two substitutions yields the exact original bytes. Original `NativeInstances.h`, `InstanceAdmission.h`, default tests and native compilation entrypoint hashes remain frozen. `NativeShaders`, Snapshot and `CanDecode` semantics are unchanged.

## Verification

Command: bundled Python `run_tests.py`, then `run_tests.py --skip-mutants` after adding the final source-release/finite-input/format assertions. VC10 x64 `/O2 /MT`, actual D3D11 WARP; native adapter additionally compiles against the real SDK with `/O2 /GL`, without invocation.

- 17 unchanged policy +8 production-admission checks.
- 4,718,646 unchanged native-source WARP checks; max packed byte delta0 and radial delta0.
- 13,762,792 RGB WARP checks /10 material-geometry scenarios /3 isolated dark, colored or border pixels.
- 116 actual extracted native controls.
- 41 original compiled functional mutants and17 new RGB mutants rejected by runtime assertions; compile failure is not counted as a kill.
- Four default DXBC hashes match frozen verification.json exactly;21 asset/source pins and47 native code windows unchanged. Exactly two RGB PS variants; original VS bytecodes reused.

The independent RGB oracle cuts the pinned original source immediately before GBuffer writers and observes native computed diffuse/gloss on RGBA32F. Production RGB PS on the same float32 target differs by at most2e-6. Actual FP16 output is checked within one half-storage quantum plus2e-6; the observed maximum absolute error is0.000487685204. This states the storage tolerance rather than assuming round-to-nearest. Native normal/translucency bytes and radial float outputs match exactly. Covered and clipped samples, varying texture mips/native sampler, all3 declarations, rotated/transformed native geometry, dust/wetness and HDR above1 are exercised. Single-sample dark/colored silhouettes and border samples retain full RGB without packed-neighbor reconstruction.

Raw-mode checks include inclusive cap and cap+1 at Init/Render, OFF/stale/NaN/SO/write/source aliases before mutation, same-device canonical input/output identity, all-stage output read restoration, PS0..6/CB/sampler/IA state, predicate suppression/restoration, hidden OM append count, repeated output stability, source COM retention after caller release, repeat packed/RGB initialization and partial shader-init cleanup/reference balance. Native geometry/material/source admission remains shared.

RGB mutants: omitted Init budget, omitted Render budget, wrong target format, lost mode, wrong ownership charge, omitted partial cleanup, inherited native predicate, lost PS state, lost source alias check, lost dust, lost wetness, packed albedo writer, metalness clobber, diffuse-alpha clip, alpha replaced by coverage, lost gloss, reversed dual blend. All reached a runtime FAIL.

## Performance, lifetime and limits

The production Render path has one mode-dependent budget choice, no new resource allocations, worker, file access, hashing, native lookup or scan. Existing per-packet draw/borrowed-source bounds are unchanged. New resources are Init-owned and reused; Clear/failure releases the same COM graph. Tests establish the exercised reference balance/state paths, not absence of every possible runtime leak. State/driver overhead is outside the declared D3D payload. No CPU/FPS/game-memory claims are made.

The native formula is preserved, including its limitations. Finite source admission does not imply finite output for arbitrary finite values: native wetness=-0.7 is singular; extreme finite inputs may overflow or exceed FP16 range. There are no added clamps or substitute formula. Legitimate finite/HDR cases and NaN input refusal are tested. A later accepted-coverage stage must mask/refuse nonfinite output and prove a scoped canonical native weather/material domain.

Before Main integration, prove actual active resolution of all21 assets, not only base-data hashes, and the required lighting closure. No verifiedSources assumption was broadened here. Private raw RGB still needs native Sun/HDR composition, accepted mask/height/occlusion, current-light and source epochs, bounded scheduling/complete ledger and later authorized runtime acceptance. Alpha48 CanDecode remains false. Nothing in this report establishes resident native DXBC equality or a complete loaded-model overlay.

Exact frozen hashes and shader manifest are in `raw-rgb-verification.json`; baseline verification.md/json are retained.
