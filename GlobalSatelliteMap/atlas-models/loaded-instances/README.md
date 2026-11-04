# Registered instanced building snapshot

C++03/VC10 helper used by `GsmLoadedTownState` at the serialized render boundary. It uses already existing native geometry/materials; it does not load models, activate zones, clone native objects or substitute boxes. Cached whole-town LODs remain a separate fallback; this snapshot does not write terrain capture files.

Approved contract: `D:/DEV/Kenshi/research/gsm-loaded-render-20261102/instance-first-contract.md`. The saved actual Hub evidence has 131 eligible records /34batches /5selected material families before current in-use/manual-visibility/readiness admission. They include two plaster dual-texture families and Wall4/Gate4/market alpha families. This is not all loaded Hub geometry; current runtime admission is untested. The old ordinary windmill helper is unchanged.

## Interfaces

Include `NativeInstances.h` only in the serialized render-thread adapter. `LoadedInstances::Snapshot` exposes:

```
HRESULT Read(bool enabled,Town* alreadyResolvedTown,Ogre::SceneManager*,
 LONG generation,volatile LONG*liveGeneration,bool verifiedSources,
 ID3D11Device*,U64 otherBorrowedBuffers=0,U64 otherBorrowedTextures=0,
 unsigned kindMask=3,const Packet* previous=NULL);
const Packet& Geometry() const;
const Stats& Values() const;
const char* LastError() const;
bool Unchanged() const;
void Clear();
```

The optional previous packet is borrowed only during the synchronous Read and
must remain alive and unchanged until Read returns. The default API still
captures a fresh packet. With a previous packet, the entire native Begin,
membership/material/pose inspection and End validation still run. Only after
End can exact copied descriptors return `S_FALSE` with `Unchanged()==true`,
before creating copied samplers or repeating `Packet::Adopt`. That result owns
no new geometry: the caller must keep its previous packet. An ordinary empty
`S_FALSE` has `Unchanged()==false` and cannot preserve an unloaded old town.
Generation changes and failures clear the receipt.

The comparison checks every retained draw field, all copied surface values,
sampler descriptors, origin, generation and device. Fresh resource tokens are
compared with exact interfaces owned by the old packet and never dereferenced
by the reuse gate. Strong COM references prevent address reuse during this
synchronous comparison; a different SRV interface conservatively requires full
admission, including a different view of the same texture. Existing total
borrow limits are checked before reuse. The caller includes the old packet in
`otherBorrowedBuffers/Textures`, as `GsmLoadedTownState` does.

Focused tests are `tests/loaded-town/run_reuse.py`, `test_state_reuse.py`,
`test_reuse_mutants.py`, the actual Publish recorder in `budget-tests/run.py`,
and native Snapshot extraction in `test_native_extracted.py`. They establish
descriptor/lifecycle behavior; in-game CPU/FPS comparisons remain necessary.

`kindMask=1` admits only Dual43 plaster; `2` admits only Alpha48; default3 admits both raw native paths. Filtering precedes batch/material capture and is reported separately from unsupported records. `verifiedSources` means the caller validated all21 generated `InstanceAssetPins.h` entries **and their actual active resolution**, including the selected building textures and shader include closure. The existing unrelated model/terrain verification Boolean is not sufficient. The helper additionally compares cached native program source/variant/compile flags and all actual texture roles. A basename or base-data file hash alone does not prove a mod did not shadow a native resource.

`Snapshot::Read` retains no Town/Building/Entity/Batch/material/parameter/texture native pointer. Only VB/IB/SRV/sampler COM references, copied raw matrices, bounds, scalar material data, origin and generation survive. Native source owners/physical records, headers, membership, matrix, material full identity/program/parameters/weather/SRVs/sampler, geometry and current world are rechecked before publishing. A captured generation is not a live source revision: reuse requires the caller's source/transform/material/weather refresh and invalidation contract. Do not read native state per window. Source selection is one supplied Town, with no whole-world geometry discovery.

Packet methods are `Count`, `Generation`, `SourceSceneOrigin`, `RawSceneWorld(i)`, `LocalBounds(i)`, `BorrowedBuffers`, `BorrowedTextures`, `RequiresNativeLighting(i)`, `CanDecode`, and `Clear`. Geometry references expire on Read/Clear/destruction. Do not memcpy Packet or keep a dangling reference. Low-level `Packet::Adopt(device,raw,count,origin,generation,otherBuffers,otherTextures)` is the production COM admission used by the adapter and WARP fixtures; `RawDraw::verifiedBlack` is a trusted source-adapter precondition, not a GPU black-image detector.

