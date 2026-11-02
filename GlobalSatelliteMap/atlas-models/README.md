# Registered DistantTown GBuffer prototype

This helper draws one **already registered** static DistantTown Entity with its
real existing geometry, native material texture and original unmodified native
DistantTown shaders. It creates no Entity, mesh, town, resource-manager entry,
world activation or substitute geometry. It is not connected to the active map.

The implementation contract is
`D:/DEV/Kenshi/research/gsm-building-render-20261102/report.md`; the scoped plan
is [plan.md](plan.md). Native candidate availability and game acceptance remain
root's later work. An absent/unsupported candidate returns failure; it is never
loaded or generated. TownList discovery/census belongs to root, not this helper.

## Interfaces

`NativeModels::Snapshot` in `NativeModel.h`:

```cpp
HRESULT Read(bool enabled, Town* resolvedLiveTown,
             Ogre::SceneManager* expectedCurrentScene,
             unsigned generation, bool modelSourcesVerified);
bool Valid(unsigned generation) const;
const AtlasModels::Packet& Geometry() const;
const float* LocalMeshBounds() const; // xmin,ymin,zmin,xmax,ymax,zmax
const char* LastError() const;
void Clear();
```

Call only at a serialized render-safe boundary, after native transforms are
ready, with a resolved live Town from the current world. The caller owns that
resolution and frame/world lifetime. Read performs a fresh admission, replacing
the prior snapshot; disabled Read releases only its own COM refs, performs no
profile lookup or world/native query and returns S_FALSE. Unverified source or
zero generation returns E_ACCESSDENIED before native admission. Other refusal
returns E_FAIL plus a bounded error string. Snapshot retains COM references and
copied values, never Town/Entity/node/mesh/material raw pointers after success.
It does not mark/unmark any Ogre resource or perform manager credit.

`modelSourcesVerified` must mean that **all three ModelSourcePins** emitted in
`_generated/ModelProfile.h` were verified against the worker's actual current
source resolution: distant_town.hlsl, distant_town.material and gbuffer.hlsl.
The existing terrain/water verification flag alone is not sufficient. Native
module/layout/code24 checks are additional admission gates, not file verification.
Root must wire those source pins and generation/cancellation before live use.

`AtlasModels::Packet` owns bounded source COM refs, copied input/draw metadata,
input layouts and rasterizer states. `RawSceneWorld()` is the source node's
cached full scene-space matrix. `SourceSceneOrigin()` is a separate copy of
getRelativeOrigin from the **same snapshot**. Neither accessor is labelled
absolute; no automatic origin addition occurs. This packet is a snapshot, not
an automatic town-change cache: consume within the admitted frame/request and
refresh explicitly for a later native update; generation equality alone does
not prove that a town's layout/material remained unchanged.

`AtlasModels::Raster` in `AtlasModel.h`:

```cpp
HRESULT Init(ID3D11Device*, const void* nativeVS, SIZE_T vsBytes,
             const void* nativePS, SIZE_T psBytes, U64 callerOwnedBytes);
HRESULT Render(bool enabled, const Packet&, const View&, U64 callerOwnedBytes);
ID3D11Texture2D* Texture(unsigned plane) const;        // borrowed
ID3D11ShaderResourceView* Output(unsigned plane) const; // borrowed
void Clear();
```

`Render(false,...)` returns S_FALSE before any D3D/world query, even if uninitialized.
Init should likewise be skipped while off. Init/Packet admission happen once per
explicit request, not on every UI tick. Outputs are private 256×256, one mip,
sample1:0=RGBA8 YCoCg/metal/gloss,1=RGBA8 encoded native normal,2=R32_FLOAT radial
depth. A private D32 depth attachment provides actual geometry occlusion. Normal
alpha remains native0; use nonzero valid radial depth for model coverage.
This first helper has no HDR lighting, exposure, terrain merge or viewer widget.

`TopDownView(rawX,rawCameraY,rawZ,width,height,near,far,generation,out)` creates a
right-handed downward view in the **same raw scene frame**. +X maps right and+Z
maps down; camera looks along−Y. Native shader world/WVP matrices are uploaded in
their reflected column-major layout. A caller may fit the copied local mesh
bounds after transforming its eight corners by RawSceneWorld to aim a standalone
diagnostic without a guessed Town→GST conversion. Town cached absolute X/Z
versus node/mesh/raw origin still needs a paired runtime proof before GST placement.
Do not invoke TownBase::getPosition to obtain that proof: its lazy height update
can write game state. Root has separately audited passive X/Z copies.

Root can use `nativeLight.device` if it is the same actual device as packet sources.
Init/Render refuse a different device, stale generation, invalid view, budget
overflow or aliases before clearing any private target. Returned output pointers
are borrowed; acquire a COM ref if retaining them and drop all uses before Clear.
Root remains responsible for world/load cancellation and actual renderer boundary.

## Native admission and ownership

Admission checks the actual EXE/Ogre/backend identities,53+ generated export
code24 windows (see current models.json for exact count), backend constructor/
getter/layout windows and exact Entity/Mesh/Material/SceneNode vtables. Town
proxy must have owner==Town, state3 and nonnull Entity. Entity manager and parent
SceneNode creator must match current au->render->scene. Existing typed Entity
registration is O(1): the native mGlobalIndex+160 must select that same Entity in
the existing const collection, with a sane count≤131072. No movable iterator
that creates a missing collection is called.

