# Geographic cliff distortion and fade: scoped implementation plan

Status: Task1 and Task2 are implemented offline under the subsequent root
instruction, pending independent review and a separately controlled installed
trial. Task3 remains acceptance planning; native fade is still zero. Use the
existing execution/TDD/verification techniques without skill-only ceremonies.

**Goal:** Restore native per-biome vertex cliff distortion over the existing
15-SID primary1,1 material while preserving geometry and fragment selection.
Native distance fade is a separate, smaller follow-up with its distance behavior
made explicit. No grid expansion, viewer, city/model/water/light work belongs here.

**Architecture:** One owned geographic VS emits15 fixed SID-indexed cliff
heights in four float4 interpolators. The geographic PS selects a height and its
precomputed derivatives by SID after its existing per-fragment selector. The
original morph, transform, normal, overlay and biome-weight operations survive.
No new texture bank, engine hook, native ABI call or automatic cap increase.

**Stack/spec:** VC10/SDK7.1 C++, actual installed HLSL, Shader Model4, isolated
D3D11 WARP. Read geography-verification.md and the native geography contract in
`D:/DEV/Kenshi/research/global-satellite-20261031/geography-material-20261101/`.

## Current evidence and limits

Installed BC-fixed candidate47d2fd8d/334336B, sourcef0de24a, passed PID4348
tokens1510/1511:15SIDs/92DDS/three actual nine-level banks, raw GBuffer and
linear Sun BMPs pixel-identical across repeats, exposed preview max delta1 in
1522pixels. Texture/material manager bytes balance, five fixed cached programs
stabilize, six owned RTV debts reach0 and native worker joins. Elapsed3299/2834ms
is whole request timing, not GPU frame cost. Short repeats do not prove FPS or
general leak freedom. The current512 geometry peaks at175961856 reported bytes;
fixed textures503315252 plus64MiB output allowance give746385972 reported/budgeted
bytes. Native allocation transients and driver overhead remain outside that sum.