`Raster::Init(device,ShaderSet,callerOwnedGpu)` allocates one private256 native GBuffer+D32 and shared instance/constant buffers. `NativeShaders()` supplies the four generated, unmodified bytecode variants. `Raster::Render(enabled,packet,AtlasModels::View,callerOwnedGpu)` performs no native discovery or allocation; Texture/Output(0..2) expose packed albedo/metal/gloss RGBA8, native normal/alpha RGBA8 and radial R32F. `Clear` releases its owned D3D objects. Packet adoption creates no layouts/states; Raster Init owns the three supported declarations and three cull states once. ShaderSet/RawDraw are lower-level trusted caller inputs; the native adapter constructs them from its fixed source profile.

An explicit private RGB mode is available by including `InstanceRawRgbShaders.h` and calling `Raster::InitRawRgb(device,RawRgbShaders(),callerOwnedGpu)`. Target0 is then RGBA16F containing native pre-GBuffer linear RGB and native gloss; target1 remains native RGBA8 normal/translucency and target2 remains radial R32F. Private D32 and all geometry, sampling, clipping and depth behavior are shared. `IsRawRgb()` identifies a successfully initialized mode and `OwnedGpuBytes()` returns its declared payload (zero after Clear/failed initialization). Reinitializing an already initialized raster refuses; use Clear first. Use a separate owner for each simultaneously live packed/RGB raster and charge both.

The generator adds exactly two PS43/48 variants, replacing only the final packed albedo write with direct RGB and omitting the final metalness overwrite. Verified black metalness is still sampled and used by the literal native wetness formula. It is not stored over RGB. The original four bytecodes, `NativeShaders`, native Snapshot/source admission and `CanDecode` remain unchanged. RGB mode does not grant permission to assert `verifiedSources=true`, send Alpha48 through GST, or interpret native normal alpha as coverage. It is a material-output primitive, with no Sun/HDR compositor, accepted-coverage/height mask, cache changes, UI, scheduling or Main caller.

RGB mode checks the same finite input preconditions as the native helper. This is not a guarantee that arbitrary finite inputs yield finite output: the original wetness expression has a singularity at wetness=-0.7, and extreme finite inputs can overflow arithmetic or FP16 storage. No clamps, alternate material formula or unknown-input fallback were added. A later accepted-coverage stage must refuse/mask nonfinite output and establish the supported native weather/material domain. The legitimate finite/HDR fixtures prove their own cases only. Active resolution of all21 source resources remains a prerequisite to any Main integration; base-file hashes alone are insufficient.

## Native semantics and composition

Two native pairs: Object_VP_83 + Object_FP_43 (COLOURING,INSTANCED /DX11,DUST,DUAL_TEXTURE,COLOURING), and Object_VP_80 + Object_FP_48 (INSTANCED /DX11,DUST,TRANSPARENCY). Original native HLSL is compiled without source changes. Native ABGR enum11 maps to DXGI28 in the actual backend switch; its code/table is pinned. Three observed declarations are supported: coloured stride64 and plain stride60/56 with tangent4/3. Binormal is identified and omitted because the original VS regenerates it. The private48B instance stream contains the three raw scene matrix rows. WVP is viewProjection only, because the native instanced VS has already transformed position. InstanceBatchHW's actual world-transform getter returns identity. Raw coordinates and the same captured origin remain separate.

Current instance ownership is proved through the SceneManager's bounded registered InstanceManager vector, batch creator/scene and unique member list, typed same-scene parent node, actual physical Town record, native in-use/manual-visible state and no skeleton. Ordinary Entity `_getManager` is not assumed for InstancedEntity: the native base constructor initializes it NULL and the instanced creation path does not establish that ordinary registration contract. Main-camera-dependent `isVisible` is not called.

Both PS variants preserve literal native dust/wet/gloss order and the native GBuffer encoding. Dual blending uses vertex alpha; colouring uses the actual ABGR RGB. Alpha48 clips the **normal texture's alpha**, then writes native translucency into normal GBuffer alpha. Threshold is copied from the typed native manual field, finite and less than1; no default is supplied. Required named fields fail closed, and current SharedWater values replace stale pass-autoconstant copies. PS binds7 or4 images at actual reflected t0..t6/t0..t3, all sampled with the copied unit0 sampler.

**Do not send Alpha48 through the current GST decoder/relighter.** It would drop native translucency by replacing normal alpha with coverage. `CanDecode()` rejects any packet containing alpha. An opaque-only mask1 capture can use the existing black-metalness decoder later. Alpha84 needs a separate approved native raw-GBuffer HDR stage; no approximation or atlas format change is introduced here. Preserve cached full-town fallback while adding any partial real-geometry layer. Restore pristine terrain and replay current models after removal/source changes; this helper does not erase old composed window pixels.

