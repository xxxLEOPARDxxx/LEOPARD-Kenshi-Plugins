# Optional native geographic fade: next scoped plan

Status: the subsequent root instruction authorized this exact data-only step;
it is implemented and verified offline, pending independent review and a separate
installed trial. See geography-native-fade-verification.md. The accepted cliff
shader sources/DXBC stay unchanged. This step changes only the existing15 biome
fade rows and an optional per-request private diagnostic flag.

## Baseline and exact scope

The native-cliff b25 candidate passed PID29328 requests1512/1513 in3514/2433ms.
Root evidence `geography-cliff-proof-20261102.json` records matching raw material
GBuffer and linear Sun repeat images, exposed max delta1 in1440pixels,262144 valid
positions/zero invalid or nonfinite, unchanged aggregate reprojection residuals,
texture/material balance, six stable cached programs, six RTV debts0 and a joined
worker. The old per-pixel depth baseline is absent, so this does not establish
exact old/new runtime geometry identity. No city/water/local-light/grid/viewer
work is required for this step.

The current generated computeGeoBiome already retains the exact native fade
operations. Its per-biome row7 (`parameters + biomeIndex*60 + 28`) is zero.
The smallest change supplies that row when explicitly requested; it does not
rewrite shader math, alter the VS or add a new GPU program.

Proposed `[Probe] GeographicNativeFade=0` defaults off and is read once when a
request begins, alongside GeographicBiomes. It applies only to this existing
geographic standalone512 primary1,1 path. Outside that path it has no effect on
normal captures. An enabled request may populate row7; every subsequent Begin
first zeros the table, so off/on/off cannot retain old fade values. No INI polling
per frame, automatic request, global material change or live GameData mutation.

## Proven native parameter contract

The hashed RE_Kenshi executable504b362c has these exact native paths, already
preserved in research/global-satellite-20261031/biome-native-20261101/
terrain-material.asm:

- A14873..A148AA reads `ground colour` from GameData's integer map at+178 and
  calls Ogre::ColourValue::setAsARGB. SDK GameData.idata is the matching typed
  map; use nonmutating find on the already validated15 records.
- A148E6..A14948 reads `fade distance` from fdata+1B8. Positive values become
  1.0f/distance; zero and negative values are retained. Do not replace the latter
  with a guessed fallback. All15 current records have positive distances.
- Hashed OgreMain's setAsARGB8D460 extracts RGB bytes R=(argb>>16)&255,
  G=(argb>>8)&255, B=argb&255 and multiplies each by native float1/255,
  bits3B808081 at6A9A60. Its input alpha is subsequently replaced with the fade
  coefficient. This can be reproduced as pure bounded C++, with no new API call.

Use the integer bit pattern as unsigned ARGB, preserving signed-storage bits;
do not decode ABGR/RGBA, apply sRGB conversion, multiply ground colour by1.2 or
preserve its original alpha. Perform scalar float operations matching native
rounding. Reject nonfinite source/derived fade coefficients before allocating;
when enabling, missing expected ground-colour/fade keys refuse rather than
inserting fields into GameData or silently inventing values. Disabled mode keeps
the existing zero row and does not require the additional fields.

Current fixed-SID rows, read from the pinned active record snapshot511a8813:

| SID | Biome | Ground ARGB | Fade metres | Native float coefficient |
|---|---|---|---:|---:|
|12613-Newwworld.mod|Swampfeeder Canyon|005C3D30|15300|6.535947614e-5|
|14390-Newwworld.mod|Blister Cliffs|00593D2D|12000|8.333333244e-5|
|17948-Newwworld.mod|Blister Sands|00FF8A61|4000|.0002500000119|
|18940-Newwworld.mod|Tekic Enclaves|00E0C7A3|14000|7.142857066e-5|
|2742-Newwworld.mod|Mafic Enclaves|006B5945|10000|9.999999747e-5|
|2743-Newwworld.mod|Skinner's Roam|005C4738|10200|9.803921421e-5|
|2804-Newwworld.mod|Tarsands|00291C17|9300|.000107526881|
|2806-Newwworld.mod|Artery|00A17D66|25000|3.999999899e-5|
|2822-Newwworld.mod|Akakus|0068594A|17400|5.747126488e-5|
|3395-Newwworld.mod|Floodwastes|005C4D40|6000|.0001666666649|
|48690-Newwworld.mod|Skinner's Crawl|005C4738|10200|9.803921421e-5|
|5532-Newwworld.mod|Robot Graveyard|00E0C7A3|7000|.0001428571413|
|5570-Newwworld.mod|Blister|00593D2D|12000|8.333333244e-5|
|5611-Newwworld.mod|Okran's Pride|0026210C|12300|8.130081551e-5|
|5746-Newwworld.mod|Tarsands Town|00292621|8700|.0001149425298|

