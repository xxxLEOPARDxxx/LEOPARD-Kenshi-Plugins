# Optional native geographic fade: local candidate

Full build exit0: `build/SatelliteCaptureProbe.dll`,345088bytes, SHA256
`27f4d8a56530baeeb53f70a6070419e9337da25f5c2ca33f977c2e00825bb539`.
Native imports remain156. Full evidence: `build/geography-native-fade-build.log`,
SHA256 `d851113e6f74172983badcf691782f34f4a8b945e1dc0984894bffc75e56ab5a`.
Independent source/native-contract review is CLEAR. Root independently reran
the43 helper/1084 route checks,420 WARP cases and five mutants, then the native
ABI audit. The peer also verified all291 dependency imports across7 modules
(156 native imports unchanged) and the four unchanged shader/source hashes.
The previous native-cliff trials remain the live baseline; this new fade mode
has only offline execution evidence so far.

After verifying Kenshi was closed, root installed this exact345088-byte DLL.
Diagnostic INI bytes were preserved, Request remained0 and the absent fade key
uses default0. Backup and installation proof:
game/_codex_work/global-satellite-20261102/before-native-fade/installation.json.
The user has been asked to load the paused SAT scene/map for off/on/off captures.
No enabled-fade game request has been made at this checkpoint.

## Production scope

`[Probe] GeographicNativeFade=0` is optional/default-off and read once at request
start. It travels through the actual CaptureProbe -> TerrainExperiment ->
GeographyReference.Begin -> Parameters route. It is used only by the existing
geographic standalone512 primary1,1 material. Nongeographic requests retain their
previous route even if the raw fade flag is1. The table is reset per request;
off/on/off cannot retain an enabled row. No installed INI is written by building.

Only row7 of each of15 validated live biome parameter records changes when
enabled. Typed const idata/fdata find requires `ground colour` and `fade distance`;
missing fields refuse before pool allocation without inserting into GameData.
Disabled mode zeros the row and does not inspect the additional fields.
NativeFadeParameters preserves unsigned ARGB RGB byte order and scalar float
1/255 conversion. Positive distance is inverted; zero/negative values remain
literal. Nonfinite input or derived coefficient refuses without changing output.

There is no production shader change: existing computeGeoBiome already contains
the native fade operations. The accepted source/DXBC hashes remain exact:

| Artifact | SHA256 |
|---|---|
|GeographyVertex.hlsl|6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9|
|GeographyTerrain.hlsl|3cde7fa65e27b76092bb14fd4636172b3f979ac4db830e677c800433ea0d1d98|
|GeographyVertex.cso|dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238|
|GeographyTerrain.cso|57f7d3ddfabcde2fae7ee39715c5f03c3ac4ebaaefa921c61f381a7cb5b9a2b4|

Eight PS SRVs, no VS SRVs,225 PS float4 rows,15 VS distortion records, native
matrix/morph bindings, source-size limits and six fixed cached programs remain.
No texture, readback, native API, process action, ownership route or allocation
cap is added. Runtime work adds30 field lookups and15 tiny conversions once per
enabled request; disabled rows return immediately. There is no per-frame INI poll
or world traversal. GPU instructions/resources are unchanged; parameter values
change the image and do not establish measured FPS or general leak freedom.

## Verification

- 43 production helper checks cover all15 pinned ARGB/distance rows bit-exactly,
 channel boundary/ignored input alpha, zero/negative distance, nonfinite source,
 reciprocal overflow, missing typed fields, no insertion and off/on/off reset.
- 1084 route checks compile the **actual source expressions** for request reads,
 Terrain geographic routing, Geography parameter dispatch and typed row7 adapter
 against a native boundary stub. They verify default0, all15 enabled rows,
 off/on/off reset, nongeographic isolation and refusal before allocation.
- 420 real WARP cases/1293156 checks compare the pinned native computeBiome body
 with the exact generated production computeGeoBiome body. Controlled tiny float
 RGBA role arrays represent native six-layer inputs and the current24/22/46 pool
 indexes. All15 real ground colours/fade distances/brightness values are covered,
 with off, zero/onset/partial/full factors and road/cliff mixes. Float albedo RGBA,
 normal RGBA and absorbance are read before GBuffer encoding/lighting. Maximum
 observed absolute difference0; declared tolerance2e-6.
- The prior zero row was observed RED in actual GPU output (nativeR.155384898
 versus ownedR.0869184211 at SID0/partial fade), then GREEN through the actual
 production helper. The helper itself was also observed RED before conversion.
- Five generated-copy mutants fail GPU comparison: swapped ARGB RGB channels,
 raw distance instead of reciprocal, fading albedo alpha, incorrectly retaining
 normal alpha, and moving fade after road/cliff composition. No production file
 or native reference is changed by mutation tests.
- Full existing suites pass, including72 completion checks,2892537 upload checks,
 65546 geographic samples,90 role pairs,144 cliff cases/5457353 checks and the14
 prior geographic/cliff mutants. Actual render ABI/DXBC/loader/source checks pass.
 Existing native/test compiler warnings remain; no new fade warning was added.

Commands: `run_geography_fade_tests.ps1`, `test_geography_fade_source.py`, or the
full `build.ps1`. Fixtures pin native terrainfp4 SHA050de26a and active record
snapshot SHA511a8813. The shader hash test prevents accidentally calling a
modified local shader a data-only change.

## Native order and acceptance limits

Fade affects albedo RGB only, before road RGB and cliff RGBA. Normal fade affects
the complete float4 toward(.5,.5,1,1), including its alpha, before road/cliff
normals. Absorbance is unchanged by fade. Brightness follows each biome result;
cross-biome weighting, makeWet, TBN/GBuffer and Sun retain their existing order.
These tests establish the tested conversion/function-output contract, not full
native render, DDS mip, whole-map or postprocess parity.

Literal native distance remains length(worldPos-cameraPos). At the actual16000m
orbital camera, base detail fades strongly: current native distances range4000..
25000m, with full fade at5200..32500m.102104 of262144 lateral pixel centers already
lie beyond32500m, so their base-ground stage fully fades for every current row
regardless of terrain height. Roads/cliffs compose later and can retain detail.
No hidden distance cap, fake camera or altered native field is used to avoid it.

Next: a separately parent-controlled off/on/off512 trial
with explicit mode labels and lighting metadata. Root's capture controller must
distinguish `native-fade=1` from the generic native-cliff/zero-fade scope before
the enabled trial. Require finite position/HDR, resource balance, six stable
programs, six RTV debts0, joined worker and restored scene/workspace. Albedo,
material normals and Sun intentionally differ when enabled. Existing BMPs still
provide no per-pixel depth/occupancy baseline; no exact old geometry claim follows.
