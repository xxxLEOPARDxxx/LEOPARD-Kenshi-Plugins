# Optional HDR stage integration

The existing `Pipeline::Render(...)` signature and behavior remain intact:
water OFF returns S_FALSE before any device/input query, and the caller keeps
the original terrain route. Three production shader bytecodes and the pipeline's
3,408,256B private payload are unchanged. No main/caller or atlas-lights file
was edited here.

New per-call C++03 POD:

```cpp
struct HdrStage {
    void* context;
    HRESULT (*apply)(void*, const AtlasView::Cell&,
                     const AtlasView::Uniforms&,
                     ID3D11ShaderResourceView* hdrView,
                     ID3D11Texture2D* hdr);
};
```

Construct with `{&state, &Apply}` and pass its address only during the call.
Neither callback/context nor any callback resource is retained. A null context
is permitted for a stateless function; a nonnull stage with a null apply fails.
The view and texture are borrowed references to the same private256 one-mip
R16G16B16A16_FLOAT linear HDR resource. The stage reads HDR plus supplied cell/
uniforms and writes its result back into this resource, normally using its own
scratch+copy. It must preserve coverage alpha and apply no exposure/tone mapping.
It must not retain the borrowed HDR objects or write the caller display.

```cpp
HRESULT RenderWithStage(
    const AtlasView::Cell&, const AtlasView::Uniforms&,
    ID3D11ShaderResourceView* const terrainLights[3],
    ID3D11SamplerState* const terrainSamplers[3],
    const AtlasWater::Uniforms& water,
    ID3D11ShaderResourceView* const waterInputs[5],
    ID3D11SamplerState* const waterSamplers[5],
    ID3D11Texture2D* display, U64 callerOwnedBytes,
    const HdrStage* stage);

HRESULT RenderTerrainWithStage(
    const AtlasView::Cell&, const AtlasView::Uniforms&,
    ID3D11ShaderResourceView* const terrainLights[3],
    ID3D11SamplerState* const terrainSamplers[3],
    ID3D11Texture2D* display, U64 callerOwnedBytes,
    const HdrStage* stage);
```

Sequence: unexposed terrain HDR → optional stage → optional native water → one
native exposure/tone → final display copy. The terrain-only overload constructs
disabled water and passes no water resources. Water inputs/shape are validated
only when water is enabled. Null stage+water OFF is S_FALSE before all queries,
even before Init. Old Render always calls the null-stage path and retains its
own explicit OFF early return.

Stage HRESULT failure stops before water/tone/final display copy; the caller's
previous display stays intact even if the stage already modified private HDR.
The caller may fall back to its original terrain renderer. Successful S_FALSE
has normal HRESULT success semantics and continues composition. Device removal
during the final copy retains the existing device-loss limitation; this extension
does not promise recovery from arbitrary device loss.

Pipeline scopes compute shader/class instances, CB0, SRV0..7,samplers0..4,UAV0
and predication around apply, without resetting hidden counters. A callback must
restore any other state it touches and report its failures through HRESULT;
the trusted component callback is not an arbitrary plug-in execution boundary.
Existing terrain/water/tone scopes preserve their state and all-stage display
read aliases. No extra pipeline resource, shader, worker or allocation is added.

Budget: `callerOwnedBytes` at **both Init and Render** includes all resources
outside this pipeline, including the real local-light stage's526,416B. The API
cannot infer allocations hidden behind a void context; root must supply the
complete actual ledger. Do not add that amount into PipelinePrivateBytes or
count it twice. Current144-window/default-renderer189,006,000B +stage526,416B
+pipeline3,408,256B =192,940,672B, leaving8,385,920B under shared192MiB.
Additional coexisting resources must also be in callerOwnedBytes. No cap raise.

Fresh verification uses the approved **real additive GPU fixture**, not a claim
of native point-light parity. The fixture deliberately leaves compute bindings
dirty, so the production stage scope is exercised. Its private HDR scratch is
separate from pipeline allocations; the test reserves the full future526,416B
in the caller ledger. The existing unmodified native terrain, opaque water and
native exposure oracles validate composition order, including submerged/dry
pixels. Null stage matches old water pixels exactly. Stage error after private
HDR writes preserves display. False predicate, full D3D context, native append
counter7 and plus1 budget rejection are checked.

Commands (run in atlas-water): `./run_pipeline_tests.ps1`, then bundled Python
`generate_pipeline.py --stage-mutants`. The existing runner stays unchanged.
Results:26 policy,262,326 WARP checks,max native raw HDR delta0,max composed
native reference delta1UNORM; all10 existing functional mutants and10 new stage
mutants fail as expected. New negatives cover missing/double stage,water-only
admission,water input validation while OFF,failure display copy/propagation,
compute/predicate restoration and stage after water/exposure.

Production HDR/water/tone DXBC hashes remain
`51013b959677e9e6a59efd43f25b4e966e5bcf17b30c9a65afbb7947e7ac0314`,
`9c5b41477ec29d1663dc58fb1242e09ac80f1b7209d90ce1f3055328be13b5d3`,
`d4d78e7aceb2ee776dbe5afab1011c2b7275072f88abeddb863a48f8868a4f77`.
Test-only StageAdd is generated under `_generated/pipeline` and is never placed
in the production shader header. Root still owns real local-light source/frame
mapping, caller integration, budget declaration and live acceptance. No game,
main DLL build/install, API calls to the engine, UI or INI changes were performed.

Independent read-only review by `container_restore_audit`: **CLEAR** on
2026-11-02. A fresh run confirmed 26 policy tests, 262,326 WARP checks, all
10 existing and all 10 stage compiled mutants. Review checked null/OFF paths,
exactly-once ordering, failure before display copy, scoped compute/predicate
restoration without hidden counter resets, the unchanged private ledger and
all three unchanged production shader binaries. Header SHA remained
`94d2de52f1d710793d28783dd97a086c926b1e3fa47a5b8cc21b0a231b835047`.
This is offline integration approval; actual native local-light integration,
coordinate mapping and game acceptance remain separate.
