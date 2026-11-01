# Private painted full-chain candidate — 2026-11-01

Local implementation and offline proof only. No installation, process/INI/file
request changes, renderer invocation or UI input is part of this work.

## Implemented contract

- Exact eight PNG identities, RGBA shape/alpha and tight mip0 layout are checked
  before either GPU array allocation; both CPU groups survive both uploads.
- Native createManual requests additional mip counts10/11 for1024/2048 arrays.
  Resource/SRV validation requires full chains11/12, four slices, UNORM
  PF12→DXGI87 or PF13→DXGI28, DEFAULT usage, exact SRV|RT binds and GENERATE_MIPS.
- The nonvirtual borrowed-SRV getter247A0 is generated from the hash-checked
  D3D11 ABI profile/code24. Actual texture vtable7CE08 is guarded before call.
  COM acquisition/release and GPU uploads use GpuMipUpload.h only. No native
  array writer is called on the mip0-only PNG images.
- Exact getCreator666D0 must return the current TextureManager. After successful
  GPU submission, notifyResourceLoaded293F30 runs once, then setToLoaded66740
  once. The real production completion seam keeps uncertainty latched on every
  post-submission failure/exception and blocks a second credit/retry.
- GPU texel budget is22,369,616+89,478,480=111,848,096bytes. Existing128MiB cap
  remains. Encoded/decoded aggregate caps also remain128MiB each. Native manager
  getSize telemetry is base-level accounting, not complete GPU footprint.

## Offline proof

Run build.ps1. It runs all prior private-probe suites plus:

- run_gpu_mip_tests.ps1:135194 real WARP checks, both accepted channel orders,
  all texels/channels of four distinct nonconstant8×8 layers and all four mips,
  bad inputs without any subresource changes,32 rewrites per format,16 uploads
  preserving pipeline state,32 real GPU object destruction-witness cycles.
- A generated helper copy omitting GenerateMips fails mip1 GPU readback:
  actual211, expected18. Production has no debug skip switch.
- run_painted_mip_tests.ps1:36 checks of the actual production native-completion
  seam and bounded plans. Success balances one accounting credit/debit;
  preloaded resources refuse; failed GPU submission/partial credit/throwing
  completion remain quarantined and cannot retry. A generated omit-notify
  policy copy fails the accounting assertion.
- Exact installed PE audits: Ogre30exports/31code windows, Terrain9/23,
  D3D11 zeroexports/13code windows. New native entry points are resolved through
  checked exports/RVAs rather than SDK virtual dispatch.

Logs remain in build/full-mip-build.log, build/painted-mip-production.log and
build/painted-mip-mutant.stderr.log, plus GlobalSatelliteMap/build/gpu-mip-tests.
Initial failing-stub logs are retained in the corresponding build folders.

## Reviewed local candidate

Independent review reported CLEAR and reran the complete build successfully.
Fresh evidence is in build/independent-full-mip-review.log:135194 WARP checks,
36 completion/budget checks, both required failing mutants and all existing
private-probe suites passed. The resulting SatelliteCaptureProbe.dll is
253440bytes, SHA256
`8adf86b10688163886023a9ca509c672f3119bd1ef8d001f172d03db2961d241`,
with156 verified native imports. An earlier equivalent local build had a
different PE timestamp/hash; this is the candidate produced by the independent
fresh build. Existing C4018 signed/unsigned and X3206 native shader truncation
warnings remain in unchanged code.

The helper and integration remain offline candidates. Review/build success
does not establish native ordinary-PNG pixel equivalence or gameplay memory
retention. No candidate installation, push, game/process/INI/request/UI action
was performed by this work.

## Expected new runtime messages

```
private painted full-mip preflight: levels=11/12 layers=4/4 gpuTexelBytes=111848096 cap=134217728; manager size is base-level accounting only
private painted full-mip submitted: kind=0 levels=11 DXGI=<28 or 87> gpuTexelBytes=22369616 creator=TextureManager notify=1 Loaded=1; native pixel equivalence pending
private painted full-mip submitted: kind=1 levels=12 DXGI=<28 or 87> gpuTexelBytes=89478480 creator=TextureManager notify=1 Loaded=1; native pixel equivalence pending
private painted arrays loaded:4pairs core2,2; RGBA full-chain; sourceBytes=45799579 decodedBytes=83886080 gpuTexelBytes=111848096
```

These prove only guarded submission/state. GenerateMips and UpdateSubresource
return void; these messages do not claim pixel equivalence or bounded whole
process/GPU retention.

## Required native ordinary-PNG versus array proof (planned, not run)

Do not hotpatch the running game. After root review and a separately coordinated
candidate installation/restart, add an explicit one-shot private comparison
request, separate from ordinary captures. The diagnostic must:

1. Own uniquely named ordinary2D PNG textures loaded through the installed
   native PNG path, using the identical hash-guarded files, gamma=false and
   matching mip counts/formats. Do not mutate/reload existing main textures,
   change global default mip counts or call global load/unload.
2. Reuse the exact guarded native SRV getter for those owned2D textures. Obtain
   resource/device/context through scoped COM and require matching descriptors
   and full mip chains. Run outside live workspace update/binding; check every
   identity before copy. Preserve all engine/pipeline state.
3. Compare one native2D texture against its array slice at a time, using bounded
   one-subresource staging resources and CopySubresourceRegion with zero array
   depth coordinates. Read mip0 and every lower mip, all RGBA channels including
   alpha, for all eight files. Report per-mip byte mismatches/max/mean errors and
   hashes, with format/channel order explicit. No rendered/desktop screenshots
   are needed. Keep staging/source storage below a separate checked budget.
4. Release comparison COM and all owned ordinary textures on all verified
   paths, then prove before/after ResourceManager counters and names balance.
   Quarantine one bounded set after uncertain native allocation/accounting;
   never retry credit or force destruction. Record texture getSize separately
   from explicit GPU texel totals.
5. Execute repeated standalone512 captures with the normal cleanup telemetry,
   resource/name/worker/RTV/cache counts, comparable idle process private bytes
   and GPU allocation measurements. Existing caps remain unchanged. Observe
   memory over time; a short WARP test cannot prove universal absence of leaks.

This comparison requires new guarded native-load lifetime instrumentation;
it is deliberately not guessed or invoked against the currently running game.