The reference is still zero distortion/fade, dry, one region. Its deliberately
nine-level pooled DDS chain differs from main Terrain's observed seven levels.
Root live artifacts are in the game's `_codex_work/global-satellite-20261101/
rtv-lifetime-20261101/512-geography-bc-fixed-{first,second}-20261102` directories.

`audit_geography_boundaries.py` is a rerunnable read-only input audit. It pins
snapshot SHA5cb085ba and actual blend PNG SHAdb1b6723, checks every manifest
selector/tree identity, and compares exact integer SID weight dictionaries:

- 65536 fine-cell centers: coarse-root decoding equals native fine-leaf decoding.
- 3584 neighboring coarse-boundary pairs: the same bilinear RGBA midpoint decodes
 identically through either coarse layout; zero decoder discontinuities.
- Actual source SID weights have145 coarse-adjacent and4483 internal pairs with
 L1 change>1, so the input itself contains sharp changes.

Run with the bundled Python: `python audit_geography_boundaries.py`; optional
`--snapshot`, `--game`, `--output` select inputs/evidence. This proves these input
weights at mip0 centers/midpoints, not complete material RGB, higher-mip seams,
asset orientation or native-render equivalence. Keep that distinction in reports.

## Exact native effect contract

Installed `data/materials/deferred/terrain.hlsl` SHA256
`7bde0c5f6aa81d542b5c25925b28508a937d5086effeb959a9b0c5b96f11eb07`:
lines37-44 morph position.y from position.w, morph the packed normal, set
position.w=1, transform clip/world position, and transform normalize(normal.zxy).
Lines51-65 derive horizontal UV from **mesh position.xz**/5000 and cliff height
`h0=1-position.y/5000`, then calculate each native biome's height at the vertex:

```text
frequency = wavelength>0 ? 1/wavelength : 0
amplitude = signedAmplitude * .01
height = h0 + (cos(position.x*frequency)+cos(position.z*frequency))*amplitude
```

Use the same morphed mesh position before worldMatrix, not worldPos, a patch
center, a pixel position or a substituted phase origin. Do not introduce2pi,
abs(amplitude), clamp, renormalization or a pixel-stage cosine. Native VS packs
four selected heights into texCoords.z/TexV.xyz; PS changes only the cliff-height
coordinate for each biome. Horizontal coordinates and triplanar blend stay native.
Floodwastes has negative amplitude-.04 after scaling; this is an essential case.

Actual native Terrain material construction atA153CE..A1545C proves reciprocal
positive wavelength/zero otherwise and signed amplitude*.01.15 snapshot records
have seven distinct nonzero frequency/amplitude pairs, but do not bake seven
slots or make a new deduplication scheme part of the initial implementation.
The fixed15 outputs handle all live values for the already validated15 records.

Installed terrainfp4.hlsl SHA050de26a defines native fade as
`saturate(length(worldPos-cameraPos)*textureFade.a-.3)`. NativeA14873..A14948
uses ground-colour ARGB RGB and reciprocal positive fade distance (nonpositive
values remain nonpositive). Ogre::ColourValue::setAsARGB at8D460 extracts RGB
bytes and multiplies by the same native1/255 float; no sRGB conversion. Native
fade affects base/grass/slope/dirt RGB and normal before road/cliff composition;
alpha, absorbance and the later brightness multiplication retain their order.

All15 current fade distances are positive4000..25000m. Fade starts at.3 times
that distance and reaches1 at1.3 times it. Current orbital altitude is16000m,
not a near-ground native gameplay camera: fade restoration can strongly reduce
fine ground detail, especially toward the edges. Do not silently replace the
distance or clamp it to preserve detail. Any orbital distance policy is a separate
explicit choice; this step can first test the literal native calculation.

## Proposed file/interface boundary

- Create `GeographyCliff.hlsl`: pure shared per-vertex height and SID pack/unpack
 helpers used by production and GPU tests. Logical output index is compactSID-1,
 with SID order exactly GeoBiomes. Pack heights0..14 into TEXCOORD5..8 float4s;
 the unused16th component is deterministically h0.
- Modify `embed_geography_shader.py`: embed an owned geographic VS derived from
 the pinned native main_vs; generate the matching PS signature and shared helpers.
 Preserve the native statement sequence before/after the replaced distortion
 block. Fail the generator if source hashes/anchors or source-size bounds differ.
- Modify `PrivateGeography.h`:15 bounded(float frequency,float amplitude) records,
 one owned vertexProgram, existing FixedProgramCache slot1 (PS remains slot0),
 exact named matrix/morph bindings and full request cleanup/telemetry accounting.
- Modify `CaptureProbe.cpp` only for the additional owned program identity/count.
- Add `geography_effects_warp.cpp`, its HLSL fixtures, reflection/mutation checks;
 integrate into `run_geography_tests.ps1` and existing build. No installs in tests.

VS/PS semantics0..4 remain the native normal/world position/base UV/map UV/cliff
blend values. Base texCoords.z stays h0. TEXCOORD5..8 carry all15 SID heights;
they must be produced for every vertex, because a triangle can cross selector
cells and its vertices cannot select one shared four-biome layout correctly.

In the PS, compute ddx/ddy of base coordinates and **each packed height float4**
before the selected-biome loop or divergent branch. Inside the loop, select
height, heightDx and heightDy by compactSID-1; replace only texCoords.z and the
matching grad.z components before computeGeoBiome. Apply the same coordinates
and gradients to diffuse and normal SampleGrad. Computing ddx of a previously
selected height across a changing SID boundary is incorrect.

Uniform VS data is at most15float4 (240bytes); the existing PS225float4 table,
eight SRVs and samplers0/2/4 remain unchanged. Keep32KiB source bounds, three
available cache slots, CPU128MiB, per-bank128MiB, global1GiB and existing geometry
limits. Using slot1 adds one fixed VS identity to the measured stable private
program count (expected five -> six); it does not increase the cache capacity.

## Task1: production-shared GPU regression before integration

- [x] Add RED fixtures for all15 native source records, including negative/zero
 amplitudes, different frequencies, large/negative coordinates, morph0/interior/1
 and nonidentity world transform. Compare an owned packed-height VS against the
 **actual native VS source** with each SID bound to native slot0 (and representative
 nonzero native slots). Read interpolated height and ddx/ddy from float targets.
- [x] Use triangles large enough for cosine-at-pixel to differ from interpolated
 vertex cosine. Include a triangle crossing multiple selector cells, and a pixel
 quad whose adjacent fragments select different SIDs. Compare each selected
 derivative to that SID's independently rendered native reference field.
- [x] Record finite output and maximum absolute/ULP differences. Require identical
 geometry position, morphed world normal, horizontal/base UV and overlay/biome UV;
 preserve native arithmetic/interpolation order. Do not claim arbitrary bit parity
 without observed readback evidence and a declared numerical tolerance.
- [x] Observe failures for generated-copy mutants: abs amplitude,2pi factor,
 pixel-stage cosine, wrong SID/float4 component, lost morph, worldPos substituted
 for mesh phase and derivatives taken after SID selection. Production files
 remain unmodified by mutation tests.
- [x] Reflect actual DXBC: VS inputs retain POSITION/NORMAL/BINORMAL; VS outputs
 and PS inputs match TEXCOORD0..8 with correct masks;15height slots/padding and
 constant-buffer stride/count match binding. Assert eight exact PS SRVs and
 samplers0/2/4; no VS SRVs or accidental extra texture resources.

## Task2: bounded owned-program integration

- [x] Bind LEOPARD_GeographyVertex only on this existing opt-in512 geographic
 material. Use existing program creation and named auto-constant APIs: native
 worldviewproj/world matrices and custom morph10 must be restored explicitly
 when replacing the copied native VP. Keep the native normal packing unchanged.
- [x] Populate all15 distortion records from the same validated live GameData
 used for PS parameters; finite/identity checks precede binding. Keep the fixed
 compact SID table, full blend map and current per-fragment weighting unchanged.
- [x] Reuse existing cache slot1 only with source/owner identity verification;
 destroy noncached VP after private material/Terrain, retain no cross-request
 smart pointers, and include its identity in telemetry/cleanup-failure handling.
- [ ] Full fresh build, all old/new suites and independent source review before
 a parent-controlled installed trial. No cap changes or new native ABI boundary.

## Task3: same512 live acceptance, then separate fade

- [ ] Capture the same primary1,1 fixture twice after a separately authorized
 install. Compare geometry/depth/reprojection and world normals against the
 zero-distortion baseline: geometric positions/seams must remain unchanged.
 Changed material normals on cliff textures are expected; require finite GBuffer
 and Sun values, source-correct signed cliff placement and repeat stability.
- [ ] Verify fixed program count stabilizes at six, texture/material credits
 balance, all six RTV debts reach0, worker joins and scene/workspace restore.
 Exercise failure/cancel/load/shutdown without retrying uncertain native credit.
- [ ] Measure request time and geometry/host/GPU behavior on this same fixture;
 computing15 cosine pairs per vertex adds bounded VS ALU and interpolation cost
 but is not free. Do not infer FPS or leak freedom from two repeat captures.
- [ ] Only afterward, independently restore row7's exact ground RGB and native
 reciprocal fade data with pure byte conversion matching8D460 (no new engine
 call needed). Test all15 records, fade0/intermediate/1, RGB/normal/order/alpha
 behavior against pinned native computeBiome. A separate controlled capture
 should make the literal16000m orbital-camera fade consequence reviewable.

Until these checks pass, keep zero fade explicit and do not label the output
native effect parity. No geographic expansion is a dependency of this plan.
