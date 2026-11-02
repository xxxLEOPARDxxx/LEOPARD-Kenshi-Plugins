# Regional catalogs and adapter verification,2026-11-02

Scope: fixed64 DATA regions; final16 logical8192² base cells unchanged. Native
dynamic relighting/channel storage remains the root/atlas-view workstream.
No installation, game/INI/UI action, commit, push or release-payload edits here.

## Implemented boundaries

- `generate_catalog.py` verifies pinned snapshot/active identities, actual219
 DDS and128 painted PNG files, blend/fullmap and native executable/module/shader
 hashes before writing deterministic `catalog.json`/`RegionCatalogData.h`.
 The independent raw fixture is generated from the pinned snapshot and blend
 directly, without catalog-builder helpers.
- `RegionCatalog.h` is VC10/C++03, no Ogre/D3D/engine calls or heap allocations.
 GLOBAL57 identities/raw ordered5slots are distinct from LOCAL15 shader rows.
16 coarse descriptors per region retain tree hashes/byte lengths/root masks.
 Existing8×8 RGBA selector has4×4 active cells and explicit zero padding.
 Bank slots remain24+22/46, with deterministic initialized duplicate padding.
- `PrivateGeography::BeginRegion(root,owner,const Region&,bool nativeFade)`
 accepts only a reference to the immutable generated table. Original `Begin`
 still chooses exact primary1,1 data. `Input` maps92 slots to existing uploader
 and shared native completion/credit/quarantine. All actual native alias paths
 are separately validated even when upload uses another path with identical
 hash. No new shader/texture/material cache or retained engine-object pointers.
- `ParametersRegion` reads only required raw SID identities (at most10 in this
 dataset, including unused source slots). Shared `FillRecord` preserves native
 keys/order/fade/distortion. All unused parameters/distortion rows are zeroed.
 Existing memory-read ABI/code windows remain; selected16 cells must match all
 ordered native pointers and exact tree hashes before GPU allocation.
- `PrivatePaintedReference::BeginPrimary` selects its parent's four distinct
 overlay/colour pairs. Old `Begin` still selects original1,1 identities.
 `BindPrimaryBase` uses the primary73728m lower corner; geographic binding
 refuses mismatched primary textures. Encoded+decoded source payload now also
 uses the existing128MiB `AddBytes` guard as one combined pre-allocation value.
- Production `GeographyCodes(worldXZ,geoSelectorRegion)` takes origin/active
 extent. Both production entry paths bind it: legacy(-73728,-73728,8,8),
 regional(worldOriginX,worldOriginZ,4,4). Historical one-argument fixture entry
 delegates to exact legacy bounds. Native VS,15 packed distortion records,
225 PS rows, signed residuals/prebranch derivatives and eight SRVs remain.

Authorized existing files changed: `PrivateGeography.h`,
`PrivatePaintedReference.h`, `GeographySampling.hlsl`,
`embed_geography_shader.py`, `generate_geography_fade.py`,
`test_geography_fade_source.py`, `test_geography_mutations.py`.
The last three only adapt the patch-center mutation needle and planned PS hash
pins; old fixture values and exact native-VS hash pins were not changed.

## Evidence

Fresh `run_tests.ps1` exit0, log `build/suite.log`:

| Actual check | Result |
|---|---|
|Generator behavior/refusal/determinism|7 tests pass|
|C++03 data/coordinates/raw identities/native selector|3282564 checks;1048576 fine leaves;63488 coarse boundaries|
|Five unused source-slot cases|all explicitly preserved; five individual drop mutants fail|
|Extracted production BeginRegion/Input/BeginPrimary/uniform bodies|12157 checks, all64 regions/all16 primaries + unchanged legacy data|
|Extracted actual ParametersRegion/FillRecord|200094 checks; all57 records through64 regions, off/on/off; zero rest; missing unused SID/slot/tree failures|
|Real D3D11 WARP production-shared selector|64 regions + legacy8×8;1050432 points;8403536 checks|
|Generated-copy catalog/file-hash mutants|14 rejected|
|Real WARP explicit-origin/scale/bounds/channel/residual mutants|6 rejected, including fixed patch-center and poisoned unused texels|
|Production DXBC|newfloat4@3840;225 rows@240 unchanged; native VS byte hash unchanged|

