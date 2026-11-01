# Private painted terrain reference implementation plan

Goal: replace neutral colour/overlay in the proven single-biome512 reference
with the four actual native painted pairs covering one73728m primary cell.
This remains a material-input experiment, not the final16×8192 atlas.

Architecture: exact allocation-free painted-grid policy; one private material
with two four-layer painted arrays. A private fragment shader retains the native
terrain algorithm but selects painted layer/UV per absolute world pixel.
Native callback research proved coarse patches can cross painted boundaries;
center-based material selection cannot represent those patches correctly.
Read files into bounded owned buffers, preflight exact size/hash/dimensions,
retain RGBA and native18m UV borders. No shared ZoneMap/overlay creation.

Tech: VS2010 C++/Ogre Terrain/D3D11, native installed PNGs, Python input audit.
Evidence/spec: docs/verification/2026-11-01-satellite-biome-reference.md and
native ZoneMapOverlay constructor A136E0, Terrain setMaterialCallbacks13C30.

## Constraints / review focus

- Default off, standalone512 only; same scene/worker/geometry/deadline caps.
- Worldorigin−147456, painted core36864, border18, UVspan36900, indices0..7.
- Exact primarycell1,1 input fixture first; all four pairs present before GPU IO.
- Preserve alpha and mip accounting; total base RGBA80MiB plus bounded overhead.
- No decode of unverified bytes; avoid holding all PNG/CPU images unnecessarily.
- No material callbacks or raw SDK Terrain fields. Native callback int is NOT
  LOD/depth (always0); depth0–2 blocks can span multiple painted tiles. Use the
  pixel shader and world-space SampleGrad to keep derivatives continuous.
- Initial reference arrays have one mip only,80MiB base GPU data. This verifies
  mapping/colour/alpha; native minification with mip chains remains subsequent work.
- Explicit PS textures t0..t4 and samplers s0/s2/s4; verify actual DXBC reflection.
- Destroy private Terrain before materials/textures; check retained
  borrows and quarantine on uncertain ownership. Resource counters repeat-balanced.
- User at PC: inspect process first; request close/load if needed, no focus changes.

## Task1: painted grid policy

- [x] Add production tests of native bounds/UV,16primary coverage, borders,
  maxedge/negative/nonfinite values, tiny positive boxes, crossing/coarsepatches.
- [x] Observe failure before adding `GlobalSatelliteMap/PaintedMapPolicy.h`.
- [x] Implement fixed4slot no-allocation cover/patch selection and verify66 checks,
  including RED/GREEN for tiny central-seam coordinates erased by origin addition.

## Task2: actual input fixture and bounded private materials

- [x] Audit and record exact eightPNG inputs of primary1,1, preserve RGBA.
- [x] Verify native callback export ABI, native thread/lifetime and patch boxes;
  reject per-patch material approach because coarse boundaries are not guaranteed.
- [x] Implement pre-created private painted textures/materials only after all
  inputs/budgets pass; use profile guards for native API additions.
- [x] Production tests, HLSL/native tests/imports, independent lifecycle/perf review.
- [x] Install while game closed; two512 file requests with actual image inspection,
  no nonfinite pixels, balanced counters/worker/RTV cleanup; report image limits.
- [x] Commit/push tested stage. Biome mixing and full atlas remain subsequent work.