The source must be loaded, skeleton-free and vertex-animation-free, with a finite
positive-determinant affine cached matrix and finite local mesh bounds. Exactly
four supported native semantics (position,normal,colour,UV0),≤16 streams/draws,
≤65536 total vertices,≤196608 total indices and≤32MiB unique borrowed VB/IB bytes
are admitted. Counts/ranges/strides/offsets/device/actual buffer descriptions are
validated with overflow-safe arithmetic. Index values are trusted native model
contents, not copied or scanned on the CPU. Unsupported topology, declarations,
subset overrides, negative-scale winding or missing data refuse explicitly.

Use the read-only native SubMesh::_getRenderOperation(0), which writes only the
caller's operation, rather than SubEntity::getRenderOperation, which can change
shared IndexData and choose another camera's LOD. Backend borrowed-buffer leaves
288E0/243A0 are guarded by actual wrapper/implementation vtables and code windows.
COM references are acquired only after source/material/declaration admission;
no source buffer is locked, copied, reuploaded or modified.

All submeshes must use the same actual loaded DistantTown Material and diffuse
SRV identity: one technique/pass/unit, exact native program names, opaque solid
depth-check/write LEQUAL, full colour writes, no separate blending/alpha rejection/
alpha-to-coverage/depth bias. The existing passive TextureUnit frame and sampler
decoder are reused from NativeWater, without its active texture getter or any
water world snapshot. Actual current diffuse SRV and sampler options are used;
no filename lookup or black/flat fallback is performed. Runtime replacement of
a shader under the same identity is outside this audited native-source profile.

Static backend proof also establishes: native vertex colour types4/10/11 all
map to DXGI28; VET_FLOAT1/2/3/4 map41/16/6/2. Native rasterizer initialization
sets FrontCounterClockwise=TRUE. Cull1→NONE; CW2→BACK and CCW3→FRONT for the new
nonflipped target/noninverted positive basis. Native-source top-down WARP roofs
verify the positive-Y face is visible and reversed winding is culled.

## Graphics state and budgets

The keeper restores all touched IA state (layout,16 VB slots,strides/offsets,IB,
topology), VS/HS/DS/GS/PS with class instances, VS/PS CB0, PS SRV0/sampler0, RS
state/viewports/scissors, OM RTV/DSV/UAV/blend/depth state and predication. It
never resets UAV counters. All-stage read aliases of the private outputs are
temporarily unbound and restored. Writable aliases and source/output identities
refuse before writes. Active stream-output bindings refuse: D3D11 cannot query
their original offsets, so no guessed restoration or cursor reset is attempted.
Compute state is untouched except scoped output-read aliases. No Ogre shader,
viewport, OM cache or accounting field is edited.

Owned payload: three256²×4B targets +D32=1,048,576B, VS128B+PS16B constants,
total1,048,720B. Every check uses shared AtlasView::GpuCap, currently192MiB;
there is no local96MiB constant or cap increase. Current144-window/default
renderer189,006,000B +water3,408,256B +model1,048,720B =193,462,976B, leaving
7,863,616B below201,326,592B. Caller bytes must include all coexisting/pending/
retired owned resources. Shader/state/input-layout objects and driver overhead
are not an absolute GPU-memory measurement.

The≤32MiB source buffers and one existing diffuse resource are **borrowed native
retention**, separate from new owned allocations. The texture may retain its
native full resource/mips, not merely the sampled mip. No geometry CPU copy,
queue, worker, world scan or per-render heap allocation is introduced. Static
assertions bound snapshot/raw metadata below64KiB. Render state snapshots use
bounded stack arrays and scoped Get references. COM aliases are checked across
bounded slots; rendering at high frequency still needs game timing, not a claim
from WARP/static bounds.

## Build and evidence

Run `./run_tests.ps1` from this folder. It generates shaders/profile, checks the
actual source/DXBC contract, compiles/runs WARP and policy/production sequencing
tests, compiles the real native adapter against MSVC10/SDK7.1/KenshiLib, and rejects
17 independently compiled functional mutants. `-SkipMutation` omits only the
mutant runs; `-StateOnly` runs the initial graphics keeper fixture.

`generate_models.py` emits `_generated/ModelShaders.h` with arrays
`AtlasModelVertexShader` and `AtlasModelPixelShader`, plus `_generated/ModelProfile.h`.
Future root main-asset generation must call it before including NativeModel.h;
no existing generator/build file was edited. The original HLSL entry points are
compiled directly; reflection pins VS matrices0/64 (128B), PS camera0/far12 (16B),
texture/sampler0 and native output signature. `audit_backend.py` emits optional
offline ABI/disassembly and passive TownList evidence; it invokes no DLL/process.

Fresh results:8 graphics,32 source/DXBC/ABI contracts,15 policy,18 production
admission-route checks,450 WARP checks and17 failing compiled mutants. Native
GBuffer bytes compare exactly; R32 depth tolerance1e−6. Tests include real split
streams,16/32-bit indices,index prefix/baseVertex,colour formats,transformed and
top-down geometry,overlapping depth,positive/reversed roof faces,real dynamic PS
class instance,GS sentinel,all six stage output aliases,OM hidden appendcounter7,
false predicate,OFF/ON/OFF,validation/alias/SO refusals,source-owner release,repeated
initialization/clear,partial initialization and whole source COM-lifetime balance.
The double-retain mutant fails that balance test.

`verification.json` records hashes/results. Independent read-only source/native-state
review is CLEAR: trader_language_fallback freshly reran the full450-check WARP suite,
17 compiled mutants, all smaller suites and actual native adapter compilation;
reviewed passive ownership/COM/shape/state/projection without source edits. No
candidate was loaded/drawn in the game, and no main DLL was built,
installed or committed. This does not establish all-town/building coverage,
unloaded-world support,GST coordinate parity,dynamic model lighting,live performance
or the absence of every possible long-session leak.