RED evidence: `build/red-generator.log`(six intended missing-generator failures),
`build/red-cpp.log`(valid catalog refused by not-yet-implemented validator),
`build/red-adapters.log`(missing regional API), `build/red-gpu.log`(missing
two-argument production function). No production debug/bypass flags were added.
One overly large test-only VC10 initialization unit was replaced with bounded
binary record input; `test_parameters.py` now compiles/runs in about1s.

Full parent probe build succeeded before root's subsequent Atlas orchestration
edits: `build/full-build.log`. It includes exact native ABI/import156,72 native
completion/ownership checks,2892537 DDS/WARP checks,65546 legacy selector points,
90 role cases,144 actual native-source VS cases/5457353 checks with zero
height/gradient/ULP differences and7 mutants;420 native-fade GPU cases/1293156
checks with zero maxAbs and5 mutants; painted completion36; existing worker,
RTV, source/loader and ownership suites pass. The expected missing-notify
mutant prints `FAIL` before the runner confirms that mutant was rejected.
Existing native HLSL truncation and older C++ signed/assignment warnings remain;
new standalone C++ tests compile `/W4 /WX` without warnings.

That pre-orchestration DLL was527360B/SHA
`fbde673c2ecdd453d3a1e415bd8bcd8941bfd5e17402fad072b5bc79cf92585a`;
it is NOT the final root candidate and predates the final combined-payload guard.
Root's fresh integrated build/independent review is authoritative before install.

Shader source SHA:
`f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1`;
PS DXBC SHA:
`4b3b71ccb9c75b79791b9cf3bef5f858ba9c53151a9cfc0d65cd2e7d113fdebe`.
VS source and DXBC remain
`6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9` /
`dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238`.

Frozen regional artifact hashes:

- Source manifest:
 `69b7a25921afa65cecbeef898bd2ca80eca0eeaaf16237790c8b3477337a4023`.
- Generated C++ data:
 `bd85da4b86a5e13bafd3ab9793c9a5f7ac3047488e57c813e430ade5d40f426f`.
- Generated JSON:
 `6e274871f3a0d894db352306a5fb6b1f004e685a75897cf8e897fc47e53bb454`.
- PrivateGeography:
 `f59edcff80aeaef1e606a430a060351519ff706026b5a31e837358bda198ca4e`.
- PrivatePaintedReference:
 `bdf1d59ba3c7eb5c42b2f115f02f198e663f1c2c9050bdb565b9c997e29b0e11`.

## Performance, ownership and limits

Generated immutable structs90536B on x64; conservative structs plus every string
literal counted without pooling197778B. No runtime catalog allocation, file
scanning, global engine lookup or per-frame work is introduced. Metadata
validation runs only at explicit region/primary begin. Maximum10 actual GameData
lookups per region;57 is an index domain, not57 lookups or a raised shader cap.
Generator streams hashes at1MiB, never decodes DDS or holds all texture files;
the observed complete generation is about4s on this machine. This is offline
IO/CPU cost, not game-frame/FPS evidence. Test fixtures are5.6MiB plus compact
record data and bounded WARP buffers, not a map cache.

GPU source bank budget385874496B and other texture budget117440756B unchanged;
bank caps128MiB, combined private1GiB, source payload128MiB unchanged. Bank
padding increases referenced-source repetition, not resident array dimensions.
Native private allocation/transient/driver/main-scene memory remains outside
any claim of absolute adapter/process peak. Painted worst encoded+decoded payload
133449487B fits128MiB; codec/allocator scratch is not an absolute-process bound.

Existing CompleteBank uncertainty remains latched across partial upload/credit;
the immutable region pointer is cleared only after successful owned cleanup.
No cleanup retry, owner eviction, main resource mutation or replacement resource
set is added. Root alone owns Terrain/worker/RTV orchestration and output worker.

Limits: CPU/source/WARP proof does not establish all-world native runtime
images, actual2048 region geometry/LOD seams, painted lower-mip parity, final
GST2 correctness, torch/day-night rendering, FPS or absence of every native
allocation retention. Those require root's bounded live controls and independent
integration review. The64 data tables must never be presented as64 final map
tiles or as an already completed atlas.