## Limits, ownership and performance

- At most256 records,64batches,16materials (the initial whitelist has5),4096 total registry/floor entries,4096 physical source entries plus4096 rereads, and4096 total batch-member source+reread entries. The initial member copy is bounded to2048 so its reread stays within4096. SceneManager registry membership is bounded4096; it is copied only during Read. Physical/member identity lookups use sorted copied segments and bounded binary search, not repeated native array walks.
- Per draw <=65536vertices/196608indices; total packet <=2097152vertices and2097152indices. The saved131-record subset totals1008567indices/vertices. No CPU mesh/texture payload copy. Count/refusal statistics are copied values. Unknown fields/unsupported states refuse, and limit truncation is explicit. A failed coherent capture publishes nothing and leaves caller fallback decisions outside this helper.
- Existing combined source budgets:32MiB retained buffers and64MiB retained textures, including the supplied other-owner charge. Packet deduplicates its own resource identities; summing charges across other owners can conservatively double-count shared sources. These are lifetime-extension budgets separate from owned surfaces.
- Owned raster payload1048848B = four256²4-byte surfaces +160B VS +64B PS +48B instance buffer. Every Init/Render checks the unchanged `AtlasView::GpuCap`. Callers count all concurrent/pending window/water/local/distant/decode/surface/private-town allocations. State/driver overhead is outside this declared payload, not an absolute whole-game GPU bound. Native capture work is statically <=2MiB and the maximum packet draw array <=256KiB; both bounds compile against the actual SDK. No per-frame or per-window native scans, no own timers/threads/IO, and no automatically raised cap.
- RGB mode adds exactly262144B for target0 (8 instead of4 bytes per sample), giving1310992B. Both InitRawRgb and every RGB Render include this charge under the same192MiB cap. Other surfaces/source budgets/work bounds stay the same. Render adds one mode-dependent budget choice and uses the existing draw loop; it creates no resources or native snapshots. External references retained after Raster Clear are the external owner's live payload and must remain charged there.
- Render updates the same48B instance stream per accepted draw and creates no resources. Foreign graphics/class/IA/OM/CB/sampler/SRV/scissor/viewport/predicate state is restored by the existing reviewed GraphicsState keeper, with an additional scoped save for PS slots1..6 and all-stage owned-output aliases. A nonempty SO binding or write alias refuses before clearing targets. Hidden OM append counts are retained. CPU/FPS and real scene lifetime balance still need measured runtime acceptance.

Unlike the private native Mesh loader, Snapshot retains no native resource owner after Read. Its later destruction only releases its own CPU/COM owners. Native/Ogre references live only in the transient capture, which is destroyed on success, exception, guarded memory fault or refusal while the serialized caller/renderer is alive. No new native teardown callback, shutdown hook or DLL hot-unload claim is made.

## Verification

Run `./run_tests.ps1` or bundled Python `run_tests.py`. Outputs are confined to `_generated`; `-SkipMutation`/`--skip-mutants` skips only negative controls. These are test-runner options, not production debug flags. Full actual adapter compilation uses the current SDK, VC10 `/O2 /GL`; it does not invoke the game. Native-source WARP oracle, compiled mutants and source/profile evidence are summarized in verification.md. Source/fixtures prove the selected offline cutpoint; they do not prove actual current Hub admission, resident native DXBC equality, whole-town coverage, final lighting parity, frame time or freedom from all runtime leaks.

`raw-rgb-verification.md/json` record the separate RGB addition without replacing the frozen packed baseline. The test-only oracle observes the pinned original shader's computed diffuse immediately before GBuffer writers on RGBA32F; the production RGB PS must match that result before storage, then RGBA16F is compared within one half-storage quantum plus2e-6 shader tolerance. Normal/translucency bytes and radial depth are compared exactly with the unmodified shader. Cases include all3 native declarations, dust/wetness/rotation, native alpha clipping, single-pixel dark/colored silhouettes at interior and texture border, and legitimate native RGB above1. Tests additionally cover state/hidden counters/predication, aliases, budget/refusal rollback, caller source release and repeated/partial initialization. These are offline WARP tests, not an installed feature or an in-game leak/FPS claim.

Black-metalness admission also checks sampler semantics: Texture2D BORDER with nonzero red is refused even when black.dds is verified. Zero-red borders and non-border modes preserve zero red sampling; all border components must be finite. The saved target materials use wrapping.
