# Native vertex cliff distortion: local candidate

Full `build.ps1` completed with exit0. Candidate `build/SatelliteCaptureProbe.dll`
is343552bytes, SHA256
`b25ef9829079bb42f828ac434acf6583c2b0a8dfb641e6ac7af6824b46087f23`.
Native imports remain156. Full log: `build/geography-cliff-build.log`, SHA256
`dd4f93e106ce5cb5d0d1f5d0532624dd3a0bc4f89f3de1524424f5323ccebd5c`.
Root and independent peer review are CLEAR for this scoped integration. Peer
fresh runs passed144 effects cases/5457353 checks with zero observed height,
gradient or ULP difference; all seven effects mutants failed. Peer also ran the
full geographic suite (72 completion,2892537 upload,65546 selection,90 roles,
14 combined mutants), production DXBC reflection, existing cache56/Terrain77/
RTV92/worker67 checks and source contracts. Root independently reran the144-case
suite/seven mutants and exact ABI check, and confirmed the candidate hash.
Peer generic import auditing found291 named imports across seven modules with
zero missing; the156 count above is the distinct native-import check.

Root installed this exact343552-byte b25 candidate after the user closed Kenshi.
Installation evidence is the game's `_codex_work/global-satellite-20261102/
before-cliff-distortion/installation.json` (UTC2026-11-01T22:31:10.0207268Z).
INI stayed unchanged, GeographicBiomes=1 and request=0; no512 cliff runtime had
occurred when this report was updated. Installation is separate from review
and runtime acceptance. Existing BC-fixed runtime proof applies to its zero-cliff
baseline, not this candidate. This source task performed no installation.

## Production change

The existing default-off GeographicBiomes primary1,1/standalone512 path now owns
LEOPARD_GeographyVertex in existing geographic cache slot1 (fragment remains0).
The native VS source is pinned to installed terrain.hlsl SHA7bde0c5f. Its morph,
normal.zxy packing, world/clip transforms, horizontal/base UV, map coordinates
and triplanar blend statements are retained. Native matrix auto constants and
custom morph10 are explicitly bound after replacing the copied native VP.

Every vertex emits all15 fixed SID-indexed cliff heights in four float4 outputs,
TEXCOORD5..8. The first15 components use native signed cosine distortion at the
morphed mesh position; component16 is the undistorted base height. This avoids
choosing one coarse layout at vertices of a triangle that crosses selector cells.
The existing per-fragment SID/channel selector and residual weights are unchanged.

GeographyCliff.hlsl is shared by production and tests. Its fragment prepare step
computes all packed ddx/ddy fields before selection. The selected SID replaces
only cliff texCoords.z and grad.z; diffuse and normal SampleGrad use those same
coordinates/gradients. Shader Model4 rejects dynamically indexing input signature
arrays, so the selector uses15 fixed unrolled comparisons. Source sizes remain
25806ASCII bytes PS and below32KiB VS. Fade, wetness/water and other effects stay
at the previous explicit isolation settings; no fade restoration is included.

The same15 validated live records populate bounded15float4 VS data. The production
parameter helper preserves signed amplitude*.01f, reciprocal positive wavelength
and zero frequency otherwise, and rejects nonfinite derived results before GPU
allocation. It does not discard a nonzero amplitude when frequency is zero:
native cosine then produces the signed constant offset. No new native API call,
texture, SRV, cache capacity or allocation cap is introduced.

## Tests and reflection

`run_geography_effects_tests.ps1` compiles a reference extracted from the pinned
actual native main_vs and the exact owned vertex generator/helper. Reference
fragment derivatives are calculated independently from the production helper.
Both draw into five bounded R32G32B32A32 float targets on isolated WARP; readback
compares the fields before any texture/material/lighting approximation.

The initial zero-distortion helper was observed RED against the native reference.
The final run passed144 cases and5457353 checks: all15 source SIDs in native slot0,
representative native slots0..3, morph0/.37/1, negative/large mesh coordinates,
nonidentity world transform, triangles spanning coarse cells and changing-SID
pixel quads. A synthetic zero-frequency/nonzero signed-amplitude case supplements
the actual records. Fixture float operations reproduce scalar native IEEE float
rounding, including5*.01f rather than a double-rounded .05 literal. The actual
record snapshot is pinned to SHA511a8813.

Observed maximum height difference0, derivative difference0 and ULP difference0;
the declared height/gradient absolute tolerance is2e-6. Geometry, morphed world
normal, horizontal/base UV, map coordinates and padding were exact in these
fixtures. This establishes tested native-source field equivalence, not arbitrary
driver bit identity or final native material/render parity.

Seven generated-copy mutants each failed actual GPU comparison: abs amplitude,
2pi frequency, wrong SID component, lost morph, world-position phase substituted
for mesh phase, derivatives after SID selection and pixel-stage cosine. Production
files and the native reference are not altered by mutation tests.

Actual production DXBC reflection checks VS POSITION/NORMAL/BINORMAL input masks,
matching VS-output/PS-input TEXCOORD0..8 masks/registers and the15float4 constant
table at offset144 with16-byte stride/240-byte total. It checks native matrix/
morph/map offsets, eight exact PS SRVs t0..7 and samplers0/2/4, no VS resources,
and **all eight packed derivative instructions before the first selection branch**.
Source checks tie those fields to production bindings, cache slot1 and cleanup.

The fresh full build also passed72 completion/ownership/parameter checks,
2892537 DDS/upload checks,65546 geographic sampling cases,90 role pairs,
135194 previous mip-upload checks and the remaining existing suites. The seven
earlier geography mutants also fail as required (14 combined). Existing native
X3206 shader warnings, CaptureProbe C4018 and test Boundary C4512 remain; no new
cliff shader/compiler warning was introduced.

## Ownership, cost and remaining acceptance

One additional fixed VS identity fits the existing three-slot geographic cache;
expected stable private program count is six instead of the baseline five.
No cross-request smart pointer is retained. Noncached VP removal follows private
material/Terrain destruction; cached reuse requires exact owner/source/identity.
Failed construction/credit remains under existing bounded quarantine rules.

Per-request parameter storage adds240bytes; CPU source streaming and texture
budgets are unchanged. The VS computes15 cosine pairs per vertex and interpolates
three additional float4 registers compared with native four-biome packing. PS
selection adds bounded comparisons and derivatives; this GPU work needs real
timing. There is no additional whole-world traversal or per-frame configuration
polling, and no independent claim of FPS or leak freedom.

Root/peer independent review is complete; next are parent-controlled same512 fixture trials. Verify
finite GBuffer/Sun output, source-correct signed cliff placement, unchanged
geometry/depth seams and reprojection, repeat stability, program-count plateau,
texture/material balance, six RTV debts0, joined worker and restored scene/
workspace. Cancellation/load/shutdown and longer CPU/GPU/memory behavior remain
acceptance requirements. Native distance fade is deliberately outside this change.

## What the existing baseline files can prove about geometry

For baseline `512-geography-bc-fixed-second-20261102`, `_gbuffer_raw.bmp` is
**only global_gbuffer texture0**, saved by CaptureProbe.cpp. It is packed
material data, not depth or position. In normal RGB image-library channel order:
R is Y luminance; G is alternating Cg/Co chroma; B is metalness (Terrain writes0).
The original buffer alpha is gloss, but Save overwrites every BMP alpha withFF.
Raw disk pixel order is BGRA; a standard BMP reader handles that and the positive
height/bottom-up row order. Do not reinterpret raw disk byte0 as luminance or
use forced alpha as an occupancy mask.

An actual albedo comparison must execute SatelliteDecodeAlbedo from
GlobalSatelliteMap/shaders/GBufferDecode.hlsl (already WARP-tested), preserving
the original512 pixel grid and its four-neighbor edge filter,30/255 threshold
and same-parity Co/Cg selection. Formulas after chroma reconstruction are
R=Y+Co-Cg, G=Y+Cg, B=Y-Co-Cg, with both chroma values offset by-.5. Merely
comparing packed BMP bytes is a packed-data comparison. Even correctly decoded
RGB is material evidence, not geometry/occupancy evidence. Cliff displacement
of texture coordinates is expected to change albedo/gloss/material normals and
Sun output; these images should not be required to match the zero-cliff baseline.

Global_gbuffer texture1 contains material world-normal RGB=.5*n+.5; it is not
saved by the current capture path. These normals can change with cliff normal-map
sampling while the native geometric vertex normal remains unchanged. Texture2
is PF_FLOAT32_R, containing radial distance/farClip; native writeDepth writes
this value from length(worldPos-cameraPos). It is read as a512x512 float vector,
then consumed by InspectPositionPass. That pass also reads float4 view-space
positions/validity, but **neither float array is persisted** in the existing
baseline. No RGB/BMP decoder can recover the missing depth.

Position reconstruction at pixel center is X=((x+.5)/512-.5)*73728,
Y=(.5-(y+.5)/512)*73728, radial=depth*farClip and
Z=-sqrt(max(radial^2-X^2-Y^2,0)), with the production helper's finite/validity
and squared-distance rounding guards. These are view-space positions; use the
actual inverse view matrix/camera basis for world positions instead of guessing
an image-axis/world-Z flip. FarClip is120000 for this fixture.

The old baseline log reports262144 valid positions,0 invalid/nonfinite,
max NDC reprojection error7.43602868e-9 and max radial residual.00393046166m.
Compare the new trial's finite validity, reprojection/radial residual, geometry
stats and restoration, alongside the source/WARP proof of unchanged VS position/
normal/morph. These are useful scoped checks, but the reprojection test is a
within-capture self-consistency check, not a per-pixel old/new depth comparison.
Black-pixel count0 in buffer0 is only a material-color coverage proxy.

An exact future geometry comparison needs separately authorized instrumentation
that persists or hashes the **already-read** R32 depth array, validity mask and
float4 position array plus camera/projection metadata for both controlled modes.
At512 this is1MiB depth plus4MiB positions, with no new engine readback required.
Compare valid masks and per-pixel depth/position deltas before material shading.
The old three BMP files cannot retrospectively supply that baseline; do not claim
exact runtime geometry identity from their packed colors. No instrumentation or
new runtime request is added by this report.

## Parent same512 runtime trial, 2026-11-02

The user loaded the same SAT scene, paused and opened the map. Fresh process29328
accepted requests1512/1513 in3514/2433ms. Both loaded all15 geographic records and
the owned cliff VS. Texture2261595594B and material13509270B accounting returned
exactly before/after each request. Six cached program identities stabilized;
each request joined/closed one worker, released all six owned RTV references to
zero and restored scene0/workspaces1.

Each capture reported262144 valid positions, zero invalid/nonfinite positions,
maximum NDC reprojection error7.43602868e-9 and radial error.00393046166m.
All262144 linear HDR pixels were finite. Repeat packed-material and linear Sun
BMPs are pixel-identical; exposed previews differ by at most one channel level
in1440pixels. Root inspected the actual before/after material previews. The old
BMPs do not contain depth/positions, so this is not exact old/new geometry parity.

Private bytes before/after were8417980416/8664338432 and8439128064/8665370624.
These short readings and request times do not measure steady-state FPS or prove
absence of leaks. Shadow/fog/local-light/water/building/LOD/viewer acceptance is
still pending, and native distance fade remains disabled.

Evidence: game/_codex_work/global-satellite-20261101/rtv-lifetime-20261101/
`512-geography-cliff-{first,second}-20261102`, `geography-cliff-proof-20261102.json`,
`geography-cliff-preview-20261102.png` and the labelled-in-JSON before/after PNG.
Root updated the controller's scope from the exact cliff success marker so it
does not mislabel these captures as zero-distortion tests.