These are expected fixture facts, not values to substitute for validated live
record lookup. Runtime uses the same actual GameData records as existing scales,
brightness and distortion, preserving current SID/asset/tree validation.

## Shader order and 16 km camera consequence

Pinned installed terrainfp4.hlsl SHA050de26a defines:

```text
fadeout = saturate(length(worldPos-cameraPos)*textureFade.a - .3)
base -> grass -> slope -> dirt
fade RGB toward ground colour
road RGB -> cliff RGBA
```

Albedo fade changes RGB only; alpha/gloss is unaffected by the fade operation.
The native normal calculation separately fades the complete float4 normal
toward(.5,.5,1,1), including normal alpha, before road and cliff normals. Absorbance
is not faded. Brightness applies to each completed biome RGB, then SID-weighted
biome RGBA/normal/absorbance blending, makeWet, TBN/GBuffer and Sun remain in their
existing native order. Do not move fade after roads, cliffs, brightness or the
cross-biome sum. Keep all existing negative residual-weight behavior unchanged.

The existing camera is at altitude16000m and uses its actual Euclidean distance,
not a fictitious near-ground or fixed16km scalar distance. Native fade starts at
.3*fadeDistance and reaches1 at1.3*fadeDistance:1200..7500m onset and5200..32500m
completion across these records. Ground base detail can therefore fade strongly.
On the512/73728m fixture,102104 of262144 pixel centers (38.9496%) have lateral
distance alone >=32500m; these fully fade the base-ground stage for every row
regardless of terrain height. Other pixels can also fully fade. Roads/cliffs
are composed afterward, so do not describe this as erasing all final detail.

This is the literal native orbital-camera consequence, not a proposed visual
improvement or proof of map suitability. Do not secretly cap distance or raise
fade distances to retain detail. A different orbital policy would be a separate
decision; the flag permits reviewing the native result while default0 preserves
the accepted cliff reference.

## Small implementation boundary, after review

1. Add one pure parameter conversion in existing GeographyPolicy.h and call it
   from PrivateGeography::Parameters only for enabled row7. Test the actual
   production helper with all15 ARGB/distance pairs, channel-boundary patterns,
   zero/negative/nonfinite/tiny distances and unchanged output on refusal.
2. Read/route the default0 flag once through CaptureProbe/TerrainExperiment to
   GeographyReference.Begin/Parameters. Keep the flag request-local; verify
   off/on/off table contents and nongeographic isolation through the real route.
   Log exact enabled/disabled scope once per request. Root may update its capture
   controller scope text separately; do not modify installed INI during build.
3. Extend the existing geographic test runner with a small native-source fade
   fixture, then run the existing full build/import/reflection suite and independent
   review. No comparison engine, asynchronous readback owner or new framework is
   needed. Production shader sources/DXBC, eight PS SRVs, VS constants, six cached
   program identities and all CPU/per-bank/global/geometry caps stay unchanged.

## Required focused WARP proof

Use the actual pinned native computeBiome source as reference and the production
generated computeGeoBiome body as the candidate. Bind the production helper's
row7, all15 role/scales/brightness records, controlled nonconstant float RGBA
diffuse/normal textures and native samplers. Tiny fixture arrays can represent
the existing24/22/46 pool indexes; do not reload385MB of DDS for a fade unit test.
Native six-layer references must use the same corresponding role texels.

Render/read **float** albedo RGBA, normal RGBA and absorbance before encoding or
lighting. Exercise off and factors0/intermediate/1, each record's native onset/
completion, road/cliff weights0/intermediate/1, different diffuse/normal alpha,
and representative mixed-SID brightness/residual cases. Keep native reference
derivatives uniform; the candidate retains its already tested shared gradients.
Report finite outputs and measured absolute/ULP tolerances, not BMP similarity.

Observe RED/GREEN with the original all-zero row against an enabled nonzero-fade
case. Generated-copy mutants must demonstrate sensitivity to RGB channel swap,
raw distance instead of reciprocal, fading albedo alpha, preserving normal alpha
incorrectly, and moving fade after road/cliff composition. Reference code remains
unaltered. This is direct parameter/native-body verification, not a fake policy
that production never calls and not whole-game lighting equivalence.

## Later controlled acceptance, not performed by this plan

After review, parent may run the same paused-map512 fixture off/on/off within one
process with stable lighting/exposure metadata. Off reproduces the accepted cliff
reference; on is explicitly labelled literal native fade at the actual camera.
Check position validity/reprojection, finite HDR, texture/material balance, six
cached-program plateau, six RTV debts0, joined worker and scene/workspace restore.
Albedo/material normals/Sun change intentionally; forced BMP alpha is not evidence
about gloss/normal alpha or geometry. Existing nonpersisted depth limits remain.
Report request timing/memory honestly; no FPS/leak-free/whole-atlas claim follows.
